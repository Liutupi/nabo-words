idf_component_register(
    SRCS "main.c"
         "nw_assets.c" "nw_data.c" "nw_db.c" "nw_font.c" "nw_game.c" "nw_gfx.c"
         "nw_scene_home.c" "nw_scene_misc.c" "nw_scene_study.c" "nw_sound.c" "nw_sprite.c" "nw_store.c"
         "words/nabo_words_battle.c" "words/nabo_words_model.c" "words/nabo_words_quiz.c"
    INCLUDE_DIRS "."
    REQUIRES bsp esp_lcd esp_timer nvs_flash esp_partition spi_flash esp_driver_usb_serial_jtag
)

# 逐像素合成和解码在单核 C3 上是热点
set_source_files_properties(nw_gfx.c nw_font.c nw_sprite.c nw_scene_misc.c nw_sound.c
                            PROPERTIES COMPILE_OPTIONS "-O2")
