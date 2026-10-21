/* 对战的联机流程，见 nw_vs.h */
#include "nw_vs.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "nw_game_int.h"
#include "nw_platform.h"
#include "nw_sound.h"

#define MAGIC0 'N'
#define MAGIC1 'W'
#define VER 1

enum { P_HELLO = 1, P_INVITE, P_ACCEPT, P_DECLINE, P_BITMAP, P_START, P_ANSWER, P_RESULT, P_BYE };

#define HELLO_MS 500
#define PEER_TTL_MS 3000
#define INVITE_MS 10000
#define SYNC_RETRY_MS 1500
#define COUNT_MS 3000
#define GRACE_MS 1200            /* 题目时间到了以后再等对方的答案多久 */
#define REVEAL_MS 1800
#define RESULT_WAIT_MS 4000
#define LOST_MS 15000
#define PART 235                 /* 位图每片字节数，4 片 */

#pragma pack(push, 1)
typedef struct { uint8_t m0, m1, ver, type; } hdr_t;
typedef struct { hdr_t h; int16_t rating; uint16_t mastered; uint8_t stage, busy; char city[NW_VS_CITY]; } p_hello_t;
typedef struct { hdr_t h; uint8_t part, n; uint8_t data[PART]; } p_bitmap_t;
typedef struct { uint16_t idx, opt[3]; uint8_t answer; } p_q_t;
typedef struct { hdr_t h; uint32_t seed; uint16_t pool; uint8_t n; p_q_t q[NW_BATTLE_Q]; } p_start_t;
typedef struct { hdr_t h; uint8_t qi; int8_t pick; uint16_t ms; } p_answer_t;
typedef struct { hdr_t h; uint16_t mine, theirs; } p_result_t;
#pragma pack(pop)
_Static_assert(sizeof(p_start_t) <= NW_RADIO_MAX && sizeof(p_bitmap_t) <= NW_RADIO_MAX, "ESP-NOW 包最大 250 字节");

static nw_vs_t *V;

static void cue(nw_cue_t c)
{
    if (!G->rec.e.muted) nw_plat_cue(c);
}

static void phase(nw_vs_phase_t p)
{
    V->phase = p;
    V->t0 = G->now;
}

static hdr_t hdr(int type) { return (hdr_t){MAGIC0, MAGIC1, VER, (uint8_t)type}; }

static bool send_to(const uint8_t *mac, const void *p, int n) { return V->radio_ok && nw_plat_radio_send(mac, p, n); }

static void send_simple(int type)
{
    hdr_t h = hdr(type);
    send_to(V->opp.mac, &h, sizeof(h));
}

static void my_hello(p_hello_t *p, int type)
{
    memset(p, 0, sizeof(*p));
    p->h = hdr(type);
    p->rating = G->rec.s.rating;
    p->mastered = (uint16_t)nw_mastered(&G->rec.s);
    p->stage = G->rec.s.stage;
    p->busy = V->phase != VS_SEARCH;
    snprintf(p->city, sizeof(p->city), "%s", G->rec.e.onboard ? nw_unit_city(0) : "");
}

/* 对方的名字：“中山的 NABO”，没选家乡的用 MAC 末两字节 */
void nw_vs_name(const nw_vs_peer_t *p, char *out, int n)
{
    if (p->city[0]) snprintf(out, (size_t)n, "%s的 NABO", p->city);
    else snprintf(out, (size_t)n, "NABO-%02X%02X", p->mac[4], p->mac[5]);
}

static void end_match(const char *msg)
{
    snprintf(V->msg, sizeof(V->msg), "%s", msg ? msg : "");
    /* 没计入积分的局，答错的词照样排进明天的复习 */
    if (!V->counted) {
        for (int k = 0; k < V->n_missed; k++) nw_challenge_miss(&G->rec.s, V->missed[k]);
    }
    G->dirty = true;
    G->dirty_since = G->now;
    G->plan_ok = false;
    phase(VS_END);
    cue(NW_CUE_DONE);
}

/* ---------- 进出 ---------- */

bool nw_vs_enter(void)
{
    free(G->vs);
    G->vs = calloc(1, sizeof(nw_vs_t));
    V = G->vs;
    if (!V) return false;
    V->radio_ok = nw_plat_radio_on();
    phase(VS_SEARCH);
    V->hello_at = 0;
    return true;
}

void nw_vs_leave(void)
{
    if (!V) return;
    if (V->phase >= VS_INVITING && V->phase < VS_END) send_simple(P_BYE);
    if (V->phase >= VS_COUNT && V->phase < VS_END) {
        for (int k = 0; k < V->n_missed; k++) nw_challenge_miss(&G->rec.s, V->missed[k]);
    }
    if (V->radio_ok) nw_plat_radio_off();
    free(V);
    V = G->vs = NULL;
}

