# 词库数据

`words.json` 由 `tools/build_wordlist.py` 生成，不要手改。原型用的 `prototype/assets/words_data.js` 是同一份数据的精简版。

| 学段 | 词数 | 来源 |
|---|---|---|
| 小学 | 700 | `tools/wordlists/primary.txt`：按人教版小学教材常见主题整理的近似词表，顺序即学习顺序（每 50 词一个单元）。**待用《义务教育英语课程标准》附录词表核对。** |
| 初中 | 900 | ECDICT 中考（zk）标签里其余的词 |
| 高中 | 1900 | 高考（gk）标签 |
| 四级 | 1000 | 四级（cet4）标签 |
| 六级 | 1500 | 六级（cet6）标签 |
| 进阶 | 1500 | 考研（ky），不够时用雅思（ielts）补 |

小学以外的学段都只取前面学段没用过的词，按词频从易到难排。

音标、词性、释义取自 ECDICT，清洗规则见 `build_wordlist.py` 的说明：音标从旧式英音记法转成课本用的 IPA；释义去掉专业义项、只取一个义项、≤ 12 字；
`tools/wordlists/meaning_overrides.tsv` 里是人工改过的释义（常用虚词和词典首义偏门的词）。

## 来源与许可

- ECDICT：https://github.com/skywind3000/ECDICT ，MIT 许可（Copyright (c) 2025 Linwei），许可全文在 `third_party/ecdict/LICENSE`。
- 使用的提交：`bc015ed2e24a7abef49fc6dbbb7fe32c1dadaf8b`；`ecdict.csv` 的 SHA256 记录在 `words.json` 的 `meta.ecdict_sha256`。
- `ecdict.csv`（66 MB）不入库。重新生成：

```sh
curl -sSfL -o third_party/ecdict/ecdict.csv https://raw.githubusercontent.com/skywind3000/ECDICT/bc015ed2e24a7abef49fc6dbbb7fe32c1dadaf8b/ecdict.csv
python3 -I tools/build_wordlist.py
```
