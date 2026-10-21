"""从 ECDICT 生成全部学段的词库（7500 词）。

小学：tools/wordlists/primary.txt 的主题词表，按文件顺序取前 700 个在 ECDICT 里查得到的词。
其余学段：按 STAGE_PLAN 依次从对应考试标签里取还没用过的词，按词频从易到难排，补满该学段；
不够时用后面列的标签补。初中 = 中考，高中 = 高考，四级、六级同名标签，进阶 = 考研 / 雅思 / 托福。
释义：优先用 tools/wordlists/meaning_overrides.tsv；否则取词性占比最高的那一行、去掉专业义项，前两个义项，≤ 12 字。
音标：ECDICT 用俄文字母 ә 和撇号表示重音，统一换成 IPA。

输出 data/words.json（固件和原型共用）和 prototype/assets/words_data.js。
用法：python3 -I tools/build_wordlist.py [third_party/ecdict/ecdict.csv]
"""
from __future__ import annotations

import csv
import hashlib
import json
import logging
import re
import sys
from dataclasses import asdict, dataclass
from pathlib import Path
from typing import Dict, List, Optional, Tuple

logger = logging.getLogger(__name__)

ROOT = Path(__file__).resolve().parents[1]
ECDICT = ROOT / "third_party" / "ecdict" / "ecdict.csv"
PRIMARY_LIST = ROOT / "tools" / "wordlists" / "primary.txt"
OVERRIDES = ROOT / "tools" / "wordlists" / "meaning_overrides.tsv"
OUT_JSON = ROOT / "data" / "words.json"
OUT_JS = ROOT / "prototype" / "assets" / "words_data.js"

PRIMARY_N = 700
# (学段, 累计词数, 取词的标签按优先顺序)；累计词数和 prototype/words.js 的 STAGES 一致
STAGE_PLAN = [("junior", 1600, ("zk", "gk")), ("senior", 3500, ("gk", "cet4")), ("cet4", 4500, ("cet4", "cet6")),
              ("cet6", 6000, ("cet6", "ky")), ("adv", 7500, ("ky", "ielts", "toefl", "gre"))]
MAX_MEANING = 12
WORD_RE = re.compile(r"[A-Za-z]+(-[A-Za-z]+)?")
# ECDICT pos 字段的字母 → 释义行开头的词性标记
POS_TAGS = {"n": ("n.",), "v": ("v.", "vi.", "vt."), "j": ("a.", "adj."), "r": ("ad.", "adv."),
            "i": ("prep.",), "c": ("conj.",), "p": ("pron.",), "u": ("int.", "interj."), "m": ("num.",), "d": ("art.", "det.")}
POS_SHOW = {"a.": "adj.", "ad.": "adv.", "int.": "interj.", "pl.": "n.", "vt.": "v.", "vi.": "v."}
HINT_TAGS = {"v": ("v.", "vi.", "vt."), "adj": ("a.", "adj.")}   # 词表标题上的 [v] / [adj]
HINT_RE = re.compile(r"\[(v|adj)\]\s*$")
LINE_RE = re.compile(r"^([a-z]+\.)\s*(.*)$")


@dataclass(frozen=True)
class Word:
    w: str          # 单词
    p: str          # IPA 音标（不含斜杠）
    pos: str        # 主要词性
    m: str          # 中文释义
    stage: str      # primary / junior / senior / cet4 / cet6 / adv
    src: str        # list = 小学主题词表，其余是取词用的 ECDICT 标签


def load_ecdict(path: Path) -> Tuple[Dict[str, Dict[str, str]], Dict[str, Dict[str, str]]]:
    """返回（区分大小写的索引，小写索引）。小写索引里同一个词保留首个出现的条目。"""
    exact: Dict[str, Dict[str, str]] = {}
    lower: Dict[str, Dict[str, str]] = {}
    with path.open(encoding="utf-8", newline="") as f:
        for row in csv.DictReader(f):
            exact[row["word"]] = row
            lower.setdefault(row["word"].lower(), row)
    return exact, lower


def load_overrides(path: Path) -> Dict[str, Tuple[str, str]]:
    out: Dict[str, Tuple[str, str]] = {}
    for line in path.read_text(encoding="utf-8").splitlines():
        if not line.strip() or line.startswith("#"):
            continue
        word, pos, meaning = line.split("\t")
        out[word] = (pos, meaning)
    return out


