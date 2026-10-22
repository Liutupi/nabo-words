// 用原型 prototype/extra.js 生成分级测试 / 加学 / 闪电挑战的轨迹（文本），extra_parity.py 和 C 版本逐行比较
'use strict';
const fs = require('fs'), path = require('path'), vm = require('vm');
const ROOT = path.resolve(__dirname, '..', '..');
const ctx = { localStorage: { getItem: () => null, setItem() {} }, console };
ctx.window = ctx;
vm.createContext(ctx);
for (const f of ['assets/words_data.js', 'words.js', 'core.js', 'quiz.js', 'extra.js'])
  vm.runInContext(fs.readFileSync(path.join(ROOT, 'prototype', f), 'utf8'), ctx, { filename: f });
const C = ctx, TYPE = { intro: 0, en2zh: 1 }, out = [];
const opts = q => (q.options || []).join('|');
for (let seed = 1; seed <= 12; seed++) {
  let rnd = C.mulberry32(seed), p = C.placeStart(rnd);
  const knows = seed % 7;
  for (;;) {
    const q = p.q[p.pos];
    out.push(`P ${seed} ${p.probe} ${p.pos} ${q.idx} ${q.answer} ${opts(q)}`);
    if (C.placeAnswer(p, p.pos >= (p.probe < knows ? 2 : 3), rnd)) { out.push(`R ${seed} ${p.lo}`); break; }
  }
  const s = C.newState();
  s.next = (seed * 37) % 690; s.doneDay = s.day;
  rnd = C.mulberry32(seed + 100);
  for (const q of C.buildExtra(s, rnd).queue)
    out.push(`E ${seed} ${TYPE[q.type]} ${q.idx} ${q.isNew ? 1 : 0} ${q.answer ?? 0} ${opts(q)}`);
  const t = C.newState();
  for (let i = 0; i < 600; i += 3) t.box[i] = 1 + (i % 5);
  rnd = C.mulberry32(seed + 200);
  for (let k = 0; k < 10; k++) { const q = C.challengeQuestion(t, rnd); out.push(`C ${seed} ${q.idx} ${q.answer} ${opts(q)}`); }
}
process.stdout.write(out.join('\n') + '\n');
