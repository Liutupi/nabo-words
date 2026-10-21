/*
 * 对战（ESP-NOW 直连，DESIGN.md 第 6 节）：搜索附近的工牌、邀请 / 接受、交换学过的词、出题、抢答、核对比分。
 * 挑战方（host）用两人都学过的词出题，把 10 道题整套发给对方（START），两边显示的题完全一样；之后只交换答案和用时。
 * 计分、积分、段位、防刷分、错词排进复习都用 words/nabo_words_battle.c。
 */
#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "words/nabo_words_battle.h"

#define NW_VS_PEERS 6
#define NW_VS_CITY 24

typedef enum {
    VS_SEARCH,      /* 广播 HELLO，列出附近的工牌 */
    VS_INVITING,    /* 我发了邀请，等回复 */
    VS_INVITED,     /* 收到邀请，等我决定 */
    VS_SYNC,        /* 交换学过的词、出题 */
    VS_COUNT,       /* 3 秒倒计时 */
    VS_Q,           /* 答题 */
    VS_REVEAL,      /* 揭晓这一题 */
    VS_WAIT,        /* 10 题做完，等对方的比分核对 */
    VS_END,         /* 结算页 */
} nw_vs_phase_t;

typedef struct {
    uint8_t mac[6];
    int16_t rating;
    uint16_t mastered;
    uint8_t stage, busy;
    char city[NW_VS_CITY];
    uint32_t seen;
} nw_vs_peer_t;

typedef struct {
    nw_vs_phase_t phase;
    uint32_t t0;                 /* 进入当前阶段的时刻 */
    bool radio_ok;
    uint32_t hello_at;
    nw_vs_peer_t peers[NW_VS_PEERS];
    int n_peers, sel;
    nw_vs_peer_t opp;
    uint32_t opp_seen;           /* 最近一次收到对手的包 */
    bool host;
    uint8_t bitmap_parts;        /* 收到对方位图的哪几片 */
    uint8_t peer_bitmap[NW_BITMAP_BYTES];
    uint8_t sync_tries;
    uint32_t sync_at;
    int pool;
    nw_question_t qs[NW_BATTLE_Q];
    int qi;
    int8_t my_pick;              /* -1 还没答；3 超时 */
    uint16_t my_ms;
    int8_t op_pick[NW_BATTLE_Q]; /* -2 还没收到；3 超时 */
    uint16_t op_ms[NW_BATTLE_Q];
    int score[2], gain[2];
    uint16_t missed[NW_BATTLE_Q];
    int n_missed;
    bool got_result;
    uint16_t peer_result[2];     /* 对方算的：它的分、我的分 */
    bool counted;                /* 比分核对一致，计入积分 */
    nw_battle_result_t res;
    char msg[64];                /* 结算页的附加说明（断线、对方退出……） */
} nw_vs_t;

/* 由 nw_game.c 调用 */
bool nw_vs_enter(void);           /* 打开无线、开始搜索；内存不足返回 false */
void nw_vs_leave(void);           /* 发 BYE、关无线、释放 */
void nw_vs_key(int key);          /* nw_key_t：上 / 下 / OK */
void nw_vs_update(void);
void nw_draw_vs(void);
void nw_vs_name(const nw_vs_peer_t *p, char *out, int n);
