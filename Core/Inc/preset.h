#ifndef __PRESET_H
#define __PRESET_H

#include <stdint.h>
#include <stdbool.h>

#define PRESET_MAX      16
#define INJ_NAME_LEN     4   // 3 видимих + '\0'
#define OP_NAME_LEN      9   // 8 видимих + '\0'

typedef struct {
    char  inj_name[INJ_NAME_LEN];
    char  op_name[OP_NAME_LEN];
    float force_min_kN;
    float force_max_kN;
} Preset_t;

extern Preset_t g_presets[PRESET_MAX];
extern uint8_t  g_preset_count;

// Списки типів форсунок і операцій
#define INJ_TYPES_COUNT  8
#define OP_TYPES_COUNT  10
extern const char * const g_inj_types[INJ_TYPES_COUNT];
extern const char * const g_op_types[OP_TYPES_COUNT];

void    preset_init(void);
void    preset_load_defaults(void);
bool    preset_add(uint8_t inj_idx, uint8_t op_idx, float fmin, float fmax);
bool    preset_set(uint8_t idx, uint8_t inj_idx, uint8_t op_idx, float fmin, float fmax);
bool    preset_delete(uint8_t idx);
uint8_t preset_find_inj_idx(const char *name);
uint8_t preset_find_op_idx(const char *name);

static inline float preset_target_kg(uint8_t idx)
{
    if (idx >= g_preset_count) return 0.0f;
    float mid_kN = (g_presets[idx].force_min_kN + g_presets[idx].force_max_kN) * 0.5f;
    return mid_kN * 101.97f;
}

#endif /* __PRESET_H */
