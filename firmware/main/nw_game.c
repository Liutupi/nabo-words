/* 画面状态机、按键、学习时钟、保存。规则在 words/（和原型逐步对拍），这里只管流程，对应 prototype/app.js */
#include "nw_game_int.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "nw_asset_ids.h"
#include "nw_cities.h"
#include "nw_assets.h"
#include "nw_db.h"
#include "nw_platform.h"
#include "nw_sound.h"
#include "words/nabo_words_battle.h"

nw_game_t *G;

static const char *const ANIM_NAMES[A_COUNT] = {NW_ANIM_NAMES};
static const uint8_t NEW_CHOICES[4] = {5, 10, 15, 20};

#define DAY_GAP_S (8u * 3600u)      /* 完成后至少 8 小时才开启下一个学习日 */
#define BREAK_S (48u * 3600u)       /* 超过 48 小时算断一天 */

static nw_state_t *S(void) { return &G->rec.s; }

static void toast(const char *msg)
{
    snprintf(G->toast, sizeof(G->toast), "%s", msg);
    G->toast_until = G->now + 2200;
}

static void mark_dirty(void)
{
    if (!G->dirty) G->dirty_since = G->now;
    G->dirty = true;
    G->plan_ok = false;
}

void nw_game_save(void)
{
    if (!G || !G->dirty) return;
    if (nw_store_save(&G->rec)) {
        G->dirty = false;
        G->answers_since_save = 0;
    }
}

bool nw_game_muted(void) { return G && G->rec.e.muted; }

static void cue(nw_cue_t c)
{
    if (!G->rec.e.muted) nw_plat_cue(c);
}

/* ---------- 学段外观与 NABO 动画 ---------- */

int nw_look_of(const nw_state_t *s) { return s->stage < 6 ? s->stage : 6; }

static void load_pack(int look)
{
    G->look = look;
    G->pack_ok = false;
    memset(G->anim_slot, -1, sizeof(G->anim_slot));
    char name[24];
    snprintf(name, sizeof(name), "nabo_%s.bin", NW_STAGE_KEYS[look]);
    size_t size = 0;
    const uint8_t *d = nw_assets_get(name, &size);
    if (!d || !nw_nts_open(&G->pack, d, size)) return;
    for (int i = 0; i < G->pack.anim_count; i++) {
        const char *an = (const char *)G->pack.data + G->pack.anims_off + 32 * i;
        for (int a = 0; a < A_COUNT; a++) {
            if (strncmp(an, ANIM_NAMES[a], 12) == 0) G->anim_slot[a] = (int8_t)i;
        }
    }
    G->pack_ok = G->anim_slot[A_IDLE] >= 0;
    nw_nts_player_init(&G->player, &G->pack, G->anim_buf, G->anim_buf_size);
}

static bool seg_of(int anim, const char *seg, uint8_t *lo, uint8_t *hi)
{
    for (size_t i = 0; i < sizeof(NW_SEGS) / sizeof(NW_SEGS[0]); i++) {
        const nw_seg_t *s = &NW_SEGS[i];
        if (!strcmp(s->stage, NW_STAGE_KEYS[G->look]) && !strcmp(s->anim, ANIM_NAMES[anim]) && !strcmp(s->seg, seg)) {
            *lo = s->lo;
            *hi = s->hi;
            return true;
        }
    }
    return false;
}

/* 缺的动作退回同学段的相近动作（大欢呼 → 开心，打瞌睡 → 待机），片段名作废，整段播放 */
static void resolve(int anim, const char *seg, int8_t *out, uint8_t *lo, uint8_t *hi)
{
    if (G->anim_slot[anim] < 0) {
        anim = anim == A_CHEER && G->anim_slot[A_HAPPY] >= 0 ? A_HAPPY : A_IDLE;
        seg = NULL;
    }
    nw_nts_anim_t a;
    nw_nts_anim(&G->pack, G->anim_slot[anim], &a);
    *out = (int8_t)anim;
    *lo = 0;
    *hi = a.frames;
    if (seg && !seg_of(anim, seg, lo, hi)) {
        *lo = 0;
        *hi = a.frames;
    }
    if (*hi > a.frames) *hi = a.frames;
    if (*lo >= *hi) *lo = 0;
}

static void play(int anim, const char *seg, bool loop, int next_anim, const char *next_seg, bool next_loop)
{
    if (!G->pack_ok) return;
    nw_play_t *p = &G->np;
    resolve(anim, seg, &p->anim, &p->lo, &p->hi);
    p->loop = loop;
    p->t0 = G->now;
    p->next_anim = -1;
    if (next_anim >= 0) {
        resolve(next_anim, next_seg, &p->next_anim, &p->next_lo, &p->next_hi);
        p->next_loop = next_loop;
    }
}

static void play_idle(void) { play(A_IDLE, NULL, true, -1, NULL, false); }

/* 当前帧；非循环动作放完时 *done = true */
static int anim_frame(bool *done)
{
    nw_nts_anim_t a;
    nw_nts_anim(&G->pack, G->anim_slot[G->np.anim], &a);
    uint32_t k = (G->now - G->np.t0) * a.fps / 1000;
    int n = G->np.hi - G->np.lo;
    if (G->np.loop) {
        *done = false;
        return G->np.lo + (int)(k % (uint32_t)n);
    }
    *done = k >= (uint32_t)n;
    return G->np.lo + (int)(k < (uint32_t)n ? k : (uint32_t)n - 1);
}

static void np_update(void)
{
    bool done;
    anim_frame(&done);
    if (!done) return;
    nw_play_t *p = &G->np;
    if (p->next_anim >= 0) {
        p->anim = p->next_anim;
        p->lo = p->next_lo;
        p->hi = p->next_hi;
        p->loop = p->next_loop;
        p->t0 = G->now;
        p->next_anim = -1;
    } else {
        play_idle();
    }
}

/* 主页画 NABO 时调用：返回动作在包里的编号和帧号 */
int nw_game_anim(int *frame)
{
    bool done;
    *frame = anim_frame(&done);
    return G->anim_slot[G->np.anim];
}

/* ---------- 菜单 ---------- */

static int extra_today(void);

