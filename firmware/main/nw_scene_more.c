/* 开局（选家乡、要不要分级测试）、分级测试、闪电挑战 */
#include <stdio.h>
#include <string.h>

#include "nw_cities.h"
#include "nw_db.h"
#include "nw_game_int.h"

#define RGB nw_rgb

static const nw_font_id_t F_WORD[] = {NW_W36, NW_W28, NW_W20, NW_W16};
static const nw_font_id_t F_OPT[] = {NW_F16, NW_F12};

/* 竖排列表：选中项居中附近，超出部分滚动 */
static void list(int y0, int rows, int n, int sel, const char *(*label)(int), const char *(*right)(int))
{
    int top = sel - rows / 2;
    if (top > n - rows) top = n - rows;
    if (top < 0) top = 0;
    for (int k = 0; k < rows && top + k < n; k++) {
        int i = top + k, y = y0 + k * 26;
        if (i == sel) nw_rect(&G->gfx, 14, y, 212, 24, 6, RGB(0x26304D));
        nw_text(&G->gfx, NW_F16, label(i), 26, y + 12, NW_LEFT, RGB(i == sel ? C_WHITE : 0xC5CADB));
        const char *r = right ? right(i) : "";
        if (*r) nw_text(&G->gfx, NW_F12, r, 214, y + 12, NW_RIGHT, RGB(C_MUTED));
    }
    if (top > 0) nw_tri(&G->gfx, 120, y0 - 6, 5, true, RGB(0x5B6584));
    if (top + rows < n) nw_tri(&G->gfx, 120, y0 + rows * 26 + 4, 5, false, RGB(0x5B6584));
}

static const char *prov_label(int i) { return NW_PROVINCES[i].name; }
static const char *prov_right(int i)
{
    static char t[16];
    if (NW_PROVINCES[i].n == 1) return "";
    snprintf(t, sizeof(t), "%d 个城市", NW_PROVINCES[i].n);
    return t;
}
static const char *city_label(int i) { return NW_PROVINCES[G->prov_sel].cities[i]; }

static void header(const char *title, const char *sub)
{
    nw_rect(&G->gfx, 0, 0, 240, 320, 0, RGB(C_INK));
    nw_text(&G->gfx, NW_F22, title, 120, 30, NW_CENTER, RGB(C_WHITE));
    if (sub) nw_text(&G->gfx, NW_F12, sub, 120, 56, NW_CENTER, RGB(C_GOLD));
}

static void hint(const char *s, uint32_t col) { nw_text(&G->gfx, NW_F12, s, 120, 304, NW_CENTER, RGB(col)); }

void nw_draw_onboard(void)
{
    if (G->ob_step == 0) {
        header("选择你的家乡", "NABO 的环游从你的家乡出发");
        list(76, 8, NW_N_PROVINCES, G->prov_sel, prov_label, prov_right);
        hint("上下键选择 · OK 下一步", C_MUTED);
    } else if (G->ob_step == 1) {
        char t[32];
        snprintf(t, sizeof(t), "%s · 选城市", NW_PROVINCES[G->prov_sel].name);
        header(t, "护照的第一枚章盖在这里");
        list(76, 8, NW_PROVINCES[G->prov_sel].n, G->city_sel, city_label, NULL);
        hint("OK 确定 · 长按上键 换省份", C_MUTED);
    } else {
        char t[48];
        snprintf(t, sizeof(t), "从%s出发！", NW_PROVINCES[G->rec.e.home_prov].cities[G->rec.e.home_city]);
        header(t, NULL);
        nw_text(&G->gfx, NW_F16, "你学过英语吗？", 120, 86, NW_CENTER, RGB(C_WHITE));
        static const char *const L[2] = {"学过，做个分级测试", "从小学第一个词开始"};
        static const char *const S[2] = {"约 3 分钟，最多 30 题", "一共 7500 词，从 hello 学起"};
        for (int k = 0; k < 2; k++) {
            int y = 118 + k * 72;
            bool on = k == G->ob_sel;
            nw_rect_line(&G->gfx, 16, y, 208, 60, 10, RGB(on ? 0x26304D : 0x1A2136), 255, RGB(on ? C_GOLD : 0x2A3350), 2);
            nw_text(&G->gfx, NW_F16, L[k], 120, y + 21, NW_CENTER, RGB(on ? C_WHITE : 0xC5CADB));
            nw_text(&G->gfx, NW_F12, S[k], 120, y + 42, NW_CENTER, RGB(C_MUTED));
        }
        hint("测出的起点之前的词记为已掌握", C_MUTED);
    }
}

