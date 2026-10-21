"""把 Grok 的图生视频处理成游戏帧（沿用牌桌的流程，改成整块桌面）。

每段视频：ffmpeg 解码并重采样到 fps → 抠掉绿底和木色桌面（保留搭在桌沿上的手）→
按“帽顶到桌沿 = 110 px”缩放 → 桌沿中点对齐 256×256 画布的锚点 (128, 224)。
输出 sprites/<stage>/<anim>/NN.png 和 sprites/<stage>/anims.json（帧数、fps、片段、体积估算）。

用法：python3 tools/clip_to_sprites.py [tools/clips.json]
"""
from __future__ import annotations

import json
import logging
import subprocess
import sys
import zlib
from dataclasses import dataclass, field
from pathlib import Path
from typing import Dict, List, Tuple

import numpy as np
from PIL import Image

logger = logging.getLogger(__name__)

ROOT = Path(__file__).resolve().parents[1]
CANVAS = 256
ANCHOR: Tuple[int, int] = (128, 224)   # 桌沿中点在画布里的位置（ART_BRIEF 第 5 节）
NABO_PX = 110                          # 帽顶到桌沿的高度
PALETTE_COLORS = 64
BG_KEY = (28.0, 40.0)                  # 离绿底的色差：小于前者全透明，再过后者全不透明
DESK_KEY = (36.0, 30.0)                # 桌面区域里离木色的色差，同上
EDGE_ROWS, EDGE_TOL = 4, 45.0          # 桌沿上下几行是绿底和木色压缩混出的过渡色：离“绿底—木色”连线近的都抠掉


@dataclass(frozen=True)
class Clip:
    """一段视频和它在游戏里的用法。segments 是 {名字: [起始帧, 结束帧（不含）]}，按输出帧计。"""

    name: str
    source: str
    stage: str
    fps: float
    loop: bool
    cap_top_y: int
    desk_y: int
    start: float = 0.0
    end: float = 0.0
    segments: Dict[str, List[int]] = field(default_factory=dict)


def probe(path: Path) -> Tuple[int, int]:
    out = subprocess.run(["ffprobe", "-v", "error", "-select_streams", "v:0", "-show_entries", "stream=width,height",
                          "-of", "json", str(path)], check=True, capture_output=True, text=True).stdout
    s = json.loads(out)["streams"][0]
    return int(s["width"]), int(s["height"])


def decode(clip: Clip) -> List[np.ndarray]:
    """按 clip.fps 解码 [start, end)；end 为 0 表示到结尾。只取视频流，忽略音轨和封面缩略图。"""
    path = ROOT / clip.source
    w, h = probe(path)
    cmd = ["ffmpeg", "-v", "error", "-ss", f"{clip.start:.3f}"]
    if clip.end > 0:
        cmd += ["-to", f"{clip.end:.3f}"]
    cmd += ["-i", str(path), "-map", "0:v:0", "-vf", f"fps={clip.fps}", "-f", "rawvideo", "-pix_fmt", "rgb24", "-"]
    raw = subprocess.run(cmd, check=True, capture_output=True).stdout
    n = len(raw) // (w * h * 3)
    return [np.frombuffer(raw, np.uint8, w * h * 3, i * w * h * 3).reshape(h, w, 3) for i in range(n)]


def _ramp(dist: np.ndarray, key: Tuple[float, float]) -> np.ndarray:
    return np.clip((dist - key[0]) / key[1], 0.0, 1.0)


