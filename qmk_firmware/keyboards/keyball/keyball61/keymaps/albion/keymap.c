/*
Copyright 2022 @Yowkees
Copyright 2022 MURAOKA Taro (aka KoRoN, @kaoriya)

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.
*/

#include QMK_KEYBOARD_H
#include "quantum.h"

enum custom_keycodes
{
  KC_MY_BTN1 = KEYBALL_SAFE_RANGE,
  KC_MY_BTN2,
  KC_MY_BTN3
};

enum click_state
{
  NONE = 0,
  WAITING,
  CLICKABLE,
  CLICKING,
};

enum click_state state;
uint16_t click_timer;

uint16_t to_reset_time = 800;
const int16_t to_clickable_movement = 0;
const uint16_t click_layer = 4;

int16_t mouse_movement;

void enable_click_layer(void)  { layer_on(click_layer);  click_timer = timer_read(); state = CLICKABLE; }
void disable_click_layer(void) { state = NONE; layer_off(click_layer); }
int16_t my_abs(int16_t num)    { return num < 0 ? -num : num; }
bool is_clickable_mode(void)   { return state == CLICKABLE || state == CLICKING; }

bool process_record_user(uint16_t keycode, keyrecord_t *record)
{
  switch (keycode) {
  case KC_MY_BTN1: case KC_MY_BTN2: case KC_MY_BTN3: {
    report_mouse_t currentReport = pointing_device_get_report();
    uint8_t btn = 1 << (keycode - KC_MY_BTN1);
    if (record->event.pressed) { currentReport.buttons |= btn;  state = CLICKING; }
    else                       { currentReport.buttons &= ~btn; enable_click_layer(); }
    pointing_device_set_report(currentReport);
    pointing_device_send();
    return false;
  }
  default:
    if (record->event.pressed) disable_click_layer();
  }
  return true;
}

report_mouse_t pointing_device_task_user(report_mouse_t mouse_report)
{
  int16_t current_x = mouse_report.x;
  int16_t current_y = mouse_report.y;
  if (current_x != 0 || current_y != 0) {
    switch (state) {
    case CLICKABLE: click_timer = timer_read(); break;
    case CLICKING:  break;
    case WAITING:
      mouse_movement += my_abs(current_x) + my_abs(current_y);
      if (mouse_movement >= to_clickable_movement) { mouse_movement = 0; enable_click_layer(); }
      break;
    default: click_timer = timer_read(); state = WAITING; mouse_movement = 0;
    }
  } else {
    switch (state) {
    case CLICKING: break;
    case CLICKABLE:
      if (timer_elapsed(click_timer) > to_reset_time) disable_click_layer();
      break;
    case WAITING:
      if (timer_elapsed(click_timer) > 50) { mouse_movement = 0; state = NONE; }
      break;
    default: mouse_movement = 0; state = NONE;
    }
  }
  mouse_report.x = current_x;
  mouse_report.y = current_y;
  return mouse_report;
}

