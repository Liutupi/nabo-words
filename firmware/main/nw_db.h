#pragma once

#include <stdbool.h>

#include "words/nabo_words_db.h"

bool nw_db_open(void);
const char *nw_db_ipa(uint16_t idx);
const char *nw_db_pos(uint16_t idx);
