// main/main.c — 《NABO 单词护照》AI Passport 独立固件：启动、按键、送屏、声音、保存、休眠。
//
// 开机直接进主页，不需要配网或激活，也不联网。
//   上键  单击：上一项；按住 1 秒：返回上一级
//   下键  下一项
//   OK    确认
// 5 分钟无操作先保存再深睡，任意键唤醒（重新启动回到主页）。
// 串口（USB）调试命令：每行一条，见 nw_game.c 的 nw_game_command（status、key、days、sim……）。
#include <stdatomic.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>

#include "bsp_audio.h"
#include "bsp_battery.h"
#include "bsp_button.h"
#include "bsp_display.h"
#include "bsp_i2c.h"
#include "bsp_pins.h"
#include "driver/usb_serial_jtag.h"
#include "driver/usb_serial_jtag_vfs.h"
#include "esp_heap_caps.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_now.h"
#include "esp_wifi.h"
#include "esp_partition.h"
#include "esp_random.h"
#include "esp_sleep.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "nw_assets.h"
#include "nw_game.h"
#include "nw_platform.h"
#include "nw_sound.h"

static const char *TAG = "words";

#define SAMPLE_RATE 16000
#define CHUNK 320                       // 20 ms
#define VOLUME 70
#define IDLE_SLEEP_US (5LL * 60 * 1000 * 1000)
#define BACKLIGHT 85
#define FRAME_MS 33
#define SLOT_SIZE 0x8000

static esp_lcd_panel_handle_t s_panel;
static esp_lcd_panel_io_handle_t s_io;
static uint16_t *s_strip[2];
static int s_which;
static SemaphoreHandle_t s_free;        // 空闲的条带缓冲个数
static QueueHandle_t s_keys;
static _Atomic int64_t s_last_input_us;
static atomic_bool s_sleep_now;
static bool s_audio_ok;
static const esp_partition_t *s_record;

typedef struct {
    uint8_t key;
    uint32_t ms;
} key_ev_t;

static uint32_t now_ms(void) { return (uint32_t)(esp_timer_get_time() / 1000); }

static uint32_t rtc_s(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint32_t)tv.tv_sec;
}

/* ---------- 平台接口（nw_platform.h） ---------- */

static bool on_trans_done(esp_lcd_panel_io_handle_t io, esp_lcd_panel_io_event_data_t *ed, void *ctx)
{
    BaseType_t woken = pdFALSE;
    xSemaphoreGiveFromISR(s_free, &woken);
    return woken == pdTRUE;
}

uint16_t *nw_plat_strip(void)
{
    xSemaphoreTake(s_free, portMAX_DELAY);
    uint16_t *b = s_strip[s_which];
    s_which ^= 1;
    return b;
}

void nw_plat_send(int x, int y, int w, int h, uint16_t *px)
{
    int n = w * h;
    for (int i = 0; i < n; i++) px[i] = (uint16_t)(px[i] << 8 | px[i] >> 8);
    if (esp_lcd_panel_draw_bitmap(s_panel, x, y, x + w, y + h, px) != ESP_OK) {
        xSemaphoreGive(s_free);   // 没送出去，也就不会有完成回调
    }
}

void nw_plat_send_wait(void)
{
    // 两块缓冲都回来了，说明前面的传输都完成了
    xSemaphoreTake(s_free, portMAX_DELAY);
    xSemaphoreTake(s_free, portMAX_DELAY);
    xSemaphoreGive(s_free);
    xSemaphoreGive(s_free);
}

bool nw_plat_record_write(int slot, const void *data, size_t size)
{
    if (!s_record || size > SLOT_SIZE) return false;
    size_t off = (size_t)slot * SLOT_SIZE, erase = (size + 4095) & ~4095u;
    if (esp_partition_erase_range(s_record, off, erase) != ESP_OK) return false;
    if (esp_partition_write(s_record, off, data, size) != ESP_OK) return false;
    ESP_LOGI(TAG, "saved %u bytes to slot %d", (unsigned)size, slot);
    return true;
}

bool nw_plat_record_read(int slot, void *data, size_t size)
{
    if (!s_record || size > SLOT_SIZE) return false;
    return esp_partition_read(s_record, (size_t)slot * SLOT_SIZE, data, size) == ESP_OK;
}

void nw_plat_cue(int cue) { nw_sound_cue((nw_cue_t)cue); }

uint32_t nw_plat_random(void) { return esp_random(); }

/* ---------- 无线：ESP-NOW，只在对战画面打开（Wi-Fi 要占 50 KB 以上内存） ---------- */

#define RADIO_CHANNEL 1

