#include "preset.h"
#include "config.h"

// PDE TDI Тип 300: розпилювач 8.5-9.0 кН, соленоїд 7.0-8.4 кН
// PDE TDI Тип 400: розпилювач 8.3-8.8 кН, соленоїд 8.3-9.7 кН, гайка вала 15.0-16.0 кН
const Preset_t g_presets[PRESET_COUNT] = {
    { "300", "Nozzle",   8.5f,  9.0f,  90.0f  },
    { "300", "Solenoid", 7.0f,  8.4f, 120.0f  },
    { "400", "Nozzle",   8.3f,  8.8f, 100.0f  },
    { "400", "Solenoid", 8.3f,  9.7f, 140.0f  },
    { "400", "ShaftNut", 15.0f, 16.0f, 180.0f },
};

static float s_preset_targets_kN[PRESET_COUNT];
static float s_preset_angles_deg[PRESET_COUNT];

static float clamp_target_kN(float target_kN)
{
    float max_kN = FORCE_MAX_KG / KN_TO_KG;
    if (target_kN < 0.0f) return 0.0f;
    if (target_kN > max_kN) return max_kN;
    return target_kN;
}

static float clamp_angle(float angle_deg)
{
    if (angle_deg < 0.0f) return 0.0f;
    if (angle_deg > ANGLE_MAX_DEG) return ANGLE_MAX_DEG;
    return angle_deg;
}

void preset_init(void)
{
    for (uint8_t i = 0; i < PRESET_COUNT; i++) {
        s_preset_targets_kN[i] = (g_presets[i].force_min_kN + g_presets[i].force_max_kN) * 0.5f;
        s_preset_angles_deg[i] = g_presets[i].angle_default_deg;
    }
}

float preset_get_target_kN(uint8_t idx)
{
    if (idx >= PRESET_COUNT) return FORCE_DEFAULT_KN;
    return s_preset_targets_kN[idx];
}

bool preset_set_target_kN(uint8_t idx, float target_kN)
{
    if (idx >= PRESET_COUNT) return false;
    s_preset_targets_kN[idx] = clamp_target_kN(target_kN);
    return true;
}

float preset_get_angle_deg(uint8_t idx)
{
    if (idx >= PRESET_COUNT) return ANGLE_DEFAULT_DEG;
    return s_preset_angles_deg[idx];
}

bool preset_set_angle_deg(uint8_t idx, float angle_deg)
{
    if (idx >= PRESET_COUNT) return false;
    s_preset_angles_deg[idx] = clamp_angle(angle_deg);
    return true;
}

void preset_export_targets(float *dst, uint8_t count)
{
    if (dst == 0) return;

    uint8_t limit = (count < PRESET_COUNT) ? count : PRESET_COUNT;
    for (uint8_t i = 0; i < limit; i++) {
        dst[i] = s_preset_targets_kN[i];
    }
}

void preset_import_targets(const float *src, uint8_t count)
{
    if (src == 0) return;

    uint8_t limit = (count < PRESET_COUNT) ? count : PRESET_COUNT;
    for (uint8_t i = 0; i < limit; i++) {
        s_preset_targets_kN[i] = clamp_target_kN(src[i]);
    }
}

void preset_export_angles(float *dst, uint8_t count)
{
    if (dst == 0) return;

    uint8_t limit = (count < PRESET_COUNT) ? count : PRESET_COUNT;
    for (uint8_t i = 0; i < limit; i++) {
        dst[i] = s_preset_angles_deg[i];
    }
}

void preset_import_angles(const float *src, uint8_t count)
{
    if (src == 0) return;

    uint8_t limit = (count < PRESET_COUNT) ? count : PRESET_COUNT;
    for (uint8_t i = 0; i < limit; i++) {
        s_preset_angles_deg[i] = clamp_angle(src[i]);
    }
}
