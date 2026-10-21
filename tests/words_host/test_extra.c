/* 分级测试、加学、闪电挑战的规则用例（tests/words_host/test_extra.py 编译运行） */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "nabo_words_extra.h"

static int fails;
#define CHECK(c, ...) do { if (!(c)) { printf("FAIL %s:%d ", __FILE__, __LINE__); printf(__VA_ARGS__); printf("\n"); fails++; } } while (0)

/* 按“能答对到第 k 学段”作答，返回测出的起点和题数 */
static int run_place(int knows, int *questions)
{
    nw_place_t p;
    nw_rng_t r;
    nw_rng_seed(&r, 7 + knows);
    nw_place_start(&p, &r);
    int n = 0;
    for (;;) {
        n++;
        if (nw_place_answer(&p, p.probe < knows, &r)) break;
        CHECK(n < 40, "too many questions");
        if (n >= 40) break;
    }
    *questions = n;
    return p.lo;
}

int main(void)
{
    static nw_state_t s;
    /* 分级测试：会到第 k 学段（k 个学段都会），起点就是 k；最多 30 题 */
    for (int k = 0; k <= NW_STAGES; k++) {
        int q, got = run_place(k, &q);
        int want = k > NW_STAGES - 1 ? NW_STAGES - 1 : k;
        CHECK(got == want, "knows %d -> start %d, want %d", k, got, want);
        CHECK(q <= 30 && q % NW_PLACE_Q == 0, "knows %d: %d questions", k, q);
    }
    /* 刚好 8 题过、7 题不过 */
    {
        nw_place_t p; nw_rng_t r; nw_rng_seed(&r, 1); nw_place_start(&p, &r);
        int first = p.probe;
        for (int i = 0; i < NW_PLACE_Q; i++) nw_place_answer(&p, i < 8, &r);
        CHECK(p.lo == first + 1, "8/10 should pass");
        nw_place_start(&p, &r);
        for (int i = 0; i < NW_PLACE_Q; i++) nw_place_answer(&p, i < 7, &r);
        CHECK(p.hi == first, "7/10 should fail");
    }
    /* 同一轮的 10 题不重复，而且都在被测学段里 */
    {
        nw_place_t p; nw_rng_t r; nw_rng_seed(&r, 3); nw_place_start(&p, &r);
        for (int i = 0; i < NW_PLACE_Q; i++) {
            CHECK(p.q[i].idx >= nw_stage_start[p.probe] && p.q[i].idx < nw_stage_end[p.probe], "word out of stage");
            for (int j = 0; j < i; j++) CHECK(p.q[i].idx != p.q[j].idx, "duplicate word");
        }
    }
    /* 套用结果：之前学段全部毕业，进度从该学段开头开始 */
    nw_state_init(&s);
    nw_introduce(&s, 0);
    nw_place_apply(&s, 2);
    CHECK(s.stage == 2 && s.next == nw_stage_start[2], "apply stage/next");
    CHECK(s.box[0] == NW_GRADUATED && s.box[1599] == NW_GRADUATED && s.box[1600] == 0, "apply boxes");
    CHECK(s.due[5] == NW_NEVER, "apply due");
    CHECK(nw_mastered(&s) == 1600, "mastered after placement");
    nw_place_apply(&s, 0);   /* 从零开始不改任何东西 */
    CHECK(s.stage == 2, "apply 0 is a no-op");

    /* 加学：没完成今天不能加；完成后 5 个新词，先卡片后检测；一天最多 4 组 */
    nw_state_init(&s);
    CHECK(!nw_extra_available(&s, 0), "extra before done");
    s.done_day = s.day;
    CHECK(nw_extra_available(&s, 0) && !nw_extra_available(&s, NW_EXTRA_GROUPS), "extra groups cap");
    static nw_session_t sess;
    nw_rng_t r; nw_rng_seed(&r, 9);
    s.next = 40;
    int n = nw_build_extra(&s, &sess, &r);
    CHECK(n == 10, "extra queue %d", n);
    for (int i = 0; i < 5; i++) CHECK(sess.q[i].type == NW_Q_INTRO && sess.q[i].idx == 40 + i, "intro order");
    for (int i = 5; i < 10; i++) CHECK(sess.q[i].type == NW_Q_EN2ZH && (sess.q[i].flags & NW_QF_NEW), "check flags");
    /* 学段末尾只剩 2 个词；发完以后（结业考之前）不能再加 */
    s.next = nw_stage_end[0] - 2;
    CHECK(nw_build_extra(&s, &sess, &r) == 4, "tail of stage");
    s.next = nw_stage_end[0];
    CHECK(!nw_extra_available(&s, 0), "no extra past stage end");

    /* 闪电挑战：只出学过的词；答错排进明天，不改盒子；金币只给每天第一次 */
    nw_state_init(&s);
    for (int i = 100; i < 130; i++) { s.box[i] = 3; s.due[i] = 20; }
    CHECK(nw_challenge_pool(&s) == 30, "pool");
    for (int k = 0; k < 200; k++) {
        nw_question_t q;
        nw_challenge_question(&s, &q, &r);
        CHECK(q.idx >= 100 && q.idx < 130 && q.type == NW_Q_EN2ZH, "challenge word %d", q.idx);
    }
    s.day = 5;
    nw_challenge_miss(&s, 100);
    CHECK(s.due[100] == 6 && s.box[100] == 3, "miss -> tomorrow");
    s.due[101] = 3;
    nw_challenge_miss(&s, 101);
    CHECK(s.due[101] == 3, "already due stays");
    CHECK(nw_challenge_coins(17, true) == 3 && nw_challenge_coins(80, true) == 6 && nw_challenge_coins(17, false) == 0, "coins");

    printf(fails ? "%d 项失败\n" : "分级测试 / 加学 / 闪电挑战 规则用例全部通过\n", fails);
    return fails != 0;
}
