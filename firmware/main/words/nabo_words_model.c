/*
 * 《NABO 单词护照》纯逻辑：记忆盒、每日题单、学习日与打卡、结业考、成就、小卖部。
 * 和 prototype/core.js 一一对应，函数名去掉了驼峰。只用整数运算（ESP32-C3 没有浮点单元）。
 */
#include "nabo_words_model.h"

#include <string.h>

const uint16_t nw_stage_start[NW_STAGES] = {0, 700, 1600, 3500, 4500, 6000};
const uint16_t nw_stage_end[NW_STAGES] = {700, 1600, 3500, 4500, 6000, 7500};
const uint8_t nw_toy_price[NW_STAGES] = {5, 8, 12, 15, 20, 25};

/* 盒子 1..6 的复习间隔（天）；盒子 7 毕业，不再排期 */
static const uint8_t INTERVAL[NW_MAX_BOX] = {0, 1, 3, 7, 14, 30, 60};

void nw_state_init(nw_state_t *s)
{
    memset(s, 0, sizeof(*s));
    s->day = 1;
    s->new_per_day = 10;
    s->acc = NW_ACC_NONE;
    s->rating = 1000;
}

/* ---------- 统计 ---------- */

int nw_count_box(const nw_state_t *s, int from, int to, int min_box)
{
    int n = 0;
    for (int i = from; i < to; i++) {
        if (s->box[i] >= min_box) {
            n++;
        }
    }
    return n;
}

int nw_mastered(const nw_state_t *s)
{
    return nw_count_box(s, 0, NW_TOTAL, NW_MASTERED);
}

int nw_learned(const nw_state_t *s)
{
    return nw_count_box(s, 0, NW_TOTAL, 1);
}

bool nw_unit_stamped(const nw_state_t *s, int unit)
{
    return nw_count_box(s, unit * NW_UNIT, (unit + 1) * NW_UNIT, NW_FAMILIAR) == NW_UNIT;
}

int nw_stamp_count(const nw_state_t *s)
{
    int n = 0;
    for (int u = 0; u < NW_UNITS; u++) {
        if (nw_unit_stamped(s, u)) {
            n++;
        }
    }
    return n;
}

bool nw_all_done(const nw_state_t *s)
{
    return s->stage >= NW_STAGES;
}

/* 结业考：当前学段的词全部发过、90% 熟悉 */
bool nw_exam_ready(const nw_state_t *s)
{
    if (nw_all_done(s)) {
        return false;
    }
    int st = s->stage, n = nw_stage_end[st] - nw_stage_start[st];
    return s->next >= nw_stage_end[st]
           && nw_count_box(s, nw_stage_start[st], nw_stage_end[st], NW_FAMILIAR) * 100 >= NW_EXAM_UNLOCK_PCT * n;
}

int nw_sprite_form(const nw_state_t *s, int stage)
{
    if (stage < s->stage) {
        return 3;
    }
    int n = nw_stage_end[stage] - nw_stage_start[stage];
    int m100 = nw_count_box(s, nw_stage_start[stage], nw_stage_end[stage], NW_MASTERED) * 100;
    return m100 >= NW_TEEN_PCT * n ? 2 : m100 >= NW_HATCH_PCT * n ? 1 : 0;
}

/* ---------- 今日题单 ---------- */

int nw_review_budget_s(const nw_state_t *s)
{
    int b = NW_BUDGET_PER_NEW_S * s->new_per_day;
    return b > NW_BUDGET_MIN_S ? b : NW_BUDGET_MIN_S;
}

int nw_review_tenths(int box)
{
    return box <= 3 ? 40 : box == 4 ? 45 : 25;
}

static bool due_before(const nw_state_t *s, uint16_t a, uint16_t b)
{
    return s->due[a] != s->due[b] ? s->due[a] < s->due[b] : a < b;
}

/* 按（到期日，序号）排序。全序，所以结果唯一；Shell 排序不需要额外内存 */
static void sort_by_due(const nw_state_t *s, uint16_t *a, int n)
{
    for (int gap = n / 2; gap > 0; gap /= 2) {
        for (int i = gap; i < n; i++) {
            uint16_t v = a[i];
            int j = i;
            while (j >= gap && due_before(s, v, a[j - gap])) {
                a[j] = a[j - gap];
                j -= gap;
            }
            a[j] = v;
        }
    }
}

