# NABO 单词护照 v0.4.0

**预发布：本版本尚未刷入实机验证，供下载测试。** 面向 FoloToy AI Passport 工牌（ESP32-C3、8 MB Flash）的独立离线固件，开机直接进入，不需要配网、激活或联网。

## 新增

- **成就页**：主页菜单「成就」，7 个成就显示获得状态和进度。
- **NABO 周报**：每满 7 个学习日回主页时，汇报这周新学、新掌握、打卡天数和最近答错的 4 个词。
- **对战再来一局**：结算页按下键，直接再邀请同一个对手。
- **串口调试**：`goto <菜单项>` 直接打开画面，`perf` 统计帧耗时。
- 原型补齐分级测试、加学、闪电挑战规则（`prototype/extra.js`），与固件逐题对拍。

保留 7500 词、六个学段、家乡与分级测试、今日学习与结业考、7 套 NABO 服装、单词精灵、书桌与收集、护照、星图、打卡和 ESP-NOW 对战。尚未合并的界面动画 PR #1 不包含在本次固件中。

## 从 v0.3.0 升级

学习记录格式未变。使用 `python flash_words.py <串口> --app` 只更新程序和资源，保留学习记录；此模式不会自动备份。周报从升级后开始统计。

`assets.bin` 和分区表与 v0.3.0 逐字节相同。本次保留云端提供的原始发布文件，未重新编译或重打包。

## 刷写

**完整安装会覆盖工牌原来的固件（包括小智），并清空学习记录。** 默认运行 `python flash_words.py <串口>` 会先把整片 8 MB Flash 备份到 `backups/`，核对备份大小后再安装。

也可以把 `nabo-words-merged.bin` 从 `0x0` 刷入，或把 `bootloader.bin` / `partition-table.bin` / `nabo-words-passport.bin` / `assets.bin` 分别写到 `0x0` / `0x8000` / `0x10000` / `0x210000`。从已有的单词护照升级并保留记录时，请使用上述 `--app` 模式。

## 验证与来源

- 本地重新验证：五个 `.bin` 的 SHA256 全部匹配；应用内嵌版本为 `0.4.0`、ESP-IDF 为 `v5.5.3`，镜像校验和及验证哈希有效；合并镜像与四个分镜像逐字节一致，分区布局和容量检查通过。
- 本地规则回归：16317 步一致，新规则用例通过；分级测试、加学、闪电挑战的原型与 C 对拍 562 行一致。新增界面中文在三种中文字号的资源字体中均存在。
- 云端构建记录：干净检出 main 的 `9f34a806a037583b07260023085fde3d1cd0dbea`，使用 ESP-IDF 5.5.3 构建；宿主渲染 18 个画面增量重画与整屏重画一致、无溢出；宿主双进程对战两局（含再来一局）比分、积分、金币一致。这两项宿主测试本次未在本机重跑。
- 发布文件来源：`release-files-v0.4.0` 分支的 `62e1ee7f6c1434d9ac8b41d49052e6ee2db7eeba`。程序 917,760 字节，资源 4,352,090 字节。
- **尚未实机验证**：成就页、周报、再来一局、实体按键、声音与帧耗时、睡眠唤醒、两台真机对战、长时间运行与跨天学习时钟。

## SHA256

```text
4839c48836d9cfbbfdc8094faff8a2e300679d0f92a7857a7d8a457a2d510ec7  assets.bin
09e04f7fd4aafe825dbce689b04082e574c156d9a1cac10f6992631d3ff3c9ed  bootloader.bin
55cc8115e94f4ddf81671956624f72721b4f3002f45fa72b6f8ca855c4cc409d  nabo-words-merged.bin
722d4d886fc2e8e32719ccaf3e68637dade263b0b9a768091574619aa4898fb9  nabo-words-passport.bin
34373abb6cd1b5c1f6de2bde0a3f7eb457fe21c3ee3a7c83a9af1fcc30d06088  partition-table.bin
```

## 许可

代码 MIT；NABO 形象和美术不随 MIT 授权；词库来自 ECDICT（MIT）；字体位图子集来自 Noto Sans SC、Noto Sans、Fredoka、Space Mono（OFL 1.1）；`firmware/components/bsp/` 来自 FoloToy 官方 ai-passport（MIT，见 `firmware/LICENSE.FoloToy`）。本项目与 FoloToy 官方无关联。
