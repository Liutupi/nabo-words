// 画图工具：房间、占位 NABO（按学段换衣服）、单词精灵、书桌道具。坐标按 240×320 屏幕。
'use strict';

const F_BODY = '"Noto Sans SC","PingFang SC","Microsoft YaHei",sans-serif';
const F_WORD = '"Fredoka","Noto Sans",sans-serif';
const F_IPA = '"Noto Sans","Lucida Sans Unicode","Arial Unicode MS",sans-serif';
const F_MONO = '"Space Mono",ui-monospace,Menlo,monospace';

const C = {
  ink: '#121829', paper: '#FFF8EC', gold: '#E8B84A', teal: '#3FB8A9', red: '#E0564F', green: '#4DBB6A',
  muted: '#8C93A8', skin: '#F6E2C6', hair: '#3A2A22', cap: '#FAFAF7', glass: '#2A2A33', screen: '#1A2238',
};

// 每个学段的房间配色与服装
const LOOK = [
  { wall: '#F6D7A7', trim: '#E3B57C', desk: '#B07845', top: '#C88A52', body: '#F2C230', acc: '#C0503A', kind: 'backpack' },
  { wall: '#BFE3E0', trim: '#8CC7C1', desk: '#9C6B48', top: '#B47C55', body: '#2F6FD6', acc: '#FFFFFF', kind: 'track' },
  { wall: '#C9D3EE', trim: '#9DAAD6', desk: '#7B5236', top: '#916243', body: '#1F2E5A', acc: '#B8323A', kind: 'tie' },
  { wall: '#D9D4C7', trim: '#B5AD99', desk: '#5E4B3C', top: '#715A48', body: '#7D8590', acc: '#2B2F38', kind: 'phones' },
  { wall: '#E8DDF0', trim: '#C3B0D6', desk: '#6A4A3A', top: '#7E5A47', body: '#F2F2F2', acc: '#3B3F4C', kind: 'shirt' },
  { wall: '#24324F', trim: '#33466E', desk: '#4A3A30', top: '#5B483B', body: '#B08A5A', acc: '#22252C', kind: 'trench' },
  { wall: '#2A2440', trim: '#3E3560', desk: '#4A3A30', top: '#5B483B', body: '#1C1C24', acc: '#E8B84A', kind: 'gown' },
];
const SPRITE_KEYS = ['doudou', 'paopao', 'xingxing', 'huohuo', 'langlang', 'jiguang'];   // 美术文件名
const SPRITE_COL = ['#7BC86C', '#5AA9E6', '#9B7BE0', '#F08A3C', '#3CC2C8', '#F07EB5'];

let ctx;
function bindCtx(c) { ctx = c; }

