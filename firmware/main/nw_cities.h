#pragma once

typedef struct {
    const char *name;
    const char *const *cities;
    int n;
} nw_province_t;

extern const nw_province_t NW_PROVINCES[];
extern const int NW_N_PROVINCES;
