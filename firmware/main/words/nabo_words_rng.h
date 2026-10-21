/*
 * 可复现随机数 mulberry32，和原型 prototype/core.js 逐位一致。
 * 原型里 Math.floor(rnd() * n) 等于这里的 nw_rng_below(n)：rnd() = x / 2^32 是精确的，
 * x * n < 2^45 也能被 double 精确表示，所以两边取整结果相同。
 */
#pragma once

#include <stdint.h>

typedef struct {
    uint32_t a;
} nw_rng_t;

static inline void nw_rng_seed(nw_rng_t *r, uint32_t seed)
{
    r->a = seed;
}

static inline uint32_t nw_rng_next(nw_rng_t *r)
{
    r->a += 0x6D2B79F5u;
    uint32_t t = (r->a ^ (r->a >> 15)) * (1u | r->a);
    t = (t + (t ^ (t >> 7)) * (61u | t)) ^ t;
    return t ^ (t >> 14);
}

/* 0 .. n-1，等于原型的 Math.floor(rnd() * n) */
static inline uint32_t nw_rng_below(nw_rng_t *r, uint32_t n)
{
    return (uint32_t)(((uint64_t)nw_rng_next(r) * n) >> 32);
}

/* 原型的 shuffle()：从后往前 Fisher–Yates，每一步都消耗一个随机数 */
static inline void nw_shuffle_u16(uint16_t *a, int n, nw_rng_t *r)
{
    for (int i = n - 1; i > 0; i--) {
        int j = (int)nw_rng_below(r, (uint32_t)(i + 1));
        uint16_t t = a[i];
        a[i] = a[j];
        a[j] = t;
    }
}

static inline void nw_shuffle_u8(uint8_t *a, int n, nw_rng_t *r)
{
    for (int i = n - 1; i > 0; i--) {
        int j = (int)nw_rng_below(r, (uint32_t)(i + 1));
        uint8_t t = a[i];
        a[i] = a[j];
        a[j] = t;
    }
}
