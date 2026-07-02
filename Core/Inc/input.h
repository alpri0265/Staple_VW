#ifndef __INPUT_H
#define __INPUT_H

#include <stdbool.h>
#include <stdint.h>

void   input_init(void);
void   input_update(void);           // викликати з main loop
void   input_debounce_tick(void);    // викликати з control timer IRQ
void   input_enc_isr(void);          // викликати з EXTI ISR (ENC_CLK falling)

bool   input_joy_up(void);           // джойстик вгору (з антидребезгом)
bool   input_joy_down(void);         // джойстик вниз
bool   input_enc_sw_pressed(void);   // latched подія натискання, скидається після читання
bool   input_enc_sw_held(void);      // кнопка утримана зараз
bool   input_enc_sw_released(void);  // latched подія відпускання, скидається після читання
int8_t input_enc_get_delta(void);    // кроки енкодера з останнього виклику

bool   input_stop_pressed(void);     // прапор аварійної зупинки
void   input_stop_clear(void);       // скинути прапор після обробки
void   input_stop_set(void);         // встановити прапор (з ISR)

#endif /* __INPUT_H */
