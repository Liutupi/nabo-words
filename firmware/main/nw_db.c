/* 词库（words.bin，在 assets 分区里原地读）：实现 words/nabo_words_db.h，另加音标和词性 */
#include "nw_db.h"

#include <string.h>

#include "nw_assets.h"

static const uint8_t *s_db;
static uint16_t s_n;

bool nw_db_open(void)
{
    size_t size = 0;
    s_db = nw_assets_get("words.bin", &size);
    if (!s_db || size < 8 || memcmp(s_db, "NWD1", 4) != 0 || nw_rd16(s_db + 4) != NW_TOTAL) {
        s_db = NULL;
        return false;
    }
    s_n = nw_rd16(s_db + 4);
    return true;
}

static const char *field(uint16_t idx, int k)
{
    if (!s_db || idx >= s_n) {
        return "";
    }
    const char *p = (const char *)s_db + nw_rd32(s_db + 8 + 4 * idx);
    while (k-- > 0) {
        p += strlen(p) + 1;
    }
    return p;
}

const char *nw_db_word(uint16_t idx) { return field(idx, 0); }
const char *nw_db_ipa(uint16_t idx) { return field(idx, 1); }
const char *nw_db_pos(uint16_t idx) { return field(idx, 2); }
const char *nw_db_meaning(uint16_t idx) { return field(idx, 3); }

static int cmp_lower(const char *word, const char *lower)
{
    for (;; word++, lower++) {
        int a = (unsigned char)*word, b = (unsigned char)*lower;
        if (a >= 'A' && a <= 'Z') {
            a += 32;
        }
        if (a != b || !a) {
            return a - b;
        }
    }
}

bool nw_db_has_lower(const char *lower)
{
    if (!s_db) {
        return false;
    }
    const uint8_t *order = s_db + 8 + 4u * s_n;
    int lo = 0, hi = s_n - 1;
    while (lo <= hi) {
        int mid = (lo + hi) / 2;
        int c = cmp_lower(nw_db_word(nw_rd16(order + 2 * mid)), lower);
        if (c == 0) {
            return true;
        }
        if (c < 0) {
            lo = mid + 1;
        } else {
            hi = mid - 1;
        }
    }
    return false;
}