/* ---------- 出题（挑战方） ---------- */

static void build_and_start(void)
{
    uint16_t *pool = malloc(sizeof(uint16_t) * NW_TOTAL);
    if (!pool) {
        send_simple(P_BYE);
        end_match("内存不足，本局取消");
        return;
    }
    V->pool = nw_battle_pool(&G->rec.s, V->peer_bitmap, pool);
    uint32_t seed = nw_plat_random();
    nw_build_battle(pool, V->pool, seed, V->qs);
    free(pool);
    p_start_t p;
    memset(&p, 0, sizeof(p));
    p.h = hdr(P_START);
    p.seed = seed;
    p.pool = (uint16_t)V->pool;
    p.n = NW_BATTLE_Q;
    for (int k = 0; k < NW_BATTLE_Q; k++) {
        p.q[k].idx = V->qs[k].idx;
        memcpy(p.q[k].opt, V->qs[k].opt, sizeof(p.q[k].opt));
        p.q[k].answer = V->qs[k].answer;
    }
    if (!send_to(V->opp.mac, &p, sizeof(p))) {
        end_match("连接不稳定，本局取消");
        return;
    }
    phase(VS_COUNT);
}

static void send_bitmap(void)
{
    uint8_t bm[NW_BITMAP_BYTES];
    nw_learned_bitmap(&G->rec.s, bm);
    for (int part = 0; part * PART < NW_BITMAP_BYTES; part++) {
        p_bitmap_t p;
        memset(&p, 0, sizeof(p));
        p.h = hdr(P_BITMAP);
        p.part = (uint8_t)part;
        int n = NW_BITMAP_BYTES - part * PART;
        p.n = (uint8_t)(n > PART ? PART : n);
        memcpy(p.data, bm + part * PART, p.n);
        send_to(V->opp.mac, &p, sizeof(p));
    }
    V->sync_at = G->now;
    V->sync_tries++;
}

static void begin_q(void)
{
    V->my_pick = -1;
    V->my_ms = 0;
    G->sel = 0;
    phase(VS_Q);
}

static void send_answer(void)
{
    p_answer_t p = {hdr(P_ANSWER), (uint8_t)V->qi, V->my_pick, V->my_ms};
    send_to(V->opp.mac, &p, sizeof(p));
}

/* ---------- 收包 ---------- */

static bool from_opp(const uint8_t *mac) { return !memcmp(mac, V->opp.mac, 6); }

