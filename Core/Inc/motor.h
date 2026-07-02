#ifndef __MOTOR_H
#define __MOTOR_H

#include "stm32f4xx_hal.h"
#include <stdbool.h>
#include <stdint.h>

typedef enum {
    MOTOR_IDLE,
    MOTOR_MOVING_UP,
    MOTOR_MOVING_DOWN,
    MOTOR_STOPPING,
    MOTOR_ERROR
} MotorState;

void motor_init(void);
void motor_update(void);           // викликати з main loop
void motor_tim_tick(void);         // викликати з control timer IRQ — генерує STEP

void motor_move_up(uint16_t speed);
void motor_move_down(uint16_t speed);
void motor_burst_up(uint16_t speed, uint16_t steps);
void motor_burst_down(uint16_t speed, uint16_t steps);
void motor_nudge_up(uint16_t speed);   // тонка підстройка вгору (без розгону)
void motor_nudge_down(uint16_t speed); // тонка підстройка вниз (без розгону)
void motor_stop(void);             // м'яка зупинка
void motor_emergency_stop(void);   // миттєва зупинка (можна з ISR)
void motor_clear_error(void);      // скинути стан ERROR → IDLE після підтвердження

bool motor_is_limit_top(void);
bool motor_is_limit_bot(void);
bool motor_is_running(void);
bool motor_is_burst_active(void);

float motor_get_position_mm(void);
void  motor_reset_position(void);
MotorState motor_get_state(void);

#endif /* __MOTOR_H */
