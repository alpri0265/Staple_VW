#ifndef __CALIBRATION_H
#define __CALIBRATION_H

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    CALIB_STEP_IDLE = 0,
    CALIB_STEP_TARE,        // "Приберіть вантаж" → tare()
    CALIB_STEP_LOAD,        // "Поставте еталон"
    CALIB_STEP_INPUT_MASS,  // Введення маси енкодером
    CALIB_STEP_CALCULATE,   // Розрахунок scale
    CALIB_STEP_SAVE,        // Збереження в Flash
    CALIB_STEP_VERIFY,      // Показ результату
    CALIB_STEP_DONE
} CalibStep;

void      calib_init(void);
void      calib_update(void);       // викликати з main loop коли active
void      calib_start(float target_kg); // вхід в режим калібровки
void      calib_confirm(void);      // підтвердження кроку (кнопка енкодера)
void      calib_adjust(int8_t d);   // зміна маси (енкодером)
bool      calib_is_active(void);
CalibStep calib_get_step(void);

// Flash EEPROM — загальний доступ для завантаження налаштувань при старті
bool  flash_load(float *scale, int32_t *offset, float *target);
bool  flash_save(float scale, int32_t offset, float target);

#endif /* __CALIBRATION_H */
