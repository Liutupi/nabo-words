# NABO 单词护照 v0.4.1

**预发布：宿主测试和 ESP32-C3 构建通过，尚未刷入实机验证。** 本版本在 v0.4.0 的基础上合并界面动画 PR #1，适用于 FoloToy AI Passport 工牌（ESP32-C3、8 MB Flash）。独立离线运行，无需配网或激活。

## 界面与 NABO 动画

- 保留你的原始 NABO 造型、7 套服装与所有静态美术，待机动画从 25 帧 / 6 fps 恢复为源素材的 49 帧 / 12 fps；已有睡眠素材同步恢复原采样率。这里的 12 fps 是资源播放节奏，实机帧率仍待测量。
- 主页的已学、掌握、学段进度分开排版，修复完成 7500 词时的统计重叠；顶部根据计数长度给城市名称留空间。
- 菜单增加 160 ms、最大 4 像素的轻微过渡与边框反馈，密集圆点改为当前位置轨道。
- 长文字在最小字号仍放不下时，按完整 UTF-8 字符省略，避免盖住相邻控件。
- 修复换装后沿用上一套动作帧范围、长时间运行后动画计时乘法溢出的问题。

保留 v0.4.0 的成就页、NABO 周报、对战再来一局、串口 `goto` 与 `perf`；以及 7500 词六个学段、家乡与分级测试、今日学习、加学、60 秒闪电挑战、结业考、单词精灵、书桌收集、小卖部、星图、护照、打卡和 ESP-NOW 对战。

## 升级与刷写

从 v0.3.0 或 v0.4.0 保留学习记录升级：

```sh
python flash_words.py <串口> --app
```

此模式同时更新 app 和 assets，保留 `0x200000` 的学习记录分区，但不会自动备份。**本版资源已更新，必须同时更新 app 与 assets。** 学习记录格式和分区布局未变。

完整安装会覆盖原固件（包括小智）并清空学习记录。默认运行 `python flash_words.py <串口>` 会先备份整片 8 MB Flash，再安装；手动刷写可将 `nabo-words-merged.bin` 从 `0x0` 写入，或分别写入：

| 文件 | 地址 |
|---|---|
| bootloader.bin | 0x0 |
| partition-table.bin | 0x8000 |
| nabo-words-passport.bin | 0x10000 |
| assets.bin | 0x210000 |

## 本轮验证

- 27,334 项专项界面检查全部通过：7500 词布局、7 套外观及动画解码、跳帧、长时间计时、换装、菜单过渡、成就页和周报弹出 / 关闭；增量重画与整屏重画逐像素一致，无绘制列表或字形池溢出。
- 18 个主要画面的宿主渲染核对通过，并检查成就和周报的渲染预览。
- 规则对拍 16,317 步一致，新规则用例通过；分级测试、加学、闪电挑战原型与 C 对拍 562 行一致。
- 两个宿主进程通过本机 UDP 打完两局，包含再来一局，两端比分与积分一致。Windows 网络适配仅用于宿主测试，不进入固件。
- ESP-IDF 5.5.3 / ESP32-C3 构建通过；应用镜像校验和及验证哈希有效；发布合并镜像与四个分镜像逐字节一致，分区容量与布局核对通过。
- 程序 918,320 字节，资源 4,658,019 字节；assets 分区余 1,567,901 字节。词库、10 块字体和静态美术共 12 块数据与 v0.4.0 逐字节相同。

app 由本次合并代码编译，assets 使用原始素材全帧打包；引导、分区表及刷写脚本复用已公开校验的 v0.4.0 文件。详细记录见 `validation/v0.4.1.json`，界面和动画预览见 `firmware/UI_ANIMATION_POLISH.md`。

**尚未实机验证**：动画流畅度、实体按键、声音、帧耗时、睡眠唤醒与耗电、两台真机对战、长时间运行和跨天学习时钟。

## SHA256

```text
01aa74d2f2a617b332bc36f90067fc914dad8f868351f14884b63c593a058611  nabo-words-merged.bin
09e04f7fd4aafe825dbce689b04082e574c156d9a1cac10f6992631d3ff3c9ed  bootloader.bin
34373abb6cd1b5c1f6de2bde0a3f7eb457fe21c3ee3a7c83a9af1fcc30d06088  partition-table.bin
c96e7eb3a503871a2de52c747b4128279b0944953308c7e91baa6fc563ab8aa4  nabo-words-passport.bin
5abc46e61f9ae1fe960d205018d0fdc6c6b6c4a382f919363db248bd0918ac6d  assets.bin
```

## 许可

代码 MIT；NABO 形象和美术不随 MIT 授权；词库 ECDICT（MIT）；Noto Sans SC、Noto Sans、Fredoka、Space Mono 字体位图子集采用 OFL 1.1；`firmware/components/bsp/` 来自 FoloToy 官方 ai-passport（MIT，见 `firmware/LICENSE.FoloToy`）。本项目与 FoloToy 官方无关联。
