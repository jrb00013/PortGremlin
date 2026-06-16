#include "portgremlin_config.h"

PortGremlinConfig g_sConfig;

void PortGremlinConfigInit(void)
{
    g_sConfig.bAutoCycle = true;
    g_sConfig.bMalformedMode = false;
    g_sConfig.bRandomStrings = true;
    g_sConfig.bRealVIDPID = false;
    g_sConfig.ui32CycleIntervalTicks = PORTGREMLIN_CYCLE_INTERVAL_DEF;
    g_sConfig.bForceCycle = false;
    g_sConfig.bForceReenum = false;
    g_sConfig.ui32EnumCount = 0;
    g_sConfig.ui32CycleCount = 0;

    for (int i = 0; i < (int)NUM_DEVICE_TYPES; i++)
    {
        g_sConfig.bClassEnabled[i] = true;
    }
}

DeviceType PortGremlinNextEnabledDevice(DeviceType eCurrent)
{
    DeviceType eNext = eCurrent;

    for (int i = 0; i < (int)NUM_DEVICE_TYPES; i++)
    {
        eNext = (DeviceType)(((int)eNext + 1) % (int)NUM_DEVICE_TYPES);
        if (g_sConfig.bClassEnabled[eNext])
        {
            return eNext;
        }
    }

    return eCurrent;
}

const char *PortGremlinDeviceName(DeviceType eDevice)
{
    switch (eDevice)
    {
        case DEVICE_KEYBOARD: return "Keyboard";
        case DEVICE_AUDIO:    return "Audio";
        case DEVICE_PRINTER:  return "Printer";
        case DEVICE_MIDI:     return "MIDI";
        case DEVICE_GAMEPAD:  return "Gamepad";
        default:              return "Unknown";
    }
}
