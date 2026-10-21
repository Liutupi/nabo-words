// 各画面的绘制：主页、学习 / 结业考、学习小结、星图、护照、书桌。只读状态，不改状态。
'use strict';

function lookFor(s) { return LOOK[Math.min(s.stage, 6)]; }
function curUnitCity(s) { return unitInfo(Math.min(unitOf(Math.min(s.next, TOTAL_WORDS - 1)), TOTAL_UNITS - 1)).city; }

function iconFlame(x, y, col) { ctx.fillStyle = col; ctx.beginPath(); ctx.moveTo(x, y - 7); ctx.quadraticCurveTo(x + 7, y, x + 4, y + 5); ctx.quadraticCurveTo(x, y + 8, x - 4, y + 5); ctx.quadraticCurveTo(x - 6, y, x, y - 7); ctx.fill(); }
function iconCoin(x, y) { circle(x, y, 6, C.gold); circle(x, y, 3.5, '#C9952E'); }

function drawTopBar(s) {
  rect(0, 0, 240, 24, 'rgba(18,24,41,.82)');
  const st = STAGES[Math.min(s.stage, STAGES.length - 1)];
  text(allDone(s) ? '环游完成' : `${st.name} · ${curUnitCity(s)}`, 8, 12, 12, '#FFFFFF', 'left', F_BODY, 700);
  iconFlame(160, 12, s.doneDay === s.day ? '#FF8A3D' : '#6B7287');
  text(String(s.streak), 168, 12, 11, '#FFFFFF', 'left', F_MONO, 700);
  iconCoin(198, 12); text(String(s.coins), 207, 12, 11, '#FFFFFF', 'left', F_MONO, 700);
}

function drawHome(s, G) {
  const look = lookFor(s), t = G.t;
  drawRoom(look, t); drawProps(s, t, 'wall');
  // 美术版 NABO 的双手搭在桌沿上，所以画在桌子之后；占位版身体要被桌子挡住
  if (naboArtFor(look)) { drawDesk(look); drawNabo(look, G.mood, t); }
  else { drawNabo(look, G.mood, t); drawDesk(look); }
  drawProps(s, t, 'desk');
  const si = Math.min(s.stage, 5);
  drawSprite(si, spriteForm(s, si), 196, 176, t);
  drawTopBar(s);
  // 底部：词汇量 + 学段进度 + 菜单
  rect(0, 214, 240, 106, C.ink);
  const m = mastered(s), st = STAGES[si], n = st.end - st.start;
  text('已掌握', 12, 230, 11, C.muted);
  text(String(m), 52, 230, 18, '#FFFFFF', 'left', F_MONO, 700);
  text('词', 54 + ctx.measureText(String(m)).width + 2, 231, 11, C.muted);
  const p = stageProgress(s, st).mastered;
  text(`${st.name} ${Math.round(p * 100)}%`, 228, 230, 11, C.gold, 'right');
  rrect(12, 242, 216, 5, 3, '#2A3350'); rrect(12, 242, Math.max(5, 216 * p), 5, 3, C.gold);
  drawMenu(G);
}

function drawMenu(G) {
  const items = G.menu, i = G.menuIdx, it = items[i];
  rrect(12, 256, 216, 56, 10, '#1E2740');
  text(it.label, 120, 276, 17, '#FFFFFF', 'center', F_BODY, 700);
  fitText(it.sub || '', 120, 297, 11, 196, C.muted, 'center');
  ctx.fillStyle = '#5B6584';
  ctx.beginPath(); ctx.moveTo(220, 270); ctx.lineTo(214, 276); ctx.lineTo(226, 276); ctx.fill();
  ctx.beginPath(); ctx.moveTo(220, 296); ctx.lineTo(214, 290); ctx.lineTo(226, 290); ctx.fill();
  items.forEach((_, k) => circle(22, 270 + k * 28 / Math.max(1, items.length - 1), k === i ? 2.5 : 1.5, k === i ? C.gold : '#5B6584'));
}

