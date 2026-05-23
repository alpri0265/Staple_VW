#include "preset.h"

// PDE TDI Тип 300: розпилювач 8.5-9.0 кН, соленоїд 7.0-8.4 кН
// PDE TDI Тип 400: розпилювач 8.3-8.8 кН, соленоїд 8.3-9.7 кН, гайка вала 15.0-16.0 кН
const Preset_t g_presets[PRESET_COUNT] = {
    { "300", "Nozzle",   8.5f,  9.0f  },
    { "300", "Solenoid", 7.0f,  8.4f  },
    { "400", "Nozzle",   8.3f,  8.8f  },
    { "400", "Solenoid", 8.3f,  9.7f  },
    { "400", "ShaftNut", 15.0f, 16.0f },
};