static void plan_summary(void)
{
    if (G->plan_ok) return;
    nw_work_t *work = malloc(sizeof(nw_work_t));
    nw_plan_t *plan = malloc(sizeof(nw_plan_t));
    if (work && plan) {
        nw_plan_today(S(), plan, work);
        G->plan_reviews = plan->n_reviews;
        G->plan_fresh = plan->n_fresh;
        G->plan_seconds = plan->seconds;
        G->plan_ok = true;
    }
    free(work);
    free(plan);
}

const char *nw_menu_label(int id, char *sub, size_t n)
{
    const nw_state_t *s = S();
    sub[0] = 0;
    switch (id) {
    case MI_STUDY:
        if (s->done_day == s->day) {
            snprintf(sub, n, "%s", nw_extra_available(s, extra_today()) ? "今天完成了 · 学有余力可以加学" : "今天完成了，明天再来");
            return "今日学习 √";
        }
        plan_summary();
        if (G->plan_reviews + G->plan_fresh) {
            int m = (G->plan_seconds + 30) / 60;
            snprintf(sub, n, "复习 %d · 新词 %d · 约 %d 分钟", G->plan_reviews, G->plan_fresh, m < 1 ? 1 : m);
        } else {
            snprintf(sub, n, "%s", nw_exam_ready(s) ? "先通过结业考解锁新词" : "没有要学的词");
        }
        return "今日学习";
    case MI_EXAM:
        snprintf(sub, n, "%s · %d 题答对 %d 题通过", NW_STAGE_INFO[s->stage].name, NW_EXAM_Q, NW_EXAM_PASS);
        return "结业考";
    case MI_BATTLE:
        snprintf(sub, n, "%s · 积分 %d", NW_RANK_NAME[nw_rank_of(s->rating)], s->rating);
        return "对战";
    case MI_STARS:
        snprintf(sub, n, "已掌握 %d / %d", nw_mastered(s), NW_TOTAL);
        return "星图";
    case MI_PASSPORT:
        snprintf(sub, n, "已盖 %d 枚章", nw_stamp_count(s));
        return "护照";
    case MI_DESK:
        snprintf(sub, n, "摆放道具 · 摆件 %d 个", s->toy_count);
        return "书桌";
    case MI_SHOP:
        snprintf(sub, n, "金币 %u · 盲盒 %d 枚一次", (unsigned)s->coins, nw_toy_price_now(s));
        return "小卖部";
    case MI_NEWPER:
        snprintf(sub, n, "每天 %d 个 · OK 切换", s->new_per_day);
        return "每日新词";
    case MI_EXTRA:
        snprintf(sub, n, "学有余力 · 今天还能加学 %d 组", NW_EXTRA_GROUPS - extra_today());
        return "加学 5 个新词";
    case MI_CHALLENGE:
        if (G->rec.e.chal_day == s->day) snprintf(sub, n, "60 秒抢答 · 最好 %u 题", G->rec.e.chal_best);
        else snprintf(sub, n, "60 秒抢答 · 今天第一次有金币");
        return "闪电挑战";
    case MI_PLACE:
        snprintf(sub, n, "有基础？测一测从哪个学段开始");
        return "分级测试";
    case MI_HOME:
        snprintf(sub, n, "%s · OK 修改", nw_unit_city(0));
        return "家乡";
    case MI_SOUND:
        snprintf(sub, n, "%s · OK 切换", G->rec.e.muted ? "关" : "开");
        return "声音";
    }
    return "";
}

static int extra_today(void)
{
    return G->rec.e.extra_day == S()->day ? G->rec.e.extra_groups : 0;
}

static void build_menu(void)
{
    int keep = G->menu_n ? G->menu[G->menu_idx] : MI_STUDY;
    int n = 0;
    G->menu[n++] = MI_STUDY;
    if (nw_exam_ready(S())) G->menu[n++] = MI_EXAM;
    if (nw_extra_available(S(), extra_today())) G->menu[n++] = MI_EXTRA;
    if (nw_challenge_pool(S()) >= NW_CHALLENGE_MIN) G->menu[n++] = MI_CHALLENGE;
    if (S()->stage == 0 && !G->rec.e.placed_end && nw_learned(S()) < 100) G->menu[n++] = MI_PLACE;
    G->menu[n++] = MI_BATTLE;
    G->menu[n++] = MI_STARS;
    G->menu[n++] = MI_PASSPORT;
    G->menu[n++] = MI_DESK;
    G->menu[n++] = MI_SHOP;
    G->menu[n++] = MI_NEWPER;
    G->menu[n++] = MI_HOME;
    G->menu[n++] = MI_SOUND;
    G->menu_n = (uint8_t)n;
    G->menu_idx = 0;
    for (int i = 0; i < n; i++) {
        if (G->menu[i] == keep) G->menu_idx = (uint8_t)i;
    }
}

/* ---------- 主页与 NABO 的状态 ---------- */

static bool sleepy(void)
{
    const nw_state_t *s = S();
    return s->done_day != s->day && s->day - s->done_day >= 2 && s->done_day > 0;
}

static void free_session(void)
{
    free(G->sess);
    G->sess = NULL;
}

static void to_home(void)
{
    G->scene = SC_HOME;
    free_session();
    G->fb = false;
    if (nw_look_of(S()) != G->look) load_pack(nw_look_of(S()));
    build_menu();
    if (G->pending >= 0) {
        play(G->pending, NULL, false, -1, NULL, false);
        G->pending = -1;
    } else if (sleepy() && !G->awake) {
        if (G->np.anim != A_SLEEP) play(A_SLEEP, "in", false, A_SLEEP, "loop", true);
    } else if (G->np.anim == A_SLEEP) {
        play_idle();
    }
}

static bool wake_if_sleeping(void)
{
    if (G->np.anim != A_SLEEP || G->awake) return false;
    G->awake = true;
    play(A_SLEEP, "out", false, -1, NULL, false);
    cue(NW_CUE_DONE);
    return true;
}

/* ---------- 学习时钟 ---------- */