// ---------- 学习 / 结业考 ----------
function drawStudy(s, G) {
  const sess = G.sess, q = sess.queue[sess.pos], w = WORDS[q.idx];
  rect(0, 0, 240, 320, C.paper);
  // 进度
  const done = sess.pos / Math.max(1, sess.queue.length);
  rrect(12, 10, 216, 5, 3, '#EADFC9'); rrect(12, 10, Math.max(5, 216 * done), 5, 3, sess.exam ? C.red : C.teal);
  const tag = sess.exam ? `结业考 ${sess.pos + 1}/${sess.total}` : q.retry ? '再记一次' : q.type === 'intro' ? '新词' : q.isNew ? '新词检测' : '复习';
  text(tag, 12, 28, 11, sess.exam ? C.red : '#7A6F5A', 'left', F_BODY, 700);
  text(unitInfo(unitOf(q.idx)).city, 228, 28, 11, '#B0A68F', 'right');
  const fb = G.fb;
  if (q.type === 'intro') {
    fitText(w.w, 120, 96, 36, 210, C.ink, 'center', F_WORD, 600);
    if (w.p) text(`/${w.p}/`, 120, 130, 14, '#7A6F5A', 'center', F_IPA);
    fitText(w.m, 120, 172, 22, 210, C.ink, 'center', F_BODY, 700);
    hint('OK 记住了');
  } else if (q.type === 'flash') {
    fitText(w.w, 120, 100, 36, 210, C.ink, 'center', F_WORD, 600);
    if (w.p) text(`/${w.p}/`, 120, 134, 14, '#7A6F5A', 'center', F_IPA);
    if (G.flipped) { fitText(w.m, 120, 180, 22, 210, C.ink, 'center', F_BODY, 700); hint(fb ? 'OK 继续' : 'UP 认识 · DOWN 不认识'); }
    else hint('OK 翻面');
    if (fb) text(fb.ok ? '✓' : '再看一眼', 120, 222, 16, fb.ok ? C.green : C.red, 'center', F_BODY, 700);
  } else {
    if (q.type === 'en2zh') { fitText(w.w, 120, 76, 34, 210, C.ink, 'center', F_WORD, 600); if (w.p) text(`/${w.p}/`, 120, 108, 13, '#7A6F5A', 'center', F_IPA); }
    else if (q.type === 'zh2en') fitText(w.m, 120, 86, 24, 210, C.ink, 'center', F_BODY, 700);
    else {
      const word = w.w.toLowerCase(), shown = word.slice(0, q.pos) + '_' + word.slice(q.pos + 1);
      fitText(fb ? word : shown, 120, 72, 32, 210, C.ink, 'center', F_WORD, 600);
      fitText(w.m, 120, 106, 16, 210, '#7A6F5A', 'center');
    }
    q.options.forEach((o, k) => {
      const y = 140 + k * 44;
      let bg = '#FFFFFF', fg = C.ink, bd = '#E6DCC6';
      if (fb) { if (k === q.answer) { bg = C.green; fg = '#fff'; bd = bg; } else if (k === fb.pick) { bg = C.red; fg = '#fff'; bd = bg; } }
      else if (k === G.sel) { bd = C.teal; }
      rrect(16, y, 208, 36, 8, bd); rrect(18, y + 2, 204, 32, 7, bg);
      fitText(o, 120, y + 18, q.type === 'zh2en' || q.type === 'letter' ? 18 : 16, 190, fg, 'center', q.type === 'en2zh' ? F_BODY : F_WORD, 600);
    });
    if (fb && !fb.ok) { fitText(`${w.w}  ${w.m}`, 120, 282, 13, 210, '#7A6F5A', 'center'); hint('OK 继续'); }
    else if (!fb) hint('UP/DOWN 选择 · OK 确认');
  }
  // 右下角小 NABO 表情
  ctx.save(); ctx.translate(196, 268); ctx.scale(0.32, 0.32); ctx.translate(-120, -104);
  drawNabo(lookFor(s), fb ? (fb.ok ? 'happy' : 'sad') : 'idle', G.t);
  ctx.restore();
}
function hint(s) { text(s, 12, 306, 11, '#9A8F78'); }

function drawSummary(s, G) {
  const r = G.summary;
  rect(0, 0, 240, 320, C.ink);
  text(r.title, 120, 34, 20, '#FFFFFF', 'center', F_BODY, 700);
  const rows = r.rows;
  rows.forEach(([a, b], k) => { text(a, 30, 76 + k * 24, 13, C.muted); text(b, 210, 76 + k * 24, 13, '#FFFFFF', 'right', F_MONO, 700); });
  let y = 84 + rows.length * 24;
  for (const n of r.notes.slice(0, 5)) { fitText(n, 120, y, 13, 210, C.gold, 'center', F_BODY, 700); y += 20; }
  const si = Math.min(s.stage, 5);
  drawSprite(si, spriteForm(s, si), 120, 286, G.t, 1.3);
  text('OK', 228, 306, 11, C.muted, 'right', F_MONO);
}

