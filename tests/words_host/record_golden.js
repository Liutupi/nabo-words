// 用原型的纯逻辑（prototype/core.js、quiz.js）录对拍轨迹，C 版本逐步重放核对。
// 用法：node tests/words_host/record_golden.js   → 写 tests/words_host/golden/*.json
// 改了原型规则后要重录。
'use strict';
const fs = require('fs'), path = require('path'), vm = require('vm');

const ROOT = path.resolve(__dirname, '..', '..');
const ctx = { localStorage: { getItem: () => null, setItem() {} }, console };
ctx.window = ctx;
vm.createContext(ctx);
for (const f of ['assets/words_data.js', 'words.js', 'core.js', 'quiz.js'])
  vm.runInContext(fs.readFileSync(path.join(ROOT, 'prototype', f), 'utf8'), ctx, { filename: f });
const P = new Proxy(ctx, { get: (t, k) => vm.runInContext(String(k), t) });   // 读原型里的 const

const { mulberry32, shuffle } = ctx;
const Q_TYPE = { intro: 0, en2zh: 1, zh2en: 2, letter: 3, flash: 4 };
const ACH = ['w100', 'w1000', 's7', 's30', 'stamp1', 'exam1', 'vs10'];

function hash(s) {   // FNV-1a，盒子和到期日（毕业词的 1e9 按 0xFFFF 算）
  let h = 2166136261 >>> 0;
  const mix = b => { h ^= b; h = Math.imul(h, 16777619) >>> 0; };
  for (let i = 0; i < P.TOTAL_WORDS; i++) { const d = Math.min(s.due[i], 0xFFFF); mix(s.box[i]); mix(d & 255); mix(d >> 8); }
  return h;
}
function digest(s) {
  const st = Math.min(s.stage, P.STAGES.length - 1);
  return {
    day: s.day, next: s.next, stage: s.stage, streak: s.streak, best: s.bestStreak, freezes: s.freezes,
    coins: s.coins, spent: s.spent, done: s.doneDay, rating: s.rating, wins: s.wins, losses: s.losses, ws: s.winStreak,
    toys: s.toys.map(t => P.BOX_TOYS.indexOf(t)),
    accs: s.accs.reduce((m, id) => m | (1 << P.ACCESSORIES.findIndex(a => a.id === id)), 0),
    acc: s.acc === null ? 255 : P.ACCESSORIES.findIndex(a => a.id === s.acc),
    ach: s.achievements.reduce((m, id) => m | (1 << ACH.indexOf(id)), 0),
    exam1: s.examLog.length ? (s.examLog[0].pass ? 1 : 2) : 0,
    mastered: ctx.mastered(s), learned: ctx.learned(s), stamps: ctx.stampCount(s),
    form: ctx.spriteForm(s, Math.min(st, 5)), examReady: ctx.examReady(s), hash: hash(s),
  };
}
const qrec = q => [Q_TYPE[q.type], q.idx, (q.first ? 1 : 0) | (q.isNew ? 2 : 0) | (q.retry ? 4 : 0),
  q.options || [], q.answer ?? 0, q.pos ?? 0];
const sessionSeed = s => (s.day * 2654435761 + s.next * 97 + s.box.length) >>> 0;   // app.js 的 sessionRng()
const pOk = (s, i, acc) => Math.min(0.98, acc + s.box[i] * 0.03);

// 按 app.js 的流程做一整个学习队列；答对与否由 r 决定并记下来
function runSession(s, sess, r, acc, rnd) {
  const init = sess.queue.map(qrec), ans = [];
  while (sess.pos < sess.queue.length) {
    const q = sess.queue[sess.pos];
    if (q.type === 'intro') { ctx.introduce(s, q.idx); sess.pos++; continue; }
    const ok = r() < pOk(s, q.idx, acc);
    ans.push(ok ? 1 : 0);
    ctx.sessionAnswer(s, sess, ok, rnd);
    sess.pos++;
  }
  const res = ctx.sessionFinish(s, sess);
  return { seed: 0, init, ans, final: sess.queue.map(qrec), right: sess.right, wrong: sess.wrong,
    passed: res.passed, stamps: res.stampsGained };
}