# ECDICT 用的是旧式英音记法（ai、əu、短 i、əː），换成现在课本用的 IPA。先换双元音，再换单元音。
DIPHTHONGS = [("ai", "aɪ"), ("ei", "eɪ"), ("ɔi", "ɔɪ"), ("əu", "əʊ"), ("au", "aʊ"), ("iə", "ɪə"), ("uə", "ʊə")]
IPA_FALLBACK = {"thanks": "θæŋks", "fries": "fraɪz", "theatre": "ˈθɪətə", "website": "ˈwebsaɪt"}
IPA_FIX = {"kilometre": "ˈkɪləmiːtə"}                  # 源数据本身有错的


def ipa(raw: str, word: str = "") -> str:
    """ECDICT 音标 → IPA：ә→ə，' → ˈ，, → ˌ，: → ː，g → ɡ，旧式元音换成新式。"""
    raw = re.split(r"[;；]|[.,]?\s", raw.strip())[0]     # 多种读音只取第一种（'mɑːstə. 'mæstə、'sɔːrɪŋ 'sɔər-）
    raw = re.sub(r"\(.*?\)", "", raw)                  # 去掉 (r) 这类可选音
    s = raw.strip().strip("/[] ").replace("ә", "ə").replace("є", "e").replace("ε", "e").replace(".", "ˌ").replace("^", "ɡ").replace("'", "ˈ").replace(",", "ˌ").replace(":", "ː").replace("g", "ɡ").replace("ɒː", "ɒ")
    if word.lower() in IPA_FIX:
        return IPA_FIX[word.lower()]
    if not s:
        return IPA_FALLBACK.get(word.lower(), "")
    marks = {}
    for k, (old, new) in enumerate(DIPHTHONGS):
        token = chr(0xE000 + k)                       # 先用占位符，免得后面的单元音规则改到双元音里
        marks[token] = new
        s = s.replace(old, token)
    s = re.sub(r"\\+ː", "ɜː", s)                         # 个别条目把 ɜ 存成了反斜杠
    s = s.replace("əː", "ɜː")
    s = re.sub(r"i(?!ː)(?=.)", "ɪ", s)                 # 短 i → ɪ，词尾的 i（happy）保留
    s = re.sub(r"u(?!ː)", "ʊ", s)
    for token, new in marks.items():
        s = s.replace(token, new)
    return s


def _freq(row: Dict[str, str]) -> int:
    """词频排名，越小越常用；没有数据的排到最后。"""
    for key in ("frq", "bnc"):
        v = int(row.get(key) or 0)
        if v > 0:
            return v
    return 10 ** 6


def _dominant_tags(pos_field: str) -> List[str]:
    """pos 字段形如 'n:60/v:40'，按占比从高到低给出对应的词性标记。"""
    parts = []
    for item in pos_field.split("/"):
        if ":" in item:
            k, v = item.split(":", 1)
            parts.append((int(v or 0), k))
    tags: List[str] = []
    for _, k in sorted(parts, reverse=True):
        tags.extend(POS_TAGS.get(k, ()))
    return tags


def _clean_senses(text: str) -> List[str]:
    text = re.sub(r"[（(][^）)]*[）)]|\[[^\]]*\]", "", text).replace("...", "…")
    senses = [re.sub(r"[^\u4e00-\u9fff…]", "", s) for s in re.split(r"[,，;；]", text)]
    return [s for s in senses if s and s != "…"]


def meaning(row: Dict[str, str], hint: str = "") -> Tuple[str, str]:
    """选词性：词表提示 > pos 字段占比 > 第一行（ECDICT 的行序基本按常用程度）；只取第一个义项（第二个常是引申义，如 school：鱼群）。返回（词性，释义）。"""
    lines = []
    text = row["translation"].replace("\\n", "\n").replace("\r", "")   # 部分条目把换行存成字面的 \n
    for raw in text.split("\n"):
        raw = raw.strip()
        if not raw or raw.startswith("["):          # [计] [医] 等专业义项
            continue
        m = LINE_RE.match(raw)
        tag, body = (m.group(1), m.group(2)) if m else ("", raw)
        senses = _clean_senses(body)
        if senses:
            lines.append((tag, senses))
    if not lines:
        return "", ""
    pick = lines[0]
    rich = max(lines, key=lambda ln: len(ln[1]))
    if len(pick[1]) == 1 and len(rich[1]) >= 3:        # 第一行只有一个偏门义项（secular：修道院外的教士）
        pick = rich
    for want in list(HINT_TAGS.get(hint, ())) + _dominant_tags(row.get("pos", "")):
        hit = next((ln for ln in lines if ln[0] == want), None)
        if hit:
            pick = hit
            break
    tag, senses = pick
    return POS_SHOW.get(tag, tag), senses[0][:MAX_MEANING]


