/*
 * 出题和今日学习的流程，对应 prototype/quiz.js 和 prototype/app.js 的 answer() / finishSession()。
 * 随机数的消耗顺序必须和原型完全一样，否则同一个种子出的题会不同。
 */
#include "nabo_words_quiz.h"

#include <string.h>

#define LETTERS "abcdefghijklmnopqrstuvwxyz"
#define MAX_WORD 40

static int unit_of(uint16_t idx)
{
    return idx / NW_UNIT;
}

nw_qtype_t nw_review_type(int box, uint16_t idx)
{
    if (box <= 2) return NW_Q_EN2ZH;
    if (box == 3) return NW_Q_ZH2EN;
    if (box == 4) return strlen(nw_db_word(idx)) >= 3 ? NW_Q_LETTER : NW_Q_ZH2EN;
    return NW_Q_FLASH;
}

/* 选择题显示的文字：英选中显示释义，中选英显示拼写；干扰项按这个文字去重 */
static const char *field(uint16_t idx, bool meaning)
{
    return meaning ? nw_db_meaning(idx) : nw_db_word(idx);
}

/* 干扰项：同单元和前后各一个单元里的词，先整段洗牌，再按顺序挑文字不重复的 */
static int distractors(uint16_t idx, int n, nw_rng_t *rng, bool meaning, uint16_t *out)
{
    int u = unit_of(idx);
    int lo = (u - 1) * NW_UNIT < 0 ? 0 : (u - 1) * NW_UNIT;
    int hi = (u + 2) * NW_UNIT > NW_TOTAL ? NW_TOTAL : (u + 2) * NW_UNIT;
    uint16_t range[3 * NW_UNIT];
    int len = hi - lo;
    for (int k = 0; k < len; k++) {
        range[k] = (uint16_t)(lo + k);
    }
    nw_shuffle_u16(range, len, rng);
    const char *seen[3] = {field(idx, meaning)};
    int n_seen = 1, got = 0;
    for (int k = 0; k < len && got < n; k++) {
        const char *v = field(range[k], meaning);
        bool dup = false;
        for (int j = 0; j < n_seen; j++) {
            if (strcmp(seen[j], v) == 0) {
                dup = true;
                break;
            }
        }
        if (!dup) {
            seen[n_seen++] = v;
            out[got++] = range[k];
        }
    }
    return got;
}

static void lower_copy(char *dst, const char *src)
{
    int k = 0;
    for (; src[k] && k < MAX_WORD - 1; k++) {
        char c = src[k];
        dst[k] = (c >= 'A' && c <= 'Z') ? (char)(c - 'A' + 'a') : c;
    }
    dst[k] = 0;
}

/* 补字母：挖掉一个字母，候选不能让单词变成词库里的另一个词 */
static void make_letter(nw_question_t *q, uint16_t idx, nw_rng_t *rng)
{
    char word[MAX_WORD], alt[MAX_WORD];
    lower_copy(word, nw_db_word(idx));
    int len = (int)strlen(word);
    q->pos = (uint8_t)(len > 2 ? 1 + nw_rng_below(rng, (uint32_t)(len - 1)) : 0);
    uint8_t cand[3] = {(uint8_t)word[q->pos]};
    int n = 1;
    uint8_t letters[26];
    memcpy(letters, LETTERS, 26);
    nw_shuffle_u8(letters, 26, rng);
    for (int k = 0; k < 26 && n < 3; k++) {
        uint8_t c = letters[k];
        bool have = false;
        for (int j = 0; j < n; j++) {
            have |= cand[j] == c;
        }
        memcpy(alt, word, (size_t)len + 1);
        alt[q->pos] = (char)c;
        if (!have && !nw_db_has_lower(alt)) {
            cand[n++] = c;
        }
    }
    nw_shuffle_u8(cand, n, rng);
    q->n_opt = (uint8_t)n;
    for (int j = 0; j < n; j++) {
        q->opt[j] = cand[j];
        if (cand[j] == (uint8_t)word[q->pos]) {
            q->answer = (uint8_t)j;
        }
    }
}

void nw_make_question(nw_question_t *q, nw_qtype_t type, uint16_t idx, uint8_t flags, nw_rng_t *rng)
{
    memset(q, 0, sizeof(*q));
    q->idx = idx;
    q->type = (uint8_t)type;
    q->flags = flags;
    if (type == NW_Q_INTRO || type == NW_Q_FLASH) {
        return;
    }
    if (type == NW_Q_LETTER) {
        make_letter(q, idx, rng);
        return;
    }
    uint16_t opts[3] = {idx};
    int n = 1 + distractors(idx, 2, rng, type == NW_Q_EN2ZH, opts + 1);
    nw_shuffle_u16(opts, n, rng);
    q->n_opt = (uint8_t)n;
    for (int j = 0; j < n; j++) {
        q->opt[j] = opts[j];
        if (opts[j] == idx) {
            q->answer = (uint8_t)j;
        }
    }
}

static void push(nw_session_t *sess, const nw_question_t *q)
{
    if (sess->len < NW_QUEUE_MAX) {
        sess->q[sess->len++] = *q;
    }
}

/*
 * 新词每 5 个一组：先看卡片，再按洗过的顺序检测；每组前面插 5 道复习，剩下的复习放最后。
 * 复习题先全部生成（和原型一样先消耗随机数），暂存在队列尾部，再按顺序搬到前面：
 * 写的位置永远落后于读的位置（新词卡和检测最多 40 道），所以不会覆盖还没搬的题。
 */
