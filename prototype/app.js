// 画面状态机与按键。UP/DOWN 选择，OK 确认，长按 UP 1 秒返回主页。
'use strict';

const W = 240, H = 320;
const cv = document.getElementById('screen'), c2d = cv.getContext('2d');
c2d.setTransform(2, 0, 0, 2, 0, 0);

let S = loadState();
const G = { np: { name: 'idle', loop: true, t0: 0 }, pending: null, awake: false, scene: 'home', t: 0, mood: 'idle', moodT: 0, menu: [], menuIdx: 0, sess: null, sel: 0, fb: null, fbT: 0, flipped: false, summary: null, page: 0, toast: '', toastT: 0 };
const NEW_CHOICES = [5, 10, 15, 20];

function sessionRng() { return mulberry32((S.day * 2654435761 + S.next * 97 + S.box.length) >>> 0); }
let rnd = sessionRng();

function toast(msg) { G.toast = msg; G.toastT = 2.2; }
function setMood(m, sec) { G.mood = m; G.moodT = sec; }

// ---------- 主页菜单 ----------
function buildMenu() {
  const plan = planToday(S), done = S.doneDay === S.day, items = [];
  items.push(done ? { id: 'study', label: '今日学习 ✓', sub: '今天完成了，明天再来' }
    : { id: 'study', label: '今日学习', sub: plan.reviews.length + plan.fresh.length ? `复习 ${plan.reviews.length} · 新词 ${plan.fresh.length} · 约 ${Math.max(1, Math.round(plan.seconds / 60))} 分钟` : (examReady(S) ? '先通过结业考解锁新词' : '没有要学的词') });
  if (examReady(S)) items.push({ id: 'exam', label: '结业考', sub: `${STAGES[S.stage].name} · ${EXAM_Q} 题答对 ${EXAM_PASS} 题通过` });
  items.push({ id: 'battle', label: '对战', sub: `${rankOf(S.rating)} · 积分 ${S.rating}` });
  items.push({ id: 'stars', label: '星图', sub: `已掌握 ${mastered(S)} / ${TOTAL_WORDS}` });
  items.push({ id: 'passport', label: '护照', sub: `已盖 ${stampCount(S)} 枚章` });
  items.push({ id: 'desk', label: '书桌', sub: `摆放道具 · 摆件 ${S.toys.length} 个` });
  items.push({ id: 'shop', label: '小卖部', sub: `金币 ${S.coins} · 盲盒 ${toyPrice(S)} 枚一次` });
  items.push({ id: 'newper', label: '每日新词', sub: `${S.newPerDay} 个 · 复习最多 ${reviewBudget(S) / 60} 分钟 · OK 切换` });
  const keep = G.menu[G.menuIdx] && G.menu[G.menuIdx].id;
  G.menu = items;
  const k = items.findIndex(it => it.id === keep);
  G.menuIdx = k >= 0 ? k : 0;
}
// ---------- 主页上 NABO 的动画 ----------
// next：非循环动作放完后接什么；默认回到待机
function npPlay(name, seg, loop, next) { G.np = { name, seg, loop, t0: G.t, next: next || null }; }
function npIdle() { npPlay('idle', null, true); }
// 至少整整一天没学（昨天没完成）就算睡着
function isSleepy() { return S.doneDay !== S.day && S.day - S.doneDay >= 2 && S.doneDay > 0; }
function npUpdate() {
  const p = G.np;
  if (p.loop || !animDone(lookFor(S), p, G.t)) return;
  if (p.next) npPlay(...p.next); else npIdle();
}
function toHome() {
  G.scene = 'home'; G.sess = null; G.fb = null; buildMenu();
  if (G.pending) { npPlay(...G.pending); G.pending = null; }
  else if (isSleepy() && !G.awake) { if (G.np.name !== 'sleep') npPlay('sleep', 'in', false, ['sleep', 'loop', true]); }
  else if (G.np.name === 'sleep') npIdle();
}
// 睡着时按任何键先叫醒，这一下不操作菜单
function wakeIfSleeping() {
  if (G.np.name !== 'sleep' || G.np.seg === 'out') return false;
  G.awake = true; npPlay('sleep', 'out', false); SFX.done();
  return true;
}

function homeOk() {
  const it = G.menu[G.menuIdx];
  switch (it.id) {
    case 'study':
      if (S.doneDay === S.day) { toast('今天完成了，NABO 去休息了'); return; }
      rnd = sessionRng(); startSession(buildSession(S, rnd)); break;
    case 'exam': rnd = sessionRng(); startSession(buildExam(S, rnd)); break;
    case 'battle': Battle.start(); break;
    case 'stars': G.scene = 'stars'; break;
    case 'passport': G.scene = 'passport'; G.page = Math.min(S.stage, STAGES.length - 1); break;
    case 'desk': G.scene = 'desk'; G.sel = 0; break;
    case 'shop': G.scene = 'shop'; G.sel = 0; break;
    case 'newper': S.newPerDay = NEW_CHOICES[(NEW_CHOICES.indexOf(S.newPerDay) + 1) % NEW_CHOICES.length]; saveState(S); buildMenu(); break;
  }
}

