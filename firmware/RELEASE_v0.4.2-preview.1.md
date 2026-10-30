<p align="right"><a href="RELEASE_v0.4.2-preview.1.zh_CN.md">简体中文</a> · <strong>English</strong></p>

# NABO Words Passport v0.4.2-preview.1

A preview release with twelve new sleep and celebration animations across junior school, senior school, CET-4, CET-6, advanced and graduation outfits.

- Sleep includes settling onto the desk, a closed-eye breathing loop and waking up. A button press wakes NABO; the animation returns to idle afterward.
- Rewards play a celebration in the current outfit. Stage accessories remain: headphones, camera and the same graduation diploma, raised in one hand.
- NABO retains the familiar face, glasses, white cap and forward-facing cap logo. The prototype and firmware include the matching animations.
- Existing vocabulary, fonts, static artwork and previous animations are preserved.

## Upgrade

The save format and partition layout are unchanged. For compatible v0.3.0, v0.4.0 or v0.4.1 installations, place all release files together and use Python 3 with esptool 5:

```sh
python flash_words.py <port> --app
```

This writes both the application and assets while preserving the learning-record partition. Use the matching files from this release. The complete `nabo-words-merged.bin` is for installation at `0x0` and clears learning records; the community full-image installer is not a progress-preserving upgrade.

The application is 918,608 bytes; assets are 6,074,022 bytes with 151,898 bytes remaining. New sleep clips use 24 frames at 6 fps; celebrations use 39 frames at 12 fps after trimming neutral holds. Existing idle animations remain 49 frames at 12 fps. New animations use up to 32 colors, with per-frame color-error and lossless index-decoding checks during packaging. Detailed build and test records are in [validation/v0.4.2-preview.1.json](../validation/v0.4.2-preview.1.json).

## SHA256

```text
6ae97bc0851ec88b2ffab88d707d7bec7302c2d347071c783955d7a317d50ab4  assets.bin
a6ecb3a836e749c97e8d9d6a17bf8f689558b2b8301bd31ae84da35b88036b4f  bootloader.bin
29b51008443abace7c493f88e5ec1ab6ce0d6492a21a0e301faafde2860f7a8e  flash_words.py
1a1a6b3b9c39e3a6c904cde270478d7cebf1be6bcff8911c6edbe41b72a49111  nabo-words-merged.bin
d09ddb885c68715cef8de8e24e813f4c106ff0d5e8b8e437fb553c074a6a16b8  nabo-words-passport.bin
34373abb6cd1b5c1f6de2bde0a3f7eb457fe21c3ee3a7c83a9af1fcc30d06088  partition-table.bin
```

## License

Code is MIT; NABO identity and artwork are excluded from MIT. ECDICT is MIT and fonts are OFL 1.1. The board support package is from FoloToy's official ai-passport project under MIT. This project is unaffiliated with FoloToy.
