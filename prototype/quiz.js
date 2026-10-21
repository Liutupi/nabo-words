// 出题：今日学习的题目队列、选项和干扰项、结业考、对战题。纯逻辑。
'use strict';

const REAL_SET = new Set(REAL_WORDS.map(w => w.w.toLowerCase()));
const LETTERS = 'abcdefghijklmnopqrstuvwxyz';

// 复习题型按盒子决定
function reviewType(box, idx) {
  if (box <= 2) return 'en2zh';
  if (box === 3) return 'zh2en';
  if (box === 4) return WORDS[idx].w.length >= 3 ? 'letter' : 'zh2en';
  return 'flash';
}

// 干扰项：同单元和相邻单元的词，释义 / 拼写不重复
function distractors(idx, n, rnd, field) {
  const u = unitOf(idx), lo = Math.max(0, (u - 1) * UNIT_SIZE), hi = Math.min(TOTAL_WORDS, (u + 2) * UNIT_SIZE);
  const seen = new Set([WORDS[idx][field]]), out = [];
  for (const j of shuffle(Array.from({ length: hi - lo }, (_, k) => lo + k), rnd)) {
    if (out.length >= n) break;
    const v = WORDS[j][field];
    if (!seen.has(v)) { seen.add(v); out.push(j); }
  }
  return out;
}

// 生成一道题：{ type, idx, first, isNew, prompt, sub, options[], answer }
function makeQuestion(type, idx, rnd, extra) {
  const w = WORDS[idx], q = { type, idx, first: true, isNew: false, ...extra };
  if (type === 'intro' || type === 'flash') return q;
  if (type === 'en2zh' || type === 'zh2en') {
    const field = type === 'en2zh' ? 'm' : 'w';
    const opts = shuffle([idx, ...distractors(idx, 2, rnd, field)], rnd);
    q.options = opts.map(j => WORDS[j][field]);
    q.answer = opts.indexOf(idx);
    return q;
  }
  // 补字母：挖掉一个字母，候选不能拼成另一个真词
  const word = w.w.toLowerCase();
  const pos = word.length > 2 ? 1 + Math.floor(rnd() * (word.length - 1)) : 0;
  const right = word[pos], cand = [right];
  for (const c of shuffle(LETTERS.split(''), rnd)) {
    if (cand.length >= 3) break;
    const alt = word.slice(0, pos) + c + word.slice(pos + 1);
    if (!cand.includes(c) && !REAL_SET.has(alt)) cand.push(c);
  }
  const opts = shuffle(cand, rnd);
  q.pos = pos; q.options = opts; q.answer = opts.indexOf(right);
  return q;
}

// 今日学习队列：新词每 5 个一组，先看卡片再检测；复习穿插在组之间
function buildSession(s, rnd) {
  const { reviews, fresh } = planToday(s);
  const revQ = shuffle(reviews, rnd).map(i => makeQuestion(reviewType(s.box[i], i), i, rnd));
  const queue = [];
  for (let k = 0; k < fresh.length; k += 5) {
    queue.push(...revQ.splice(0, 5));
    const group = fresh.slice(k, k + 5);
    for (const i of group) queue.push(makeQuestion('intro', i, rnd));
    for (const i of shuffle(group, rnd)) queue.push(makeQuestion('en2zh', i, rnd, { isNew: true }));
  }
  queue.push(...revQ);
  return { queue, pos: 0, total: queue.length, right: 0, wrong: 0, wrongSet: new Set(), stampsBefore: stampCount(s),
    plan: { reviews: reviews.length, fresh: fresh.length } };
}

// 答错后在队尾加一道重问（不改盒子）
function requeue(sess, q, rnd) {
  sess.queue.push(makeQuestion('en2zh', q.idx, rnd, { first: false, retry: true }));
}

// 当前题作答：记对错（只算第一次作答）、改盒子（新词检测和结业考不改）、答错时在队尾加一道重问
function sessionAnswer(s, sess, ok, rnd) {
  const q = sess.queue[sess.pos];
  if (q.first) { if (ok) sess.right++; else { sess.wrong++; sess.wrongSet.add(q.idx); } }
  if (!sess.exam && !q.isNew) grade(s, q.idx, ok, q.first);
  if (!ok && !sess.exam) requeue(sess, q, rnd);
}
// 队列做完：普通学习记完成今日学习，结业考按答对数通过或不通过
function sessionFinish(s, sess) {
  if (sess.exam) {
    const passed = sess.right >= EXAM_PASS;
    const notes = passed ? passExam(s) : (failExam(s, [...sess.wrongSet]), ['错题已放回复习盒子']);
    return { exam: true, passed, notes, stampsGained: 0 };
  }
  const notes = finishDay(s, sess.wrong === 0);
  return { exam: false, passed: false, notes, stampsGained: stampCount(s) - sess.stampsBefore };
}

function buildExam(s, rnd) {
  const st = STAGES[s.stage], pool = Array.from({ length: st.end - st.start }, (_, k) => st.start + k);
  const types = ['en2zh', 'zh2en', 'letter'];
  const queue = shuffle(pool, rnd).slice(0, EXAM_Q).map((i, n) => makeQuestion(WORDS[i].w.length >= 3 ? types[n % 3] : 'en2zh', i, rnd));
  return { queue, pos: 0, total: queue.length, right: 0, wrong: 0, wrongSet: new Set(), exam: true };
}

// 对战出题范围：双方都学过的词；不足 30 个时用较低一方学过的词
function battlePool(boxA, learnedB) {
  const both = [];
  for (let i = 0; i < TOTAL_WORDS; i++) if (boxA[i] > 0 && learnedB(i)) both.push(i);
  if (both.length >= 30) return both;
  const lowA = boxA.filter(b => b > 0).length;
  const fallback = [];
  for (let i = 0; i < TOTAL_WORDS && fallback.length < Math.max(30, lowA); i++) if (boxA[i] > 0 || learnedB(i) || i < 100) fallback.push(i);
  return fallback;
}
function buildBattle(pool, seed, n) {
  const rnd = mulberry32(seed);
  const real = pool.filter(i => WORDS[i].real), use = real.length >= n ? real : pool;
  return shuffle(use, rnd).slice(0, n).map(i => makeQuestion('en2zh', i, rnd));
}
