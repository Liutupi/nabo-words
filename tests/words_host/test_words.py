"""《NABO 单词护照》逻辑模块对拍测试：把原型录下的轨迹逐天在 C 上重放，每一步都比对。

先编译 firmware/main/words/*.c + 生成的词库 + shim.c 成共享库，再用 ctypes 调用。
轨迹由 tests/words_host/record_golden.js 从 prototype/ 录制；改了原型规则要重录。
用法：python3 tests/words_host/test_words.py
"""
from __future__ import annotations

import ctypes as C
import json
import logging
import subprocess
import sys
from pathlib import Path
from typing import Dict, List, Sequence

logger = logging.getLogger(__name__)

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]
SRC = ROOT / "firmware" / "main" / "words"
BUILD = HERE / "build"
GOLDEN = HERE / "golden"
DIGEST_KEYS = ["day", "next", "stage", "streak", "best", "freezes", "coins", "spent", "done", "rating", "wins",
               "losses", "ws", "accs", "acc", "ach", "exam1", "mastered", "learned", "stamps", "form", "examReady", "hash"]
I32 = C.c_int32
U16 = C.c_uint16


class Mismatch(AssertionError):
    pass


def build_lib() -> C.CDLL:
    BUILD.mkdir(exist_ok=True)
    db = BUILD / "words_db_host.c"
    subprocess.run([sys.executable, "-I", str(HERE / "gen_db.py"), str(db)], check=True, capture_output=True)
    lib = BUILD / "libnabo_words.so"
    srcs = [str(p) for p in sorted(SRC.glob("nabo_words_*.c"))] + [str(db), str(HERE / "shim.c")]
    cmd = ["cc", "-std=c11", "-O1", "-g", "-Wall", "-Wextra", "-Werror", "-Wno-unused-parameter",
           "-shared", "-fPIC", "-I", str(SRC), "-o", str(lib), *srcs]
    subprocess.run(cmd, check=True)
    so = C.CDLL(str(lib))
    so.t_new.restype = C.c_void_p
    for name in ("t_free", "t_digest", "t_plan", "t_grade", "t_introduce", "t_finish_day", "t_advance_day",
                 "t_session_build", "t_session_intro", "t_session_answer", "t_session_finish", "t_open_toy",
                 "t_buy_freeze", "t_use_acc", "t_battle", "t_battle_finish"):
        getattr(so, name).argtypes = None
    so.t_session_build.argtypes = [C.c_void_p, C.c_uint32, C.c_int]
    so.t_battle.argtypes = [C.c_void_p, C.c_char_p, C.c_uint32, C.POINTER(I32)]
    so.t_make_q.argtypes = [C.c_int, C.c_int, C.c_uint32, C.POINTER(I32)]
    so.t_shop_seed.argtypes = [C.c_uint32]
    so.t_session_seed.restype = C.c_uint32
    return so


