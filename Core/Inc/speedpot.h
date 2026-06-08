#ifndef __SPEEDPOT_H
#define __SPEEDPOT_H

#include <stdint.h>

void     speedpot_init(void);
uint16_t speedpot_get_speed(void);   // повертає швидкість SPEED_POT_MIN..SPEED_POT_MAX

#endif /* __SPEEDPOT_H */