// ---------- 学习 ----------
function curStampCity() {
  let last = -1;
  for (let u = 0; u < TOTAL_UNITS; u++) if (unitStamped(S, u)) last = u;
  return last >= 0 ? unitInfo(last).city : '';
}
function startSession(sess) {
  if (!sess.queue.length) { toast('今天没有要学的词'); return; }
  sess.stampsBefore = stampCount(S);
  G.sess = sess; G.scene = 'study'; prepQ();
}
function prepQ() { G.sel = 0; G.fb = null; G.flipped = false; }
function curQ() { return G.sess.queue[G.sess.pos]; }

function answer(ok, pick) {
  const sess = G.sess, q = curQ();
  G.fb = { ok, pick }; G.fbT = 0;
  if (q.first) { if (ok) sess.right++; else { sess.wrong++; sess.wrongSet.add(q.idx); } }
  if (!sess.exam && !q.isNew) grade(S, q.idx, ok, q.first);
  if (!ok && !sess.exam) requeue(sess, q, rnd);
  setMood(ok ? 'happy' : 'sad', 1);
  SFX[ok ? 'right' : 'wrong']();
  saveState(S);
}
function nextQ() {
  const sess = G.sess;
  sess.pos++;
  if (sess.pos >= sess.queue.length) finishSession(); else prepQ();
}
function finishSession() {
  const sess = G.sess, stampsBefore = sess.stampsBefore;
  const acc = sess.right + sess.wrong ? Math.round(sess.right / (sess.right + sess.wrong) * 100) : 100;
  let notes, title;
  if (sess.exam) {
    const pass = sess.right >= EXAM_PASS;
    title = pass ? '结业考通过！' : '差一点，明天再考';
    notes = pass ? passExam(S) : (failExam(S, [...sess.wrongSet]), ['错题已放回复习盒子']);
    setMood(pass ? 'happy' : 'sad', 3);
    G.pending = pass ? ['cheer', null, false] : ['sad', null, false];
  } else {
    title = '今日学习完成';
    notes = finishDay(S, sess.wrong === 0);
    setMood('happy', 2);
    const gained = stampCount(S) - stampsBefore;
    if (gained > 0) notes.unshift(`护照盖章：${curStampCity()}`);
    G.pending = gained > 0 ? ['cheer', null, false] : ['happy', null, false];
  }
  saveState(S);
  G.summary = { title, notes, rows: sess.exam
    ? [['答对', `${sess.right} / ${sess.total}`], ['及格线', String(EXAM_PASS)]]
    : [['新词', String(sess.plan.fresh)], ['复习', String(sess.plan.reviews)], ['正确率', acc + '%'], ['已掌握', mastered(S) + ' 词']] };
  G.scene = 'summary'; SFX.done();
}

function studyKey(k) {
  const q = curQ();
  if (q.type === 'intro') { if (k === 'ok') { introduce(S, q.idx); saveState(S); nextQ(); } return; }
  if (G.fb) { if (k === 'ok') nextQ(); return; }
  if (q.type === 'flash') {
    if (!G.flipped) { if (k === 'ok') G.flipped = true; return; }
    if (k === 'up') answer(true); else if (k === 'down') answer(false);
    return;
  }
  if (k === 'up') G.sel = (G.sel + q.options.length - 1) % q.options.length;
  else if (k === 'down') G.sel = (G.sel + 1) % q.options.length;
  else answer(G.sel === q.answer, G.sel);
}

// ---------- 按键 ----------
function act(k) {
  SFX.tick();
  if (G.scene === 'battle') return Battle.key(k);
  switch (G.scene) {
    case 'home':
      if (wakeIfSleeping()) break;
      if (k === 'up') G.menuIdx = (G.menuIdx + G.menu.length - 1) % G.menu.length;
      else if (k === 'down') G.menuIdx = (G.menuIdx + 1) % G.menu.length;
      else homeOk();
      break;
    case 'study': studyKey(k); break;
    case 'summary': if (k === 'ok') toHome(); break;
    case 'stars': if (k === 'ok') toHome(); break;
    case 'passport':
      if (k === 'ok') toHome();
      else G.page = (G.page + (k === 'up' ? STAGES.length - 1 : 1)) % STAGES.length;
      break;
    case 'desk': deskKey(k); break;
    case 'shop': shopKey(k); break;
  }
}
function deskKey(k) {
  const items = deskItems(S);
  if (k === 'up') { G.sel = (G.sel + items.length - 1) % items.length; return; }
  if (k === 'down') { G.sel = (G.sel + 1) % items.length; return; }
  const it = items[G.sel];
  if (it.kind === 'back') return toHome();
  if (it.locked) toast(`再掌握 ${it.need - mastered(S)} 个词解锁`);
  else S.hidden = it.on ? [...S.hidden, it.id] : S.hidden.filter(x => x !== it.id);
  saveState(S);
}
function shopKey(k) {
  const items = shopItems(S);
  if (k === 'up') { G.sel = (G.sel + items.length - 1) % items.length; return; }
  if (k === 'down') { G.sel = (G.sel + 1) % items.length; return; }
  const it = items[G.sel];
  if (it.kind === 'back') return toHome();
  const r = it.kind === 'box' ? openToyBox(S, rnd) : it.kind === 'freeze' ? buyFreeze(S) : useAccessory(S, it.id);
  toast(r.msg);
  if (r.ok) { SFX.right(); saveState(S); } else SFX.wrong();
}
function back() {
  if (G.scene === 'battle') Battle.abort();
  if (G.scene !== 'home') { SFX.tick(); toHome(); }
}

