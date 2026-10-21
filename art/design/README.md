<p align="right"><a href="README.zh_CN.md">简体中文</a> · <strong>English</strong></p>

# NABO Words — art rounds 1–4

Latest checkpoint: **round 4 is complete, with 50 canonical static PNGs ready for owner review**. See [the round 4 inventory](round4/README.md), [six-companion family](../preview/companion_family.png), [toy silhouettes](../preview/toys_silhouette.png) and [acceptance report](../QA.md). This round uses Codex built-in ImageGen; all prompts and sources are retained. No further Grok work is required for the received video sources. Final animated 48 px expression crops, loop seams and physical display validation remain separate. Round 5's additional toys/accessories are not included.

Review **[the contact sheet](../preview/round1_contact.png)** first. These are two primary-school clothing candidates, a four-form Doudou design and three prop samples. The owner selected **outfit A** on 2026-10-21. `art/start/primary.png` is an identical copy of `art/design/primary_a.png`; B remains an alternative.

## Review images

All paths below are relative to the project root `nabo-words/`.

| Image | Path | Details |
| --- | --- | --- |
| Round 1 overview | `art/preview/round1_contact.png` | Two outfits, size proofs, Doudou and home |
| Outfit A | `art/design/primary_a.png` | V-neck yellow vest, white T-shirt, brick-red backpack straps |
| A size proof | `art/design/primary_a_check.png` | 110 px cap-to-desk and 48 px head; nearest-neighbor ×4 |
| Outfit B | `art/design/primary_b.png` | Round-neck yellow vest, white T-shirt, dark teal straps and broad pocket |
| B size proof | `art/design/primary_b_check.png` | Same two target sizes and backgrounds; nearest-neighbor ×4 |
| Doudou evolution sheet | `art/design/doudou_sheet.png` | Egg, baby, teen, final; transparent background |
| Doudou size proof | `art/design/doudou_check.png` | 28/32/40/48 px on the primary tabletop; nearest-neighbor ×4 |
| Egg design sprite | `art/design/doudou_egg.png` | 25×28 target; 100×112 transparent PNG |
| Baby design sprite | `art/design/doudou_baby.png` | 23×32 target; 92×128 transparent PNG |
| Teen design sprite | `art/design/doudou_teen.png` | 29×40 target; 116×160 transparent PNG |
| Final design sprite | `art/design/doudou_final.png` | 37×48 target; 148×192 transparent PNG |
| Desk lamp | `art/props/lamp.png` | 24×44 target; 96×176 transparent PNG |
| Alarm clock | `art/props/clock.png` | 28×28 target; 112×112 transparent PNG; no hands |
| Pencil case | `art/props/pencilcase.png` | 34×12 target; 136×48 transparent PNG |
| Home composition | `art/preview/home_primary.png` | 240×320 native size; outfit A + baby + all three props |
| Enlarged home | `art/preview/home_primary_2x.png` | 480×640, exact nearest-neighbor ×2 |

The contact sheet contains reduced summaries of the individual checks. Use the individual `*_check.png` files for their exact ×4 proofs.

## Simplifications for the display

- NABO keeps the reference's white cap, forward-facing NABO lettering and blue emblem, dark tousled hair, round glasses, blue-gray eyes and small smile. Only the primary-school outfit changes. Broad V/round neck shapes, large buckles and contrasting straps survive reduction; fine stitching, fabric texture and accessories are omitted.
- A fixed 32-swatch palette flattens NABO's lighting and limits fine gradients. Hair is grouped into dark masses while keeping the reference silhouette. At a 48 px head height, the cap/glasses/hair/eyes identify NABO; the tiny cap lettering should not be treated as readable UI text.
- Doudou retains a bean silhouette, forest-green outline and shared eyes across forms. The egg has leaf markings; the baby has paired seed leaves; the teen gains arms and larger leaves; the final gains a three-leaf coronet with one gold seed. Leaf veins, extra jewelry, complex fingers and texture are omitted. The exported forms contain **11/16/16/16 visible RGB colors**, below the 32-color limit. Widths are padded to exact multiples of four, with bottom-center anchors.
- The lamp has a broad shade and base, the clock has four broad indices and an empty center, and the pencil case uses a broad coral capsule with one zipper pull. Exports use a fixed 16-swatch palette. No clock hands are baked into the PNG.
- Native-size images are reduced first, then enlarged with nearest-neighbor sampling. The ×4 proofs do not use smoothing. Alpha in the sample sprites is binary; a later packer may add edge premixing if appropriate.

## Layout and checks

Both starting candidates are **1024×1280**. The flat green background is exactly **#00B140**, and the empty wood band is **#C88A52**. The cap top is at y=366, the horizontal desk edge at y=1070 and its center at x=512: 704 px, exactly 55% of the canvas height. Hands rest at and slightly over the table edge. `art/manifest.json` records candidate geometry, prop sizes and anchors.

The home follows the prototype's wall/trim/table/body colors and major positions: NABO center x=120, cap top y=66, table edge y=176, clock center (34,52), lamp base near (29,176), Doudou base (196,176), pencil case in front of the table edge. UI text is drawn only as composition context. The prototype files are unchanged.

