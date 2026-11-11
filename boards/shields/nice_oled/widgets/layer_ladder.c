#include <stdio.h>
#include <zephyr/kernel.h>
#include <zmk/keymap.h>
#include <zmk/hid.h>
#include <zmk/rgb_underglow.h>
#include <dt-bindings/zmk/modifiers.h>
#include <fonts.h>
#include "layer_ladder.h"

LV_FONT_DECLARE(pixel_operator_mono_22);

#define INK LVGL_FOREGROUND
#define PAPER LVGL_BACKGROUND

#define LADDER_Y 27
#define ROW_H 10

static const char *const layer_names[] = {"QWE", "COL", "LOW", "RAI", "ADJ", "RGB"};
#define LAYER_NAMES_LEN ((int)(sizeof(layer_names) / sizeof(layer_names[0])))

static void fill(lv_obj_t *canvas, int x, int y, int w, int h, lv_color_t color) {
    lv_draw_rect_dsc_t dsc;
    init_rect_dsc(&dsc, color);
    canvas_draw_rect(canvas, x, y, w, h, &dsc);
}

/* 1-pixel outline box */
static void outline(lv_obj_t *canvas, int x, int y, int w, int h) {
    fill(canvas, x, y, w, h, INK);
    fill(canvas, x + 1, y + 1, w - 2, h - 2, PAPER);
}

static void label(lv_obj_t *canvas, int x, int y, int w, const lv_font_t *font, lv_color_t color,
                  const char *txt) {
    lv_draw_label_dsc_t dsc;
    init_label_dsc(&dsc, color, font, LV_TEXT_ALIGN_LEFT);
    canvas_draw_text(canvas, x, y, w, &dsc, txt);
}


#define RGB_LAYER_INDEX 5

#if __has_include(<nzmk/rgb_engine.h>)
#include <nzmk/rgb_engine.h>
#define NZMK_RGB 1
#endif

/* Flipper-style RGB menu, drawn instead of the ladder while the RGB layer (5) is on top */
static void draw_rgb_menu(lv_obj_t *canvas) {
    static const char *const effects[] = {"SOL", "BRE", "SPE", "SWI", "LAY", "RIP", "INV"};
    const int count = 7;
#ifdef NZMK_RGB
    const int cur = nzmk_rgb_mode();
    const bool on = nzmk_rgb_is_on();
#else
    const int cur = zmk_rgb_underglow_calc_effect(0);
    bool on = false;
    zmk_rgb_underglow_get_state(&on);
#endif
    const struct zmk_led_hsb color = zmk_rgb_underglow_calc_brt(0);

    for (int i = 0; i < count; i++) {
        const int y = 1 + i * ROW_H;
        if (i == cur) {
            fill(canvas, 0, y, 32, ROW_H - 1, INK);
            label(canvas, 2, y + 1, 22, &pixel_operator_mono_8, PAPER, effects[i]);
            fill(canvas, 24, y + 2, 6, 5, PAPER);
        } else {
            label(canvas, 2, y + 1, 22, &pixel_operator_mono_8, INK, effects[i]);
            outline(canvas, 24, y + 2, 6, 5);
        }
    }

    fill(canvas, 0, 73, 32, 1, INK);

    char buf[12];
    const int y0 = 77;
    snprintf(buf, sizeof(buf), "H%3d", color.h);
    label(canvas, 2, y0, 30, &pixel_operator_mono_8, INK, buf);
    snprintf(buf, sizeof(buf), "S%3d", color.s);
    label(canvas, 2, y0 + 10, 30, &pixel_operator_mono_8, INK, buf);
    snprintf(buf, sizeof(buf), "B%3d", color.b);
    label(canvas, 2, y0 + 20, 30, &pixel_operator_mono_8, INK, buf);

    /* brightness bar */
    outline(canvas, 2, y0 + 31, 28, 6);
    fill(canvas, 3, y0 + 32, (26 * color.b) / 100, 4, INK);

    /* on/off bar */
    if (on) {
        fill(canvas, 0, 120, 32, 7, INK);
    } else {
        outline(canvas, 0, 120, 32, 7);
    }
}