static void advance_days(int n)
{
    for (int i = 0; i < n; i++) {
        nw_day_result_t r = nw_advance_day(S());
        if (r == NW_DAY_FREEZE_USED) toast("昨天没学，用掉 1 张补签卡");
        else if (r == NW_DAY_STREAK_BROKEN) toast("连续打卡中断");
        else toast("新的一天，NABO 等你来学");
    }
    if (n > 0) {
        G->awake = false;
        mark_dirty();
        if (G->scene == SC_HOME) to_home();
    }
}

static void clock_sync(void)
{
    nw_extra_t *e = &G->rec.e;
    const nw_state_t *s = S();
    if (!e->clock_ok || s->done_day == 0) return;
    if (G->rtc < e->done_rtc) {
        e->clock_ok = 0;
        return;
    }
    uint32_t el = G->rtc - e->done_rtc;
    int days = el < DAY_GAP_S ? 0 : el <= BREAK_S ? 1 : 1 + (int)((el - BREAK_S + 86399u) / 86400u);
    int target = s->done_day + days;
    /* 学习中途不换天，回主页再换 */
    if (s->day < target && (G->scene == SC_HOME || G->scene == SC_STARS || G->scene == SC_PASSPORT)) {
        advance_days(target - s->day);
    }
}

/* ---------- 学习 ---------- */

enum { SESS_DAY, SESS_EXAM, SESS_EXTRA };

static void start_session(int mode)
{
    nw_session_t *sess = malloc(sizeof(nw_session_t));
    nw_work_t *work = malloc(sizeof(nw_work_t));
    if (!sess || !work) {
        free(sess);
        free(work);
        toast("内存不足，请重启工牌");
        return;
    }
    if (mode == SESS_EXTRA) {
        nw_rng_seed(&G->rng, nw_plat_random());
        nw_build_extra(S(), sess, &G->rng);
    } else {
        nw_rng_seed(&G->rng, nw_session_seed(S()));
        if (mode == SESS_EXAM) nw_build_exam(S(), sess, &G->rng, work);
        else nw_build_session(S(), sess, &G->rng, work);
    }
    free(work);
    if (!sess->len) {
        free(sess);
        toast("今天没有要学的词");
        return;
    }
    free_session();
    G->sess = sess;
    G->extra_mode = mode == SESS_EXTRA;
    G->scene = SC_STUDY;
    G->sel = 0;
    G->fb = false;
    G->flipped = false;
    G->mood = 0;
}

static const nw_question_t *cur_q(void) { return &G->sess->q[G->sess->pos]; }

static void add_note(const char *fmt, const char *a, int v)
{
    if (G->n_notes >= NW_NOTES) return;
    char *n = G->notes[G->n_notes++];
    if (a) snprintf(n, sizeof(G->notes[0]), fmt, a, v);
    else snprintf(n, sizeof(G->notes[0]), fmt, v);
}

static void ach_notes(uint8_t gained)
{
    for (int k = 0; k < 7; k++) {
        if (gained & (1 << k)) add_note("成就：%s", NW_ACH_NAME[k], 0);
    }
}

static void finish_session(void)
{
    nw_session_t *sess = G->sess;
    nw_state_t *s = S();
    int acc = sess->right + sess->wrong ? (sess->right * 100 + (sess->right + sess->wrong) / 2) / (sess->right + sess->wrong) : 100;
    int stage_before = s->stage;
    nw_session_result_t res;
    G->n_notes = 0;
    G->n_rows = 0;
    if (G->extra_mode) {
        nw_extra_t *e = &G->rec.e;
        if (e->extra_day != s->day) {
            e->extra_day = s->day;
            e->extra_groups = 0;
        }
        e->extra_groups++;
        snprintf(G->title, sizeof(G->title), "加学完成");
        if (sess->wrong == 0) {
            s->coins += 1;
            add_note("%s", "全对，金币 +1", 0);
        }
        int left = nw_extra_available(s, e->extra_groups) ? NW_EXTRA_GROUPS - e->extra_groups : 0;
        if (left) add_note("今天还能再加学 %d 组", NULL, left);
        else add_note("%s", "今天加学满了，去闪电挑战试试", 0);
        snprintf(G->row_l[0], 16, "新词");
        snprintf(G->row_r[0], 24, "%d", sess->plan_fresh);
        snprintf(G->row_l[1], 16, "正确率");
        snprintf(G->row_r[1], 24, "%d%%", acc);
        snprintf(G->row_l[2], 16, "已学");
        snprintf(G->row_r[2], 24, "%d 词", nw_learned(s));
        G->n_rows = 3;
        G->pending = A_HAPPY;
        cue(NW_CUE_DONE);
        mark_dirty();
        nw_game_save();
        free_session();
        G->extra_mode = false;
        G->scene = SC_SUMMARY;
        return;
    }
    nw_finish_session(s, sess, &res);
    if (res.exam) {
        snprintf(G->title, sizeof(G->title), "%s", res.passed ? "结业考通过！" : "差一点，明天再考");
        if (res.passed) {
            add_note("%s结业！金币 +10", NW_STAGE_INFO[stage_before].name, 0);
            if (s->stage < NW_STAGES) {
                char t[48];
                snprintf(t, sizeof(t), "解锁%s：%s", NW_STAGE_INFO[s->stage].name, NW_STAGE_INFO[s->stage].region);
                add_note("%s", t, 0);
            } else {
                add_note("%s", "环游完成，换上学士服", 0);
            }
            ach_notes(res.gained);
            G->pending = A_CHEER;
            cue(NW_CUE_STAMP);
        } else {
            add_note("%s", "错题已放回复习盒子", 0);
            G->pending = A_SAD;
            cue(NW_CUE_DONE);
        }
        snprintf(G->row_l[0], 16, "答对");
        snprintf(G->row_r[0], 24, "%d / %d", sess->right, sess->total);
        snprintf(G->row_l[1], 16, "及格线");
        snprintf(G->row_r[1], 24, "%d", NW_EXAM_PASS);
        G->n_rows = 2;
    } else {
        snprintf(G->title, sizeof(G->title), "今日学习完成");
        G->rec.e.done_rtc = G->rtc;
        G->rec.e.clock_ok = 1;
        if (res.stamps_gained > 0) {
            int last = -1;
            for (int u = 0; u < NW_UNITS; u++) {
                if (nw_unit_stamped(s, u)) last = u;
            }
            add_note("护照盖章：%s", last >= 0 ? nw_unit_city(last) : "", 0);
        }
        if (res.freeze_gained) add_note("连续 %d 天，得到 1 张补签卡", NULL, s->streak);
        add_note(sess->wrong == 0 ? "全对，金币 +3" : "金币 +2", NULL, 0);
        ach_notes(res.gained);
        G->pending = res.stamps_gained > 0 ? A_CHEER : A_HAPPY;
        cue(res.stamps_gained > 0 ? NW_CUE_STAMP : NW_CUE_DONE);
        const char *L[4] = {"新词", "复习", "正确率", "已掌握"};
        for (int k = 0; k < 4; k++) snprintf(G->row_l[k], 16, "%s", L[k]);
        snprintf(G->row_r[0], 24, "%d", sess->plan_fresh);
        snprintf(G->row_r[1], 24, "%d", sess->plan_reviews);
        snprintf(G->row_r[2], 24, "%d%%", acc);
        snprintf(G->row_r[3], 24, "%d 词", nw_mastered(s));
        G->n_rows = 4;
    }
    mark_dirty();
    nw_game_save();
    free_session();
    G->scene = SC_SUMMARY;
}