Verified locally: expected PNG sizes, true prop/companion transparency, flat background swatches, companion color counts, integer enlargement, unclipped sheet cells, and changes confined to `art/`. Visual review covered neutral NABO identity, cap spelling/orientation, both small sizes, consistent Doudou growth, no clock hands, and the native home composition. **The owner selected outfit A; Doudou/prop design approval remains pending.** Happy/sad expressions, animation consistency and physical RGB565-screen rendering are not covered by this static round.

## Sources and reproduction

Illustrations were generated with **Codex built-in ImageGen**. The identity reference was the left, open-eyed frame of `../ai-passport-nabo-xiaozhi/docs/images/nabo-idle.png`. No mirror was applied. The production prompt set is in `art/design/prompts.json`.

Original generated images are retained in `art/design/source/`: `primary_a.png`, `primary_b.png`, `doudou_sheet.png`, `lamp.png`, `clock.png`, `pencilcase.png`. These are provenance inputs, not final exports. `primary_a_cutout.png` and `primary_b_cutout.png` are derived normalization/compositing inputs. The raw green backgrounds and shading were normalized for the delivered files.

To repeat the requested sizing, palette reduction, checks and composition from those sources: `python3 art/tools/build_round1.py` (Pillow, numpy and the local PingFang font via fontconfig). This rebuilds only files under `art/`; it does not invoke paid generation or alter the source illustrations. `art/design/validation.json` records the final local asset checks.

## Next handoff

Outfit A is locked as `art/start/primary.png`. Primary video sources 01–05 have now passed source review, using `art/video/primary/primary_04_sleep_v2.mp4` for sleep. The approximate middle 1.4–2.9 s is a candidate sleep segment, pending final loop extraction. See [the video review](../preview/video_review/REVIEW.md). No final game animation frames have been exported here. **Junior B and senior B are selected, and all six 01–03 source videos have passed source review.** See [the junior/senior video review](../preview/video_review/JUNIOR_SENIOR_REVIEW.md). They can proceed to frame processing; no Grok revision is requested.

## Round 3 — junior and senior clothing candidates

Review [all four options](../preview/round3_contact.png) and the [growth comparison](../preview/outfit_growth.png). The owner selected **junior B and senior B** on 2026-10-21. The growth comparison now shows primary A → junior B → senior B, all marked as selected.

| Candidate | Starting image | Exact ×4 size proof | Clothing |
| --- | --- | --- | --- |
| Junior A | `art/design/junior_a.png` | `art/design/junior_a_check.png` | Blue torso, white sleeves, simple central zipper |
| Junior B | `art/design/junior_b.png` | `art/design/junior_b_check.png` | White torso, broad blue shoulder/chest panel and blue sleeves |
| Senior A | `art/design/senior_a.png` | `art/design/senior_a_check.png` | Plain navy blazer, white shirt, red tie |
| Senior B | `art/design/senior_b.png` | `art/design/senior_b_check.png` | Navy blazer with light lapel trim, white shirt, red tie |

Every candidate is 1024×1280. The complete upper region through y=830, including cap/hair/face/glasses, is copied from the selected primary image with **zero pixel difference**. The table, lower hands and background from y=1070 down are also identical; original hand pixels near the edge are restored. The only changed region is the clothing band between jaw and desk. This is a deterministic identity-preserving composite of ImageGen garment artwork, not a new head generated for each outfit.

Geometry remains cap top y=366, desk edge y=1070, center x=512. Junior proofs use #BFE3E0 wall / #B47C55 table; senior proofs use #C9D3EE / #916243. The 48 px head uses the same primary reference on #FFF8EC. Each individual proof is native size enlarged exactly ×4; the four-option overview uses ×3 for compact comparison. Junior exports have 30 visible sprite colors; senior exports have 32. No real school badges, logos or scarves are included. The long sleeves are presented with the forearm area clear so the original hand placement remains unchanged.

Source images and derived cutouts are retained as `art/design/source/junior_a.png`, `junior_b.png`, `senior_a.png`, `senior_b.png` and their `*_cutout.png` siblings. Exact production prompts are in `art/design/round3_prompts.json`; local checks are in `art/design/round3_validation.json`; exports can be rebuilt with `python3 art/tools/build_round3.py`. `art/start/junior.png` is an exact copy of `art/design/junior_b.png`; `art/start/senior.png` is an exact copy of `art/design/senior_b.png`. The six shots 01–03 (idle/happy/sad) for these stages are now received under `art/video/junior/` and `art/video/senior/` and have passed source review. Selection hashes are in `art/manifest.json`.

### Junior/senior video checkpoint

All six videos in `art/video/junior/` and `art/video/senior/` have been received and reviewed. They retain the selected B uniforms and complete their respective idle/happy/sad actions, returning to neutral. Source review is complete; final frame processing and native-screen verification remain. Round 4's static delivery is now complete; see the latest checkpoint above.
