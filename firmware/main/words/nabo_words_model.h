/*
 * 《NABO 单词护照》纯逻辑：记忆盒、每日题单、学习日与打卡、结业考、成就、小卖部。
 * 不依赖屏幕、声音和 RTOS；“今天是第几个学习日”由调用方推进。
 * 规则的标准答案是 prototype/core.js，改规则先改原型，再重录 tests/words_host 的对拍轨迹。
 */
#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "nabo_words_db.h"
#include "nabo_words_rng.h"

#define NW_UNIT          50
#define NW_STAGES        6
#define NW_UNITS         (NW_TOTAL / NW_UNIT)

#define NW_MAX_BOX       7
#define NW_FAMILIAR      3
#define NW_MASTERED      4
#define NW_GRADUATED     7
#define NW_NEVER         0xFFFFu   /* 毕业词不再排期（原型里是 1e9） */

#define NW_MAINT_PER_DAY 3
#define NW_BACKLOG_SOFT  20
#define NW_BUDGET_MIN_S  300
#define NW_BUDGET_PER_NEW_S 30
#define NW_TENTHS_INTRO  40
#define NW_TENTHS_CHECK  40

#define NW_EXAM_Q        20
#define NW_EXAM_PASS     16
#define NW_EXAM_UNLOCK_PCT 90
#define NW_HATCH_PCT     10
#define NW_TEEN_PCT      45

#define NW_FREEZE_EVERY  7
#define NW_FREEZE_MAX    2
#define NW_FREEZE_PRICE  20
#define NW_ACC_PRICE     40
#define NW_TOYS_PER_STAGE 10
#define NW_TOYS          (NW_TOYS_PER_STAGE * NW_STAGES)
#define NW_ACCS          8
#define NW_ACC_NONE      0xFF

#define NW_NEW_PER_DAY_MAX 20
/* 预算最多 max(300, 30×20) = 600 秒，最快的题 2.5 秒，所以一天最多 240 道复习 */
#define NW_PLAN_MAX      256
#define NW_VS_SLOTS      8

extern const uint16_t nw_stage_start[NW_STAGES];
extern const uint16_t nw_stage_end[NW_STAGES];
extern const uint8_t nw_toy_price[NW_STAGES];

typedef struct {
    uint8_t mac[6];
    uint8_t games;          /* 今天和这台徽章打了几局（防刷分） */
} nw_vs_slot_t;

typedef struct {
    uint16_t day;           /* 第几个学习日，从 1 开始 */
    uint16_t done_day;      /* 最近完成今日学习的那一天，0 表示从没完成过 */
    uint16_t next;          /* 下一个新词的序号 */
    uint16_t streak, best_streak;
    uint8_t stage;          /* 已解锁的学段，NW_STAGES 表示全部完成 */
    uint8_t new_per_day;
    uint8_t freezes;
    uint8_t toy_count;
    uint8_t toys[NW_TOYS];  /* 按开到的顺序，书架摆最后几个 */
    uint8_t accs;           /* 已有饰品的位图 */
    uint8_t acc;            /* 戴着的饰品，NW_ACC_NONE 表示不戴 */
    uint8_t achievements;   /* 位图，见 nw_achievement_t */
    uint8_t exam_first;     /* 第一次结业考：0 没考过，1 通过，2 没过 */
    uint32_t coins, spent;
    int16_t rating;
    uint16_t wins, losses, win_streak;
    nw_vs_slot_t vs[NW_VS_SLOTS];
    uint8_t box[NW_TOTAL];
    uint16_t due[NW_TOTAL];
} nw_state_t;

typedef enum {
    NW_ACH_W100 = 1 << 0, NW_ACH_W1000 = 1 << 1, NW_ACH_S7 = 1 << 2, NW_ACH_S30 = 1 << 3,
    NW_ACH_STAMP1 = 1 << 4, NW_ACH_EXAM1 = 1 << 5, NW_ACH_VS10 = 1 << 6,
} nw_achievement_t;

typedef struct {
    uint16_t reviews[NW_PLAN_MAX];
    uint16_t n_reviews;
    uint16_t fresh[NW_NEW_PER_DAY_MAX];
    uint8_t n_fresh;
    uint16_t backlog;       /* 到期但排不进今天的复习 */
    uint16_t seconds;       /* 预计总用时（秒） */
} nw_plan_t;

/* 排题单要两块临时数组（到期词、毕业词），各 NW_TOTAL 个；由调用方提供，用完就能释放 */
typedef struct {
    uint16_t a[NW_TOTAL];
    uint16_t b[NW_TOTAL];
} nw_work_t;

void nw_state_init(nw_state_t *s);

int nw_count_box(const nw_state_t *s, int from, int to, int min_box);
int nw_mastered(const nw_state_t *s);
int nw_learned(const nw_state_t *s);
bool nw_unit_stamped(const nw_state_t *s, int unit);
int nw_stamp_count(const nw_state_t *s);
bool nw_all_done(const nw_state_t *s);
bool nw_exam_ready(const nw_state_t *s);
int nw_sprite_form(const nw_state_t *s, int stage);   /* 0 蛋 1 幼崽 2 少年 3 完全体 */

int nw_review_budget_s(const nw_state_t *s);
int nw_review_tenths(int box);
void nw_plan_today(const nw_state_t *s, nw_plan_t *plan, nw_work_t *work);

void nw_grade(nw_state_t *s, uint16_t idx, bool correct, bool first);
void nw_introduce(nw_state_t *s, uint16_t idx);

/* 完成今日学习；返回值 bit0 = 这次得到了补签卡。*gained 返回新得到的成就位图 */
int nw_finish_day(nw_state_t *s, bool perfect, uint8_t *gained);

typedef enum { NW_DAY_OK = 0, NW_DAY_FREEZE_USED, NW_DAY_STREAK_BROKEN } nw_day_result_t;
nw_day_result_t nw_advance_day(nw_state_t *s);

uint8_t nw_pass_exam(nw_state_t *s);                  /* 返回新得到的成就位图 */
void nw_fail_exam(nw_state_t *s, const uint16_t *wrong, int n);
uint8_t nw_check_achievements(nw_state_t *s);

/* 小卖部：成功返回 >= 0，失败返回下面的错误码，失败时不扣金币 */
typedef enum {
    NW_SHOP_NO_COINS = -1, NW_SHOP_SET_DONE = -2, NW_SHOP_ALL_DONE = -3, NW_SHOP_FREEZE_FULL = -4,
} nw_shop_err_t;
int nw_toy_stage(const nw_state_t *s);
int nw_toy_price_now(const nw_state_t *s);
int nw_open_toy_box(nw_state_t *s, nw_rng_t *rng);    /* 返回开到的摆件编号 0..59 */
int nw_buy_freeze(nw_state_t *s);                     /* 返回持有张数 */
/* 没有就买下并戴上（返回 1），有就戴上 / 摘下（返回 2 戴上，3 摘下） */
int nw_use_accessory(nw_state_t *s, int id);