// clang-format off
const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
  [0] = LAYOUT_universal(
    KC_ESC      , KC_1        , KC_2        , KC_3        , KC_4        , KC_5        , KC_6        , KC_7        , KC_8        , KC_9        , KC_0        , KC_MINS     ,
    KC_DEL      , KC_Q        , KC_W        , KC_E        , KC_R        , KC_T        , KC_Y        , KC_U        , KC_I        , KC_O        , KC_P        , KC_INT3     ,
    KC_TAB      , KC_A        , KC_S        , KC_D        , KC_F        , KC_G        , KC_H        , KC_J        , KC_K        , KC_L        , KC_SCLN     , S(KC_7)     ,
    MO(1)       , KC_Z        , KC_X        , KC_C        , KC_V        , KC_B        , KC_RBRC     , KC_NUHS     , KC_N        , KC_M        , KC_COMM     , KC_DOT      , KC_SLSH     , KC_RSFT     ,
    _______     , KC_LCTL     , KC_LALT     , KC_LGUI     , LT(1,KC_LNG2), LT(2,KC_SPC), LT(3,KC_LNG1), KC_BSPC     , LT(2,KC_ENT), LT(1,KC_LNG2), KC_RGUI     , _______     , KC_RALT     , KC_PSCR     
  ),
  [1] = LAYOUT_universal(
    S(KC_ESC)   , S(KC_1)     , KC_LBRC     , S(KC_3)     , S(KC_4)     , S(KC_5)     , KC_EQL      , S(KC_6)     , S(KC_QUOT)  , S(KC_8)     , S(KC_9)     , S(KC_INT1)  ,
    S(KC_DEL)   , S(KC_Q)     , S(KC_W)     , S(KC_E)     , S(KC_R)     , S(KC_T)     , S(KC_Y)     , S(KC_U)     , S(KC_I)     , S(KC_O)     , S(KC_P)     , S(KC_INT3)  ,
    S(KC_TAB)   , S(KC_A)     , S(KC_S)     , S(KC_D)     , S(KC_F)     , S(KC_G)     , S(KC_H)     , S(KC_J)     , S(KC_K)     , S(KC_L)     , KC_QUOT     , S(KC_2)     ,
    _______     , S(KC_Z)     , S(KC_X)     , S(KC_C)     , S(KC_V)     , S(KC_B)     , S(KC_RBRC)  , S(KC_NUHS)  , S(KC_N)     , S(KC_M)     , S(KC_COMM)  , S(KC_DOT)   , S(KC_SLSH)  , S(KC_RSFT)  ,
    _______     , S(KC_LCTL)  , S(KC_LALT)  , S(KC_LGUI)  , _______     , _______     , _______     , _______     , _______     , _______     , S(KC_RGUI)  , _______     , S(KC_RALT)  , _______     
  ),
  [2] = LAYOUT_universal(
    _______     , KC_F1       , KC_F2       , KC_F3       , KC_F4       , KC_F5       , KC_F6       , KC_F7       , KC_F8       , KC_F9       , KC_F10      , KC_F11      ,
    _______     , _______     , KC_7        , KC_8        , KC_9        , _______     , _______     , KC_LEFT     , KC_UP       , KC_RGHT     , _______     , KC_F12      ,
    _______     , _______     , KC_4        , KC_5        , KC_6        , S(KC_SCLN)  , KC_PGUP     , KC_BTN1     , KC_DOWN     , KC_BTN2     , KC_BTN3     , _______     ,
    _______     , _______     , KC_1        , KC_2        , KC_3        , S(KC_MINS)  , S(KC_8)     , S(KC_9)     , KC_PGDN     , _______     , _______     , _______     , _______     , _______     ,
    _______     , _______     , KC_0        , KC_DOT      , _______     , _______     , _______     , KC_DEL      , _______     , _______     , _______     , _______     , _______     , _______     
  ),
  [3] = LAYOUT_universal(
    RGB_TOG     , _______     , _______     , _______     , _______     , _______     , RGB_M_P     , RGB_M_B     , RGB_M_R     , RGB_M_SW    , RGB_M_SN    , RGB_M_K     ,
    RGB_MOD     , RGB_HUI     , RGB_SAI     , RGB_VAI     , _______     , _______     , RGB_M_X     , RGB_M_G     , RGB_M_T     , RGB_M_TW    , _______     , _______     ,
    RGB_RMOD    , RGB_HUD     , RGB_SAD     , RGB_VAD     , _______     , _______     , CPI_D1K     , CPI_D100    , CPI_I100    , CPI_I1K     , KBC_SAVE    , KBC_RST     ,
    _______     , _______     , SCRL_DVD    , SCRL_DVI    , SCRL_MO     , SCRL_TO     , QK_CLEAR_EEPROM, QK_CLEAR_EEPROM, KC_HOME     , KC_PGDN     , KC_PGUP     , KC_END      , _______     , _______     ,
    QK_BOOT     , _______     , KC_LEFT     , KC_DOWN     , KC_UP       , KC_RGHT     , _______     , _______     , KC_BSPC     , _______     , _______     , _______     , _______     , QK_BOOT     
  ),
  [4] = LAYOUT_universal(
    _______     , _______     , _______     , _______     , _______     , _______     , _______     , _______     , _______     , _______     , _______     , _______     ,
    _______     , _______     , _______     , _______     , _______     , _______     , _______     , _______     , _______     , _______     , _______     , _______     ,
    _______     , _______     , _______     , _______     , _______     , _______     , _______     , KC_MY_BTN1  , _______     , KC_MY_BTN2  , _______     , _______     ,
    _______     , _______     , _______     , _______     , _______     , _______     , _______     , _______     , _______     , _______     , _______     , _______     , _______     , _______     ,
    _______     , _______     , _______     , _______     , _______     , _______     , _______     , _______     , _______     , _______     , _______     , _______     , _______     , _______     
  )
};
// clang-format on

