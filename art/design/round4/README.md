<p align="right"><a href="README.zh_CN.md">简体中文</a> · <strong>English</strong></p>

# Round 4 — companions, props, toys and UI

Completed on 2026-10-21. **50 canonical 4× transparent PNGs** are available: six companions × four forms, twelve props, ten primary-school toys, and four UI images. The UI set is three icon types with active/inactive flame variants. Seven files reuse the existing approved exports byte-for-byte: Doudou's four forms and the lamp, clock and pencil case.

## Review first

- [Six final companions](../../preview/companion_family.png)
- [All growth forms](../../preview/companion_growth.png) and [native-size proof](../../preview/companion_size_check.png)
- [Twelve props](../../preview/props_check.png)
- [Ten toys and solid silhouettes](../../preview/toys_silhouette.png), [12 px on all stage backgrounds](../../preview/toys_check.png)
- [UI on light/dark backgrounds](../../preview/ui_check.png)
- [Acceptance results](../../QA.md) and [machine checks](validation.json)

Companion proofs reduce the actual exported files to the target heights before nearest-neighbor enlargement. Toys/UI are drawn onto a 12×12 pixel grid and exported at 48×48. Prop and companion exports preserve more edge detail at 4× for the final packer. Native-size proofs use Lanczos for these reductions; the final packer must verify its chosen sampler and RGB565 appearance.

## Design decisions

All six companions share large dark oval eyes with cream highlights, peach cheeks, small smiles, round bodies and short feet. Leaves, water, star/book pages, flame, wave/shell and aurora/feathers identify the species. The final forms gain different completion features: Doudou's leaf coronet, Paopao's halo, Xingxing's golden star and book wings, Huohuo's flame crest and medallion, Langlang's wave and shells, and Jiguang's feather crown and wings. Each retains its egg motif.

Exports use 10–16 visible colors per companion. Fixed palettes remove subtle broad gradients. Jiguang includes a dedicated pink body swatch to avoid speckling between two nearby palette entries. Props and toys keep broad recognizable structures; fine writing, needles, fabric grain and tiny decorations are omitted. The clock has no baked hands. Dark outlines use subject colors rather than a white sticker border.

## Asset inventory

Paths below are relative to `art/`. **Target** is in game pixels, **file** is the 4× canvas, and **anchor** is measured in game pixels from the canvas top-left. Exact file-space anchors, palettes and SHA-256 hashes are in [manifest.json](../../manifest.json). Bottom-center is used for companions, tabletop props and toys; the clock, certificate, map and UI use the center. Transparent horizontal padding is included in the companion canvas width.

### Companions

| Path | Target | File | Anchor | Colors |
| --- | --- | --- | --- | --- |
| [`companion/doudou/egg.png`](../../companion/doudou/egg.png) | 25×28 | 100×112 | (12.5, 28) | 11 |
| [`companion/doudou/baby.png`](../../companion/doudou/baby.png) | 23×32 | 92×128 | (11.5, 32) | 16 |
| [`companion/doudou/teen.png`](../../companion/doudou/teen.png) | 29×40 | 116×160 | (14.5, 40) | 16 |
| [`companion/doudou/final.png`](../../companion/doudou/final.png) | 37×48 | 148×192 | (18.5, 48) | 16 |
| [`companion/paopao/egg.png`](../../companion/paopao/egg.png) | 25×28 | 100×112 | (12.5, 28) | 12 |
| [`companion/paopao/baby.png`](../../companion/paopao/baby.png) | 25×32 | 100×128 | (12.5, 32) | 16 |
| [`companion/paopao/teen.png`](../../companion/paopao/teen.png) | 34×40 | 136×160 | (17, 40) | 16 |
| [`companion/paopao/final.png`](../../companion/paopao/final.png) | 36×48 | 144×192 | (18, 48) | 16 |
| [`companion/xingxing/egg.png`](../../companion/xingxing/egg.png) | 25×28 | 100×112 | (12.5, 28) | 12 |
| [`companion/xingxing/baby.png`](../../companion/xingxing/baby.png) | 27×32 | 108×128 | (13.5, 32) | 14 |
| [`companion/xingxing/teen.png`](../../companion/xingxing/teen.png) | 40×40 | 160×160 | (20, 40) | 14 |
| [`companion/xingxing/final.png`](../../companion/xingxing/final.png) | 47×48 | 188×192 | (23.5, 48) | 16 |
| [`companion/huohuo/egg.png`](../../companion/huohuo/egg.png) | 25×28 | 100×112 | (12.5, 28) | 14 |
| [`companion/huohuo/baby.png`](../../companion/huohuo/baby.png) | 25×32 | 100×128 | (12.5, 32) | 14 |
| [`companion/huohuo/teen.png`](../../companion/huohuo/teen.png) | 36×40 | 144×160 | (18, 40) | 16 |
| [`companion/huohuo/final.png`](../../companion/huohuo/final.png) | 40×48 | 160×192 | (20, 48) | 16 |
| [`companion/langlang/egg.png`](../../companion/langlang/egg.png) | 25×28 | 100×112 | (12.5, 28) | 10 |
| [`companion/langlang/baby.png`](../../companion/langlang/baby.png) | 24×32 | 96×128 | (12, 32) | 16 |
| [`companion/langlang/teen.png`](../../companion/langlang/teen.png) | 31×40 | 124×160 | (15.5, 40) | 15 |
| [`companion/langlang/final.png`](../../companion/langlang/final.png) | 40×48 | 160×192 | (20, 48) | 16 |
| [`companion/jiguang/egg.png`](../../companion/jiguang/egg.png) | 24×28 | 96×112 | (12, 28) | 12 |
| [`companion/jiguang/baby.png`](../../companion/jiguang/baby.png) | 24×32 | 96×128 | (12, 32) | 15 |
| [`companion/jiguang/teen.png`](../../companion/jiguang/teen.png) | 34×40 | 136×160 | (17, 40) | 14 |
| [`companion/jiguang/final.png`](../../companion/jiguang/final.png) | 46×48 | 184×192 | (23, 48) | 14 |

