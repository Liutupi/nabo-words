/* 学习 / 结业考的答题页和学习小结。对应原型 screens.js 的 drawStudy / drawSummary */
#include <stdio.h>
#include <string.h>

#include "nw_db.h"
#include "nw_game_int.h"

#define RGB nw_rgb

static const nw_font_id_t F_WORD[] = {NW_W36, NW_W28, NW_W20, NW_W16};
static const nw_font_id_t F_MEAN[] = {NW_F22, NW_F16, NW_F12};
static const nw_font_id_t F_OPT_ZH[] = {NW_F16, NW_F12};
static const nw_font_id_t F_OPT_EN[] = {NW_W20, NW_W16};
static const nw_font_id_t F_SMALL[] = {NW_F16, NW_F12};

static void hint(const char *s) { nw_text(&G->gfx, NW_F12, s, 18, 306, NW_LEFT, RGB(0x9A8F78)); }

static void ipa_line(uint16_t idx, int y)
{
    const char *p = nw_db_ipa(idx);
    if (!*p) return;
    char t[64];
    snprintf(t, sizeof(t), "/%s/", p);
    nw_text(&G->gfx, NW_IPA, t, 120, y, NW_CENTER, RGB(0x7A6F5A));
}

/* 右下角的单词精灵：答对跳一下，答错晃一下 */
static void corner_sprite(void)
{
    const nw_state_t *s = &G->rec.s;
    int si = s->stage < NW_STAGES ? s->stage : NW_STAGES - 1;
    int dt = (int)(G->now - G->mood_t), bob = 0, dx = 0;
    if (G->mood == 1) bob = -(nw_isin(dt * 1024 / 400) < 0 ? -nw_isin(dt * 1024 / 400) : nw_isin(dt * 1024 / 400)) * 6 / 256;
    if (G->mood == 2) dx = nw_isin(dt * 1024 / 250) * 3 / 256;
    nw_draw_sprite(si, nw_sprite_form(s, si), 208 + dx, 314, s->acc == NW_ACC_NONE ? -1 : s->acc, bob);
}

