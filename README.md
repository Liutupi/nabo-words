# NABO 单词护照

AI Passport 工牌（ESP32-C3，240×320，三键）上的英语单词养成游戏。NABO 坐在书桌后陪你背单词：从小学一路学到考研 / 雅思，共 7500 词，学段一升 NABO 就换一套衣服，书桌上的道具越来越多，单词精灵从蛋孵化长大；护照集章、词汇星图、连续打卡、小卖部，两台工牌还能面对面用 ESP-NOW 对战。

计划做成只装这一个游戏的离线固件（不带小智）。完整设计见 [DESIGN.md](DESIGN.md)。

## 现在做到哪里

| 阶段 | 状态 |
|---|---|
| M0 设计与浏览器原型 | ✅ `prototype/`，7 个学段的 NABO 动画、全部精灵 / 道具 / 摆件 / 饰品已接入 |
| M1 美术 | ✅ 基本完成（小披风待重画），流程见 `ART_BRIEF.md`、`CODEX_PROMPTS.md`、`art/reference/SHOT_LIST.md` |
| 词库 | ✅ 7500 词（ECDICT），小学词表待用课标附录核对 |
| M2 纯逻辑模块（C） | ✅ `firmware/main/words/`，和原型逐步对拍一致 |
| M3 渲染 / 独立固件工程 | 未开始 |

## 目录

| 路径 | 内容 |
|---|---|
| `DESIGN.md` | 玩法、成长曲线、记忆算法、小卖部、对战、技术预算、进度 |
| `ART_BRIEF.md`、`CODEX_PROMPTS.md` | 美术规格和给 Codex / Grok 的分轮提示词 |
| `art/` | 美术交付：起始图、视频、设计稿、精灵、道具、摆件、饰品、图标、`manifest.json`、检查图、美术脚本 |
| `sprites/` | 从视频抽出的 NABO 动画帧（256×256，锚点 128,224） |
| `prototype/` | 网页原型（多文件，`assets/` 是脚本生成的词库和图集） |
| `data/` | 词库 `words.json` 和来源说明 |
| `tools/` | 词库生成、视频抽帧、原型图集打包 |
| `firmware/` | 固件代码（目前是 M2 逻辑模块），见 `firmware/README.md` |
| `tests/words_host/` | C 逻辑模块和原型的对拍测试 |

## 在另一台电脑上接着干

需要：Python 3（Pillow、numpy）、Node.js 18+、ffmpeg、C 编译器（cc / clang / gcc）。

```sh
git clone https://github.com/Liutupi/nabo-words.git
cd nabo-words
python3 -m pip install pillow numpy
```

**打开原型**：直接用浏览器打开 `prototype/index.html`，或者起一个本地服务：

```sh
python3 -m http.server 8767 --directory prototype
```

键盘 ↑ ↓ 选择、回车确认、长按 ↑ 返回；右侧面板可以快进学习日、调对手机器人。浏览器控制台里的调试钩子是 `NW`（`NW.press('ok')`、`NW.days(30)`、`NW.state()`）。

**跑对拍测试**（改了 C 代码或原型规则之后）：

```sh
node tests/words_host/record_golden.js   # 只有改了原型规则才需要重录
python3 tests/words_host/test_words.py
```

**重新生成词库**（改了 `tools/wordlists/` 或生成脚本之后）：ECDICT 原始数据 66 MB 不入库，先按 `data/README.md` 下载到 `third_party/ecdict/ecdict.csv`，再运行：

```sh
python3 -I tools/build_wordlist.py
```

**接入新的美术**：Codex / Grok 交付放进 `art/`（视频放 `art/video/<学段>/`，在 `tools/clips.json` 里登记），然后：

```sh
python3 tools/clip_to_sprites.py      # 视频 → sprites/
python3 tools/build_proto_atlas.py    # art/ + sprites/ → prototype/assets/
```

## 待办

- 小披风已由 Codex 重画（2026-10-21 19:13），要按 `placement_suggestion` 放大后才看得清；原型里的饰品都按原尺寸 12 px 画，所以披风暂时仍用程序画的。
- 掌握 2500 词后的笔记本电脑放在桌子正中，会挡住 NABO 胸前的服装细节，考虑挪到一侧。
- 饰品固定 12 px，戴在 48 px 的完全体上偏小，固件里按 `manifest.json` 的 `placement_suggestion` 缩放。
- 四级到毕业的 12 段视频是 Codex 断线时由 Claude Code 检查后抽帧的，`art/QA.md` 还没覆盖第 5 轮和这批视频，可让 Codex 复核。
- 小学词表是按教材主题整理的近似版，待用课标附录核对。
- 下一步 M3：渲染（沿用牌桌的显示列表 + 脏矩形）、ESP-IDF 5.5.3 独立固件工程、词库和精灵打包进 assets 分区。

## 许可

代码 MIT；NABO 形象和美术素材不随 MIT 授权；词库数据来自 ECDICT（MIT）。详见 [NOTICE.md](NOTICE.md)。本项目与 FoloToy 官方无关联。
