/*
 * 对战的纯逻辑：出题范围、出题、计分、积分（Elo）、段位、对局结算。
 * 两台徽章用同一个种子在本地出同样的题，只交换答案和用时（报文见 DESIGN.md 第 6 节）。
 * 对应 prototype/quiz.js 的 battlePool / buildBattle、core.js 的对战计分和 battle.js 的 finish()。
 */
#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "nabo_words_model.h"
#include "nabo_words_quiz.h"

#define NW_BATTLE_Q      10
#define NW_BATTLE_MS     8000
#define NW_ELO_K         32
#define NW_FARM_LIMIT    3
#define NW_BITMAP_BYTES  ((NW_TOTAL + 7) / 8)
#define NW_RANKS         8

/* 学过的词的位图（盒子 ≥ 1），对战前两台徽章交换 */
void nw_learned_bitmap(const nw_state_t *s, uint8_t *bitmap);

/* 出题范围：双方都学过的词；不足 30 个时退回较低一方学过的词。返回个数，结果写进 out */
int nw_battle_pool(const nw_state_t *s, const uint8_t *peer_bitmap, uint16_t *out);
void nw_build_battle(uint16_t *pool, int n_pool, uint32_t seed, nw_question_t *qs);   /* pool 会被就地洗牌 */

int nw_elo_expected(int mine, int theirs);            /* 预期得分，千分比 */
int nw_elo_delta(int mine, int theirs, int result);   /* result：赢 1000、平 500、输 0 */
int nw_battle_points(bool correct, int ms, bool last);
int nw_rank_of(int rating);                           /* 0 童生 … 7 状元 */
extern const int16_t nw_rank_min[NW_RANKS];

typedef struct {
    int result;             /* 1000 / 500 / 0 */
    int delta;              /* 积分变化 */
    int coins;
    int missed;             /* 答错的词数（已排进明天的复习） */
    bool farm;              /* 今天和这台徽章打满了，积分不变 */
    uint8_t gained;         /* 新得到的成就 */
} nw_battle_result_t;

/*
 * 结算一局。my_score / their_score 是双方总分；missed 是本机答错的词。
 * 同一台徽章一个学习日内第 4 局起不改积分，只给金币。
 */
void nw_battle_finish(nw_state_t *s, const uint8_t peer_mac[6], int peer_rating, int my_score, int their_score,
                      const uint16_t *missed, int n_missed, nw_battle_result_t *res);