// ---------- 已交付的美术（tools/build_proto_atlas.py 生成），没有就用下面的占位画法 ----------
const ART_IMG = {};
for (const [name, a] of Object.entries(window.NABO_ART || {})) { const im = new Image(); im.src = a.src; ART_IMG[name] = { im, ...a }; }
function hasArt(name) { const a = ART_IMG[name]; return !!(a && a.im.complete && a.im.naturalWidth); }
// 把图的锚点放到 (x, y)
function drawArt(name, x, y) {
  if (!hasArt(name)) return false;
  const a = ART_IMG[name];
  ctx.imageSmoothingEnabled = false;
  ctx.drawImage(a.im, Math.round(x - a.ax), Math.round(y - a.ay), a.w, a.h);
  return true;
}
const NABO_ART_BY_LOOK = ['nabo_primary'];
const STAGE_KEYS = ['primary', 'junior', 'senior', 'cet4', 'cet6', 'adv', 'grad'];
// 视频抽出的动画（sprites/ → tools/build_proto_atlas.py → assets/nabo_anims.js）
const ANIMS = {};
for (const [stage, set] of Object.entries(window.NABO_ANIMS || {})) {
  ANIMS[stage] = {};
  for (const [name, a] of Object.entries(set)) { const im = new Image(); im.src = a.src; ANIMS[stage][name] = { im, ...a }; }
}
function animsFor(look) {
  const set = ANIMS[STAGE_KEYS[LOOK.indexOf(look)]];
  return set && set.idle && set.idle.im.complete && set.idle.im.naturalWidth ? set : null;
}
function naboArtFor(look) {
  if (animsFor(look)) return 'anim';
  const n = NABO_ART_BY_LOOK[LOOK.indexOf(look)]; return n && hasArt(n) ? n : null;
}
// 播放状态 play = { name, seg, t0, loop }；返回第几帧，以及非循环时是否已播完
function animFrame(a, play, t) {
  const [lo, hi] = (play.seg && a.segments[play.seg]) || [0, a.n];
  const k = Math.max(0, Math.floor((t - play.t0) * a.fps));
  if (play.loop) return { i: lo + k % (hi - lo), done: false };
  return { i: Math.min(lo + k, hi - 1), done: k >= hi - lo };
}
// 某学段还没做的动作，退回同学段的相近动作（不借别的学段，服装会对不上）
const ANIM_FALLBACK = { cheer: 'happy', sleep: 'idle' };
function resolveAnim(set, play) {
  if (set[play.name]) return { a: set[play.name], play };
  const alt = set[ANIM_FALLBACK[play.name]] || set.idle;
  return { a: alt, play: { ...play, seg: null } };   // 片段名是原动作的，退回后整段播放
}
function animDone(look, play, t) {
  const set = animsFor(look);
  if (!set) return true;
  const r = resolveAnim(set, play);
  return animFrame(r.a, r.play, t).done;
}
// 表情 → 默认播放：答题角标用高潮段循环
const MOOD_PLAY = { idle: { name: 'idle', loop: true }, happy: { name: 'happy', seg: 'peak', loop: true }, sad: { name: 'sad', seg: 'peak', loop: true } };

function rect(x, y, w, h, col) { ctx.fillStyle = col; ctx.fillRect(x, y, w, h); }
function rrect(x, y, w, h, r, col) { ctx.fillStyle = col; ctx.beginPath(); ctx.roundRect(x, y, w, h, r); ctx.fill(); }
function circle(x, y, r, col) { ctx.fillStyle = col; ctx.beginPath(); ctx.arc(x, y, r, 0, Math.PI * 2); ctx.fill(); }
function text(s, x, y, size, col, align = 'left', font = F_BODY, weight = 400) {
  ctx.font = `${weight} ${size}px ${font}`; ctx.fillStyle = col; ctx.textAlign = align; ctx.textBaseline = 'middle'; ctx.fillText(s, x, y);
}
function fitText(s, x, y, size, maxW, col, align, font, weight) {
  let sz = size;
  ctx.font = `${weight || 400} ${sz}px ${font || F_BODY}`;
  while (sz > 9 && ctx.measureText(s).width > maxW) { sz--; ctx.font = `${weight || 400} ${sz}px ${font || F_BODY}`; }
  text(s, x, y, sz, col, align, font, weight);
}

// ---------- 房间 ----------
function drawRoom(look, t) {
  rect(0, 0, 240, 214, look.wall);
  rect(0, 132, 240, 3, look.trim);
  if (look === LOOK[5] || look === LOOK[6]) {           // 夜晚学段：墙上星星
    for (let i = 0; i < 14; i++) { const x = (i * 53) % 240, y = 20 + (i * 37) % 100; circle(x, y, (i + Math.floor(t * 2)) % 5 ? 0.8 : 1.4, '#E9E3FF'); }
  }
}
function drawDesk(look) {
  rect(0, 176, 240, 6, look.top);
  rect(0, 182, 240, 32, look.desk);
  rect(0, 182, 240, 2, 'rgba(0,0,0,.18)');
}

