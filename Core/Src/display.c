#include "display.h"
#include "main.h"
#include "i2c.h"
#include "config.h"
#include "preset.h"
#include <stdio.h>
#include <string.h>

// ===== LCD2004 через PCF8574 I2C =====
// PCF8574 бітове відображення:
//   P0 = RS,  P1 = RW,  P2 = EN,  P3 = Backlight
//   P4-P7 = D4-D7

#define LCD_RS        0x01
#define LCD_RW        0x02
#define LCD_EN        0x04
#define LCD_BACKLIGHT 0x08

#define LCD_COLS  20
#define LCD_ROWS  4

// DDRAM адреси рядків для LCD2004
static const uint8_t ROW_ADDR[4] = { 0x00, 0x40, 0x14, 0x54 };

// ===== Стан дисплея =====

static DisplayScreen  s_screen        = SCREEN_MAIN;
static float          s_force         = 0.0f;   // кН
static float          s_target        = FORCE_DEFAULT_KN;  // кН
static float          s_angle_current = 0.0f;
static float          s_angle_target  = ANGLE_DEFAULT_DEG;
static bool           s_angle_reached = false;
static DisplayMotionMode s_motion_mode = DISPLAY_MODE_IDLE;
static uint16_t       s_motion_speed  = 0;      // командна швидкість, steps/s
static float          s_auto_force    = 0.0f;   // кН для AUTO екрана
static float          s_auto_target   = FORCE_DEFAULT_KN;
static float          s_auto_error    = 0.0f;   // кН
static bool           s_dirty         = true;
static uint8_t        s_menu_item     = 0;
static uint8_t        s_preset_item   = 0;  // поточна позиція у списку пресетів
static int8_t         s_active_preset = -1; // обраний пресет (-1 = немає)
static uint8_t        s_preset_edit_idx   = 0;
static float          s_preset_edit_target = FORCE_DEFAULT_KN;
static float          s_preset_edit_angle  = ANGLE_DEFAULT_DEG;
static uint8_t        s_preset_edit_field  = 0;

// Буфер кожного рядка для порівняння (щоб не надсилати незмінені)
static char s_lcd_buf[LCD_ROWS][LCD_COLS + 1];
static char s_new_buf[LCD_ROWS][LCD_COLS + 1];

// Текст калібровки (керується зовні через display_set_calib_text)
static char s_calib_lines[LCD_ROWS][LCD_COLS + 1];

// Помилка
static char s_error_msg[LCD_COLS + 1];
static uint8_t s_blink_tick = 0;
static float   s_settings_angle = ANGLE_DEFAULT_DEG;
static float   s_settings_step  = 10.0f;

// ===== PCF8574 / LCD низькорівневі функції =====

static void pcf_write(uint8_t byte)
{
    HAL_I2C_Master_Transmit(&hi2c1, LCD_I2C_ADDR, &byte, 1, 5);
}

static void lcd_nibble(uint8_t nibble, uint8_t flags)
{
    uint8_t b = (nibble & 0xF0) | flags | LCD_BACKLIGHT;
    pcf_write(b | LCD_EN);
    pcf_write(b);
}

static void lcd_byte(uint8_t data, uint8_t flags)
{
    lcd_nibble(data & 0xF0,        flags);
    lcd_nibble((data << 4) & 0xF0, flags);
}

static void lcd_cmd(uint8_t cmd)
{
    lcd_byte(cmd, 0);
}

static void lcd_data(uint8_t ch)
{
    lcd_byte(ch, LCD_RS);
}

static void lcd_set_cursor(uint8_t col, uint8_t row)
{
    lcd_cmd(0x80 | (ROW_ADDR[row & 3] + col));
}

static void lcd_print(const char *str)
{
    while (*str) {
        lcd_data((uint8_t)*str++);
    }
}