typedef struct {
    uint8_t mac[6];
    uint8_t len;
    uint8_t data[NW_RADIO_MAX];
} radio_pkt_t;

static QueueHandle_t s_rx;
static SemaphoreHandle_t s_tx_done;
static volatile bool s_tx_ok;
static bool s_radio;
static const uint8_t BCAST[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

static void radio_rx(const esp_now_recv_info_t *info, const uint8_t *data, int len)
{
    if (len <= 0 || len > NW_RADIO_MAX) return;
    radio_pkt_t p;
    memcpy(p.mac, info->src_addr, 6);
    p.len = (uint8_t)len;
    memcpy(p.data, data, (size_t)len);
    xQueueSend(s_rx, &p, 0);
}

static void radio_tx(const esp_now_send_info_t *info, esp_now_send_status_t status)
{
    s_tx_ok = status == ESP_NOW_SEND_SUCCESS;
    xSemaphoreGive(s_tx_done);
}

bool nw_plat_radio_on(void)
{
    if (s_radio) return true;
    static bool loop;
    if (!loop) {
        esp_err_t e = esp_event_loop_create_default();
        loop = e == ESP_OK || e == ESP_ERR_INVALID_STATE;
        s_rx = xQueueCreate(12, sizeof(radio_pkt_t));
        s_tx_done = xSemaphoreCreateBinary();
    }
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    if (esp_wifi_init(&cfg) != ESP_OK) return false;
    bool ok = esp_wifi_set_storage(WIFI_STORAGE_RAM) == ESP_OK && esp_wifi_set_mode(WIFI_MODE_STA) == ESP_OK &&
              esp_wifi_start() == ESP_OK && esp_wifi_set_channel(RADIO_CHANNEL, WIFI_SECOND_CHAN_NONE) == ESP_OK &&
              esp_now_init() == ESP_OK;
    if (ok) {
        esp_now_register_recv_cb(radio_rx);
        esp_now_register_send_cb(radio_tx);
        esp_now_peer_info_t peer = {0};
        memcpy(peer.peer_addr, BCAST, 6);
        peer.channel = RADIO_CHANNEL;
        peer.ifidx = WIFI_IF_STA;
        ok = esp_now_add_peer(&peer) == ESP_OK;
    }
    if (!ok) {
        esp_now_deinit();
        esp_wifi_stop();
        esp_wifi_deinit();
        return false;
    }
    xQueueReset(s_rx);
    s_radio = true;
    ESP_LOGI(TAG, "radio on, free heap %u", (unsigned)esp_get_free_heap_size());
    return true;
}

void nw_plat_radio_off(void)
{
    if (!s_radio) return;
    esp_now_deinit();
    esp_wifi_stop();
    esp_wifi_deinit();
    s_radio = false;
    ESP_LOGI(TAG, "radio off, free heap %u", (unsigned)esp_get_free_heap_size());
}

bool nw_plat_radio_send(const uint8_t *mac, const void *data, int len)
{
    if (!s_radio || len > NW_RADIO_MAX) return false;
    const uint8_t *to = mac ? mac : BCAST;
    if (mac && !esp_now_is_peer_exist(mac)) {
        esp_now_peer_info_t peer = {0};
        memcpy(peer.peer_addr, mac, 6);
        peer.channel = RADIO_CHANNEL;
        peer.ifidx = WIFI_IF_STA;
        if (esp_now_add_peer(&peer) != ESP_OK) return false;
    }
    for (int tries = 0; tries < (mac ? 4 : 1); tries++) {
        xSemaphoreTake(s_tx_done, 0);
        if (esp_now_send(to, data, (size_t)len) != ESP_OK) continue;
        if (xSemaphoreTake(s_tx_done, pdMS_TO_TICKS(60)) == pdTRUE && (s_tx_ok || !mac)) return true;
    }
    return !mac;
}

int nw_plat_radio_recv(uint8_t mac[6], void *buf, int max)
{
    radio_pkt_t p;
    if (!s_rx || xQueueReceive(s_rx, &p, 0) != pdTRUE) return 0;
    memcpy(mac, p.mac, 6);
    int n = p.len < max ? p.len : max;
    memcpy(buf, p.data, (size_t)n);
    return n;
}

void nw_plat_mac(uint8_t mac[6]) { esp_read_mac(mac, ESP_MAC_WIFI_STA); }

/* ---------- 按键（esp_timer 任务里，只入队） ---------- */

static void push_key(nw_key_t k)
{
    key_ev_t ev = {(uint8_t)k, now_ms()};
    xQueueSend(s_keys, &ev, 0);
}

static void on_button(bsp_btn_t btn, bsp_btn_ev_t ev, void *user)
{
    atomic_store(&s_last_input_us, esp_timer_get_time());
    if (btn == BSP_BTN_UP) {
        if (ev == BSP_BTN_PRESS) push_key(NW_KEY_UP_PRESS);
        else if (ev == BSP_BTN_RELEASE) push_key(NW_KEY_UP_RELEASE);
    } else if (btn == BSP_BTN_DOWN && ev == BSP_BTN_PRESS) {
        push_key(NW_KEY_DOWN);
    } else if (btn == BSP_BTN_OK && ev == BSP_BTN_PRESS) {
        push_key(NW_KEY_OK);
    }
}

/* ---------- 深睡 ---------- */

static void warn(const char *what, esp_err_t err)
{
    if (err != ESP_OK) ESP_LOGW(TAG, "%s: %s", what, esp_err_to_name(err));
}

// 由音频任务调用（它独占编解码器）；游戏任务这时已经存过档并停下
static void enter_deep_sleep(void)
{
    ESP_LOGI(TAG, "idle, entering deep sleep");
    bsp_display_backlight(0);
    int level = 0;
    if (bsp_button_prepare_deep_sleep(&level) != ESP_OK) esp_restart();
    warn("gpio wake", esp_deep_sleep_enable_gpio_wakeup(1ULL << BSP_BTN_GPIO, ESP_GPIO_WAKEUP_GPIO_LOW));
    warn("battery sleep", bsp_battery_sleep());
    if (s_audio_ok) {
        warn("audio sleep", bsp_audio_sleep());
        warn("i2s release", bsp_audio_prepare_deep_sleep());
    }
    warn("i2c release", bsp_i2c_prepare_deep_sleep());
    warn("display sleep", bsp_display_prepare_deep_sleep());
    esp_deep_sleep_start();
    esp_restart();
}

/* ---------- 音频 ---------- */

static void audio_task(void *arg)
{
    static int16_t pcm[CHUNK];
    bool muted = nw_game_muted();
    if (s_audio_ok) bsp_audio_set_volume(muted ? 0 : VOLUME);
    for (;;) {
        if (atomic_load(&s_sleep_now)) enter_deep_sleep();
        bool m = nw_game_muted();
        if (m != muted && s_audio_ok) bsp_audio_set_volume(m ? 0 : VOLUME);
        muted = m;
        memset(pcm, 0, sizeof(pcm));
        nw_sound_fill(pcm, CHUNK);
        if (!s_audio_ok || bsp_audio_write(pcm, sizeof(pcm)) != ESP_OK) vTaskDelay(pdMS_TO_TICKS(20));
    }
}

/* ---------- 游戏任务 ---------- */

static void console_poll(void)
{
    static char line[96];
    static int len;
    uint8_t buf[32];
    int n = usb_serial_jtag_read_bytes(buf, sizeof(buf), 0);
    for (int i = 0; i < n; i++) {
        char c = (char)buf[i];
        if (c == '\r' || c == '\n') {
            if (len) {
                line[len] = 0;
                static char out[400];
                const char *cmd = line[0] == '>' ? line + 1 : line;
                if (!strncmp(cmd, "nw ", 3)) cmd += 3;
                nw_game_command(cmd, out, sizeof(out), now_ms(), rtc_s());
                printf("NW %s heap=%u largest=%u\n", out, (unsigned)esp_get_free_heap_size(),
                       (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_8BIT));
                atomic_store(&s_last_input_us, esp_timer_get_time());
            }
            len = 0;
        } else if (len < (int)sizeof(line) - 1) {
            line[len++] = c;
        }
    }
}

static void game_task(void *arg)
{
    TickType_t wake = xTaskGetTickCount();
    for (;;) {
        key_ev_t ev;
        while (xQueueReceive(s_keys, &ev, 0) == pdTRUE) nw_game_key((nw_key_t)ev.key, ev.ms);
        console_poll();
        nw_game_frame(now_ms(), rtc_s());
        if (esp_timer_get_time() - atomic_load(&s_last_input_us) > IDLE_SLEEP_US) {
            nw_game_save();
            nw_plat_send_wait();
            atomic_store(&s_sleep_now, true);
            vTaskSuspend(NULL);
        }
        vTaskDelayUntil(&wake, pdMS_TO_TICKS(FRAME_MS));
    }
}

/* ---------- 启动 ---------- */

static bool map_assets(void)
{
    const esp_partition_t *p = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, 0x41, "assets");
    if (!p) return false;
    uint8_t head[8];
    if (esp_partition_read(p, 0, head, sizeof(head)) != ESP_OK || memcmp(head, "NWA1", 4) != 0) return false;
    uint32_t n = nw_rd32(head + 4), end = 0;
    for (uint32_t i = 0; i < n && i < 64; i++) {
        uint8_t e[32];
        if (esp_partition_read(p, 8 + 32 * i, e, sizeof(e)) != ESP_OK) return false;
        uint32_t stop = nw_rd32(e + 24) + nw_rd32(e + 28);
        if (stop > end) end = stop;
    }
    if (!end || end > p->size) return false;
    const void *ptr = NULL;
    esp_partition_mmap_handle_t h;
    uint32_t len = (end + 0xFFFF) & ~0xFFFFu;
    if (len > p->size) len = p->size;
    if (esp_partition_mmap(p, 0, len, ESP_PARTITION_MMAP_DATA, &ptr, &h) != ESP_OK) {
        return false;
    }
    ESP_LOGI(TAG, "assets %u bytes mapped", (unsigned)end);
    return nw_assets_open(ptr, end);
}

