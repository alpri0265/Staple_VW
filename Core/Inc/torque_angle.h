#ifndef __TORQUE_ANGLE_H
#define __TORQUE_ANGLE_H

#include <stdbool.h>
#include <stdint.h>

// Ініціалізація: GPIO PC6/PC7 як TIM3_CH1/CH2, запуск TIM3 у Encoder mode (4x)
void  torque_angle_init(void);

// Скинути відлік у 0° (викликати перед початком затяжки)
void  torque_angle_zero(void);

// Поточний кут від нульової точки (°), може бути від'ємним
float torque_angle_get_deg(void);

// Задати цільовий кут (°)
void  torque_angle_set_target(float deg);
float torque_angle_get_target(void);

// Чи досягнуто |кут| >= цільового
bool  torque_angle_is_reached(void);

// Сире значення лічильника TIM3 (для діагностики)
uint16_t torque_angle_get_raw(void);

#endif /* __TORQUE_ANGLE_H */
