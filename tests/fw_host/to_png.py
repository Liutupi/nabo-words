"""把宿主渲染的 RGB565 原始帧（240×320，本机字节序）转成 PNG，可选拼成一张对照图。
用法：python3 tests/fw_host/to_png.py <输出.png> <帧1.rgb> [帧2.rgb ...]"""
import sys
import numpy as np
from PIL import Image


def load(path: str) -> Image.Image:
    v = np.fromfile(path, dtype="<u2").reshape(320, 240).astype(np.uint32)
    r = ((v >> 11) & 31) * 255 // 31
    g = ((v >> 5) & 63) * 255 // 63
    b = (v & 31) * 255 // 31
    return Image.fromarray(np.dstack([r, g, b]).astype(np.uint8), "RGB")


def main() -> None:
    out, frames = sys.argv[1], [load(p) for p in sys.argv[2:]]
    sheet = Image.new("RGB", (250 * len(frames) - 10, 320), (40, 40, 40))
    for i, f in enumerate(frames):
        sheet.paste(f, (250 * i, 0))
    sheet.save(out)


if __name__ == "__main__":
    main()