def key(frame: np.ndarray, desk_y: int) -> np.ndarray:
    """绿底和桌面变透明。桌面只在桌沿以下的行里按木色抠，手和手臂的肤色、衣服色离木色足够远。"""
    rgb = frame.astype(np.float32)
    border = np.concatenate([rgb[:8, :].reshape(-1, 3), rgb[:desk_y, :8].reshape(-1, 3), rgb[:desk_y, -8:].reshape(-1, 3)])
    bg = np.median(border, axis=0)
    alpha = _ramp(np.abs(rgb - bg).sum(axis=2), BG_KEY)
    desk = rgb[desk_y + 12:, :].reshape(-1, 3)
    wood = np.median(np.concatenate([desk[: desk.shape[0] // 4], desk[-desk.shape[0] // 4:]]), axis=0)
    below = slice(max(0, desk_y - 2), None)
    alpha[below] = np.minimum(alpha[below], _ramp(np.abs(rgb[below] - wood).sum(axis=2), DESK_KEY))
    edge = slice(max(0, desk_y - EDGE_ROWS), desk_y + EDGE_ROWS + 1)
    seg = wood - bg
    t = np.clip(((rgb[edge] - bg) @ seg) / float(seg @ seg), 0.0, 1.0)
    resid = np.abs(rgb[edge] - (bg + t[..., None] * seg)).sum(axis=2)
    alpha[edge][resid < EDGE_TOL] = 0.0
    out = rgb.copy()                       # 去绿边：半透明处把绿压到红蓝较大者
    g_cap = np.maximum(out[..., 0], out[..., 2]) + 12
    spill = (alpha < 1.0) & (out[..., 1] > g_cap)
    out[..., 1][spill] = g_cap[spill]
    return np.dstack([out, alpha * 255.0]).clip(0, 255).astype(np.uint8)


def place(rgba: np.ndarray, clip: Clip) -> Image.Image:
    """缩放并放到 256×256 画布上，桌沿中点对齐锚点。缩放前预乘 alpha，避免边缘带出背景色。"""
    scale = NABO_PX / float(clip.desk_y - clip.cap_top_y)
    h, w = rgba.shape[:2]
    pre = rgba.astype(np.float32)
    pre[..., :3] *= pre[..., 3:4] / 255.0
    nw, nh = round(w * scale), round(h * scale)
    arr = np.asarray(Image.fromarray(pre.astype(np.uint8), "RGBA").resize((nw, nh), Image.Resampling.LANCZOS), np.float32).copy()
    a = arr[..., 3:4]
    arr[..., :3] = np.where(a > 0, arr[..., :3] * 255.0 / np.maximum(a, 1), 0)
    small = Image.fromarray(arr.clip(0, 255).astype(np.uint8), "RGBA")
    canvas = Image.new("RGBA", (CANVAS, CANVAS))
    canvas.paste(small, (ANCHOR[0] - nw // 2, ANCHOR[1] - round(clip.desk_y * scale)), small)
    return canvas


def packed_size(im: Image.Image) -> int:
    """按固件格式估算一帧：64 色索引 + 16 级 alpha，裁到包围盒后 deflate。"""
    box = im.getbbox()
    if not box:
        return 0
    crop = im.crop(box)
    idx = crop.convert("RGB").quantize(PALETTE_COLORS, method=Image.Quantize.MEDIANCUT)
    alpha = (np.asarray(crop)[..., 3] >> 4).astype(np.uint8)
    return len(zlib.compress(np.asarray(idx, np.uint8).tobytes() + alpha.tobytes(), 9)) + PALETTE_COLORS * 2


def run(config_path: Path) -> Dict[str, Dict]:
    cfg = json.loads(config_path.read_text(encoding="utf-8"))
    out_root = ROOT / cfg["out_dir"]
    reports: Dict[str, Dict[str, Dict]] = {}
    for c in cfg["clips"]:
        clip = Clip(**c)
        try:
            frames = decode(clip)
        except subprocess.CalledProcessError as e:
            logger.error("解码失败 %s: %s", clip.source, e.stderr)
            raise
        d = out_root / clip.stage / clip.name
        d.mkdir(parents=True, exist_ok=True)
        for old in d.glob("*.png"):  # 含卷宗自动生成的 ._* 文件
            old.unlink(missing_ok=True)   # 删掉原文件时卷宗会顺带删掉它的 ._ 伴生文件
        total = 0
        for i, f in enumerate(frames):
            im = place(key(f, clip.desk_y), clip)
            im.save(d / f"{i:02d}.png")
            total += packed_size(im)
        bad = {k: v for k, v in clip.segments.items() if not (0 <= v[0] < v[1] <= len(frames))}
        if bad:
            raise ValueError(f"{clip.name} 的片段超出 {len(frames)} 帧：{bad}")
        reports.setdefault(clip.stage, {})[clip.name] = {
            "source": clip.source, "fps": clip.fps, "loop": clip.loop, "frames": len(frames),
            "segments": clip.segments, "packed_bytes": total}
        logger.info("%s/%s: %d 帧，估算 %.1f KB", clip.stage, clip.name, len(frames), total / 1024)
    for stage, rep in reports.items():
        meta = {"anchor": ANCHOR, "canvas": CANVAS, "nabo_px": NABO_PX, "anims": rep}
        (out_root / stage / "anims.json").write_text(json.dumps(meta, indent=2, ensure_ascii=False), encoding="utf-8")
    return reports


if __name__ == "__main__":
    logging.basicConfig(level=logging.INFO, format="%(message)s")
    run(Path(sys.argv[1]) if len(sys.argv) > 1 else ROOT / "tools" / "clips.json")
