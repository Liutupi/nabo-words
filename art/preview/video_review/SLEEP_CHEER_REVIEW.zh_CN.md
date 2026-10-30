<p align="right"><strong>简体中文</strong> · <a href="SLEEP_CHEER_REVIEW.md">English</a></p>

# sleep / cheer 源视频：收片与帧检查

2026-10-30 已收到 **14 个原始 MP4，覆盖全部 12 条任务**。四级 sleep 和毕业 sleep 各有一条 v2 重拍。14 条均可完整解码：每条主视频 97 帧，共 1,358 帧，720×960、24 fps、4.041667 秒，即名义 4 秒加最后一帧的时长。原片中的音轨和附带缩略图均保留，未修改原片。

所选 12 条候选片均通过源帧检查。用户已于 2026-10-30 明确接受毕业 cheer 的单手持证动作；原始更严格提示词的偏差仍保留记录。所选 12 条现已接入 v0.4.2-preview.1，完成精灵抽帧、资源打包、原型同步及宿主验证；实机验收门槛仍未完成。

## 选用

| 学段 | sleep 候选 | cheer 候选 | 源片审核 |
| --- | --- | --- | --- |
| junior | [v1](../../video/junior/junior_04_sleep.mp4) | [v1](../../video/junior/junior_05_cheer.mp4) | 通过源帧检查 |
| senior | [v1](../../video/senior/senior_04_sleep.mp4) | [v1](../../video/senior/senior_05_cheer.mp4) | 通过源帧检查 |
| cet4 | [v2](../../video/cet4/cet4_04_sleep_v2.mp4) | [v1](../../video/cet4/cet4_05_cheer.mp4) | 通过源帧检查；保留未选用的 sleep v1 |
| cet6 | [v1](../../video/cet6/cet6_04_sleep.mp4) | [v1](../../video/cet6/cet6_05_cheer.mp4) | 通过源帧检查 |
| adv | [v1](../../video/adv/adv_04_sleep.mp4) | [v1](../../video/adv/adv_05_cheer.mp4) | 通过源帧检查 |
| grad | [v2](../../video/grad/grad_04_sleep_v2.mp4) | [v1，已接受握持变化](../../video/grad/grad_05_cheer.mp4) | 通过；用户已接受单手持证欢呼 |

生成方原始选版备注保留在配对的[选版说明](../../video/PICKS_sleep_cheer.zh_CN.md)。文件哈希和逐条选择见 [sleep_cheer_validation.json](sleep_cheer_validation.json) 及[任务清单](../../reference/GROK_SLEEP_CHEER.json)。

## 发现

- 四级 sleep v1 的桌沿形变、漂移，木色带检测在 7 帧无法找到稳定区域；毕业 sleep v1 也有桌沿漂移。两条 v2 在所有已解码的 240×320 检查帧里，左右桌沿均稳定在 y=268，后续抽帧应使用 v2。
- 所选 12 条候选片的桌沿侧边均为 y=268，未使用检测回退；人物边界均在画内，包括举起的手和毕业证。抽样目检保留 NABO 的脸、眼镜、白帽和正向帽标。四级耳机保持在颈部；进阶相机保持存在，或在低头动作中自然被遮挡。
- 毕业 cheer 始终只有一卷毕业证，但从起始双手握持变成欢呼高举时单手持证。抬起和放下过程中证书可见，**不完全符合更严格的双手握持提示词**。见[密集过渡检查图](grad_cheer_diploma_detail.png)。用户已于 2026-10-30 明确接受这一变化。保留同一卷毕业证的连续性与本条确认记录，不把它描述成完全符合原始双手握持指令。
- 视频是 3:4，已批准起始图是 4:5。首尾对比图仅为等尺寸观察，对参考图做等比缩放和居中裁切；没有拉伸或覆盖任何原图、原视频。
- 首尾帧在目检中回到中性姿势，存在压缩级差异。这不能证明睡眠循环已经无缝；选择稳定呼吸区间时必须排除趴下和起身过渡。

本目录为每条原片提供 `_contact.png` 时序图和 `_endpoints.png` 首尾对照图。[metadata.json](metadata.json) 记录全帧解码、边界、桌沿测量及帧差。桌沿检测现使用左右两侧宽木色带，避免先前把初中 cheer 举起的手误判为桌沿。

## 运行时接入

帧段、限色参数与预算见[接入交接说明](../../reference/GROK_SLEEP_CHEER.zh_CN.md)。新增 sleep 为 24 帧 / 6 fps，循环排除了提前起身的帧；cheer 裁去中性姿势停顿后保留 39 帧 / 12 fps。已目检生产渲染器原生尺寸的睡眠与欢呼图；7 套服装均通过解码／跳帧，以及趴下、循环、唤醒、奖励和待机转换检查。保留用户已接受的毕业握持动作。

Build: PASS。Host tests: PASS（HANDOFF 全套、29,278 项界面检查）。Device tests: NOT RUN（本次用户暂缓刷机）。

连续播放验收、实屏显示与耗时仍待完成。以前的实机观察属于 v0.4.1，不能作为本版文件的实机结果。精确文件身份与验证范围见 [validation/v0.4.2-preview.1.json](../../../validation/v0.4.2-preview.1.json)。