### Props

| Path | Target | File | Anchor | Colors |
| --- | --- | --- | --- | --- |
| [`props/lamp.png`](../../props/lamp.png) | 24×44 | 96×176 | (12, 44) | 14 |
| [`props/clock.png`](../../props/clock.png) | 28×28 | 112×112 | (14, 14) | 8 |
| [`props/pencilcase.png`](../../props/pencilcase.png) | 34×12 | 136×48 | (17, 12) | 12 |
| [`props/plant.png`](../../props/plant.png) | 16×24 | 64×96 | (8, 24) | 14 |
| [`props/globe.png`](../../props/globe.png) | 24×30 | 96×120 | (12, 30) | 17 |
| [`props/shelf.png`](../../props/shelf.png) | 66×8 | 264×32 | (33, 8) | 5 |
| [`props/cert.png`](../../props/cert.png) | 36×28 | 144×112 | (18, 14) | 21 |
| [`props/phones.png`](../../props/phones.png) | 24×14 | 96×56 | (12, 14) | 11 |
| [`props/laptop.png`](../../props/laptop.png) | 56×30 | 224×120 | (28, 30) | 13 |
| [`props/mug.png`](../../props/mug.png) | 18×16 | 72×64 | (9, 16) | 9 |
| [`props/map.png`](../../props/map.png) | 60×38 | 240×152 | (30, 19) | 23 |
| [`props/trophy.png`](../../props/trophy.png) | 16×30 | 64×120 | (8, 30) | 8 |

### Toys

| Path | Target | File | Anchor | Colors |
| --- | --- | --- | --- | --- |
| [`toys/duck.png`](../../toys/duck.png) | 12×12 | 48×48 | (6, 12) | 7 |
| [`toys/rocket.png`](../../toys/rocket.png) | 12×12 | 48×48 | (6, 12) | 17 |
| [`toys/snowglobe.png`](../../toys/snowglobe.png) | 12×12 | 48×48 | (6, 12) | 13 |
| [`toys/cactus.png`](../../toys/cactus.png) | 12×12 | 48×48 | (6, 12) | 10 |
| [`toys/cube.png`](../../toys/cube.png) | 12×12 | 48×48 | (6, 12) | 17 |
| [`toys/plane.png`](../../toys/plane.png) | 12×12 | 48×48 | (6, 12) | 10 |
| [`toys/dino.png`](../../toys/dino.png) | 12×12 | 48×48 | (6, 12) | 8 |
| [`toys/hourglass.png`](../../toys/hourglass.png) | 12×12 | 48×48 | (6, 12) | 14 |
| [`toys/shell.png`](../../toys/shell.png) | 12×12 | 48×48 | (6, 12) | 7 |
| [`toys/windmill.png`](../../toys/windmill.png) | 12×12 | 48×48 | (6, 12) | 14 |

### UI

| Path | Target | File | Anchor | Colors |
| --- | --- | --- | --- | --- |
| [`ui/coin.png`](../../ui/coin.png) | 12×12 | 48×48 | (6, 6) | 6 |
| [`ui/flame.png`](../../ui/flame.png) | 12×12 | 48×48 | (6, 6) | 10 |
| [`ui/freeze.png`](../../ui/freeze.png) | 12×12 | 48×48 | (6, 6) | 11 |
| [`ui/flame_dim.png`](../../ui/flame_dim.png) | 12×12 | 48×48 | (6, 6) | 3 |

## Sources and reproduction

All 27 new source images were produced with **Codex built-in ImageGen**. Exact prompts and reference paths are in [prompts.json](prompts.json); each generated original is retained in `source/<job-id>.png` and its original saved location is recorded in `receipts/<job-id>.json`. The five companion sheets each contain four forms. The inactive flame is a deterministic grayscale variant of the active flame, with identical alpha. No CLI/API fallback was used.

Run `python3 art/tools/build_round4.py`, then `python3 art/tools/validate_round4.py` from the project root to repeat normalization, sizing, palette export and file checks. These commands write only within `art/`; they do not regenerate illustrations, edit prototypes, extract video frames or publish files. `export_summary.json` lists all exported color counts; `validation.json` records successful checks and hashes. `baseline.json` preserves the pre-round protection hashes.

## Handoff

The 50 files are ready for owner art review and subsequent packing. Primary A, junior B and senior B stay selected. Existing source-video reviews require no further Grok revision. Final animation loops, 48 px happy/sad head crops and physical display checks remain with frame processing/integration.

Round 5 was added to the brief during this work: another 50 toys and eight accessories. They are not part of this round. The ten current toys remain under `art/toys/`; move them into `art/toys/primary/` together with their manifest references during round 5. Historical design entries in the manifest remain for provenance; consume the canonical `companion`, `props`, `toys` and `ui` groups for this static delivery.
