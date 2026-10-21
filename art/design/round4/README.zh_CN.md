<p align="right"><strong>简体中文</strong> · <a href="README.md">English</a></p>

# 第四轮——精灵、道具、摆件和图标

2026-10-21 完成。共 **50 张正式 4 倍透明 PNG**：六只精灵各四形态、十二件道具、十个小学摆件、四张界面图。图标为三类，其中火苗有亮暗两版。七个文件逐字节沿用既有定稿：豆豆四形态、台灯、闹钟和铅笔盒。

## 先看这些

- [六只完全体](../../preview/companion_family.png)
- [全部成长形态](../../preview/companion_growth.png)与[实际尺寸检查](../../preview/companion_size_check.png)
- [十二件道具](../../preview/props_check.png)
- [十个摆件及实心剪影](../../preview/toys_silhouette.png)、[六组背景上的 12 px 检查](../../preview/toys_check.png)
- [浅／深背景图标](../../preview/ui_check.png)
- [验收结果](../../QA.zh_CN.md)与[自动检查数据](validation.json)

精灵检查图先将正式文件缩至目标高度，再用最近邻放大。摆件和图标先落在 12×12 像素网格，再导出 48×48。道具和精灵在 4 倍文件中保留更多边缘细节供最终打包器处理；原尺寸检查使用 Lanczos 缩小，最终打包还需验证所选采样方式和 RGB565 实屏效果。

## 为小尺寸做的设计

六只统一大椭圆深色眼睛、奶油色高光、桃色脸颊、小笑嘴、圆身体和短脚，用叶子、水滴、星星／书页、火苗、海浪／贝壳、极光／羽毛区分。完全体各有收束特征：豆豆的叶冠、泡泡的光环、星星的金星与书页翅膀、火火的火冠和徽章、浪浪的波浪与贝壳、极光的羽冠与小翅膀。各自延续蛋壳主题。

每张精灵使用 10–16 个可见颜色，固定调色板去掉大面积细微渐变。极光补入专用粉色身体色阶，避免相邻颜色量化产生斑点。道具和摆件保留大结构，省去细字、细针、织物颗粒和微小装饰；闹钟没有指针。描边采用物体固有深色，没有白色贴纸边。

## 正式文件清单

下列路径相对 `art/`。**目标**为游戏像素，**文件**为 4 倍画布，**锚点**为从画布左上角算起的游戏像素坐标；文件坐标锚点、调色板和 SHA-256 见 [manifest.json](../../manifest.json)。精灵、桌面道具、摆件采用底部中点；闹钟、奖状、地图和图标采用中心。精灵画布宽度包含横向透明留边。

### 精灵

| 路径 | 目标 | 文件 | 锚点 | 色数 |
| --- | --- | --- | --- | --- |
| [`companion/doudou/egg.png`](../../companion/doudou/egg.png) | 25×28 | 100×112 | (12.5, 28) | 11 |
| [`companion/doudou/baby.png`](../../companion/doudou/baby.png) | 23×32 | 92×128 | (11.5, 32) | 16 |
| [`companion/doudou/teen.png`](../../companion/doudou/teen.png) | 29×40 | 116×160 | (14.5, 40) | 16 |
| [`companion/doudou/final.png`](../../companion/doudou/final.png) | 37×48 | 148×192 | (18.5, 48) | 16 |
| [`companion/paopao/egg.png`](../../companion/paopao/egg.png) | 25×28 | 100×112 | (12.5, 28) | 12 |
| [`companion/paopao/baby.png`](../../companion/paopao/baby.png) | 25×32 | 100×128 | (12.5, 32) | 16 |
| [`companion/paopao/teen.png`](../../companion/paopao/teen.png) | 34×40 | 136×160 | (17, 40) | 16 |
| [`companion/paopao/final.png`](../../companion/paopao/final.png) | 36×48 | 144×192 | (18, 48) | 16 |
| [`companion/xingxing/egg.png`](../../companion/xingxing/egg.png) | 25×28 | 100×112 | (12.5, 28) | 12 |
| [`companion/xingxing/baby.png`](../../companion/xingxing/baby.png) | 27×32 | 108×128 | (13.5, 32) | 14 |
| [`companion/xingxing/teen.png`](../../companion/xingxing/teen.png) | 40×40 | 160×160 | (20, 40) | 14 |
| [`companion/xingxing/final.png`](../../companion/xingxing/final.png) | 47×48 | 188×192 | (23.5, 48) | 16 |
| [`companion/huohuo/egg.png`](../../companion/huohuo/egg.png) | 25×28 | 100×112 | (12.5, 28) | 14 |
| [`companion/huohuo/baby.png`](../../companion/huohuo/baby.png) | 25×32 | 100×128 | (12.5, 32) | 14 |
| [`companion/huohuo/teen.png`](../../companion/huohuo/teen.png) | 36×40 | 144×160 | (18, 40) | 16 |
| [`companion/huohuo/final.png`](../../companion/huohuo/final.png) | 40×48 | 160×192 | (20, 48) | 16 |
| [`companion/langlang/egg.png`](../../companion/langlang/egg.png) | 25×28 | 100×112 | (12.5, 28) | 10 |
| [`companion/langlang/baby.png`](../../companion/langlang/baby.png) | 24×32 | 96×128 | (12, 32) | 16 |
| [`companion/langlang/teen.png`](../../companion/langlang/teen.png) | 31×40 | 124×160 | (15.5, 40) | 15 |
| [`companion/langlang/final.png`](../../companion/langlang/final.png) | 40×48 | 160×192 | (20, 48) | 16 |
| [`companion/jiguang/egg.png`](../../companion/jiguang/egg.png) | 24×28 | 96×112 | (12, 28) | 12 |
| [`companion/jiguang/baby.png`](../../companion/jiguang/baby.png) | 24×32 | 96×128 | (12, 32) | 15 |
| [`companion/jiguang/teen.png`](../../companion/jiguang/teen.png) | 34×40 | 136×160 | (17, 40) | 14 |
| [`companion/jiguang/final.png`](../../companion/jiguang/final.png) | 46×48 | 184×192 | (23, 48) | 14 |

