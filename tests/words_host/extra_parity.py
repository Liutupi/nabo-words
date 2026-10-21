"""原型 prototype/extra.js 和 C 规则 nabo_words_extra.c 对拍：分级测试、加学、闪电挑战逐题一致。
用法：python3 tests/words_host/extra_parity.py（需要 node）"""
import subprocess
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
SRC = HERE.parents[1] / "firmware" / "main" / "words"
BUILD = HERE / "build"


def main() -> int:
    BUILD.mkdir(exist_ok=True)
    db = BUILD / "words_db_host.c"
    subprocess.run([sys.executable, "-I", str(HERE / "gen_db.py"), str(db)], check=True, capture_output=True)
    exe = BUILD / "extra_trace"
    srcs = [str(p) for p in sorted(SRC.glob("nabo_words_*.c"))] + [str(db), str(HERE / "extra_trace.c")]
    subprocess.run(["cc", "-std=c11", "-O1", "-Wall", "-Werror", "-I", str(SRC), "-o", str(exe), *srcs], check=True)
    c = subprocess.run([str(exe)], capture_output=True, text=True, check=True).stdout.splitlines()
    js = subprocess.run(["node", str(HERE / "extra_trace.js")], capture_output=True, text=True, check=True).stdout.splitlines()
    for i, (a, b) in enumerate(zip(js, c)):
        if a != b:
            print(f"第 {i + 1} 行不一致\n  原型 {a}\n  C    {b}")
            return 1
    if len(js) != len(c):
        print(f"行数不一致：原型 {len(js)}，C {len(c)}")
        return 1
    print(f"分级测试 / 加学 / 闪电挑战 原型与 C 一致，共 {len(c)} 行")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