// ---------- 星图：7500 个点，100 列 × 75 行 ----------
const STAR_COL = ['#1C2440', '#1F5A63', '#2A7C82', '#3FB8A9', '#E8B84A', '#F2CF73', '#FBE3A3', '#FFFFFF'];
function drawStars(s, G) {
  rect(0, 0, 240, 320, '#0B1020');
  text('词汇星图', 12, 18, 14, '#FFFFFF', 'left', F_BODY, 700);
  text(`${mastered(s)} / ${TOTAL_WORDS}`, 228, 18, 12, C.gold, 'right', F_MONO, 700);
  const x0 = 2, y0 = 40, pitch = 2.18;
  for (let i = 0; i < TOTAL_WORDS; i++) {
    const b = s.box[i];
    const col = i % 100, row = Math.floor(i / 100);
    const tw = b >= 6 && ((i * 7 + Math.floor(G.t * 3)) % 23 === 0);
    ctx.fillStyle = tw ? '#FFFFFF' : STAR_COL[b];
    ctx.fillRect(x0 + col * pitch + 0.3, y0 + row * pitch + 0.3, b ? 1.8 : 1.2, b ? 1.8 : 1.2);
  }
  // 学段分界线
  for (const st of STAGES.slice(0, -1)) {
    const row = st.end / 100, y = y0 + row * pitch;
    ctx.fillStyle = 'rgba(232,184,74,.35)'; ctx.fillRect(0, y - 0.3, 220, 0.6);
  }
  STAGES.forEach(st => text(st.name, 239, y0 + ((st.start + st.end) / 200) * pitch, 8, '#6B7287', 'right'));
  const legend = [['没学', 0], ['刚学', 1], ['熟悉', 3], ['掌握', 4], ['毕业', 7]];
  legend.forEach(([n, b], k) => { rect(14 + k * 46, 236, 8, 8, STAR_COL[b]); text(n, 26 + k * 46, 240, 10, C.muted); });
  const h = boxHistogram(s);
  text(`学过 ${learned(s)} · 熟悉 ${h.slice(3).reduce((a, b) => a + b, 0)} · 毕业 ${h[7]}`, 120, 270, 11, '#C5CADB', 'center');
  text('OK 返回', 12, 306, 11, C.muted);
}

// ---------- 护照：每个学段一页 ----------
const INK = ['#C0503A', '#2F6FD6', '#6A4BC4', '#D9762B', '#178F96', '#C2457E'];
function drawPassport(s, G) {
  const st = STAGES[G.page];
  rect(0, 0, 240, 320, '#F3EBDA');
  rect(0, 0, 240, 40, '#24345C');
  text(`护照 · ${st.region}`, 12, 20, 14, '#F3EBDA', 'left', F_BODY, 700);
  text(`${G.page + 1}/${STAGES.length}`, 228, 20, 11, '#AEB8D6', 'right', F_MONO);
  const u0 = st.start / UNIT_SIZE, nu = (st.end - st.start) / UNIT_SIZE;
  const cols = nu > 30 ? 7 : nu > 20 ? 6 : nu > 16 ? 5 : 4, cell = 228 / cols, r = cell / 2 - 3;
  for (let k = 0; k < nu; k++) {
    const u = u0 + k, cx = 6 + cell * (k % cols) + cell / 2, cy = 52 + cell * Math.floor(k / cols) + cell / 2;
    const city = st.cities[k] || `第${k + 1}站`, done = unitStamped(s, u);
    ctx.save(); ctx.translate(cx, cy); ctx.rotate(((k * 37) % 21 - 10) * Math.PI / 180);
    ctx.strokeStyle = done ? INK[G.page] : 'rgba(0,0,0,.12)'; ctx.lineWidth = done ? 2 : 1; ctx.setLineDash(done ? [] : [3, 3]);
    ctx.beginPath(); ctx.arc(0, 0, r, 0, Math.PI * 2); ctx.stroke(); ctx.setLineDash([]);
    fitText(city, 0, 0, cols === 4 ? 13 : 10, r * 1.8, done ? INK[G.page] : 'rgba(0,0,0,.25)', 'center', F_BODY, 700);
    ctx.restore();
  }
  text(`已盖 ${Array.from({ length: nu }, (_, k) => unitStamped(s, u0 + k)).filter(Boolean).length} / ${nu} 枚`, 12, 306, 11, '#7A6F5A');
  text('UP/DOWN 翻页 · OK 返回', 228, 306, 10, '#9A8F78', 'right');
}

// ---------- 书桌 ----------
function deskItems(s) {
  const m = mastered(s);
  const items = PROPS.map(p => ({ kind: 'prop', id: p.id, label: p.name, locked: m < p.need, need: p.need, on: !s.hidden.includes(p.id) }));
  items.push({ kind: 'box', label: `开盲盒（5 金币）· 已有 ${s.toys.length}/${BOX_TOYS.length}` });
  items.push({ kind: 'back', label: '返回' });
  return items;
}
function drawDeskMenu(s, G) {
  rect(0, 0, 240, 320, C.ink);
  text('书桌', 12, 18, 14, '#FFFFFF', 'left', F_BODY, 700);
  iconCoin(200, 18); text(String(s.coins), 210, 18, 12, '#FFFFFF', 'left', F_MONO, 700);
  const items = deskItems(s), top = Math.max(0, Math.min(G.sel - 4, items.length - 10));
  items.slice(top, top + 10).forEach((it, k) => {
    const i = top + k, y = 40 + k * 26;
    if (i === G.sel) rrect(8, y, 224, 24, 6, '#26304D');
    let right = '';
    if (it.kind === 'prop') right = it.locked ? `掌握 ${it.need} 词解锁` : it.on ? '摆着' : '收起';
    text(it.label, 16, y + 12, 13, it.locked ? '#5B6584' : '#FFFFFF');
    text(right, 224, y + 12, 11, it.locked ? '#5B6584' : it.on ? C.teal : C.muted, 'right');
  });
  if (G.toast) fitText(G.toast, 120, 306, 12, 220, C.gold, 'center', F_BODY, 700);
  else text('OK 摆放 / 收起', 12, 306, 11, C.muted);
}
