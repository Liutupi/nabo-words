/* 游戏层内部共享的状态（nw_game.c 管逻辑，nw_scene_*.c 只读它来画画面） */
#pragma once

#include "nw_data.h"
#include "nw_font.h"
#include "nw_game.h"
#include "nw_gfx.h"
#include "nw_sprite.h"
#include "nw_store.h"
#include "nw_vs.h"
#include "words/nabo_words_extra.h"
#include "words/nabo_words_quiz.h"

typedef enum {
    SC_HOME, SC_STUDY, SC_SUMMARY, SC_STARS, SC_PASSPORT, SC_DESK, SC_SHOP, SC_BATTLE,
    SC_ONBOARD, SC_PLACE, SC_CHALLENGE, SC_ACH, SC_WEEK,
} nw_scene_t;

typedef enum {
    MI_STUDY, MI_EXAM, MI_BATTLE, MI_STARS, MI_PASSPORT, MI_DESK, MI_SHOP, MI_NEWPER, MI_SOUND,
    MI_EXTRA, MI_CHALLENGE, MI_PLACE, MI_HOME, MI_ACH,
} nw_menu_id_t;

typedef enum { A_IDLE, A_HAPPY, A_SAD, A_CHEER, A_SLEEP, A_COUNT } nw_anim_id_t;

typedef struct {
    int8_t anim;            /* nw_anim_id_t（已按当前学段的包换算过缺的动作） */
    uint8_t lo, hi;         /* 帧区间 [lo, hi) */
    bool loop;
    uint32_t t0;
    int8_t next_anim;       /* 非循环动作放完以后：-1 回待机 */
    uint8_t next_lo, next_hi;
    bool next_loop;
} nw_play_t;

#define NW_NOTES 6
#define NW_ROWS 4

typedef struct {
    nw_record_t rec;
    bool dirty;             /* 有没保存的改动 */
    uint32_t dirty_since;
    uint16_t answers_since_save;

    nw_scene_t scene;
    uint32_t now;           /* 毫秒 */
    uint32_t rtc;

    /* 主页菜单 */
    uint8_t menu[14], menu_n, menu_idx;
    int8_t menu_dir;          /* 最近一次翻菜单的方向；只用于 160 ms 的轻微过渡 */
    uint32_t menu_t;
    bool plan_ok;
    uint16_t plan_reviews, plan_fresh, plan_seconds;

    /* NABO */
    int look;                       /* 0..6 */
    nw_nts_pack_t pack;
    bool pack_ok;
    int8_t anim_slot[A_COUNT];      /* 动作在包里的编号，-1 没有 */
    nw_nts_player_t player;
    uint8_t *anim_buf;
    size_t anim_buf_size;
    nw_play_t np;
    bool awake;
    int pending;                    /* 回主页时要播的动作，-1 没有 */

    /* 学习 */
    nw_session_t *sess;
    nw_rng_t rng;
    uint8_t sel;
    bool fb, fb_ok, flipped;
    uint8_t fb_pick;
    uint32_t fb_t;
    int8_t mood;                    /* 0 平常 1 答对 2 答错（答题页的精灵） */
    uint32_t mood_t;

    /* 小结 */
    char title[48];
    char row_l[NW_ROWS][16], row_r[NW_ROWS][24];
    int n_rows;
    char notes[NW_NOTES][48];
    int n_notes;

    bool extra_mode;                /* 这次学习是加学 */

    /* 开局：选家乡、要不要分级测试 */
    uint8_t ob_step;                /* 0 选省 1 选市 2 有没有基础 */
    bool ob_first;                  /* 第一次开机（不能长按返回主页） */
    int prov_sel, city_sel, ob_sel;

    /* 分级测试 */
    nw_place_t *place;
    bool place_done;
    uint8_t place_sel;

    /* 闪电挑战 */
    nw_question_t cq;
    bool c_over;
    uint16_t c_score;
    uint32_t c_end;
    int c_coins;
    bool c_best, c_first;

    nw_vs_t *vs;                    /* 对战（只在对战画面分配） */

    /* 周报（弹出时算好） */
    struct {
        int week, learned, mastered, done, streak;
        uint16_t wrong[4];
        int n_wrong;
    } wk;

    /* 其它画面 */
    int page_unit;                  /* 护照：选中的单元 */
    int list_sel;                   /* 书桌 / 小卖部 */
    char toast[64];
    uint32_t toast_until;

    /* 上键长按 */
    bool up_held, up_long;
    uint32_t up_t;

    nw_gfx_t gfx;
    int art_ui[4];                  /* coin flame freeze flame_dim */
    int art_prop[NW_PROPS];
    int art_toy[NW_TOYS];
    int art_sprite[NW_STAGES][4];
    int art_acc[NW_ACCS][4];
} nw_game_t;

extern nw_game_t *G;

/* 画面（nw_scene_home.c / nw_scene_study.c / nw_scene_misc.c） */
void nw_draw_home(void);
void nw_draw_study(void);
void nw_draw_summary(void);
void nw_draw_stars(void);
void nw_draw_passport(void);
void nw_draw_desk(void);
void nw_draw_shop(void);
void nw_draw_overlay(void);
void nw_draw_onboard(void);
void nw_draw_place(void);
void nw_draw_challenge(void);
void nw_draw_ach(void);
void nw_draw_week(void);

/* 画面共用的小工具 */
int nw_game_anim(int *frame);         /* 主页 NABO：动作在包里的编号和帧号 */
const char *nw_menu_label(int id, char *sub, size_t n);
void nw_draw_sprite(int stage, int form, int x, int y, int acc, int bob);
int nw_isin(int phase1024);          /* sin，输出 -256..256 */
int nw_look_of(const nw_state_t *s);
int nw_cur_unit(const nw_state_t *s);
int nw_desk_count(void);
int nw_shop_count(void);
void nw_shop_item(int i, char *label, size_t ln, char *right, size_t rn, int *icon);

#define C_INK 0x121829
#define C_PAPER 0xFFF8EC
#define C_GOLD 0xE8B84A
#define C_TEAL 0x3FB8A9
#define C_RED 0xE0564F
#define C_GREEN 0x4DBB6A
#define C_MUTED 0x8C93A8
#define C_WHITE 0xFFFFFF
