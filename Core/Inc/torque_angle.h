#ifndef __TORQUE_ANGLE_H
#define __TORQUE_ANGLE_H

#include <stdbool.h>
#include <stdint.h>

void     torque_angle_init(void);
void     torque_angle_zero(void);
float    torque_angle_get_deg(void);
void     torque_angle_set_target(float deg);
float    torque_angle_get_target(void);
bool     torque_angle_is_reached(void);
uint16_t torque_angle_get_raw(void);

#endif /* __TORQUE_ANGLE_H */
