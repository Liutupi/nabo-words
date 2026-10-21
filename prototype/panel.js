// 右侧面板：时间快进、进度、记忆盒分布、成长曲线、对战机器人、ESP-NOW 报文记录。声音也放这里。
'use strict';

const SFX = (() => {
  let ac = null, on = true;
  function tone(f, d, type = 'square', vol = 0.05, delay = 0) {
    if (!on) return;
    try {
      ac = ac || new AudioContext();
      const o = ac.createOscillator(), g = ac.createGain(), t0 = ac.currentTime + delay;
      o.type = type; o.frequency.value = f; g.gain.setValueAtTime(vol, t0); g.gain.exponentialRampToValueAtTime(0.001, t0 + d);
      o.connect(g).connect(ac.destination); o.start(t0); o.stop(t0 + d);
    } catch { /* 浏览器不允许时静音 */ }
  }
  return {
    set on(v) { on = v; }, get on() { return on; },
    tick: () => tone(1200, 0.03, 'square', 0.02),
    right: () => { tone(880, 0.08); tone(1320, 0.12, 'square', 0.05, 0.08); },
    wrong: () => tone(220, 0.2, 'sawtooth', 0.04),
    done: () => [523, 659, 784, 1046].forEach((f, i) => tone(f, 0.14, 'triangle', 0.06, i * 0.1)),
  };
})();

