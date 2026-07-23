#ifndef __BUZZER_H
#define __BUZZER_H

#include <stdbool.h>
#include <stdint.h>

void buzzer_init(void);
void buzzer_update(void);
void buzzer_beep(uint16_t ms);
void buzzer_stop(void);
bool buzzer_is_active(void);

#endif /* __BUZZER_H */

