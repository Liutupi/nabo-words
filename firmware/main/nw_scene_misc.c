/* 星图、护照、书桌、小卖部、对战占位页，以及所有画面上层的提示条和长按进度。对应原型 screens.js */
#include <stdio.h>
#include <string.h>

#include "nw_game_int.h"
#include "words/nabo_words_battle.h"

#define RGB nw_rgb

static const nw_font_id_t F_SMALL[] = {NW_F16, NW_F12};
static const nw_font_id_t F_12[] = {NW_F12};

static void title(const char *t, uint32_t bg)
{
    (void)bg;
    nw_text(&G->gfx, NW_F16, t, 20, 20, NW_LEFT, RGB(C_WHITE));
}

static void coins_tr(void)
{
    char v[12];
    snprintf(v, sizeof(v), "%u", (unsigned)G->rec.s.coins);
    nw_image(&G->gfx, G->art_ui[0], 192, 20);
    nw_text(&G->gfx, NW_M11, v, 201, 20, NW_LEFT, RGB(C_WHITE));
}

/* ---------- 星图：7500 个点，100 列 × 75 行 ---------- */

static const uint32_t STAR_COL[8] = {0x1C2440, 0x1F5A63, 0x2A7C82, 0x3FB8A9, 0xE8B84A, 0xF2CF73, 0xFBE3A3, 0xFFFFFF};
#define SX0 10
#define SY0 44
/* 第 i 列（行）点的左上角，间距 2.2 px */
static inline int star_pos(int i, int base) { return base + i * 22 / 10; }

static void paint_stars(const nw_op_t *op, int y, int x0, int x1, uint16_t *row)
{
    const uint8_t *box = (const uint8_t *)op->u.custom.ctx;
    int ph = op->u.custom.a, placed = op->u.custom.b;   /* 分级测试跳过的词用浅色 */
    /* 这一行落在哪一行点上（每行点 2 px 高） */
    int r = (y - SY0) * 10 / 22;
    if (r < 0 || r >= 75) return;
    int top = star_pos(r, SY0);
    if (y - top >= 2) return;
    uint16_t col[8];
    for (int k = 0; k < 8; k++) col[k] = RGB(STAR_COL[k]);
    for (int c = 0; c < 100; c++) {
        int x = star_pos(c, SX0), i = r * 100 + c, b = box[i];
        int sz = b ? 2 : 1;
        if (!b && y - top >= 1) continue;
        uint16_t v = (b >= 6 && (i * 7 + ph) % 23 == 0) ? 0xFFFF : col[b];
        if (b == 7 && i < placed) v = RGB(0x8C7F5E);
        for (int k = 0; k < sz; k++) {
            if (x + k >= x0 && x + k < x1) row[x + k - x0] = v;
        }
    }
}

void nw_draw_stars(void)
{
    nw_gfx_t *g = &G->gfx;
    const nw_state_t *s = &G->rec.s;
    nw_rect(g, 0, 0, 240, 320, 0, RGB(0x0B1020));
    title("词汇星图", 0);
    char t[64];
    snprintf(t, sizeof(t), "%d / %d", nw_mastered(s), NW_TOTAL);
    nw_text(g, NW_M11, t, 222, 20, NW_RIGHT, RGB(C_GOLD));
    int ph = (int)(G->now / 333);
    uint32_t key = nw_hash(2166136261u, s->box, NW_TOTAL);
    nw_custom(g, SX0, SY0, SX0 + 222, SY0 + 166, paint_stars, s->box, ph, G->rec.e.placed_end, 0, 0, 0, key);
    /* 学段分界线 */
    for (int st = 0; st < NW_STAGES - 1; st++) {
        int y = star_pos(nw_stage_end[st] / 100, SY0) - 1;
        nw_rect_a(g, SX0, y, 220, 1, 0, RGB(C_GOLD), 90);
    }
    if (G->rec.e.placed_end) {
        nw_rect(g, 18, 214, 8, 8, 2, RGB(0x8C7F5E));
        nw_text(g, NW_F12, "分级测试跳过的词", 29, 218, NW_LEFT, RGB(C_MUTED));
    }
    static const char *const LG[5] = {"没学", "刚学", "熟悉", "掌握", "毕业"};
    static const int LB[5] = {0, 1, 3, 4, 7};
    for (int k = 0; k < 5; k++) {
        nw_rect(g, 18 + k * 42, 232, 8, 8, 2, RGB(STAR_COL[LB[k]]));
        nw_text(g, NW_F12, LG[k], 29 + k * 42, 236, NW_LEFT, RGB(C_MUTED));
    }
    int fam = 0, grad = 0;
    for (int i = 0; i < NW_TOTAL; i++) {
        fam += s->box[i] >= 3;
        grad += s->box[i] == 7;
    }
    snprintf(t, sizeof(t), "学过 %d · 熟悉 %d · 毕业 %d", nw_learned(s), fam, grad);
    nw_text(g, NW_F12, t, 120, 266, NW_CENTER, RGB(0xC5CADB));
    nw_text(g, NW_F12, "OK 返回", 20, 302, NW_LEFT, RGB(C_MUTED));
}

