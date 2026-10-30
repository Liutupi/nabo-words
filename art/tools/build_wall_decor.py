"""Export and verify the seven wall ornaments; does not modify firmware assets.

Run after the host UI probe:
    python3 art/tools/build_wall_decor.py --homes build/ui
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path

import numpy as np
from PIL import Image, ImageDraw

ART = Path(__file__).resolve().parents[1]
DESIGN = ART / "design/wall-decor"
STAGES = ["primary", "junior", "senior", "cet4", "cet6", "adv", "grad"]
WALLS = ["#F6D7A7", "#BFE3E0", "#C9D3EE", "#D9D4C7", "#E8DDF0", "#24324F", "#2A2440"]
TARGET = (48, 24)
ORIGIN = (180, 108)
# Half-open screen rectangles from nw_scene_home.c / nw_data.c and manifest anchors.
KEEPOUTS = {
    "NABO head corridor": (70, 24, 170, 132),
    "clock": (20, 38, 48, 66),
    "certificate": (17, 77, 53, 105),
    "map": (171, 21, 231, 59),
    "shelf": (170, 96, 236, 104),
    "plant on shelf": (218, 73, 234, 97),
}


def digest(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def export(source: Path) -> Image.Image:
    im = Image.open(source).convert("RGBA")
    alpha = im.getchannel("A")
    assert alpha.getextrema() == (0, 255), f"Source lacks genuine alpha: {source}"
    box = alpha.point(lambda v: 255 if v >= 128 else 0).getbbox()
    assert box, source
    im = im.crop(box)
    scale = min((TARGET[0] - 2) / im.width, (TARGET[1] - 2) / im.height)
    im = im.resize((max(1, round(im.width * scale)), max(1, round(im.height * scale))),
                   Image.Resampling.LANCZOS)
    canvas = Image.new("RGBA", TARGET)
    canvas.alpha_composite(im, ((TARGET[0] - im.width) // 2, (TARGET[1] - im.height) // 2))
    alpha = canvas.getchannel("A").point(lambda v: 255 if v >= 128 else 0)
    flat = canvas.convert("RGB").quantize(colors=32, dither=Image.Dither.NONE).convert("RGB")
    # Match the RGB565 display space before counting colors or exporting.
    pixels = np.asarray(flat).astype(np.uint16)
    pixels[:, :, 0] = (pixels[:, :, 0] >> 3) * 255 // 31
    pixels[:, :, 1] = (pixels[:, :, 1] >> 2) * 255 // 63
    pixels[:, :, 2] = (pixels[:, :, 2] >> 3) * 255 // 31
    out = Image.fromarray(pixels.astype(np.uint8)).convert("RGBA")
    out.putalpha(alpha)
    return out


def main() -> None:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--homes", type=Path, required=True, help="test_ui.py output with home_0.png through home_6.png")
    args = ap.parse_args()
    manifest_path = ART / "manifest.json"
    manifest = json.loads(manifest_path.read_text())
    jobs = json.loads((DESIGN / "prompts.json").read_text())["jobs"]
    assert [job["stage"] for job in jobs] == STAGES
    (ART / "wall").mkdir(exist_ok=True)
    preview_dir = ART / "preview/wall-decor"
    preview_dir.mkdir(parents=True, exist_ok=True)
    sheet = Image.new("RGB", (7 * 240, 320), "#FFF8EC")
    proof = Image.new("RGB", (7 * 240, 152), "#FFF8EC")
    rows = []
    raw_pixel_bytes = 0
    for i, job in enumerate(jobs):
        stage = job["stage"]
        source = DESIGN / job["source"]
        native = export(source)
        path = ART / f"wall/{stage}.png"
        native.resize((192, 96), Image.Resampling.NEAREST).save(path, optimize=True)
        pixels = np.asarray(native)
        colors = np.unique(pixels[:, :, :3][pixels[:, :, 3] > 0], axis=0)
        assert 0 < len(colors) <= 32, stage
        assert set(np.unique(pixels[:, :, 3])) == {0, 255}, stage
        screen_mask = np.zeros((320, 240), dtype=bool)
        x, y = ORIGIN
        screen_mask[y:y + TARGET[1], x:x + TARGET[0]] = pixels[:, :, 3] > 0
        assert not screen_mask[:24].any() and not screen_mask[132:].any(), stage
        for label, (x0, y0, x1, y1) in KEEPOUTS.items():
            assert not screen_mask[y0:y1, x0:x1].any(), (stage, label)
        home = Image.open(args.homes / f"home_{i}.png").convert("RGBA")
        assert home.size == (240, 320), stage
        original = np.asarray(home).copy()
        home.alpha_composite(native, ORIGIN)
        changed = np.any(np.asarray(home) != original, axis=2)
        assert not (changed & ~screen_mask).any(), stage
        home.convert("RGB").save(preview_dir / f"home_{stage}.png", optimize=True)
        home.resize((480, 640), Image.Resampling.NEAREST).convert("RGB").save(
            preview_dir / f"home_{stage}_2x.png", optimize=True)
        sheet.paste(home.convert("RGB"), (240 * i, 0))
        tile = Image.new("RGBA", (240, 152), WALLS[i])
        ImageDraw.Draw(tile).text((8, 4), stage + " / 48 x 24", fill="white" if i >= 5 else "#121829")
        tile.alpha_composite(native, (96, 24))
        tile.alpha_composite(native.resize((192, 96), Image.Resampling.NEAREST), (24, 52))
        proof.paste(tile.convert("RGB"), (240 * i, 0))
        raw_pixel_bytes += TARGET[0] * TARGET[1] * 3
        info = {
            "id": f"wall_{stage}", "stage": stage, "canvas_px": [192, 96],
            "target_px": list(TARGET), "anchor_px": [0, 0], "target_anchor_px": [0, 0],
            "anchor_type": "top_left", "screen_origin_px": list(ORIGIN),
            "opaque_rgb_colors": len(colors), "palette": ["#" + bytes(c).hex() for c in colors],
            "pixel_grid_4x": True, "sha256": digest(path),
            "source": f"design/wall-decor/{job['source']}", "source_sha256": digest(source),
            "integration": "prepared_only; not included by pack_assets.py or drawn by firmware",
        }
        manifest.setdefault("wall_decor", {})[f"wall/{stage}.png"] = info
        rows.append({"stage": stage, "result": "PASS", "visible_colors": len(colors),
                     "rgba_size": [192, 96], "keepouts_clear": True, "sha256": digest(path)})
    sheet.save(preview_dir / "homes.png", optimize=True)
    proof.save(preview_dir / "native-and-4x.png", optimize=True)
    report = {
        "result": "PASS", "generator": "Codex built-in ImageGen", "exports": rows,
        "screen_origin_px": list(ORIGIN), "keepouts_screen_px": KEEPOUTS,
        "raw_rgb565_alpha_pixel_bytes": raw_pixel_bytes,
        "notes": "Pixel payload estimate only, excluding future art-index and alignment overhead. No firmware integration or hardware validation.",
        "device_tested": False,
    }
    (DESIGN / "validation.json").write_text(json.dumps(report, indent=2) + "\n")
    manifest_path.write_text(json.dumps(manifest, ensure_ascii=False, indent=2) + "\n")
    print(json.dumps(report))


if __name__ == "__main__":
    main()
