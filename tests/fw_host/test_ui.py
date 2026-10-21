"""Compile the production UI, verify redraw/layout/animation, and export actual renderer previews.

python tests/fw_host/test_ui.py --assets firmware/assets/assets.bin --out build/ui
CC may be a compiler path or command (including 'zig cc'). --source-root can check an earlier checkout.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import shlex
import shutil
import subprocess

import numpy as np
from PIL import Image

ROOT = Path(__file__).resolve().parents[2]


def compiler() -> list[str]:
    value = os.environ.get("CC", "cc")
    command = [value] if Path(value).is_file() else shlex.split(value)
    if Path(command[0]).stem == "zig" and len(command) == 1:
        command.append("cc")
    if not Path(command[0]).is_file() and not shutil.which(command[0]):
        raise SystemExit("No host C compiler. Set CC to gcc, clang, or a portable zig executable.")
    return command


def read_frame(path: Path) -> Image.Image:
    pixels = np.frombuffer(path.read_bytes(), dtype="<u2").reshape(320, 240).astype(np.uint32)
    rgb = np.stack((((pixels >> 11) & 31) * 255 // 31,
                    ((pixels >> 5) & 63) * 255 // 63, (pixels & 31) * 255 // 31), axis=-1)
    return Image.fromarray(rgb.astype(np.uint8))


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--assets", type=Path, required=True)
    ap.add_argument("--out", type=Path, default=ROOT / "build/ui")
    ap.add_argument("--source-root", type=Path, default=ROOT)
    ap.add_argument("--baseline", action="store_true")
    args = ap.parse_args()
    out, assets = args.out.resolve(), args.assets.resolve()
    out.mkdir(parents=True, exist_ok=True)
    main_dir = args.source_root.resolve() / "firmware/main"
    exe = out / ("ui-probe.exe" if os.name == "nt" else "ui-probe")
    sources = [ROOT / "tests/fw_host/ui_probe.c", *sorted(main_dir.glob("nw_*.c")),
               *sorted((main_dir / "words").glob("*.c"))]
    subprocess.run([*compiler(), "-O2", "-std=gnu11", "-Wall", "-Werror", f"-I{main_dir}",
                    "-o", str(exe), *map(str, sources)], check=True)
    command = [str(exe), str(assets)] + (["--baseline"] if args.baseline else [])
    result = subprocess.run(command, cwd=out, capture_output=True, text=True)
    (out / "checks.txt").write_text(result.stdout + result.stderr, encoding="utf-8")
    print(result.stdout, result.stderr, flush=True)
    for raw in out.glob("*.rgb"):
        if not raw.stem.startswith("motion_"):
            read_frame(raw).save(raw.with_suffix(".png"))
    motion = [read_frame(p) for p in sorted(out.glob("motion_*.rgb"))]
    if motion:
        # GIF's centisecond clock: 80+80+90 ms repeats, keeping a 12 fps average.
        durations = [80 if i % 3 != 2 else 90 for i in range(len(motion))]
        motion[0].save(out / "idle-preview.gif", save_all=True, append_images=motion[1:],
                       duration=durations, loop=0, disposal=2)
    report = {"source_root": str(args.source_root.resolve()), "assets_sha256": hashlib.sha256(assets.read_bytes()).hexdigest(),
              "baseline": args.baseline, "passed": result.returncode == 0 and not args.baseline,
              "output": result.stdout.strip(), "device_tested": False}
    (out / "verification.json").write_text(json.dumps(report, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    return result.returncode


if __name__ == "__main__":
    raise SystemExit(main())
