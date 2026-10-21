<p align="right"><a href="REVIEW.zh_CN.md">简体中文</a> · <strong>English</strong></p>

# Primary video source review — 2026-10-21

**All five actions now have usable sources.** Sleep uses `primary_04_sleep_v2.mp4`; the original 04 is retained as superseded. This is a source-art review, not a firmware or on-device acceptance result.

All five supplied MP4s decode successfully: 720×960, 24 fps, 97 frames, about 4.04 seconds. They include AAC audio and an attached thumbnail; the attached thumbnail is not the motion stream. No original video was changed. Source hashes and measured metadata are in [metadata.json](metadata.json).

| Shot | Source review | Finding |
| --- | --- | --- |
| 01 idle | Usable | Slight breathing/head tilt and blinks; returns to the neutral pose. Two blinks rather than exactly one is acceptable for this idle source. |
| 02 happy | Usable | Smiling eyes, small fist raises and a bounce; returns to neutral. |
| 03 sad | Usable | Lowered head/shoulders and subdued face, then recovery. |
| 04 sleep | **v2 usable** | V2 keeps both eyes closed through the prone sleeping section, then returns to neutral. Approximately 1.4–2.9 s is a candidate stable sleep segment; a seamless loop still needs extraction/verification. The original open-eyed 04 is superseded. |
| 05 cheer | Usable | Both hands rise above shoulders, a clear joyful expression, then return. |

The sampled whole-frame sequences show consistent A clothing, round glasses, white cap and forward NABO lettering. Desk height is stable at y=268 in the 240×320 diagnostic resampling across all 485 decoded motion frames; first/last subject positions are close. This measurement supports a stable desk/camera, but does not prove perfect registration of every feature or a seamless loop. Original green texture/compression and small outline differences still require keying/normalization. Final action-frame selection, loop seams, palette reduction and RGB565-screen appearance remain to be checked.

The 12-frame `*_contact.png` sheets show each action in time order. `*_endpoints.png` compare the selected start image against first/last frames. `primary_01_idle_eye_sequence.png` and `primary_04_sleep_eye_sequence.png` show every decoded frame around the face and establish the blink/sleep findings. Diagnostic sheets are not final game sprites.

## Completed Grok revision: 04 v2

Use `art/start/primary.png` again, preserving the selected A identity, yellow V-neck vest, brick-red straps, cap lettering, fixed front camera, green background and empty horizontal desk. Make a roughly four-second source: descend onto folded arms during the opening, **close both eyes fully and keep them closed while resting on the arms for at least the middle 1.5–2 seconds**, with only gentle breathing; return to the exact starting pose near the end. The middle stable sleeping section will supply the game loop. Avoid the current open-eyed chin-on-hands pose, talking or repeated head lifting.

Save as **`art/video/primary/primary_04_sleep_v2.mp4`**, keeping the original. Shots 01/02/03/05 need no regeneration for the issues checked here. The v2 source was reviewed in all 97 decoded face frames. Its desk remains at y=268 in the 240×320 diagnostic view. First/last posture is close. No further Grok revision is required for this source review. Round 3 clothing design is the next stage.

V2 diagnostics: [sequential frames](primary_04_sleep_v2_contact.png), [every face frame](primary_04_sleep_v2_eye_sequence.png), [endpoints](primary_04_sleep_v2_endpoints.png). Original source files are unchanged.
