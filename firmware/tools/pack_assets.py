"""打包独立固件的 assets 分区：词库、字体、NABO 动画、静态美术。

输出 firmware/assets/assets.bin（烧到 assets 分区）和 firmware/main/nw_asset_ids.h。
每一块数据打包后都会解码回来和源数据比对，对不上就报错退出。

用法（在仓库根目录）：
    python3 -I firmware/tools/pack_assets.py --fonts <字体目录>
    python3 -I firmware/tools/pack_assets.py --base-assets <已有 assets.bin>
字体目录里需要（Google Fonts，OFL 1.1）：
    NotoSansSC[wght].ttf、Fredoka[wdth,wght].ttf、NotoSans[wdth,wght].ttf、SpaceMono-Bold.ttf

文件格式（小端）：
  assets.bin  'NWA1' u32 count，然后 count × {char name[24]; u32 off; u32 size}，数据 4 字节对齐
  words.bin   'NWD1' u16 count u16 pad；count × u32 记录偏移；count × u16 按小写拼写排序的序号；
              记录：word\\0 ipa\\0 pos\\0 meaning\\0
  font_*.bin  'NWF1' u16 count u8 px u8 line_h i8 ascent u8 pad[3]；count × {u32 cp; u32 off; u8 w,h; i8 x,y; u8 adv; u8 pad[3]}
              按码位排序；位图 4 bpp，每行 ceil(w/2) 字节，高半字节在前。y 是位图顶边相对基线的偏移（向下为正）
  art.bin     'NWI1' u16 count u16 pad；count × {char name[20]; u16 w,h; i16 ax,ay; u32 off}；像素 w×h×3：u16 RGB565 + u8 alpha
  nabo_<学段>.bin  NTS1 动画包，格式同牌桌 tools/generate_table_sprites.py（调色板 + 首帧关键帧 + 差分帧）
"""
from __future__ import annotations

import argparse
import json
import logging
import re
import struct
from pathlib import Path
from typing import Dict, List, Tuple

import numpy as np
from PIL import Image, ImageFont

logger = logging.getLogger(__name__)

ROOT = Path(__file__).resolve().parents[2]
FW = ROOT / "firmware"
ART = ROOT / "art"
SPRITES = ROOT / "sprites"
OUT_BIN = FW / "assets" / "assets.bin"
OUT_H = FW / "main" / "nw_asset_ids.h"

STAGES = ["primary", "junior", "senior", "cet4", "cet6", "adv", "grad"]
# 每个学段的墙色：NABO 边缘预先和它混合（固件里不做 alpha 混合），和 nw_theme.c 的 LOOK 一致
WALL = ["#F6D7A7", "#BFE3E0", "#C9D3EE", "#D9D4C7", "#E8DDF0", "#24324F", "#2A2440"]
ANIM_ORDER = ["idle", "happy", "sad", "cheer", "sleep"]
ANCHOR = (128, 224)
COLORS = 64
ALPHA_CUT = 64
STABLE = 24
MAX_COLOR_ERR = 8.0

ASCII = "".join(chr(c) for c in range(0x20, 0x7F))
IPA_EXTRA = "æðŋɑɒɔəɜɡɪʃʊʌʒˈˌːθ"


def hex_rgb(h: str) -> Tuple[int, int, int]:
    return int(h[1:3], 16), int(h[3:5], 16), int(h[5:7], 16)


def rgb565(r: int, g: int, b: int) -> int:
    return (r >> 3) << 11 | (g >> 2) << 5 | b >> 3


# ---------------------------------------------------------------- 词库

