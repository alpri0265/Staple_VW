#ifndef __PRESET_H
#define __PRESET_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    const char *inj_name;     // тип форсунки, макс 3 символи: "300", "400"
    const char *op_name;      // операція, макс 8 символів
    float       force_min_kN;
    float       force_max_kN;
    float       angle_default_deg;
} Preset_t;

#define PRESET_COUNT  5

extern const Preset_t g_presets[PRESET_COUNT];

void  preset_init(void);
float preset_get_target_kN(uint8_t idx);
bool  preset_set_target_kN(uint8_t idx, float target_kN);
float preset_get_angle_deg(uint8_t idx);
bool  preset_set_angle_deg(uint8_t idx, float angle_deg);
void  preset_export_targets(float *dst, uint8_t count);
void  preset_import_targets(const float *src, uint8_t count);
void  preset_export_angles(float *dst, uint8_t count);
void  preset_import_angles(const float *src, uint8_t count);

// Середина діапазону → ціль в кг (для s_target_kg у main.c)
static inline float preset_target_kg(uint8_t idx)
{
    if (idx >= PRESET_COUNT) return 0.0f;
    return preset_get_target_kN(idx) * 101.97f;
}

#endif /* __PRESET_H */
