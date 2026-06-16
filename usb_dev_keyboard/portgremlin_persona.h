#ifndef PORTGREMLIN_PERSONA_H
#define PORTGREMLIN_PERSONA_H

#include <stdint.h>
#include <stdbool.h>
#include "usb_keyb_structs.h"

typedef enum
{
    PERSONA_MANUAL = 0,
    PERSONA_CHIMERA,
    PERSONA_MIMIC,
    PERSONA_STORM,
    PERSONA_HAUNTED,
    PERSONA_PHANTOM,
    PERSONA_SPECTRE,
    PERSONA_NUM
} GremlinPersona;

extern GremlinPersona g_ePersona;

void PortGremlinPersonaInit(void);
void PortGremlinPersonaApply(GremlinPersona ePersona);
void PortGremlinPersonaNext(void);
void PortGremlinPersonaForHost(void);
const char *PortGremlinPersonaName(GremlinPersona ePersona);
void PortGremlinChoreoStart(uint32_t ui32ScriptId);
void PortGremlinChoreoStop(void);
void PortGremlinChoreoTick(void);

#endif