// Ініціалізація LCD у 4-бітному режимі
static void lcd_hw_init(void)
{
    HAL_Delay(50);

    // Переключення в 4-бітний режим (три спроби для надійності)
    lcd_nibble(0x30, 0); HAL_Delay(5);
    lcd_nibble(0x30, 0); HAL_Delay(2);
    lcd_nibble(0x30, 0); HAL_Delay(2);
    lcd_nibble(0x20, 0); HAL_Delay(2);

    lcd_cmd(0x28); HAL_Delay(1);  // Function Set: 4-bit, 2 lines, 5x8 dots
    lcd_cmd(0x0C); HAL_Delay(1);  // Display ON, cursor OFF, blink OFF
    lcd_cmd(0x01); HAL_Delay(2);  // Clear display
    lcd_cmd(0x06); HAL_Delay(1);  // Entry Mode: increment, no shift
}

// ===== Формування рядків =====

static void make_line_padded(char *dst, const char *src)
{
    int len = (int)strlen(src);
    if (len > LCD_COLS) len = LCD_COLS;
    memcpy(dst, src, len);
    for (int i = len; i < LCD_COLS; i++) dst[i] = ' ';
    dst[LCD_COLS] = '\0';
}

// Форматує діапазон зусилля в кН у 7-символьний рядок
// 8.5-9.0 → "8.5-9.0"  |  15-16 → "15-16  " (padded)
static void format_range_kN(char *buf, float fmin, float fmax)
{
    if (fmin >= 10.0f) {
        snprintf(buf, 8, "%.0f-%.0f", (double)fmin, (double)fmax);
    } else {
        snprintf(buf, 8, "%.1f-%.1f", (double)fmin, (double)fmax);
    }
}

static void build_screen_main(void)
{
    char tmp[LCD_COLS + 1];
    const char *mode = "IDLE";

    // Рядок 0: активний пресет або назва пристрою
    if (s_active_preset >= 0 && s_active_preset < PRESET_COUNT) {
        const Preset_t *p = &g_presets[s_active_preset];
        char rng[8];
        format_range_kN(rng, p->force_min_kN, p->force_max_kN);
        // "300 Nozzle  8.5-9.0"  = 3+1+8+7 = 19 chars → padded to 20
        snprintf(tmp, sizeof(tmp), "%-3s %-8s%s", p->inj_name, p->op_name, rng);
    } else {
        snprintf(tmp, sizeof(tmp), "VW NF PRESS");
    }
    make_line_padded(s_new_buf[0], tmp);

    // Рядок 1: поточне і задане зусилля
    snprintf(tmp, sizeof(tmp), "F:%5.2f kN  T:%5.2f",
             (double)s_force, (double)s_target);
    make_line_padded(s_new_buf[1], tmp);

    // Рядок 2: поточний / заданий кут
    s_blink_tick++;
    if (s_angle_reached && (s_blink_tick & 0x03U)) {
        make_line_padded(s_new_buf[2], "Angle: [  DONE!  ]  ");
    } else {
        snprintf(tmp, sizeof(tmp), "Ang:%6.1f /%6.1f",
                 (double)s_angle_current, (double)s_angle_target);
        make_line_padded(s_new_buf[2], tmp);
    }

    switch (s_motion_mode) {
        case DISPLAY_MODE_APPROACH: mode = "APPR"; break;
        case DISPLAY_MODE_SOFT:     mode = "SOFT"; break;
        case DISPLAY_MODE_PRESS:    mode = "PRESS"; break;
        case DISPLAY_MODE_ASEEK:    mode = "ASEEK"; break;
        case DISPLAY_MODE_HOLD:     mode = "HOLD"; break;
        case DISPLAY_MODE_DONE:     mode = "DONE"; break;
        case DISPLAY_MODE_FINE:     mode = "FINE"; break;
        case DISPLAY_MODE_RETRACT:  mode = "BACK"; break;
        case DISPLAY_MODE_IDLE:
        default:                    mode = "IDLE"; break;
    }

    snprintf(tmp, sizeof(tmp), "%-5s%4u  STOP", mode, (unsigned)s_motion_speed);
    make_line_padded(s_new_buf[3], tmp);
}

