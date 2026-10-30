<p align="right"><strong>简体中文</strong> · <a href="GROK_SLEEP_CHEER.md">English</a></p>

# Grok 交接：12 段打瞌睡和大欢呼视频

这是路线图第 4 节 A 级缺口：初中到毕业的 **04 打瞌睡、05 大欢呼**。它与旧的 [GROK_REMAINING.md](GROK_REMAINING.md) 中四级至毕业 01–03 批次分开。已有 01–03 视频和小学全部视频保持不变。

**状态：12 条任务已收到 14 个原始 MP4；最终接入仍待完成。** [GROK_SLEEP_CHEER.json](GROK_SLEEP_CHEER.json) 包含 12 个独立 Grok 任务、完整提示词、文件名及绑定 SHA256 的起始图引用。本批源片来自 Grok，没有改用其他视频生成服务。详见[源片审核](../preview/video_review/SLEEP_CHEER_REVIEW.zh_CN.md)：四级／毕业 sleep 选 v2，12 条候选片均通过源帧检查，毕业 cheer 单手持证的变化已获用户明确接受。

本地制作从[工作区交接文件](../../GROK_WORKSPACE_HANDOFF.txt)开始读。[中文总提示词](GROK_PROMPT.zh_CN.txt)和 [12 条逐镜完整中文提示词](grok-prompts/)已写明参考图与输出路径。原始 MP4 直接保存到本工作区的 `art/video/<stage>/`，Codex 从磁盘读取。

| 学段 | 必须使用的参考图 | 打瞌睡输出 | 大欢呼输出 |
| --- | --- | --- | --- |
| junior | [junior.png](../start/junior.png) | `art/video/junior/junior_04_sleep.mp4` | `art/video/junior/junior_05_cheer.mp4` |
| senior | [senior.png](../start/senior.png) | `art/video/senior/senior_04_sleep.mp4` | `art/video/senior/senior_05_cheer.mp4` |
| cet4 | [cet4.png](../start/cet4.png) | `art/video/cet4/cet4_04_sleep.mp4` | `art/video/cet4/cet4_05_cheer.mp4` |
| cet6 | [cet6.png](../start/cet6.png) | `art/video/cet6/cet6_04_sleep.mp4` | `art/video/cet6/cet6_05_cheer.mp4` |
| adv | [adv.png](../start/adv.png) | `art/video/adv/adv_04_sleep.mp4` | `art/video/adv/adv_05_cheer.mp4` |
| grad | [grad.png](../start/grad.png) | `art/video/grad/grad_04_sleep.mp4` | `art/video/grad/grad_05_cheer.mp4` |

## 锁定要求

遵循 [SHOT_LIST.md](SHOT_LIST.md) 表格第 04、05 条和 [ART_BRIEF.md](../../ART_BRIEF.md)。每条源视频 3–4 秒。使用对应已有起始图、固定正面机位、纯绿底 `#00B140`、不变的大小与构图，以及同一条水平空桌沿。从参考图的准确中性姿势开始，结束时回到同一姿势并短暂停留。

保留 NABO 的脸、眼睛、圆眼镜、发型、白帽、比例和正向 **NABO** 帽标，禁止镜像人物或帽标。服装按各学段锁定。四级耳机始终在颈部，进阶相机始终连在胸前肩带上，毕业保留白帽、黑金学士服和手中的唯一一卷毕业证。手和已有配件留在画内，不新增物体、文字或水印。

- **04 打瞌睡：** 缓慢低头枕在桌上的手臂，慢慢呼吸、轻微点头，再起身回到中性姿势。中间至少保留 1.2 秒稳定呼吸，以便后续截取循环。完整源视频要回到中性姿势；不能把“中性→趴下”的全过程直接循环。毕业装保持双手在桌沿处托住同一卷毕业证。
- **05 大欢呼：** 大笑、双手举过肩且不出画，再平滑放下回到中性姿势，带预备和缓入缓出。毕业装高举时可以单手持有同一卷毕业证，用户已于 2026-10-30 明确接受；不复制、不丢失、不替换，首尾回到原中性握法。

不要覆盖已有视频。修订版用 `_v2`、`_v3` 后缀，在对应学段 `NOTES.md` 及配对翻译中记录选用版本和缺陷。

## 验收与后续接入

验收必须播放真实视频，不能只看接触表：核对身份、正向帽标、手和配件、固定机位、动作完整及回到参考姿势。睡眠片段选择稳定呼吸区间并检查抽帧后的循环接缝。之后检查对齐后的 256×256 RGBA 帧、桌沿锚点 `(128,224)`、主页 110 像素尺寸和 48 像素头部角标。

12 条输出受剩余资源预算限制。路线图估算约 1.1 MB；必要时在目检后评估 sleep 使用 6 fps。估算不能代表最终打包大小。验收后的片段还要登记进 `tools/clips.json`、用 `tools/clip_to_sprites.py` 处理、完整打包并匹配应用资源编号，以及同步原型图集。这些接入工作仍等待实机验收以及剩余的源片／抽帧检查。

Build: NOT RUN（仅交接）。

Host tests: PASS（全部现有宿主测试，以及与 v0.4.1 逐字节一致的资源重打包）。

Device tests: NOT RUN。

Unverified: 连续播放、抽帧循环接缝、最终包大小、运行时效果和实屏显示。
