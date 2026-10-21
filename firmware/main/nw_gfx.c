#include "nw_gfx.h"

#include <string.h>

#include "nw_assets.h"
#include "nw_platform.h"

uint32_t nw_hash(uint32_t h, const void *data, int n)
{
    const uint8_t *p = (const uint8_t *)data;
    for (int i = 0; i < n; i++) {
        h = (h ^ p[i]) * 16777619u;
    }
    return h;
}

void nw_gfx_init(nw_gfx_t *g, nw_nts_player_t *nabo)
{
    memset(g, 0, sizeof(*g));
    g->nabo = nabo;
    g->full = true;
}

void nw_gfx_begin(nw_gfx_t *g)
{
    g->n = 0;
    g->glyphs.used = 0;
    g->overflow = false;
}

void nw_gfx_invalidate(nw_gfx_t *g) { g->full = true; }

static nw_op_t *push(nw_gfx_t *g, int kind, int x0, int y0, int x1, int y1)
{
    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    if (x1 > NW_W) x1 = NW_W;
    if (y1 > NW_H) y1 = NW_H;
    if (x1 <= x0 || y1 <= y0) {
        return NULL;
    }
    if (g->n >= NW_MAX_OPS) {
        g->overflow = true;
        return NULL;
    }
    nw_op_t *op = &g->ops[g->n++];
    memset(op, 0, sizeof(*op));
    op->kind = (uint8_t)kind;
    op->x0 = (int16_t)x0;
    op->y0 = (int16_t)y0;
    op->x1 = (int16_t)x1;
    op->y1 = (int16_t)y1;
    return op;
}

/* ---------- 构建 ---------- */

void nw_rect_line(nw_gfx_t *g, int x, int y, int w, int h, int r, uint16_t fill, int alpha, uint16_t line, int lw)
{
    if (r * 2 > w) r = w / 2;
    if (r * 2 > h) r = h / 2;
    nw_op_t *op = push(g, NW_OP_RECT, x, y, x + w, y + h);
    if (!op) return;
    op->u.rect.x = (int16_t)x;
    op->u.rect.y = (int16_t)y;
    op->u.rect.w = (int16_t)w;
    op->u.rect.h = (int16_t)h;
    op->u.rect.r = (int16_t)r;
    op->u.rect.fill = fill;
    op->u.rect.line = line;
    op->u.rect.alpha = (uint8_t)(alpha < 0 ? 0 : alpha > 255 ? 255 : alpha);
    op->u.rect.lw = (uint8_t)lw;
    op->key = nw_hash(2166136261u, &op->u.rect, sizeof(op->u.rect));
}

void nw_rect(nw_gfx_t *g, int x, int y, int w, int h, int r, uint16_t fill)
{
    nw_rect_line(g, x, y, w, h, r, fill, 255, 0, 0);
}

void nw_rect_a(nw_gfx_t *g, int x, int y, int w, int h, int r, uint16_t fill, int alpha)
{
    nw_rect_line(g, x, y, w, h, r, fill, alpha, 0, 0);
}

void nw_circle(nw_gfx_t *g, int cx, int cy, int r, uint16_t fill)
{
    nw_rect(g, cx - r, cy - r, 2 * r, 2 * r, r, fill);
}

void nw_text(nw_gfx_t *g, nw_font_id_t f, const char *s, int x, int y, nw_align_t al, uint16_t color)
{
    if (!s || !*s) return;
    nw_text_t t;
    int mark = g->glyphs.used;
    if (!nw_text_layout(&g->glyphs, &t, f, s, x, y, al)) {
        g->overflow = true;
        g->glyphs.used = mark;
        return;
    }
    nw_op_t *op = push(g, NW_OP_TEXT, t.x0, t.y0, t.x1, t.y1);
    if (!op) {
        g->glyphs.used = mark;
        return;
    }
    op->u.text.t = t;
    op->u.text.color = color;
    uint32_t k = nw_hash(2166136261u, s, (int)strlen(s));
    int16_t meta[5] = {(int16_t)f, (int16_t)x, (int16_t)y, (int16_t)al, (int16_t)color};
    op->key = nw_hash(k, meta, sizeof(meta));
}

