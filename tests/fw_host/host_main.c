/*
 * 宿主上跑固件的游戏层：读 assets.bin，按脚本按键、推进时间、截屏。
 * 每次截屏都会再整屏重画一遍，核对增量重画和整屏重画逐像素一致。
 * 脚本（stdin）每行一条：key up|down|ok|back、wait <ms>、shot <文件>、cmd <调试命令>、rtc <秒>、hold <ms>
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <unistd.h>

#include "nw_assets.h"
#include "nw_game.h"
#include "nw_platform.h"
#include "nw_sound.h"

static uint16_t fb[240 * 320];
static uint16_t strip[NW_STRIP_PX];
static unsigned char slots[2][64 * 1024];
static size_t slot_len[2];
static uint32_t rng = 12345;

uint16_t *nw_plat_strip(void) { return strip; }
void nw_plat_send(int x, int y, int w, int h, uint16_t *px)
{
    for (int r = 0; r < h; r++) memcpy(&fb[(y + r) * 240 + x], px + r * w, (size_t)w * 2);
}
void nw_plat_send_wait(void) {}
bool nw_plat_record_write(int slot, const void *d, size_t n)
{
    if (n > sizeof(slots[0])) return false;
    memcpy(slots[slot], d, n);
    slot_len[slot] = n;
    return true;
}
bool nw_plat_record_read(int slot, void *d, size_t n)
{
    if (slot_len[slot] != n) return false;
    memcpy(d, slots[slot], n);
    return true;
}
void nw_plat_cue(int c) { nw_sound_cue((nw_cue_t)c); }
uint32_t nw_plat_random(void) { rng = rng * 1103515245u + 12345u; return rng; }

/* 无线：两个宿主进程用本机 UDP 互通（环境变量 NW_PORT / NW_PEER），MAC 由端口号生成 */
static int s_sock = -1, s_port, s_peer;
static void port_mac(int port, uint8_t mac[6])
{
    uint8_t m[6] = {0x02, 0, 0, 0, (uint8_t)(port >> 8), (uint8_t)port};
    memcpy(mac, m, 6);
}
void nw_plat_mac(uint8_t mac[6]) { port_mac(s_port, mac); }
bool nw_plat_radio_on(void)
{
    const char *p = getenv("NW_PORT"), *q = getenv("NW_PEER");
    if (!p || !q) return false;
    s_port = atoi(p);
    s_peer = atoi(q);
    s_sock = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in a = {0};
    a.sin_family = AF_INET;
    a.sin_port = htons((uint16_t)s_port);
    a.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    int one = 1;
    setsockopt(s_sock, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
    if (bind(s_sock, (struct sockaddr *)&a, sizeof(a)) < 0) return false;
    fcntl(s_sock, F_SETFL, O_NONBLOCK);
    return true;
}
void nw_plat_radio_off(void)
{
    if (s_sock >= 0) close(s_sock);
    s_sock = -1;
}
bool nw_plat_radio_send(const uint8_t *mac, const void *data, int len)
{
    uint8_t peer[6];
    port_mac(s_peer, peer);
    if (mac && memcmp(mac, peer, 6)) return false;
    struct sockaddr_in a = {0};
    a.sin_family = AF_INET;
    a.sin_port = htons((uint16_t)s_peer);
    a.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    return sendto(s_sock, data, (size_t)len, 0, (struct sockaddr *)&a, sizeof(a)) == len || !mac;
}
int nw_plat_radio_recv(uint8_t mac[6], void *buf, int max)
{
    struct sockaddr_in a;
    socklen_t al = sizeof(a);
    if (s_sock < 0) return 0;
    ssize_t n = recvfrom(s_sock, buf, (size_t)max, 0, (struct sockaddr *)&a, &al);
    if (n <= 0) return 0;
    port_mac(ntohs(a.sin_port), mac);
    return (int)n;
}

static int s_realtime;
static void tick(uint32_t *now, uint32_t rtc)
{
    *now += 33;
    nw_game_frame(*now, rtc);
    if (s_realtime) usleep(33000);
}

int main(int argc, char **argv)
{
    if (argc < 2) return 2;
    FILE *f = fopen(argv[1], "rb");
    if (!f) return 2;
    fseek(f, 0, SEEK_END);
    long n = ftell(f);
    fseek(f, 0, SEEK_SET);
    unsigned char *a = malloc((size_t)n);
    if (fread(a, 1, (size_t)n, f) != (size_t)n) return 2;
    fclose(f);
    if (!nw_assets_open(a, (size_t)n)) { fprintf(stderr, "assets bad\n"); return 3; }
    uint32_t now = 1000, rtc = 100000;
    s_realtime = getenv("NW_REALTIME") != NULL;
    if (!nw_game_open(now, rtc, true)) { fprintf(stderr, "open failed\n"); return 4; }
    char line[256], out[512];
    int bad = 0;
    while (fgets(line, sizeof(line), stdin)) {
        line[strcspn(line, "\r\n")] = 0;
        if (!strncmp(line, "key ", 4)) {
            const char *k = line + 4;
            if (!strcmp(k, "up")) { nw_game_key(NW_KEY_UP_PRESS, now); nw_game_key(NW_KEY_UP_RELEASE, now + 50); }
            else if (!strcmp(k, "down")) nw_game_key(NW_KEY_DOWN, now);
            else if (!strcmp(k, "ok")) nw_game_key(NW_KEY_OK, now);
            else if (!strcmp(k, "back")) {
                nw_game_key(NW_KEY_UP_PRESS, now);
                for (int t = 0; t < 1100; t += 33) tick(&now, rtc);
                nw_game_key(NW_KEY_UP_RELEASE, now);
            }
            tick(&now, rtc);
        } else if (!strncmp(line, "wait ", 5)) {
            int ms = atoi(line + 5);
            for (int t = 0; t < ms; t += 33) tick(&now, rtc);
        } else if (!strncmp(line, "rtc ", 4)) {
            rtc = (uint32_t)atol(line + 4);
        } else if (!strncmp(line, "cmd ", 4)) {
            nw_game_command(line + 4, out, sizeof(out), now, rtc);
            printf("%s\n", out);
            now += 33;
            nw_game_frame(now, rtc);
        } else if (!strncmp(line, "shot ", 5)) {
            static uint16_t inc[240 * 320];
            memcpy(inc, fb, sizeof(fb));
            /* 同一时刻整屏重画，核对和增量结果一致 */
            nw_game_command("status", out, sizeof(out), now, rtc);
            extern void nw_test_full_redraw(uint32_t now, uint32_t rtc);
            nw_test_full_redraw(now, rtc);
            if (memcmp(inc, fb, sizeof(fb))) {
                int diff = 0;
                for (int i = 0; i < 240 * 320; i++) diff += inc[i] != fb[i];
                printf("MISMATCH %s: %d px differ\n", line + 5, diff);
                bad++;
            }
            if (nw_game_overflow()) { printf("OVERFLOW %s\n", line + 5); bad++; }
            FILE *o = fopen(line + 5, "wb");
            fwrite(fb, 2, 240 * 320, o);
            fclose(o);
            printf("shot %s scene=%s\n", line + 5, nw_game_scene_name());
        }
    }
    return bad ? 1 : 0;
}
