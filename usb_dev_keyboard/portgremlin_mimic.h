#ifndef PORTGREMLIN_MIMIC_H
#define PORTGREMLIN_MIMIC_H

#include <stdint.h>
#include <stdbool.h>
#include "usb_keyb_structs.h"

typedef struct
{
    uint16_t ui16VID;
    uint16_t ui16PID;
    DeviceType eClass;
    const char *pcManufacturer;
    const char *pcProduct;
} MimicProfile;

uint32_t PortGremlinMimicCount(void);
const MimicProfile *PortGremlinMimicGet(uint32_t ui32Index);
bool PortGremlinMimicApply(uint32_t ui32Index, DeviceType *peTargetClass);
void PortGremlinMimicPrintVault(void);

#endif