void nw_draw_study(void)
{
    nw_gfx_t *g = &G->gfx;
    const nw_session_t *sess = G->sess;
    const nw_question_t *q = &sess->q[sess->pos];
    uint16_t idx = q->idx;
    nw_rect(g, 0, 0, 240, 320, 0, RGB(C_PAPER));
    /* 进度 */
    nw_rect(g, 20, 12, 200, 5, 2, RGB(0xEADFC9));
    int w = 200 * sess->pos / (sess->len ? sess->len : 1);
    nw_rect(g, 20, 12, w < 5 ? 5 : w, 5, 2, RGB(sess->exam ? C_RED : C_TEAL));
    char t[64];
    if (sess->exam) snprintf(t, sizeof(t), "结业考 %d/%d", sess->pos + 1, sess->total);
    else snprintf(t, sizeof(t), "%s", (q->flags & NW_QF_RETRY) ? "再记一次" : q->type == NW_Q_INTRO ? "新词" : (q->flags & NW_QF_NEW) ? "新词检测" : "复习");
    nw_text(g, NW_F12, t, 20, 30, NW_LEFT, RGB(sess->exam ? C_RED : 0x7A6F5A));
    nw_text(g, NW_F12, nw_unit_city(idx / NW_UNIT), 220, 30, NW_RIGHT, RGB(0xB0A68F));

    const char *word = nw_db_word(idx), *mean = nw_db_meaning(idx), *pos = nw_db_pos(idx);
    if (q->type == NW_Q_INTRO) {
        nw_text_fit(g, F_WORD, 4, word, 120, 96, 210, NW_CENTER, RGB(C_INK));
        ipa_line(idx, 130);
        if (*pos) nw_text(g, NW_IPA, pos, 120, 152, NW_CENTER, RGB(0x9A8F78));
        nw_text_fit(g, F_MEAN, 3, mean, 120, 180, 210, NW_CENTER, RGB(C_INK));
        hint("OK 记住了");
    } else if (q->type == NW_Q_FLASH) {
        nw_text_fit(g, F_WORD, 4, word, 120, 100, 210, NW_CENTER, RGB(C_INK));
        ipa_line(idx, 134);
        if (G->flipped) {
            if (*pos) nw_text(g, NW_IPA, pos, 120, 158, NW_CENTER, RGB(0x9A8F78));
            nw_text_fit(g, F_MEAN, 3, mean, 120, 184, 210, NW_CENTER, RGB(C_INK));
            hint(G->fb ? "OK 继续" : "上键 认识 · 下键 不认识");
        } else {
            hint("OK 翻面");
        }
        if (G->fb) nw_text(g, NW_F16, G->fb_ok ? "√ 认识" : "再看一眼", 120, 224, NW_CENTER, RGB(G->fb_ok ? C_GREEN : C_RED));
    } else {
        if (q->type == NW_Q_EN2ZH) {
            nw_text_fit(g, F_WORD, 4, word, 120, 76, 210, NW_CENTER, RGB(C_INK));
            ipa_line(idx, 108);
        } else if (q->type == NW_Q_ZH2EN) {
            nw_text_fit(g, F_MEAN, 3, mean, 120, 86, 210, NW_CENTER, RGB(C_INK));
        } else {
            char low[40];
            size_t n = strlen(word);
            if (n >= sizeof(low)) n = sizeof(low) - 1;
            for (size_t k = 0; k < n; k++) low[k] = (char)(word[k] >= 'A' && word[k] <= 'Z' ? word[k] + 32 : word[k]);
            low[n] = 0;
            if (!G->fb && q->pos < n) low[q->pos] = '_';
            nw_text_fit(g, F_WORD, 4, low, 120, 72, 210, NW_CENTER, RGB(C_INK));
            nw_text_fit(g, F_SMALL, 2, mean, 120, 108, 210, NW_CENTER, RGB(0x7A6F5A));
        }
        for (int k = 0; k < q->n_opt; k++) {
            int y = 140 + k * 44;
            uint32_t bg = 0xFFFFFF, fg = C_INK, bd = 0xE6DCC6;
            if (G->fb) {
                if (k == q->answer) { bg = C_GREEN; fg = 0xFFFFFF; bd = bg; }
                else if (k == G->fb_pick) { bg = C_RED; fg = 0xFFFFFF; bd = bg; }
            } else if (k == G->sel) {
                bd = C_TEAL;
            }
            nw_rect_line(g, 16, y, 208, 36, 8, RGB(bg), 255, RGB(bd), k == G->sel && !G->fb ? 3 : 2);
            if (q->type == NW_Q_LETTER) {
                char c[2] = {(char)q->opt[k], 0};
                nw_text(g, NW_W28, c, 120, y + 18, NW_CENTER, RGB(fg));
            } else if (q->type == NW_Q_EN2ZH) {
                nw_text_fit(g, F_OPT_ZH, 2, nw_db_meaning(q->opt[k]), 120, y + 18, 190, NW_CENTER, RGB(fg));
            } else {
                nw_text_fit(g, F_OPT_EN, 2, nw_db_word(q->opt[k]), 120, y + 18, 190, NW_CENTER, RGB(fg));
            }
        }
        if (G->fb && !G->fb_ok) {
            snprintf(t, sizeof(t), "%s  %s", word, mean);
            nw_text_fit(g, F_SMALL + 1, 1, t, 110, 282, 180, NW_CENTER, RGB(0x7A6F5A));
            hint("OK 继续");
        } else if (!G->fb) {
            hint("上下键选择 · OK 确认");
        }
    }
    corner_sprite();
}

void nw_draw_summary(void)
{
    nw_gfx_t *g = &G->gfx;
    const nw_state_t *s = &G->rec.s;
    nw_rect(g, 0, 0, 240, 320, 0, RGB(C_INK));
    nw_text(g, NW_F22, G->title, 120, 38, NW_CENTER, RGB(C_WHITE));
    for (int k = 0; k < G->n_rows; k++) {
        nw_text(g, NW_F16, G->row_l[k], 32, 80 + k * 26, NW_LEFT, RGB(C_MUTED));
        nw_text(g, NW_F16, G->row_r[k], 208, 80 + k * 26, NW_RIGHT, RGB(C_WHITE));
    }
    int y = 92 + G->n_rows * 26;
    for (int k = 0; k < G->n_notes && k < 5; k++) {
        nw_text_fit(g, F_SMALL, 2, G->notes[k], 120, y, 210, NW_CENTER, RGB(C_GOLD));
        y += 22;
    }
    int si = s->stage < NW_STAGES ? s->stage : NW_STAGES - 1;
    int bob = nw_isin((int)((G->now % 2094) * 1024 / 2094)) * 2 / 256;
    nw_draw_sprite(si, nw_sprite_form(s, si), 120, 296, s->acc == NW_ACC_NONE ? -1 : s->acc, bob);
    nw_text(g, NW_M11, "OK", 222, 306, NW_RIGHT, RGB(C_MUTED));
}
