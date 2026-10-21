/* 提示音（和原型 panel.js 的 SFX 对应），16 kHz 单声道，整数合成 */
#pragma once

#include <stdint.h>

typedef enum { NW_CUE_TICK = 0, NW_CUE_RIGHT, NW_CUE_WRONG, NW_CUE_DONE, NW_CUE_STAMP, NW_CUE_COUNT } nw_cue_t;

void nw_sound_cue(nw_cue_t cue);                 /* 任何任务都可以调；只是记一笔 */
void nw_sound_fill(int16_t *pcm, int n);         /* 音频任务调用，混进 pcm */
