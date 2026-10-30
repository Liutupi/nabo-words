<p align="right"><a href="SLEEP_CHEER_REVIEW.zh_CN.md">简体中文</a> · <strong>English</strong></p>

# Sleep/cheer sources: receipt and frame review

Received on 2026-10-30: **14 original MP4 files covering all 12 requested slots**. CET4 sleep and graduation sleep each have a v2 retake. All 14 sources decode successfully: 97 primary-video frames each, 1,358 frames total, 720x960, 24 fps and 4.041667 seconds. This is nominally four seconds plus the final frame duration. Audio and attached thumbnails remain in the untouched originals.

The source-frame checks pass for all twelve selected candidates. The user explicitly accepted the graduation single-hand diploma grip on 2026-10-30; the deviation from the original stricter prompt is documented. The twelve selected clips are now integrated in v0.4.2-preview.1. Runtime extraction, packing, prototype sync and host validation are complete; the device-acceptance gate remains open.

## Selection

| Stage | Sleep candidate | Cheer candidate | Source review |
| --- | --- | --- | --- |
| junior | [v1](../../video/junior/junior_04_sleep.mp4) | [v1](../../video/junior/junior_05_cheer.mp4) | Pass source-frame checks |
| senior | [v1](../../video/senior/senior_04_sleep.mp4) | [v1](../../video/senior/senior_05_cheer.mp4) | Pass source-frame checks |
| cet4 | [v2](../../video/cet4/cet4_04_sleep_v2.mp4) | [v1](../../video/cet4/cet4_05_cheer.mp4) | Pass source-frame checks; retain rejected sleep v1 |
| cet6 | [v1](../../video/cet6/cet6_04_sleep.mp4) | [v1](../../video/cet6/cet6_05_cheer.mp4) | Pass source-frame checks |
| adv | [v1](../../video/adv/adv_04_sleep.mp4) | [v1](../../video/adv/adv_05_cheer.mp4) | Pass source-frame checks |
| grad | [v2](../../video/grad/grad_04_sleep_v2.mp4) | [v1, accepted grip variation](../../video/grad/grad_05_cheer.mp4) | Pass; single-hand cheer accepted by the user |

The producer's original picks are preserved in the paired [selection notes](../../video/PICKS_sleep_cheer.md). File identities and per-slot selections are in [sleep_cheer_validation.json](sleep_cheer_validation.json) and [the task manifest](../../reference/GROK_SLEEP_CHEER.json).

## Findings

- CET4 sleep v1 deforms/shifts the desk. The wood-band diagnostic cannot find a stable band in seven frames. Graduation sleep v1 also shifts the desk. Their v2 sources keep the side desk line at y=268 in every decoded 240x320 diagnostic frame; use v2 for later extraction.
- All twelve selected candidates keep the desk side band at y=268 with no diagnostic fallback. Character bounds stay inside the frame, including raised hands and the graduation diploma. Sampled visual checks retain the NABO face, glasses, white cap and forward emblem. CET4 headphones stay at the neck; the advanced camera remains present or naturally occluded during the head-rest action.
- Graduation cheer retains one diploma, but releases the two-handed starting grip and holds it in one hand during the raised-arm pose. The roll is visible along the lift and return; this does **not** meet the stricter two-handed-grip prompt exactly. See the [dense transition proof](grad_cheer_diploma_detail.png). The user explicitly accepted this variation on 2026-10-30. Preserve one continuous diploma and retain this approval note; do not describe the source as matching the original two-handed instruction exactly.
- The supplied videos are 3:4; the approved starts are 4:5. Endpoint comparison sheets uniformly resize and center-crop the reference for like-sized viewing; no original image or video is stretched or overwritten.
- First/last frames visibly return to the neutral pose, with compression-level differences. This does not certify a seamless sleep loop: the neutral-to-sleep and wake transitions must be excluded when selecting a steady middle breathing interval.

Each source has a sequential `_contact.png` sheet and `_endpoints.png` comparison in this directory. [metadata.json](metadata.json) records full-frame decoding, bounds, desk measurements and differences. The desk detector now uses a broad wood-colored band on both sides, avoiding the earlier false detection of raised hands in junior cheer.

## Runtime integration

See [the integration handoff](../../reference/GROK_SLEEP_CHEER.md) for frame ranges, palette settings and budget. New sleep uses 24 frames at 6 fps; its loop excludes early wake-up frames. Cheer retains 39 frames at 12 fps after trimming neutral holds. Native-size production renderer images were inspected for sleep and cheer; all seven outfits pass decode/seek and entry/loop/wake/reward/idle transitions. The user-approved graduation grip is retained.

Build: PASS. Host tests: PASS (HANDOFF suite, 29,278 UI checks). Device tests: NOT RUN for this revision; flashing was postponed by the user.

Continuous-playback acceptance and physical display/timing remain unverified. Previous device observations belong to v0.4.1 and do not validate these artifacts. Exact images and test scope are recorded in [validation/v0.4.2-preview.1.json](../../../validation/v0.4.2-preview.1.json).