/* ---------- 护照：每个学段一页，上下键逐个看城市章 ---------- */

static const uint32_t INK[NW_STAGES] = {0xC0503A, 0x2F6FD6, 0x6A4BC4, 0xD9762B, 0x178F96, 0xC2457E};

void nw_draw_passport(void)
{
    nw_gfx_t *g = &G->gfx;
    const nw_state_t *s = &G->rec.s;
    int u = G->page_unit, st = nw_unit_stage(u);
    const nw_stage_info_t *si = &NW_STAGE_INFO[st];
    nw_rect(g, 0, 0, 240, 320, 0, RGB(0xF3EBDA));
    nw_rect(g, 0, 0, 240, 40, 0, RGB(0x24345C));
    char t[64];
    snprintf(t, sizeof(t), "护照 · %s", si->region);
    nw_text(g, NW_F16, t, 20, 21, NW_LEFT, RGB(0xF3EBDA));
    snprintf(t, sizeof(t), "%d/%d", st + 1, NW_STAGES);
    nw_text(g, NW_M11, t, 222, 21, NW_RIGHT, RGB(0xAEB8D6));
    int u0 = nw_stage_start[st] / NW_UNIT, nu = (nw_stage_end[st] - nw_stage_start[st]) / NW_UNIT;
    int cols = nu > 30 ? 7 : nu > 20 ? 6 : nu > 16 ? 5 : 4;
    int cell = 216 / cols, r = cell / 2 - 3, stamped = 0;
    uint16_t ink = RGB(INK[st]);
    for (int k = 0; k < nu; k++) {
        int cx = 12 + cell * (k % cols) + cell / 2, cy = 50 + cell * (k / cols) + cell / 2;
        bool done = nw_unit_stamped(s, u0 + k), sel = u0 + k == u;
        stamped += done;
        if (sel) nw_rect(g, cx - r - 3, cy - r - 3, 2 * r + 6, 2 * r + 6, r + 3, RGB(0xE2D5B8));
        if (done) {
            nw_rect_line(g, cx - r, cy - r, 2 * r, 2 * r, r, RGB(0xF3EBDA), 255, ink, 2);
            /* 章里写城市的第一个字 */
            const char *c = nw_unit_city(u0 + k), *p = c;
            nw_utf8_next(&p);
            char first[8] = {0};
            memcpy(first, c, (size_t)(p - c) < 7 ? (size_t)(p - c) : 7);
            nw_text(g, r >= 14 ? NW_F16 : NW_F12, first, cx, cy, NW_CENTER, ink);
        } else {
            nw_rect_line(g, cx - r, cy - r, 2 * r, 2 * r, r, 0, 0, RGB(0xD3C8B0), 1);
        }
    }
    /* 选中的城市：名字和进度 */
    int fam = nw_count_box(s, u * NW_UNIT, (u + 1) * NW_UNIT, NW_FAMILIAR);
    int y = 268;
    nw_rect(g, 14, y - 18, 212, 34, 8, RGB(0xFFFFFF));
    snprintf(t, sizeof(t), "第 %d 站 %s", u - u0 + 1, nw_unit_city(u));
    nw_text_fit(g, F_SMALL, 2, t, 24, y - 1, 120, NW_LEFT, RGB(0x3A3328));
    if (fam == NW_UNIT) snprintf(t, sizeof(t), "已盖章");
    else snprintf(t, sizeof(t), "熟悉 %d/%d", fam, NW_UNIT);
    nw_text(g, NW_F12, t, 216, y - 1, NW_RIGHT, fam == NW_UNIT ? ink : RGB(0x9A8F78));
    snprintf(t, sizeof(t), "已盖 %d/%d", stamped, nu);
    nw_text(g, NW_F12, t, 20, 302, NW_LEFT, RGB(0x7A6F5A));
    nw_text(g, NW_F12, "上下键翻看 · OK 返回", 222, 302, NW_RIGHT, RGB(0x9A8F78));
}

/* ---------- 书桌 ---------- */

static void list_row(int y, bool sel, const char *l, const char *r, uint16_t lc, uint16_t rc, int icon)
{
    if (sel) nw_rect(&G->gfx, 10, y, 220, 24, 6, RGB(0x26304D));
    int x = 20;
    if (icon >= 0 && nw_image(&G->gfx, icon, 26, y + 18)) x = 38;
    nw_text_fit(&G->gfx, F_12, 1, l, x, y + 12, 112, NW_LEFT, lc);
    nw_text_fit(&G->gfx, F_12, 1, r, 222, y + 12, 96, NW_RIGHT, rc);
}

static void footer(const char *idle)
{
    if (G->toast[0]) nw_text_fit(&G->gfx, F_12, 1, G->toast, 120, 302, 200, NW_CENTER, RGB(C_GOLD));
    else nw_text(&G->gfx, NW_F12, idle, 20, 302, NW_LEFT, RGB(C_MUTED));
}