def pack_words(words: List[dict]) -> bytes:
    n = len(words)
    recs = bytearray()
    offs = []
    head = 8 + 4 * n + 2 * n
    for w in words:
        offs.append(head + len(recs))
        for f in (w["w"], w["p"], w.get("pos", ""), w["m"]):
            recs += f.encode("utf-8") + b"\0"
    order = sorted(range(n), key=lambda i: (words[i]["w"].lower(), i))
    blob = struct.pack("<4sHH", b"NWD1", n, 0) + struct.pack(f"<{n}I", *offs) + struct.pack(f"<{n}H", *order) + recs
    # 核对
    for i in (0, n // 2, n - 1):
        o = struct.unpack_from("<I", blob, 8 + 4 * i)[0]
        parts = blob[o:].split(b"\0")[:4]
        assert parts[0].decode() == words[i]["w"] and parts[3].decode() == words[i]["m"], i
    lowers = [words[struct.unpack_from("<H", blob, 8 + 4 * n + 2 * k)[0]]["w"].lower() for k in range(n)]
    assert lowers == sorted(lowers)
    return blob


# ---------------------------------------------------------------- 字体

FONT_SPECS = [
    # id, 文件, 变体轴, 像素, 字符集
    ("f12", "NotoSansSC[wght].ttf", [500], 12, "zh"),
    ("f16", "NotoSansSC[wght].ttf", [700], 16, "zh"),
    ("f22", "NotoSansSC[wght].ttf", [700], 22, "zh"),
    ("w36", "Fredoka[wdth,wght].ttf", [600, 100], 36, "ascii"),
    ("w28", "Fredoka[wdth,wght].ttf", [600, 100], 28, "ascii"),
    ("w20", "Fredoka[wdth,wght].ttf", [600, 100], 20, "ascii"),
    ("w16", "Fredoka[wdth,wght].ttf", [600, 100], 16, "ascii"),
    ("ipa", "NotoSans[wdth,wght].ttf", [400, 100], 13, "ipa"),
    ("m11", "SpaceMono-Bold.ttf", None, 11, "ascii"),
    ("m18", "SpaceMono-Bold.ttf", None, 18, "ascii"),
]


def ui_chars() -> str:
    """固件源码里出现的所有非 ASCII 字符（界面文字、城市名、摆件名……）。"""
    chars = set()
    for p in sorted((FW / "main").glob("*.[ch]")):
        chars.update(c for c in p.read_text("utf-8") if ord(c) > 0x7E)
    # 注释里的符号不需要；只留 CJK、全角标点和常用符号
    return "".join(sorted(c for c in chars if 0x3000 <= ord(c) <= 0x9FFF or 0xFF00 <= ord(c) <= 0xFFEF
                          or c in "·—…“”‘’×√✓←→↑↓"))


def font_charset(kind: str, words: List[dict]) -> str:
    if kind == "ascii":
        return ASCII
    if kind == "ipa":
        return ASCII + IPA_EXTRA
    s = set(ASCII) | set(ui_chars())
    for w in words:
        s.update(w["m"])
    return "".join(sorted(s))


def pack_font(path: Path, axes, px: int, chars: str) -> Tuple[bytes, Dict]:
    font = ImageFont.truetype(str(path), px)
    if axes:
        font.set_variation_by_axes(axes)
    asc, desc = font.getmetrics()
    glyphs = []
    missing = []
    for ch in chars:
        cp = ord(ch)
        if cp != 0x20 and cp > 0x7E:
            # 字体里没有这个字：FreeType 会画 .notdef 方框，跳过
            if font.getmask(ch).getbbox() is None and not ch.isspace():
                missing.append(ch)
                continue
        mask, (ox, oy) = font.getmask2(ch, mode="L", anchor="ls")
        w, h = mask.size
        a = np.asarray(mask, np.uint8).reshape(h, w) if w and h else np.zeros((0, 0), np.uint8)
        adv = int(round(font.getlength(ch)))
        # 裁掉全空的边
        if a.size and a.max() > 0:
            ys, xs = np.where(a > 8)
            y0, y1, x0, x1 = ys.min(), ys.max() + 1, xs.min(), xs.max() + 1
            a = a[y0:y1, x0:x1]
            ox += int(x0)
            oy += int(y0)
        else:
            a = np.zeros((0, 0), np.uint8)
        q = ((a.astype(np.int32) * 15 + 127) // 255).astype(np.uint8)
        glyphs.append((cp, q, ox, oy, adv))
    glyphs.sort(key=lambda g: g[0])
    n = len(glyphs)
    head = 12 + 16 * n
    table, bits = bytearray(), bytearray()
    for cp, q, ox, oy, adv in glyphs:
        h, w = q.shape if q.size else (0, 0)
        off = head + len(bits)
        for row in q:
            padded = np.append(row, 0) if w % 2 else row
            bits += bytes((int(padded[i]) << 4) | int(padded[i + 1]) for i in range(0, len(padded), 2))
        assert -128 <= ox < 128 and -128 <= oy < 128 and adv < 256 and w < 256 and h < 256
        table += struct.pack("<IIBBbbB3x", cp, off, w, h, ox, oy, adv)
    line_h = asc + desc
    blob = struct.pack("<4sHBBb3x", b"NWF1", n, px, line_h, asc) + table + bits
    # 核对：解出每个字的位图
    for k, (cp, q, ox, oy, adv) in enumerate(glyphs):
        c2, off, w, h, x, y, a2 = struct.unpack_from("<IIBBbbB", blob, 12 + 16 * k)
        assert c2 == cp and (w, h) == (q.shape[1] if q.size else 0, q.shape[0] if q.size else 0)
        stride = (w + 1) // 2
        for yy in range(h):
            row = blob[off + yy * stride: off + (yy + 1) * stride]
            vals = [v for b in row for v in (b >> 4, b & 15)][:w]
            assert vals == list(q[yy]), (cp, yy)
    return blob, {"glyphs": n, "missing": "".join(missing), "line_h": line_h, "asc": asc}


# ---------------------------------------------------------------- 静态美术

def art_entries(manifest: Dict) -> List[Tuple[str, Image.Image, float, float]]:
    """(名字, 缩到游戏尺寸的 RGBA 图, 锚点 x, 锚点 y)"""
    out = []
    for group in ("props", "companion", "toys", "ui"):
        for rel, info in manifest[group].items():
            im = Image.open(ART / rel).convert("RGBA")
            cw, ch = info["canvas_px"]
            tw, th = info["target_px"]
            ax, ay = info.get("anchor_px", (cw / 2, ch))
            name = info.get("id") or Path(rel).stem
            prefix = {"toys": "toy_", "ui": "ui_"}.get(group, "")
            k = tw / cw
            out.append((prefix + name, im.resize((tw, th), Image.Resampling.LANCZOS), ax * k, ay * k))
    # 饰品：按 placement_suggestion 给每种形态各缩一份（README 待办：12 px 戴在 48 px 的完全体上太小）
    # 形态的平均宽度（像素），和 companion 的 target_px 对应
    form_w = {"egg": 25, "baby": 25, "teen": 35, "final": 42}
    for rel, info in manifest["accessories"].items():
        src = Image.open(ART / rel).convert("RGBA")
        bbox = src.getbbox()
        cw, ch = info["canvas_px"]
        ax, ay = info["anchor_px"]
        ps = info["placement_suggestion"]
        for form, fw in form_w.items():
            w = max(6, round(fw * ps["width_ratio"]))
            # 按内容宽度缩放（画布两边有留白）
            content_w = bbox[2] - bbox[0]
            k = w / content_w
            tw, th = max(1, round(cw * k)), max(1, round(ch * k))
            if "height_ratio" in ps:
                th = max(1, round(ch * k * 1.0))
            out.append((f"{info['id']}_{form}", src.resize((tw, th), Image.Resampling.LANCZOS), ax * k, ay * k))
    return out


def pack_art(entries) -> bytes:
    n = len(entries)
    head = 8 + 32 * n
    table, data = bytearray(), bytearray()
    for name, im, ax, ay in entries:
        a = np.asarray(im, np.uint8)
        h, w = a.shape[:2]
        off = head + len(data)
        for y in range(h):
            for x in range(w):
                r, g, b, al = (int(v) for v in a[y, x])
                data += struct.pack("<HB", rgb565(r, g, b) if al else 0, al)
        while len(data) % 4:
            data += b"\0"
        assert len(name) < 20, name
        table += struct.pack("<20sHHhhI", name.encode(), w, h, int(round(ax)), int(round(ay)), off)
    blob = struct.pack("<4sHH", b"NWI1", n, 0) + table + data
    # 核对
    for k, (name, im, ax, ay) in enumerate(entries):
        nm, w, h, x, y, off = struct.unpack_from("<20sHHhhI", blob, 8 + 32 * k)
        a = np.asarray(im, np.uint8)
        px = np.frombuffer(blob[off: off + 3 * w * h], np.uint8).reshape(h, w, 3)
        assert np.array_equal(px[..., 2], a[..., 3]), name
    return blob


# ---------------------------------------------------------------- NABO 动画（NTS1，与牌桌相同格式）

def flatten(frames: List[np.ndarray], bg) -> Tuple[List[np.ndarray], List[np.ndarray], Tuple[int, int, int, int]]:
    rgbs, masks = [], []
    for f in frames:
        a = f[..., 3:4].astype(np.float32) / 255.0
        rgb = f[..., :3].astype(np.float32) * a + np.array(bg, np.float32) * (1 - a)
        rgbs.append(rgb.round().clip(0, 255).astype(np.uint8))
        masks.append(f[..., 3] >= ALPHA_CUT)
    union = np.any(masks, axis=0)
    ys, xs = np.where(union)
    box = (int(xs.min()), int(ys.min()), int(xs.max()) + 1, int(ys.max()) + 1)
    x0, y0, x1, y1 = box
    return [r[y0:y1, x0:x1] for r in rgbs], [m[y0:y1, x0:x1] for m in masks], box


def quantize(rgbs, masks):
    strip = Image.fromarray(np.concatenate(rgbs, axis=1), "RGB")
    q = strip.quantize(colors=COLORS, method=Image.MEDIANCUT, dither=Image.Dither.NONE)
    pal = np.array(q.getpalette()[: 255 * 3], np.uint8).reshape(-1, 3)
    idx = np.asarray(q, np.uint8).astype(np.int32) + 1
    w = rgbs[0].shape[1]
    out = []
    for i, m in enumerate(masks):
        part = idx[:, i * w:(i + 1) * w].copy()
        part[~m] = 0
        out.append(part.astype(np.uint8))
    for i in range(1, len(out)):
        prev, cur = out[i - 1], out[i]
        both = (prev > 0) & (cur > 0)
        drift = np.abs(rgbs[i].astype(np.int32) - pal[np.maximum(prev, 1) - 1].astype(np.int32)).sum(axis=2)
        keep = both & (drift < STABLE)
        cur[keep] = prev[keep]
    used = int(max(p.max() for p in out))
    return pal[:used], out


def encode_row(row) -> bytes:
    out = bytearray()
    i, n = 0, len(row)
    while i < n:
        j = i
        while j + 1 < n and row[j + 1] == row[i] and j - i < 128:
            j += 1
        if j - i >= 1:
            out += bytes((0x80 | (j - i - 1), int(row[i])))
            i = j + 1
            continue
        k = i
        while k + 1 < n and k - i < 127 and row[k + 1] != row[k]:
            k += 1
        if k + 1 < n and row[k + 1] == row[k] and k > i:
            k -= 1
        out += bytes((k - i,)) + bytes(int(v) for v in row[i:k + 1])
        i = k + 1
    return bytes(out)


def encode_delta_row(prev, cur) -> bytes:
    out = bytearray()
    i, n = 0, len(cur)
    while i < n:
        same = cur[i] == prev[i]
        j = i
        while j < n and (cur[j] == prev[j]) == same and j - i < 128:
            j += 1
        if same:
            out.append(0x80 | (j - i - 1))
        else:
            out.append(j - i - 1)
            out += bytes(int(v) for v in cur[i:j])
        i = j
    return bytes(out)


def decode_row(data: bytes, w: int):
    row, x, pos = np.zeros(w, np.uint8), 0, 0
    while pos < len(data):
        t = data[pos]; pos += 1
        if t & 0x80:
            run = (t & 127) + 2; row[x:x + run] = data[pos]; pos += 1
        else:
            run = (t & 127) + 1; row[x:x + run] = np.frombuffer(data[pos:pos + run], np.uint8); pos += run
        x += run
    return row


def apply_delta_row(data: bytes, row) -> None:
    x, pos = 0, 0
    while pos < len(data):
        t = data[pos]; pos += 1
        run = (t & 127) + 1
        if not t & 0x80:
            row[x:x + run] = np.frombuffer(data[pos:pos + run], np.uint8); pos += run
        x += run


def pack_stage(stage: str, bg, step: int) -> Tuple[bytes, List[Dict]]:
    meta = json.loads((SPRITES / stage / "anims.json").read_text("utf-8"))
    anims = []
    for name in ANIM_ORDER:
        info = meta["anims"].get(name)
        if not info:
            continue
        paths = sorted((SPRITES / stage / name).glob("[0-9]*.png"))
        st = 1 if name in ("happy", "sad", "cheer") else step   # 表演动作保留 12 fps，循环的待机 / 睡眠降帧
        frames = [np.asarray(Image.open(p).convert("RGBA")) for p in paths[::st]]
        rgbs, masks, (x0, y0, x1, y1) = flatten(frames, bg)
        pal, idx = quantize(rgbs, masks)
        enc = []
        for k, f in enumerate(idx):
            rows = [encode_row(r) for r in f] if k == 0 else [encode_delta_row(p, r) for p, r in zip(idx[k - 1], f)]
            offs, acc = [], 0
            for r in rows:
                offs.append(acc); acc += len(r)
            offs.append(acc)
            assert acc <= 0xFFFF
            enc.append((struct.pack(f"<{len(offs)}H", *offs), b"".join(rows)))
        segs = {s: [v[0] // st, max(v[0] // st + 1, v[1] // st)] for s, v in info.get("segments", {}).items()}
        anims.append(dict(name=name, w=x1 - x0, h=y1 - y0, ox=x0 - ANCHOR[0], oy=y0 - ANCHOR[1], pal=pal, enc=enc,
                          idx=idx, rgb=rgbs, fps=max(1, round(info["fps"] / st)), loop=info["loop"], segs=segs))
    n_frames = sum(len(a["enc"]) for a in anims)
    anims_off, frames_off = 16, 16 + 32 * len(anims)
    cursor = frames_off + 8 * n_frames
    body, ftab, atab, first = bytearray(), bytearray(), bytearray(), 0
    for a in anims:
        pal_off = cursor + len(body)
        body += a["pal"].tobytes()
        for rows, rle in a["enc"]:
            ro = cursor + len(body); body += rows
            lo = cursor + len(body); body += rle
            ftab += struct.pack("<II", ro, lo)
        atab += struct.pack("<12sBBBBHHhhHHI", a["name"].encode(), 1, len(a["enc"]), a["fps"], 1 if a["loop"] else 0,
                            a["w"], a["h"], a["ox"], a["oy"], first, len(a["pal"]), pal_off)
        a["first"] = first
        first += len(a["enc"])
    blob = bytes(struct.pack("<4sHHII", b"NTS1", len(anims), n_frames, anims_off, frames_off) + atab + ftab + body)
    # 核对：逐帧解码回索引图，再核对颜色
    for k, a in enumerate(anims):
        f0 = struct.unpack_from("<12sBBBBHHhhHHI", blob, anims_off + 32 * k)[9]
        buf = np.zeros((a["h"], a["w"]), np.uint8)
        for i, want in enumerate(a["idx"]):
            ro, lo = struct.unpack_from("<II", blob, frames_off + 8 * (f0 + i))
            for y in range(a["h"]):
                s, e = struct.unpack_from("<HH", blob, ro + 2 * y)
                if i == 0:
                    buf[y] = decode_row(blob[lo + s: lo + e], a["w"])
                else:
                    apply_delta_row(blob[lo + s: lo + e], buf[y])
            if not np.array_equal(buf, want):
                raise ValueError(f"{stage}/{a['name']} 第 {i} 帧解码不一致")
            m = buf > 0
            err = np.abs(a["pal"][buf[m] - 1].astype(np.int32) - a["rgb"][i][m].astype(np.int32)).mean()
            if err > MAX_COLOR_ERR:
                raise ValueError(f"{stage}/{a['name']} 第 {i} 帧颜色误差 {err:.1f}")
    return blob, anims


# ---------------------------------------------------------------- 总装

def read_base_assets(path: Path) -> List[Tuple[str, bytes]]:
    """保留已有词库、字体和美术的原始字节，只更新从原始帧生成的动画。"""
    data = path.read_bytes()
    if len(data) < 8 or data[:4] != b"NWA1":
        raise ValueError("基础资源不是 NWA1 assets.bin")
    count = struct.unpack_from("<I", data, 4)[0]
    head = 8 + count * 32
    if head > len(data):
        raise ValueError("基础资源目录不完整")
    files, seen = [], set()
    for i in range(count):
        name, off, size = struct.unpack_from("<24sII", data, 8 + i * 32)
        name = name.split(b"\0", 1)[0].decode("ascii")
        if not name or name in seen or off < head or off + size > len(data):
            raise ValueError(f"基础资源条目无效：{name}")
        seen.add(name)
        if not name.startswith("nabo_"):
            files.append((name, data[off:off + size]))
    required = {"words.bin", "art.bin"} | {f"font_{f[0]}.bin" for f in FONT_SPECS}
    if {name for name, _ in files} != required:
        raise ValueError("基础资源的词库、字体或美术清单不符")
    return files


def main() -> None:
    ap = argparse.ArgumentParser()
    source = ap.add_mutually_exclusive_group(required=True)
    source.add_argument("--fonts", type=Path)
    source.add_argument("--base-assets", type=Path, help="复用已校验资源中的词库、字体和静态美术")
    ap.add_argument("--idle-step", type=int, default=1, choices=(1, 2), help="待机 / 睡眠取帧间隔，默认保留 12 fps 原帧")
    args = ap.parse_args()
    manifest = json.loads((ART / "manifest.json").read_text("utf-8"))

    art = art_entries(manifest)
    if args.base_assets:
        files = read_base_assets(args.base_assets)
        base_art = dict(files)["art.bin"]
        names = [base_art[8 + i * 32:28 + i * 32].split(b"\0", 1)[0].decode("ascii")
                 for i in range(struct.unpack_from("<H", base_art, 4)[0])]
        if names != [entry[0] for entry in art]:
            raise ValueError("基础资源美术编号与本仓库不一致，不能复用")
        logger.info("原样复用 %d 块静态资源", len(files))
    else:
        words = json.loads((ROOT / "data" / "words.json").read_text("utf-8"))["words"]
        files = [("words.bin", pack_words(words))]
        logger.info("词库 %d 词 %.0f KB", len(words), len(files[0][1]) / 1024)
        for fid, fname, axes, px, kind in FONT_SPECS:
            blob, info = pack_font(args.fonts / fname, axes, px, font_charset(kind, words))
            if info["missing"]:
                logger.warning("字体 %s 缺字：%s", fid, info["missing"])
            files.append((f"font_{fid}.bin", blob))
            logger.info("字体 %-4s %4d 字 行高 %d %.0f KB", fid, info["glyphs"], info["line_h"], len(blob) / 1024)
        files.append(("art.bin", pack_art(art)))
        logger.info("美术 %d 张 %.0f KB", len(art), len(files[-1][1]) / 1024)
    anim_info = {}
    for i, stage in enumerate(STAGES):
        if not (SPRITES / stage / "anims.json").exists():
            continue
        blob, anims = pack_stage(stage, hex_rgb(WALL[i]), args.idle_step)
        files.append((f"nabo_{stage}.bin", blob))
        anim_info[stage] = anims
        logger.info("NABO %-7s %s  %.0f KB", stage, " ".join(f"{a['name']}:{len(a['enc'])}@{a['fps']} {a['w']}x{a['h']}" for a in anims), len(blob) / 1024)

    n = len(files)
    head = 8 + 32 * n
    table, data = bytearray(), bytearray()
    for name, blob in files:
        while (head + len(data)) % 4:
            data += b"\0"
        table += struct.pack("<24sII", name.encode(), head + len(data), len(blob))
        data += blob
    out = struct.pack("<4sI", b"NWA1", n) + table + data
    if len(out) > 0x5F0000:
        raise ValueError(f"资源超出 assets 分区：{len(out)} > {0x5F0000}")
    OUT_BIN.parent.mkdir(parents=True, exist_ok=True)
    OUT_BIN.write_bytes(out)
    logger.info("assets.bin %.2f MB", len(out) / 1048576)

    # 头文件：美术和动画的编号、动作片段
    lines = ["#pragma once", "/* 由 firmware/tools/pack_assets.py 生成，不要手改 */", "", "enum {"]
    for k, (name, *_ ) in enumerate(art):
        lines.append(f"    NWA_{name.upper()} = {k},")
    lines += [f"    NWA_COUNT = {len(art)}", "};", "", "/* NABO 动作：每个学段的包里按这个顺序，缺的动作不占位，用 nw_anim_find() 按名字找 */"]
    lines.append("#define NW_ANIM_NAMES " + ", ".join(f'"{a}"' for a in ANIM_ORDER))
    lines.append("")
    # 片段（帧号已按降帧换算）
    lines.append("typedef struct { const char *stage, *anim, *seg; unsigned char lo, hi; } nw_seg_t;")
    lines.append("static const nw_seg_t NW_SEGS[] = {")
    for stage, anims in anim_info.items():
        for a in anims:
            for s, (lo, hi) in a["segs"].items():
                lines.append(f'    {{"{stage}", "{a["name"]}", "{s}", {lo}, {hi}}},')
    lines += ["};", ""]
    OUT_H.write_text("\n".join(lines), encoding="utf-8")
    logger.info("写入 %s", OUT_H.relative_to(ROOT))


if __name__ == "__main__":
    logging.basicConfig(level=logging.INFO, format="%(message)s")
    main()
