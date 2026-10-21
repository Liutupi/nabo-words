#include "nw_assets.h"

#include <string.h>

static const uint8_t *s_base;
static size_t s_size;
static uint32_t s_count;

static const uint8_t *s_art;
static size_t s_art_size;
static uint16_t s_art_count;

bool nw_assets_open(const void *base, size_t size)
{
    const uint8_t *b = (const uint8_t *)base;
    if (!b || size < 8 || memcmp(b, "NWA1", 4) != 0) {
        return false;
    }
    uint32_t n = nw_rd32(b + 4);
    if (8 + 32ull * n > size) {
        return false;
    }
    for (uint32_t i = 0; i < n; i++) {
        const uint8_t *e = b + 8 + 32 * i;
        if ((uint64_t)nw_rd32(e + 24) + nw_rd32(e + 28) > size) {
            return false;
        }
    }
    s_base = b;
    s_size = size;
    s_count = n;
    return true;
}

const uint8_t *nw_assets_get(const char *name, size_t *size)
{
    for (uint32_t i = 0; i < s_count; i++) {
        const uint8_t *e = s_base + 8 + 32 * i;
        if (strncmp((const char *)e, name, 24) == 0) {
            if (size) {
                *size = nw_rd32(e + 28);
            }
            return s_base + nw_rd32(e + 24);
        }
    }
    return NULL;
}

bool nw_art_open(void)
{
    s_art = nw_assets_get("art.bin", &s_art_size);
    if (!s_art || s_art_size < 8 || memcmp(s_art, "NWI1", 4) != 0) {
        s_art = NULL;
        return false;
    }
    s_art_count = nw_rd16(s_art + 4);
    return 8 + 32u * s_art_count <= s_art_size;
}

bool nw_art_get(int id, nw_image_t *out)
{
    if (!s_art || id < 0 || id >= s_art_count) {
        return false;
    }
    const uint8_t *e = s_art + 8 + 32 * id;
    out->w = nw_rd16(e + 20);
    out->h = nw_rd16(e + 22);
    out->ax = (int16_t)nw_rd16(e + 24);
    out->ay = (int16_t)nw_rd16(e + 26);
    uint32_t off = nw_rd32(e + 28);
    if (off + 3ull * out->w * out->h > s_art_size) {
        return false;
    }
    out->px = s_art + off;
    return true;
}

int nw_art_find(const char *name)
{
    for (int i = 0; s_art && i < s_art_count; i++) {
        if (strncmp((const char *)s_art + 8 + 32 * i, name, 20) == 0) {
            return i;
        }
    }
    return -1;
}
