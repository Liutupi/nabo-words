/*
 * 出题和今日学习的流程：题型、干扰项、补字母、学习队列、结业考、答题后的处理。
 * 对应 prototype/quiz.js 和 prototype/app.js 里的 answer() / finishSession()。
 */
#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "nabo_words_model.h"

typedef enum {
    NW_Q_INTRO = 0,   /* 新词卡 */
    NW_Q_EN2ZH,       /* 英选中 */
    NW_Q_ZH2EN,       /* 中选英 */
    NW_Q_LETTER,      /* 补字母 */
    NW_Q_FLASH,       /* 闪卡自评 */
} nw_qtype_t;

enum {
    NW_QF_FIRST = 1 << 0,   /* 本次学习里第一次作答（重问不改盒子） */
    NW_QF_NEW = 1 << 1,     /* 新词检测 */
    NW_QF_RETRY = 1 << 2,   /* 答错后的重问 */
};

typedef struct {
    uint16_t idx;
    uint8_t type;           /* nw_qtype_t */
    uint8_t flags;
    uint16_t opt[3];        /* 选择题：词的序号（显示它的释义或拼写）；补字母：候选字母 */
    uint8_t n_opt;
    uint8_t answer;         /* 正确选项在 opt 里的位置 */
    uint8_t pos;            /* 补字母：挖掉的位置 */
} nw_question_t;

/* 最多 240 道复习 + 20 张新词卡 + 20 道新词检测，再留出重问的余量 */
#define NW_QUEUE_MAX 400

typedef struct {
    nw_question_t q[NW_QUEUE_MAX];
    uint16_t len, pos, total;
    uint16_t right, wrong;          /* 只算第一次作答 */
    bool exam;
    uint16_t plan_reviews, plan_fresh;
    uint16_t stamps_before;
    uint16_t exam_wrong[NW_EXAM_Q]; /* 结业考答错的词（去重） */
    uint8_t n_exam_wrong;
} nw_session_t;

nw_qtype_t nw_review_type(int box, uint16_t idx);
void nw_make_question(nw_question_t *q, nw_qtype_t type, uint16_t idx, uint8_t flags, nw_rng_t *rng);

void nw_build_session(const nw_state_t *s, nw_session_t *sess, nw_rng_t *rng, nw_work_t *work);
void nw_build_exam(const nw_state_t *s, nw_session_t *sess, nw_rng_t *rng, nw_work_t *work);
void nw_requeue(nw_session_t *sess, const nw_question_t *q, nw_rng_t *rng);

/* 当前题作答：记对错、改盒子（新词检测和结业考不改）、答错时在队尾加一道重问 */
void nw_session_answer(nw_state_t *s, nw_session_t *sess, bool correct, nw_rng_t *rng);
/* 新词卡按 OK：记为已学 */
void nw_session_intro_seen(nw_state_t *s, const nw_session_t *sess);

typedef struct {
    bool exam, passed;
    uint8_t gained;         /* 新得到的成就 */
    bool freeze_gained;
    int stamps_gained;
} nw_session_result_t;

/* 队列做完：普通学习记完成今日学习，结业考按答对数通过或不通过 */
void nw_finish_session(nw_state_t *s, const nw_session_t *sess, nw_session_result_t *res);

/* 学习用的随机数种子，和原型 app.js 的 sessionRng() 一致 */
uint32_t nw_session_seed(const nw_state_t *s);