// ---------- 占位 NABO：脸、眼镜、白帽身份固定，只换衣服 ----------
function drawNabo(stageLook, mood, t, play) {
  const bob = mood === 'happy' ? -Math.abs(Math.sin(t * 9)) * 5 : Math.sin(t * 2) * 1.2;
  const cx = 120, hy = 104 + bob, L = stageLook;
  const set = animsFor(stageLook);
  if (set) {
    const p = play || { ...MOOD_PLAY[mood] || MOOD_PLAY.idle, t0: 0 };
    const r = resolveAnim(set, p), a = r.a, { i } = animFrame(a, r.play, t);
    ctx.imageSmoothingEnabled = false;
    ctx.drawImage(a.im, i * a.fw, 0, a.fw, a.fh, Math.round(cx - a.ax), Math.round(176 - a.ay), a.fw, a.fh);
    return;
  }
  const art = naboArtFor(stageLook);
  if (art) {   // 视频帧到之前先用起始图；开心 / 沮丧只做位移和汗滴
    drawArt(art, cx, 176 + (mood === 'happy' ? bob : mood === 'sad' ? 2 : 0));
    if (mood === 'sad') { ctx.fillStyle = '#7CC4F0'; ctx.beginPath(); ctx.ellipse(cx + 30, hy - 8, 3, 5, 0, 0, Math.PI * 2); ctx.fill(); }
    return;
  }
  // 身体
  ctx.fillStyle = L.body; ctx.beginPath(); ctx.moveTo(cx - 40, 178); ctx.quadraticCurveTo(cx - 38, hy + 34, cx, hy + 32); ctx.quadraticCurveTo(cx + 38, hy + 34, cx + 40, 178); ctx.fill();
  if (L.kind === 'backpack') { rect(cx - 30, hy + 38, 6, 40, L.acc); rect(cx + 24, hy + 38, 6, 40, L.acc); }
  if (L.kind === 'track') { rect(cx - 36, hy + 50, 6, 30, L.acc); rect(cx + 30, hy + 50, 6, 30, L.acc); rect(cx - 3, hy + 34, 6, 44, L.acc); }
  if (L.kind === 'tie') { rect(cx - 9, hy + 32, 18, 8, '#F4F4F4'); ctx.fillStyle = L.acc; ctx.beginPath(); ctx.moveTo(cx - 4, hy + 38); ctx.lineTo(cx + 4, hy + 38); ctx.lineTo(cx + 6, hy + 68); ctx.lineTo(cx, hy + 74); ctx.lineTo(cx - 6, hy + 68); ctx.fill(); }
  if (L.kind === 'shirt') { rect(cx - 1, hy + 36, 2, 40, '#C9CBD3'); rect(cx - 30, hy + 40, 5, 38, L.acc); rect(cx + 25, hy + 40, 5, 38, L.acc); }
  if (L.kind === 'trench') { rect(cx - 2, hy + 34, 4, 44, '#8A6A42'); rrect(cx - 26, hy + 52, 18, 13, 3, L.acc); circle(cx - 17, hy + 58, 4, '#5C6170'); }
  if (L.kind === 'gown') { ctx.fillStyle = L.acc; ctx.beginPath(); ctx.moveTo(cx - 16, hy + 32); ctx.lineTo(cx, hy + 56); ctx.lineTo(cx + 16, hy + 32); ctx.fill(); }
  // 头
  circle(cx, hy, 30, L.kind === 'phones' ? L.body : C.skin);
  circle(cx, hy + 2, 28, C.skin);
  ctx.fillStyle = C.hair; ctx.beginPath(); ctx.arc(cx, hy - 4, 29, Math.PI * 1.05, Math.PI * 1.95); ctx.fill();
  if (L.kind === 'phones') { ctx.strokeStyle = L.acc; ctx.lineWidth = 5; ctx.beginPath(); ctx.arc(cx, hy, 33, Math.PI * 1.1, Math.PI * 1.9); ctx.stroke(); rrect(cx - 38, hy - 6, 10, 18, 4, L.acc); rrect(cx + 28, hy - 6, 10, 18, 4, L.acc); }
  // 白帽 + 帽标
  ctx.fillStyle = C.cap; ctx.beginPath(); ctx.ellipse(cx, hy - 18, 30, 17, 0, Math.PI, 0); ctx.fill();
  rrect(cx - 34, hy - 20, 68, 7, 3, C.cap);
  ctx.strokeStyle = 'rgba(0,0,0,.12)'; ctx.lineWidth = 1; ctx.strokeRect(cx - 34, hy - 20, 68, 7);
  text('NABO', cx, hy - 27, 9, '#2B2B33', 'center', F_WORD, 700);
  // 眼镜和眼睛
  const blink = mood === 'idle' && (t % 4) < 0.12;
  for (const dx of [-11, 11]) {
    ctx.strokeStyle = C.glass; ctx.lineWidth = 2; ctx.beginPath(); ctx.arc(cx + dx, hy + 2, 8, 0, Math.PI * 2); ctx.stroke();
    if (blink) rect(cx + dx - 3, hy + 2, 6, 1.5, C.hair);
    else if (mood === 'happy') { ctx.lineWidth = 2; ctx.strokeStyle = C.hair; ctx.beginPath(); ctx.arc(cx + dx, hy + 4, 3, Math.PI * 1.1, Math.PI * 1.9); ctx.stroke(); }
    else circle(cx + dx, hy + 2, 2.4, C.hair);
  }
  rect(cx - 3, hy + 1, 6, 1.5, C.glass);
  // 嘴
  ctx.strokeStyle = '#8A4B3A'; ctx.lineWidth = 2; ctx.beginPath();
  if (mood === 'sad') ctx.arc(cx, hy + 20, 5, Math.PI * 1.15, Math.PI * 1.85);
  else ctx.arc(cx, hy + 14, mood === 'happy' ? 6 : 4, Math.PI * 0.15, Math.PI * 0.85);
  ctx.stroke();
  if (mood === 'sad') { ctx.fillStyle = '#7CC4F0'; ctx.beginPath(); ctx.ellipse(cx + 26, hy - 6, 3, 5, 0, 0, Math.PI * 2); ctx.fill(); }
}