static void build_screen_auto(void)
{
    char tmp[LCD_COLS + 1];
    const char *mode = "IDLE";
    bool auto_idle_hint = (s_motion_mode == DISPLAY_MODE_IDLE && s_motion_speed == 0);

    switch (s_motion_mode) {
        case DISPLAY_MODE_APPROACH: mode = "APPR"; break;
        case DISPLAY_MODE_SOFT:     mode = "SOFT"; break;
        case DISPLAY_MODE_PRESS:    mode = "PRESS"; break;
        case DISPLAY_MODE_ASEEK:    mode = "ASEEK"; break;
        case DISPLAY_MODE_HOLD:     mode = "HOLD"; break;
        case DISPLAY_MODE_DONE:     mode = "DONE"; break;
        case DISPLAY_MODE_FINE:     mode = "FINE"; break;
        case DISPLAY_MODE_RETRACT:  mode = "BACK"; break;
        case DISPLAY_MODE_IDLE:
        default:                    mode = "IDLE"; break;
    }

    make_line_padded(s_new_buf[0], "=== AUTO FORCE ===");

    snprintf(tmp, sizeof(tmp), "Now:   %6.2f kN", (double)s_auto_force);
    make_line_padded(s_new_buf[1], tmp);

    snprintf(tmp, sizeof(tmp), "T:%5.2f E:%+5.2f", (double)s_auto_target, (double)s_auto_error);
    make_line_padded(s_new_buf[2], tmp);

    if (auto_idle_hint) {
        snprintf(tmp, sizeof(tmp), "UP=start DN=stop");
    } else {
        snprintf(tmp, sizeof(tmp), "%-5s%4u  STOP", mode, (unsigned)s_motion_speed);
    }
    make_line_padded(s_new_buf[3], tmp);
}

#define MENU_ITEMS 6
static const char *MENU_LABELS[MENU_ITEMS] = {
    "1.Auto force",
    "2.Presets",
    "3.Edit presets",
    "4.Calibration",
    "5.Settings",
    "6.Manual + zero"
};

static void build_screen_menu(void)
{
    make_line_padded(s_new_buf[0], "=== MENU ===");

    for (int i = 0; i < 3; i++) {
        uint8_t item = (s_menu_item <= 1) ? (uint8_t)i : (uint8_t)(s_menu_item - 1 + i);
        if (item >= MENU_ITEMS) {
            make_line_padded(s_new_buf[i + 1], "");
        } else {
            char tmp[LCD_COLS + 1];
            snprintf(tmp, sizeof(tmp), "%c%s",
                     (item == s_menu_item) ? '>' : ' ',
                     MENU_LABELS[item]);
            make_line_padded(s_new_buf[i + 1], tmp);
        }
    }
}

static void build_screen_preset(void)
{
    if (s_screen == SCREEN_PRESET_EDIT_LIST) {
        make_line_padded(s_new_buf[0], "== EDIT PRESET ==");
    } else {
        make_line_padded(s_new_buf[0], "=== PRESET ===");
    }

    // Вікно прокрутки: показуємо 3 пункти, обраний завжди видимий
    uint8_t start = 0;
    if (s_preset_item > 1) {
        start = s_preset_item - 1;
        if (start + 3 > PRESET_COUNT) {
            start = (PRESET_COUNT > 3) ? (PRESET_COUNT - 3) : 0;
        }
    }

    for (int i = 0; i < 3; i++) {
        uint8_t idx = start + (uint8_t)i;
        if (idx >= PRESET_COUNT) {
            make_line_padded(s_new_buf[i + 1], "");
            continue;
        }
        const Preset_t *p = &g_presets[idx];
        char tmp[LCD_COLS + 1];
        if (s_screen == SCREEN_PRESET_EDIT_LIST) {
            snprintf(tmp, sizeof(tmp), "%c%-3s %-6.6s%4.1f/%3.0f%c",
                     (idx == s_preset_item) ? '>' : ' ',
                     p->inj_name, p->op_name,
                     (double)preset_get_target_kN(idx),
                     (double)preset_get_angle_deg(idx),
                     0xDF);
        } else {
            char rng[8];
            format_range_kN(rng, p->force_min_kN, p->force_max_kN);
            snprintf(tmp, sizeof(tmp), "%c%-3s %-8s%s",
                     (idx == s_preset_item) ? '>' : ' ',
                     p->inj_name, p->op_name, rng);
        }
        make_line_padded(s_new_buf[i + 1], tmp);
    }
}

