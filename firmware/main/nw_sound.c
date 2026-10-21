#include "nw_sound.h"

#include <stdatomic.h>

#define RATE 16000
#define VOICES 4

typedef struct {
    uint16_t hz, ms, delay_ms;
    uint8_t wave;      /* 0 方波 1 三角 2 锯齿 */
    uint8_t vol;       /* 0..100 */
} note_t;

typedef struct {
    uint32_t phase, step;
    int32_t left, total, wait;
    uint8_t wave;
    int32_t amp;
} voice_t;

static const note_t TICK[] = {{1200, 30, 0, 0, 25}};
static const note_t RIGHT[] = {{880, 80, 0, 0, 55}, {1320, 120, 80, 0, 55}};
static const note_t WRONG[] = {{220, 200, 0, 2, 45}};
static const note_t DONE[] = {{523, 140, 0, 1, 80}, {659, 140, 100, 1, 80}, {784, 140, 200, 1, 80}, {1046, 160, 300, 1, 80}};
static const note_t STAMP[] = {{392, 120, 0, 1, 80}, {523, 120, 110, 1, 80}, {659, 120, 220, 1, 80}, {784, 260, 330, 1, 90}};

static const struct { const note_t *n; int count; } CUES[NW_CUE_COUNT] = {
    {TICK, 1}, {RIGHT, 2}, {WRONG, 1}, {DONE, 4}, {STAMP, 4},
};

static atomic_uint s_pending;   /* 位图：要播的提示音 */
static voice_t s_v[VOICES];

void nw_sound_cue(nw_cue_t cue)
{
    if (cue < NW_CUE_COUNT) atomic_fetch_or(&s_pending, 1u << cue);
}

static void start(const note_t *n)
{
    int k = 0;
    for (int i = 1; i < VOICES; i++) {   /* 用空闲的，没有就挤掉剩得最少的 */
        if (s_v[i].left + s_v[i].wait < s_v[k].left + s_v[k].wait) k = i;
    }
    voice_t *v = &s_v[k];
    v->phase = 0;
    v->step = (uint32_t)(((uint64_t)n->hz << 32) / RATE);
    v->total = v->left = n->ms * RATE / 1000;
    v->wait = n->delay_ms * RATE / 1000;
    v->wave = n->wave;
    v->amp = n->vol * 70;   /* 满音量也给扬声器留余量 */
}

void nw_sound_fill(int16_t *pcm, int n)
{
    unsigned p = atomic_exchange(&s_pending, 0);
    for (int c = 0; c < NW_CUE_COUNT; c++) {
        if (p & (1u << c)) {
            for (int i = 0; i < CUES[c].count; i++) start(&CUES[c].n[i]);
        }
    }
    for (int i = 0; i < n; i++) {
        int32_t acc = pcm[i];
        for (int k = 0; k < VOICES; k++) {
            voice_t *v = &s_v[k];
            if (v->wait > 0) {
                v->wait--;
                continue;
            }
            if (v->left <= 0) continue;
            int32_t ph = (int32_t)(v->phase >> 16);   /* 0..65535 */
            int32_t s;
            if (v->wave == 0) s = ph < 32768 ? 32767 : -32767;
            else if (v->wave == 1) s = ph < 32768 ? ph * 2 - 32767 : 32767 - (ph - 32768) * 2;
            else s = ph - 32768;
            /* 指数近似：剩余比例的平方 */
            int32_t env = (int32_t)((int64_t)v->left * 256 / v->total);
            env = env * env >> 8;
            acc += (int32_t)((int64_t)s * v->amp / 32767 * env >> 8) * 1;
            v->phase += v->step;
            v->left--;
        }
        pcm[i] = (int16_t)(acc > 32767 ? 32767 : acc < -32768 ? -32768 : acc);
    }
}
