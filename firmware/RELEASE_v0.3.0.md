# NABO 单词护照 v0.3.0

面向 FoloToy AI Passport 工牌（ESP32-C3、8 MB Flash）的独立离线固件。开机直接进入单词护照，无需配网、激活或联网。

本次发布直接使用已经刷入两台工牌的预编译文件，不重新编译或重新打包。

## 功能

- 7500 词，覆盖小学、初中、高中、四级、六级、进阶六个学段。
- 开局选择家乡，分级测试确定学习起点。
- 今日学习、加学、60 秒闪电挑战、结业考。
- 7 套服装动画、单词精灵、书桌、摆件、饰品、小卖部。
- 星图、护照、打卡，记录学习与收集进度。
- 两台工牌通过 ESP-NOW 对战。

## 刷写

**刷写会覆盖工牌原来的小智固件。首次安装请先备份原固件；完整安装会清空学习记录。**

有两种镜像写入方式，任选其一：

1. 将合并镜像 `nabo-words-merged.bin` 从 `0x0` 刷入。
2. 分别将以下四个镜像写到对应地址：

| 文件 | 写入地址 |
| --- | --- |
| `bootloader.bin` | `0x0` |
| `partition-table.bin` | `0x8000` |
| `nabo-words-passport.bin` | `0x10000` |
| `assets.bin` | `0x210000` |

使用发布附件中的 `flash_words.py` 时，将脚本和四个分段镜像放在同一目录，安装 Python 3、`esptool` 和 `pyserial`，再运行：

```sh
python flash_words.py <串口>
```

默认流程先把原工牌的整片 8 MB Flash 备份到本地 `backups/`，检查备份大小后再清空学习记录并写入四个镜像；写入由 esptool 校验。不填写串口时，脚本只在恰好找到一个 Espressif USB 串口时自动选择。

已安装本项目兼容分区布局的版本时，可以只更新程序和资源，保留学习记录：

```sh
python flash_words.py <串口> --app
```

`--app` 仅写入 `nabo-words-passport.bin` 和 `assets.bin`，不写引导、分区表，不清空学习记录，也不再执行整片备份；首次从小智固件切换时使用默认完整安装。

## 验证

已有验证记录：

- 规则对拍 16317 步一致。
- 新规则用例通过。
- 宿主渲染核对通过。
- 两进程联机对战测试通过。
- 两台实机已刷写并开机正常。

尚未验证：两台真机之间的 ESP-NOW 对战、跨天学习时钟。宿主联机测试不代表真机无线对战已验证。

## 许可

- 项目代码采用 MIT 许可，见仓库根目录 `LICENSE`。
- NABO 形象与美术不随 MIT 授权，使用与再分发条件见 `NOTICE.md`。
- 词库来源为 ECDICT（MIT），见 `third_party/ecdict/LICENSE`。
- `assets.bin` 及合并镜像内的 Noto Sans SC、Noto Sans、Fredoka、Space Mono 位图字体子集采用 OFL 1.1，由 `firmware/tools/pack_assets.py` 生成。
- `firmware/components/bsp/` 来自 FoloToy 官方 ai-passport（MIT，见 `firmware/LICENSE.FoloToy`）。
- 本项目与 FoloToy 官方无关联。AI Passport 是 FoloToy 的产品。

## 发布附件与 SHA256

附件共 7 个：以下 5 个 `.bin`、`flash_words.py`、`SHA256SUMS.txt`。下载后可在附件所在目录运行 `sha256sum -c SHA256SUMS.txt`，核对五个镜像。

```text
b73d236f528d570ee72d29f01f20ea17bbe9991928d78761b3425ab71568795d  nabo-words-merged.bin
92075b70d46c20fa069c037ffcd1e4df8993e9545b7e1e1d13c90d45a00627da  bootloader.bin
34373abb6cd1b5c1f6de2bde0a3f7eb457fe21c3ee3a7c83a9af1fcc30d06088  partition-table.bin
7ae6bcddd632c5a089f659b6687723414a875bd0b898c5e12d2f536c64496ab3  nabo-words-passport.bin
4839c48836d9cfbbfdc8094faff8a2e300679d0f92a7857a7d8a457a2d510ec7  assets.bin
```