static void build_screen_preset_edit(void)
{
    char tmp[LCD_COLS + 1];
    if (s_preset_edit_idx < PRESET_COUNT) {
        const Preset_t *p = &g_presets[s_preset_edit_idx];
        snprintf(tmp, sizeof(tmp), "%-3s %-8s EDIT", p->inj_name, p->op_name);
    } else {
        snprintf(tmp, sizeof(tmp), "PRESET %u EDIT", (unsigned)s_preset_edit_idx);
    }
    make_line_padded(s_new_buf[0], tmp);

    snprintf(tmp, sizeof(tmp), "%cForce:%6.2f kN",
             (s_preset_edit_field == 0) ? '>' : ' ',
             (double)s_preset_edit_target);
    make_line_padded(s_new_buf[1], tmp);

    snprintf(tmp, sizeof(tmp), "%cAngle:%6.1f%c",
             (s_preset_edit_field == 1) ? '>' : ' ',
             (double)s_preset_edit_angle, 0xDF);
    make_line_padded(s_new_buf[2], tmp);

    make_line_padded(s_new_buf[3], "UP=field  BTN=save");
}

static void build_screen_calib(void)
{
    for (int i = 0; i < LCD_ROWS; i++) {
        make_line_padded(s_new_buf[i], s_calib_lines[i]);
    }
}

static void build_screen_settings(void)
{
    char tmp[LCD_COLS + 1];
    make_line_padded(s_new_buf[0], "=== SETTINGS ===");
    snprintf(tmp, sizeof(tmp), "Angle target:%6.1f", (double)s_settings_angle);
    make_line_padded(s_new_buf[1], tmp);
    snprintf(tmp, sizeof(tmp), "Step:%4.0f  UP=step", (double)s_settings_step);
    make_line_padded(s_new_buf[2], tmp);
    make_line_padded(s_new_buf[3], "ENC=change BTN=save");
}

static void build_screen_error(void)
{
    make_line_padded(s_new_buf[0], "!!! ERROR !!!");
    make_line_padded(s_new_buf[1], s_error_msg);
    make_line_padded(s_new_buf[2], "");
    make_line_padded(s_new_buf[3], "Press STOP to reset");
}

// Надіслати рядки що змінились
static void flush_screen(void)
{
    for (int row = 0; row < LCD_ROWS; row++) {
        if (memcmp(s_new_buf[row], s_lcd_buf[row], LCD_COLS) != 0) {
            lcd_set_cursor(0, row);
            lcd_print(s_new_buf[row]);
            memcpy(s_lcd_buf[row], s_new_buf[row], LCD_COLS + 1);
        }
    }
}

// ===== Публічні функції =====

void display_init(void)
{
    memset(s_lcd_buf,     0, sizeof(s_lcd_buf));
    memset(s_new_buf,     0, sizeof(s_new_buf));
    memset(s_calib_lines, 0, sizeof(s_calib_lines));
    memset(s_error_msg,   0, sizeof(s_error_msg));

    lcd_hw_init();
    s_dirty = true;
}

void display_update(void)
{
    if (!s_dirty) return;
    s_dirty = false;

    switch (s_screen) {
        case SCREEN_MAIN:         build_screen_main();     break;
        case SCREEN_AUTO:         build_screen_auto();     break;
        case SCREEN_MENU:         build_screen_menu();     break;
        case SCREEN_PRESET:       build_screen_preset();   break;
        case SCREEN_PRESET_EDIT_LIST: build_screen_preset(); break;
        case SCREEN_PRESET_EDIT:  build_screen_preset_edit(); break;
        case SCREEN_CALIBRATION:  build_screen_calib();    break;
        case SCREEN_SETTINGS:     build_screen_settings(); break;
        case SCREEN_ERROR:        build_screen_error();    break;
        default:                  build_screen_main();     break;
    }

    flush_screen();
}

DisplayScreen display_get_screen(void)
{
    return s_screen;
}

void display_set_screen(DisplayScreen s)
{
    if (s_screen != s) {
        s_screen = s;
        memset(s_lcd_buf, 0, sizeof(s_lcd_buf));
        s_dirty = true;
        if (s == SCREEN_MENU) s_menu_item = 0;  // курсор завжди з початку
    }
}

