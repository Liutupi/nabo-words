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
    uint8_t onboard;        /* 1：选过家乡、做完开局 */
    uint8_t home_prov, home_city;
    uint8_t extra_groups;   /* extra_day 那天加学了几组 */
    uint16_t placed_end;    /* 分级测试跳过的词 [0, placed_end)，星图上用浅色 */
    uint16_t extra_day;
    uint16_t chal_day;      /* 最近一次闪电挑战是第几个学习日（每天第一次给金币） */
    uint16_t chal_best;
    /* 周报：每 7 个学习日一份（v0.4.0 起，旧存档里这几项是 0，从下一天开始统计） */
    uint16_t week_day;      /* 这一周从第几个学习日开始，0 表示还没开始统计 */
    uint16_t week_learned, week_mastered;   /* 周初的已学 / 掌握 */
    uint8_t week_done;      /* 这周完成今日学习几天 */
    uint8_t week_pad;
    uint16_t week_wrong[4]; /* 这周最近答错的 4 个词（序号 + 1，0 表示空） */
} nw_extra_t;
_Static_assert(sizeof(nw_extra_t) == 40, "记录大小要和 v0.1.0 一致");

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
