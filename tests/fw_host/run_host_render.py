"""编译固件游戏层的宿主版本，按脚本走一遍主要画面并截图。
每张截图都核对增量重画和整屏重画逐像素一致、绘制列表没有溢出。
需要先运行 firmware/tools/pack_assets.py 生成 firmware/assets/assets.bin。
用法：python3 tests/fw_host/run_host_render.py [输出目录]"""
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
MAIN = ROOT / "firmware" / "main"
SCRIPT = """wait 1500
shot home.rgb
key ok
shot intro.rgb
key ok
key ok
key ok
key ok
key ok
shot quiz.rgb
key ok
wait 900
key back
key down
key down
key ok
shot stars.rgb
key ok
key down
key ok
shot passport.rgb
key back
cmd sim 400 90
wait 2500
shot home_cet4.rgb
cmd coins 300
key down
key down
key down
key down
key down
key ok
key down
key down
key down
key ok
shot shop.rgb
key back
key down
key ok
shot desk.rgb
"""


def main() -> int:
    out = Path(sys.argv[1] if len(sys.argv) > 1 else tempfile.mkdtemp())
    out.mkdir(parents=True, exist_ok=True)
    exe = out / "host"
    srcs = [str(ROOT / "tests/fw_host/host_main.c")] + [str(p) for p in sorted(MAIN.glob("nw_*.c"))] + \
           [str(p) for p in sorted((MAIN / "words").glob("*.c"))]
    subprocess.run(["cc", "-O1", "-std=gnu11", "-Wall", "-Werror", f"-I{MAIN}", "-o", str(exe), *srcs], check=True)
    r = subprocess.run([str(exe), str(ROOT / "firmware/assets/assets.bin")], input=SCRIPT, text=True,
                       capture_output=True, cwd=out)
    print(r.stdout, r.stderr)
    shots = [l.split()[1] for l in r.stdout.splitlines() if l.startswith("shot ")]
    subprocess.run([sys.executable, str(ROOT / "tests/fw_host/to_png.py"), str(out / "screens.png"),
                    *[str(out / s) for s in shots]], check=True)
    print("截图：", out / "screens.png")
    return r.returncode


if __name__ == "__main__":
    raise SystemExit(main())
