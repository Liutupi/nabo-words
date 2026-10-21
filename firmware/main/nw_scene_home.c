/* 主页：房间、道具、书桌后的 NABO、单词精灵、顶栏、词汇量和菜单。对应原型 screens.js 的 drawHome */
#include <stdio.h>
#include <string.h>

#include "nw_assets.h"
#include "nw_game_int.h"

#define RGB nw_rgb
#define NW_SAFE 16   /* 屏幕是圆角，四角的文字往里收 */

int nw_isin(int p)
{
    /* 抛物线近似：p 是 0..1023 对应 0..2π */
    p &= 1023;
    int sgn = p < 512 ? 1 : -1;
    int x = p & 511;                  /* 0..511 → 半周期 */
    int v = (x * (512 - x)) >> 8;     /* 峰值 256 */
    return sgn * v;
}

/* 精灵和饰品。饰品图已按 placement_suggestion 给每种形态缩好，这里只按比例摆位置 */
typedef struct { int16_t xh, yh; } acc_place_t;   /* 百分比：x 是半宽的几成，y 是身高的几成（从头顶往下） */
static const acc_place_t ACC_AT[NW_ACCS] = {
    {-55, 16}, {0, 16}, {0, 88}, {55, 16}, {0, 55}, {0, 70}, {0, 57}, {0, 10},
};

void nw_draw_sprite(int stage, int form, int x, int y, int acc, int bob)
{
    if (stage > NW_STAGES - 1) stage = NW_STAGES - 1;
    nw_image_t im;
    int id = G->art_sprite[stage][form];
    if (id < 0 || !nw_art_get(id, &im)) return;
    y += bob;
    int h = im.h, hw = im.w / 2;
    bool can = acc >= 0 && acc < NW_ACCS && G->art_acc[acc][form] >= 0 &&
               !(form == 0 && (acc == 2 || acc == 4));   /* 蛋没有眼睛和脖子 */
    int ax = 0, ay = 0;
    if (can) {
        int yh = ACC_AT[acc].yh;
        if (acc == 4 && form == 1) yh = 56;
        if (acc == 4 && form == 3) yh = 53;
        ax = x + ACC_AT[acc].xh * hw / 100;
        ay = y - h + yh * h / 100;
    }
    if (can && acc == 5) nw_image(&G->gfx, G->art_acc[acc][form], ax, ay);   /* 披风在身后 */
    nw_image(&G->gfx, id, x, y);
    if (can && acc != 5) nw_image(&G->gfx, G->art_acc[acc][form], ax, ay);
}

/* 闹钟的分针：按 RTC 走 */
static void paint_hand(const nw_op_t *op, int y, int x0, int x1, uint16_t *row)
{
    int cx = op->u.custom.a, cy = op->u.custom.b, ex = op->u.custom.c, ey = op->u.custom.d;
    for (int k = 0; k <= 16; k++) {
        int px = cx + (ex - cx) * k / 16, py = cy + (ey - cy) * k / 16;
        if (py == y && px >= x0 && px < x1) row[px - x0] = op->u.custom.color;
    }
}

/* 夜晚学段墙上的星星 */
static void wall_stars(int look)
{
    if (look < 5) return;
    int ph = (int)(G->now / 500);
    for (int i = 0; i < 14; i++) {
        int x = (i * 53) % 240, y = 20 + (i * 37) % 100;
        int big = (i + ph) % 5 == 0;
        nw_rect(&G->gfx, x - 1 - big, y - 1 - big, 2 + 2 * big, 2 + 2 * big, 1 + big, RGB(0xE9E3FF));
    }
}

static bool prop_on(int i, int m) { return m >= NW_PROP[i].need && !(G->rec.e.hidden & (1u << i)); }

static void props(bool wall_layer)
{
    const nw_state_t *s = &G->rec.s;
    int m = nw_mastered(s);
    bool shelf = prop_on(5, m), laptop = prop_on(8, m);
    for (int i = 0; i < NW_PROPS; i++) {
        if (!prop_on(i, m)) continue;
        const nw_prop_t *p = &NW_PROP[i];
        bool on_wall = p->wall || (i == 3 && shelf);   /* 有书架时绿植放书架右端 */
        if (on_wall != wall_layer) continue;
        int x = p->x, y = p->y;
        if (i == 3 && shelf) { x = 226; y = 97; }
        if (i == 0 && laptop) x = 66;                   /* 笔记本电脑占了桌子前沿正中 */
        nw_image(&G->gfx, G->art_prop[i], x, y);
        if (i == 1) {   /* 闹钟分针 */
            int a = (int)((G->rtc / 60) % 60) * 1024 / 60;
            int ex = x + nw_isin(a) * 7 / 256, ey = y + 1 - nw_isin(a + 256) * 7 / 256;
            nw_custom(&G->gfx, x - 8, y - 8, x + 9, y + 9, paint_hand, NULL, x, y + 1, ex, ey, RGB(0x2A3A44), 1);
        }
    }
    /* 盲盒摆件：有书架放书架上（最近 4 个），没有就放桌沿前 */
    if (shelf != wall_layer) return;
    int n = s->toy_count < 4 ? s->toy_count : 4;
    for (int k = 0; k < n; k++) {
        int t = s->toys[s->toy_count - n + k];
        int x = shelf ? 180 + k * 11 : 12 + k * 13, y = shelf ? 97 : 211;
        nw_image(&G->gfx, G->art_toy[t], x, y);
    }
}

