#ifndef PORTGREMLIN_STRINGS_H
#define PORTGREMLIN_STRINGS_H

#include <stdint.h>
#include "usb_keyb_structs.h"

#define PORTGREMLIN_STRING_MAX_CHARS 24

void PortGremlinStringsInit(void);
void PortGremlinRandomizeIdentity(DeviceType eDevice);
void PortGremlinRestoreIdentity(DeviceType eDevice);
void PortGremlinSetIdentityStrings(const char *pcManufacturer, const char *pcProduct,
                                   DeviceType eDevice);

#endif