class Replay:
    """一条轨迹的 C 端重放。"""

    def __init__(self, so: C.CDLL, words: List[dict], name: str) -> None:
        self.so, self.words, self.name = so, words, name
        self.steps = 0

    def check(self, what: str, got, want) -> None:
        self.steps += 1
        if got != want:
            raise Mismatch(f"{self.name} 第 {self.day} 天 {what}：C = {got!r}，原型 = {want!r}")

    # ---- 题目 → 和原型一样的可比形式 ----
    def q_text(self, raw: Sequence[int]) -> list:
        qtype, idx, flags, n_opt = raw[0], raw[1], raw[2], raw[3]
        opts = list(raw[4:4 + n_opt])
        text = lambda j, k: self.words[j][k] if 0 <= j < len(self.words) else f"<非法序号 {j}>"  # noqa: E731
        if qtype == 1:
            opts = [text(j, "m") for j in opts]
        elif qtype == 2:
            opts = [text(j, "w") for j in opts]
        elif qtype == 3:
            opts = [chr(c) for c in opts]
        else:
            opts = []
        return [qtype, idx, flags, opts, raw[7] if n_opt else 0, raw[8]]

    def queue(self, start: int, n: int) -> list:
        buf = (I32 * 9)()
        out = []
        for i in range(start, n):
            self.so.t_session_q(i, buf)
            out.append(self.q_text(list(buf)))
        return out

    @staticmethod
    def norm(q: list) -> list:
        """原型里新词卡 / 闪卡没有 answer、pos，记成 0。"""
        return [q[0], q[1], q[2], q[3], q[4] if q[3] else 0, q[5]]

    def run_session(self, s: int, rec: dict, exam: bool) -> None:
        self.check("学习种子", self.so.t_session_seed(C.c_void_p(s)), rec["seed"])
        n = self.so.t_session_build(s, rec["seed"], 1 if exam else 0)
        self.check("学习队列", self.queue(0, n), [self.norm(q) for q in rec["init"]])
        answers = iter(rec["ans"])
        buf = (I32 * 9)()
        while self.so.t_session_pos() < self.so.t_session_len():
            self.so.t_session_q(self.so.t_session_pos(), buf)
            if buf[0] == 0:
                self.so.t_session_intro(C.c_void_p(s))
            else:
                self.so.t_session_answer(C.c_void_p(s), next(answers))
            self.so.t_session_next()
        self.check("做完后的队列（含重问）", self.queue(0, self.so.t_session_len()), [self.norm(q) for q in rec["final"]])
        out = (I32 * 4)()
        self.so.t_session_finish(C.c_void_p(s), out)
        self.check("答对 / 答错", (out[0], out[1]), (rec["right"], rec["wrong"]))
        if exam:
            self.check("结业考是否通过", bool(out[2]), rec["passed"])
        else:
            self.check("新盖的章", out[3], rec["stamps"])

    def digest(self, s: int) -> Dict[str, int]:
        v, toys = (I32 * len(DIGEST_KEYS))(), (I32 * 60)()
        n = self.so.t_digest(C.c_void_p(s), v, toys)
        d = dict(zip(DIGEST_KEYS, list(v)))
        d["hash"] &= 0xFFFFFFFF
        d["examReady"] = bool(d["examReady"])
        d["toys"] = list(toys)[:n]
        return d

    def run(self, trace: dict) -> int:
        cfg = trace["cfg"]
        so = self.so
        s = so.t_new(cfg["newPer"])
        so.t_shop_seed(cfg["seed"] ^ 0x5A5A)
        try:
            for rec in trace["days"]:
                self.day = rec["d"]
                if rec["mode"] == "sim":
                    revs, fresh, meta = (U16 * 256)(), (U16 * 20)(), (I32 * 4)()
                    so.t_plan(C.c_void_p(s), revs, fresh, meta)
                    p = rec["plan"]
                    self.check("今日题单", (list(revs)[:meta[0]], list(fresh)[:meta[1]], meta[2], meta[3]),
                               (p["r"], p["f"], p["b"], p["s"]))
                    for i, ok in zip(p["r"], rec["ans"]):
                        so.t_grade(C.c_void_p(s), i, ok)
                    for i in p["f"]:
                        so.t_introduce(C.c_void_p(s), i)
                    so.t_finish_day(C.c_void_p(s), 1 if rec["perfect"] else 0)
                elif rec["mode"] == "session":
                    self.run_session(s, rec["sess"], exam=False)
                if "exam" in rec:
                    self.run_session(s, rec["exam"], exam=True)
                for op, arg, ok in rec["shop"]:
                    got = so.t_open_toy(C.c_void_p(s)) if op == 3 else so.t_buy_freeze(C.c_void_p(s)) if op == 4 \
                        else so.t_use_acc(C.c_void_p(s), arg)
                    self.check(f"小卖部操作 {op}", 1 if got >= 0 else 0, ok)
                for vs in rec["vs"]:
                    self.battle(s, vs)
                self.check("翻到下一天", so.t_advance_day(C.c_void_p(s)), rec["adv"])
                self.check("状态摘要", self.digest(s), {k: rec["dg"][k] for k in DIGEST_KEYS + ["toys"]})
        finally:
            so.t_free(C.c_void_p(s))
        return self.steps

    def battle(self, s: int, vs: dict) -> None:
        qs = (I32 * 90)()
        n = self.so.t_battle(C.c_void_p(s), bytes.fromhex(vs["bits"]), vs["seed"], qs)
        self.check("对战题库大小", n, vs["pool"])
        got = [self.q_text(list(qs)[9 * k:9 * k + 9]) for k in range(len(vs["qs"]))]
        self.check("对战题目", got, [self.norm(q) for q in vs["qs"]])
        missed = (U16 * 10)(*vs["missed"])
        out = (I32 * 3)()
        self.so.t_battle_finish(C.c_void_p(s), vs["peer"], vs["peerRating"], vs["my"], vs["their"], missed,
                                len(vs["missed"]), out)
        self.check("对战结算", (out[0], out[1], out[2]), (vs["result"], vs["delta"], vs["farm"]))