/*
 * 复习按到期先后排，累计预计用时到当天预算为止，排不下的留到明天；
 * 毕业词每天随机抽查 3 个，排在到期词后面。只有复习排不下时才减新词：
 * 积压超过 20 个减半，积压超过当天复习量停发。
 */
void nw_plan_today(const nw_state_t *s, nw_plan_t *plan, nw_work_t *work)
{
    memset(plan, 0, sizeof(*plan));
    int n_due = 0, n_grad = 0;
    for (int i = 0; i < s->next; i++) {
        if (s->box[i] > 0 && s->due[i] <= s->day) {
            work->a[n_due++] = (uint16_t)i;
        }
        if (s->box[i] == NW_GRADUATED) {
            work->b[n_grad++] = (uint16_t)i;
        }
    }
    sort_by_due(s, work->a, n_due);
    nw_rng_t pick;
    nw_rng_seed(&pick, (uint32_t)s->day * 7919u + 13u);
    nw_shuffle_u16(work->b, n_grad, &pick);
    int n_maint = n_grad < NW_MAINT_PER_DAY ? n_grad : NW_MAINT_PER_DAY;

    int budget = nw_review_budget_s(s) * 10, tenths = 0;
    for (int k = 0; k < n_due + n_maint; k++) {
        uint16_t i = k < n_due ? work->a[k] : work->b[k - n_due];
        int t = nw_review_tenths(s->box[i]);
        if (tenths + t > budget || plan->n_reviews >= NW_PLAN_MAX) {
            break;
        }
        plan->reviews[plan->n_reviews++] = i;
        tenths += t;
    }
    plan->backlog = (uint16_t)(n_due + n_maint - plan->n_reviews);

    int quota = s->new_per_day > NW_NEW_PER_DAY_MAX ? NW_NEW_PER_DAY_MAX : s->new_per_day;
    if (plan->backlog > plan->n_reviews) {
        quota = 0;
    } else if (plan->backlog > NW_BACKLOG_SOFT) {
        quota /= 2;
    }
    if (!nw_all_done(s)) {
        int limit = nw_stage_end[s->stage];
        for (int i = s->next; i < limit && plan->n_fresh < quota; i++) {
            plan->fresh[plan->n_fresh++] = (uint16_t)i;
        }
    }
    plan->seconds = (uint16_t)((tenths + plan->n_fresh * (NW_TENTHS_INTRO + NW_TENTHS_CHECK) + 5) / 10);
}

/* ---------- 记忆盒 ---------- */

static uint16_t day_plus(uint16_t day, int n)
{
    int d = day + n;
    return (uint16_t)(d >= (int)NW_NEVER ? (int)NW_NEVER - 1 : d);
}

/* first=false 表示本次学习里的重问，不改盒子 */
void nw_grade(nw_state_t *s, uint16_t idx, bool correct, bool first)
{
    if (!first) {
        return;
    }
    if (correct) {
        if (s->box[idx] < NW_MAX_BOX) {
            s->box[idx]++;
        }
    } else if (s->box[idx] > 1) {
        s->box[idx]--;
    } else {
        s->box[idx] = 1;
    }
    if (!correct) {
        s->due[idx] = day_plus(s->day, 1);
    } else if (s->box[idx] == NW_GRADUATED) {
        s->due[idx] = NW_NEVER;
    } else {
        s->due[idx] = day_plus(s->day, INTERVAL[s->box[idx]]);
    }
}

void nw_introduce(nw_state_t *s, uint16_t idx)
{
    if (s->box[idx] == 0) {
        s->box[idx] = 1;
        s->due[idx] = day_plus(s->day, 1);
    }
    if (s->next < idx + 1) {
        s->next = idx + 1;
    }
}

/* ---------- 学习日、打卡、结业考、成就 ---------- */

uint8_t nw_check_achievements(nw_state_t *s)
{
    uint8_t now = 0;
    int m = nw_mastered(s);
    if (m >= 100) now |= NW_ACH_W100;
    if (m >= 1000) now |= NW_ACH_W1000;
    if (s->best_streak >= 7) now |= NW_ACH_S7;
    if (s->best_streak >= 30) now |= NW_ACH_S30;
    if (!(s->achievements & NW_ACH_STAMP1) && nw_stamp_count(s) >= 1) now |= NW_ACH_STAMP1;
    if (s->exam_first == 1) now |= NW_ACH_EXAM1;
    if (s->win_streak >= 10) now |= NW_ACH_VS10;
    uint8_t gained = now & (uint8_t)~s->achievements;
    s->achievements |= gained;
    return gained;
}

