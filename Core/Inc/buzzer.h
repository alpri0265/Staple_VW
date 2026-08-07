#ifndef __BUZZER_H
#define __BUZZER_H

#include <stdbool.h>
#include <stdint.h>

void buzzer_init(void);
void buzzer_update(void);
void buzzer_tim_tick(void);  // викликати з TIM3 IRQ (100us) — генерує тон для пасивного п'єзо
void buzzer_beep(uint16_t ms);
void buzzer_on(void);   // безперервний тон, вимикається лише через buzzer_stop()
void buzzer_stop(void);
bool buzzer_is_active(void);

#endif /* __BUZZER_H */

