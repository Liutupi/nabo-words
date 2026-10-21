"""把 art/ 下已交付的静态图缩到游戏尺寸，嵌成 data URI 写进原型。

输出 prototype/assets/art_atlas.js，定义 window.NABO_ART = {name: {src, w, h, ax, ay}}，
(ax, ay) 是锚点在缩小后图里的位置。用法：python3 tools/build_proto_atlas.py
"""
from __future__ import annotations

import base64
import io
import json
import logging
from dataclasses import dataclass
from pathlib import Path
from typing import Dict, Optional, Tuple

from PIL import Image

logger = logging.getLogger(__name__)

ROOT = Path(__file__).resolve().parents[1]
ART = ROOT / "art"
OUT = ROOT / "prototype" / "assets" / "art_atlas.js"
OUT_ANIMS = ROOT / "prototype" / "assets" / "nabo_anims.js"
SPRITES = ROOT / "sprites"
NABO_CAP_TO_DESK_PX = 110   # ART_BRIEF 第 5 节：主页大图帽顶到桌沿


@dataclass(frozen=True)
class Entry:
    name: str
    path: Path
    size: Tuple[int, int]               # 缩小后的宽高
    anchor: Tuple[float, float]         # 原图里的锚点
    scale: float                        # 原图 → 游戏尺寸


def _nabo_entries(manifest: Dict) -> list[Entry]:
    """各学段起始图（抠好底的版本），按帽顶到桌沿 110 px 缩放。"""
    entries = []
    sel = manifest.get("selected_primary", "a")
    geo = manifest["start_candidates"][sel]
    cutout = ART / "design" / "source" / f"primary_{sel}_cutout.png"
    if not cutout.exists():
        logger.warning("缺少抠图 %s，跳过 NABO", cutout)
        return entries
    k = NABO_CAP_TO_DESK_PX / geo["head_to_desk_px"]
    cw, ch = geo["canvas_px"]
    entries.append(Entry("nabo_primary", cutout, (round(cw * k), round(ch * k)), tuple(geo["desk_anchor"]), k))
    return entries


def _static_entries(manifest: Dict) -> list[Entry]:
    """manifest.json 里登记了 target_px 的道具和精灵。"""
    entries = []
    for group in ("props", "companion", "companion_designs"):
        for rel, info in manifest.get(group, {}).items():
            path = ART / rel
            if not path.exists():
                logger.warning("manifest 登记的 %s 不存在", rel)
                continue
            cw, ch = info["canvas_px"]
            tw, th = info["target_px"]
            ax, ay = info.get("anchor_px", (cw / 2, ch))
            entries.append(Entry(path.stem, path, (tw, th), (ax, ay), tw / cw))
    return entries


def _doudou_entries() -> list[Entry]:
    """manifest 没登记精灵时，按 ART_BRIEF 第 6 节的最终高度缩放第 1 轮的设计图。"""
    heights = {"egg": 28, "baby": 32, "teen": 40, "final": 48}
    entries = []
    for form, h in heights.items():
        path = ART / "design" / f"doudou_{form}.png"
        if not path.exists():
            continue
        with Image.open(path) as im:
            w0, h0 = im.size
        k = h / h0
        entries.append(Entry(f"doudou_{form}", path, (max(1, round(w0 * k)), h), (w0 / 2, h0), k))
    return entries


def _encode(entry: Entry) -> Optional[Dict]:
    try:
        with Image.open(entry.path) as im:
            small = im.convert("RGBA").resize(entry.size, Image.Resampling.LANCZOS)
    except OSError as e:
        logger.error("读不了 %s: %s", entry.path, e)
        return None
    buf = io.BytesIO()
    small.save(buf, "PNG", optimize=True)
    return {
        "src": "data:image/png;base64," + base64.b64encode(buf.getvalue()).decode(),
        "w": entry.size[0], "h": entry.size[1],
        "ax": round(entry.anchor[0] * entry.scale, 2), "ay": round(entry.anchor[1] * entry.scale, 2),
        "from": str(entry.path.relative_to(ROOT)),
    }


def _png_uri(im: Image.Image) -> str:
    """256 色带透明的 PNG，原型里够用，体积比 RGBA 小得多。"""
    buf = io.BytesIO()
    im.quantize(256, method=Image.Quantize.FASTOCTREE).save(buf, "PNG", optimize=True)
    return "data:image/png;base64," + base64.b64encode(buf.getvalue()).decode()


def build_anims() -> Dict[str, Dict]:
    """sprites/<stage>/anims.json 里的每个动作拼成一条横向帧条，裁到所有帧的并集包围盒。"""
    out: Dict[str, Dict] = {}
    for meta_path in sorted(SPRITES.glob("*/anims.json")):
        meta = json.loads(meta_path.read_text(encoding="utf-8"))
        ax, ay = meta["anchor"]
        stage = meta_path.parent.name
        for name, info in meta["anims"].items():
            frames = [Image.open(p).convert("RGBA") for p in sorted((meta_path.parent / name).glob("[0-9]*.png"))]
            if not frames:
                logger.warning("%s/%s 没有帧", stage, name)
                continue
            boxes = [f.getbbox() for f in frames if f.getbbox()]
            x0, y0 = min(b[0] for b in boxes), min(b[1] for b in boxes)
            x1, y1 = max(b[2] for b in boxes), max(b[3] for b in boxes)
            fw, fh = x1 - x0, y1 - y0
            strip = Image.new("RGBA", (fw * len(frames), fh))
            for i, f in enumerate(frames):
                strip.paste(f.crop((x0, y0, x1, y1)), (i * fw, 0))
            out.setdefault(stage, {})[name] = {
                "src": _png_uri(strip), "fw": fw, "fh": fh, "n": len(frames), "ax": ax - x0, "ay": ay - y0,
                "fps": info["fps"], "loop": info["loop"], "segments": info.get("segments", {}),
            }
            logger.info("动画 %s/%-6s %2d 帧 %d×%d", stage, name, len(frames), fw, fh)
    return out


def main() -> None:
    logging.basicConfig(level=logging.INFO, format="%(message)s")
    manifest = json.loads((ART / "manifest.json").read_text(encoding="utf-8"))
    entries = _nabo_entries(manifest) + _static_entries(manifest)
    if not any(e.name.startswith("doudou_") for e in entries):
        entries += _doudou_entries()
    atlas = {e.name: enc for e in entries if (enc := _encode(e))}
    OUT.parent.mkdir(parents=True, exist_ok=True)
    OUT.write_text("// 由 tools/build_proto_atlas.py 生成，不要手改\nwindow.NABO_ART = " + json.dumps(atlas, ensure_ascii=False) + ";\n", encoding="utf-8")
    for name, a in atlas.items():
        logger.info("%-16s %3d×%-3d 锚点 (%s, %s)  ← %s", name, a["w"], a["h"], a["ax"], a["ay"], a["from"])
    logger.info("写入 %s（%.0f KB）", OUT.relative_to(ROOT), OUT.stat().st_size / 1024)
    anims = build_anims()
    OUT_ANIMS.write_text("// 由 tools/build_proto_atlas.py 从 sprites/ 生成，不要手改\nwindow.NABO_ANIMS = " + json.dumps(anims, ensure_ascii=False) + ";\n", encoding="utf-8")
    logger.info("写入 %s（%.0f KB）", OUT_ANIMS.relative_to(ROOT), OUT_ANIMS.stat().st_size / 1024)


if __name__ == "__main__":
    main()