static void next_q(void)
{
    G->sess->pos++;
    if (G->sess->pos >= G->sess->len) {
        finish_session();
        return;
    }
    G->sel = 0;
    G->fb = false;
    G->flipped = false;
}

static void answer(bool ok, int pick)
{
    G->fb = true;
    G->fb_ok = ok;
    G->fb_pick = (uint8_t)pick;
    G->fb_t = G->now;
    nw_session_answer(S(), G->sess, ok, &G->rng);
    G->mood = ok ? 1 : 2;
    G->mood_t = G->now;
    cue(ok ? NW_CUE_RIGHT : NW_CUE_WRONG);
    mark_dirty();
    G->answers_since_save++;
}

static void study_key(nw_key_t k)
{
    const nw_question_t *q = cur_q();
    if (q->type == NW_Q_INTRO) {
        if (k == NW_KEY_OK) {
            nw_session_intro_seen(S(), G->sess);
            mark_dirty();
            next_q();
        }
        return;
    }
    if (G->fb) {
        if (k == NW_KEY_OK) next_q();
        return;
    }
    if (q->type == NW_Q_FLASH) {
        if (!G->flipped) {
            if (k == NW_KEY_OK) G->flipped = true;
            return;
        }
        if (k == NW_KEY_UP_PRESS) answer(true, 0);
        else if (k == NW_KEY_DOWN) answer(false, 1);
        return;
    }
    int n = q->n_opt;
    if (k == NW_KEY_UP_PRESS) G->sel = (uint8_t)((G->sel + n - 1) % n);
    else if (k == NW_KEY_DOWN) G->sel = (uint8_t)((G->sel + 1) % n);
    else answer(G->sel == q->answer, G->sel);
}

/* ---------- 书桌 / 小卖部 ---------- */

int nw_desk_count(void) { return NW_PROPS + 1; }

static void desk_key(nw_key_t k)
{
    int n = nw_desk_count();
    if (k == NW_KEY_UP_PRESS) { G->list_sel = (G->list_sel + n - 1) % n; return; }
    if (k == NW_KEY_DOWN) { G->list_sel = (G->list_sel + 1) % n; return; }
    if (G->list_sel == NW_PROPS) { to_home(); return; }
    const nw_prop_t *p = &NW_PROP[G->list_sel];
    int m = nw_mastered(S());
    char t[64];
    if (m < p->need) {
        snprintf(t, sizeof(t), "再掌握 %d 个词解锁", p->need - m);
        toast(t);
        return;
    }
    G->rec.e.hidden ^= (uint16_t)(1u << G->list_sel);
    mark_dirty();
}

int nw_shop_count(void) { return 2 + NW_ACCS + 1; }

static int toys_unlocked(void) { return (nw_toy_stage(S()) + 1) * NW_TOYS_PER_STAGE; }

void nw_shop_item(int i, char *label, size_t ln, char *right, size_t rn, int *icon)
{
    const nw_state_t *s = S();
    *icon = -1;
    right[0] = 0;
    if (i == 0) {
        snprintf(label, ln, "开盲盒");
        bool left = s->toy_count < toys_unlocked();
        if (left) snprintf(right, rn, "%d 金币 · %d/%d", nw_toy_price_now(s), s->toy_count, toys_unlocked());
        else snprintf(right, rn, "已集齐 %d/%d", s->toy_count, toys_unlocked());
        /* 图标：下一批里第一个还没有的摆件 */
        uint8_t own[NW_TOYS] = {0};
        for (int k = 0; k < s->toy_count; k++) own[s->toys[k]] = 1;
        for (int t = 0; t < toys_unlocked(); t++) {
            if (!own[t]) { *icon = G->art_toy[t]; break; }
        }
        if (*icon < 0) *icon = G->art_toy[0];
    } else if (i == 1) {
        snprintf(label, ln, "补签卡");
        if (s->freezes >= NW_FREEZE_MAX) snprintf(right, rn, "已有 %d 张（满）", s->freezes);
        else snprintf(right, rn, "%d 金币 · 已有 %d", NW_FREEZE_PRICE, s->freezes);
        *icon = G->art_ui[2];
    } else if (i < 2 + NW_ACCS) {
        int a = i - 2;
        snprintf(label, ln, "饰品 · %s", NW_ACC_NAME[a]);
        if (!(s->accs & (1 << a))) snprintf(right, rn, "%d 金币", NW_ACC_PRICE);
        else snprintf(right, rn, "%s", s->acc == a ? "戴着" : "已有");
    } else {
        snprintf(label, ln, "返回");
    }
}

