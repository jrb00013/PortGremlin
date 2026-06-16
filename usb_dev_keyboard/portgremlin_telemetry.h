#ifndef PORTGREMLIN_TELEMETRY_H
#define PORTGREMLIN_TELEMETRY_H

#include <stdint.h>
#include <stdbool.h>
#include "portgremlin_oracle.h"
#include "portgremlin_persona.h"

extern bool g_bTelemetryEnabled;

void PortGremlinTelemetryInit(void);
void PortGremlinTelemetryToggle(void);
void PortGremlinTelemetryHost(HostProfile eHost, uint32_t ui32Latency, uint32_t ui32Resets);
void PortGremlinTelemetryEnum(uint16_t ui16VID, uint16_t ui16PID, const char *pcClass);
void PortGremlinTelemetryPersona(GremlinPersona ePersona);
void PortGremlinTelemetryBrain(BrainPhase ePhase, uint32_t ui32Tolerance);
void PortGremlinTelemetryDisconnect(uint32_t ui32Total);
void PortGremlinTelemetryEvolve(uint32_t ui32Gen, uint32_t ui32Fitness);
void PortGremlinTelemetryCurrentIdentity(void);

#endif
