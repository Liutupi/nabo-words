<p align="right"><strong>简体中文</strong> · <a href="README.md">English</a></p>

# NABO 单词护照 — 第 1–4 轮美术

最新检查点：**第四轮完成，50 张正式静态 PNG 已齐，待主人审图**。见[第四轮完整清单](round4/README.zh_CN.md)、[六只精灵家族](../preview/companion_family.png)、[摆件剪影](../preview/toys_silhouette.png)和[验收记录](../QA.zh_CN.md)。本轮使用 Codex 内置 ImageGen，提示词与原稿均已保留。已收到的视频源当前不需要 Grok 继续返工。最终 48 px 动画表情角标、循环接缝及实体屏幕另行验收。第五轮新增摆件和饰品不在本次范围内。

请先看 **[总览图](../preview/round1_contact.png)**。本轮交付两版小学服装、豆豆四形态、三个道具试样和主页合成图。你已于 2026-10-21 选定 **A 版**。`art/start/primary.png` 与 `art/design/primary_a.png` 完全一致，B 版保留作备选。

## 图片清单

下面所有路径相对于项目根目录 `nabo-words/`。

| 图片 | 路径 | 内容 |
| --- | --- | --- |
| 第 1 轮总览 | `art/preview/round1_contact.png` | 两版服装、小尺寸检查、豆豆和主页 |
| 小学 A 版 | `art/design/primary_a.png` | 黄色 V 领背心、白 T 恤、砖红书包肩带 |
| A 版检查 | `art/design/primary_a_check.png` | 帽顶到桌沿 110 px、头部 48 px；最近邻放大 4 倍 |
| 小学 B 版 | `art/design/primary_b.png` | 黄色圆领背心、白 T 恤、深青肩带和大口袋 |
| B 版检查 | `art/design/primary_b_check.png` | 同样的两个目标尺寸和底色；最近邻放大 4 倍 |
| 豆豆设定图 | `art/design/doudou_sheet.png` | 蛋、幼崽、少年、完全体；透明背景 |
| 豆豆尺寸检查 | `art/design/doudou_check.png` | 小学桌色上 28/32/40/48 px；最近邻放大 4 倍 |
| 蛋设计图 | `art/design/doudou_egg.png` | 目标 25×28；透明 PNG 100×112 |
| 幼崽设计图 | `art/design/doudou_baby.png` | 目标 23×32；透明 PNG 92×128 |
| 少年设计图 | `art/design/doudou_teen.png` | 目标 29×40；透明 PNG 116×160 |
| 完全体设计图 | `art/design/doudou_final.png` | 目标 37×48；透明 PNG 148×192 |
| 台灯 | `art/props/lamp.png` | 目标 24×44；透明 PNG 96×176 |
| 闹钟 | `art/props/clock.png` | 目标 28×28；透明 PNG 112×112；没有指针 |
| 铅笔盒 | `art/props/pencilcase.png` | 目标 34×12；透明 PNG 136×48 |
| 主页合成 | `art/preview/home_primary.png` | 原尺寸 240×320；A 版 + 幼崽 + 三个道具 |
| 主页放大 | `art/preview/home_primary_2x.png` | 480×640；严格最近邻放大 2 倍 |

总览图里的检查区是缩略展示；需要检查严格的 4 倍效果时，请打开各自的 `*_check.png`。

## 为小屏做的简化

- NABO 保留参考图的白帽、正向 NABO 字样和蓝色帽标、深色卷发、圆眼镜、蓝灰眼睛和小微笑，只换小学服装。用大块 V/圆领、方扣和对比明显的肩带，删掉细针脚、布料纹理和多余配件。
- NABO 使用固定 32 色色板收紧光影与细渐变；头发整理成深色块，保留原有轮廓。头部缩到 48 px 时，靠帽子、眼镜、发型和眼睛辨认，不把帽上的微小字样当作可读的界面文字。
- 豆豆始终保留豆形身体、深绿轮廓和同款眼睛。蛋有叶形花纹，幼崽有双叶芽，少年长出手臂和大叶，完全体有三叶冠与一颗金色种子。省去细叶脉、复杂首饰、手指和纹理。四形态导出后可见 RGB 颜色分别为 **11/16/16/16 色**，都低于 32 色限制。宽度补透明边到 4 的倍数，以底部中点为锚点。
- 台灯保留大灯罩与底座，闹钟只留四个大刻度与空表盘，铅笔盒是珊瑚红胶囊轮廓加一个拉链头。道具采用固定 16 色色板，闹钟没有烘焙指针。
- 先缩到实际尺寸，再最近邻放大；4 倍检查图没有平滑放大。当前试样用二值透明通道，后续打包可按需要做边缘预混。

## 版式与检查

两版起始图均为 **1024×1280**。纯绿底精确为 **#00B140**，空白木色带为 **#C88A52**。帽顶 y=366，平直桌沿 y=1070，桌沿中点 x=512；帽顶到桌沿为 704 px，正好占画布高度 55%。双手轻放在桌沿并略微覆盖桌边。`art/manifest.json` 记录两版几何位置、道具尺寸和锚点。

主页沿用原型的墙、墙裙线、桌面和桌身颜色与主要坐标：NABO 中心 x=120、帽顶 y=66、桌沿 y=176、闹钟中心 (34,52)、台灯底部约 (29,176)、豆豆底部 (196,176)，铅笔盒位于桌沿前。界面文字仅用于检验合成效果，没有修改原型文件。

