/* 对战画面：搜索 / 邀请、倒计时、答题和揭晓、结算。对应原型 battle.js 的 drawView(0) */
#include <stdio.h>
#include <string.h>

#include "nw_db.h"
#include "nw_game_int.h"

#define RGB nw_rgb
#define C_RIVAL 0xF08A3C

static const nw_font_id_t F_WORD[] = {NW_W36, NW_W28, NW_W20, NW_W16};
static const nw_font_id_t F_OPT[] = {NW_F16, NW_F12};

/* 搜索时的雷达圈：一圈细环，从中心往外扩 */
static void paint_ring(const nw_op_t *op, int y, int x0, int x1, uint16_t *row)
{
    int cx = op->u.custom.a, cy = op->u.custom.b, r = op->u.custom.c, dy = y - cy;
    int lo = (r - 1) * (r - 1), hi = (r + 1) * (r + 1);
    for (int x = x0; x < x1; x++) {
        int d = (x - cx) * (x - cx) + dy * dy;
        if (d >= lo && d <= hi) row[x - x0] = op->u.custom.color;
    }
}

static void radar(void)
{
    for (int k = 0; k < 3; k++) {
        int r = (int)((G->now / 25 + k * 40) % 120);
        if (r < 8) continue;
        uint16_t c = nw_blend(RGB(0x141A30), RGB(C_TEAL), 256 - r * 2);
        nw_custom(&G->gfx, 120 - r - 1, 120 - r - 1, 120 + r + 2, 120 + r + 2, paint_ring, NULL, 120, 120, r, 0, c, 11);
    }
}

static void sprite(int x, int y)
{
    const nw_state_t *s = &G->rec.s;
    int si = s->stage < NW_STAGES ? s->stage : NW_STAGES - 1;
    int form = nw_sprite_form(s, si);
    nw_draw_sprite(si, form < 1 ? 1 : form, x, y, s->acc == NW_ACC_NONE ? -1 : s->acc,
                   nw_isin((int)((G->now % 2094) * 1024 / 2094)) * 2 / 256);
}

static void bottom(const char *s) { nw_text(&G->gfx, NW_F12, s, 120, 302, NW_CENTER, RGB(0x5B6584)); }

static void lobby(const nw_vs_t *V)
{
    nw_gfx_t *g = &G->gfx;
    char t[64], name[40];
    radar();
    sprite(120, 140);
    if (!V->radio_ok) {
        nw_text(g, NW_F16, "无线没打开", 120, 214, NW_CENTER, RGB(C_WHITE));
        bottom("OK 重试 · 长按上键 返回");
        return;
    }
    if (V->phase == VS_INVITED) {
        nw_vs_name(&V->opp, name, sizeof(name));
        nw_rect(g, 14, 196, 212, 78, 10, RGB(0x222B48));
        snprintf(t, sizeof(t), "%s发来挑战！", name);
        nw_text_fit(g, F_OPT, 2, t, 120, 214, 196, NW_CENTER, RGB(C_GOLD));
        snprintf(t, sizeof(t), "%s · 积分 %d · 掌握 %u 词", NW_RANK_NAME[nw_rank_of(V->opp.rating)], V->opp.rating,
                 V->opp.mastered);
        nw_text(g, NW_F12, t, 120, 238, NW_CENTER, RGB(C_MUTED));
        nw_text(g, NW_F16, "OK 接受 · 下键 拒绝", 120, 260, NW_CENTER, RGB(C_WHITE));
        bottom("ESP-NOW 直连 · 不需要网络");
        return;
    }
    if (V->phase == VS_SYNC) {
        nw_text(g, NW_F16, "交换词库，出题中…", 120, 224, NW_CENTER, RGB(C_WHITE));
        bottom("两台工牌靠近一点更稳");
        return;
    }
    if (!V->n_peers) {
        nw_text(g, NW_F16, "正在寻找附近的 NABO…", 120, 222, NW_CENTER, RGB(C_WHITE));
        nw_text(g, NW_F12, "让对方也在主页选“对战”", 120, 248, NW_CENTER, RGB(C_MUTED));
        bottom("长按上键 返回");
        return;
    }
    /* 附近的工牌（最多显示 3 个） */
    int top = V->sel > 2 ? V->sel - 2 : 0;
    for (int k = 0; k < 3 && top + k < V->n_peers; k++) {
        const nw_vs_peer_t *p = &V->peers[top + k];
        int y = 188 + k * 34, i = top + k;
        bool on = i == V->sel;
        nw_rect_line(g, 14, y, 212, 30, 8, RGB(on ? 0x2A3560 : 0x1C2440), 255, RGB(on ? C_GOLD : 0x222B48), on ? 2 : 1);
        nw_vs_name(p, name, sizeof(name));
        nw_text_fit(g, F_OPT, 2, name, 24, y + 15, 110, NW_LEFT, RGB(C_WHITE));
        const char *right = p->busy ? "对战中" : V->phase == VS_INVITING && on ? "等待接受…" : NULL;
        if (right) {
            snprintf(t, sizeof(t), "%s", right);
        } else {
            snprintf(t, sizeof(t), "%s · %u 词", NW_RANK_NAME[nw_rank_of(p->rating)], p->mastered);
        }
        nw_text(g, NW_F12, t, 218, y + 15, NW_RIGHT, RGB(right ? C_GOLD : C_MUTED));
    }
    bottom(V->phase == VS_INVITING ? "已发出挑战，等对方按 OK" : "OK 发起挑战 · 长按上键 返回");
}

