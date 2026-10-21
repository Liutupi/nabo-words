"""两个宿主进程通过本机 UDP 模拟两台工牌，完整打一局对战，核对两边比分互为相反、积分一增一减。
需要先运行 firmware/tools/pack_assets.py。用法：python3 tests/fw_host/run_host_battle.py"""
import re
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
MAIN = ROOT / "firmware" / "main"
ONBOARD = "key ok\nkey ok\nkey down\nkey ok\n"   # 家乡选北京，从小学开始


def script(host: bool) -> str:
    s = ONBOARD + f"cmd sim {20 if host else 30}\ncmd vs\n"
    if host:
        s += "wait 1500\nkey ok\nwait 6500\n" + "wait 800\nkey ok\nwait 3600\n" * 10
    else:
        s += "wait 3000\nkey ok\nwait 3500\n" + "wait 1300\nkey down\nkey ok\nwait 3100\n" * 10
    return s + "wait 4000\ncmd vstat\n"


def main() -> int:
    out = Path(tempfile.mkdtemp())
    exe = out / "host"
    srcs = [str(ROOT / "tests/fw_host/host_main.c")] + [str(p) for p in sorted(MAIN.glob("nw_*.c"))] + \
           [str(p) for p in sorted((MAIN / "words").glob("*.c"))]
    subprocess.run(["cc", "-O1", "-std=gnu11", "-Wall", f"-I{MAIN}", "-o", str(exe), *srcs], check=True)
    assets = str(ROOT / "firmware/assets/assets.bin")
    env = lambda me, peer: {"NW_REALTIME": "1", "NW_PORT": str(me), "NW_PEER": str(peer), "PATH": "/usr/bin:/bin"}
    a = subprocess.Popen([str(exe), assets], stdin=subprocess.PIPE, stdout=subprocess.PIPE, text=True, env=env(41001, 41002))
    b = subprocess.Popen([str(exe), assets], stdin=subprocess.PIPE, stdout=subprocess.PIPE, text=True, env=env(41002, 41001))
    for proc, host in ((a, True), (b, False)):   # 两边同时开始
        proc.stdin.write(script(host))
        proc.stdin.close()
    oa, ob = a.stdout.read(), b.stdout.read()
    a.wait()
    b.wait()
    pat = re.compile(r"phase=(\d+) .*score=(\d+):(\d+) counted=(\d) .*rating=(-?\d+)")
    ra, rb = pat.findall(oa)[-1], pat.findall(ob)[-1]
    print("挑战方", ra, "\n接受方", rb)
    ok = ra[0] == rb[0] == "8" and ra[1:3] == rb[2:0:-1] and ra[3] == rb[3] == "1" and int(ra[4]) + int(rb[4]) == 2000
    print("对战联机测试", "通过" if ok else "失败")
    return 0 if ok else 1


if __name__ == "__main__":
    raise SystemExit(main())
