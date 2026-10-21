/* 和 extra_trace.js 同样的轨迹，由 C 规则模块生成（extra_parity.py 逐行比较） */
#include <stdio.h>
#include <string.h>

#include "nabo_words_extra.h"

static void opts(const nw_question_t *q, char *buf, size_t n)
{
    buf[0] = 0;
    for (int k = 0; k < q->n_opt; k++) {
        if (k) strncat(buf, "|", n - strlen(buf) - 1);
        strncat(buf, nw_db_meaning(q->opt[k]), n - strlen(buf) - 1);
    }
}

int main(void)
{
    static nw_state_t s;
    char o[256];
    for (int seed = 1; seed <= 12; seed++) {
        nw_rng_t r;
        nw_place_t p;
        nw_rng_seed(&r, (uint32_t)seed);
        nw_place_start(&p, &r);
        int knows = seed % 7;
        for (;;) {
            const nw_question_t *q = &p.q[p.pos];
            opts(q, o, sizeof(o));
            printf("P %d %d %d %d %d %s\n", seed, p.probe, p.pos, q->idx, q->answer, o);
            if (nw_place_answer(&p, p.pos >= (p.probe < knows ? 2 : 3), &r)) { printf("R %d %d\n", seed, p.lo); break; }
        }
        nw_state_init(&s);
        s.next = (uint16_t)((seed * 37) % 690);
        s.done_day = s.day;
        static nw_session_t sess;
        nw_rng_seed(&r, (uint32_t)(seed + 100));
        nw_build_extra(&s, &sess, &r);
        for (int k = 0; k < sess.len; k++) {
            const nw_question_t *q = &sess.q[k];
            opts(q, o, sizeof(o));
            printf("E %d %d %d %d %d %s\n", seed, q->type, q->idx, (q->flags & NW_QF_NEW) ? 1 : 0, q->answer, o);
        }
        nw_state_init(&s);
        for (int i = 0; i < 600; i += 3) s.box[i] = (uint8_t)(1 + i % 5);
        nw_rng_seed(&r, (uint32_t)(seed + 200));
        for (int k = 0; k < 10; k++) {
            nw_question_t q;
            nw_challenge_question(&s, &q, &r);
            opts(&q, o, sizeof(o));
            printf("C %d %d %d %s\n", seed, q.idx, q.answer, o);
        }
    }
    return 0;
}
