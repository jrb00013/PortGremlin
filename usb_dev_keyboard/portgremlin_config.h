#ifndef PORTGREMLIN_CONFIG_H
#define PORTGREMLIN_CONFIG_H

#include <stdbool.h>
#include <stdint.h>
#include "usb_keyb_structs.h"

#define PORTGREMLIN_CYCLE_INTERVAL_MIN  1
#define PORTGREMLIN_CYCLE_INTERVAL_MAX  100
#define PORTGREMLIN_CYCLE_INTERVAL_DEF  5

typedef struct
{
    volatile bool bAutoCycle;
    volatile bool bMalformedMode;
    volatile bool bRandomStrings;
    volatile bool bRealVIDPID;
    volatile uint32_t ui32CycleIntervalTicks;
    volatile bool bClassEnabled[NUM_DEVICE_TYPES];
    volatile bool bForceCycle;
    volatile bool bForceReenum;
    volatile uint32_t ui32EnumCount;
    volatile uint32_t ui32CycleCount;
} PortGremlinConfig;

extern PortGremlinConfig g_sConfig;

void PortGremlinConfigInit(void);
DeviceType PortGremlinNextEnabledDevice(DeviceType eCurrent);
const char *PortGremlinDeviceName(DeviceType eDevice);

#endif
