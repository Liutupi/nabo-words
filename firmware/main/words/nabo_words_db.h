/*
 * 词库接口。逻辑模块只通过这几个函数读词库，不关心数据放在哪里：
 * 固件里从 assets 分区读，宿主测试里由 tests/words_host/gen_db.py 生成的 C 数组提供。
 * 数据来自 data/words.json（tools/build_wordlist.py 生成）。
 */
#pragma once

#include <stdbool.h>
#include <stdint.h>

#define NW_TOTAL 7500   /* 词数，和 prototype/words.js 的 TOTAL_WORDS 一致 */

/* 第 idx 个词的英文（和词表里的大小写一致，如 "I"、"Monday"） */
const char *nw_db_word(uint16_t idx);

/* 第 idx 个词的中文释义（UTF-8） */
const char *nw_db_meaning(uint16_t idx);

/* 某个全小写的拼写是不是词库里的词（补字母题用来排除能拼成别的词的候选） */
bool nw_db_has_lower(const char *lower);