void nw_text_fit(nw_gfx_t *g, const nw_font_id_t *fonts, int n, const char *s, int x, int y, int max_w,
                 nw_align_t al, uint16_t color)
{
    if (n <= 0 || max_w <= 0 || !s) return;
    int k = 0;
    while (k < n - 1 && nw_text_width(fonts[k], s) > max_w) {
        k++;
    }
    if (nw_text_width(fonts[k], s) <= max_w) {
        nw_text(g, fonts[k], s, x, y, al, color);
        return;
    }
    /* 最小字号仍放不下时，在完整 UTF-8 字符后省略，避免盖住箭头或相邻统计。 */
    char clipped[256], dots[4] = "...";
    while (*dots && nw_text_width(fonts[k], dots) > max_w) dots[strlen(dots) - 1] = 0;
    int limit = max_w - nw_text_width(fonts[k], dots), width = 0;
    size_t used = 0;
    const char *p = s;
    while (*p) {
        const char *start = p;
        nw_utf8_next(&p);
        size_t bytes = (size_t)(p - start);
        char cp[5] = {0};
        memcpy(cp, start, bytes);
        int advance = nw_text_width(fonts[k], cp);
        if (width + advance > limit || used + bytes + sizeof(dots) > sizeof(clipped)) break;
        memcpy(clipped + used, start, bytes);
        used += bytes;
        width += advance;
    }
    memcpy(clipped + used, dots, strlen(dots) + 1);
    nw_text(g, fonts[k], clipped, x, y, al, color);
}

bool nw_image(nw_gfx_t *g, int id, int ax, int ay)
{
    nw_image_t im;
    if (!nw_art_get(id, &im)) return false;
    int x = ax - im.ax, y = ay - im.ay;
    nw_op_t *op = push(g, NW_OP_IMAGE, x, y, x + im.w, y + im.h);
    if (!op) return true;
    op->u.image.x = (int16_t)x;
    op->u.image.y = (int16_t)y;
    op->u.image.id = (uint16_t)id;
    op->key = nw_hash(2166136261u, &op->u.image, sizeof(op->u.image));
    return true;
}

void nw_anim(nw_gfx_t *g, int anim, int frame, int ax, int ay)
{
    nw_nts_player_t *pl = g->nabo;
    if (!pl || !nw_nts_player_set(pl, anim, frame)) return;
    int x = ax + pl->a.ox, y = ay + pl->a.oy;
    nw_op_t *op = push(g, NW_OP_ANIM, x, y, x + pl->a.w, y + pl->a.h);
    if (!op) return;
    op->u.anim.x = (int16_t)x;
    op->u.anim.y = (int16_t)y;
    op->u.anim.anim = (uint8_t)anim;
    op->u.anim.frame = (uint8_t)pl->frame;
    op->key = nw_hash(2166136261u, &op->u.anim, sizeof(op->u.anim)) ^ (uint32_t)(uintptr_t)pl->pack->data;
}

void nw_tri(nw_gfx_t *g, int cx, int cy, int size, bool up, uint16_t color)
{
    nw_op_t *op = push(g, NW_OP_TRI, cx - size, cy - size / 2, cx + size + 1, cy - size / 2 + size);
    if (!op) return;
    op->u.tri.cx = (int16_t)cx;
    op->u.tri.cy = (int16_t)cy;
    op->u.tri.color = color;
    op->u.tri.up = up;
    op->u.tri.size = (uint8_t)size;
    op->key = nw_hash(2166136261u, &op->u.tri, sizeof(op->u.tri));
}

void nw_custom(nw_gfx_t *g, int x0, int y0, int x1, int y1, nw_paint_fn fn, const void *ctx, int32_t a, int32_t b,
               int32_t c, int32_t d, uint16_t color, uint32_t key)
{
    nw_op_t *op = push(g, NW_OP_CUSTOM, x0, y0, x1, y1);
    if (!op) return;
    op->u.custom.fn = fn;
    op->u.custom.ctx = ctx;
    op->u.custom.a = a;
    op->u.custom.b = b;
    op->u.custom.c = c;
    op->u.custom.d = d;
    op->u.custom.color = color;
    int32_t v[5] = {a, b, c, d, color};
    op->key = nw_hash(key, v, sizeof(v)) ^ (uint32_t)(uintptr_t)fn;
}

/* ---------- 逐行绘制 ---------- */

static int isqrt(int v)
{
    if (v <= 0) return 0;
    int r = 0, bit = 1 << 30;
    while (bit > v) bit >>= 2;
    while (bit) {
        if (v >= r + bit) {
            v -= r + bit;
            r = (r >> 1) + bit;
        } else {
            r >>= 1;
        }
        bit >>= 2;
    }
    return r;
}

/* 圆角矩形在第 y 行的覆盖：返回 x 处的覆盖度 0..256 */
typedef struct {
    int l, r;           /* 完全覆盖 [l, r) */
    int cov;            /* l-1 和 r 两个边缘像素的覆盖度 */
    bool on;
} span_t;

