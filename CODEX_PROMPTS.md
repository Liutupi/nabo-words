# 给 Codex 和 Grok 的提示词

分四轮，每轮结束你检查一次再进下一轮。每段提示词都能在新会话里单独使用。Codex 在 `nabo-words/` 目录下启动。

---

## 第 1 轮（Codex）：小学 NABO 起始图 + 豆豆 + 道具试样

```
请先完整阅读 ART_BRIEF.md，这是美术规格，必须遵守；玩法见 DESIGN.md。

任务：做三件事，只做设计，不做动画。
1. 小学服装（ART_BRIEF 第 3 节 primary）的 NABO 起始图，按第 4 节的起始图要求（绿底、平直书桌、正面坐姿、双手轻放桌面）。
   出 2–3 个服装版本：art/design/primary_a.png、primary_b.png ……
   每个版本另附一张检查图：缩到帽顶到桌沿 110 px 放在小学墙色上、只取头部缩到 48 px 放在 #FFF8EC 上，最近邻放大 4 倍展示。
2. 单词精灵“豆豆”的设定图 art/design/doudou_sheet.png：蛋、幼崽、少年、完全体四个形态并排，
   另附缩到第 6 节最终高度、放在小学桌面色上的检查图。
3. 道具风格试样：台灯、闹钟、铅笔盒三个，按第 7 节尺寸的 4 倍出图放 art/props/，
   并做一张 240×320 主页合成图 art/preview/home_primary.png（小学墙和桌 + 版本 a 的 NABO + 豆豆幼崽 + 三个道具）。

在 art/design/README.md 里说明为了小尺寸做了哪些简化。
只在 art/ 下工作，不要修改其他目录。完成后告诉我所有图的路径。
```

**你要检查的：** 是不是你的 NABO 本人；48 px 角标还认不认得出；豆豆四个形态像不像同一只在长大；道具画风和 NABO 搭不搭。选定后告诉 Codex 用哪一版，把它另存为 `art/start/primary.png`。

---

## 第 2 轮（Grok）：小学动作视频

把 `art/start/primary.png` 交给 Grok，按 `art/reference/SHOT_LIST.md` 逐条生成小学的 01–05 号镜头，提示词模板在那份文件里。原视频放进 `art/video/primary/`。

给 Grok 的话可以直接这样说：

```
请阅读 nabo-words/art/reference/SHOT_LIST.md，用 nabo-words/art/start/primary.png 作为起始图，
按表里的顺序生成小学（primary）的 01–05 号视频，每条都用文件里的提示词模板，
生成后按规定的文件名放进 nabo-words/art/video/primary/。
```

**你要检查的：** 镜头有没有动；中途有没有变脸、变衣服；帽标字样有没有乱；结尾是否回到起始姿势。交回后由我抽帧放进原型里看效果。

---

## 第 3 轮（Codex，之后 Grok）：初中、高中服装

```
请先完整阅读 ART_BRIEF.md。小学定稿是 art/start/primary.png。

任务：按第 3 节做初中（junior）和高中（senior）的起始图。
- NABO 的身份、头部位置和大小必须和 primary.png 完全一致，只换衣服；构图、绿底、书桌位置也完全一致，
  这样三张叠在一起时只有衣服不同。
- 每个学段出 2 个版本：art/design/junior_a.png、junior_b.png、senior_a.png、senior_b.png，附小尺寸检查图。
- 另做一张 art/preview/outfit_growth.png：小学、初中、高中三套并排，用来检查成长感。
只在 art/ 下工作。
```

选定后另存为 `art/start/junior.png`、`art/start/senior.png`，再按第 2 轮的方式交给 Grok 做 01–03 号镜头。

---

## 第 4 轮（Codex）：其余精灵、道具、摆件、图标

```
请先完整阅读 ART_BRIEF.md 第 6–10 节。豆豆的定稿是 art/design/doudou_sheet.png。

任务：
1. 按豆豆的画风做其余 5 只精灵（泡泡、星星、火火、浪浪、极光），每只 4 个形态，
   放 art/companion/<拼音名>/egg.png、baby.png、teen.png、final.png（4 倍尺寸）。豆豆也按这个结构导出。
   另做 art/preview/companion_family.png：6 只完全体并排。
2. 第 7 节剩下的 9 个道具、10 个盲盒摆件、3 个界面图标，按 4 倍尺寸出图。
3. manifest.json 写明每个静态图的 target_px 和锚点。
4. 跑一遍第 10 节验收清单，把结果写进 art/QA.md。
只在 art/ 下工作。
```

**你要检查的：** `companion_family.png` 里 6 只像不像一家人；12 px 的摆件剪影能不能认出来。