void nw_draw_desk(void)
{
    nw_gfx_t *g = &G->gfx;
    nw_rect(g, 0, 0, 240, 320, 0, RGB(C_INK));
    title("书桌", 0);
    coins_tr();
    int n = nw_desk_count(), m = nw_mastered(&G->rec.s);
    int top = G->list_sel - 4;
    if (top > n - 10) top = n - 10;
    if (top < 0) top = 0;
    for (int k = 0; k < 10 && top + k < n; k++) {
        int i = top + k, y = 40 + k * 25;
        char r[32] = "";
        if (i == NW_PROPS) {
            list_row(y, i == G->list_sel, "返回", "", RGB(C_WHITE), RGB(C_MUTED), -1);
            continue;
        }
        const nw_prop_t *p = &NW_PROP[i];
        bool locked = m < p->need, on = !(G->rec.e.hidden & (1u << i));
        if (locked) snprintf(r, sizeof(r), "掌握 %d 词解锁", p->need);
        else snprintf(r, sizeof(r), "%s", on ? "摆着" : "收起");
        list_row(y, i == G->list_sel, p->name, r, RGB(locked ? 0x5B6584 : C_WHITE),
                 RGB(locked ? 0x5B6584 : on ? C_TEAL : C_MUTED), -1);
    }
    footer("OK 摆放 / 收起");
}

/* ---------- 小卖部 ---------- */

void nw_draw_shop(void)
{
    nw_gfx_t *g = &G->gfx;
    const nw_state_t *s = &G->rec.s;
    nw_rect(g, 0, 0, 240, 320, 0, RGB(C_INK));
    title("小卖部", 0);
    coins_tr();
    /* 右上预览：选中饰品时预览它，否则是现在戴的 */
    int si = s->stage < NW_STAGES ? s->stage : NW_STAGES - 1;
    int sel = G->list_sel, n = nw_shop_count();
    int preview = sel >= 2 && sel < 2 + NW_ACCS ? sel - 2 : (s->acc == NW_ACC_NONE ? -1 : s->acc);
    nw_rect(g, 150, 32, 76, 64, 8, RGB(0x1E2740));
    int form = nw_sprite_form(s, si);
    nw_draw_sprite(si, form < 1 ? 1 : form, 188, 90, preview, 0);
    nw_text(g, NW_F12, "戴饰品的样子", 20, 60, NW_LEFT, RGB(C_MUTED));
    int top = sel - 4;
    if (top > n - 8) top = n - 8;
    if (top < 0) top = 0;
    for (int k = 0; k < 8 && top + k < n; k++) {
        int i = top + k, icon;
        char l[48], r[48];
        nw_shop_item(i, l, sizeof(l), r, sizeof(r), &icon);
        list_row(102 + k * 24, i == sel, l, r, RGB(C_WHITE), RGB(!strcmp(r, "戴着") ? C_TEAL : C_MUTED), icon);
    }
    footer("OK 购买 / 戴上");
}

/* ---------- 上层：提示条、长按返回的进度 ---------- */

static void paint_ring(const nw_op_t *op, int y, int x0, int x1, uint16_t *row)
{
    int cx = op->u.custom.a, cy = op->u.custom.b, ang = op->u.custom.c;   /* ang 0..1024 */
    int dy = y - cy;
    for (int x = x0; x < x1; x++) {
        int dx = x - cx, d2 = dx * dx + dy * dy;
        if (d2 < 36 || d2 > 90) continue;
        /* 从 12 点顺时针的角度，用象限 + 斜率粗算 */
        int a;
        int ax = dx < 0 ? -dx : dx, ay = dy < 0 ? -dy : dy;
        int t = ax + ay ? ax * 256 / (ax + ay) : 0;   /* 0..256 = 0..90° */
        if (dx >= 0 && dy < 0) a = t;
        else if (dx >= 0) a = 512 - t;
        else if (dy >= 0) a = 512 + t;
        else a = 1024 - t;
        if (a <= ang) row[x - x0] = op->u.custom.color;
    }
}

void nw_draw_overlay(void)
{
    nw_gfx_t *g = &G->gfx;
    if (G->toast[0] && G->scene != SC_DESK && G->scene != SC_SHOP && G->scene != SC_WEEK) {
        nw_rect_a(g, 20, 34, 200, 26, 13, RGB(0x121829), 235);
        nw_text_fit(g, F_12, 1, G->toast, 120, 47, 186, NW_CENTER, RGB(C_GOLD));
    }
    if (G->up_held && !G->up_long && G->scene != SC_HOME) {
        uint32_t held = G->now - G->up_t;
        if (held > 250) {
            int ang = (int)(held > 1000 ? 1024 : held * 1024 / 1000);
            nw_custom(g, 212, 6, 232, 26, paint_ring, NULL, 222, 16, ang, 0, RGB(C_GOLD), 7);
        }
    }
}