function peerBitmap(s, r, kind) {   // 对手学过的词：0 和我差不多，1 比我少很多，2 随机稀疏
  const bits = new Uint8Array(Math.ceil(P.TOTAL_WORDS / 8));
  const learned = i => kind === 0 ? (s.box[i] > 0 && r() < 0.9) || (i >= s.next && i < s.next + 40)
    : kind === 1 ? i < 25 : r() < 0.02;
  for (let i = 0; i < P.TOTAL_WORDS; i++) if (learned(i)) bits[i >> 3] |= 1 << (i & 7);
  return bits;
}

function trace(name, cfg) {
  const s = ctx.newState(); s.newPerDay = cfg.newPer;
  const r = mulberry32(cfg.seed), shopRnd = mulberry32(cfg.seed ^ 0x5A5A);
  const days = [];
  for (let d = 0; d < cfg.days; d++) {
    const rec = { d: s.day };
    if (r() < cfg.lazy) rec.mode = 'skip';
    else if (d % cfg.sessionEvery === 0) {
      rec.mode = 'session';
      const seed = sessionSeed(s), rnd = mulberry32(seed);
      rec.sess = { ...runSession(s, ctx.buildSession(s, rnd), r, cfg.acc, rnd), seed };
    } else {
      rec.mode = 'sim';
      const plan = ctx.planToday(s);
      rec.plan = { r: plan.reviews, f: plan.fresh, b: plan.backlog, s: plan.seconds };
      rec.ans = plan.reviews.map(i => { const ok = r() < pOk(s, i, cfg.acc); ctx.grade(s, i, ok, true); return ok ? 1 : 0; });
      rec.fw = plan.fresh.map(i => { ctx.introduce(s, i); return r() >= cfg.acc ? 1 : 0; });
      rec.perfect = !rec.ans.includes(0) && !rec.fw.includes(1);
      ctx.finishDay(s, rec.perfect);
    }
    if (rec.mode !== 'skip' && ctx.examReady(s)) {
      const seed = sessionSeed(s), rnd = mulberry32(seed);
      rec.exam = { ...runSession(s, ctx.buildExam(s, rnd), r, cfg.acc + 0.1, rnd), seed };
    }
    rec.shop = [];
    for (let k = 0; k < 2; k++) {
      const op = Math.floor(r() * 6), arg = Math.floor(r() * 8);
      if (op < 3) continue;
      const res = op === 3 ? ctx.openToyBox(s, shopRnd) : op === 4 ? ctx.buyFreeze(s) : ctx.useAccessory(s, P.ACCESSORIES[arg].id);
      rec.shop.push([op, arg, res.ok ? 1 : 0]);
    }
    rec.vs = [];
    // 对战日打 1–5 局，同一天和同一台徽章打满 3 局后不再改积分
    for (let g = 0, games = d % 9 === 4 ? 1 + Math.floor(r() * 5) : 0; g < games; g++) {
      const kind = Math.floor(r() * 3), bits = peerBitmap(s, r, kind), seed = (r() * 4294967296) >>> 0;
      const pool = ctx.battlePool(s.box, i => (bits[i >> 3] >> (i & 7)) & 1);
      const qs = ctx.buildBattle(pool, seed, P.BATTLE_Q);
      const peer = Math.floor(r() * 2), peerRating = 900 + Math.floor(r() * 500);
      const my = Math.floor(r() * 1600), their = Math.floor(r() * 1600);
      const missed = qs.filter(() => r() < 0.3).map(q => q.idx);
      const res = ctx.battleFinish(s, 'peer' + peer, peerRating, my, their, missed);
      rec.vs.push({ bits: Buffer.from(bits).toString('hex'), seed, pool: pool.length, qs: qs.map(qrec), peer, peerRating, my, their,
        missed, result: res.result, delta: res.delta, farm: res.farm ? 1 : 0 });
    }
    const msg = ctx.advanceDay(s);
    rec.adv = msg.includes('补签') ? 1 : msg.includes('中断') ? 2 : 0;
    rec.dg = digest(s);
    days.push(rec);
  }
  return { name, cfg, days };
}