/* ---------- 分级测试 ---------- */

void nw_draw_place(void)
{
    nw_gfx_t *g = &G->gfx;
    const nw_place_t *p = G->place;
    char t[64];
    if (!p) return;
    if (G->place_done) {
        header("测试完成", NULL);
        int st = p->lo;
        nw_text(g, NW_F16, "建议从这里开始", 120, 84, NW_CENTER, RGB(C_MUTED));
        snprintf(t, sizeof(t), "%s · %s", NW_STAGE_INFO[st].name, NW_STAGE_INFO[st].region);
        nw_text(g, NW_F22, t, 120, 116, NW_CENTER, RGB(C_GOLD));
        if (st > 0) snprintf(t, sizeof(t), "前面 %d 个词记为已掌握", nw_stage_start[st]);
        else snprintf(t, sizeof(t), "从小学第一个词学起，基础打牢");
        nw_text(g, NW_F12, t, 120, 144, NW_CENTER, RGB(0xC5CADB));
        const char *L[2] = {st > 0 ? "就从这里开始" : "好的，开始", "还是从小学开始"};
        int n = st > 0 ? 2 : 1;
        for (int k = 0; k < n; k++) {
            int y = 178 + k * 52;
            bool on = k == G->place_sel;
            nw_rect_line(g, 26, y, 188, 40, 10, RGB(on ? 0x26304D : 0x1A2136), 255, RGB(on ? C_GOLD : 0x2A3350), 2);
            nw_text(g, NW_F16, L[k], 120, y + 20, NW_CENTER, RGB(on ? C_WHITE : 0xC5CADB));
        }
        hint("OK 确定", C_MUTED);
        return;
    }
    const nw_question_t *q = &p->q[p->pos];
    nw_rect(g, 0, 0, 240, 320, 0, RGB(C_PAPER));
    int done = p->rounds * NW_PLACE_Q + p->pos;
    nw_rect(g, 20, 12, 200, 5, 2, RGB(0xEADFC9));
    nw_rect(g, 20, 12, 5 + 195 * done / 30, 5, 2, RGB(C_GOLD));
    snprintf(t, sizeof(t), "分级测试 · 第 %d 轮", p->rounds + 1);
    nw_text(g, NW_F12, t, 20, 30, NW_LEFT, RGB(0x7A6F5A));
    snprintf(t, sizeof(t), "%d/%d", p->pos + 1, NW_PLACE_Q);
    nw_text(g, NW_M11, t, 220, 30, NW_RIGHT, RGB(0xB0A68F));
    nw_text_fit(g, F_WORD, 4, nw_db_word(q->idx), 120, 74, 210, NW_CENTER, RGB(C_INK));
    const char *ipa = nw_db_ipa(q->idx);
    if (*ipa) {
        snprintf(t, sizeof(t), "/%s/", ipa);
        nw_text(g, NW_IPA, t, 120, 106, NW_CENTER, RGB(0x7A6F5A));
    }
    for (int k = 0; k <= q->n_opt; k++) {
        int y = 126 + k * 40;
        bool on = k == G->sel;
        nw_rect_line(g, 16, y, 208, 34, 8, RGB(0xFFFFFF), 255, RGB(on ? C_TEAL : 0xE6DCC6), on ? 3 : 2);
        const char *s = k < q->n_opt ? nw_db_meaning(q->opt[k]) : "不认识";
        nw_text_fit(g, F_OPT, 2, s, 120, y + 17, 190, NW_CENTER, RGB(k < q->n_opt ? C_INK : 0x9A8F78));
    }
    hint("不确定就选“不认识”，测得更准", 0x9A8F78);
}