static span_t row_span(int x, int y0, int w, int h, int r, int y)
{
    span_t s = {0, 0, 0, false};
    int dy = y - y0;
    if (dy < 0 || dy >= h || w <= 0) return s;
    int in16 = 0;
    if (r > 0 && (dy < r || dy >= h - r)) {
        int d16 = dy < r ? r * 16 - (dy * 16 + 8) : (dy - (h - r)) * 16 + 8;
        in16 = r * 16 - isqrt(r * r * 256 - d16 * d16);
    }
    int full = (in16 + 15) >> 4;
    s.l = x + full;
    s.r = x + w - full;
    s.cov = (in16 & 15) ? (16 * full - in16) * 16 : 0;
    s.on = true;
    return s;
}

static inline int span_cov(const span_t *s, int x)
{
    if (!s->on) return 0;
    if (x >= s->l && x < s->r) return 256;
    if (s->cov && (x == s->l - 1 || x == s->r)) return s->cov;
    return 0;
}

static void paint_rect(const nw_op_t *op, int y, int x0, int x1, uint16_t *row)
{
    const typeof(op->u.rect) *R = &op->u.rect;
    span_t o = row_span(R->x, R->y, R->w, R->h, R->r, y);
    if (!o.on) return;
    span_t in = {0, 0, 0, false};
    if (R->lw) {
        int r2 = R->r - R->lw;
        in = row_span(R->x + R->lw, R->y + R->lw, R->w - 2 * R->lw, R->h - 2 * R->lw, r2 < 0 ? 0 : r2, y);
    }
    int a = op->x0 > x0 ? op->x0 : x0, b = op->x1 < x1 ? op->x1 : x1;
    int alpha = R->alpha == 255 ? 256 : R->alpha;
    for (int x = a; x < b; x++) {
        int co = span_cov(&o, x);
        if (!co) continue;
        uint16_t *p = &row[x - x0];
        if (R->lw) {
            int ci = span_cov(&in, x);
            if (ci < 256) *p = nw_blend(*p, R->line, co);
            if (ci && alpha) *p = nw_blend(*p, R->fill, ci * alpha >> 8);
        } else if (alpha) {
            int k = co * alpha >> 8;
            *p = k >= 256 ? R->fill : nw_blend(*p, R->fill, k);
        }
    }
}

static void paint_image(const nw_op_t *op, int y, int x0, int x1, uint16_t *row)
{
    nw_image_t im;
    if (!nw_art_get(op->u.image.id, &im)) return;
    int sy = y - op->u.image.y;
    if (sy < 0 || sy >= im.h) return;
    const uint8_t *p = im.px + 3 * sy * im.w;
    int a = op->x0 > x0 ? op->x0 : x0, b = op->x1 < x1 ? op->x1 : x1;
    for (int x = a; x < b; x++) {
        const uint8_t *q = p + 3 * (x - op->u.image.x);
        int al = q[2];
        if (!al) continue;
        uint16_t c = (uint16_t)(q[0] | q[1] << 8);
        row[x - x0] = al == 255 ? c : nw_blend(row[x - x0], c, al + (al >> 7));
    }
}

static void paint_tri(const nw_op_t *op, int y, int x0, int x1, uint16_t *row)
{
    int s = op->u.tri.size, top = op->u.tri.cy - s / 2, dy = y - top;
    if (dy < 0 || dy >= s) return;
    int half = op->u.tri.up ? dy : s - 1 - dy;
    int a = op->u.tri.cx - half, b = op->u.tri.cx + half + 1;
    if (a < x0) a = x0;
    if (b > x1) b = x1;
    for (int x = a; x < b; x++) row[x - x0] = op->u.tri.color;
}

static void paint(const nw_gfx_t *g, const nw_op_t *op, int y, int x0, int x1, uint16_t *row)
{
    switch (op->kind) {
    case NW_OP_RECT: paint_rect(op, y, x0, x1, row); break;
    case NW_OP_TEXT: nw_text_row(&g->glyphs, &op->u.text.t, y, x0, x1, row, op->u.text.color); break;
    case NW_OP_IMAGE: paint_image(op, y, x0, x1, row); break;
    case NW_OP_ANIM:
        if (g->nabo) nw_nts_player_row(g->nabo, y - op->u.anim.y, op->u.anim.x, x0, x1, row);
        break;
    case NW_OP_TRI: paint_tri(op, y, x0, x1, row); break;
    case NW_OP_CUSTOM: op->u.custom.fn(op, y, x0, x1, row); break;
    default: break;
    }
}

/* ---------- 脏矩形与送屏 ---------- */

typedef struct { int x0, y0, x1, y1; } box_t;