static void on_packet(const uint8_t *mac, const uint8_t *d, int n)
{
    if (n < (int)sizeof(hdr_t) || d[0] != MAGIC0 || d[1] != MAGIC1 || d[2] != VER) return;
    int type = d[3];
    if (from_opp(mac)) V->opp_seen = G->now;
    if (type == P_HELLO || type == P_INVITE) {
        if (n < (int)sizeof(p_hello_t)) return;
        const p_hello_t *h = (const p_hello_t *)d;
        nw_vs_peer_t *p = NULL;
        for (int k = 0; k < V->n_peers; k++) {
            if (!memcmp(V->peers[k].mac, mac, 6)) p = &V->peers[k];
        }
        if (!p && V->n_peers < NW_VS_PEERS) p = &V->peers[V->n_peers++];
        if (p) {
            memcpy(p->mac, mac, 6);
            p->rating = h->rating;
            p->mastered = h->mastered;
            p->stage = h->stage;
            p->busy = h->busy;
            memcpy(p->city, h->city, NW_VS_CITY);
            p->city[NW_VS_CITY - 1] = 0;
            p->seen = G->now;
        }
        if (type == P_INVITE) {
            if (V->phase == VS_SEARCH && p) {
                V->opp = *p;
                V->opp_seen = G->now;
                phase(VS_INVITED);
                cue(NW_CUE_RIGHT);
            } else {
                hdr_t no = hdr(P_DECLINE);
                send_to(mac, &no, sizeof(no));   /* 正忙 */
            }
        }
        return;
    }
    if (!from_opp(mac)) return;
    switch (type) {
    case P_ACCEPT:
        if (V->phase == VS_INVITING) {
            V->host = true;
            V->bitmap_parts = 0;
            phase(VS_SYNC);
        }
        break;
    case P_DECLINE:
        if (V->phase == VS_INVITING) {
            snprintf(G->toast, sizeof(G->toast), "对方现在不方便");
            G->toast_until = G->now + 2200;
            phase(VS_SEARCH);
        }
        break;
    case P_BITMAP:
        if (V->phase == VS_SYNC && V->host && n >= (int)sizeof(hdr_t) + 2) {
            const p_bitmap_t *b = (const p_bitmap_t *)d;
            if (b->part * PART + b->n <= NW_BITMAP_BYTES) {
                memcpy(V->peer_bitmap + b->part * PART, b->data, b->n);
                V->bitmap_parts |= (uint8_t)(1u << b->part);
            }
            if (V->bitmap_parts == 0x0F) build_and_start();
        }
        break;
    case P_START:
        if (V->phase == VS_SYNC && !V->host && n >= (int)sizeof(p_start_t)) {
            const p_start_t *p = (const p_start_t *)d;
            V->pool = p->pool;
            for (int k = 0; k < NW_BATTLE_Q; k++) {
                nw_question_t *q = &V->qs[k];
                memset(q, 0, sizeof(*q));
                q->idx = p->q[k].idx < NW_TOTAL ? p->q[k].idx : 0;
                q->type = NW_Q_EN2ZH;
                q->n_opt = 3;
                for (int j = 0; j < 3; j++) q->opt[j] = p->q[k].opt[j] < NW_TOTAL ? p->q[k].opt[j] : 0;
                q->answer = p->q[k].answer < 3 ? p->q[k].answer : 0;
            }
            phase(VS_COUNT);
        }
        break;
    case P_ANSWER: {
        if (n < (int)sizeof(p_answer_t)) return;
        const p_answer_t *a = (const p_answer_t *)d;
        if (a->qi < NW_BATTLE_Q && V->op_pick[a->qi] == -2) {
            V->op_pick[a->qi] = a->pick;
            V->op_ms[a->qi] = a->ms;
        }
        break;
    }
    case P_RESULT:
        if (n >= (int)sizeof(p_result_t)) {
            const p_result_t *r = (const p_result_t *)d;
            V->got_result = true;
            V->peer_result[0] = r->mine;
            V->peer_result[1] = r->theirs;
        }
        break;
    case P_BYE:
        if (V->phase == VS_INVITED || V->phase == VS_INVITING || V->phase == VS_SYNC) {
            phase(VS_SEARCH);
        } else if (V->phase >= VS_COUNT && V->phase < VS_END) {
            end_match("对方退出了，本局不计");
        }
        break;
    }
}

/* ---------- 每帧 ---------- */

static int points(bool correct, int ms) { return nw_battle_points(correct, ms, V->qi == NW_BATTLE_Q - 1); }

static void reveal(void)
{
    const nw_question_t *q = &V->qs[V->qi];
    bool mine = V->my_pick == q->answer;
    int8_t op = V->op_pick[V->qi];
    if (op == -2) op = 3;   /* 没收到算超时 */
    V->gain[0] = points(mine, V->my_ms);
    V->gain[1] = points(op == q->answer, V->op_ms[V->qi]);
    V->score[0] += V->gain[0];
    V->score[1] += V->gain[1];
    if (!mine && V->n_missed < NW_BATTLE_Q) V->missed[V->n_missed++] = q->idx;
    cue(mine ? NW_CUE_RIGHT : NW_CUE_WRONG);
    phase(VS_REVEAL);
}

static void finish(void)
{
    p_result_t r = {hdr(P_RESULT), (uint16_t)V->score[0], (uint16_t)V->score[1]};
    send_to(V->opp.mac, &r, sizeof(r));
    phase(VS_WAIT);
}

static void settle(void)
{
    /* 两边各自算的比分要对得上才计入积分（丢包时可能不一致） */
    if (!V->got_result) {
        end_match("没等到对方的比分，本局不计积分");
        return;
    }
    if (V->peer_result[0] != V->score[1] || V->peer_result[1] != V->score[0]) {
        end_match("两边比分没对上（信号不好），本局不计积分");
        return;
    }
    V->counted = true;
    nw_battle_finish(&G->rec.s, V->opp.mac, V->opp.rating, V->score[0], V->score[1], V->missed, V->n_missed, &V->res);
    end_match(V->res.farm ? "今天和这位对手已打满 3 局，积分不再变化" : NULL);
}