def run_math(so: C.CDLL, words: List[dict]) -> int:
    t = json.loads((GOLDEN / "math.json").read_text())
    steps = 0
    for me, op, res, exp, delta in t["elo"]:
        got = (so.nw_elo_expected(me, op), so.nw_elo_delta(me, op, res))
        if got != (exp, delta):
            raise Mismatch(f"Elo {me} vs {op} 结果 {res}：C = {got}，原型 = {(exp, delta)}")
        steps += 1
    for c, ms, last, p in t["pts"]:
        if so.nw_battle_points(c, ms, last) != p:
            raise Mismatch(f"计分 correct={c} ms={ms} last={last}")
        steps += 1
    for rt, r in t["rank"]:
        if so.nw_rank_of(rt) != r:
            raise Mismatch(f"段位 {rt}")
        steps += 1
    for n, n_rev, n_fresh, backlog, seconds in t["plans"]:
        s = so.t_new(10)
        for i in range(n):
            so.t_introduce(C.c_void_p(s), i)
        so.t_advance_day(C.c_void_p(s))
        revs, fresh, meta = (U16 * 256)(), (U16 * 20)(), (I32 * 4)()
        so.t_plan(C.c_void_p(s), revs, fresh, meta)
        so.t_free(C.c_void_p(s))
        if list(meta) != [n_rev, n_fresh, backlog, seconds]:
            raise Mismatch(f"积压 {n} 个词的题单：C = {list(meta)}，原型 = {[n_rev, n_fresh, backlog, seconds]}")
        steps += 1
    for k, ready, form in t["exams"]:
        s = so.t_new(10)
        for i in range(700):
            so.t_introduce(C.c_void_p(s), i)
            if i < k:
                so.t_set_box(C.c_void_p(s), i, 3)
        got = (so.t_exam_ready(C.c_void_p(s)), so.t_sprite_form(C.c_void_p(s), 0))
        so.t_free(C.c_void_p(s))
        if got != (ready, form):
            raise Mismatch(f"熟悉 {k} 个词时能否结业考 / 精灵形态：C = {got}，原型 = {(ready, form)}")
        steps += 1
    rp = Replay(so, words, "math")
    rp.day = 0
    buf = (I32 * 9)()
    for seed, q in t["qs"]:
        so.t_make_q(q[0], q[1], seed, buf)
        rp.check(f"出题 {q[1]}", rp.q_text(list(buf)), Replay.norm(q))
    return steps + rp.steps


def rule_cases(so: C.CDLL) -> int:
    """原型轨迹覆盖不到的边界规则。"""
    s = so.t_new(10)
    d = lambda: dict(zip(DIGEST_KEYS, list(_dig(so, s))))  # noqa: E731
    try:
        so.t_introduce(C.c_void_p(s), 0)
        for _ in range(6):
            so.t_grade(C.c_void_p(s), 0, 1)                 # 1 → 7 毕业
        assert d()["mastered"] == 1
        so.t_grade(C.c_void_p(s), 0, 0)                     # 毕业词答错退到 6
        assert so.t_buy_freeze(C.c_void_p(s)) == -1          # 没钱
        assert so.t_open_toy(C.c_void_p(s)) == -1
        assert so.t_use_acc(C.c_void_p(s), 3) == -1
        so.t_finish_day(C.c_void_p(s), 1)
        so.t_finish_day(C.c_void_p(s), 1)                   # 同一天第二次不再加金币
        assert d()["coins"] == 3, d()
        assert so.t_advance_day(C.c_void_p(s)) == 0
        assert so.t_advance_day(C.c_void_p(s)) == 2          # 没学、没补签卡 → 断签
        assert so.t_exam_passes(15) == 0 and so.t_exam_passes(16) == 1   # 20 题答对 16 题及格
    finally:
        so.t_free(C.c_void_p(s))
    return 9


def _dig(so: C.CDLL, s: int):
    v, toys = (I32 * len(DIGEST_KEYS))(), (I32 * 60)()
    so.t_digest(C.c_void_p(s), v, toys)
    return v


def main() -> int:
    logging.basicConfig(level=logging.INFO, format="%(message)s")
    so = build_lib()
    words = json.loads((ROOT / "data" / "words.json").read_text(encoding="utf-8"))["words"]
    total = 0
    try:
        for path in sorted(GOLDEN.glob("[a-z]*.json")):
            if path.stem == "math":
                continue
            trace = json.loads(path.read_text())
            steps = Replay(so, words, trace["name"]).run(trace)
            logger.info("%-7s %4d 天，%6d 步一致", trace["name"], len(trace["days"]), steps)
            total += steps
        n = run_math(so, words)
        logger.info("math    Elo / 计分 / 段位 / 出题 %d 项一致", n)
        n2 = rule_cases(so)
        logger.info("规则用例 %d 项通过", n2)
        total += n + n2
    except Mismatch as e:
        logger.error("不一致：%s", e)
        return 1
    logger.info("全部通过，共 %d 步", total)
    return 0


if __name__ == "__main__":
    sys.exit(main())