static void count(const nw_vs_t *V)
{
    char t[72];
    int left = 3 - (int)((G->now - V->t0) / 1000);
    snprintf(t, sizeof(t), "%d", left < 1 ? 1 : left);
    nw_text(&G->gfx, NW_W36, t, 120, 120, NW_CENTER, RGB(C_GOLD));
    char name[40];
    nw_vs_name(&V->opp, name, sizeof(name));
    snprintf(t, sizeof(t), "对手：%s", name);
    nw_text(&G->gfx, NW_F16, t, 120, 184, NW_CENTER, RGB(C_WHITE));
    snprintf(t, sizeof(t), "题库：双方都学过的 %d 个词", V->pool);
    nw_text(&G->gfx, NW_F12, t, 120, 212, NW_CENTER, RGB(C_MUTED));
    bottom("10 题抢答，每题 8 秒，最后一题分数翻倍");
}

static void question(const nw_vs_t *V)
{
    nw_gfx_t *g = &G->gfx;
    char t[48];
    const nw_question_t *q = &V->qs[V->qi];
    bool rev = V->phase == VS_REVEAL;
    snprintf(t, sizeof(t), "我 %d", V->score[0]);
    nw_text(g, NW_F12, t, 18, 14, NW_LEFT, RGB(C_TEAL));
    snprintf(t, sizeof(t), "%d 对手", V->score[1]);
    nw_text(g, NW_F12, t, 222, 14, NW_RIGHT, RGB(C_RIVAL));
    snprintf(t, sizeof(t), "%d/%d%s", V->qi + 1, NW_BATTLE_Q, V->qi == NW_BATTLE_Q - 1 ? " ×2" : "");
    nw_text(g, NW_M11, t, 120, 14, NW_CENTER, RGB(C_MUTED));
    int el = (int)(G->now - V->t0);
    int left = rev || V->my_pick >= 0 ? 0 : NW_BATTLE_MS - el;
    if (left < 0) left = 0;
    nw_rect(g, 12, 26, 216, 4, 2, RGB(0x2A3350));
    int w = 216 * left / NW_BATTLE_MS;
    nw_rect(g, 12, 26, w < 4 ? 4 : w, 4, 2, RGB(left * 10 < NW_BATTLE_MS * 3 ? C_RED : C_TEAL));
    nw_text_fit(g, F_WORD, 4, nw_db_word(q->idx), 120, 76, 210, NW_CENTER, RGB(C_WHITE));
    const char *ipa = nw_db_ipa(q->idx);
    if (*ipa) {
        snprintf(t, sizeof(t), "/%s/", ipa);
        nw_text(g, NW_IPA, t, 120, 108, NW_CENTER, RGB(C_MUTED));
    }
    int op = V->op_pick[V->qi];
    for (int k = 0; k < 3; k++) {
        int y = 136 + k * 44;
        uint32_t bg = 0x222B48, bd = 0x222B48;
        if (rev) {
            if (k == q->answer) bg = bd = C_GREEN;
            else if (k == V->my_pick) bg = bd = C_RED;
        } else if (V->my_pick == k) {
            bd = C_GOLD;
            bg = 0x2E3860;
        } else if (V->my_pick < 0 && k == G->sel) {
            bd = C_TEAL;
        }
        nw_rect_line(g, 16, y, 208, 36, 8, RGB(bg), 255, RGB(bd), 2);
        nw_text_fit(g, F_OPT, 2, nw_db_meaning(q->opt[k]), 112, y + 18, 160, NW_CENTER, RGB(C_WHITE));
        if (rev && op == k) {
            nw_rect(g, 186, y + 8, 32, 20, 10, RGB(C_RIVAL));
            nw_text(g, NW_F12, "对手", 202, y + 18, NW_CENTER, RGB(C_WHITE));
        }
    }
    if (rev) {
        if (V->gain[0]) snprintf(t, sizeof(t), "+%d", V->gain[0]);
        else snprintf(t, sizeof(t), "没得分");
        nw_text(g, NW_F16, t, 120, 286, NW_CENTER, RGB(V->gain[0] ? C_GOLD : C_MUTED));
    } else if (V->my_pick >= 0) {
        nw_text(g, NW_F12, op != -2 ? "" : "已作答，等对方…", 120, 286, NW_CENTER, RGB(C_MUTED));
    } else if (op != -2) {
        nw_text(g, NW_F16, "对方已作答！", 120, 286, NW_CENTER, RGB(C_RIVAL));
    }
}

