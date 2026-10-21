"""编译并运行 test_extra.c（分级测试、加学、闪电挑战的规则用例）。用法：python3 tests/words_host/test_extra.py"""
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
    exe = BUILD / "test_extra"
    srcs = [str(p) for p in sorted(SRC.glob("nabo_words_*.c"))] + [str(db), str(HERE / "test_extra.c")]
    subprocess.run(["cc", "-std=c11", "-O1", "-Wall", "-Wextra", "-Werror", "-Wno-unused-parameter", "-I", str(SRC),
                    "-o", str(exe), *srcs], check=True)
    return subprocess.run([str(exe)]).returncode


if __name__ == "__main__":
    raise SystemExit(main())