int nw_finish_day(nw_state_t *s, bool perfect, uint8_t *gained)
{
    *gained = 0;
    if (s->done_day == s->day) {
        return 0;
    }
    int flags = 0;
    s->done_day = s->day;
    s->streak++;
    if (s->streak > s->best_streak) {
        s->best_streak = s->streak;
    }
    if (s->streak % NW_FREEZE_EVERY == 0 && s->freezes < NW_FREEZE_MAX) {
        s->freezes++;
        flags |= 1;
    }
    s->coins += perfect ? 3 : 2;
    *gained = nw_check_achievements(s);
    return flags;
}

nw_day_result_t nw_advance_day(nw_state_t *s)
{
    bool missed = s->done_day != s->day;
    s->day++;
    memset(s->vs, 0, sizeof(s->vs));
    if (!missed) {
        return NW_DAY_OK;
    }
    if (s->freezes > 0) {
        s->freezes--;
        return NW_DAY_FREEZE_USED;
    }
    if (s->streak) {
        s->streak = 0;
        return NW_DAY_STREAK_BROKEN;
    }
    return NW_DAY_OK;
}

uint8_t nw_pass_exam(nw_state_t *s)
{
    if (s->exam_first == 0) {
        s->exam_first = 1;
    }
    s->stage++;
    s->coins += 10;
    return nw_check_achievements(s);
}

void nw_fail_exam(nw_state_t *s, const uint16_t *wrong, int n)
{
    if (s->exam_first == 0) {
        s->exam_first = 2;
    }
    for (int k = 0; k < n; k++) {
        s->box[wrong[k]] = 1;
        s->due[wrong[k]] = day_plus(s->day, 1);
    }
}

/* ---------- 小卖部 ---------- */

static void spend(nw_state_t *s, int price)
{
    s->coins -= (uint32_t)price;
    s->spent += (uint32_t)price;
}

int nw_toy_stage(const nw_state_t *s)
{
    return s->stage < NW_STAGES - 1 ? s->stage : NW_STAGES - 1;
}

int nw_toy_price_now(const nw_state_t *s)
{
    return nw_toy_price[nw_toy_stage(s)];
}

/* 只开已解锁学段里还没有的摆件，价格按当前学段 */
int nw_open_toy_box(nw_state_t *s, nw_rng_t *rng)
{
    uint8_t owned[NW_TOYS] = {0}, pool[NW_TOYS];
    for (int k = 0; k < s->toy_count; k++) {
        owned[s->toys[k]] = 1;
    }
    int n = 0, unlocked = (nw_toy_stage(s) + 1) * NW_TOYS_PER_STAGE;
    for (int t = 0; t < unlocked; t++) {
        if (!owned[t]) {
            pool[n++] = (uint8_t)t;
        }
    }
    if (n == 0) {
        return s->stage < NW_STAGES - 1 ? NW_SHOP_SET_DONE : NW_SHOP_ALL_DONE;
    }
    int price = nw_toy_price_now(s);
    if (s->coins < (uint32_t)price) {
        return NW_SHOP_NO_COINS;
    }
    spend(s, price);
    uint8_t toy = pool[nw_rng_below(rng, (uint32_t)n)];
    s->toys[s->toy_count++] = toy;
    return toy;
}

int nw_buy_freeze(nw_state_t *s)
{
    if (s->freezes >= NW_FREEZE_MAX) {
        return NW_SHOP_FREEZE_FULL;
    }
    if (s->coins < NW_FREEZE_PRICE) {
        return NW_SHOP_NO_COINS;
    }
    spend(s, NW_FREEZE_PRICE);
    return ++s->freezes;
}

int nw_use_accessory(nw_state_t *s, int id)
{
    uint8_t bit = (uint8_t)(1u << id);
    if (!(s->accs & bit)) {
        if (s->coins < NW_ACC_PRICE) {
            return NW_SHOP_NO_COINS;
        }
        spend(s, NW_ACC_PRICE);
        s->accs |= bit;
        s->acc = (uint8_t)id;
        return 1;
    }
    s->acc = s->acc == id ? NW_ACC_NONE : (uint8_t)id;
    return s->acc == NW_ACC_NONE ? 3 : 2;
}
