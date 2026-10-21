/*
 * 游戏层（nw_game.c 及画面）需要宿主提供的几样东西。固件里由 main.c 实现，
 * 宿主测试里由 tests/fw_host/host_platform.c 实现，所以游戏代码不依赖 ESP-IDF。
 */
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* 送屏用的条带缓冲：NW_STRIP_PX 个像素。固件里有两块轮流用，拿到的那块保证已经传完 */
#define NW_STRIP_ROWS 16
#define NW_STRIP_PX (240 * NW_STRIP_ROWS)
uint16_t *nw_plat_strip(void);
/* 把屏幕 [x, x+w) × [y, y+h) 的像素送到屏幕；px 来自 nw_plat_strip()，本机字节序 RGB565，
 * 函数内部负责换字节序并排队传输，传完之前不能再写这块缓冲。 */
void nw_plat_send(int x, int y, int w, int h, uint16_t *px);
/* 等前面送出的像素全部传完 */
void nw_plat_send_wait(void);

/* 学习记录：两个槽轮流写（slot 0 / 1），序号和 CRC 由 nw_store.c 处理 */
bool nw_plat_record_write(int slot, const void *data, size_t size);
bool nw_plat_record_read(int slot, void *data, size_t size);

/* 声音提示，见 nw_sound.h */
void nw_plat_cue(int cue);

uint32_t nw_plat_random(void);

/* ---------- 无线（对战用 ESP-NOW，只在对战画面打开） ---------- */
#define NW_RADIO_MAX 250
bool nw_plat_radio_on(void);
void nw_plat_radio_off(void);
/* mac 为 NULL 时广播。单播会等对方的链路层确认，失败重试几次；返回是否送达（广播总是 true） */
bool nw_plat_radio_send(const uint8_t *mac, const void *data, int len);
/* 取一个收到的包，没有返回 0 */
int nw_plat_radio_recv(uint8_t mac[6], void *buf, int max);
void nw_plat_mac(uint8_t mac[6]);
