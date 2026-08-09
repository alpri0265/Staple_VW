#ifndef __DISPLAY_H
#define __DISPLAY_H

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    SCREEN_MAIN,
    SCREEN_AUTO,
    SCREEN_MENU,
    SCREEN_PRESET,        // вибір пресету (тип НФ + операція)
    SCREEN_PRESET_EDIT_LIST,
    SCREEN_PRESET_EDIT,
    SCREEN_CALIBRATION,
    SCREEN_SETTINGS,
    SCREEN_ERROR
} DisplayScreen;

typedef enum {
    DISPLAY_MODE_IDLE = 0,
    DISPLAY_MODE_APPROACH,
    DISPLAY_MODE_SOFT,
    DISPLAY_MODE_PRESS,
    DISPLAY_MODE_ASEEK,
    DISPLAY_MODE_HOLD,
    DISPLAY_MODE_DONE,
    DISPLAY_MODE_FINE,
    DISPLAY_MODE_RETRACT
} DisplayMotionMode;

void display_init(void);
void display_update(void);                       // оновити LCD якщо змінились дані

void          display_set_screen(DisplayScreen s);
DisplayScreen display_get_screen(void);
// current та target передаються в кН
void          display_set_force(float current_kN, float target_kN);
void          display_set_angle(float current_deg, float target_deg, bool reached);
void          display_set_motion_mode(DisplayMotionMode mode);
void          display_set_motion_speed(uint16_t speed_steps);
void          display_set_auto_metrics(float current_kN, float target_kN, float error_kN);
void          display_show_error(const char *msg);
void          display_set_calib_text(uint8_t line, const char *text);
void          display_settings_set_angle(float deg);
void          display_settings_set_step(float step_deg);

// Активний пресет для головного екрану (-1 = немає)
void display_set_active_preset(int8_t idx);

// Меню (навігація енкодером)
void    display_menu_next(void);
void    display_menu_prev(void);
void    display_menu_select(void);
uint8_t display_menu_get_item(void);

// Список пресетів (навігація енкодером)
void    display_preset_scroll(int8_t delta);
uint8_t display_preset_get_item(void);

void display_preset_edit_set(uint8_t idx, float target_kN, float angle_deg, uint8_t field);

#endif /* __DISPLAY_H */