// ---------- 单词精灵 ----------
// acc：戴的饰品 id（ACCESSORIES），null 表示不戴
function drawSprite(stageIdx, form, x, y, t, scale = 1, acc = null) {
  const col = SPRITE_COL[Math.min(stageIdx, 5)], bob = Math.sin(t * 3 + stageIdx) * 2 * scale;
  const artName = SPRITE_KEYS[Math.min(stageIdx, 5)] + '_' + ['egg', 'baby', 'teen', 'final'][form];
  const art = artName && hasArt(artName) ? ART_IMG[artName] : null;
  const r = [0, 11, 13, 15][form];
  // 饰品按形态的高度和半宽定位（原点在精灵底部中点）
  const h = art ? art.h : form === 0 ? 28 : 2 * r, hw = art ? art.w / 2 : form === 0 ? 11 : r;
  ctx.save(); ctx.translate(x, y + bob); ctx.scale(scale, scale);
  if (acc === 'cape') drawAccessory('cape', form, h, hw);
  if (art) drawArt(artName, 0, 0);
  else {
    if (form === 0) {
      ctx.fillStyle = '#FBF4E6'; ctx.beginPath(); ctx.ellipse(0, -14, 11, 14, 0, 0, Math.PI * 2); ctx.fill();
      circle(-4, -18, 2.5, col); circle(4, -10, 2, col); circle(3, -22, 1.6, col);
      ctx.strokeStyle = 'rgba(0,0,0,.25)'; ctx.lineWidth = 1; ctx.beginPath(); ctx.ellipse(0, -14, 11, 14, 0, 0, Math.PI * 2); ctx.stroke();
    } else {
      const r = [0, 11, 13, 15][form];
      if (form >= 2) { ctx.fillStyle = col; ctx.beginPath(); ctx.moveTo(-r + 3, -r - 2); ctx.lineTo(-r + 1, -r * 2 - 4); ctx.lineTo(-2, -r * 1.6); ctx.fill(); ctx.beginPath(); ctx.moveTo(r - 3, -r - 2); ctx.lineTo(r - 1, -r * 2 - 4); ctx.lineTo(2, -r * 1.6); ctx.fill(); }
      if (form === 3) { ctx.fillStyle = 'rgba(255,255,255,.7)'; ctx.beginPath(); ctx.ellipse(-r - 4, -r, 7, 4, -0.5, 0, Math.PI * 2); ctx.ellipse(r + 4, -r, 7, 4, 0.5, 0, Math.PI * 2); ctx.fill(); }
      ctx.fillStyle = col; ctx.beginPath(); ctx.ellipse(0, -r, r, r * 0.92, 0, 0, Math.PI * 2); ctx.fill();
      circle(-4, -r - 1, 2.2, '#1A1A22'); circle(4, -r - 1, 2.2, '#1A1A22'); circle(-3.4, -r - 1.8, 0.8, '#fff'); circle(4.6, -r - 1.8, 0.8, '#fff');
      circle(-7, -r + 3, 1.8, 'rgba(255,120,140,.5)'); circle(7, -r + 3, 1.8, 'rgba(255,120,140,.5)');
      if (form === 3) { ctx.fillStyle = C.gold; ctx.beginPath(); ctx.moveTo(-6, -2 * r + 1); ctx.lineTo(-6, -2 * r - 6); ctx.lineTo(-2, -2 * r - 2); ctx.lineTo(0, -2 * r - 8); ctx.lineTo(2, -2 * r - 2); ctx.lineTo(6, -2 * r - 6); ctx.lineTo(6, -2 * r + 1); ctx.fill(); }
    }
  }
  if (acc && acc !== 'cape') drawAccessory(acc, form, h, hw);
  ctx.restore();
}