### 道具

| 路径 | 目标 | 文件 | 锚点 | 色数 |
| --- | --- | --- | --- | --- |
| [`props/lamp.png`](../../props/lamp.png) | 24×44 | 96×176 | (12, 44) | 14 |
| [`props/clock.png`](../../props/clock.png) | 28×28 | 112×112 | (14, 14) | 8 |
| [`props/pencilcase.png`](../../props/pencilcase.png) | 34×12 | 136×48 | (17, 12) | 12 |
| [`props/plant.png`](../../props/plant.png) | 16×24 | 64×96 | (8, 24) | 14 |
| [`props/globe.png`](../../props/globe.png) | 24×30 | 96×120 | (12, 30) | 17 |
| [`props/shelf.png`](../../props/shelf.png) | 66×8 | 264×32 | (33, 8) | 5 |
| [`props/cert.png`](../../props/cert.png) | 36×28 | 144×112 | (18, 14) | 21 |
| [`props/phones.png`](../../props/phones.png) | 24×14 | 96×56 | (12, 14) | 11 |
| [`props/laptop.png`](../../props/laptop.png) | 56×30 | 224×120 | (28, 30) | 13 |
| [`props/mug.png`](../../props/mug.png) | 18×16 | 72×64 | (9, 16) | 9 |
| [`props/map.png`](../../props/map.png) | 60×38 | 240×152 | (30, 19) | 23 |
| [`props/trophy.png`](../../props/trophy.png) | 16×30 | 64×120 | (8, 30) | 8 |

### 摆件

| 路径 | 目标 | 文件 | 锚点 | 色数 |
| --- | --- | --- | --- | --- |
| [`toys/duck.png`](../../toys/duck.png) | 12×12 | 48×48 | (6, 12) | 7 |
| [`toys/rocket.png`](../../toys/rocket.png) | 12×12 | 48×48 | (6, 12) | 17 |
| [`toys/snowglobe.png`](../../toys/snowglobe.png) | 12×12 | 48×48 | (6, 12) | 13 |
| [`toys/cactus.png`](../../toys/cactus.png) | 12×12 | 48×48 | (6, 12) | 10 |
| [`toys/cube.png`](../../toys/cube.png) | 12×12 | 48×48 | (6, 12) | 17 |
| [`toys/plane.png`](../../toys/plane.png) | 12×12 | 48×48 | (6, 12) | 10 |
| [`toys/dino.png`](../../toys/dino.png) | 12×12 | 48×48 | (6, 12) | 8 |
| [`toys/hourglass.png`](../../toys/hourglass.png) | 12×12 | 48×48 | (6, 12) | 14 |
| [`toys/shell.png`](../../toys/shell.png) | 12×12 | 48×48 | (6, 12) | 7 |
| [`toys/windmill.png`](../../toys/windmill.png) | 12×12 | 48×48 | (6, 12) | 14 |

### 图标

| 路径 | 目标 | 文件 | 锚点 | 色数 |
| --- | --- | --- | --- | --- |
| [`ui/coin.png`](../../ui/coin.png) | 12×12 | 48×48 | (6, 6) | 6 |
| [`ui/flame.png`](../../ui/flame.png) | 12×12 | 48×48 | (6, 6) | 10 |
| [`ui/freeze.png`](../../ui/freeze.png) | 12×12 | 48×48 | (6, 6) | 11 |
| [`ui/flame_dim.png`](../../ui/flame_dim.png) | 12×12 | 48×48 | (6, 6) | 3 |

## 来源和复现

27 张新增原图全部使用 **Codex 内置 ImageGen** 生成。完整提示词和参考路径在 [prompts.json](prompts.json)；原生成图保留在 `source/<任务名>.png`，生成时原保存位置记录在 `receipts/<任务名>.json`。五张精灵设定稿各含四形态。暗火苗由亮火苗确定性转为灰阶，透明通道完全一致。没有使用 CLI/API 备用生成方式。

在项目根目录依次运行 `python3 art/tools/build_round4.py`、`python3 art/tools/validate_round4.py` 可复现规范化、尺寸、限色和文件检查。它们仅写入 `art/`，不会重新生成图片、修改原型、抽视频帧或发布文件。`export_summary.json` 列出导出与色数，`validation.json` 记录检查与哈希，`baseline.json` 保留本轮开始前的保护哈希。

## 交接

50 张文件已齐，可进行主人审图与后续打包。小学 A、初中 B、高中 B 的选择保持不变。既有视频源已通过审核，当前不需要 Grok 返工。最终动画循环、48 px 开心／沮丧头部角标及实体屏幕仍需在抽帧接入时验收。

制作期间规格新增第五轮：其余 50 个摆件和 8 件饰品，本轮不包含。现有十个摆件仍位于 `art/toys/`；第五轮再连同清单引用迁入 `art/toys/primary/`。manifest 的历史设计项保留作溯源；本轮静态接入使用正式的 `companion`、`props`、`toys`、`ui` 分组。