def lookup(word: str, exact: Dict, lower: Dict) -> Optional[Dict[str, str]]:
    return exact.get(word) or lower.get(word.lower())


def make(word: str, row: Dict[str, str], stage: str, src: str, overrides: Dict[str, Tuple[str, str]], hint: str = "") -> Optional[Word]:
    pos, m = overrides.get(word) or overrides.get(row["word"]) or meaning(row, hint)
    if not m:
        logger.warning("%s 没有可用的中文释义，跳过", word)
        return None
    return Word(word, ipa(row["phonetic"], word), pos, m, stage, src)


def build(ecdict_path: Path) -> List[Word]:
    exact, lower = load_ecdict(ecdict_path)
    overrides = load_overrides(OVERRIDES)
    words: List[Word] = []
    seen = set()
    missing = []
    hint = ""
    for line in PRIMARY_LIST.read_text(encoding="utf-8").splitlines():
        if line.startswith("#"):
            m = HINT_RE.search(line)
            if line.startswith("# ") and not line.startswith("# 小学") and not line.startswith("# 待") and not line.startswith("# 超过"):
                hint = m.group(1) if m else ""
            continue
        for w in line.split():
            if w.lower() in seen:
                continue
            row = lookup(w, exact, lower)
            if row is None:
                missing.append(w)
                continue
            if len(words) >= PRIMARY_N:
                continue                              # 多出来的留给初中
            item = make(w, row, "primary", "list", overrides, hint)
            if item:
                words.append(item)
                seen.add(w.lower())
    if missing:
        logger.warning("小学词表里 ECDICT 查不到：%s", " ".join(missing))
    if len(words) < PRIMARY_N:
        raise ValueError(f"小学只有 {len(words)} 个词，不够 {PRIMARY_N}，请补 primary.txt")
    for stage, end, tags in STAGE_PLAN:
        for tag in tags:
            if len(words) >= end:
                break
            pool = [r for r in exact.values() if tag in r["tag"].split() and WORD_RE.fullmatch(r["word"])
                    and r["word"].lower() not in seen]
            for row in sorted(pool, key=lambda r: (_freq(r), r["word"])):
                if len(words) >= end:
                    break
                item = make(row["word"], row, stage, tag, overrides)
                if item:
                    words.append(item)
                    seen.add(row["word"].lower())
        if len(words) != end:
            raise ValueError(f"{stage} 只凑到 {len(words)} 个词，应为 {end}")
    return words


def sha256(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(1 << 20), b""):
            h.update(chunk)
    return h.hexdigest()


def main() -> None:
    logging.basicConfig(level=logging.INFO, format="%(message)s")
    src = Path(sys.argv[1]) if len(sys.argv) > 1 else ECDICT
    words = build(src)
    meta = {"source": "ECDICT (MIT) https://github.com/skywind3000/ECDICT", "ecdict_sha256": sha256(src),
            "stage_ends": {"primary": PRIMARY_N, **{st: end for st, end, _ in STAGE_PLAN}}, "count": len(words)}
    OUT_JSON.parent.mkdir(parents=True, exist_ok=True)
    OUT_JSON.write_text(json.dumps({"meta": meta, "words": [asdict(w) for w in words]}, ensure_ascii=False, indent=0), encoding="utf-8")
    slim = [[w.w, w.p, w.pos, w.m] for w in words]
    OUT_JS.write_text("// 由 tools/build_wordlist.py 从 ECDICT（MIT）生成，不要手改。[单词, 音标, 词性, 释义]\n"
                      "window.NABO_WORDS = " + json.dumps(slim, ensure_ascii=False, separators=(",", ":")) + ";\n", encoding="utf-8")
    src_count = {}
    for w in words:
        src_count[w.src] = src_count.get(w.src, 0) + 1
    logger.info("共 %d 词：%s；写入 %s、%s", len(words), src_count, OUT_JSON.relative_to(ROOT), OUT_JS.relative_to(ROOT))


if __name__ == "__main__":
    main()
