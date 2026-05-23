#ifndef __PRESET_H
#define __PRESET_H

#include <stdint.h>

typedef struct {
    const char *inj_name;     // тип форсунки, макс 3 символи: "300", "400"
    const char *op_name;      // операція, макс 8 символів
    float       force_min_kN;
    float       force_max_kN;
} Preset_t;

#define PRESET_COUNT  5

extern const Preset_t g_presets[PRESET_COUNT];

// Середина діапазону → ціль в кг (для s_target_kg у main.c)
static inline float preset_target_kg(uint8_t idx)
{
    if (idx >= PRESET_COUNT) return 0.0f;
    float mid_kN = (g_presets[idx].force_min_kN + g_presets[idx].force_max_kN) * 0.5f;
    return mid_kN * 101.97f;
}

#endif /* __PRESET_H */