void nw_build_session(const nw_state_t *s, nw_session_t *sess, nw_rng_t *rng, nw_work_t *work)
{
    nw_plan_t plan;
    nw_plan_today(s, &plan, work);
    memset(sess, 0, sizeof(*sess));
    sess->plan_reviews = plan.n_reviews;
    sess->plan_fresh = plan.n_fresh;
    sess->stamps_before = (uint16_t)nw_stamp_count(s);

    uint16_t order[NW_PLAN_MAX];
    memcpy(order, plan.reviews, plan.n_reviews * sizeof(uint16_t));
    nw_shuffle_u16(order, plan.n_reviews, rng);
    int tail = NW_QUEUE_MAX - NW_PLAN_MAX;
    for (int k = 0; k < plan.n_reviews; k++) {
        nw_make_question(&sess->q[tail + k], nw_review_type(s->box[order[k]], order[k]), order[k], NW_QF_FIRST, rng);
    }
    int r = 0;
    for (int k = 0; k < plan.n_fresh; k += 5) {
        for (int j = 0; j < 5 && r < plan.n_reviews; j++) {
            sess->q[sess->len++] = sess->q[tail + r++];
        }
        int g = plan.n_fresh - k < 5 ? plan.n_fresh - k : 5;
        uint16_t group[5];
        memcpy(group, plan.fresh + k, (size_t)g * sizeof(uint16_t));
        for (int j = 0; j < g; j++) {
            nw_make_question(&sess->q[sess->len++], NW_Q_INTRO, group[j], NW_QF_FIRST, rng);
        }
        nw_shuffle_u16(group, g, rng);
        for (int j = 0; j < g; j++) {
            nw_make_question(&sess->q[sess->len++], NW_Q_EN2ZH, group[j], NW_QF_FIRST | NW_QF_NEW, rng);
        }
    }
    while (r < plan.n_reviews) {
        sess->q[sess->len++] = sess->q[tail + r++];
    }
    sess->total = sess->len;
}

void nw_build_exam(const nw_state_t *s, nw_session_t *sess, nw_rng_t *rng, nw_work_t *work)
{
    static const uint8_t TYPES[3] = {NW_Q_EN2ZH, NW_Q_ZH2EN, NW_Q_LETTER};
    memset(sess, 0, sizeof(*sess));
    sess->exam = true;
    if (nw_all_done(s)) {
        return;
    }
    int start = nw_stage_start[s->stage], n = nw_stage_end[s->stage] - start;
    for (int k = 0; k < n; k++) {
        work->a[k] = (uint16_t)(start + k);
    }
    nw_shuffle_u16(work->a, n, rng);
    for (int k = 0; k < NW_EXAM_Q && k < n; k++) {
        uint16_t i = work->a[k];
        nw_qtype_t t = strlen(nw_db_word(i)) >= 3 ? (nw_qtype_t)TYPES[k % 3] : NW_Q_EN2ZH;
        nw_make_question(&sess->q[sess->len++], t, i, NW_QF_FIRST, rng);
    }
    sess->total = sess->len;
}

void nw_requeue(nw_session_t *sess, const nw_question_t *q, nw_rng_t *rng)
{
    nw_question_t retry;
    nw_make_question(&retry, NW_Q_EN2ZH, q->idx, NW_QF_RETRY, rng);
    push(sess, &retry);
}

void nw_session_answer(nw_state_t *s, nw_session_t *sess, bool correct, nw_rng_t *rng)
{
    const nw_question_t q = sess->q[sess->pos];
    bool first = q.flags & NW_QF_FIRST;
    if (first) {
        if (correct) {
            sess->right++;
        } else {
            sess->wrong++;
            bool seen = false;
            for (int k = 0; k < sess->n_exam_wrong; k++) {
                seen |= sess->exam_wrong[k] == q.idx;
            }
            if (sess->exam && !seen && sess->n_exam_wrong < NW_EXAM_Q) {
                sess->exam_wrong[sess->n_exam_wrong++] = q.idx;
            }
        }
    }
    if (!sess->exam && !(q.flags & NW_QF_NEW)) {
        nw_grade(s, q.idx, correct, first);
    }
    if (!correct && !sess->exam) {
        nw_requeue(sess, &q, rng);
    }
}

void nw_session_intro_seen(nw_state_t *s, const nw_session_t *sess)
{
    nw_introduce(s, sess->q[sess->pos].idx);
}

void nw_finish_session(nw_state_t *s, const nw_session_t *sess, nw_session_result_t *res)
{
    memset(res, 0, sizeof(*res));
    res->exam = sess->exam;
    if (sess->exam) {
        res->passed = sess->right >= NW_EXAM_PASS;
        if (res->passed) {
            res->gained = nw_pass_exam(s);
        } else {
            nw_fail_exam(s, sess->exam_wrong, sess->n_exam_wrong);
        }
        return;
    }
    res->freeze_gained = nw_finish_day(s, sess->wrong == 0, &res->gained) & 1;
    res->stamps_gained = nw_stamp_count(s) - sess->stamps_before;
}

uint32_t nw_session_seed(const nw_state_t *s)
{
    return (uint32_t)s->day * 2654435761u + (uint32_t)s->next * 97u + NW_TOTAL;
}
