// 纯逻辑：随机数、存档、Leitner 记忆盒、学习日、奖励与成长。不碰画面，固件 model 层照此移植。
'use strict';

// ---------- 可复现随机数（mulberry32），固件用同一算法 ----------
function mulberry32(a) {
  return function () {
    a |= 0; a = (a + 0x6D2B79F5) | 0;
    let t = Math.imul(a ^ (a >>> 15), 1 | a);
    t = (t + Math.imul(t ^ (t >>> 7), 61 | t)) ^ t;
    return ((t ^ (t >>> 14)) >>> 0) / 4294967296;
  };
}
function shuffle(arr, rnd) {
  const a = arr.slice();
  for (let i = a.length - 1; i > 0; i--) { const j = Math.floor(rnd() * (i + 1)); [a[i], a[j]] = [a[j], a[i]]; }
  return a;
}

// ---------- 记忆盒 ----------
const INTERVAL = [0, 1, 2, 4, 7, 15, 30, 60];   // 盒子 1..7 的复习间隔（天）
const MAX_BOX = 7, FAMILIAR = 3, MASTERED = 4;
const REVIEW_CAP = 80, REVIEW_SOFT = 50;
const EXAM_UNLOCK = 0.9, EXAM_Q = 20, EXAM_PASS = 16;
const STREAK_FREEZE_EVERY = 7, FREEZE_MAX = 2;

const SAVE_KEY = 'nabo.words.v1';

function newState() {
  return {
    day: 1, box: new Array(TOTAL_WORDS).fill(0), due: new Array(TOTAL_WORDS).fill(0),
    next: 0,              // 下一个新词的序号（按词库顺序学）
    stage: 0,             // 已解锁的学段
    newPerDay: 10, doneDay: 0, streak: 0, bestStreak: 0, freezes: 0,
    coins: 0, toys: [], hidden: [], rating: 1000, wins: 0, losses: 0, winStreak: 0,
    examTried: 0, examLog: [], achievements: [], history: [], vsToday: {},
  };
}
function loadState() {
  try {
    const raw = localStorage.getItem(SAVE_KEY);
    if (raw) return { ...newState(), ...JSON.parse(raw) };
  } catch { /* 无痕模式等：用新存档 */ }
  return newState();
}
function saveState(s) { try { localStorage.setItem(SAVE_KEY, JSON.stringify(s)); } catch { /* 忽略 */ } }

// ---------- 统计 ----------
function countBox(s, from, to, min) { let n = 0; for (let i = from; i < to; i++) if (s.box[i] >= min) n++; return n; }
function mastered(s) { return countBox(s, 0, TOTAL_WORDS, MASTERED); }
function learned(s) { return countBox(s, 0, TOTAL_WORDS, 1); }
function boxHistogram(s) { const h = new Array(MAX_BOX + 1).fill(0); for (const b of s.box) h[b]++; return h; }
function dueList(s) { const r = []; for (let i = 0; i < s.next; i++) if (s.box[i] > 0 && s.due[i] <= s.day) r.push(i); return r; }
function stageProgress(s, st) {
  const n = st.end - st.start;
  return { familiar: countBox(s, st.start, st.end, FAMILIAR) / n, mastered: countBox(s, st.start, st.end, MASTERED) / n };
}
function unitStamped(s, u) { return countBox(s, u * UNIT_SIZE, (u + 1) * UNIT_SIZE, FAMILIAR) === UNIT_SIZE; }
function stampCount(s) { let n = 0; for (let u = 0; u < TOTAL_UNITS; u++) if (unitStamped(s, u)) n++; return n; }

// 结业考：当前学段的词全部发过、90% 熟悉
function examReady(s) {
  if (s.stage >= STAGES.length) return false;
  const st = STAGES[s.stage];
  return s.next >= st.end && stageProgress(s, st).familiar >= EXAM_UNLOCK;
}
function allDone(s) { return s.stage >= STAGES.length; }

// 精灵形态：0 蛋 1 幼崽 2 少年 3 完全体
function spriteForm(s, stageIdx) {
  if (stageIdx < s.stage) return 3;
  const m = stageProgress(s, STAGES[stageIdx]).mastered;
  return m >= 0.45 ? 2 : m >= 0.10 ? 1 : 0;
}

// ---------- 今日学习的题单 ----------
// 返回 { reviews: [idx], fresh: [idx] }
function planToday(s) {
  const due = dueList(s).sort((a, b) => s.due[a] - s.due[b] || a - b).slice(0, REVIEW_CAP);
  let quota = s.newPerDay;
  if (due.length > REVIEW_CAP - 1) quota = 0; else if (due.length > REVIEW_SOFT) quota = Math.floor(quota / 2);
  const fresh = [];
  if (!allDone(s)) {
    const limit = STAGES[s.stage].end;
    for (let i = s.next; i < limit && fresh.length < quota; i++) fresh.push(i);
  }
  return { reviews: due, fresh };
}

