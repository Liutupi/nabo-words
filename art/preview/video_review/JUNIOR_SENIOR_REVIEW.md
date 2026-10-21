<p align="right"><a href="JUNIOR_SENIOR_REVIEW.zh_CN.md">简体中文</a> · <strong>English</strong></p>

# Junior and senior video source review — 2026-10-21

**All six sources are usable for frame processing. No Grok revision is needed for the issues checked here.** Junior matches selected B (white torso, blue shoulders/sleeves); senior matches selected B (navy blazer, light lapel trim, white shirt, red tie).

| Source | Result | Visual finding |
| --- | --- | --- |
| `junior_01_idle.mp4` | Usable | Blink and slight head tilt/breathing, then returns to neutral. |
| `junior_02_happy.mp4` | Usable | Smiling eyes, both fists rise near chest and a small bounce, then hands return. |
| `junior_03_sad.mp4` | Usable | Lowered head/shoulders and subdued expression, then recovery. |
| `senior_01_idle.mp4` | Usable | Blink and gentle tilt/breathing, then returns to neutral. |
| `senior_02_happy.mp4` | Usable | Clear happy face and small fist raises; blazer trim and tie remain consistent. |
| `senior_03_sad.mp4` | Usable | Lowered head/shoulders and sad face, then returns to neutral. |

All six decode successfully: **720×960, 24 fps, 97 frames, approximately 4.04 seconds each**. All 582 motion frames were decoded for metadata and position checks. Twelve evenly spaced frames per clip were inspected visually; the idle endpoint sheets were also checked against the approved stage images. The sampled sequences show no obvious identity swap, clothing swap, mirrored cap lettering, missing action, clipping or new desk objects.

Desk height is stable at y=268 in every 240×320 diagnostic frame, and first/last subject bounding boxes agree. First/last whole-frame mean RGB differences range from 0.948 to 1.288 on a 0–255 scale. These are source review findings, not a claim of pixel-identical frames or a proven seamless loop. Green-background compression, fine outline drift and the final frame selection still need processing.

The videos include AAC audio and attached thumbnails. Motion review uses the H.264 stream, not the thumbnails. No original video, prototype code or existing sprite output was changed in this review. Source hashes and details are in [metadata.json](metadata.json).

For each source, `*_contact.png` shows the time sequence and `*_endpoints.png` compares the approved start image with first/last frames. Those diagnostics are not final game sprites. Junior/senior keying, stabilization, 256×256 anchor alignment, palette reduction, loop/single-shot timing and native-size/physical-screen rendering remain to be verified in the frame-processing stage.

The next art stage in `CODEX_PROMPTS.md` is round 4: remaining companion forms, props, toys and UI icons. No additional junior/senior videos are requested by this review.
