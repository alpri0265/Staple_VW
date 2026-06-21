#ifndef __DISPLAY_H
#define __DISPLAY_H

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    SCREEN_MAIN,
    SCREEN_MENU,
    SCREEN_PRESET,        // вибір пресету (тип НФ + операція)
    SCREEN_CALIBRATION,
    SCREEN_SETTINGS,
    SCREEN_ERROR
} DisplayScreen;

void display_init(void);
void display_update(void);                       // оновити LCD якщо змінились дані

void          display_set_screen(DisplayScreen s);
DisplayScreen display_get_screen(void);
// current та target передаються в кН
void          display_set_force(float current_kN, float target_kN);
// current та target — кут у градусах; якщо reached=true → рядок блимає "ДОСЯГНУТО!"
void          display_set_angle(float current_deg, float target_deg, bool reached);
void          display_show_error(const char *msg);
void          display_set_calib_text(uint8_t line, const char *text);

// Активний пресет для головного екрану (-1 = немає)
void display_set_active_preset(int8_t idx);

// Налаштування: відображення цільового кута у SCREEN_SETTINGS
void display_settings_set_angle(float deg);

// Меню (навігація енкодером)
void    display_menu_next(void);
void    display_menu_prev(void);
void    display_menu_select(void);
uint8_t display_menu_get_item(void);

// Список пресетів (навігація енкодером)
void    display_preset_scroll(int8_t delta);
uint8_t display_preset_get_item(void);

// Тимчасова діагностика: пряме виведення рядка на рядок 3 LCD
void display_debug_line(const char *text);

#endif /* __DISPLAY_H */