void display_set_force(float current_kN, float target_kN)
{
    if (s_force != current_kN || s_target != target_kN) {
        s_force  = current_kN;
        s_target = target_kN;
        if (s_screen == SCREEN_MAIN) s_dirty = true;
    }
}

void display_set_angle(float current_deg, float target_deg, bool reached)
{
    if (s_angle_current != current_deg || s_angle_target != target_deg ||
        s_angle_reached != reached) {
        s_angle_current = current_deg;
        s_angle_target  = target_deg;
        s_angle_reached = reached;
        if (s_screen == SCREEN_MAIN) s_dirty = true;
    }
}

void display_set_motion_mode(DisplayMotionMode mode)
{
    if (s_motion_mode != mode) {
        s_motion_mode = mode;
        if (s_screen == SCREEN_MAIN || s_screen == SCREEN_AUTO) s_dirty = true;
    }
}

void display_set_motion_speed(uint16_t speed_steps)
{
    if (s_motion_speed != speed_steps) {
        s_motion_speed = speed_steps;
        if (s_screen == SCREEN_MAIN || s_screen == SCREEN_AUTO) s_dirty = true;
    }
}

void display_set_auto_metrics(float current_kN, float target_kN, float error_kN)
{
    if (s_auto_force != current_kN || s_auto_target != target_kN || s_auto_error != error_kN) {
        s_auto_force  = current_kN;
        s_auto_target = target_kN;
        s_auto_error  = error_kN;
        if (s_screen == SCREEN_AUTO) s_dirty = true;
    }
}

void display_show_error(const char *msg)
{
    strncpy(s_error_msg, msg, LCD_COLS);
    s_error_msg[LCD_COLS] = '\0';
    display_set_screen(SCREEN_ERROR);
    s_dirty = true;
}

void display_set_calib_text(uint8_t line, const char *text)
{
    if (line >= LCD_ROWS) return;
    strncpy(s_calib_lines[line], text, LCD_COLS);
    s_calib_lines[line][LCD_COLS] = '\0';
    if (s_screen == SCREEN_CALIBRATION) s_dirty = true;
}

void display_settings_set_angle(float deg)
{
    if (s_settings_angle != deg) {
        s_settings_angle = deg;
        if (s_screen == SCREEN_SETTINGS) s_dirty = true;
    }
}

void display_settings_set_step(float step_deg)
{
    if (s_settings_step != step_deg) {
        s_settings_step = step_deg;
        if (s_screen == SCREEN_SETTINGS) s_dirty = true;
    }
}

void display_set_active_preset(int8_t idx)
{
    s_active_preset = idx;
    if (s_screen == SCREEN_MAIN) {
        memset(s_lcd_buf, 0, sizeof(s_lcd_buf));  // примусово перемалювати рядок 0
        s_dirty = true;
    }
}

void display_menu_next(void)
{
    if (s_menu_item < MENU_ITEMS - 1) {
        s_menu_item++;
        if (s_screen == SCREEN_MENU) s_dirty = true;
    }
}

void display_menu_prev(void)
{
    if (s_menu_item > 0) {
        s_menu_item--;
        if (s_screen == SCREEN_MENU) s_dirty = true;
    }
}

void display_menu_select(void)
{
    (void)0;
}

uint8_t display_menu_get_item(void)
{
    return s_menu_item;
}

void display_preset_scroll(int8_t delta)
{
    if (delta > 0 && s_preset_item < PRESET_COUNT - 1) {
        s_preset_item++;
    } else if (delta < 0 && s_preset_item > 0) {
        s_preset_item--;
    }
    if (s_screen == SCREEN_PRESET || s_screen == SCREEN_PRESET_EDIT_LIST) s_dirty = true;
}

uint8_t display_preset_get_item(void)
{
    return s_preset_item;
}

void display_preset_edit_set(uint8_t idx, float target_kN, float angle_deg, uint8_t field)
{
    s_preset_edit_idx = idx;
    s_preset_edit_target = target_kN;
    s_preset_edit_angle  = angle_deg;
    s_preset_edit_field  = field;
    if (s_screen == SCREEN_PRESET_EDIT) s_dirty = true;
}
