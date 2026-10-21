// 开局分级测试、学有余力的加学、60 秒闪电挑战的规则（固件 v0.2.0 起有，规则和 firmware/main/words/nabo_words_extra.c 一致）。
// 对拍：node tests/words_host/extra_parity.js 和 C 逐题比较。纯逻辑，不碰画面。
'use strict';

// ---------- 分级测试：每轮考一个学段 10 题，答对 8 题算过，二分查找起点，最多 3 轮 30 题 ----------
const PLACE_Q = 10, PLACE_PASS = 8;
function placeRound(p, rnd) {
  p.probe = Math.floor((p.lo + p.hi) / 2); p.pos = 0; p.right = 0;
  const st = STAGES[p.probe], n = st.end - st.start, pick = [];
  p.q = [];
  for (let k = 0; k < PLACE_Q; k++) {
    let w;
    do { w = st.start + Math.floor(rnd() * n); } while (pick.includes(w));
    pick.push(w);
    p.q.push(makeQuestion('en2zh', w, rnd));
  }
}
function placeStart(rnd) { const p = { lo: 0, hi: STAGES.length - 1, rounds: 0 }; placeRound(p, rnd); return p; }
// 返回 true 表示测试结束，起点学段是 p.lo
function placeAnswer(p, ok, rnd) {
  if (ok) p.right++;
  if (++p.pos < PLACE_Q) return false;
  p.rounds++;
  if (p.right >= PLACE_PASS) p.lo = p.probe + 1; else p.hi = p.probe;
  if (p.lo >= p.hi) { p.lo = Math.min(p.lo, STAGES.length - 1); return true; }
  placeRound(p, rnd);
  return false;
}
// 从第 stage 学段开始：之前学段的词记为毕业（不再排期，在每天的毕业抽查里自然复习）
function placeApply(s, stage) {
  if (stage <= 0 || stage >= STAGES.length) return;
  const end = STAGES[stage].start;
  for (let i = 0; i < end; i++) { s.box[i] = GRADUATED; s.due[i] = NEVER; }
  s.stage = Math.max(s.stage, stage); s.next = Math.max(s.next, end);
}

// ---------- 加学：今天学完以后再学一组 5 个新词，一天最多 4 组 ----------
const EXTRA_WORDS = 5, EXTRA_GROUPS = 4;
function extraAvailable(s, groupsToday) {
  return s.doneDay === s.day && groupsToday < EXTRA_GROUPS && s.stage < STAGES.length && s.next < STAGES[s.stage].end;
}
function buildExtra(s, rnd) {
  const end = STAGES[Math.min(s.stage, STAGES.length - 1)].end, fresh = [];
  for (let i = s.next; i < end && fresh.length < EXTRA_WORDS; i++) fresh.push(i);
  const queue = fresh.map(i => makeQuestion('intro', i, rnd));
  for (const i of shuffle(fresh, rnd)) queue.push(makeQuestion('en2zh', i, rnd, { isNew: true }));
  return { queue, pos: 0, exam: false, extra: true, right: 0, wrong: 0, plan: { fresh: fresh.length, reviews: 0 } };
}

// ---------- 闪电挑战：60 秒抢答学过的词；答错的排进明天的复习（和对战答错一样） ----------
const CHALLENGE_MS = 60000, CHALLENGE_MIN = 20, CHALLENGE_COIN_PER = 5, CHALLENGE_COIN_MAX = 6;
function challengeQuestion(s, rnd) {
  const n = learned(s);
  let k = Math.floor(rnd() * Math.max(1, n)), w = 0;
  for (let i = 0; i < TOTAL_WORDS; i++) if (s.box[i] >= 1 && k-- === 0) { w = i; break; }
  return makeQuestion('en2zh', w, rnd);
}
function challengeMiss(s, idx) { if (s.box[idx] > 0 && s.due[idx] > s.day + 1) s.due[idx] = s.day + 1; }
function challengeCoins(score, firstToday) { return firstToday ? Math.min(CHALLENGE_COIN_MAX, Math.floor(score / CHALLENGE_COIN_PER)) : 0; }
