/*
 * 给 test_words.py（ctypes）用的薄封装：把逻辑模块的结构体操作摊平成简单函数。
 * 只用于宿主测试，不进固件。
 */
#include <stdlib.h>
#include <string.h>

#include "nabo_words_battle.h"
#include "nabo_words_model.h"
#include "nabo_words_quiz.h"

static nw_work_t g_work;
static nw_session_t g_sess;
static nw_rng_t g_sess_rng, g_shop_rng;

nw_state_t *t_new(int new_per_day)
{
    nw_state_t *s = malloc(sizeof(*s));
    nw_state_init(s);
    s->new_per_day = (uint8_t)new_per_day;
    return s;
}

void t_free(nw_state_t *s)
{
    free(s);
}

/* 和 record_golden.js 的 digest() 同序；toys 另外写进 toys_out，返回摆件个数 */
int t_digest(nw_state_t *s, int32_t *out, int32_t *toys_out)
{
    uint32_t h = 2166136261u;
    for (int i = 0; i < NW_TOTAL; i++) {
        uint16_t d = s->due[i];
        uint8_t bytes[3] = {s->box[i], (uint8_t)(d & 255), (uint8_t)(d >> 8)};
        for (int k = 0; k < 3; k++) {
            h ^= bytes[k];
            h *= 16777619u;
        }
    }
    int st = s->stage < NW_STAGES ? s->stage : NW_STAGES - 1;
    int32_t v[] = {s->day, s->next, s->stage, s->streak, s->best_streak, s->freezes, (int32_t)s->coins, (int32_t)s->spent,
                   s->done_day, s->rating, s->wins, s->losses, s->win_streak, s->accs, s->acc, s->achievements,
                   s->exam_first, nw_mastered(s), nw_learned(s), nw_stamp_count(s), nw_sprite_form(s, st),
                   nw_exam_ready(s), (int32_t)h};
    memcpy(out, v, sizeof(v));
    for (int k = 0; k < s->toy_count; k++) {
        toys_out[k] = s->toys[k];
    }
    return s->toy_count;
}

/* meta：复习数、新词数、积压、预计秒数 */
void t_plan(nw_state_t *s, uint16_t *reviews, uint16_t *fresh, int32_t *meta)
{
    static nw_plan_t plan;
    nw_plan_today(s, &plan, &g_work);
    memcpy(reviews, plan.reviews, plan.n_reviews * sizeof(uint16_t));
    memcpy(fresh, plan.fresh, plan.n_fresh * sizeof(uint16_t));
    meta[0] = plan.n_reviews;
    meta[1] = plan.n_fresh;
    meta[2] = plan.backlog;
    meta[3] = plan.seconds;
}

void t_set_box(nw_state_t *s, int idx, int box) { s->box[idx] = (uint8_t)box; }
int t_exam_ready(nw_state_t *s) { return nw_exam_ready(s); }
int t_sprite_form(nw_state_t *s, int stage) { return nw_sprite_form(s, stage); }
void t_grade(nw_state_t *s, int idx, int ok) { nw_grade(s, (uint16_t)idx, ok, true); }
void t_introduce(nw_state_t *s, int idx) { nw_introduce(s, (uint16_t)idx); }
void t_finish_day(nw_state_t *s, int perfect) { uint8_t g; nw_finish_day(s, perfect, &g); }
int t_advance_day(nw_state_t *s) { return nw_advance_day(s); }

/* ---------- 学习队列 ---------- */

int t_session_build(nw_state_t *s, uint32_t seed, int exam)
{
    nw_rng_seed(&g_sess_rng, seed);
    if (exam) {
        nw_build_exam(s, &g_sess, &g_sess_rng, &g_work);
    } else {
        nw_build_session(s, &g_sess, &g_sess_rng, &g_work);
    }
    return g_sess.len;
}

uint32_t t_session_seed(nw_state_t *s) { return nw_session_seed(s); }
int t_session_len(void) { return g_sess.len; }
int t_session_pos(void) { return g_sess.pos; }
void t_session_next(void) { g_sess.pos++; }
void t_session_intro(nw_state_t *s) { nw_session_intro_seen(s, &g_sess); }
void t_session_answer(nw_state_t *s, int ok) { nw_session_answer(s, &g_sess, ok, &g_sess_rng); }

/* out：类型、序号、标志、选项数、选项×3、答案、挖空位置 */
static void put_q(const nw_question_t *q, int32_t *out)
{
    int32_t v[] = {q->type, q->idx, q->flags, q->n_opt, q->opt[0], q->opt[1], q->opt[2], q->answer, q->pos};
    memcpy(out, v, sizeof(v));
}

void t_session_q(int i, int32_t *out) { put_q(&g_sess.q[i], out); }

/* out：答对、答错、是否通过（结业考）、新盖的章 */
void t_session_finish(nw_state_t *s, int32_t *out)
{
    nw_session_result_t r;
    nw_finish_session(s, &g_sess, &r);
    out[0] = g_sess.right;
    out[1] = g_sess.wrong;
    out[2] = r.passed;
    out[3] = r.stamps_gained;
}

/* 结业考答对 right 题时是否通过 */
int t_exam_passes(int right)
{
    nw_state_t *s = t_new(10);
    nw_session_t *sess = malloc(sizeof(*sess));
    nw_session_result_t r;
    memset(sess, 0, sizeof(*sess));
    sess->exam = true;
    sess->right = (uint16_t)right;
    nw_finish_session(s, sess, &r);
    free(sess);
    free(s);
    return r.passed;
}

void t_make_q(int type, int idx, uint32_t seed, int32_t *out)
{
    nw_rng_t r;
    nw_question_t q;
    nw_rng_seed(&r, seed);
    nw_make_question(&q, (nw_qtype_t)type, (uint16_t)idx, NW_QF_FIRST, &r);
    put_q(&q, out);
}

/* ---------- 小卖部 ---------- */

void t_shop_seed(uint32_t seed) { nw_rng_seed(&g_shop_rng, seed); }
int t_open_toy(nw_state_t *s) { return nw_open_toy_box(s, &g_shop_rng); }
int t_buy_freeze(nw_state_t *s) { return nw_buy_freeze(s); }
int t_use_acc(nw_state_t *s, int id) { return nw_use_accessory(s, id); }

/* ---------- 对战 ---------- */

/* 出题：返回题库大小，qs_out 每题 9 个数（同 put_q） */
int t_battle(nw_state_t *s, const uint8_t *bits, uint32_t seed, int32_t *qs_out)
{
    static uint16_t pool[NW_TOTAL];
    static nw_question_t qs[NW_BATTLE_Q];
    int n = nw_battle_pool(s, bits, pool);
    nw_build_battle(pool, n, seed, qs);
    for (int k = 0; k < NW_BATTLE_Q && k < n; k++) {
        put_q(&qs[k], qs_out + 9 * k);
    }
    return n;
}

/* out：结果、积分变化、是否打满 */
void t_battle_finish(nw_state_t *s, int peer, int peer_rating, int my, int their, const uint16_t *missed, int n, int32_t *out)
{
    uint8_t mac[6] = {(uint8_t)(peer + 1), 0xAB, 0, 0, 0, 0};
    nw_battle_result_t r;
    nw_battle_finish(s, mac, peer_rating, my, their, missed, n, &r);
    out[0] = r.result;
    out[1] = r.delta;
    out[2] = r.farm;
}
