# 交接说明（给下一位接手的 Claude / Codex）

## 现在的状态

- 固件 v0.4.0（分支 `optimize`，还没发布）。v0.3.0 已发布并刷进作者的两台工牌（MAC 98:c3:77:f5:78:74 学习中，98:c3:77:f4:a9:58 新开）。
- 规则以原型为准：`prototype/core.js`、`quiz.js`（记忆盒、题单、结业考、对战计分）和 `prototype/extra.js`（分级测试、加学、闪电挑战）。
  改规则的顺序：改原型 → 重录 / 重跑对拍 → 改 C → 跑全部测试。

## 测试（全部在宿主上，不碰设备）

```sh
python3 -I firmware/tools/pack_assets.py --fonts <字体目录>   # 改了界面文字（新汉字）也要重新打包，否则缺字
python3 tests/words_host/test_words.py
python3 tests/words_host/test_extra.py
python3 tests/words_host/extra_parity.py
python3 tests/fw_host/run_host_render.py out/
python3 tests/fw_host/run_host_battle.py
```

## 构建环境（云端）

- ESP-IDF 5.5.3；云端连不上 components.espressif.com，依赖组件（button 4.2.0、cmake_utilities 1.1.1、esp_codec_dev 1.6.2、
  esp_lvgl_port 2.9.0、lvgl 9.5.0）放在 `/opt/esp/vendor/`，构建时 `IDF_COMPONENT_MANAGER=0 idf.py -DEXTRA_COMPONENT_DIRS=/opt/esp/vendor build`。
  在能联网的电脑上照常 `idf.py build` 即可（组件管理器按 `components/bsp/idf_component.yml` 下载）。

## 刷机

- 新工牌：`flash_words.py`（先整片备份到 backups/ 再写）。已经装了这个游戏的：`flash_words.py --app`（只写程序和 assets，保留学习记录）。
- 学习记录在 record 分区（0x200000），`nw_record_t` 大小不能变（`nw_store.h` 有静态断言）；加字段只能用 `nw_extra_t` 里剩下的位置。
  v0.4.0 已经把 `reserved` 用完了，再要加字段需要升记录版本并写迁移。

## 注意

- 断电 / USB 复位后 RTC 时间未知：今天学完了就直接开启新的一天（DESIGN.md 第 2 节的设计），刷机也会触发。
- 对战只在对战画面打开 Wi-Fi；第一次打开后空闲堆从约 179 KB 降到约 125 KB 并保持稳定（Wi-Fi 初始化留下的），不是泄漏。
- 宿主截图里的 toast 会跨画面停留 2.2 秒，是正常的。

## 没做 / 待验证

- 两台真机对战（宿主双进程测试通过）、长时间待机和跨天学习时钟、按键手感、提示音音量、`perf` 实测帧耗时。
- 小学词表按课标附录核对、小披风饰品美术、发音（存不下 7500 个录音）。