static void fatal_screen(void)
{
    // 资源没刷进去：整屏红色，背光亮起来让人知道
    uint16_t *b = s_strip[0];
    for (int i = 0; i < NW_STRIP_PX; i++) b[i] = 0x00F8;   // 红色（大端）
    for (int y = 0; y < 320; y += NW_STRIP_ROWS) {
        esp_lcd_panel_draw_bitmap(s_panel, 0, y, 240, y + NW_STRIP_ROWS, b);
        esp_lcd_panel_io_tx_param(s_io, 0x00, NULL, 0);   // 等这一条传完
    }
    bsp_display_backlight(BACKLIGHT);
}

void app_main(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        err = nvs_flash_init();
    }
    warn("nvs", err);

    usb_serial_jtag_driver_config_t uc = USB_SERIAL_JTAG_DRIVER_CONFIG_DEFAULT();
    if (usb_serial_jtag_driver_install(&uc) == ESP_OK) usb_serial_jtag_vfs_use_driver();

    ESP_ERROR_CHECK(bsp_display_init());
    s_panel = bsp_display_panel();
    s_io = bsp_display_io();
    s_free = xSemaphoreCreateCounting(2, 2);
    for (int i = 0; i < 2; i++) {
        s_strip[i] = heap_caps_malloc(NW_STRIP_PX * 2, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
        ESP_ERROR_CHECK(s_strip[i] ? ESP_OK : ESP_ERR_NO_MEM);
    }
    esp_lcd_panel_io_callbacks_t cbs = {.on_color_trans_done = on_trans_done};
    ESP_ERROR_CHECK(esp_lcd_panel_io_register_event_callbacks(s_io, &cbs, NULL));

    s_record = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, 0x40, "record");
    warn("battery", bsp_battery_init());
    s_audio_ok = bsp_audio_init() == ESP_OK && bsp_audio_set_format(SAMPLE_RATE, 16, 1) == ESP_OK;
    if (!s_audio_ok) ESP_LOGE(TAG, "audio unavailable, running silent");

    esp_reset_reason_t why = esp_reset_reason();
    // 深睡、软件重启、看门狗都不断电，RTC 时间可信；上电、掉电复位后时间从 0 开始
    bool rtc_ok = why == ESP_RST_DEEPSLEEP || why == ESP_RST_SW || why == ESP_RST_PANIC || why == ESP_RST_INT_WDT ||
                  why == ESP_RST_TASK_WDT || why == ESP_RST_WDT;
    ESP_LOGI(TAG, "reset reason %d, rtc %s, t=%u", (int)why, rtc_ok ? "kept" : "unknown", (unsigned)rtc_s());

    if (!map_assets() || !nw_game_open(now_ms(), rtc_s(), rtc_ok)) {
        ESP_LOGE(TAG, "assets missing or broken: flash assets.bin to the assets partition");
        fatal_screen();
        for (;;) vTaskDelay(pdMS_TO_TICKS(1000));
    }
    atomic_store(&s_last_input_us, esp_timer_get_time());
    s_keys = xQueueCreate(16, sizeof(key_ev_t));
    nw_game_frame(now_ms(), rtc_s());
    nw_plat_send_wait();
    bsp_display_backlight(BACKLIGHT);

    xTaskCreate(game_task, "nw_game", 8192, NULL, 6, NULL);
    xTaskCreate(audio_task, "nw_audio", 4096, NULL, 5, NULL);
    ESP_ERROR_CHECK(bsp_button_init(on_button, NULL));
    ESP_LOGI(TAG, "NABO words ready, free heap %u, largest %u", (unsigned)esp_get_free_heap_size(),
             (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_8BIT));
}