static void shop_key(nw_key_t k)
{
    int n = nw_shop_count();
    if (k == NW_KEY_UP_PRESS) { G->list_sel = (G->list_sel + n - 1) % n; return; }
    if (k == NW_KEY_DOWN) { G->list_sel = (G->list_sel + 1) % n; return; }
    nw_state_t *s = S();
    char t[64];
    int i = G->list_sel, r;
    if (i == n - 1) { to_home(); return; }
    nw_rng_t rng;
    nw_rng_seed(&rng, nw_plat_random());
    if (i == 0) {
        r = nw_open_toy_box(s, &rng);
        if (r >= 0) snprintf(t, sizeof(t), "开到了：%s", NW_TOY_NAME[r]);
        else if (r == NW_SHOP_NO_COINS) snprintf(t, sizeof(t), "金币不够，需要 %d 枚", nw_toy_price_now(s));
        else snprintf(t, sizeof(t), "%s", r == NW_SHOP_SET_DONE ? "这批摆件已集齐，升学段解锁新一批" : "摆件全部集齐");
    } else if (i == 1) {
        r = nw_buy_freeze(s);
        if (r >= 0) snprintf(t, sizeof(t), "买到 1 张补签卡");
        else if (r == NW_SHOP_FREEZE_FULL) snprintf(t, sizeof(t), "补签卡最多存 %d 张", NW_FREEZE_MAX);
        else snprintf(t, sizeof(t), "金币不够，需要 %d 枚", NW_FREEZE_PRICE);
    } else {
        int a = i - 2;
        r = nw_use_accessory(s, a);
        if (r == NW_SHOP_NO_COINS) snprintf(t, sizeof(t), "金币不够，需要 %d 枚", NW_ACC_PRICE);
        else snprintf(t, sizeof(t), "%s%s", r == 1 ? "买下并戴上：" : r == 2 ? "戴上：" : "摘下：", NW_ACC_NAME[a]);
    }
    toast(t);
    if (r >= 0) {
        cue(NW_CUE_RIGHT);
        mark_dirty();
    } else {
        cue(NW_CUE_WRONG);
    }
}

/* ---------- 按键分发 ---------- */

int nw_cur_unit(const nw_state_t *s)
{
    int w = s->next < NW_TOTAL ? s->next : NW_TOTAL - 1;
    return w / NW_UNIT;
}

/* ---------- 开局：选家乡、分级测试 ---------- */

static void apply_home(void)
{
    const nw_extra_t *e = &G->rec.e;
    if (e->onboard && e->home_prov < NW_N_PROVINCES && e->home_city < NW_PROVINCES[e->home_prov].n) {
        nw_set_home(NW_PROVINCES[e->home_prov].cities[e->home_city]);
    } else {
        nw_set_home(NULL);
    }
}

static void onboard_start(bool first)
{
    G->scene = SC_ONBOARD;
    G->ob_first = first;
    G->ob_step = 0;
    G->prov_sel = G->rec.e.onboard ? G->rec.e.home_prov : 0;
    G->city_sel = 0;
    G->ob_sel = 0;
}

static void onboard_finish(void)
{
    G->rec.e.onboard = 1;
    mark_dirty();
    nw_game_save();
    to_home();
}

static void place_start(void)
{
    free(G->place);
    G->place = malloc(sizeof(nw_place_t));
    if (!G->place) {
        toast("内存不足，请重启工牌");
        return;
    }
    nw_rng_seed(&G->rng, nw_plat_random());
    nw_place_start(G->place, &G->rng);
    G->place_done = false;
    G->sel = 0;
    G->place_sel = 0;
    G->scene = SC_PLACE;
}

static void place_end(bool apply)
{
    nw_place_t *p = G->place;
    if (apply && p && p->lo > 0) {
        nw_place_apply(S(), p->lo);
        G->rec.e.placed_end = nw_stage_start[p->lo];
        G->pending = A_CHEER;
        if (nw_look_of(S()) != G->look) load_pack(nw_look_of(S()));
        char t[64];
        snprintf(t, sizeof(t), "从%s开始，前面 %d 个词记为已掌握", NW_STAGE_INFO[p->lo].name, nw_stage_start[p->lo]);
        toast(t);
    }
    free(G->place);
    G->place = NULL;
    G->rec.e.onboard = 1;
    mark_dirty();
    nw_game_save();
    to_home();
}

static void onboard_key(nw_key_t k)
{
    int n = G->ob_step == 0 ? NW_N_PROVINCES : G->ob_step == 1 ? NW_PROVINCES[G->prov_sel].n : 2;
    int *sel = G->ob_step == 0 ? &G->prov_sel : G->ob_step == 1 ? &G->city_sel : &G->ob_sel;
    if (k == NW_KEY_UP_PRESS) { *sel = (*sel + n - 1) % n; return; }
    if (k == NW_KEY_DOWN) { *sel = (*sel + 1) % n; return; }
    if (G->ob_step == 0) {
        G->ob_step = 1;
        G->city_sel = G->rec.e.onboard && G->rec.e.home_prov == G->prov_sel ? G->rec.e.home_city : 0;
        return;
    }
    if (G->ob_step == 1) {
        G->rec.e.home_prov = (uint8_t)G->prov_sel;
        G->rec.e.home_city = (uint8_t)G->city_sel;
        bool was = G->rec.e.onboard;
        G->rec.e.onboard = 1;   /* 先记上家乡，apply_home 要用 */
        apply_home();
        G->rec.e.onboard = was;
        mark_dirty();
        /* 已经在学的人只是改家乡；新开局（或者还在小学开头）问一下要不要分级测试 */
        if (was && (S()->stage > 0 || G->rec.e.placed_end || nw_learned(S()) >= 100)) {
            char t[48];
            snprintf(t, sizeof(t), "家乡改为%s", NW_PROVINCES[G->prov_sel].cities[G->city_sel]);
            toast(t);
            onboard_finish();
            return;
        }
        G->ob_step = 2;
        G->ob_sel = 0;
        return;
    }
    if (G->ob_sel == 0) place_start();
    else onboard_finish();
}

