#include "nw_store.h"

#include <stddef.h>
#include <string.h>

#include "nw_platform.h"

#define MAGIC 0x3153574Eu   /* 'NWS1' */
#define VERSION 1

uint32_t nw_crc32(const void *data, uint32_t n)
{
    const uint8_t *p = (const uint8_t *)data;
    uint32_t c = 0xFFFFFFFFu;
    while (n--) {
        c ^= *p++;
        for (int k = 0; k < 8; k++) c = (c >> 1) ^ (0xEDB88320u & (0u - (c & 1)));
    }
    return ~c;
}

#define BODY_OFF offsetof(nw_record_t, s)
#define BODY_LEN (sizeof(nw_record_t) - BODY_OFF)

void nw_store_defaults(nw_record_t *r)
{
    memset(r, 0, sizeof(*r));
    nw_state_init(&r->s);
}

static bool valid(const nw_record_t *r)
{
    return r->magic == MAGIC && r->version == VERSION && r->len == BODY_LEN &&
           r->crc == nw_crc32((const uint8_t *)r + BODY_OFF, BODY_LEN);
}

static int s_slot = -1;   /* 上次读到 / 写入的槽 */

bool nw_store_load(nw_record_t *r)
{
    static nw_record_t tmp;
    bool have = false;
    uint32_t best = 0;
    for (int slot = 0; slot < 2; slot++) {
        if (!nw_plat_record_read(slot, &tmp, sizeof(tmp)) || !valid(&tmp)) continue;
        if (!have || (int32_t)(tmp.seq - best) > 0) {
            *r = tmp;
            best = tmp.seq;
            s_slot = slot;
            have = true;
        }
    }
    if (!have) nw_store_defaults(r);
    return have;
}

bool nw_store_save(nw_record_t *r)
{
    r->magic = MAGIC;
    r->version = VERSION;
    r->len = BODY_LEN;
    r->seq++;
    r->e.saves++;
    r->crc = nw_crc32((const uint8_t *)r + BODY_OFF, BODY_LEN);
    int slot = s_slot == 0 ? 1 : 0;
    if (!nw_plat_record_write(slot, r, sizeof(*r))) return false;
    s_slot = slot;
    return true;
}