#ifdef RGB_MATRIX_ENABLE
led_config_t g_led_config = { {
  // Key Matrix to LED Index
  {  24, 19,     14, NO_LED,      9,      5,  1, NO_LED },
  {  25, 20,     15, NO_LED,     10,      6,  2, NO_LED },
  {  26, 21,     16, NO_LED,     11,      7,  3, NO_LED },
  {  27, 22,     17, NO_LED,     12,      8,  4,      0 },
  {  28, 23,     18, NO_LED,     13,     34, 35,     36 },
  {  44, 49,     54, NO_LED,     58,     62, 66, NO_LED },
  {  45, 50,     55, NO_LED,     59,     63, 67, NO_LED },
  {  46, 51,     56, NO_LED,     60,     64, 68, NO_LED },
  {  47, 52,     57, NO_LED,     61,     65, 69,     70 },
  {  48, 53, NO_LED, NO_LED, NO_LED, NO_LED, 38,     37 }
}, {
  // LED Index to Physical Position
  // {x,y}: x = 224 / (NUMBER_OF_COLS - 1) * COL_POSITION , y = 64 / (NUMBER_OF_ROWS - 1) * ROW_POSITION
  {  84, 48 }, //  0
  {  70,  0 }, //  1
  {  70, 16 }, //  2
  {  70, 32 }, //  3
  {  70, 48 }, //  4
  {  56,  0 }, //  5
  {  56, 16 }, //  6
  {  56, 32 }, //  7
  {  56, 48 }, //  8
  {  42,  0 }, //  9
  {  42, 16 }, // 10
  {  42, 32 }, // 11
  {  42, 48 }, // 12
  {  42, 64 }, // 13
  {  28,  0 }, // 14
  {  28, 16 }, // 15
  {  28, 32 }, // 16
  {  28, 48 }, // 17
  {  28, 64 }, // 18
  {  14,  0 }, // 19
  {  14, 16 }, // 20
  {  14, 32 }, // 21
  {  14, 48 }, // 22
  {  14, 64 }, // 23
  {   0,  0 }, // 24
  {   0, 16 }, // 25
  {   0, 32 }, // 26
  {   0, 48 }, // 27
  {   0, 64 }, // 28
  {   0,  0 }, // 29 (underglow)
  {   0,  0 }, // 30 (underglow)
  {   0,  0 }, // 31 (underglow)
  {   0,  0 }, // 32 (underglow)
  {   0,  0 }, // 33 (underglow)
  {  56, 64 }, // 34
  {  70, 64 }, // 35
  {  84, 64 }, // 36
  { 126, 64 }, // 37
  { 140, 64 }, // 38
  {   0,  0 }, // 39 (underglow)
  {   0,  0 }, // 40 (underglow)
  {   0,  0 }, // 41 (underglow)
  {   0,  0 }, // 42 (underglow)
  {   0,  0 }, // 43 (underglow)
  { 210,  0 }, // 44
  { 210, 16 }, // 45
  { 210, 32 }, // 46
  { 210, 48 }, // 47
  { 210, 64 }, // 48
  { 196,  0 }, // 49
  { 196, 16 }, // 50
  { 196, 32 }, // 51
  { 196, 48 }, // 52
  { 196, 64 }, // 53
  { 182,  0 }, // 54
  { 182, 16 }, // 55
  { 182, 32 }, // 56
  { 182, 48 }, // 57
  { 168,  0 }, // 58
  { 168, 16 }, // 59
  { 168, 32 }, // 60
  { 168, 48 }, // 61
  { 154,  0 }, // 62
  { 154, 16 }, // 63
  { 154, 32 }, // 64
  { 154, 48 }, // 65
  { 140,  0 }, // 66
  { 140, 16 }, // 67
  { 140, 32 }, // 68
  { 140, 48 }, // 69
  { 126, 48 }, // 70
  {   0,  0 }, // 71 (underglow)
  {   0,  0 }, // 72 (underglow)
  {   0,  0 }, // 73 (underglow)
}, {
  // LED Index to Flag
  LED_FLAG_KEYLIGHT, //  0
  LED_FLAG_KEYLIGHT, //  1
  LED_FLAG_KEYLIGHT, //  2
  LED_FLAG_KEYLIGHT, //  3
  LED_FLAG_KEYLIGHT, //  4
  LED_FLAG_KEYLIGHT, //  5
  LED_FLAG_KEYLIGHT, //  6
  LED_FLAG_KEYLIGHT, //  7
  LED_FLAG_KEYLIGHT, //  8
  LED_FLAG_KEYLIGHT, //  9
  LED_FLAG_KEYLIGHT, // 10
  LED_FLAG_KEYLIGHT, // 11
  LED_FLAG_KEYLIGHT, // 12
  LED_FLAG_KEYLIGHT, // 13
  LED_FLAG_KEYLIGHT, // 14
  LED_FLAG_KEYLIGHT, // 15
  LED_FLAG_KEYLIGHT, // 16
  LED_FLAG_KEYLIGHT, // 17
  LED_FLAG_KEYLIGHT, // 18
  LED_FLAG_KEYLIGHT, // 19
  LED_FLAG_KEYLIGHT, // 20
  LED_FLAG_KEYLIGHT, // 21
  LED_FLAG_KEYLIGHT, // 22
  LED_FLAG_KEYLIGHT, // 23
  LED_FLAG_KEYLIGHT, // 24
  LED_FLAG_KEYLIGHT, // 25
  LED_FLAG_KEYLIGHT, // 26
  LED_FLAG_KEYLIGHT, // 27
  LED_FLAG_KEYLIGHT, // 28
  LED_FLAG_NONE,     // 29 (underglow)
  LED_FLAG_NONE,     // 30 (underglow)
  LED_FLAG_NONE,     // 31 (underglow)
  LED_FLAG_NONE,     // 32 (underglow)
  LED_FLAG_NONE,     // 33 (underglow)
  LED_FLAG_KEYLIGHT, // 34
  LED_FLAG_KEYLIGHT, // 35
  LED_FLAG_KEYLIGHT, // 36
  LED_FLAG_KEYLIGHT, // 37
  LED_FLAG_KEYLIGHT, // 38
  LED_FLAG_NONE,     // 39 (underglow)
  LED_FLAG_NONE,     // 40 (underglow)
  LED_FLAG_NONE,     // 41 (underglow)
  LED_FLAG_NONE,     // 42 (underglow)
  LED_FLAG_NONE,     // 43 (underglow)
  LED_FLAG_KEYLIGHT, // 44
  LED_FLAG_KEYLIGHT, // 45
  LED_FLAG_KEYLIGHT, // 46
  LED_FLAG_KEYLIGHT, // 47
  LED_FLAG_KEYLIGHT, // 48
  LED_FLAG_KEYLIGHT, // 49
  LED_FLAG_KEYLIGHT, // 50
  LED_FLAG_KEYLIGHT, // 51
  LED_FLAG_KEYLIGHT, // 52
  LED_FLAG_KEYLIGHT, // 53
  LED_FLAG_KEYLIGHT, // 54
  LED_FLAG_KEYLIGHT, // 55
  LED_FLAG_KEYLIGHT, // 56
  LED_FLAG_KEYLIGHT, // 57
  LED_FLAG_KEYLIGHT, // 58
  LED_FLAG_KEYLIGHT, // 59
  LED_FLAG_KEYLIGHT, // 60
  LED_FLAG_KEYLIGHT, // 61
  LED_FLAG_KEYLIGHT, // 62
  LED_FLAG_KEYLIGHT, // 63
  LED_FLAG_KEYLIGHT, // 64
  LED_FLAG_KEYLIGHT, // 65
  LED_FLAG_KEYLIGHT, // 66
  LED_FLAG_KEYLIGHT, // 67
  LED_FLAG_KEYLIGHT, // 68
  LED_FLAG_KEYLIGHT, // 69
  LED_FLAG_KEYLIGHT, // 70
  LED_FLAG_NONE,     // 71 (underglow)
  LED_FLAG_NONE,     // 72 (underglow)
  LED_FLAG_NONE      // 73 (underglow)
} };
#endif