static void place_key(nw_key_t k)
{
    nw_place_t *p = G->place;
    if (G->place_done) {
        if (k == NW_KEY_UP_PRESS || k == NW_KEY_DOWN) G->place_sel ^= 1;
        else place_end(G->place_sel == 0);
        return;
    }
    int n = p->q[p->pos].n_opt + 1;   /* 最后一项是“不认识” */
    if (k == NW_KEY_UP_PRESS) { G->sel = (uint8_t)((G->sel + n - 1) % n); return; }
    if (k == NW_KEY_DOWN) { G->sel = (uint8_t)((G->sel + 1) % n); return; }
    bool ok = G->sel == p->q[p->pos].answer;
    G->sel = 0;
    if (nw_place_answer(p, ok, &G->rng)) {
        G->place_done = true;
        G->place_sel = 0;
        cue(NW_CUE_DONE);
    }
}

/* ---------- 闪电挑战 ---------- */

static void challenge_next(void)
{
    nw_challenge_question(S(), &G->cq, &G->rng);
    G->sel = 0;
    G->fb = false;
}

static void challenge_start(void)
{
    nw_rng_seed(&G->rng, nw_plat_random());
    G->c_score = 0;
    G->c_over = false;
    G->c_end = G->now + NW_CHALLENGE_MS;
    G->scene = SC_CHALLENGE;
    challenge_next();
}

static void challenge_finish(void)
{
    nw_extra_t *e = &G->rec.e;
    bool first = e->chal_day != S()->day;
    G->c_first = first;
    G->c_coins = nw_challenge_coins(G->c_score, first);
    S()->coins += (uint32_t)G->c_coins;
    if (first) {
        e->chal_day = S()->day;
        e->chal_best = 0;
    }
    G->c_best = G->c_score > e->chal_best;
    if (G->c_best) e->chal_best = G->c_score;
    G->c_over = true;
    G->fb = false;
    cue(NW_CUE_DONE);
    mark_dirty();
}

static void challenge_key(nw_key_t k)
{
    if (G->c_over) {
        if (k == NW_KEY_OK) to_home();
        return;
    }
    if (G->fb) return;
    int n = G->cq.n_opt;
    if (k == NW_KEY_UP_PRESS) { G->sel = (uint8_t)((G->sel + n - 1) % n); return; }
    if (k == NW_KEY_DOWN) { G->sel = (uint8_t)((G->sel + 1) % n); return; }
    G->fb = true;
    G->fb_ok = G->sel == G->cq.answer;
    G->fb_pick = G->sel;
    G->fb_t = G->now;
    if (G->fb_ok) G->c_score++;
    else nw_challenge_miss(S(), G->cq.idx);
    cue(G->fb_ok ? NW_CUE_RIGHT : NW_CUE_WRONG);
}

static void challenge_update(void)
{
    if (G->c_over) return;
    if ((int32_t)(G->now - G->c_end) >= 0) {
        challenge_finish();
        return;
    }
    if (G->fb && G->now - G->fb_t > (G->fb_ok ? 250u : 700u)) challenge_next();
}

static void home_ok(void)
{
    nw_state_t *s = S();
    switch (G->menu[G->menu_idx]) {
    case MI_STUDY:
        if (s->done_day == s->day) toast("今天完成了，NABO 去休息了");
        else start_session(SESS_DAY);
        break;
    case MI_EXAM: start_session(SESS_EXAM); break;
    case MI_EXTRA: start_session(SESS_EXTRA); break;
    case MI_CHALLENGE: challenge_start(); break;
    case MI_PLACE: place_start(); break;
    case MI_HOME: onboard_start(false); break;
    case MI_BATTLE: G->scene = SC_BATTLE; break;
    case MI_STARS: G->scene = SC_STARS; break;
    case MI_PASSPORT: G->scene = SC_PASSPORT; G->page_unit = nw_cur_unit(s); break;
    case MI_DESK: G->scene = SC_DESK; G->list_sel = 0; break;
    case MI_SHOP: G->scene = SC_SHOP; G->list_sel = 0; break;
    case MI_NEWPER: {
        int k = 0;
        for (int i = 0; i < 4; i++) {
            if (NEW_CHOICES[i] == s->new_per_day) k = i;
        }
        s->new_per_day = NEW_CHOICES[(k + 1) % 4];
        mark_dirty();
        break;
    }
    case MI_SOUND:
        G->rec.e.muted = !G->rec.e.muted;
        mark_dirty();
        break;
    }
    if (G->scene != SC_HOME) nw_gfx_invalidate(&G->gfx);
}

static void act(nw_key_t k)
{
    cue(NW_CUE_TICK);
    nw_scene_t before = G->scene;
    switch (G->scene) {
    case SC_HOME:
        if (wake_if_sleeping()) break;
        if (k == NW_KEY_UP_PRESS) G->menu_idx = (uint8_t)((G->menu_idx + G->menu_n - 1) % G->menu_n);
        else if (k == NW_KEY_DOWN) G->menu_idx = (uint8_t)((G->menu_idx + 1) % G->menu_n);
        else home_ok();
        break;
    case SC_STUDY: study_key(k); break;
    case SC_SUMMARY:
    case SC_STARS:
    case SC_BATTLE:
        if (k == NW_KEY_OK) to_home();
        break;
    case SC_PASSPORT:
        if (k == NW_KEY_OK) to_home();
        else G->page_unit = (G->page_unit + (k == NW_KEY_UP_PRESS ? NW_UNITS - 1 : 1)) % NW_UNITS;
        break;
    case SC_DESK: desk_key(k); break;
    case SC_SHOP: shop_key(k); break;
    case SC_ONBOARD: onboard_key(k); break;
    case SC_PLACE: place_key(k); break;
    case SC_CHALLENGE: challenge_key(k); break;
    }
    if (G->scene != before) nw_gfx_invalidate(&G->gfx);
}

static void back(void)
{
    if (G->scene == SC_HOME) return;
    cue(NW_CUE_TICK);
    if (G->scene == SC_ONBOARD) {   /* 开局里一步步往回退；第一次开机不能跳过选家乡 */
        if (G->ob_step > 0) G->ob_step--;
        else if (!G->ob_first) to_home();
        nw_gfx_invalidate(&G->gfx);
        return;
    }
    if (G->scene == SC_PLACE) {
        free(G->place);
        G->place = NULL;
        if (!G->rec.e.onboard) {
            G->scene = SC_ONBOARD;
            G->ob_step = 2;
            nw_gfx_invalidate(&G->gfx);
            return;
        }
    }
    if (G->scene == SC_CHALLENGE && !G->c_over) challenge_finish();
    if (G->scene == SC_STUDY) nw_game_save();
    G->extra_mode = false;
    to_home();
    nw_gfx_invalidate(&G->gfx);
}

