<p align="right"><strong>简体中文</strong> · <a href="RELEASE_v0.4.2-preview.1.md">English</a></p>

# NABO 单词护照 v0.4.2-preview.1

动画预览版：补齐初中、高中、四级、六级、进阶和毕业六套服装的打瞌睡与大欢呼，共 12 段新动画。

- 睡眠包含趴下、闭眼呼吸循环和起身；按键唤醒后回到待机。
- 获得奖励时播放当前服装的欢呼，保留耳机、相机和同一卷毕业证；毕业欢呼为单手举证。
- 保留 NABO 熟悉的脸、眼镜、白帽和正向帽标，原型与固件同步接入动画。
- 原有词库、字体、静态美术和已有动画保持原样。

## 升级

学习记录格式和分区布局未变。已安装兼容的 v0.3.0、v0.4.0 或 v0.4.1 时，把发布文件放在同一目录，用 Python 3 与 esptool 5 运行：

```sh
python flash_words.py <串口> --app
```

同时更新程序与资源，保留学习记录分区；必须使用本版配套文件。完整的 `nabo-words-merged.bin` 用于从 `0x0` 安装，会清空学习记录；社区完整镜像安装不适合作为保留进度升级方式。

程序 918,608 字节，资源 6,074,022 字节，资源分区剩余 151,898 字节。新增睡眠为 24 帧 / 6 fps；欢呼裁去两端短暂停顿，保留 39 帧 / 12 fps；已有待机仍为 49 帧 / 12 fps。新动画最多 32 色，打包时逐帧检查颜色误差与索引解码一致性。详细构建与测试记录见 [validation/v0.4.2-preview.1.json](../validation/v0.4.2-preview.1.json)。

## SHA256

```text
6ae97bc0851ec88b2ffab88d707d7bec7302c2d347071c783955d7a317d50ab4  assets.bin
a6ecb3a836e749c97e8d9d6a17bf8f689558b2b8301bd31ae84da35b88036b4f  bootloader.bin
29b51008443abace7c493f88e5ec1ab6ce0d6492a21a0e301faafde2860f7a8e  flash_words.py
1a1a6b3b9c39e3a6c904cde270478d7cebf1be6bcff8911c6edbe41b72a49111  nabo-words-merged.bin
d09ddb885c68715cef8de8e24e813f4c106ff0d5e8b8e437fb553c074a6a16b8  nabo-words-passport.bin
34373abb6cd1b5c1f6de2bde0a3f7eb457fe21c3ee3a7c83a9af1fcc30d06088  partition-table.bin
```

## 许可

代码 MIT；NABO 形象与美术不随 MIT 授权；词库 ECDICT（MIT），字体 OFL 1.1；BSP 来自 FoloToy 官方 ai-passport（MIT）。本项目与 FoloToy 官方无关联。
