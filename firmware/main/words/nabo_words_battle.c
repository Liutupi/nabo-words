/*
 * 对战的纯逻辑，对应 prototype/quiz.js、core.js 的对战部分和 battle.js 的 finish()。
 */
#include "nabo_words_battle.h"

#include <string.h>

/* 对手比我高 d 分时我的预期得分（千分比），d = -800..800 每 50 一档 */
static const int16_t ELO_TABLE[33] = {990, 987, 983, 977, 969, 960, 947, 930, 909, 882, 849, 808, 760, 703, 640, 571, 500,
                                      429, 360, 297, 240, 192, 151, 118, 91, 70, 53, 40, 31, 23, 17, 13, 10};
const int16_t nw_rank_min[NW_RANKS] = {0, 1100, 1200, 1350, 1500, 1700, 1850, 2000};

static bool bit(const uint8_t *bm, int i)
{
    return bm[i >> 3] & (1u << (i & 7));
}

void nw_learned_bitmap(const nw_state_t *s, uint8_t *bitmap)
{
    memset(bitmap, 0, NW_BITMAP_BYTES);
    for (int i = 0; i < NW_TOTAL; i++) {
        if (s->box[i] > 0) {
            bitmap[i >> 3] |= (uint8_t)(1u << (i & 7));
        }
    }
}

int nw_battle_pool(const nw_state_t *s, const uint8_t *peer_bitmap, uint16_t *out)
{
    int n = 0, low = 0;
    for (int i = 0; i < NW_TOTAL; i++) {
        if (s->box[i] > 0) {
            low++;
            if (bit(peer_bitmap, i)) {
                out[n++] = (uint16_t)i;
            }
        }
    }
    if (n >= 30) {
        return n;
    }
    int want = low > 30 ? low : 30;
    n = 0;
    for (int i = 0; i < NW_TOTAL && n < want; i++) {
        if (s->box[i] > 0 || bit(peer_bitmap, i) || i < 100) {
            out[n++] = (uint16_t)i;
        }
    }
    return n;
}

void nw_build_battle(uint16_t *pool, int n_pool, uint32_t seed, nw_question_t *qs)
{
    nw_rng_t rng;
    nw_rng_seed(&rng, seed);
    nw_shuffle_u16(pool, n_pool, &rng);
    for (int k = 0; k < NW_BATTLE_Q && k < n_pool; k++) {
        nw_make_question(&qs[k], NW_Q_EN2ZH, pool[k], NW_QF_FIRST, &rng);
    }
}

int nw_elo_expected(int mine, int theirs)
{
    int d = theirs - mine;
    d = (d < -800 ? -800 : d > 800 ? 800 : d) + 800;
    int k = d / 50, r = d % 50;
    if (k >= 32) {
        return ELO_TABLE[32];
    }
    return ELO_TABLE[k] + (ELO_TABLE[k + 1] - ELO_TABLE[k]) * r / 50;   /* C 的除法向零取整，和原型的 Math.trunc 一致 */
}

static int floor_div(int a, int b)
{
    int q = a / b;
    return (a % b != 0 && (a < 0) != (b < 0)) ? q - 1 : q;
}

int nw_elo_delta(int mine, int theirs, int result)
{
    return floor_div(NW_ELO_K * (result - nw_elo_expected(mine, theirs)) + 500, 1000);
}

int nw_battle_points(bool correct, int ms, bool last)
{
    if (!correct) {
        return 0;
    }
    int left = NW_BATTLE_MS - ms;
    int p = 100 + 50 * (left > 0 ? left : 0) / NW_BATTLE_MS;
    return last ? p * 2 : p;
}

int nw_rank_of(int rating)
{
    int r = 0;
    for (int k = 0; k < NW_RANKS; k++) {
        if (rating >= nw_rank_min[k]) {
            r = k;
        }
    }
    return r;
}

/* 今天和这台徽章打了几局（含这一局）；表满了就挤掉第一格 */
static int count_game(nw_state_t *s, const uint8_t mac[6])
{
    for (int k = 0; k < NW_VS_SLOTS; k++) {
        if (s->vs[k].games && memcmp(s->vs[k].mac, mac, 6) == 0) {
            return ++s->vs[k].games;
        }
    }
    for (int k = 0; k < NW_VS_SLOTS; k++) {
        if (!s->vs[k].games) {
            memcpy(s->vs[k].mac, mac, 6);
            return s->vs[k].games = 1;
        }
    }
    memmove(s->vs, s->vs + 1, sizeof(s->vs[0]) * (NW_VS_SLOTS - 1));
    memcpy(s->vs[NW_VS_SLOTS - 1].mac, mac, 6);
    return s->vs[NW_VS_SLOTS - 1].games = 1;
}

void nw_battle_finish(nw_state_t *s, const uint8_t peer_mac[6], int peer_rating, int my_score, int their_score,
                      const uint16_t *missed, int n_missed, nw_battle_result_t *res)
{
    memset(res, 0, sizeof(*res));
    res->result = my_score > their_score ? 1000 : my_score < their_score ? 0 : 500;
    int games = count_game(s, peer_mac);
    res->farm = games > NW_FARM_LIMIT;
    res->delta = res->farm ? 0 : nw_elo_delta(s->rating, peer_rating, res->result);
    s->rating = (int16_t)(s->rating + res->delta);
    res->coins = res->result == 1000 ? 3 : res->result == 500 ? 2 : 1;
    s->coins += (uint32_t)res->coins;
    if (res->result == 1000) {
        s->wins++;
        s->win_streak++;
    } else {
        s->win_streak = 0;
        if (res->result == 0) {
            s->losses++;
        }
    }
    for (int k = 0; k < n_missed; k++) {
        uint16_t i = missed[k];
        if (s->box[i] > 0 && s->due[i] > s->day + 1) {
            s->due[i] = (uint16_t)(s->day + 1);
        }
    }
    res->missed = n_missed;
    res->gained = nw_check_achievements(s);
}