void nw_game_key(nw_key_t k, uint32_t now_ms)
{
    if (!G) return;
    G->now = now_ms;
    switch (k) {
    case NW_KEY_UP_PRESS:
        G->up_held = true;
        G->up_long = false;
        G->up_t = now_ms;
        break;
    case NW_KEY_UP_RELEASE:
        if (G->up_held && !G->up_long) act(NW_KEY_UP_PRESS);
        G->up_held = false;
        break;
    default: act(k); break;
    }
}

/* ---------- 每帧 ---------- */

static void draw_scene(void)
{
    switch (G->scene) {
    case SC_HOME: nw_draw_home(); break;
    case SC_STUDY: nw_draw_study(); break;
    case SC_SUMMARY: nw_draw_summary(); break;
    case SC_STARS: nw_draw_stars(); break;
    case SC_PASSPORT: nw_draw_passport(); break;
    case SC_DESK: nw_draw_desk(); break;
    case SC_SHOP: nw_draw_shop(); break;
    case SC_BATTLE: nw_draw_battle(); break;
    case SC_ONBOARD: nw_draw_onboard(); break;
    case SC_PLACE: nw_draw_place(); break;
    case SC_CHALLENGE: nw_draw_challenge(); break;
    }
    nw_draw_overlay();
}

const char *nw_game_scene_name(void)
{
    static const char *const N[] = {"home", "study", "summary", "stars", "passport", "desk", "shop", "battle", "onboard", "place", "challenge"};
    return G ? N[G->scene] : "";
}

bool nw_game_overflow(void) { return G && G->gfx.overflow; }

void nw_game_frame(uint32_t now_ms, uint32_t rtc_s)
{
    G->now = now_ms;
    G->rtc = rtc_s;
    static uint32_t last_clock;
    if (now_ms - last_clock > 10000 || last_clock == 0) {
        last_clock = now_ms ? now_ms : 1;
        clock_sync();
    }
    if (G->up_held && !G->up_long && now_ms - G->up_t >= 1000) {
        G->up_long = true;
        back();
    }
    if (G->toast[0] && (int32_t)(now_ms - G->toast_until) > 0) G->toast[0] = 0;
    if (G->scene == SC_HOME && G->pack_ok) np_update();
    if (G->scene == SC_STUDY && G->fb && G->fb_ok && now_ms - G->fb_t > 700) next_q();
    if (G->scene == SC_CHALLENGE) challenge_update();
    if (G->mood && now_ms - G->mood_t > 1000) G->mood = 0;

    /* 保存：学习中每 25 题存一次，其它画面改动 3 秒后存 */
    if (G->dirty) {
        if (G->scene == SC_STUDY ? G->answers_since_save >= 25 && !G->fb : now_ms - G->dirty_since > 3000) {
            nw_game_save();
        }
    }

    nw_gfx_begin(&G->gfx);
    draw_scene();
    nw_gfx_flush(&G->gfx);
}

/* ---------- 启动 ---------- */

static void cache_art(void)
{
    static const char *const UI[4] = {"ui_coin", "ui_flame", "ui_freeze", "ui_flame_dim"};
    char n[24];
    for (int i = 0; i < 4; i++) G->art_ui[i] = nw_art_find(UI[i]);
    for (int i = 0; i < NW_PROPS; i++) G->art_prop[i] = nw_art_find(NW_PROP[i].id);
    for (int i = 0; i < NW_TOYS; i++) {
        snprintf(n, sizeof(n), "toy_%s", NW_TOY_SLUG[i]);
        G->art_toy[i] = nw_art_find(n);
    }
    static const char *const FORM[4] = {"egg", "baby", "teen", "final"};
    for (int s = 0; s < NW_STAGES; s++) {
        for (int f = 0; f < 4; f++) {
            snprintf(n, sizeof(n), "%s_%s", NW_SPRITE_KEY[s], FORM[f]);
            G->art_sprite[s][f] = nw_art_find(n);
        }
    }
    for (int a = 0; a < NW_ACCS; a++) {
        for (int f = 0; f < 4; f++) {
            snprintf(n, sizeof(n), "acc_%s_%s", NW_ACC_ID[a], FORM[f]);
            G->art_acc[a][f] = nw_art_find(n);
        }
    }
}

/* 所有学段的动画包里最大的差分帧缓冲 */
static size_t max_anim_buffer(void)
{
    size_t need = 0;
    for (int look = 0; look < NW_LOOKS; look++) {
        char name[24];
        snprintf(name, sizeof(name), "nabo_%s.bin", NW_STAGE_KEYS[look]);
        size_t size = 0;
        const uint8_t *d = nw_assets_get(name, &size);
        nw_nts_pack_t p;
        if (d && nw_nts_open(&p, d, size)) {
            size_t k = nw_nts_buffer_need(&p);
            if (k > need) need = k;
        }
    }
    return need;
}

bool nw_game_open(uint32_t now_ms, uint32_t rtc_s, bool rtc_ok)
{
    if (!nw_fonts_open() || !nw_db_open() || !nw_art_open()) return false;
    G = calloc(1, sizeof(nw_game_t));
    if (!G) return false;
    G->now = now_ms;
    G->rtc = rtc_s;
    G->pending = -1;
    G->anim_buf_size = max_anim_buffer();
    G->anim_buf = malloc(G->anim_buf_size ? G->anim_buf_size : 1);
    if (!G->anim_buf) return false;
    cache_art();
    nw_store_load(&G->rec);
    /* 断过电：时间未知，不判断断签；今天学完了就直接开启新的一天（DESIGN.md 第 2 节） */
    if (!rtc_ok) {
        G->rec.e.clock_ok = 0;
        if (S()->done_day == S()->day) advance_days(1);
        G->toast[0] = 0;
    }
    nw_gfx_init(&G->gfx, &G->player);
    load_pack(nw_look_of(S()));
    play_idle();
    apply_home();
    to_home();
    if (!G->rec.e.onboard) onboard_start(true);   /* 第一次开机：先选家乡，再问要不要分级测试 */
    return true;
}

