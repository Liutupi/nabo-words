/* 学习记录的保存和读取：记忆盒、金币、设置、学习时钟。两个槽轮流写，带序号和 CRC，断电不会写坏 */
#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "words/nabo_words_model.h"

typedef struct {
    uint16_t hidden;        /* 收起的书桌道具（位图，按 NW_PROP 顺序） */
    uint8_t muted;
    uint8_t clock_ok;       /* done_rtc 可信（没有断过电） */
    uint32_t done_rtc;      /* 完成今日学习时的 RTC 秒数 */
    uint32_t saves;         /* 写过多少次（诊断用） */
    uint32_t reserved[7];
} nw_extra_t;

typedef struct {
    uint32_t magic, version, seq, len, crc;
    nw_state_t s;
    nw_extra_t e;
} nw_record_t;

void nw_store_defaults(nw_record_t *r);
/* 读最新的有效记录；都没有就填默认值并返回 false */
bool nw_store_load(nw_record_t *r);
bool nw_store_save(nw_record_t *r);
uint32_t nw_crc32(const void *data, uint32_t n);