#ifdef RGB_MATRIX_ENABLE
void keyboard_post_init_user(void) { rgb_matrix_reload_from_eeprom(); }
#endif

layer_state_t layer_state_set_user(layer_state_t state)
{
  keyball_set_scroll_mode(get_highest_layer(state) == 3);
#ifdef RGB_MATRIX_ENABLE
  // レイヤーと LED を連動させる（一時的な変更で、保存はされない）
  uint8_t layer = biton32(state);
  switch (layer) {
  case 1:
    rgb_matrix_mode_noeeprom(RGB_MATRIX_SOLID_COLOR);
    rgb_matrix_sethsv_noeeprom(170, 255, 255);
    break;
  case 2:
    rgb_matrix_mode_noeeprom(RGB_MATRIX_SOLID_COLOR);
    rgb_matrix_sethsv_noeeprom(0, 255, 255);
    break;
  case 4:
    rgb_matrix_mode_noeeprom(RGB_MATRIX_SOLID_COLOR);
    rgb_matrix_sethsv_noeeprom(0, 0, 255);
    break;
  default:
    rgb_matrix_reload_from_eeprom();
  }
#endif
  return state;
}

#ifdef OLED_ENABLE
#include "lib/oledkit/oledkit.h"
void oledkit_render_info_user(void)
{
  keyball_oled_render_keyinfo();
  keyball_oled_render_ballinfo();
  oled_write_P(PSTR("Layer:"), false);
  oled_write(get_u8_str(get_highest_layer(layer_state), ' '), false);
}
#endif