/* ---------- 串口调试命令 ---------- */

/* 按原型 panel.js 的“自动模拟 N 天”：每天做完今日学习（按正确率答题），然后进入下一天 */
static void simulate(int days, int pct)
{
    nw_work_t *work = malloc(sizeof(nw_work_t));
    nw_session_t *sess = malloc(sizeof(nw_session_t));
    nw_rng_t r;
    nw_rng_seed(&r, nw_plat_random());
    for (int d = 0; work && sess && d < days; d++) {
        nw_state_t *s = S();
        nw_rng_seed(&G->rng, nw_session_seed(s));
        bool exam = nw_exam_ready(s) && (d % 2 == 1);
        if (exam) nw_build_exam(s, sess, &G->rng, work);
        else nw_build_session(s, sess, &G->rng, work);
        for (sess->pos = 0; sess->pos < sess->len; sess->pos++) {
            if (sess->q[sess->pos].type == NW_Q_INTRO) nw_session_intro_seen(s, sess);
            else nw_session_answer(s, sess, (int)nw_rng_below(&r, 100) < pct, &G->rng);
        }
        nw_session_result_t res;
        if (sess->len) nw_finish_session(s, sess, &res);
        nw_advance_day(s);
    }
    free(work);
    free(sess);
    G->rec.e.clock_ok = 0;
    mark_dirty();
    to_home();
    nw_gfx_invalidate(&G->gfx);
}

void nw_game_command(const char *line, char *out, size_t n, uint32_t now_ms, uint32_t rtc_s)
{
    out[0] = 0;
    if (!G) return;
    G->now = now_ms;
    G->rtc = rtc_s;
    nw_state_t *s = S();
    char word[16] = {0};
    int a = 0, b = 0;
    int got = sscanf(line, "%15s %d %d", word, &a, &b);
    if (got < 1) return;
    if (!strcmp(word, "status")) {
        snprintf(out, n, "scene=%s day=%u done=%u stage=%u next=%u learned=%d mastered=%d coins=%u streak=%u "
                         "freezes=%u stamps=%d clock_ok=%u dirty=%d saves=%u ops=%d px=%u overflow=%d",
                 nw_game_scene_name(), s->day, s->done_day, s->stage, s->next, nw_learned(s), nw_mastered(s),
                 (unsigned)s->coins, s->streak, s->freezes, nw_stamp_count(s), G->rec.e.clock_ok, G->dirty,
                 (unsigned)G->rec.e.saves, G->gfx.n, (unsigned)G->gfx.pixels, G->gfx.overflow);
    } else if (!strcmp(word, "key")) {
        char k[8] = {0};
        sscanf(line, "%*s %7s", k);
        if (!strcmp(k, "up")) { nw_game_key(NW_KEY_UP_PRESS, now_ms); nw_game_key(NW_KEY_UP_RELEASE, now_ms); }
        else if (!strcmp(k, "down")) nw_game_key(NW_KEY_DOWN, now_ms);
        else if (!strcmp(k, "ok")) nw_game_key(NW_KEY_OK, now_ms);
        else if (!strcmp(k, "back")) back();
        snprintf(out, n, "scene=%s", nw_game_scene_name());
    } else if (!strcmp(word, "days") && got >= 2) {
        advance_days(a);
        snprintf(out, n, "day=%u streak=%u", s->day, s->streak);
    } else if (!strcmp(word, "sim") && got >= 2) {
        simulate(a, got >= 3 ? b : 82);
        snprintf(out, n, "day=%u stage=%u learned=%d mastered=%d coins=%u", s->day, s->stage, nw_learned(s),
                 nw_mastered(s), (unsigned)s->coins);
    } else if (!strcmp(word, "solve")) {
        /* 把当前的学习 / 分级测试全部答对做完（got>=2 时只答 a 题） */
        int left = got >= 2 ? a : 10000;
        while (left-- > 0) {
            if (G->scene == SC_STUDY && G->sess) {
                const nw_question_t *q = cur_q();
                if (G->fb) next_q();
                else if (q->type == NW_Q_INTRO) study_key(NW_KEY_OK);
                else if (q->type == NW_Q_FLASH) { G->flipped = true; answer(true, 0); }
                else { G->sel = q->answer; study_key(NW_KEY_OK); }
            } else if (G->scene == SC_PLACE && G->place && !G->place_done) {
                G->sel = G->place->q[G->place->pos].answer;
                place_key(NW_KEY_OK);
            } else {
                break;
            }
        }
        snprintf(out, n, "scene=%s", nw_game_scene_name());
    } else if (!strcmp(word, "coins") && got >= 2) {
        s->coins = (uint32_t)a;
        mark_dirty();
        snprintf(out, n, "coins=%u", (unsigned)s->coins);
    } else if (!strcmp(word, "wipe")) {
        uint32_t seq = G->rec.seq;
        nw_store_defaults(&G->rec);
        G->rec.seq = seq;
        mark_dirty();
        nw_game_save();
        apply_home();
        if (G->look != 0) load_pack(0);
        to_home();
        onboard_start(true);
        nw_gfx_invalidate(&G->gfx);
        snprintf(out, n, "wiped");
    } else if (!strcmp(word, "save")) {
        G->dirty = true;
        nw_game_save();
        snprintf(out, n, "saved=%u", (unsigned)G->rec.e.saves);
    } else {
        snprintf(out, n, "commands: status | key up/down/ok/back | solve [N] | days N | sim N [pct] | coins N | wipe | save");
    }
}

/* 宿主测试用：同一时刻整屏重画一遍（核对增量重画） */
void nw_test_full_redraw(uint32_t now_ms, uint32_t rtc_s)
{
    nw_gfx_invalidate(&G->gfx);
    G->now = now_ms;
    G->rtc = rtc_s;
    nw_gfx_begin(&G->gfx);
    draw_scene();
    nw_gfx_flush(&G->gfx);
}