// 饰品美术图的锚点位置（帽子类是底边中点，围巾、墨镜是中心）。小披风的美术图像两条腿，先用程序画的
const ACC_ART_AT = {
  flower: (h, hw) => [-hw * 0.55, -h + 8], bow: (h) => [0, -h + 6], star: (h, hw) => [hw * 0.55, -h + 8],
  gradcap: (h) => [0, -h + 3], phones: (h) => [0, -h + h * 0.45],
  scarf: (h) => [0, -h + h * 0.8 + 2], shades: (h) => [0, -h + h * 0.55],
};
function drawAccessory(id, form, h, hw) {
  if (form === 0 && (id === 'shades' || id === 'scarf')) return;   // 蛋没有眼睛和脖子
  if (ACC_ART_AT[id]) { const [x, y] = ACC_ART_AT[id](h, hw); if (drawArt('acc_' + id, x, y)) return; }
  const top = -h, ey = top + h * 0.55;   // 眼睛大约在身高一半略下（豆豆头顶有豆芽）
  switch (id) {
    case 'flower':
      for (let k = 0; k < 5; k++) circle(-hw * 0.55 + Math.cos(k * 1.26) * 2.6, top + 4 + Math.sin(k * 1.26) * 2.6, 2, '#F0607A');
      circle(-hw * 0.55, top + 4, 1.6, '#FFD84A'); break;
    case 'bow':
      ctx.fillStyle = '#E0466A'; ctx.beginPath(); ctx.moveTo(0, top + 2); ctx.lineTo(-6, top - 2); ctx.lineTo(-6, top + 6); ctx.closePath();
      ctx.moveTo(0, top + 2); ctx.lineTo(6, top - 2); ctx.lineTo(6, top + 6); ctx.closePath(); ctx.fill(); circle(0, top + 2, 1.8, '#B8304F'); break;
    case 'scarf':
      rrect(-hw * 0.85, top + h * 0.8, hw * 1.7, 4, 2, '#D9423B'); rect(hw * 0.3, top + h * 0.8 + 3, 3.5, 5, '#D9423B'); break;
    case 'star': {
      ctx.fillStyle = '#FFD84A'; ctx.beginPath();
      for (let k = 0; k < 10; k++) { const rr = k % 2 ? 1.8 : 4.2, a = -Math.PI / 2 + k * Math.PI / 5; ctx.lineTo(hw * 0.55 + Math.cos(a) * rr, top + 4 + Math.sin(a) * rr); }
      ctx.fill(); break;
    }
    case 'shades':
      rrect(-hw * 0.62, ey - 2, hw * 0.55, 4, 1.5, '#1A1A22'); rrect(hw * 0.07, ey - 2, hw * 0.55, 4, 1.5, '#1A1A22'); rect(-hw * 0.1, ey - 1, hw * 0.2, 1.2, '#1A1A22'); break;
    case 'cape':
      ctx.fillStyle = '#C0392B'; ctx.beginPath(); ctx.moveTo(-hw * 0.7, top + h * 0.45); ctx.lineTo(hw * 0.7, top + h * 0.45);
      ctx.lineTo(hw * 1.1, 0); ctx.lineTo(-hw * 1.1, 0); ctx.closePath(); ctx.fill(); break;
    case 'phones':
      ctx.strokeStyle = '#2B2F38'; ctx.lineWidth = 2; ctx.beginPath(); ctx.arc(0, top + h * 0.3, hw * 0.95, Math.PI * 1.1, Math.PI * 1.9); ctx.stroke();
      rrect(-hw - 2, top + h * 0.28, 4, 7, 2, '#2B2F38'); rrect(hw - 2, top + h * 0.28, 4, 7, 2, '#2B2F38'); break;
    case 'gradcap':
      ctx.fillStyle = '#1C1C24'; ctx.beginPath(); ctx.moveTo(0, top - 6); ctx.lineTo(9, top - 2); ctx.lineTo(0, top + 2); ctx.lineTo(-9, top - 2); ctx.closePath(); ctx.fill();
      rect(-4, top - 1, 8, 3, '#1C1C24'); ctx.strokeStyle = C.gold; ctx.lineWidth = 1; ctx.beginPath(); ctx.moveTo(0, top - 2); ctx.lineTo(7, top + 3); ctx.stroke(); break;
  }
}

