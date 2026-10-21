/*
 * 开局分级测试、学有余力时的加学、闪电挑战。和 nabo_words_model / quiz 一样是纯逻辑，不依赖屏幕和 RTOS。
 * DESIGN.md 第 2 节（分级测试）、第 5 节（持续动力）。原型还没有这三项，规则以这里和 tests/words_host/test_extra.c 为准。
 */
#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "nabo_words_model.h"
#include "nabo_words_quiz.h"

/* ---------- 分级测试：每轮考一个学段 10 题，答对 8 题算过，二分查找起点，最多 3 轮 30 题 ---------- */
#define NW_PLACE_Q 10
#define NW_PLACE_PASS 8

typedef struct {
    uint8_t lo, hi;            /* 起点学段在 [lo, hi] 里 */
    uint8_t probe;             /* 这一轮考的学段 */
    uint8_t pos, right;        /* 这一轮第几题、答对几题 */
    uint8_t rounds;            /* 已经考完几轮 */
    nw_question_t q[NW_PLACE_Q];
} nw_place_t;

void nw_place_start(nw_place_t *p, nw_rng_t *rng);
/* 当前题作答。返回 true 表示测试结束，起点学段是 p->lo */
bool nw_place_answer(nw_place_t *p, bool correct, nw_rng_t *rng);
/* 按测试结果从第 stage 学段开始：之前学段的词记为毕业（不再排期，每天的毕业抽查里自然复习） */
void nw_place_apply(nw_state_t *s, int stage);

/* ---------- 加学：今天的学完以后再学一组新词（新词卡 + 检测），一天最多 NW_EXTRA_GROUPS 组 ---------- */
#define NW_EXTRA_WORDS 5
#define NW_EXTRA_GROUPS 4

/* 还能加学吗：今天已完成、当前学段还有没发的词（结业考之前不发下一学段） */
bool nw_extra_available(const nw_state_t *s, int groups_today);
/* 生成一组加学，返回题数（0 表示没有可学的） */
int nw_build_extra(const nw_state_t *s, nw_session_t *sess, nw_rng_t *rng);

/* ---------- 闪电挑战：60 秒抢答学过的词，答错的排进明天的复习 ---------- */
#define NW_CHALLENGE_MS 60000
#define NW_CHALLENGE_MIN 20       /* 学过的词少于这个数不开放 */
#define NW_CHALLENGE_COIN_PER 5   /* 每天第一次挑战：每答对 5 题 1 枚金币 */
#define NW_CHALLENGE_COIN_MAX 6

int nw_challenge_pool(const nw_state_t *s);   /* 学过的词数 */
void nw_challenge_question(const nw_state_t *s, nw_question_t *q, nw_rng_t *rng);
void nw_challenge_miss(nw_state_t *s, uint16_t idx);
int nw_challenge_coins(int score, bool first_today);
