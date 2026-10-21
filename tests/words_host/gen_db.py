"""把 data/words.json 生成宿主测试用的词库实现（nabo_words_db.h 的三个函数）。

固件里词库从 assets 分区读；这里直接编成 C 数组，小写拼写排好序后二分查找。
用法：python3 tests/words_host/gen_db.py <输出 .c>
"""
from __future__ import annotations

import json
import logging
import sys
from pathlib import Path
from typing import List

logger = logging.getLogger(__name__)

ROOT = Path(__file__).resolve().parents[2]


def c_str(s: str) -> str:
    """C 字符串字面量；UTF-8 原样写进源文件，只转义引号和反斜杠。"""
    return '"' + s.replace("\\", "\\\\").replace('"', '\\"') + '"'


def render(words: List[dict]) -> str:
    lower = sorted({w["w"].lower() for w in words})
    lines = [
        "/* 由 tests/words_host/gen_db.py 从 data/words.json 生成，不要手改 */",
        '#include <string.h>',
        '#include "nabo_words_db.h"',
        "",
        f"static const char *const WORD[{len(words)}] = {{",
        *[f"    {c_str(w['w'])}," for w in words],
        "};",
        f"static const char *const MEANING[{len(words)}] = {{",
        *[f"    {c_str(w['m'])}," for w in words],
        "};",
        f"static const char *const LOWER[{len(lower)}] = {{",
        *[f"    {c_str(w)}," for w in lower],
        "};",
        "",
        "const char *nw_db_word(uint16_t idx) { return WORD[idx]; }",
        "const char *nw_db_meaning(uint16_t idx) { return MEANING[idx]; }",
        "bool nw_db_has_lower(const char *lower)",
        "{",
        f"    int lo = 0, hi = {len(lower)} - 1;",
        "    while (lo <= hi) {",
        "        int mid = (lo + hi) / 2, c = strcmp(LOWER[mid], lower);",
        "        if (c == 0) return true;",
        "        if (c < 0) lo = mid + 1; else hi = mid - 1;",
        "    }",
        "    return false;",
        "}",
        "",
    ]
    return "\n".join(lines)


def main() -> None:
    logging.basicConfig(level=logging.INFO, format="%(message)s")
    words = json.loads((ROOT / "data" / "words.json").read_text(encoding="utf-8"))["words"]
    out = Path(sys.argv[1])
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_text(render(words), encoding="utf-8")
    logger.info("词库 %d 词 → %s", len(words), out)


if __name__ == "__main__":
    main()