void draw_layer_ladder(lv_obj_t *canvas, const struct status_state *state) {
    if (state->layer_index == RGB_LAYER_INDEX) {
        draw_rgb_menu(canvas);
        return;
    }
    int top = state->layer_index;
    if (top == 6) {
        top = 4; /* latched mouse layer = shown as ADJ */
    }
    const int base = zmk_keymap_layer_active(1) ? 1 : 0;

    /* 1. big base letter; replaced by a CAPS / WORD bar only while Caps is active */
#ifdef NZMK_RGB
    const int caps = nzmk_caps_state();
#else
    const int caps = 0;
#endif
    if (caps != 0) {
        fill(canvas, 0, 0, 32, 23, INK);
        label(canvas, 4, 5, 28, &pixel_operator_mono_12, PAPER, caps == NZMK_CAPS_LOCK ? "CAPS" : "WORD");
    } else {
        label(canvas, 10, 1, 22, &pixel_operator_mono_22, INK, base == 1 ? "C" : "Q");
    }
    fill(canvas, 0, 24, 32, 1, INK);

    /* 2. ladder: base layer and active layer are inverted bars */
    for (int i = 0; i < LAYER_NAMES_LEN; i++) {
        const int y = LADDER_Y + i * ROW_H;
        const bool lit = (i == top) || (i == base);

        if (lit) {
            fill(canvas, 0, y, 32, ROW_H - 1, INK);
            label(canvas, 2, y + 1, 22, &pixel_operator_mono_8, PAPER, layer_names[i]);
            fill(canvas, 24, y + 2, 6, 5, PAPER);
        } else {
            label(canvas, 2, y + 1, 22, &pixel_operator_mono_8, INK, layer_names[i]);
            outline(canvas, 24, y + 2, 6, 5);
        }
    }

    const int sep2 = LADDER_Y + LAYER_NAMES_LEN * ROW_H + 1; /* 88 */
    fill(canvas, 0, sep2, 32, 1, INK);

    /* 3. modifiers: ^ S A W in four small boxes */
    const uint8_t mods = zmk_hid_get_explicit_mods();
    const uint8_t masks[4] = {MOD_LCTL | MOD_RCTL, MOD_LSFT | MOD_RSFT, MOD_LALT | MOD_RALT,
                              MOD_LGUI | MOD_RGUI};
    const char *const glyphs[4] = {"^", "S", "A", "W"};
    for (int k = 0; k < 4; k++) {
        const int x = k * 8;
        const int y = sep2 + 3;
        if (mods & masks[k]) {
            fill(canvas, x, y, 7, 10, INK);
            label(canvas, x + 1, y + 2, 6, &pixel_operator_mono_8, PAPER, glyphs[k]);
        } else {
            outline(canvas, x, y, 7, 10);
            label(canvas, x + 1, y + 2, 6, &pixel_operator_mono_8, INK, glyphs[k]);
        }
    }

    /* 4. connection (USB / BT) and Bluetooth profile squares */
    const int y_conn = sep2 + 17; /* 105 */
    const bool usb = state->selected_endpoint.transport == ZMK_TRANSPORT_USB;
    label(canvas, 0, y_conn, 12, &pixel_operator_mono_8, INK, usb ? "USB" : "BT");
    if (!usb) {
        for (int p = 0; p < 5; p++) {
            const int x = 13 + p * 4;
            if (p == state->active_profile_index) {
                fill(canvas, x, y_conn + 1, 3, 5, INK);
            } else {
                outline(canvas, x, y_conn + 1, 3, 5);
            }
        }
    }

    /* 5. battery icon + percent */
    const int y_bat = y_conn + 12; /* 117 */
    int level = state->battery;
    if (level > 100) {
        level = 100;
    }
    outline(canvas, 0, y_bat, 13, 8);
    fill(canvas, 13, y_bat + 2, 1, 4, INK);
    fill(canvas, 2, y_bat + 2, (level * 9) / 100, 4, INK);

    char pct[6];
    snprintf(pct, sizeof(pct), "%d", level);
    label(canvas, 16, y_bat, 16, &pixel_operator_mono_8, INK, pct);
}
