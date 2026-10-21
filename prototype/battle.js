// 对战：模拟两台徽章经 ESP-NOW 直连。右边的第二台由机器人操作，两台用同一个种子在本地出同样的题，只交换答案。
'use strict';

const Battle = (() => {
  const Q_N = 10, Q_SEC = 8, ELO_K = 32, FARM_LIMIT = 3;
  const BOT_ID = 'b4:3a:45:12:9c:e1';
  const cvB = document.getElementById('screenB'), ctxB = cvB.getContext('2d');
  ctxB.setTransform(2, 0, 0, 2, 0, 0);
  const bot = { name: '小林', rating: 1180, vocab: 120, acc: 0.75, react: 3.2, stage: 0 };
  let M = null;   // 当前对局

  function pkt(dir, name, bytes, info) { Panel.packet(dir, name, bytes, info || ''); }

  function start() {
    M = { phase: 'search', t: 0, qi: 0, score: [0, 0], picks: [], sel: 0, log: [] };
    G.scene = 'battle';
    pkt('A→*', 'HELLO', 32, `${rankOf(S.rating)} · 词汇 ${mastered(S)}`);
  }
  function abort() { if (M && M.phase !== 'end') pkt('A→B', 'BYE', 8, '中途退出，本局不计'); M = null; }

  function beginMatch() {
    M.seed = (Date.now() ^ (S.day * 2654435761)) >>> 0;
    const pool = battlePool(S.box, i => i < bot.vocab);
    M.qs = buildBattle(pool, M.seed, Q_N);
    M.poolSize = pool.length;
    pkt('A→B', 'START', 16, `种子 ${M.seed} · 题库 ${pool.length} 词`);
    M.phase = 'count'; M.t = 0;
  }
  function beginQ() {
    const rnd = mulberry32(M.seed + M.qi * 7919);
    const q = M.qs[M.qi];
    const ok = rnd() < bot.acc;
    const wrongs = [0, 1, 2].filter(k => k !== q.answer);
    M.botPick = ok ? q.answer : wrongs[Math.floor(rnd() * 2)];
    M.botAt = Math.min(Q_SEC - 0.3, Math.max(0.8, bot.react * (0.6 + rnd() * 0.8)));
    M.pick = [null, null]; M.at = [Q_SEC, Q_SEC]; M.sel = 0; M.phase = 'q'; M.t = 0;
  }
  function points(k) {
    const q = M.qs[M.qi];
    if (M.pick[k] !== q.answer) return 0;
    const p = 100 + Math.floor(50 * Math.max(0, Q_SEC - M.at[k]) / Q_SEC);
    return M.qi === Q_N - 1 ? p * 2 : p;
  }
  function reveal() {
    M.gain = [points(0), points(1)];
    M.score[0] += M.gain[0]; M.score[1] += M.gain[1];
    M.picks.push({ idx: M.qs[M.qi].idx, a: M.pick[0] === M.qs[M.qi].answer });
    M.phase = 'reveal'; M.t = 0;
    SFX[M.gain[0] ? 'right' : 'wrong']();
  }
  function finish() {
    const [a, b] = M.score, res = a > b ? 1 : a < b ? 0 : 0.5;
    const exp = 1 / (1 + Math.pow(10, (bot.rating - S.rating) / 400));
    const n = (S.vsToday[BOT_ID] || 0) + 1;
    S.vsToday = { ...S.vsToday, [BOT_ID]: n };
    const delta = n > FARM_LIMIT ? 0 : Math.round(ELO_K * (res - exp));
    S.rating += delta; bot.rating -= delta;
    const coins = res === 1 ? 3 : res === 0.5 ? 2 : 1;
    S.coins += coins;
    if (res === 1) { S.wins++; S.winStreak++; } else { S.winStreak = 0; if (res === 0) S.losses++; }
    const missed = M.picks.filter(p => !p.a).map(p => p.idx);
    for (const i of missed) if (S.box[i] > 0) S.due[i] = Math.min(S.due[i], S.day + 1);
    const notes = checkAchievements(S);
    saveState(S);
    pkt('A→B', 'RESULT', 12, `${a} : ${b}`); pkt('B→A', 'RESULT', 12, '比分核对一致');
    M.result = { res, delta, coins, missed: missed.length, farm: n > FARM_LIMIT, notes };
    M.phase = 'end'; M.t = 0;
    SFX.done(); setMood(res === 1 ? 'happy' : res === 0 ? 'sad' : 'idle', 2);
  }

  function key(k) {
    if (!M) return;
    if (M.phase === 'list' && k === 'ok') { pkt('A→B', 'INVITE', 12, BOT_ID); M.phase = 'invite'; M.t = 0; }
    else if (M.phase === 'q' && M.pick[0] === null) {
      if (k === 'up') M.sel = (M.sel + 2) % 3;
      else if (k === 'down') M.sel = (M.sel + 1) % 3;
      else { M.pick[0] = M.sel; M.at[0] = M.t; pkt('A→B', 'ANSWER', 8, `第 ${M.qi + 1} 题 · ${Math.round(M.t * 1000)} ms`); }
    } else if (M.phase === 'end' && k === 'ok') { M = null; toHome(); }
  }

  function update(dt) {
    if (!M) return;
    M.t += dt;
    switch (M.phase) {
      case 'search':
        if (M.t > 1.6) { pkt('B→*', 'HELLO', 32, `${bot.name} · ${rankOf(bot.rating)} · 词汇 ${bot.vocab}`); M.phase = 'list'; M.t = 0; }
        break;
      case 'invite':
        if (M.t > 0.9 && !M.accepted) {
          M.accepted = true; pkt('B→A', 'ACCEPT', 12, '');
          for (let k = 0; k < 4; k++) { pkt('A→B', 'BITMAP', 236, `分片 ${k + 1}/4`); pkt('B→A', 'BITMAP', 236, `分片 ${k + 1}/4`); }
        }
        if (M.t > 1.6) beginMatch();
        break;
      case 'count': if (M.t > 3) beginQ(); break;
      case 'q':
        if (M.pick[1] === null && M.t >= M.botAt) { M.pick[1] = M.botPick; M.at[1] = M.botAt; pkt('B→A', 'ANSWER', 8, `第 ${M.qi + 1} 题 · ${Math.round(M.botAt * 1000)} ms`); }
        if ((M.pick[0] !== null && M.pick[1] !== null) || M.t >= Q_SEC) reveal();
        break;
      case 'reveal':
        if (M.t > 1.8) { M.qi++; if (M.qi >= Q_N) finish(); else beginQ(); }
        break;
    }
  }

  // ---------- 画面（view 0 = 本机，1 = 对方） ----------
  const NAMES = () => ['你', bot.name];
  function drawView(v) {
    const me = v, op = 1 - v;
    rect(0, 0, 240, 320, '#141A30');
    if (!M) {
      drawRoom(LOOK[bot.stage], G.t); drawNabo(LOOK[bot.stage], 'idle', G.t); drawDesk(LOOK[bot.stage]);
      rect(0, 214, 240, 106, C.ink);
      text(`${bot.name}的徽章`, 120, 246, 15, '#FFFFFF', 'center', F_BODY, 700);
      text(`${rankOf(bot.rating)} · 积分 ${bot.rating} · 词汇 ${bot.vocab}`, 120, 270, 11, C.muted, 'center');
      text('在主页选“对战”就能搜到它', 120, 296, 11, '#5B6584', 'center');
      return;
    }
    if (M.phase === 'search' || M.phase === 'list' || M.phase === 'invite') {
      for (let k = 0; k < 3; k++) { const r = ((G.t * 40 + k * 40) % 120); ctx.strokeStyle = `rgba(63,184,169,${1 - r / 120})`; ctx.lineWidth = 2; ctx.beginPath(); ctx.arc(120, 120, r, 0, Math.PI * 2); ctx.stroke(); }
      drawSprite(Math.min(S.stage, 5), 1, 120, 136, G.t, 1.4);
      if (M.phase === 'search') text('正在寻找附近的 NABO…', 120, 230, 13, '#FFFFFF', 'center');
      else {
        const other = v === 0 ? { n: bot.name, r: bot.rating, w: bot.vocab } : { n: '你', r: S.rating, w: mastered(S) };
        rrect(16, 204, 208, 54, 10, '#222B48');
        text(`${other.n}的 NABO`, 30, 222, 14, '#FFFFFF', 'left', F_BODY, 700);
        text(`${rankOf(other.r)} · 词汇 ${other.w}`, 30, 242, 11, C.muted);
        text(M.phase === 'invite' ? (M.accepted ? '交换词库…' : (v === 0 ? '等待接受…' : '收到挑战')) : (v === 0 ? 'OK 挑战' : ''), 214, 232, 12, C.gold, 'right', F_BODY, 700);
      }
      text(v === 0 ? '长按 UP 退出' : 'ESP-NOW 直连 · 无需网络', 120, 300, 10, '#5B6584', 'center');
      return;
    }
    if (M.phase === 'count') {
      text(String(3 - Math.floor(M.t)), 120, 140, 72, C.gold, 'center', F_WORD, 700);
      text(`题库：双方都学过的 ${M.poolSize} 个词`, 120, 220, 12, C.muted, 'center');
      return;
    }
    if (M.phase === 'end') {
      const r = M.result, res = v === 0 ? r.res : 1 - r.res;
      text(res === 1 ? '胜利！' : res === 0 ? '惜败' : '平局', 120, 60, 30, res === 1 ? C.gold : res === 0 ? C.muted : '#FFFFFF', 'center', F_BODY, 700);
      text(`${M.score[me]} : ${M.score[op]}`, 120, 104, 22, '#FFFFFF', 'center', F_MONO, 700);
      if (v === 0) {
        const rows = [['积分', `${S.rating}（${r.delta >= 0 ? '+' : ''}${r.delta}）`], ['段位', rankOf(S.rating)], ['金币', `+${r.coins}`], ['错词', `${r.missed} 个加入复习`]];
        rows.forEach(([a, b], k) => { text(a, 34, 146 + k * 24, 13, C.muted); text(b, 206, 146 + k * 24, 13, '#FFFFFF', 'right', F_BODY, 700); });
        if (r.farm) fitText('今天和同一位对手已打满 3 局，积分不再变化', 120, 254, 11, 210, C.muted, 'center');
        (r.notes || []).slice(0, 1).forEach(n => text(n, 120, 274, 12, C.gold, 'center', F_BODY, 700));
        text('OK 回主页', 120, 302, 11, C.muted, 'center');
      }
      return;
    }
    // 答题 / 揭晓
    const q = M.qs[M.qi], w = WORDS[q.idx], reveal = M.phase === 'reveal';
    text(`${NAMES()[me]} ${M.score[me]}`, 12, 14, 12, C.teal, 'left', F_BODY, 700);
    text(`${M.score[op]} ${NAMES()[op]}`, 228, 14, 12, '#F08A3C', 'right', F_BODY, 700);
    text(`${M.qi + 1}/${Q_N}${M.qi === Q_N - 1 ? ' ×2' : ''}`, 120, 14, 11, C.muted, 'center', F_MONO);
    const left = reveal ? 0 : Math.max(0, 1 - M.t / Q_SEC);
    rrect(12, 26, 216, 4, 2, '#2A3350'); rrect(12, 26, Math.max(4, 216 * left), 4, 2, left < 0.3 ? C.red : C.teal);
    fitText(w.w, 120, 78, 34, 210, '#FFFFFF', 'center', F_WORD, 600);
    if (w.p) text(`/${w.p}/`, 120, 110, 12, C.muted, 'center', F_IPA);
    const myPick = M.pick[me], sel = v === 0 ? M.sel : M.pick[1] ?? -1;
    q.options.forEach((o, k) => {
      const y = 140 + k * 44;
      let bg = '#222B48', bd = '#222B48', fg = '#FFFFFF';
      if (reveal) { if (k === q.answer) { bg = C.green; bd = bg; } else if (k === myPick) { bg = C.red; bd = bg; } }
      else if (myPick === k) { bd = C.gold; bg = '#2E3860'; }
      else if (myPick === null && k === sel && v === 0) bd = C.teal;
      rrect(16, y, 208, 36, 8, bd); rrect(18, y + 2, 204, 32, 7, bg);
      fitText(o, 120, y + 18, 16, 170, fg, 'center', F_BODY, 600);
      if (reveal && M.pick[op] === k) { rrect(184, y + 8, 34, 20, 10, '#F08A3C'); text(NAMES()[op].slice(0, 2), 201, y + 18, 10, '#fff', 'center', F_BODY, 700); }
    });
    if (reveal) text(M.gain[me] ? `+${M.gain[me]}` : '没得分', 120, 290, 16, M.gain[me] ? C.gold : C.muted, 'center', F_MONO, 700);
    else text(myPick !== null ? (M.pick[op] !== null ? '' : '已作答，等对方…') : (M.pick[op] !== null ? '对方已作答！' : ''), 120, 290, 12, M.pick[op] !== null && myPick === null ? '#F08A3C' : C.muted, 'center', F_BODY, 700);
  }

  return {
    bot, start, abort, key, update,
    drawA() { drawView(0); },
    drawB() { bindCtx(ctxB); drawView(1); bindCtx(c2d); },
    state: () => M && { phase: M.phase, qi: M.qi, score: M.score.slice(), pick: M.pick && M.pick.slice(), result: M.result },
  };
})();
