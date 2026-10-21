#include "nabo_words_extra.h"

#include <string.h>

/* ---------- 分级测试 ---------- */

static void place_round(nw_place_t *p, nw_rng_t *rng)
{
    p->probe = (uint8_t)((p->lo + p->hi) / 2);
    p->pos = 0;
    p->right = 0;
    int a = nw_stage_start[p->probe], n = nw_stage_end[p->probe] - a;
    uint16_t pick[NW_PLACE_Q];
    for (int k = 0; k < NW_PLACE_Q; k++) {
        uint16_t w;
        bool dup;
        do {
            w = (uint16_t)(a + (int)nw_rng_below(rng, (uint32_t)n));
            dup = false;
            for (int j = 0; j < k; j++) dup |= pick[j] == w;
        } while (dup);
        pick[k] = w;
        nw_make_question(&p->q[k], NW_Q_EN2ZH, w, 0, rng);
    }
}

void nw_place_start(nw_place_t *p, nw_rng_t *rng)
{
    memset(p, 0, sizeof(*p));
    p->lo = 0;
    p->hi = NW_STAGES - 1;   /* 最多从进阶开始 */
    place_round(p, rng);
}

bool nw_place_answer(nw_place_t *p, bool correct, nw_rng_t *rng)
{
    p->right += correct;
    if (++p->pos < NW_PLACE_Q) return false;
    p->rounds++;
    if (p->right >= NW_PLACE_PASS) p->lo = (uint8_t)(p->probe + 1);
    else p->hi = p->probe;
    if (p->lo >= p->hi) {
        if (p->lo > NW_STAGES - 1) p->lo = NW_STAGES - 1;
        return true;
    }
    place_round(p, rng);
    return false;
}

void nw_place_apply(nw_state_t *s, int stage)
{
    if (stage <= 0 || stage >= NW_STAGES) return;
    int end = nw_stage_start[stage];
    for (int i = 0; i < end; i++) {
        s->box[i] = NW_GRADUATED;
        s->due[i] = NW_NEVER;
    }
    if (s->stage < stage) s->stage = (uint8_t)stage;
    if (s->next < end) s->next = (uint16_t)end;
}

/* ---------- 加学 ---------- */

bool nw_extra_available(const nw_state_t *s, int groups_today)
{
    return s->done_day == s->day && groups_today < NW_EXTRA_GROUPS && s->stage < NW_STAGES &&
           s->next < nw_stage_end[s->stage];
}

int nw_build_extra(const nw_state_t *s, nw_session_t *sess, nw_rng_t *rng)
{
    memset(sess, 0, sizeof(*sess));
    int end = nw_stage_end[s->stage < NW_STAGES ? s->stage : NW_STAGES - 1];
    uint16_t fresh[NW_EXTRA_WORDS];
    int n = 0;
    for (int i = s->next; i < end && n < NW_EXTRA_WORDS; i++) fresh[n++] = (uint16_t)i;
    for (int k = 0; k < n; k++) nw_make_question(&sess->q[sess->len++], NW_Q_INTRO, fresh[k], NW_QF_FIRST, rng);
    nw_shuffle_u16(fresh, n, rng);
    for (int k = 0; k < n; k++) {
        nw_make_question(&sess->q[sess->len++], NW_Q_EN2ZH, fresh[k], NW_QF_FIRST | NW_QF_NEW, rng);
    }
    sess->total = sess->len;
    sess->plan_fresh = (uint16_t)n;
    sess->stamps_before = (uint16_t)nw_stamp_count(s);
    return sess->len;
}

/* ---------- 闪电挑战 ---------- */

int nw_challenge_pool(const nw_state_t *s) { return nw_learned(s); }

void nw_challenge_question(const nw_state_t *s, nw_question_t *q, nw_rng_t *rng)
{
    int n = nw_learned(s), k = (int)nw_rng_below(rng, (uint32_t)(n ? n : 1));
    uint16_t w = 0;
    for (int i = 0; i < NW_TOTAL; i++) {
        if (s->box[i] >= 1 && k-- == 0) {
            w = (uint16_t)i;
            break;
        }
    }
    nw_make_question(q, NW_Q_EN2ZH, w, 0, rng);
}

/* 和对战答错一样：排进明天的复习，不改盒子 */
void nw_challenge_miss(nw_state_t *s, uint16_t idx)
{
    if (s->box[idx] > 0 && s->due[idx] > s->day + 1) s->due[idx] = (uint16_t)(s->day + 1);
}

int nw_challenge_coins(int score, bool first_today)
{
    if (!first_today) return 0;
    int c = score / NW_CHALLENGE_COIN_PER;
    return c > NW_CHALLENGE_COIN_MAX ? NW_CHALLENGE_COIN_MAX : c;
}