function mathTrace() {
  const elo = [], pts = [], rank = [];
  for (let me = 600; me <= 2400; me += 37) for (let op = 600; op <= 2400; op += 53)
    for (const res of [0, 500, 1000]) elo.push([me, op, res, ctx.eloExpected(me, op), ctx.eloDelta(me, op, res)]);
  for (let ms = -100; ms <= 9000; ms += 113) for (const last of [0, 1]) for (const c of [0, 1]) pts.push([c, ms, last, ctx.battlePoints(!!c, ms, !!last)]);
  for (let rt = 800; rt <= 2200; rt += 25) rank.push([rt, P.RANKS.findIndex((x, k) => rt >= x[0] && (k === P.RANKS.length - 1 || rt < P.RANKS[k + 1][0]))]);
  // 补字母和选择题：直接出题核对选项
  const qs = [];
  const r = mulberry32(77);
  for (let k = 0; k < 400; k++) {
    const idx = Math.floor(r() * P.TOTAL_WORDS), type = ['en2zh', 'zh2en', 'letter'][k % 3], seed = (r() * 4294967296) >>> 0;
    if (type === 'letter' && P.WORDS[idx].w.length < 3) continue;
    qs.push([seed, qrec(ctx.makeQuestion(type, idx, mulberry32(seed)))]);
  }
  // 积压门槛：第 1 天学 n 个词，第 2 天全部到期；预算 300 秒、每题 4 秒，排得下 75 道
  const plans = [];
  for (let n = 60; n <= 200; n++) {
    const s = ctx.newState();
    for (let i = 0; i < n; i++) ctx.introduce(s, i);
    ctx.advanceDay(s);
    const p = ctx.planToday(s);
    plans.push([n, p.reviews.length, p.fresh.length, p.backlog, p.seconds]);
  }
  // 结业考解锁门槛：小学 700 词全学过，熟悉（盒子 3）的词从 615 加到 635（90% = 630）
  const exams = [];
  for (let k = 615; k <= 635; k++) {
    const s = ctx.newState();
    for (let i = 0; i < 700; i++) { ctx.introduce(s, i); if (i < k) s.box[i] = 3; }
    exams.push([k, ctx.examReady(s) ? 1 : 0, ctx.spriteForm(s, 0)]);
  }
  return { name: 'math', elo, pts, rank, qs, plans, exams };
}

const OUT = path.join(__dirname, 'golden');
fs.mkdirSync(OUT, { recursive: true });
const CONFIGS = [
  ['steady', { seed: 101, newPer: 10, acc: 0.82, lazy: 0.05, days: 420, sessionEvery: 3 }],
  ['lazy', { seed: 202, newPer: 15, acc: 0.7, lazy: 0.3, days: 360, sessionEvery: 2 }],
  ['fast', { seed: 303, newPer: 20, acc: 0.92, lazy: 0.0, days: 520, sessionEvery: 5 }],
  ['slow', { seed: 404, newPer: 5, acc: 0.6, lazy: 0.15, days: 300, sessionEvery: 1 }],
];
for (const [name, cfg] of CONFIGS) {
  const t = trace(name, cfg);
  fs.writeFileSync(path.join(OUT, name + '.json'), JSON.stringify(t));
  const last = t.days[t.days.length - 1].dg;
  console.log(`${name}: ${t.days.length} 天，学到第 ${last.stage} 学段，掌握 ${last.mastered}，金币 ${last.coins}`);
}
fs.writeFileSync(path.join(OUT, 'math.json'), JSON.stringify(mathTrace()));
console.log('math: Elo / 计分 / 段位 / 出题');
