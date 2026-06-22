#ifndef __DISPLAY_H
#define __DISPLAY_H

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    SCREEN_MAIN,
    SCREEN_MENU,
    SCREEN_PRESET,
    SCREEN_PRESET_EDIT,   // редагування / додавання пресету
    SCREEN_CALIBRATION,
    SCREEN_SETTINGS,
    SCREEN_ERROR
} DisplayScreen;

typedef enum {
    PEDIT_INJ = 0,   // вибір типу форсунки
    PEDIT_OP,        // вибір операції
    PEDIT_FMIN,      // мінімальне зусилля
    PEDIT_FMAX,      // максимальне зусилля
    PEDIT_CONFIRM,   // підтвердити збереження
    PEDIT_DELETE,    // підтвердити видалення
} PresetEditStep_t;

void display_init(void);
void display_update(void);

void          display_set_screen(DisplayScreen s);
DisplayScreen display_get_screen(void);
void          display_set_force(float current_kN, float target_kN);
void          display_set_angle(float current_deg, float target_deg, bool reached);
void          display_show_error(const char *msg);
void          display_set_calib_text(uint8_t line, const char *text);

void display_set_active_preset(int8_t idx);
void display_settings_set_angle(float deg);

void    display_menu_next(void);
void    display_menu_prev(void);
void    display_menu_select(void);
uint8_t display_menu_get_item(void);

void    display_preset_scroll(int8_t delta);
uint8_t display_preset_get_item(void);

// Редактор пресету: оновити стан і перемалювати
void display_preset_edit_set(PresetEditStep_t step,
                              uint8_t inj_idx, uint8_t op_idx,
                              float fmin, float fmax, bool is_new);

void display_set_status(const char *status); // рядок 3: стан AUTO/MANUAL
void display_debug_line(const char *text);

#endif /* __DISPLAY_H */