const held = { up: null, down: null, ok: null };
let upLong = false;
function keyDown(k) {
  if (held[k] !== null) return;
  held[k] = G.t; KEYBTN[k].classList.add('down');
  if (k === 'up') upLong = false; else act(k);
}
function keyUp(k) {
  if (held[k] === null) return;
  held[k] = null; KEYBTN[k].classList.remove('down');
  if (k === 'up' && !upLong) act('up');
}
const KEYMAP = { ArrowUp: 'up', KeyW: 'up', ArrowDown: 'down', KeyS: 'down', Enter: 'ok', Space: 'ok', KeyK: 'ok' };
const KEYBTN = { up: document.getElementById('kUp'), down: document.getElementById('kDown'), ok: document.getElementById('kOk') };
window.addEventListener('keydown', e => {
  const k = KEYMAP[e.code];
  if (!k || e.target.closest('input, select, textarea, button.text')) return;
  e.preventDefault(); if (!e.repeat) keyDown(k);
});
window.addEventListener('keyup', e => { const k = KEYMAP[e.code]; if (k) { e.preventDefault(); keyUp(k); } });
window.addEventListener('blur', () => Object.keys(held).forEach(keyUp));
for (const [k, el] of Object.entries(KEYBTN)) {
  el.addEventListener('pointerdown', e => { e.preventDefault(); el.setPointerCapture(e.pointerId); keyDown(k); });
  el.addEventListener('pointerup', () => keyUp(k));
  el.addEventListener('pointercancel', () => keyUp(k));
}

// ---------- 主循环 ----------
function update(dt) {
  G.t += dt;
  if (G.moodT > 0 && (G.moodT -= dt) <= 0) G.mood = 'idle';
  if (G.scene === 'home') npUpdate();
  if (G.toastT > 0 && (G.toastT -= dt) <= 0) G.toast = '';
  if (held.up !== null && !upLong && G.t - held.up >= 1) { upLong = true; back(); }
  if (G.scene === 'study' && G.fb && G.fb.ok && (G.fbT += dt) > 0.7) nextQ();
  Battle.update(dt);
}
function draw() {
  bindCtx(c2d);
  switch (G.scene) {
    case 'home': drawHome(S, G); break;
    case 'study': drawStudy(S, G); break;
    case 'summary': drawSummary(S, G); break;
    case 'stars': drawStars(S, G); break;
    case 'passport': drawPassport(S, G); break;
    case 'desk': drawDeskMenu(S, G); break;
    case 'shop': drawShop(S, G); break;
    case 'battle': Battle.drawA(); break;
  }
  if (G.toast && G.scene !== 'desk' && G.scene !== 'shop') { rrect(20, 34, 200, 26, 13, 'rgba(18,24,41,.92)'); fitText(G.toast, 120, 47, 12, 186, C.gold, 'center', F_BODY, 700); }
  if (held.up !== null && !upLong && G.t - held.up > 0.25 && G.scene !== 'home') {
    const f = Math.min(1, (G.t - held.up) / 1);
    c2d.lineWidth = 3; c2d.strokeStyle = C.gold; c2d.beginPath(); c2d.arc(W - 16, 16, 8, -Math.PI / 2, -Math.PI / 2 + f * Math.PI * 2); c2d.stroke();
  }
  Battle.drawB();
}
let last = performance.now(), panelT = 0;
function frame(now) {
  const dt = Math.min(0.05, (now - last) / 1000); last = now;
  update(dt); draw();
  if ((panelT += dt) > 0.25) { panelT = 0; Panel.render(); }
  requestAnimationFrame(frame);
}

// 调试钩子：NW.press('ok')、NW.hold('up', 1100)、NW.days(30)、NW.state()（WORDS 是词表，钩子另起名）
window.NW = {
  press(k) { keyDown(k); keyUp(k); },
  hold(k, ms) { keyDown(k); NW.advance(ms / 1000); keyUp(k); },
  days(n) { Panel.simulate(n); },
  advance(sec) { for (let i = 0; i < Math.round(sec * 60); i++) update(1 / 60); draw(); },
  state: () => ({ scene: G.scene, day: S.day, stage: S.stage, next: S.next, mastered: mastered(S), learned: learned(S), streak: S.streak, coins: S.coins, rating: S.rating, q: G.sess && curQ(), battle: Battle.state() }),
  get S() { return S; }, get G() { return G; },
};
