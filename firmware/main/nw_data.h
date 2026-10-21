/* 学段、城市、道具、摆件、饰品、段位、配色等固定表，和 prototype/words.js、draw.js 一致 */
#pragma once

#include <stdint.h>

#include "words/nabo_words_model.h"

#define NW_LOOKS 7      /* 6 个学段 + 全部学完（学士服） */
#define NW_PROPS 12

typedef struct {
    const char *name, *region;
    const char *const *cities;
    uint8_t n_cities;
} nw_stage_info_t;

typedef struct {
    uint32_t wall, trim, desk, top;
} nw_look_t;

typedef struct {
    const char *id, *name;
    uint16_t need;          /* 掌握多少词解锁 */
    int16_t x, y;           /* 锚点位置 */
    uint8_t wall;           /* 1 画在 NABO 后面（墙上） */
} nw_prop_t;

extern const nw_stage_info_t NW_STAGE_INFO[NW_STAGES];
extern const char *const NW_STAGE_KEYS[NW_LOOKS];
extern const nw_look_t NW_LOOK[NW_LOOKS];
extern const nw_prop_t NW_PROP[NW_PROPS];
extern const char *const NW_TOY_NAME[NW_TOYS];
extern const char *const NW_TOY_SLUG[NW_TOYS];
extern const char *const NW_ACC_ID[NW_ACCS];
extern const char *const NW_ACC_NAME[NW_ACCS];
extern const char *const NW_SPRITE_KEY[NW_STAGES];
extern const char *const NW_SPRITE_NAME[NW_STAGES];
extern const char *const NW_RANK_NAME[8];
extern const char *const NW_ACH_NAME[7];

/* 第 u 个单元的城市名 */
const char *nw_unit_city(int unit);
void nw_set_home(const char *city);   /* NULL：没选家乡，用默认城市表 */
int nw_unit_stage(int unit);