本地已检查：PNG 尺寸、道具和豆豆真实透明通道、背景纯色值、豆豆颜色数、整数倍放大、设定图单元无裁切，以及改动全部在 `art/`。视觉检查覆盖了中性表情 NABO 的身份、帽标拼写与方向、两种小尺寸、豆豆成长一致性、无指针表盘和原尺寸主页。**你已选定 A 版；豆豆与道具造型仍待后续确认。** 开心/沮丧表情、动画一致性、真实 RGB565 屏幕显示不属于本轮静态检查。

## 来源与重建

插画使用 **Codex 内置 ImageGen** 生成。身份参考是 `../ai-passport-nabo-xiaozhi/docs/images/nabo-idle.png` 中最左侧睁眼的一帧，没有镜像。制作提示词集保存在 `art/design/prompts.json`。

生成原图保存在 `art/design/source/`：`primary_a.png`、`primary_b.png`、`doudou_sheet.png`、`lamp.png`、`clock.png`、`pencilcase.png`。这些用于追溯，不是最终导出；`primary_a_cutout.png` 和 `primary_b_cutout.png` 是归一化与合成中间图。交付文件已把原图绿底和光影归一化。

运行 `python3 art/tools/build_round1.py` 可从这些原图重复生成尺寸、限色、检查图与合成图，需要 Pillow、numpy 和 fontconfig 可定位的本机苹方字体。脚本只写入 `art/`，不重新收费生图，也不改动生成原图。最终本地检查记录见 `art/design/validation.json`。

## 下一步交接

A 版已锁定为 `art/start/primary.png`。小学 01–05 视频现已通过源素材检查，睡眠采用 `art/video/primary/primary_04_sleep_v2.mp4`；中间约 1.4–2.9 秒可作为睡眠片段候选，仍需抽帧检查循环接缝。详见[视频检查报告](../preview/video_review/REVIEW.zh_CN.md)。这里尚未导出最终游戏动画帧。**初中 B、高中 B 已选定，六条 01–03 视频也已通过源素材检查。**详见[初中/高中视频检查报告](../preview/video_review/JUNIOR_SENIOR_REVIEW.zh_CN.md)。可以进入抽帧处理，本轮无需 Grok 补做。

## 第 3 轮 — 初中、高中服装候选

请看[四版总览](../preview/round3_contact.png)与[成长对照](../preview/outfit_growth.png)。你已于 2026-10-21 选定 **初中 B、高中 B**；成长对照现已更新为小学 A → 初中 B → 高中 B，并标注为已定稿。

| 候选 | 起始图 | 严格 4 倍尺寸检查 | 服装 |
| --- | --- | --- | --- |
| 初中 A | `art/design/junior_a.png` | `art/design/junior_a_check.png` | 蓝色衣身、白袖、简单中拉链 |
| 初中 B | `art/design/junior_b.png` | `art/design/junior_b_check.png` | 白色衣身、大块蓝肩胸与蓝袖 |
| 高中 A | `art/design/senior_a.png` | `art/design/senior_a_check.png` | 素面藏青西装、白衬衫、红领带 |
| 高中 B | `art/design/senior_b.png` | `art/design/senior_b_check.png` | 浅色翻领滚边藏青西装、白衬衫、红领带 |

四版均为 1024×1280。从顶部到 y=830 的完整区域（包括帽、发、脸、眼镜）直接复用小学定稿，**像素差为 0**。从 y=1070 开始的桌面、下部手指和背景也完全相同，桌沿附近原手部像素已还原。所有变化都限定在下颌与桌沿之间的服装区域。这是将 ImageGen 生成的衣服与原定稿精确合成，不是为每套衣服重新生成头部。

几何位置保持帽顶 y=366、桌沿 y=1070、中心 x=512。初中检查图使用 #BFE3E0 墙 / #B47C55 桌；高中为 #C9D3EE / #916243。48 px 头像同样复用小学参考，放在 #FFF8EC 上。单独的检查图均为原尺寸严格放大 4 倍；四版总览为便于比较采用 3 倍。初中精灵图可见颜色 30 色，高中 32 色。没有真实学校徽章、标识或红领巾。长袖采用前臂处留出的呈现方式，使原有手部位置保持不变。

原生成图与合成抠图保存在 `art/design/source/junior_a.png`、`junior_b.png`、`senior_a.png`、`senior_b.png` 及同名 `*_cutout.png`。完整制作提示词在 `art/design/round3_prompts.json`，本地检查在 `art/design/round3_validation.json`；运行 `python3 art/tools/build_round3.py` 可重建导出。`art/start/junior.png` 与 `art/design/junior_b.png` 完全一致；`art/start/senior.png` 与 `art/design/senior_b.png` 完全一致。两个学段的六条 01–03（待机/开心/沮丧）镜头现已收到，分别保存在 `art/video/junior/` 与 `art/video/senior/`，并已通过源素材检查。选择记录和哈希见 `art/manifest.json`。

### 初中/高中视频检查点

`art/video/junior/` 与 `art/video/senior/` 中的六条视频已收到并检查，保持所选 B 版服装，待机/开心/沮丧动作完整且最后回到中性姿势。源素材检查完成，最终抽帧处理和原尺寸实屏验证仍待后续完成。第四轮静态交付现已完成，见上方最新检查点。
