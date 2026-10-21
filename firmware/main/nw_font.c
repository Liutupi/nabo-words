#include "nw_font.h"

#include <string.h>

#include "nw_assets.h"

static nw_font_t s_fonts[NW_FONT_COUNT];
static const char *const NAMES[NW_FONT_COUNT] = {
    "font_f12.bin", "font_f16.bin", "font_f22.bin", "font_w36.bin", "font_w28.bin",
    "font_w20.bin", "font_w16.bin", "font_ipa.bin", "font_m11.bin", "font_m18.bin",
};

bool nw_fonts_open(void)
{
    for (int i = 0; i < NW_FONT_COUNT; i++) {
        size_t size = 0;
        const uint8_t *d = nw_assets_get(NAMES[i], &size);
        if (!d || size < 12 || memcmp(d, "NWF1", 4) != 0) {
            return false;
        }
        nw_font_t *f = &s_fonts[i];
        f->data = d;
        f->count = nw_rd16(d + 4);
        f->px = d[6];
        f->line_h = d[7];
        f->ascent = (int8_t)d[8];
        if (12u + 16u * f->count > size) {
            return false;
        }
    }
    return true;
}

const nw_font_t *nw_font(nw_font_id_t id) { return &s_fonts[id]; }

uint32_t nw_utf8_next(const char **s)
{
    const uint8_t *p = (const uint8_t *)*s;
    uint32_t c = *p++;
    if (c >= 0xF0 && p[0] && p[1] && p[2]) {
        c = ((c & 0x07) << 18) | ((p[0] & 0x3F) << 12) | ((p[1] & 0x3F) << 6) | (p[2] & 0x3F);
        p += 3;
    } else if (c >= 0xE0 && p[0] && p[1]) {
        c = ((c & 0x0F) << 12) | ((p[0] & 0x3F) << 6) | (p[1] & 0x3F);
        p += 2;
    } else if (c >= 0xC0 && p[0]) {
        c = ((c & 0x1F) << 6) | (p[0] & 0x3F);
        p += 1;
    }
    *s = (const char *)p;
    return c;
}

/* 码位对应的字形表项，没有返回 NULL */
static const uint8_t *glyph(const nw_font_t *f, uint32_t cp)
{
    int lo = 0, hi = f->count - 1;
    while (lo <= hi) {
        int mid = (lo + hi) / 2;
        const uint8_t *e = f->data + 12 + 16 * mid;
        uint32_t c = nw_rd32(e);
        if (c == cp) {
            return e;
        }
        if (c < cp) {
            lo = mid + 1;
        } else {
            hi = mid - 1;
        }
    }
    return NULL;
}

int nw_text_width(nw_font_id_t id, const char *s)
{
    const nw_font_t *f = &s_fonts[id];
    if (!f->data) {
        return 0;
    }
    int w = 0;
    while (*s) {
        const uint8_t *e = glyph(f, nw_utf8_next(&s));
        w += e ? e[12] : f->px / 3;
    }
    return w;
}

bool nw_text_layout(nw_glyph_pool_t *pool, nw_text_t *t, nw_font_id_t id, const char *s, int x, int y,
                    nw_align_t align)
{
    const nw_font_t *f = &s_fonts[id];
    memset(t, 0, sizeof(*t));
    t->first = (uint16_t)pool->used;
    if (!f->data) {
        return true;
    }
    int w = nw_text_width(id, s);
    if (align == NW_CENTER) {
        x -= w / 2;
    } else if (align == NW_RIGHT) {
        x -= w;
    }
    /* 中线 → 基线：行高居中。ascent 是基线到行顶的距离 */
    int base = y - f->line_h / 2 + f->ascent;
    t->w = (int16_t)w;
    t->x0 = (int16_t)x;
    t->x1 = (int16_t)(x + w);
    t->y0 = (int16_t)(y - f->line_h / 2);
    t->y1 = (int16_t)(t->y0 + f->line_h);
    int pen = x;
    while (*s) {
        const uint8_t *e = glyph(f, nw_utf8_next(&s));
        if (!e) {
            pen += f->px / 3;
            continue;
        }
        uint8_t gw = e[8], gh = e[9];
        if (gw && gh) {
            if (pool->used >= NW_GLYPH_POOL) {
                return false;
            }
            nw_glyph_t *g = &pool->g[pool->used++];
            t->count++;
            g->bits = f->data + nw_rd32(e + 4);
            g->x = (int16_t)(pen + (int8_t)e[10]);
            g->y = (int16_t)(base + (int8_t)e[11]);
            g->w = gw;
            g->h = gh;
            if (g->x < t->x0) t->x0 = g->x;
            if (g->x + gw > t->x1) t->x1 = (int16_t)(g->x + gw);
            if (g->y < t->y0) t->y0 = g->y;
            if (g->y + gh > t->y1) t->y1 = (int16_t)(g->y + gh);
        }
        pen += e[12];
    }
    return true;
}

void nw_text_row(const nw_glyph_pool_t *pool, const nw_text_t *t, int y, int x0, int x1, uint16_t *row,
                 uint16_t color)
{
    if (y < t->y0 || y >= t->y1 || x1 <= t->x0 || x0 >= t->x1) {
        return;
    }
    for (int i = t->first; i < t->first + t->count; i++) {
        const nw_glyph_t *g = &pool->g[i];
        int gy = y - g->y;
        if (gy < 0 || gy >= g->h) {
            continue;
        }
        const uint8_t *r = g->bits + gy * ((g->w + 1) / 2);
        int a = g->x > x0 ? g->x : x0, b = g->x + g->w < x1 ? g->x + g->w : x1;
        for (int x = a; x < b; x++) {
            int k = x - g->x;
            int v = (k & 1) ? (r[k >> 1] & 15) : (r[k >> 1] >> 4);
            if (v == 15) {
                row[x - x0] = color;
            } else if (v) {
                row[x - x0] = nw_blend(row[x - x0], color, v * 17);
            }
        }
    }
}
