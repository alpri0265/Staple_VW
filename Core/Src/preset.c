#include "preset.h"
#include <string.h>

Preset_t g_presets[PRESET_MAX];
uint8_t  g_preset_count = 0;

const char * const g_inj_types[INJ_TYPES_COUNT] = {
    "300", "400", "PD1", "PD2", "1.9", "2.0", "CR1", "OTH"
};

const char * const g_op_types[OP_TYPES_COUNT] = {
    "Nozzle", "Solenoid", "ShaftNut", "Press",
    "Valve",  "Spring",   "Assem",    "Disasm", "Tighten", "Other"
};

void preset_init(void)
{
    g_preset_count = 0;
}

void preset_load_defaults(void)
{
    g_preset_count = 0;
    preset_add(0, 0, 8.5f,  9.0f);
    preset_add(0, 1, 7.0f,  8.4f);
    preset_add(1, 0, 8.3f,  8.8f);
    preset_add(1, 1, 8.3f,  9.7f);
    preset_add(1, 2, 15.0f, 16.0f);
}

bool preset_add(uint8_t inj_idx, uint8_t op_idx, float fmin, float fmax)
{
    if (g_preset_count >= PRESET_MAX) return false;
    if (inj_idx >= INJ_TYPES_COUNT) inj_idx = INJ_TYPES_COUNT - 1;
    if (op_idx  >= OP_TYPES_COUNT)  op_idx  = OP_TYPES_COUNT  - 1;
    Preset_t *p = &g_presets[g_preset_count++];
    strncpy(p->inj_name, g_inj_types[inj_idx], INJ_NAME_LEN - 1);
    p->inj_name[INJ_NAME_LEN - 1] = '\0';
    strncpy(p->op_name, g_op_types[op_idx], OP_NAME_LEN - 1);
    p->op_name[OP_NAME_LEN - 1] = '\0';
    p->force_min_kN = fmin;
    p->force_max_kN = fmax;
    return true;
}

bool preset_set(uint8_t idx, uint8_t inj_idx, uint8_t op_idx, float fmin, float fmax)
{
    if (idx >= g_preset_count) return false;
    if (inj_idx >= INJ_TYPES_COUNT) inj_idx = INJ_TYPES_COUNT - 1;
    if (op_idx  >= OP_TYPES_COUNT)  op_idx  = OP_TYPES_COUNT  - 1;
    Preset_t *p = &g_presets[idx];
    strncpy(p->inj_name, g_inj_types[inj_idx], INJ_NAME_LEN - 1);
    p->inj_name[INJ_NAME_LEN - 1] = '\0';
    strncpy(p->op_name, g_op_types[op_idx], OP_NAME_LEN - 1);
    p->op_name[OP_NAME_LEN - 1] = '\0';
    p->force_min_kN = fmin;
    p->force_max_kN = fmax;
    return true;
}

bool preset_delete(uint8_t idx)
{
    if (idx >= g_preset_count) return false;
    for (uint8_t i = idx; i < g_preset_count - 1; i++) {
        g_presets[i] = g_presets[i + 1];
    }
    g_preset_count--;
    return true;
}

uint8_t preset_find_inj_idx(const char *name)
{
    for (uint8_t i = 0; i < INJ_TYPES_COUNT; i++) {
        if (strncmp(g_inj_types[i], name, INJ_NAME_LEN - 1) == 0) return i;
    }
    return INJ_TYPES_COUNT - 1;
}

uint8_t preset_find_op_idx(const char *name)
{
    for (uint8_t i = 0; i < OP_TYPES_COUNT; i++) {
        if (strncmp(g_op_types[i], name, OP_NAME_LEN - 1) == 0) return i;
    }
    return OP_TYPES_COUNT - 1;
}
