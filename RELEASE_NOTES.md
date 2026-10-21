# NABO 单词护照 v0.4.0

面向 FoloToy AI Passport 工牌（ESP32-C3、8 MB Flash）的独立离线固件，开机直接进入，不需要配网、激活或联网。

## 新增

- **成就页**：主页菜单「成就」，7 个成就都显示进度（如 58 / 100 词、连续 3 / 7 天）。
- **NABO 周报**：每满 7 个学习日回主页时弹出：这周新学、新掌握、打卡天数、最近答错的 4 个词。
- **对战再来一局**：结算页按下键，直接再邀请同一个对手。
- **串口调试**：`goto <菜单项>` 直接打开某个画面，`perf` 统计帧耗时。
- 原型补齐分级测试 / 加学 / 闪电挑战规则（`prototype/extra.js`），和固件逐题对拍一致。

## 从 v0.3.0 升级

学习记录格式没变，用 `flash_words.py <串口> --app` 只更新程序和资源，进度保留。周报从升级后那天开始算第一周。
`assets.bin` 和 v0.3.0 完全相同（SHA256 一致）。

## 刷写

**首次安装会覆盖工牌原来的固件（包括小智）。** `flash_words.py <串口>` 会先把整片 8 MB Flash 备份到 `backups/` 再写入；也可以把
`nabo-words-merged.bin` 从 `0x0` 刷入，或把 `bootloader.bin` / `partition-table.bin` / `nabo-words-passport.bin` / `assets.bin`
分别写到 `0x0` / `0x8000` / `0x10000` / `0x210000`。

## 验证

- 规则对拍 16317 步一致；分级测试 / 加学 / 闪电挑战用例通过；原型与 C 对拍 562 行一致
- 宿主渲染 18 个画面，增量重画与整屏重画逐像素一致，无溢出
- 宿主双进程对战两局（含再来一局），比分、积分、金币一致
- 从干净检出构建（ESP-IDF 5.5.3），合并镜像布局检查通过；发布文件即这次构建的输出
- **还没刷进实机验证**：成就页、周报、再来一局、两台真机对战

## SHA256

```
4839c48836d9cfbbfdc8094faff8a2e300679d0f92a7857a7d8a457a2d510ec7  assets.bin
09e04f7fd4aafe825dbce689b04082e574c156d9a1cac10f6992631d3ff3c9ed  bootloader.bin
55cc8115e94f4ddf81671956624f72721b4f3002f45fa72b6f8ca855c4cc409d  nabo-words-merged.bin
722d4d886fc2e8e32719ccaf3e68637dade263b0b9a768091574619aa4898fb9  nabo-words-passport.bin
34373abb6cd1b5c1f6de2bde0a3f7eb457fe21c3ee3a7c83a9af1fcc30d06088  partition-table.bin
```

代码 MIT；NABO 形象和美术不随 MIT 授权；词库来自 ECDICT（MIT）；字体子集来自 Noto Sans SC / Noto Sans / Fredoka / Space Mono（OFL 1.1）；`components/bsp` 来自 FoloToy ai-passport（MIT）。本项目与 FoloToy 官方无关联。