void nw_vs_update(void)
{
    if (!V) return;
    uint8_t mac[6], buf[NW_RADIO_MAX];
    int n;
    while (V->radio_ok && (n = nw_plat_radio_recv(mac, buf, sizeof(buf))) > 0) on_packet(mac, buf, n);
    uint32_t el = G->now - V->t0;

    /* 搜索和邀请时一直广播 HELLO，别人才看得到我 */
    if (V->phase <= VS_INVITED && G->now - V->hello_at >= HELLO_MS) {
        p_hello_t h;
        my_hello(&h, P_HELLO);
        send_to(NULL, &h, sizeof(h));
        V->hello_at = G->now;
    }
    /* 过期的工牌从列表里拿掉 */
    for (int k = 0; k < V->n_peers;) {
        if (G->now - V->peers[k].seen > PEER_TTL_MS) V->peers[k] = V->peers[--V->n_peers];
        else k++;
    }
    if (V->sel >= V->n_peers) V->sel = V->n_peers ? V->n_peers - 1 : 0;

    bool in_match = V->phase >= VS_SYNC && V->phase < VS_END;
    if (in_match && G->now - V->opp_seen > LOST_MS) {
        end_match("连接断开，本局不计");
        return;
    }
    switch (V->phase) {
    case VS_INVITING:
        if (el > INVITE_MS) {
            snprintf(G->toast, sizeof(G->toast), "对方没有回应");
            G->toast_until = G->now + 2200;
            phase(VS_SEARCH);
        }
        break;
    case VS_INVITED:
        if (el > INVITE_MS) phase(VS_SEARCH);
        break;
    case VS_SYNC:
        /* 接受方：发位图；没等到 START 就重发 */
        if (!V->host && (V->sync_tries == 0 || (G->now - V->sync_at > SYNC_RETRY_MS && V->sync_tries < 4))) send_bitmap();
        if (el > 8000) end_match("连接不稳定，本局取消");
        break;
    case VS_COUNT:
        if (el >= COUNT_MS) {
            V->qi = 0;
            for (int k = 0; k < NW_BATTLE_Q; k++) V->op_pick[k] = -2;
            begin_q();
        }
        break;
    case VS_Q:
        if (V->my_pick < 0 && el >= NW_BATTLE_MS) {
            V->my_pick = 3;
            V->my_ms = NW_BATTLE_MS;
            send_answer();
        }
        if (V->my_pick >= 0 && (V->op_pick[V->qi] != -2 || el >= NW_BATTLE_MS + GRACE_MS)) reveal();
        break;
    case VS_REVEAL:
        if (el >= REVEAL_MS) {
            if (++V->qi >= NW_BATTLE_Q) finish();
            else begin_q();
        }
        break;
    case VS_WAIT:
        if (V->got_result || el >= RESULT_WAIT_MS) settle();
        break;
    default:
        break;
    }
}

/* ---------- 按键 ---------- */

void nw_vs_key(int key)
{
    switch (V->phase) {
    case VS_SEARCH:
        if (!V->n_peers) {
            if (key == NW_KEY_OK && !V->radio_ok) {
                V->radio_ok = nw_plat_radio_on();
            }
            return;
        }
        if (key == NW_KEY_UP_PRESS) V->sel = (V->sel + V->n_peers - 1) % V->n_peers;
        else if (key == NW_KEY_DOWN) V->sel = (V->sel + 1) % V->n_peers;
        else if (V->peers[V->sel].busy) {
            snprintf(G->toast, sizeof(G->toast), "对方正在对战");
            G->toast_until = G->now + 2200;
        } else {
            V->opp = V->peers[V->sel];
            V->opp_seen = G->now;
            p_hello_t h;
            phase(VS_INVITING);
            my_hello(&h, P_INVITE);
            if (!send_to(V->opp.mac, &h, sizeof(h))) {
                snprintf(G->toast, sizeof(G->toast), "没连上，再试一次");
                G->toast_until = G->now + 2200;
                phase(VS_SEARCH);
            }
        }
        break;
    case VS_INVITED:
        if (key == NW_KEY_OK) {
            hdr_t h = hdr(P_ACCEPT);
            if (send_to(V->opp.mac, &h, sizeof(h))) {
                V->host = false;
                V->sync_tries = 0;
                phase(VS_SYNC);
            } else {
                phase(VS_SEARCH);
            }
        } else if (key == NW_KEY_DOWN) {
            send_simple(P_DECLINE);
            phase(VS_SEARCH);
        }
        break;
    case VS_Q:
        if (V->my_pick >= 0) return;
        if (key == NW_KEY_UP_PRESS) G->sel = (uint8_t)((G->sel + 2) % 3);
        else if (key == NW_KEY_DOWN) G->sel = (uint8_t)((G->sel + 1) % 3);
        else {
            V->my_pick = (int8_t)G->sel;
            V->my_ms = (uint16_t)(G->now - V->t0);
            send_answer();
        }
        break;
    default:
        break;
    }
}