// ---------- 书桌道具 ----------
const PROP_DRAW = {
  clock(t) { if (!drawArt('clock', 34, 52)) { circle(34, 52, 13, '#FFFFFF'); ctx.strokeStyle = '#333'; ctx.lineWidth = 2; ctx.beginPath(); ctx.arc(34, 52, 13, 0, Math.PI * 2); ctx.stroke(); } ctx.strokeStyle = '#2A3A44'; ctx.lineWidth = 1.5; const a = t * 0.5; ctx.beginPath(); ctx.moveTo(34, 53); ctx.lineTo(34 + Math.sin(a) * 6, 53 - Math.cos(a) * 6); ctx.moveTo(34, 53); ctx.lineTo(34, 49); ctx.stroke(); },
  cert() { if (drawArt('cert', 35, 91)) return; rect(18, 78, 34, 26, '#FFF6D8'); ctx.strokeStyle = C.gold; ctx.lineWidth = 2; ctx.strokeRect(18, 78, 34, 26); circle(35, 98, 4, C.red); },
  map() { if (drawArt('map', 201, 40)) return; rect(172, 22, 58, 36, '#BFE0F0'); ctx.fillStyle = '#7FBF6A'; ctx.beginPath(); ctx.ellipse(186, 36, 8, 6, 0, 0, 7); ctx.ellipse(208, 42, 11, 7, 0.4, 0, 7); ctx.ellipse(222, 30, 5, 4, 0, 0, 7); ctx.fill(); ctx.strokeStyle = '#6B4B2E'; ctx.lineWidth = 2; ctx.strokeRect(172, 22, 58, 36); },
  shelf() { if (!drawArt('shelf', 203, 104)) rect(170, 100, 66, 4, '#7A5434'); },
  // 有书架放书架右端；书架（800 词）之前放在桌面右端，奖杯（6000 词）比书架晚得多，不会冲突
  plant(t, onShelf) { if (drawArt('plant', onShelf ? 226 : 229, onShelf ? 97 : 176)) return; rrect(206, 86, 14, 14, 2, '#C8693A'); ctx.fillStyle = '#4FA35A'; ctx.beginPath(); ctx.ellipse(209, 80, 4, 9, -0.4, 0, 7); ctx.ellipse(217, 80, 4, 9, 0.4, 0, 7); ctx.fill(); },
  lamp() { if (drawArt('lamp', 28, 176)) return; rect(20, 172, 16, 4, '#444'); rect(26, 140, 3, 32, '#555'); ctx.fillStyle = '#F2A93B'; ctx.beginPath(); ctx.moveTo(18, 146); ctx.lineTo(40, 146); ctx.lineTo(34, 132); ctx.lineTo(24, 132); ctx.fill(); },
  globe() { if (drawArt('globe', 59, 176)) return; rect(52, 172, 14, 4, '#6B4B2E'); circle(59, 158, 11, '#5AA9E6'); ctx.fillStyle = '#7FBF6A'; ctx.beginPath(); ctx.ellipse(55, 155, 4, 5, 0.3, 0, 7); ctx.ellipse(63, 162, 3, 3, 0, 0, 7); ctx.fill(); },
  pencilcase() { if (drawArt('pencilcase', 87, 197)) return; rrect(70, 186, 34, 10, 4, '#E05A7A'); rect(94, 182, 3, 6, '#F2C230'); },
  laptop() { if (drawArt('laptop', 120, 178)) return; rrect(98, 150, 44, 26, 2, '#C9CDD6'); rect(101, 153, 38, 20, C.screen); rect(92, 176, 56, 3, '#AEB3BE'); },
  mug() { if (drawArt('mug', 157, 177)) return; rrect(150, 162, 14, 14, 2, '#FFFFFF'); ctx.strokeStyle = '#FFFFFF'; ctx.lineWidth = 2; ctx.beginPath(); ctx.arc(165, 169, 4, -1.4, 1.4); ctx.stroke(); rect(152, 164, 10, 3, '#6B3E26'); },
  phones() { if (drawArt('phones', 160, 204)) return; ctx.strokeStyle = '#2B2F38'; ctx.lineWidth = 3; ctx.beginPath(); ctx.arc(160, 196, 9, Math.PI, 0); ctx.stroke(); rrect(149, 194, 6, 8, 2, '#2B2F38'); rrect(165, 194, 6, 8, 2, '#2B2F38'); },
  trophy() { if (drawArt('trophy', 229, 176)) return; ctx.fillStyle = C.gold; ctx.beginPath(); ctx.moveTo(222, 146); ctx.lineTo(236, 146); ctx.lineTo(232, 160); ctx.lineTo(226, 160); ctx.fill(); rect(227, 160, 4, 8, C.gold); rect(222, 168, 14, 6, '#8A6A2A'); },
};
const TOY_COL = ['#F2D23A', '#E0564F', '#4FA35A', '#BFE0F0', '#5AA9E6', '#7BC86C', '#FFFFFF', '#E8B84A', '#F0B8A0', '#9B7BE0'];

