<p align="right"><strong>简体中文</strong> · <a href="README.md">English</a></p>

# 七套学段墙面装饰

新增七张透明墙饰，对应小学、初中、高中、四级、六级、进阶和毕业外观。主题依次为中国窗花、亚洲灯笼、欧洲窗景、美洲明信片、海浪与草原明信片、极光窗景和旅行纪念。这是路线图第 4 节的素材准备；固件接入仍等待第 1 节实机验收。

查看[原尺寸与精确 4 倍检查图](../../preview/wall-decor/native-and-4x.png)和[七套主页合成图](../../preview/wall-decor/homes.png)。预览目录还提供每套 240×320 原尺寸和精确 2 倍图。这些预览是在真实宿主渲染器输出上叠加候选墙饰；当前固件尚未绘制墙饰。

## 导出约定

- 正式素材：`art/wall/<stage>.png`，RGBA、192×96，是 48×24 游戏像素的最近邻精确 4 倍图。
- 锚点：左上角，素材坐标和游戏坐标均为 `(0,0)`。建议屏幕原点为 `(180,108)`。非透明像素仅占屏幕 y=108–131、x=180–227。
- RGB565 归一化后，每张使用二值透明度和 27–30 个可见 RGB 色，低于 32 色上限。按当前打包器的 RGB565＋透明度表示，像素数据共 24,192 字节，不含索引和对齐开销。
- 避开头部走廊 `x=70..169`、闹钟、奖状、地图、书架和书架绿植。导出脚本逐像素检查这些排除区域。
- `art/manifest.json` 的独立 `wall_decor` 分组记录尺寸、锚点、建议位置、调色板和哈希。当前打包器忽略该分组，因此应用、资源编号和发布版资源包均未改变。

## 来源与验证

七张 ImageGen 原图保留在 [source/](source/)；准确提示词和透明度设置见 [prompts.json](prompts.json)。使用 Codex 内置 ImageGen。已有 NABO 起始图、帽标、服装、精灵和道具均未重新生成，也未镜像。

运行现有宿主 UI 检查后，可重建导出与检查：

```sh
python3 tests/fw_host/test_ui.py --assets firmware/assets/assets.bin --out build/checks/ui
python3 art/tools/build_wall_decor.py --homes build/checks/ui
```

[validation.json](validation.json) 记录尺寸、原图真实透明度、导出二值透明度、可见颜色、屏幕排除区域和哈希。桌面目检覆盖七套原尺寸合成图和放大精灵图。作者美术验收、实屏显示和固件接入仍未验证。

Build: NOT RUN（仅准备美术）。

Host tests: PASS（规则、扩展规则与对拍、18 个画面、27,334 项 UI 检查、两局对战及再来一局）。使用 `--base-assets` 重新打包后，与公开 v0.4.1 资源逐字节一致。

Device tests: NOT RUN。

Unverified: 作者审图、实屏效果，以及实机验收后的固件接入。
