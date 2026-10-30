<p align="right"><a href="GROK_SLEEP_CHEER.zh_CN.md">简体中文</a> · <strong>English</strong></p>

# Grok handoff: twelve sleep and cheer clips

This is the roadmap section 4 A-priority gap: shots **04 sleep and 05 cheer** for junior through graduation. It is a separate batch from the older CET4–graduation 01–03 handoff in [GROK_REMAINING.md](GROK_REMAINING.md). Existing 01–03 clips and all primary clips remain unchanged.

**Status: all twelve slots received as fourteen original MP4s; final integration remains pending.** [GROK_SLEEP_CHEER.json](GROK_SLEEP_CHEER.json) contains twelve independent Grok jobs, exact prompts, filenames and SHA256-bound starting-image references. The supplied sources are from Grok. No substitute provider has been used. See [the source review](../preview/video_review/SLEEP_CHEER_REVIEW.md): CET4/graduation sleep use v2, all twelve candidates pass source-frame checks, with the graduation single-hand grip explicitly accepted by the user.

For the local production workspace, start with [the workspace handoff](../../GROK_WORKSPACE_HANDOFF.txt). The [Chinese batch prompt](GROK_PROMPT.zh_CN.txt) and [twelve standalone Chinese shot prompts](grok-prompts/) include reference and output paths. Save original MP4s directly in this checkout's `art/video/<stage>/`; Codex reads them from disk.

| Stage | Required reference | Sleep output | Cheer output |
| --- | --- | --- | --- |
| junior | [junior.png](../start/junior.png) | `art/video/junior/junior_04_sleep.mp4` | `art/video/junior/junior_05_cheer.mp4` |
| senior | [senior.png](../start/senior.png) | `art/video/senior/senior_04_sleep.mp4` | `art/video/senior/senior_05_cheer.mp4` |
| cet4 | [cet4.png](../start/cet4.png) | `art/video/cet4/cet4_04_sleep.mp4` | `art/video/cet4/cet4_05_cheer.mp4` |
| cet6 | [cet6.png](../start/cet6.png) | `art/video/cet6/cet6_04_sleep.mp4` | `art/video/cet6/cet6_05_cheer.mp4` |
| adv | [adv.png](../start/adv.png) | `art/video/adv/adv_04_sleep.mp4` | `art/video/adv/adv_05_cheer.mp4` |
| grad | [grad.png](../start/grad.png) | `art/video/grad/grad_04_sleep.mp4` | `art/video/grad/grad_05_cheer.mp4` |

## Locked requirements

Follow [SHOT_LIST.md](SHOT_LIST.md), table rows 04 and 05, and [ART_BRIEF.md](../../ART_BRIEF.md). Each source is 3–4 seconds. Use the corresponding existing start, a fixed front camera, flat green `#00B140`, unchanged scale/framing and the same horizontal empty desk. Begin and end at the reference's exact neutral pose, with a brief final hold.

Preserve NABO's face, eyes, round glasses, hair, white cap, proportions and forward **NABO** cap emblem. Never mirror the character or emblem. Preserve each stage's locked outfit. CET4 headphones remain around the neck; the advanced camera stays on its chest strap; graduation keeps the white cap, black/gold gown and exactly one held diploma. Keep hands and existing accessories inside frame; do not introduce objects, text or watermarks.

- **04 sleep:** lower the head onto folded arms, breathe slowly with a tiny natural nod, then rise back to neutral. Include at least 1.2 seconds of steady middle breathing for later loop extraction. The full video returns to neutral; do not loop the full neutral-to-sleep transition. Graduation retains the one diploma in the hands at the desk edge.
- **05 cheer:** smile broadly, raise both hands above the shoulders within frame, lower them smoothly and return to neutral. Use anticipation and easing. Graduation may hold the single diploma in one hand during the raised pose, as explicitly accepted by the user on 2026-10-30; do not duplicate, drop or replace it. Return to the original neutral grip at the endpoints.

Do not overwrite existing sources. Name revisions `_v2`, `_v3`, and record the selection and defects in the corresponding stage's `NOTES.md` with its paired translation.

## Acceptance and later integration

Review the actual video, not only a contact sheet: identity, unmirrored emblem, hands/accessories, fixed camera, action completion and reference-pose return. For sleep, choose a steady breathing interval and verify its extracted seam. Then check registered 256x256 RGBA frames with desk anchor `(128,224)`, the 110-pixel home size and the 48-pixel head crop.

Keep twelve outputs within the remaining asset budget. The roadmap estimate is about 1.1 MB; if needed, evaluate sleep at 6 fps after visual review. This estimate does not certify packed size. Accepted clips must later be registered in `tools/clips.json`, processed by `tools/clip_to_sprites.py`, fully packed with matching app asset IDs, and synchronized to the prototype atlas. Those changes remain pending device acceptance and the remaining source/extraction checks.

Build: NOT RUN (handoff only).

Host tests: PASS (full existing host suite and byte-identical v0.4.1 asset repack).

Device tests: NOT RUN.

Unverified: continuous playback, extracted loop seams, final packed size, runtime appearance and physical display.