// 答题结果写回记忆盒。first=true 表示本次学习里这个词第一次作答（重问不改盒子）
function grade(s, idx, correct, first) {
  if (!first) return;
  if (correct) s.box[idx] = Math.min(MAX_BOX, s.box[idx] + 1);
  else s.box[idx] = Math.max(1, s.box[idx] - 2);
  s.due[idx] = s.day + (correct ? INTERVAL[s.box[idx]] : 1);
}
function introduce(s, idx) { if (s.box[idx] === 0) { s.box[idx] = 1; s.due[idx] = s.day + 1; } s.next = Math.max(s.next, idx + 1); }

// ---------- 学习日、打卡、奖励 ----------
// 完成今日学习。返回奖励说明
function finishDay(s, perfect) {
  if (s.doneDay === s.day) return [];
  const notes = [];
  s.doneDay = s.day;
  s.streak += 1; s.bestStreak = Math.max(s.bestStreak, s.streak);
  if (s.streak % STREAK_FREEZE_EVERY === 0 && s.freezes < FREEZE_MAX) { s.freezes++; notes.push('连续 ' + s.streak + ' 天，得到 1 张补签卡'); }
  s.coins += perfect ? 3 : 2;
  notes.push(perfect ? '全对，金币 +3' : '金币 +2');
  return notes.concat(checkAchievements(s));
}
// 进入下一天（固件里由学习时钟触发）
function advanceDay(s) {
  const missed = s.doneDay !== s.day;
  s.history.push({ day: s.day, mastered: mastered(s), done: !missed });
  if (s.history.length > 800) s.history.shift();
  s.day += 1; s.vsToday = {};
  if (missed) {
    if (s.freezes > 0) { s.freezes--; return '昨天没学，用掉 1 张补签卡'; }
    if (s.streak) { s.streak = 0; return '连续打卡中断'; }
  }
  return '';
}

function passExam(s) {
  const st = STAGES[s.stage];
  s.examLog.push({ day: s.day, stage: st.name, pass: true });
  s.stage += 1; s.coins += 10;
  return [`${st.name}结业！金币 +10`, s.stage < STAGES.length ? `解锁${STAGES[s.stage].name}：${STAGES[s.stage].region}` : '环游完成，换上学士服'].concat(checkAchievements(s));
}
function failExam(s, wrongIdx) {
  s.examLog.push({ day: s.day, stage: STAGES[s.stage].name, pass: false });
  for (const i of wrongIdx) { s.box[i] = 1; s.due[i] = s.day + 1; }
}

const ACHIEVEMENTS = [
  ['w100', '第一个 100 词', s => mastered(s) >= 100],
  ['w1000', '词汇量 1000', s => mastered(s) >= 1000],
  ['s7', '连续 7 天', s => s.bestStreak >= 7],
  ['s30', '连续 30 天', s => s.bestStreak >= 30],
  ['stamp1', '第一枚护照章', s => stampCount(s) >= 1],
  ['exam1', '一次通过结业考', s => s.examLog.length > 0 && s.examLog[0].pass],
  ['vs10', '对战 10 连胜', s => s.winStreak >= 10],
];
function checkAchievements(s) {
  const got = [];
  for (const [id, name, ok] of ACHIEVEMENTS) if (!s.achievements.includes(id) && ok(s)) { s.achievements.push(id); got.push('成就：' + name); }
  return got;
}

function openToyBox(s, rnd) {
  if (s.coins < 5) return null;
  const left = BOX_TOYS.filter(t => !s.toys.includes(t));
  s.coins -= 5;
  if (!left.length) { s.coins += 2; return '摆件已集齐，退回 2 枚金币'; }
  const t = left[Math.floor(rnd() * left.length)];
  s.toys.push(t);
  return t;
}

// ---------- 时间快进模拟 ----------
// acc：新词检测的正确率；lazy：某天不学的概率
function simulateDay(s, rnd, acc, lazy) {
  if (rnd() >= lazy && s.doneDay !== s.day) {
    const { reviews, fresh } = planToday(s);
    let wrong = 0;
    for (const i of reviews) { const ok = rnd() < Math.min(0.98, acc + s.box[i] * 0.03); if (!ok) wrong++; grade(s, i, ok, true); }
    for (const i of fresh) { introduce(s, i); if (rnd() >= acc) wrong++; }   // 新词检测不改盒子，只决定是否重问
    finishDay(s, wrong === 0);
    if (examReady(s)) {
      const st = STAGES[s.stage], pool = [];
      for (let i = st.start; i < st.end; i++) pool.push(i);
      const qs = shuffle(pool, rnd).slice(0, EXAM_Q), bad = qs.filter(i => rnd() >= Math.min(0.98, acc + s.box[i] * 0.03));
      if (EXAM_Q - bad.length >= EXAM_PASS) passExam(s); else failExam(s, bad);
    }
  }
  advanceDay(s);
}