/* ---------- 闪电挑战 ---------- */

void nw_draw_challenge(void)
{
    nw_gfx_t *g = &G->gfx;
    char t[64];
    if (G->c_over) {
        header("闪电挑战", NULL);
        nw_text(g, NW_F16, "60 秒答对", 120, 84, NW_CENTER, RGB(C_MUTED));
        snprintf(t, sizeof(t), "%u", G->c_score);
        nw_text(g, NW_W36, t, 120, 124, NW_CENTER, RGB(C_GOLD));
        snprintf(t, sizeof(t), "%s今天最好 %u 题", G->c_best ? "新纪录！" : "", G->rec.e.chal_best);
        nw_text(g, NW_F16, t, 120, 166, NW_CENTER, RGB(C_WHITE));
        if (G->c_coins) snprintf(t, sizeof(t), "今天第一次挑战，金币 +%d", G->c_coins);
        else if (G->c_first) snprintf(t, sizeof(t), "每答对 %d 题得 1 枚金币", NW_CHALLENGE_COIN_PER);
        else snprintf(t, sizeof(t), "金币只给每天第一次，纪录随便刷");
        nw_text(g, NW_F12, t, 120, 196, NW_CENTER, RGB(C_GOLD));
        nw_text(g, NW_F12, "答错的词已排进明天的复习", 120, 220, NW_CENTER, RGB(C_MUTED));
        const nw_state_t *s = &G->rec.s;
        int si = s->stage < NW_STAGES ? s->stage : NW_STAGES - 1;
        nw_draw_sprite(si, nw_sprite_form(s, si), 120, 290, s->acc == NW_ACC_NONE ? -1 : s->acc,
                       nw_isin((int)((G->now % 700) * 1024 / 700)) * 3 / 256);
        hint("OK 返回", C_MUTED);
        return;
    }
    const nw_question_t *q = &G->cq;
    nw_rect(g, 0, 0, 240, 320, 0, RGB(C_INK));
    int left = (int)(G->c_end - G->now);
    if (left < 0) left = 0;
    int w = 200 * left / NW_CHALLENGE_MS;
    nw_rect(g, 20, 12, 200, 6, 3, RGB(0x2A3350));
    nw_rect(g, 20, 12, w < 6 ? 6 : w, 6, 3, RGB(left < 10000 ? C_RED : C_GOLD));
    snprintf(t, sizeof(t), "%d 秒", (left + 999) / 1000);
    nw_text(g, NW_F12, t, 20, 32, NW_LEFT, RGB(C_MUTED));
    snprintf(t, sizeof(t), "答对 %u", G->c_score);
    nw_text(g, NW_F12, t, 220, 32, NW_RIGHT, RGB(C_GOLD));
    nw_text_fit(g, F_WORD, 4, nw_db_word(q->idx), 120, 84, 210, NW_CENTER, RGB(C_WHITE));
    for (int k = 0; k < q->n_opt; k++) {
        int y = 128 + k * 44;
        uint32_t bg = 0x1E2740, bd = 0x2A3350, fg = 0xE4E8F3;
        if (G->fb) {
            if (k == q->answer) { bg = C_GREEN; bd = bg; fg = 0xFFFFFF; }
            else if (k == G->fb_pick) { bg = C_RED; bd = bg; fg = 0xFFFFFF; }
        } else if (k == G->sel) {
            bd = C_GOLD;
        }
        nw_rect_line(g, 16, y, 208, 36, 8, RGB(bg), 255, RGB(bd), k == G->sel && !G->fb ? 3 : 2);
        nw_text_fit(g, F_OPT, 2, nw_db_meaning(q->opt[k]), 120, y + 18, 190, NW_CENTER, RGB(fg));
    }
    hint("长按上键 提前结束", 0x5B6584);
}
