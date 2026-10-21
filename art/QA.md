<p align="right"><a href="QA.zh_CN.md">简体中文</a> · <strong>English</strong></p>

# Art acceptance — rounds 1–4

Reviewed on 2026-10-21 against `ART_BRIEF.md` section 10. Round 4 contains **50 canonical static PNGs: 24 companion forms, 12 props, 10 toys and 4 UI images** (three icon types, including two flame states). This is desktop art validation. Owner approval of the new designs and physical-screen acceptance remain open.

## Section 10 checklist

| # | Requirement | Result | Evidence and limits |
| --- | --- | --- | --- |
| 1 | NABO identity: face, glasses, hair, white cap and emblem retained | PASS, static review | Selected primary A, junior B and senior B retain the approved identity. Compare [outfits](preview/outfit_growth.png) and round 1 size proofs. No NABO files were changed in round 4. |
| 2 | Forward, unmirrored NABO cap lettering | PASS, static/source review | Starting images and previously reviewed video contact sheets show forward lettering. Tiny lettering at 48 px is an identity mark, not readable UI text. |
| 3 | Identity and happy/sad expressions readable at 110 px and 48 px | PARTIAL | Neutral identity was checked at both sizes. Happy/sad actions passed source-video review; the final packed animated 48 px head crops have not been accepted in this art round. |
| 4 | Head position and scale identical across stage outfits | PASS, selected starts | Primary/junior/senior pixels through y=830 are identical; the region from desk y=1070 down is identical. Cap top y=366 and desk center (512,1070) are unchanged. Final processed animation registration remains a separate check. |
| 5 | Starting images: flat green, horizontal empty desk, action margins | PASS | Three selected images are 1024×1280, green #00B140, desk #C88A52; cap-to-desk height 704 px = 55% of canvas height. Existing geometry reports and start files are retained. |
| 6 | Fixed video camera; return to starting pose; seamless loops | PARTIAL | All primary 01–05 sources (sleep v2) and junior/senior 01–03 sources passed [source review](preview/video_review/REVIEW.md) and [junior/senior review](preview/video_review/JUNIOR_SENIOR_REVIEW.md). Final loop seams, extraction, palette packing and runtime playback are not certified here. Sleep's middle 1.4–2.9 s remains a candidate segment. |
| 7 | Four forms read as growth; six species share a family style | PASS, desktop visual review | [Growth](preview/companion_growth.png), [family](preview/companion_family.png) and [native-size checks](preview/companion_size_check.png). Shared round bodies, dark colored outlines, cream eye highlights and small feet; each species retains its egg motif through the final form. Owner design review pending. |
| 8 | Props/toys visible on stage wall/table colors; 12 px silhouettes distinct | PASS, desktop visual review | [Prop proof](preview/props_check.png) uses corresponding stage backgrounds. [Toy proof](preview/toys_check.png) checks all ten at 12 px on all six wall/table pairs; [silhouettes](preview/toys_silhouette.png) show their different outlines. The cube's colored tile pattern identifies the block as a puzzle cube; fine tile counts are deliberately not preserved at 12 px. Owner readability review pending. |
| 9 | Each companion ≤32 colors, no broad subtle gradients | PASS | All 24 RGBA exports contain 10–16 visible RGB colors, with binary alpha and fixed palettes. Jiguang's pink body swatch removes quantization speckling. Source illustrations remain untouched. Counts are in [validation](design/round4/validation.json). |
| 10 | Native 240×320 home composition and exact ×2 version | PASS | [Home](preview/home_primary.png) is 240×320; [×2 home](preview/home_primary_2x.png) is 480×640 and is an exact nearest-neighbor enlargement. Primary A, Doudou baby and the original three prop samples remain in this composition. |

## File checks

`python3 art/tools/validate_round4.py` completed successfully after export. It checks all 50 expected paths, RGBA, actual transparent pixels, binary alpha, exact 4× canvas sizes, ≤32 visible colors, unique IDs, anchors and file hashes. Toys and UI are exact 4× pixel-grid enlargements of 12×12 art; active/inactive flames have identical alpha. All 27 ImageGen source images and receipts are retained. The 24 protected existing art/video files match the pre-round hashes, including all selected starts, original Doudou art, original three props and supplied videos.

Canonical metadata is in [manifest.json](manifest.json); a complete inventory and reproduction notes are in [the round 4 README](design/round4/README.md). Historical candidate/design groups are retained for provenance; integration should use `companion`, `props`, `toys`, `ui` for this static set. `anchor_px` uses the 4× file canvas, while `target_anchor_px` uses game pixels. The clock's PNG still has no hands.

## Remaining acceptance

- Owner review of the family and ten small toy silhouettes.
- Final game animation loops, happy/sad 48 px crops and runtime registration.
- Final packer sampling/alpha premixing and RGB565 physical display appearance.
- The additional 50 toys and eight accessories added as **round 5** are outside this delivery. Round 4 keeps its ten toys at `art/toys/<name>.png`; migration into `primary/` belongs to round 5.

Build: NOT RUN — art-only scope.  
Host tests: NOT RUN — asset validation passed; no application logic was changed.  
Device tests: NOT RUN.  
Unverified: the remaining acceptance items above.
