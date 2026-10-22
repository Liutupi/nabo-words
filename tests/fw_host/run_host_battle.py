"""两个宿主进程通过本机 UDP 模拟两台工牌，完整打一局对战，核对两边比分互为相反、积分一增一减。
需要先运行 firmware/tools/pack_assets.py。用法：python3 tests/fw_host/run_host_battle.py"""
import os
import re
from concurrent.futures import ThreadPoolExecutor
import subprocess
import sys
import tempfile
from pathlib import Path
from host_compile import build_host

ROOT = Path(__file__).resolve().parents[2]
MAIN = ROOT / "firmware" / "main"
ONBOARD = "key ok\nkey ok\nkey down\nkey ok\n"   # 家乡选北京，从小学开始


def script(host: bool) -> str:
    s = ONBOARD + f"cmd sim {20 if host else 30}\ncmd vs\n"
    if host:
        s += "wait 1500\nkey ok\nwait 6500\n" + "wait 800\nkey ok\nwait 3600\n" * 10
    else:
        s += "wait 3000\nkey ok\nwait 3500\n" + "wait 1300\nkey down\nkey ok\nwait 3100\n" * 10
    s += "wait 4000\ncmd vstat\n"
    # 再来一局：挑战方在结算页按下键，接受方按 OK
    if host:
        s += "wait 500\ncmd vstat\nkey down\ncmd vstat\nwait 6500\ncmd vstat\n" + "wait 800\nkey ok\nwait 3600\n" * 10
    else:
        s += "wait 8000\ncmd vstat\nkey ok\ncmd vstat\nwait 5000\ncmd vstat\n" + "wait 1300\nkey down\nkey ok\nwait 3100\n" * 10
    return s + "wait 12000\ncmd vstat\n"


def main() -> int:
    out = Path(tempfile.mkdtemp())
    exe = build_host(out)
    assets = str(ROOT / "firmware/assets/assets.bin")
    env = lambda me, peer: {**os.environ, "NW_REALTIME": "1", "NW_PORT": str(me), "NW_PEER": str(peer)}
    a = subprocess.Popen([str(exe), assets], stdin=subprocess.PIPE, stdout=subprocess.PIPE, text=True, env=env(41001, 41002))
    b = subprocess.Popen([str(exe), assets], stdin=subprocess.PIPE, stdout=subprocess.PIPE, text=True, env=env(41002, 41001))
    try:
        with ThreadPoolExecutor(max_workers=2) as pool:
            runs = [pool.submit(proc.communicate, script(host), timeout=300)
                    for proc, host in ((a, True), (b, False))]
            oa, ob = [run.result()[0] for run in runs]
    finally:
        for proc in (a, b):
            if proc.poll() is None:
                proc.kill()
            proc.wait()
    pat = re.compile(r"phase=(\d+) .*score=(\d+):(\d+) counted=(\d) .*rating=(-?\d+)")
    if "-v" in sys.argv: print(oa[-700:], "\n----\n", ob[-700:])
    ga = [m for m in pat.findall(oa) if m[0] == "8"]
    gb = [m for m in pat.findall(ob) if m[0] == "8"]
    if a.returncode or b.returncode or len(ga) < 2 or len(gb) < 2:
        print("对战未完成两局\n", oa, ob)
        return 1
    ok = len(ga) >= 2 and len(gb) >= 2 and ga[0][4] != ga[-1][4]   # 两局都打完，第二局积分又变了
    for game in (0, -1):
        ra, rb = ga[game], gb[game]
        print("第 1 局" if game == 0 else "再来一局", " 挑战方", ra, " 接受方", rb)
        ok &= ra[1:3] == rb[2:0:-1] and ra[3] == rb[3] == "1" and int(ra[4]) + int(rb[4]) == 2000
    print("对战联机测试", "通过" if ok else "失败")
    return 0 if ok else 1


if __name__ == "__main__":
    raise SystemExit(main())
