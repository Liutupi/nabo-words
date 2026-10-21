/* Real game/renderer regression probe. No ESP-IDF or connected badge is needed. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "nw_assets.h"
#include "nw_db.h"
#include "nw_game_int.h"
#include "nw_platform.h"
#include "nw_sound.h"

void nw_test_full_redraw(uint32_t now_ms, uint32_t rtc_s);

static uint16_t fb[240 * 320], saved[240 * 320], strip[NW_STRIP_PX];
static unsigned char slots[2][0x8000];
static size_t lengths[2];
static unsigned checks, failures, max_ops, max_glyphs;
static uint32_t now = 1000;
static bool baseline;

uint16_t *nw_plat_strip(void) { return strip; }
void nw_plat_send(int x, int y, int w, int h, uint16_t *px)
{
    for (int r = 0; r < h; r++) memcpy(fb + (y + r) * 240 + x, px + r * w, (size_t)w * 2);
}
void nw_plat_send_wait(void) {}
bool nw_plat_record_write(int slot, const void *data, size_t n)
{
    if (slot < 0 || slot > 1 || n > sizeof(slots[0])) return false;
    memcpy(slots[slot], data, n);
    lengths[slot] = n;
    return true;
}
bool nw_plat_record_read(int slot, void *data, size_t n)
{
    if (slot < 0 || slot > 1 || lengths[slot] != n) return false;
    memcpy(data, slots[slot], n);
    return true;
}
void nw_plat_cue(int cue) { nw_sound_cue((nw_cue_t)cue); }
uint32_t nw_plat_random(void) { return 12345; }
bool nw_plat_radio_on(void) { return false; }
void nw_plat_radio_off(void) {}
bool nw_plat_radio_send(const uint8_t *mac, const void *data, int n)
{ (void)mac; (void)data; (void)n; return false; }
int nw_plat_radio_recv(uint8_t mac[6], void *data, int n)
{ (void)mac; (void)data; (void)n; return 0; }
void nw_plat_mac(uint8_t mac[6]) { memset(mac, 0, 6); }

static void check(bool ok, const char *what)
{
    checks++;
    if (!ok) {
        failures++;
        if (failures <= 12) printf("FAIL %s\n", what);
    }
}

static void frame(uint32_t t)
{
    now = t;
    nw_game_frame(t, 100000);
    check(!nw_game_overflow(), "draw-list/glyph overflow");
    if ((unsigned)G->gfx.n > max_ops) max_ops = (unsigned)G->gfx.n;
    if ((unsigned)G->gfx.glyphs.used > max_glyphs) max_glyphs = (unsigned)G->gfx.glyphs.used;
    memcpy(saved, fb, sizeof(fb));
    nw_test_full_redraw(t, 100000);
    check(memcmp(fb, saved, sizeof(fb)) == 0, "incremental != full redraw");
}

static void shot(const char *name)
{
    FILE *f = fopen(name, "wb");
    if (!f || fwrite(fb, sizeof(fb), 1, f) != 1) exit(3);
    fclose(f);
}

static void home(int stage, int learned)
{
    nw_store_defaults(&G->rec);
    nw_state_t *s = &G->rec.s;
    s->stage = (uint8_t)stage;
    s->next = (uint16_t)learned;
    s->day = 42;
    s->done_day = 41;
    s->streak = 31;
    s->coins = 350;
    s->freezes = 2;
    memset(s->box, NW_MASTERED, (size_t)learned);
    G->rec.e.onboard = 1;
    G->rec.e.clock_ok = 0;
    G->pending = -1;
    G->plan_ok = false;
    G->menu_n = 0;
    G->scene = SC_STARS;
    char out[64];
    nw_game_command("key back", out, sizeof(out), now += 1000, 100000);
    frame(now);
}

static bool overlap(const nw_op_t *a, const nw_op_t *b)
{
    return a->x0 < b->x1 && b->x0 < a->x1 && a->y0 < b->y1 && b->y0 < a->y1;
}

static void home_layout(void)
{
    for (int i = 0; i < G->gfx.n; i++) {
        const nw_op_t *a = &G->gfx.ops[i];
        if (a->kind != NW_OP_TEXT) continue;
        if (a->y0 >= 256) check(a->x0 >= 27 && a->x1 <= 205, "menu text touches controls");
        if (a->y1 <= 24 || (a->y0 >= 214 && a->y1 <= 254)) {
            for (int j = 0; j < i; j++) {
                const nw_op_t *b = &G->gfx.ops[j];
                if (b->kind == NW_OP_TEXT) check(!overlap(a, b), "home statistics overlap");
            }
        }
    }
}

static void text_limits(void)
{
    static const nw_font_id_t fonts[] = {NW_F16, NW_F12};
    static const char *const texts[] = {
        "今天完成了 · 学有余力可以加学", "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz",
        "第 18 站 圣彼得堡", "学过 7500 · 熟悉 7500 · 毕业 7500",
    };
    for (unsigned i = 0; i < sizeof(texts) / sizeof(texts[0]); i++) {
        for (int width = 12; width <= 210; width += 11) {
            nw_gfx_begin(&G->gfx);
            nw_text_fit(&G->gfx, fonts, 2, texts[i], 120, 30, width, NW_CENTER, 0xffff);
            check(!G->gfx.overflow, "bounded text overflows glyph pool");
            for (int j = 0; j < G->gfx.n; j++)
                check(G->gfx.ops[j].u.text.t.w <= width, "fitted text exceeds allotted width");
        }
    }
    nw_gfx_invalidate(&G->gfx);
}

static void vocabulary_layout(void)
{
    static const nw_font_id_t word[] = {NW_W36, NW_W28, NW_W20, NW_W16};
    static const nw_font_id_t meaning[] = {NW_F22, NW_F16, NW_F12};
    for (int i = 0; i < NW_TOTAL; i++) {
        nw_gfx_begin(&G->gfx);
        nw_text_fit(&G->gfx, word, 4, nw_db_word((uint16_t)i), 120, 80, 210, NW_CENTER, 0xffff);
        nw_text_fit(&G->gfx, meaning, 3, nw_db_meaning((uint16_t)i), 120, 180, 210, NW_CENTER, 0xffff);
        check(!G->gfx.overflow, "vocabulary exhausts glyph pool");
        for (int j = 0; j < G->gfx.n; j++)
            check(G->gfx.ops[j].u.text.t.w <= 210, "vocabulary exceeds card width");
    }
    nw_gfx_invalidate(&G->gfx);
}

static void other_scenes(void)
{
    char out[128];
    home(0, 0);
    G->scene = SC_ONBOARD;
    for (int step = 0; step < 3; step++) {
        G->ob_step = (uint8_t)step;
        frame(now += 33);
    }
    G->scene = SC_HOME;
    nw_game_key(NW_KEY_OK, now += 33);
    frame(now); shot("study_intro.rgb");
    nw_game_command("solve 2", out, sizeof(out), now += 33, 100000);
    frame(now); shot("study_quiz.rgb");
    nw_game_command("solve", out, sizeof(out), now += 33, 100000);
    frame(now); shot("study_summary.rgb");
    home(3, 4000);
    const nw_scene_t scenes[] = {SC_STARS, SC_PASSPORT, SC_DESK, SC_SHOP};
    const char *const names[] = {"stars.rgb", "passport.rgb", "desk.rgb", "shop.rgb"};
    for (unsigned i = 0; i < sizeof(scenes) / sizeof(scenes[0]); i++) {
        G->scene = scenes[i];
        G->page_unit = 81;
        G->list_sel = 0;
        frame(now += 33); shot(names[i]);
    }
    home(0, 100);
    for (int i = 0; i < G->menu_n; i++) {
        if (G->menu[i] == MI_CHALLENGE) {
            G->menu_idx = (uint8_t)i;
            nw_game_key(NW_KEY_OK, now += 33);
            frame(now); shot("challenge.rgb");
            break;
        }
    }
}

static void animation_seek(void)
{
    size_t n = nw_nts_buffer_need(&G->pack);
    uint8_t *a = malloc(n ? n : 1), *b = malloc(n ? n : 1);
    if (!a || !b) exit(3);
    for (int anim = 0; anim < G->pack.anim_count; anim++) {
        nw_nts_anim_t info;
        check(nw_nts_anim(&G->pack, anim, &info), "animation metadata");
        nw_nts_player_t sequential, seek;
        nw_nts_player_init(&sequential, &G->pack, a, n);
        nw_nts_player_init(&seek, &G->pack, b, n);
        for (int f = 0; f < info.frames; f++) {
            check(nw_nts_player_set(&sequential, anim, f), "advance animation");
            check(nw_nts_player_set(&seek, anim, 0) && nw_nts_player_set(&seek, anim, f), "seek animation");
            check(memcmp(a, b, (size_t)info.w * info.h) == 0, "skipped/restarted frames decode differently");
        }
    }
    free(a); free(b);
}

int main(int argc, char **argv)
{
    if (argc < 2) return 2;
    baseline = argc > 2 && !strcmp(argv[2], "--baseline");
    FILE *f = fopen(argv[1], "rb");
    if (!f) return 2;
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    rewind(f);
    void *data = malloc((size_t)size);
    if (!data || fread(data, (size_t)size, 1, f) != 1) return 2;
    fclose(f);
    if (!nw_assets_open(data, (size_t)size) || !nw_game_open(now, 100000, true)) return 2;
    text_limits();
    vocabulary_layout();
    for (int look = 0; look < 7; look++) {
        home(look, look < 6 ? nw_stage_start[look] + 100 : NW_TOTAL);
        home_layout();
        char name[32];
        snprintf(name, sizeof(name), "home_%d.rgb", look);
        shot(name);
        animation_seek();
        nw_nts_anim_t idle;
        check(nw_nts_anim(&G->pack, G->anim_slot[A_IDLE], &idle), "idle exists");
        check(idle.fps == 12 && idle.frames == 49, "full-cadence original idle frames");
        for (int k = 0; k < 3; k++) {
            G->np.anim = A_IDLE;
            G->np.lo = 0;
            G->np.hi = idle.frames;
            G->np.loop = true;
            G->np.t0 = now;
            uint32_t elapsed = k == 0 ? 1000000000u : k == 1 ? 2000000000u : 3000000000u;
            G->now = now + elapsed;
            int got;
            nw_game_anim(&got);
            int expected = (int)(((uint64_t)elapsed * idle.fps / 1000) % idle.frames);
            check(got == expected, "idle timing overflows after long uptime");
            G->now = now;
        }
    }
    /* Changing clothes while a different-length expression was active. */
    home(0, 100);
    G->np.anim = A_CHEER; G->np.lo = 2; G->np.hi = 42; G->np.loop = true;
    G->np.t0 = now;
    home(1, 800);
    nw_nts_anim_t idle;
    nw_nts_anim(&G->pack, G->anim_slot[A_IDLE], &idle);
    check(G->np.anim == A_IDLE && G->np.lo == 0 && G->np.hi == idle.frames,
          "new outfit inherits previous expression range");
    /* Worst-case counters and the completed-day hint. */
    home(6, NW_TOTAL);
    G->rec.s.coins = 12345678; G->rec.s.streak = 999; G->rec.s.done_day = G->rec.s.day;
    frame(now += 33); home_layout(); shot("home_counters.rgb");
    home(0, 100);
    G->rec.s.done_day = G->rec.s.day;
    frame(now += 33); home_layout(); shot("home_done.rgb");
    for (int i = 0; i < G->menu_n; i++) {
        nw_game_key(NW_KEY_DOWN, now += 500);
        uint32_t start = now;
        const int times[] = {0, 33, 80, 160, 300};
        for (unsigned t = 0; t < sizeof(times) / sizeof(times[0]); t++) {
            frame(start + (uint32_t)times[t]); home_layout();
        }
    }
    other_scenes();
    home(0, 100);
    uint32_t start = now;
    unsigned char before[NW_TOTAL];
    memcpy(before, G->rec.s.box, sizeof(before));
    for (int i = 0; i < 49; i++) {
        frame(start + (uint32_t)(i * 1000 + 11) / 12);
        char name[32];
        snprintf(name, sizeof(name), "motion_%02d.rgb", i);
        shot(name);
    }
    check(!memcmp(before, G->rec.s.box, sizeof(before)), "animation changes learning state");
    printf("UI checks=%u failures=%u max_ops=%u/%d max_glyphs=%u/%d\n",
           checks, failures, max_ops, NW_MAX_OPS, max_glyphs, NW_GLYPH_POOL);
    return !baseline && failures ? 1 : 0;
}
