/*
 * 宿主上跑固件的游戏层：读 assets.bin，按脚本按键、推进时间、截屏。
 * 每次截屏都会再整屏重画一遍，核对增量重画和整屏重画逐像素一致。
 * 脚本（stdin）每行一条：key up|down|ok|back、wait <ms>、shot <文件>、cmd <调试命令>、rtc <秒>、hold <ms>
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "nw_assets.h"
#include "nw_game.h"
#include "nw_platform.h"
#include "nw_sound.h"

static uint16_t fb[240 * 320];
static uint16_t strip[NW_STRIP_PX];
static unsigned char slots[2][64 * 1024];
static size_t slot_len[2];
static uint32_t rng = 12345;

uint16_t *nw_plat_strip(void) { return strip; }
void nw_plat_send(int x, int y, int w, int h, uint16_t *px)
{
    for (int r = 0; r < h; r++) memcpy(&fb[(y + r) * 240 + x], px + r * w, (size_t)w * 2);
}
void nw_plat_send_wait(void) {}
bool nw_plat_record_write(int slot, const void *d, size_t n)
{
    if (n > sizeof(slots[0])) return false;
    memcpy(slots[slot], d, n);
    slot_len[slot] = n;
    return true;
}
bool nw_plat_record_read(int slot, void *d, size_t n)
{
    if (slot_len[slot] != n) return false;
    memcpy(d, slots[slot], n);
    return true;
}
void nw_plat_cue(int c) { nw_sound_cue((nw_cue_t)c); }
uint32_t nw_plat_random(void) { rng = rng * 1103515245u + 12345u; return rng; }

int main(int argc, char **argv)
{
    if (argc < 2) return 2;
    FILE *f = fopen(argv[1], "rb");
    if (!f) return 2;
    fseek(f, 0, SEEK_END);
    long n = ftell(f);
    fseek(f, 0, SEEK_SET);
    unsigned char *a = malloc((size_t)n);
    if (fread(a, 1, (size_t)n, f) != (size_t)n) return 2;
    fclose(f);
    if (!nw_assets_open(a, (size_t)n)) { fprintf(stderr, "assets bad\n"); return 3; }
    uint32_t now = 1000, rtc = 100000;
    if (!nw_game_open(now, rtc, true)) { fprintf(stderr, "open failed\n"); return 4; }
    char line[256], out[512];
    int bad = 0;
    while (fgets(line, sizeof(line), stdin)) {
        line[strcspn(line, "\r\n")] = 0;
        if (!strncmp(line, "key ", 4)) {
            const char *k = line + 4;
            if (!strcmp(k, "up")) { nw_game_key(NW_KEY_UP_PRESS, now); nw_game_key(NW_KEY_UP_RELEASE, now + 50); }
            else if (!strcmp(k, "down")) nw_game_key(NW_KEY_DOWN, now);
            else if (!strcmp(k, "ok")) nw_game_key(NW_KEY_OK, now);
            else if (!strcmp(k, "back")) {
                nw_game_key(NW_KEY_UP_PRESS, now);
                for (int t = 0; t < 1100; t += 33) { now += 33; nw_game_frame(now, rtc); }
                nw_game_key(NW_KEY_UP_RELEASE, now);
            }
            now += 33;
            nw_game_frame(now, rtc);
        } else if (!strncmp(line, "wait ", 5)) {
            int ms = atoi(line + 5);
            for (int t = 0; t < ms; t += 33) { now += 33; nw_game_frame(now, rtc); }
        } else if (!strncmp(line, "rtc ", 4)) {
            rtc = (uint32_t)atol(line + 4);
        } else if (!strncmp(line, "cmd ", 4)) {
            nw_game_command(line + 4, out, sizeof(out), now, rtc);
            printf("%s\n", out);
            now += 33;
            nw_game_frame(now, rtc);
        } else if (!strncmp(line, "shot ", 5)) {
            static uint16_t inc[240 * 320];
            memcpy(inc, fb, sizeof(fb));
            /* 同一时刻整屏重画，核对和增量结果一致 */
            nw_game_command("status", out, sizeof(out), now, rtc);
            extern void nw_test_full_redraw(uint32_t now, uint32_t rtc);
            nw_test_full_redraw(now, rtc);
            if (memcmp(inc, fb, sizeof(fb))) {
                int diff = 0;
                for (int i = 0; i < 240 * 320; i++) diff += inc[i] != fb[i];
                printf("MISMATCH %s: %d px differ\n", line + 5, diff);
                bad++;
            }
            if (nw_game_overflow()) { printf("OVERFLOW %s\n", line + 5); bad++; }
            FILE *o = fopen(line + 5, "wb");
            fwrite(fb, 2, 240 * 320, o);
            fclose(o);
            printf("shot %s scene=%s\n", line + 5, nw_game_scene_name());
        }
    }
    return bad ? 1 : 0;
}
