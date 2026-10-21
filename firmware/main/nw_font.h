/*
 * 文字：assets 里的 NWF1 位图字体（4 bpp，firmware/tools/pack_assets.py 生成），原地读取。
 * 一帧里所有文字的字形共用一个池；排版时只记字形位置，画的时候按行混合。
 */
#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    NW_F12, NW_F16, NW_F22,            /* 思源黑体（Noto Sans SC）：界面和中文释义 */
    NW_W36, NW_W28, NW_W20, NW_W16,    /* Fredoka：英文单词 */
    NW_IPA,                            /* Noto Sans 13 px：音标 */
    NW_M11, NW_M18,                    /* Space Mono Bold：数字 */
    NW_FONT_COUNT
} nw_font_id_t;

typedef struct {
    const uint8_t *data;
    uint16_t count;
    uint8_t px, line_h;
    int8_t ascent;
} nw_font_t;

bool nw_fonts_open(void);
const nw_font_t *nw_font(nw_font_id_t id);

#define NW_GLYPH_POOL 360

typedef struct {
    const uint8_t *bits;
    int16_t x, y;
    uint8_t w, h;
} nw_glyph_t;

typedef struct {
    nw_glyph_t g[NW_GLYPH_POOL];
    int used;
} nw_glyph_pool_t;

typedef struct {
    uint16_t first, count;
    int16_t x0, y0, x1, y1;   /* 包围盒（不含右下） */
    int16_t w;
} nw_text_t;

typedef enum { NW_LEFT, NW_CENTER, NW_RIGHT } nw_align_t;

int nw_text_width(nw_font_id_t f, const char *utf8);
/* y 是文字竖直方向的中线（和原型 textBaseline='middle' 一致） */
bool nw_text_layout(nw_glyph_pool_t *pool, nw_text_t *t, nw_font_id_t f, const char *utf8, int x, int y,
                    nw_align_t align);
void nw_text_row(const nw_glyph_pool_t *pool, const nw_text_t *t, int y, int x0, int x1, uint16_t *row,
                 uint16_t color);
uint32_t nw_utf8_next(const char **s);

static inline uint16_t nw_blend(uint16_t dst, uint16_t src, int a)   /* a: 0..256 */
{
    int r = (((src >> 11) - (dst >> 11)) * a >> 8) + (dst >> 11);
    int g = ((((src >> 5) & 63) - ((dst >> 5) & 63)) * a >> 8) + ((dst >> 5) & 63);
    int b = (((src & 31) - (dst & 31)) * a >> 8) + (dst & 31);
    return (uint16_t)((r << 11) | (g << 5) | b);
}