static void result(const nw_vs_t *V)
{
    nw_gfx_t *g = &G->gfx;
    const nw_state_t *s = &G->rec.s;
    char t[64];
    int a = V->score[0], b = V->score[1];
    const char *title = a > b ? "胜利！" : a < b ? "惜败" : "平局";
    nw_text(g, NW_F22, title, 120, 52, NW_CENTER, RGB(a > b ? C_GOLD : a < b ? C_MUTED : C_WHITE));
    snprintf(t, sizeof(t), "%d : %d", a, b);
    nw_text(g, NW_M18, t, 120, 90, NW_CENTER, RGB(C_WHITE));
    const char *L[4] = {"积分", "段位", "金币", "错词"};
    char R[4][40];
    if (V->counted) {
        snprintf(R[0], 40, "%d（%s%d）", s->rating, V->res.delta >= 0 ? "+" : "", V->res.delta);
        snprintf(R[2], 40, "+%d", V->res.coins);
    } else {
        snprintf(R[0], 40, "%d（不变）", s->rating);
        snprintf(R[2], 40, "+0");
    }
    snprintf(R[1], 40, "%s", NW_RANK_NAME[nw_rank_of(s->rating)]);
    snprintf(R[3], 40, "%d 个加入复习", V->n_missed);
    for (int k = 0; k < 4; k++) {
        nw_text(g, NW_F16, L[k], 34, 132 + k * 26, NW_LEFT, RGB(C_MUTED));
        nw_text(g, NW_F16, R[k], 206, 132 + k * 26, NW_RIGHT, RGB(C_WHITE));
    }
    if (V->msg[0]) nw_text_fit(g, F_OPT + 1, 1, V->msg, 120, 250, 210, NW_CENTER, RGB(C_MUTED));
    else if (V->res.gained & NW_ACH_VS10) nw_text(g, NW_F16, "成就：对战 10 连胜", 120, 250, NW_CENTER, RGB(C_GOLD));
    sprite(120, 290);
    nw_text(g, NW_F12, "OK 回主页", 222, 304, NW_RIGHT, RGB(C_MUTED));
}

void nw_draw_vs(void)
{
    const nw_vs_t *V = G->vs;
    nw_rect(&G->gfx, 0, 0, 240, 320, 0, RGB(0x141A30));
    if (!V) return;
    switch (V->phase) {
    case VS_SEARCH:
    case VS_INVITING:
    case VS_INVITED:
    case VS_SYNC: lobby(V); break;
    case VS_COUNT: count(V); break;
    case VS_Q:
    case VS_REVEAL: question(V); break;
    case VS_WAIT:
        nw_text(&G->gfx, NW_F16, "核对比分…", 120, 150, NW_CENTER, RGB(C_WHITE));
        break;
    case VS_END: result(V); break;
    }
}