const WALL_PROPS = new Set(['clock', 'cert', 'map', 'shelf']);
function isWallProp(id, s, m) { return WALL_PROPS.has(id) || (id === 'plant' && m >= 800 && !s.hidden.includes('shelf')); }
// layer：'wall' 画在 NABO 后面，'desk' 画在桌面之后
function drawProps(s, t, layer) {
  const m = mastered(s);
  for (const p of PROPS) {
    if (m < p.need || s.hidden.includes(p.id) || isWallProp(p.id, s, m) !== (layer === 'wall')) continue;
    PROP_DRAW[p.id](t, m >= 800 && !s.hidden.includes('shelf'));
  }
  // 盲盒摆件：有书架放书架上，没有就放桌角
  const onShelf = m >= 800 && !s.hidden.includes('shelf');
  if (onShelf !== (layer === 'wall')) return;
  // 书架上放最近开到的 4 个（右端留给绿植），没有书架就放桌沿前
  s.toys.slice(-4).forEach((name, i) => {
    const k = BOX_TOYS.indexOf(name), x = onShelf ? 180 + i * 11 : 10 + i * 13, y = onShelf ? 97 : 211;
    if (!drawArt('toy_' + TOY_SLUGS[k], x, y)) rrect(x - 3, y - 6, 6, 6, 2, TOY_COL[k % TOY_COL.length]);
  });
}