const Panel = (() => {
  const $ = id => document.getElementById(id);
  const simRnd = mulberry32(20261021);
  const pkts = [];
  let cache = '';

  function sliders() {
    const bind = (id, out, fmt, on) => { const el = $(id); const f = () => { $(out).textContent = fmt(el.value); on && on(Number(el.value)); }; el.addEventListener('input', f); f(); };
    bind('sAcc', 'oAcc', v => v + '%');
    bind('sLazy', 'oLazy', v => v + '%');
    bind('bAcc', 'obAcc', v => v + '%', v => { Battle.bot.acc = v / 100; });
    bind('bReact', 'obReact', v => (v / 10).toFixed(1) + ' 秒', v => { Battle.bot.react = v / 10; });
    bind('bVocab', 'obVocab', v => v + ' 词', v => { Battle.bot.vocab = v; Battle.bot.stage = STAGES.findIndex(st => v < st.end); });
    $('sound').addEventListener('change', e => { SFX.on = e.target.checked; });
  }

  function simulate(n) {
    const acc = $('sAcc').value / 100, lazy = $('sLazy').value / 100;
    for (let i = 0; i < n; i++) simulateDay(S, simRnd, acc, lazy);
    G.awake = false;
    saveState(S);
    if (G.scene !== 'battle') toHome(); else buildMenu();
    render(true);
  }
  function nextDay() {
    const msg = advanceDay(S);
    G.awake = false;
    saveState(S); if (G.scene !== 'battle') toHome();
    toast(msg || `第 ${S.day} 个学习日`);
    render(true);
  }
  function reset() {
    S = newState(); saveState(S);
    if (G.scene === 'battle') Battle.abort();
    toHome(); render(true);
  }

  function packet(dir, name, bytes, info) {
    pkts.unshift(`<li><span class="dir">${dir}</span> <b>${name}</b> <span class="bytes">${bytes} B</span> ${info}</li>`);
    if (pkts.length > 40) pkts.pop();
    $('pkts').innerHTML = pkts.join('');
  }

  function drawHist() {
    const cv = $('hist'), g = cv.getContext('2d'), h = boxHistogram(S), w = cv.width, ht = cv.height;
    g.clearRect(0, 0, w, ht);
    const max = Math.max(1, ...h.slice(1)), bw = (w - 20) / 7;
    const names = ['1', '2', '3', '4', '5', '6', '毕业'];
    for (let b = 1; b <= 7; b++) {
      const v = h[b], bh = (ht - 40) * v / max, x = 10 + (b - 1) * bw;
      g.fillStyle = STAR_COL[b]; g.fillRect(x + 4, ht - 22 - bh, bw - 8, Math.max(1, bh));
      g.fillStyle = '#9AA3BF'; g.font = '11px ' + F_MONO; g.textAlign = 'center';
      g.fillText(String(v), x + bw / 2, ht - 26 - bh); g.fillText(names[b - 1], x + bw / 2, ht - 6);
    }
  }
  function drawCurve() {
    const cv = $('curve'), g = cv.getContext('2d'), w = cv.width, ht = cv.height, hs = S.history;
    g.clearRect(0, 0, w, ht);
    const pad = { l: 36, r: 8, t: 10, b: 20 }, days = Math.max(30, S.day), x = d => pad.l + (w - pad.l - pad.r) * (d - 1) / (days - 1);
    const y = v => ht - pad.b - (ht - pad.t - pad.b) * v / TOTAL_WORDS;
    g.font = '10px ' + F_MONO; g.fillStyle = '#6B7490'; g.textAlign = 'right';
    for (const st of STAGES) {
      g.strokeStyle = 'rgba(255,255,255,.07)'; g.beginPath(); g.moveTo(pad.l, y(st.end)); g.lineTo(w - pad.r, y(st.end)); g.stroke();
      g.fillText(st.name, pad.l - 4, y(st.end) + 3);
    }
    g.textAlign = 'center'; g.fillText(`第 ${days} 天`, w - 30, ht - 4); g.fillText('第 1 天', pad.l + 16, ht - 4);
    if (!hs.length) return;
    g.beginPath(); g.moveTo(x(hs[0].day), y(hs[0].mastered));
    for (const p of hs) g.lineTo(x(p.day), y(p.mastered));
    g.lineTo(x(hs[hs.length - 1].day), y(0)); g.lineTo(x(hs[0].day), y(0)); g.closePath();
    g.fillStyle = 'rgba(232,184,74,.15)'; g.fill();
    g.beginPath(); hs.forEach((p, i) => (i ? g.lineTo : g.moveTo).call(g, x(p.day), y(p.mastered)));
    g.strokeStyle = '#E8B84A'; g.lineWidth = 2; g.stroke();
    const lp = hs[hs.length - 1]; g.fillStyle = '#E8B84A'; g.beginPath(); g.arc(x(lp.day), y(lp.mastered), 3, 0, 7); g.fill();
  }

  function render(force) {
    const st = STAGES[Math.min(S.stage, STAGES.length - 1)], plan = planToday(S);
    const rows = [
      ['学习日', `第 ${S.day} 天${S.doneDay === S.day ? ' · 今天已完成' : ''}`],
      ['学段', allDone(S) ? '全部完成' : `${st.name}（${st.region}）${examReady(S) ? ' · 可以结业考' : ''}`],
      ['词汇', `已掌握 ${mastered(S)} · 学过 ${learned(S)} / ${TOTAL_WORDS}`],
      ['今日', `复习 ${plan.reviews.length} · 新词 ${plan.fresh.length} · 约 ${(plan.seconds / 60).toFixed(1)} 分钟${plan.backlog ? ` · 积压 ${plan.backlog}` : ''}`],
      ['打卡', `连续 ${S.streak} 天 · 最长 ${S.bestStreak} · 补签卡 ${S.freezes}`],
      ['金币', `${S.coins} · 已花 ${S.spent} · 摆件 ${S.toys.length}/${BOX_TOYS.length} · 饰品 ${S.accs.length}/${ACCESSORIES.length}`],
      ['护照', `${stampCount(S)} / ${TOTAL_UNITS} 枚`],
      ['对战', `${rankOf(S.rating)} ${S.rating} · ${S.wins} 胜 ${S.losses} 负`],
      ['精灵', STAGES.map((s2, i) => `${s2.sprite}${['蛋', '幼崽', '少年', '完全体'][spriteForm(S, i)]}`).slice(0, Math.min(S.stage, 5) + 1).join(' · ')],
    ];
    const html = rows.map(([a, b]) => `<dt>${a}</dt><dd>${b}</dd>`).join('');
    if (html !== cache || force) { cache = html; $('live').innerHTML = html; drawHist(); drawCurve(); }
  }

  function init() {
    sliders();
    $('day1').addEventListener('click', nextDay);
    for (const [id, n] of [['day7', 7], ['day30', 30], ['day365', 365]]) $(id).addEventListener('click', () => simulate(n));
    $('reset').addEventListener('click', () => { const b = $('reset'); if (b.dataset.armed) { delete b.dataset.armed; b.textContent = '清空存档'; reset(); } else { b.dataset.armed = '1'; b.textContent = '再点一次确认清空'; setTimeout(() => { delete b.dataset.armed; b.textContent = '清空存档'; }, 3000); } });
  }
  return { init, render, simulate, packet };
})();