static void add_box(box_t *list, int *n, box_t b)
{
    if (b.x1 <= b.x0 || b.y1 <= b.y0) return;
    for (;;) {
        bool merged = false;
        for (int i = 0; i < *n; i++) {
            box_t *o = &list[i];
            /* 相交或相距很近（省一次送屏的开销）就合并 */
            if (b.x0 <= o->x1 + 4 && o->x0 <= b.x1 + 4 && b.y0 <= o->y1 + 4 && o->y0 <= b.y1 + 4) {
                if (o->x0 < b.x0) b.x0 = o->x0;
                if (o->y0 < b.y0) b.y0 = o->y0;
                if (o->x1 > b.x1) b.x1 = o->x1;
                if (o->y1 > b.y1) b.y1 = o->y1;
                list[i] = list[--*n];
                merged = true;
                break;
            }
        }
        if (!merged) break;
    }
    if (*n < NW_MAX_RECTS) {
        list[(*n)++] = b;
        return;
    }
    /* 满了：并进面积增加最少的那个 */
    int best = 0;
    long best_cost = -1;
    for (int i = 0; i < *n; i++) {
        box_t u = list[i];
        if (b.x0 < u.x0) u.x0 = b.x0;
        if (b.y0 < u.y0) u.y0 = b.y0;
        if (b.x1 > u.x1) u.x1 = b.x1;
        if (b.y1 > u.y1) u.y1 = b.y1;
        long cost = (long)(u.x1 - u.x0) * (u.y1 - u.y0) - (long)(list[i].x1 - list[i].x0) * (list[i].y1 - list[i].y0);
        if (best_cost < 0 || cost < best_cost) {
            best_cost = cost;
            best = i;
        }
    }
    box_t u = list[best];
    list[best] = list[--*n];
    if (b.x0 < u.x0) u.x0 = b.x0;
    if (b.y0 < u.y0) u.y0 = b.y0;
    if (b.x1 > u.x1) u.x1 = b.x1;
    if (b.y1 > u.y1) u.y1 = b.y1;
    add_box(list, n, u);
}

static bool same(const nw_op_t *a, const nw_op_t *b)
{
    return a->kind == b->kind && a->key == b->key && a->x0 == b->x0 && a->y0 == b->y0 && a->x1 == b->x1 &&
           a->y1 == b->y1;
}

static void compose(nw_gfx_t *g, box_t b)
{
    int w = b.x1 - b.x0;
    int rows_max = NW_STRIP_PX / w;
    if (rows_max > NW_STRIP_ROWS) rows_max = NW_STRIP_ROWS;
    const nw_op_t *hit[NW_MAX_OPS];
    for (int y = b.y0; y < b.y1; y += rows_max) {
        int rows = b.y1 - y < rows_max ? b.y1 - y : rows_max;
        int nh = 0;
        for (int i = 0; i < g->n; i++) {
            const nw_op_t *op = &g->ops[i];
            if (op->y1 > y && op->y0 < y + rows && op->x1 > b.x0 && op->x0 < b.x1) hit[nh++] = op;
        }
        uint16_t *buf = nw_plat_strip();
        for (int r = 0; r < rows; r++) {
            uint16_t *row = buf + r * w;
            memset(row, 0, (size_t)w * 2);
            for (int k = 0; k < nh; k++) {
                if (hit[k]->y0 <= y + r && hit[k]->y1 > y + r) paint(g, hit[k], y + r, b.x0, b.x1, row);
            }
        }
        nw_plat_send(b.x0, y, w, rows, buf);
        g->pixels += (uint32_t)(w * rows);
    }
}

void nw_gfx_flush(nw_gfx_t *g)
{
    box_t list[NW_MAX_RECTS];
    int n = 0;
    g->pixels = 0;
    if (g->full) {
        list[n++] = (box_t){0, 0, NW_W, NW_H};
    } else {
        int m = g->n > g->prev_n ? g->n : g->prev_n;
        for (int i = 0; i < m; i++) {
            const nw_op_t *a = i < g->n ? &g->ops[i] : NULL, *p = i < g->prev_n ? &g->prev[i] : NULL;
            if (a && p && same(a, p)) continue;
            if (a) add_box(list, &n, (box_t){a->x0, a->y0, a->x1, a->y1});
            if (p) add_box(list, &n, (box_t){p->x0, p->y0, p->x1, p->y1});
        }
    }
    for (int i = 0; i < n; i++) compose(g, list[i]);
    g->full = false;
    memcpy(g->prev, g->ops, sizeof(nw_op_t) * (size_t)g->n);
    g->prev_n = g->n;
}
