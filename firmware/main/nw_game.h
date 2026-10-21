/*
 * 《NABO 单词护照》游戏层：画面状态机、按键、学习时钟、保存。对应原型 prototype/app.js。
 * 不依赖 ESP-IDF：时间、送屏、保存、声音都通过 nw_platform.h 由宿主提供。
 * 所有 nw_game_* 函数都必须在同一个任务里调用（固件里是 main.c 的游戏任务）。
 */
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum {
    NW_KEY_UP_PRESS,     /* 上键按下（松开前 1 秒算长按：返回） */
    NW_KEY_UP_RELEASE,
    NW_KEY_DOWN,         /* 下键按下 */
    NW_KEY_OK,           /* OK 按下 */
} nw_key_t;

/* assets 已用 nw_assets_open() 打开。rtc_ok=false 表示断过电、RTC 时间不可信 */
bool nw_game_open(uint32_t now_ms, uint32_t rtc_s, bool rtc_ok);
void nw_game_key(nw_key_t key, uint32_t now_ms);
/* 推进一帧并送屏；rtc_s 用于学习时钟 */
void nw_game_frame(uint32_t now_ms, uint32_t rtc_s);
/* 有未保存的改动就保存（休眠前调用） */
void nw_game_save(void);
bool nw_game_muted(void);
/* 串口调试命令（见 nw_game.c 的 nw_game_command），结果写进 out */
void nw_game_command(const char *line, char *out, size_t n, uint32_t now_ms, uint32_t rtc_s);
/* 宿主测试：当前画面名、绘制列表是否溢出 */
const char *nw_game_scene_name(void);
bool nw_game_overflow(void);
