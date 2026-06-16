#ifndef PORTGREMLIN_ORACLE_H
#define PORTGREMLIN_ORACLE_H

#include <stdint.h>
#include <stdbool.h>

typedef enum
{
    HOST_UNKNOWN = 0,
    HOST_WINDOWS,
    HOST_LINUX,
    HOST_MACOS,
    HOST_EMBEDDED
} HostProfile;

typedef enum
{
    BRAIN_IDLE = 0,
    BRAIN_PROBE,
    BRAIN_ESCALATE,
    BRAIN_CORRUPT,
    BRAIN_CHAOS
} BrainPhase;

typedef struct
{
    volatile HostProfile eHost;
    volatile BrainPhase eBrainPhase;
    volatile bool bBrainActive;
    volatile bool bContradictionMode;
    volatile bool bIdentityLocked;
    volatile uint16_t ui16PinnedVID;
    volatile uint16_t ui16PinnedPID;
    volatile uint32_t ui32ResetCount;
    volatile uint32_t ui32ConfigLatencyTicks;
    volatile uint32_t ui32StableEnums;
    volatile uint32_t ui32Disconnects;
    volatile uint32_t ui32ToleranceScore;
    volatile bool bSessionActive;
    volatile bool bConfigSet;
    volatile uint32_t ui32SessionStartTick;
} PortGremlinOracle;

extern PortGremlinOracle g_sOracle;
extern volatile uint32_t g_ui32SysTickCount;

void PortGremlinOracleInit(void);
void PortGremlinOracleOnEvent(uint32_t ui32Event);
void PortGremlinOracleOnEnumerate(void);
void PortGremlinOracleOnDisconnect(void);
void PortGremlinBrainTick(void);
void PortGremlinOraclePrintReport(void);
const char *PortGremlinHostName(HostProfile eHost);
const char *PortGremlinBrainPhaseName(BrainPhase ePhase);

#endif
