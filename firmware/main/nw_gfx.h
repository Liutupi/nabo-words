/*
 * 渲染核心（思路同牌桌 nabo_table_gfx）：每帧由画面函数重新生成一份绘制列表，
 * 和上一帧逐项比较，变了的项（新旧包围盒）记为脏矩形，只把脏矩形按条带合成后送屏。
 * 只用整数运算（ESP32-C3 没有浮点单元）。
 */
#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "nw_font.h"
#include "nw_sprite.h"

#define NW_W 240
#define NW_H 320
#define NW_MAX_OPS 96
#define NW_MAX_RECTS 12

typedef enum {
    NW_OP_RECT,     /* 圆角矩形（r = 0 是直角），可半透明，可描边 */
    NW_OP_TEXT,
    NW_OP_IMAGE,    /* art.bin 里的静态图，带 alpha */
    NW_OP_ANIM,     /* NABO 动画（全局只有一个播放器） */
    NW_OP_TRI,      /* 小三角箭头 */
    NW_OP_CUSTOM,   /* 画面自己的逐行绘制函数（星图、虚线圈、指针……） */
} nw_op_kind_t;

struct nw_op;
typedef void (*nw_paint_fn)(const struct nw_op *op, int y, int x0, int x1, uint16_t *row);

typedef struct nw_op {
    uint8_t kind;
    int16_t x0, y0, x1, y1;     /* 包围盒，已裁到屏幕内 */
    uint32_t key;               /* 影响像素的全部内容的哈希 */
    union {
        struct { int16_t x, y, w, h, r; uint16_t fill, line; uint8_t alpha, lw; } rect;
        struct { nw_text_t t; uint16_t color; } text;
        struct { int16_t x, y; uint16_t id; uint8_t alpha; } image;
        struct { int16_t x, y; uint8_t anim, frame; } anim;
        struct { int16_t cx, cy; uint16_t color; uint8_t up, size; } tri;
        struct { nw_paint_fn fn; const void *ctx; int32_t a, b, c, d; uint16_t color; } custom;
    } u;
} nw_op_t;

typedef struct {
    nw_op_t ops[NW_MAX_OPS], prev[NW_MAX_OPS];
    int n, prev_n;
    nw_glyph_pool_t glyphs;
    bool full;
    bool overflow;              /* 本帧绘制列表或字形池满了（宿主测试检查） */
    nw_nts_player_t *nabo;      /* NW_OP_ANIM 用的播放器 */
    uint32_t pixels;            /* 上一帧送了多少像素 */
} nw_gfx_t;

void nw_gfx_init(nw_gfx_t *g, nw_nts_player_t *nabo);
void nw_gfx_begin(nw_gfx_t *g);
void nw_gfx_invalidate(nw_gfx_t *g);
void nw_gfx_flush(nw_gfx_t *g);

void nw_rect(nw_gfx_t *g, int x, int y, int w, int h, int r, uint16_t fill);
void nw_rect_a(nw_gfx_t *g, int x, int y, int w, int h, int r, uint16_t fill, int alpha);
void nw_rect_line(nw_gfx_t *g, int x, int y, int w, int h, int r, uint16_t fill, int alpha, uint16_t line, int lw);
void nw_circle(nw_gfx_t *g, int cx, int cy, int r, uint16_t fill);
void nw_text(nw_gfx_t *g, nw_font_id_t f, const char *s, int x, int y, nw_align_t al, uint16_t color);
/* 依次试 fonts 里的字号，用第一个宽度不超过 max_w 的；都超过就用最后一个（可能被裁） */
void nw_text_fit(nw_gfx_t *g, const nw_font_id_t *fonts, int n, const char *s, int x, int y, int max_w,
                 nw_align_t al, uint16_t color);
bool nw_image(nw_gfx_t *g, int id, int ax, int ay);        /* 锚点放在 (ax, ay) */
void nw_anim(nw_gfx_t *g, int anim, int frame, int ax, int ay);
void nw_tri(nw_gfx_t *g, int cx, int cy, int size, bool up, uint16_t color);
void nw_custom(nw_gfx_t *g, int x0, int y0, int x1, int y1, nw_paint_fn fn, const void *ctx, int32_t a, int32_t b,
               int32_t c, int32_t d, uint16_t color, uint32_t key);

static inline uint16_t nw_rgb(uint32_t hex)
{
    return (uint16_t)(((hex >> 19) & 31) << 11 | ((hex >> 10) & 63) << 5 | ((hex >> 3) & 31));
}
uint32_t nw_hash(uint32_t h, const void *data, int n);
