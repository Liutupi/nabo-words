/*
 * assets 分区（firmware/tools/pack_assets.py 打包）：词库、字体、静态美术、NABO 动画。
 * 整个分区映射到地址空间里原地读取，不占内存。
 */
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

bool nw_assets_open(const void *base, size_t size);
/* 按文件名找一块数据；找不到返回 NULL */
const uint8_t *nw_assets_get(const char *name, size_t *size);

static inline uint16_t nw_rd16(const uint8_t *p) { return (uint16_t)(p[0] | p[1] << 8); }
static inline uint32_t nw_rd32(const uint8_t *p)
{
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}

/* ---------- 静态美术（art.bin） ---------- */
typedef struct {
    uint16_t w, h;
    int16_t ax, ay;          /* 锚点在图里的位置 */
    const uint8_t *px;       /* w×h×3：u16 RGB565 + u8 alpha */
} nw_image_t;

bool nw_art_open(void);
bool nw_art_get(int id, nw_image_t *out);
int nw_art_find(const char *name);   /* 找不到返回 -1 */
