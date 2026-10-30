<p align="right"><a href="README.zh_CN.md">简体中文</a> · <strong>English</strong></p>

# Seven stage wall ornaments

Seven new transparent ornaments cover primary, junior, senior, CET4, CET6, advanced and graduation appearances. The motifs are Chinese paper cutouts, Asian lanterns, a European window, an Americas postcard, an ocean/savanna postcard, an aurora window and travel memories. They are prepared assets for roadmap section 4; firmware integration remains pending the section 1 device gate.

Review the [native and exact 4x proof](../../preview/wall-decor/native-and-4x.png) and [seven home compositions](../../preview/wall-decor/homes.png). Each individual home has a native 240x320 PNG and an exact 2x proof in that preview directory. These are composites of the actual production host renderer with the proposed ornament added; the firmware does not draw the ornament yet.

## Export contract

- Canonical files: `art/wall/<stage>.png`, RGBA, 192x96, exact nearest-neighbor 4x exports of 48x24 game pixels.
- Anchor: top-left, `(0,0)` in both file and game coordinates. Proposed screen origin: `(180,108)`. Nontransparent pixels stay within screen rows 108–131 and columns 180–227.
- Each export has binary alpha and 27–30 visible RGB colors after RGB565 normalization, below the 32-color ceiling. Native pixel payload totals 24,192 bytes in the current packer's RGB565-plus-alpha representation, before index/alignment overhead.
- The head corridor `x=70..169`, clock, certificate, map, shelf and shelf plant rectangles are excluded. The exporter checks actual nontransparent pixels against these rectangles.
- `art/manifest.json` contains a separate `wall_decor` group with sizes, anchors, proposed placement, palette and hashes. The current packer ignores this group, so the application, asset IDs and packed release remain unchanged.

## Sources and verification

The seven original ImageGen PNGs are retained in [source/](source/); exact prompts and transparency settings are in [prompts.json](prompts.json). Built-in Codex ImageGen was used. Existing NABO starts, cap lettering, clothing, companions and props were not regenerated or mirrored.

Reproduce exports and checks after running the existing host UI probe:

```sh
python3 tests/fw_host/test_ui.py --assets firmware/assets/assets.bin --out build/checks/ui
python3 art/tools/build_wall_decor.py --homes build/checks/ui
```

[validation.json](validation.json) records dimensions, genuine source alpha, binary export alpha, visible colors, screen keepouts and hashes. Desktop review covered all seven native compositions and enlarged sprites. Owner design approval, actual display appearance and firmware integration remain unverified.

Build: NOT RUN (art preparation only).

Host tests: PASS (rules, extra rules/parity, 18 screens, 27,334 UI checks and two battle rounds with rematch). Asset repack with `--base-assets` is byte-identical to the published v0.4.1 pack.

Device tests: NOT RUN.

Unverified: owner review, physical display and integration after device acceptance.
