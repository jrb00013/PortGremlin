#ifndef PORTGREMLIN_VIDPID_H
#define PORTGREMLIN_VIDPID_H

#include <stdint.h>
#include "usb_keyb_structs.h"

void PortGremlinRandomizeVIDPID(void *pDevice, VIDPIDDeviceType eType);
void PortGremlinApplyPowerAttributes(void *pDevice, VIDPIDDeviceType eType);
void PortGremlinSetPinnedVIDPID(uint16_t ui16VID, uint16_t ui16PID);

#endif
