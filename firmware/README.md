# 《NABO 单词护照》独立固件

只装这一个游戏的离线固件（不带小智），开机直接进主页，不需要配网、激活或联网。规则模块在 `main/words/`（M2，和原型逐步对拍），
渲染和画面在 `main/nw_*.c`（M3）。

## 操作

| 按键 | 作用 |
|---|---|
| 上键 | 单击：上一项（闪卡：认识）；按住 1 秒：返回上一级（右上角有进度圈） |
| 下键 | 下一项（闪卡：不认识） |
| OK | 确认 |

第一次开机先选家乡（34 个省级行政区 → 地级城市），护照第 1 枚章就是家乡；然后可以做分级测试（每轮考一个学段 10 题，答对 8 题算过，
二分查找起点，最多 3 轮 30 题，每题都有“不认识”），测出的起点之前的词记为毕业，星图上用浅色。

主页菜单：今日学习、结业考（达到条件才出现）、加学 5 个新词（今天学完后，每天最多 4 组，全对 +1 金币）、闪电挑战（60 秒抢答学过的词，
答错的排进明天复习，每天第一次每答对 5 题 1 金币）、分级测试（还在小学开头时）、对战（下一版）、星图、护照、书桌、小卖部、每日新词、家乡、声音。
规则在 `main/words/nabo_words_extra.c`，用例 `tests/words_host/test_extra.py`（原型里还没有这三项）。
5 分钟没有操作会先保存再深睡，按任意键唤醒。

## 结构

| 路径 | 内容 |
|---|---|
| `main/main.c` | 启动、按键入队、双缓冲送屏、16 kHz 提示音、学习记录分区读写、自动深睡、串口调试命令 |
| `main/nw_game.c` | 画面状态机、按键、学习时钟、保存（对应 `prototype/app.js`） |
| `main/nw_scene_*.c` | 各画面的绘制（对应 `prototype/screens.js`、`draw.js`） |
| `main/nw_gfx.c` | 绘制列表 + 脏矩形 + 条带合成（思路同牌桌 `nabo_table_gfx`），只用整数 |
| `main/nw_font.c` | NWF1 位图字体（4 bpp），从 assets 分区原地读 |
| `main/nw_sprite.c` | NABO 动画包读取（NTS1 格式，来自牌桌） |
| `main/nw_store.c` | 学习记录：两个槽轮流写，带序号和 CRC，断电不会写坏 |
| `main/nw_db.c`、`nw_assets.c`、`nw_data.c` | 词库、assets 目录和静态美术、学段 / 城市 / 道具等固定表 |
| `tools/pack_assets.py` | 打包 assets 分区：词库、字体子集、静态美术、7 个学段的 NABO 动画，逐块解码核对 |
| `components/bsp/` | FoloToy 官方 ai-passport 板级支持包，原样复制（MIT，见 `LICENSE.FoloToy`） |

### 分区

| 分区 | 偏移 | 大小 | 内容 |
|---|---|---|---|
| nvs / phy_init | 0x9000 / 0xF000 | 24 KB / 4 KB | |
| factory | 0x10000 | 1.94 MB | 程序（v0.1.0 约 423 KB） |
| record | 0x200000 | 64 KB | 学习记录，两个 32 KB 槽 |
| assets | 0x210000 | 5.94 MB | v0.1.0 用了 4.11 MB |

### 学习时钟

没有网络对时，用 RTC（深睡时继续走）。完成今日学习后过 8 小时开启下一个学习日；距上次完成超过 48 小时，每多 24 小时算漏一天
（有补签卡自动用掉）。上电 / 掉电复位后 RTC 时间未知：不判断断签，今天学完了就直接开启新的一天（DESIGN.md 第 2 节）。

### 字体（Google Fonts，OFL 1.1，源文件不入库）

思源黑体 Noto Sans SC（12 / 16 / 22 px，界面和 7500 词释义共 2670 字）、Fredoka（英文单词 36 / 28 / 20 / 16 px）、
Noto Sans（13 px，音标）、Space Mono Bold（数字）。

## 构建

需要 ESP-IDF 5.5.3、Python 3（Pillow、numpy）。

```sh
python3 -I firmware/tools/pack_assets.py --fonts <放 4 个字体文件的目录>   # 生成 firmware/assets/assets.bin
cd firmware
idf.py set-target esp32c3
idf.py build
idf.py merge-bin -o nabo-words-merged.bin
python3 tools/verify_firmware.py build
```

`build/nabo-words-merged.bin` 可以从 `0x0` 刷写（AI Passport 网页刷机工具的“本地固件刷机”）。用 esptool 时按 `build/flash_args` 分别写四个镜像更快。

## 测试

```sh
python3 tests/words_host/test_words.py          # 规则模块和原型逐步对拍
python3 tests/fw_host/run_host_render.py out/   # 游戏层在宿主上跑一遍主要画面，截图；核对增量重画 = 整屏重画、绘制列表不溢出
```

## 串口调试命令

USB 串口（115200）每行一条：`status`、`key up|down|ok|back`、`solve [N]`（把当前学习 / 分级测试答对做完）、`days N`（推进 N 个学习日）、`sim N [正确率]`（按原型面板的方法自动学 N 天）、
`coins N`、`save`、`wipe`（清空学习记录）。

## 验证记录

v0.1.0（2026-10-21）：

- 规则对拍 16317 步一致；宿主渲染：主页、新词卡、选择题、星图、护照、四级学段主页、闪卡、对战页的增量重画和整屏重画逐像素一致，无溢出
- 构建：PASS，程序 433,344 字节，assets 4,308,522 字节，合并镜像布局检查通过
- 实机：作者的工牌（COM11）先整片备份 8 MB，再写入，esptool 校验通过；开机后串口 `status` 正常（已经在学第一组新词），空闲堆 173 KB、最大连续块 112 KB
- 未验证：按键手感、提示音音量、帧率实测、10 轮进出与 5 分钟深睡唤醒、跨天的学习时钟、断电后记录保存