static void top_bar(void)
{
    const nw_state_t *s = &G->rec.s;
    nw_rect_a(&G->gfx, 0, 0, 240, 24, 0, RGB(0x121829), 210);
    char t[48];
    if (nw_all_done(s)) snprintf(t, sizeof(t), "环游完成");
    else snprintf(t, sizeof(t), "%s · %s", NW_STAGE_INFO[s->stage].name, nw_unit_city(nw_cur_unit(s)));
    static const nw_font_id_t F[] = {NW_F12};
    nw_text_fit(&G->gfx, F, 1, t, NW_SAFE + 4, 12, 116, NW_LEFT, RGB(C_WHITE));
    /* 右边从右往左排：金币、连续天数、补签卡（数字位数会变） */
    char v[12];
    int x = 226;
    snprintf(v, sizeof(v), "%u", (unsigned)s->coins);
    nw_text(&G->gfx, NW_M11, v, x, 12, NW_RIGHT, RGB(C_WHITE));
    x -= nw_text_width(NW_M11, v) + 8;
    nw_image(&G->gfx, G->art_ui[0], x, 12);
    x -= 12;
    bool done = s->done_day == s->day;
    snprintf(v, sizeof(v), "%u", s->streak);
    nw_text(&G->gfx, NW_M11, v, x, 12, NW_RIGHT, RGB(C_WHITE));
    x -= nw_text_width(NW_M11, v) + 8;
    nw_image(&G->gfx, G->art_ui[done ? 1 : 3], x, 12);
    if (s->freezes) {
        x -= 12;
        snprintf(v, sizeof(v), "%u", s->freezes);
        nw_text(&G->gfx, NW_M11, v, x, 12, NW_RIGHT, RGB(C_WHITE));
        x -= nw_text_width(NW_M11, v) + 8;
        nw_image(&G->gfx, G->art_ui[2], x, 12);
    }
}

static void menu(void)
{
    nw_rect(&G->gfx, 12, 256, 216, 56, 10, RGB(0x1E2740));
    char sub[64];
    const char *label = nw_menu_label(G->menu[G->menu_idx], sub, sizeof(sub));
    nw_text(&G->gfx, NW_F16, label, 120, 275, NW_CENTER, RGB(C_WHITE));
    static const nw_font_id_t F[] = {NW_F12};
    nw_text_fit(&G->gfx, F, 1, sub, 120, 297, 190, NW_CENTER, RGB(C_MUTED));
    nw_tri(&G->gfx, 214, 273, 6, true, RGB(0x5B6584));
    nw_tri(&G->gfx, 214, 293, 6, false, RGB(0x5B6584));
    int n = G->menu_n;
    for (int k = 0; k < n; k++) {
        int y = 266 + k * 36 / (n > 1 ? n - 1 : 1);
        bool on = k == G->menu_idx;
        nw_circle(&G->gfx, 24, y, on ? 3 : 2, RGB(on ? C_GOLD : 0x5B6584));
    }
}

void nw_draw_home(void)
{
    const nw_state_t *s = &G->rec.s;
    int look = nw_look_of(s);
    const nw_look_t *L = &NW_LOOK[look];
    nw_gfx_t *g = &G->gfx;
    nw_rect(g, 0, 0, 240, 214, 0, RGB(L->wall));
    nw_rect(g, 0, 132, 240, 3, 0, RGB(L->trim));
    wall_stars(look);
    props(true);
    /* NABO 的双手搭在桌沿上，所以先画桌子再画 NABO */
    nw_rect(g, 0, 176, 240, 6, 0, RGB(L->top));
    nw_rect(g, 0, 182, 240, 32, 0, RGB(L->desk));
    nw_rect_a(g, 0, 182, 240, 2, 0, 0, 46);
    if (G->pack_ok) {
        int frame, anim = nw_game_anim(&frame);
        nw_anim(g, anim, frame, 120, 176);
    }
    props(false);
    int si = s->stage < NW_STAGES ? s->stage : NW_STAGES - 1;
    int bob = nw_isin((int)((G->now % 2094) * 1024 / 2094) + si * 160) * 2 / 256;   /* 原型 sin(t·3) */
    nw_draw_sprite(si, nw_sprite_form(s, si), 196, 176, s->acc == NW_ACC_NONE ? -1 : s->acc, bob);
    top_bar();

    /* 底部：已学 / 掌握 + 学段进度 + 菜单 */
    nw_rect(g, 0, 214, 240, 106, 0, RGB(C_INK));
    char t[48];
    int l = nw_learned(s), m = nw_mastered(s);
    nw_text(g, NW_F12, "已学", 14, 230, NW_LEFT, RGB(C_MUTED));
    snprintf(t, sizeof(t), "%d", l);
    nw_text(g, NW_M18, t, 40, 229, NW_LEFT, RGB(C_WHITE));
    int lw = nw_text_width(NW_M18, t);
    snprintf(t, sizeof(t), "词 · 掌握 %d", m);
    nw_text(g, NW_F12, t, 42 + lw, 230, NW_LEFT, RGB(C_MUTED));
    int n = nw_stage_end[si] - nw_stage_start[si];
    int done = nw_count_box(s, nw_stage_start[si], nw_stage_end[si], 1);
    snprintf(t, sizeof(t), "%s %d%%", NW_STAGE_INFO[si].name, done * 100 / n);
    nw_text(g, NW_F12, t, 226, 230, NW_RIGHT, RGB(C_GOLD));
    nw_rect(g, 12, 242, 216, 5, 2, RGB(0x2A3350));
    int bw = 216 * done / n;
    nw_rect(g, 12, 242, bw < 5 ? 5 : bw, 5, 2, RGB(C_GOLD));
    menu();
}
