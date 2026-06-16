#include "portgremlin_oracle.h"
#include "portgremlin_persona.h"
#include "portgremlin_config.h"
#include "portgremlin_telemetry.h"
#include "portgremlin_evolve.h"
#include "utils/uartstdio.h"
#include "usblib/usblib.h"

PortGremlinOracle g_sOracle;

static const char * const g_ppcHostNames[] =
{
    "Unknown",
    "Windows",
    "Linux",
    "macOS",
    "Embedded"
};

static const char * const g_ppcBrainNames[] =
{
    "Idle",
    "Probe",
    "Escalate",
    "Corrupt",
    "Chaos"
};

void PortGremlinOracleInit(void)
{
    g_sOracle.eHost = HOST_UNKNOWN;
    g_sOracle.eBrainPhase = BRAIN_IDLE;
    g_sOracle.bBrainActive = false;
    g_sOracle.bContradictionMode = false;
    g_sOracle.bIdentityLocked = false;
    g_sOracle.ui16PinnedVID = 0;
    g_sOracle.ui16PinnedPID = 0;
    g_sOracle.ui32ResetCount = 0;
    g_sOracle.ui32ConfigLatencyTicks = 0;
    g_sOracle.ui32StableEnums = 0;
    g_sOracle.ui32Disconnects = 0;
    g_sOracle.ui32ToleranceScore = 0;
    g_sOracle.bSessionActive = false;
    g_sOracle.bConfigSet = false;
    g_sOracle.ui32SessionStartTick = 0;
}

const char *PortGremlinHostName(HostProfile eHost)
{
    if (eHost >= HOST_EMBEDDED + 1)
    {
        return "Unknown";
    }
    return g_ppcHostNames[eHost];
}

const char *PortGremlinBrainPhaseName(BrainPhase ePhase)
{
    if (ePhase >= BRAIN_CHAOS + 1)
    {
        return "Unknown";
    }
    return g_ppcBrainNames[ePhase];
}

static void OracleClassifyHost(void)
{
    uint32_t ui32Latency = g_sOracle.ui32ConfigLatencyTicks;

    if (g_sOracle.ui32ResetCount >= 2U)
    {
        g_sOracle.eHost = HOST_WINDOWS;
    }
    else if (ui32Latency > 80U)
    {
        g_sOracle.eHost = HOST_LINUX;
    }
    else if (ui32Latency > 0U && ui32Latency < 25U)
    {
        g_sOracle.eHost = HOST_WINDOWS;
    }
    else if (ui32Latency >= 25U && ui32Latency <= 80U)
    {
        g_sOracle.eHost = HOST_MACOS;
    }
    else if (!g_sOracle.bConfigSet)
    {
        g_sOracle.eHost = HOST_EMBEDDED;
    }
    else
    {
        g_sOracle.eHost = HOST_LINUX;
    }

    UARTprintf("[ORACLE] Host classified: %s (cfg=%u ticks, resets=%u)\n\r",
               PortGremlinHostName(g_sOracle.eHost),
               g_sOracle.ui32ConfigLatencyTicks,
               g_sOracle.ui32ResetCount);
    PortGremlinTelemetryHost(g_sOracle.eHost, g_sOracle.ui32ConfigLatencyTicks,
                             g_sOracle.ui32ResetCount);
}

void PortGremlinOracleOnEvent(uint32_t ui32Event)
{
    switch (ui32Event)
    {
        case USB_EVENT_CONNECTED:
            g_sOracle.bSessionActive = true;
            g_sOracle.bConfigSet = false;
            g_sOracle.ui32SessionStartTick = g_ui32SysTickCount;
            g_sOracle.ui32ResetCount = 0;
            g_sOracle.ui32ConfigLatencyTicks = 0;
            break;

        case USB_EVENT_CONFIG_SET:
            g_sOracle.bConfigSet = true;
            g_sOracle.ui32ConfigLatencyTicks =
                g_ui32SysTickCount - g_sOracle.ui32SessionStartTick;
            OracleClassifyHost();
            if (g_ePersona == PERSONA_SPECTRE)
            {
                PortGremlinPersonaForHost();
            }
            break;

        case USB_EVENT_RESET:
            g_sOracle.ui32ResetCount++;
            break;

        case USB_EVENT_DISCONNECTED:
            PortGremlinOracleOnDisconnect();
            break;

        default:
            break;
    }
}

void PortGremlinOracleOnEnumerate(void)
{
    g_sOracle.ui32StableEnums++;
    PortGremlinEvolveReward();

    if (g_sOracle.bBrainActive)
    {
        if (g_sOracle.ui32ToleranceScore < 100U)
        {
            g_sOracle.ui32ToleranceScore++;
        }
    }
}

void PortGremlinOracleOnDisconnect(void)
{
    g_sOracle.bSessionActive = false;
    g_sOracle.ui32Disconnects++;
    PortGremlinTelemetryDisconnect(g_sOracle.ui32Disconnects);
    PortGremlinEvolvePunish();

    if (g_sOracle.ui32ToleranceScore > 5U)
    {
        g_sOracle.ui32ToleranceScore -= 5U;
    }

    if (g_sOracle.bBrainActive && g_sOracle.eBrainPhase > BRAIN_PROBE)
    {
        UARTprintf("[BRAIN] Host rejected attack - de-escalating to PROBE\n\r");
        g_sOracle.eBrainPhase = BRAIN_PROBE;
        PortGremlinPersonaApply(PERSONA_PHANTOM);
    }
}

void PortGremlinBrainTick(void)
{
    if (!g_sOracle.bBrainActive)
    {
        return;
    }

    switch (g_sOracle.eBrainPhase)
    {
        case BRAIN_IDLE:
            g_sOracle.eBrainPhase = BRAIN_PROBE;
            PortGremlinPersonaApply(PERSONA_PHANTOM);
            PortGremlinTelemetryBrain(BRAIN_PROBE, g_sOracle.ui32ToleranceScore);
            UARTprintf("[BRAIN] Autonomous mode: entering PROBE\n\r");
            break;

        case BRAIN_PROBE:
            if (g_sOracle.ui32StableEnums >= 15U)
            {
                g_sOracle.eBrainPhase = BRAIN_ESCALATE;
                PortGremlinPersonaApply(PERSONA_STORM);
                PortGremlinTelemetryBrain(BRAIN_ESCALATE, g_sOracle.ui32ToleranceScore);
                UARTprintf("[BRAIN] Host tolerant - escalating to STORM\n\r");
            }
            break;

        case BRAIN_ESCALATE:
            if (g_sOracle.ui32StableEnums >= 40U)
            {
                g_sOracle.eBrainPhase = BRAIN_CORRUPT;
                PortGremlinPersonaApply(PERSONA_CHIMERA);
                PortGremlinTelemetryBrain(BRAIN_CORRUPT, g_sOracle.ui32ToleranceScore);
                UARTprintf("[BRAIN] Host still standing - escalating to CORRUPT\n\r");
            }
            break;

        case BRAIN_CORRUPT:
            if (g_sOracle.ui32StableEnums >= 80U)
            {
                g_sOracle.eBrainPhase = BRAIN_CHAOS;
                PortGremlinPersonaApply(PERSONA_HAUNTED);
                g_sConfig.ui32CycleIntervalTicks = PORTGREMLIN_CYCLE_INTERVAL_MIN;
                PortGremlinTelemetryBrain(BRAIN_CHAOS, g_sOracle.ui32ToleranceScore);
                UARTprintf("[BRAIN] Maximum chaos - HAUNTED persona engaged\n\r");
            }
            break;

        case BRAIN_CHAOS:
            break;
    }
}

void PortGremlinOraclePrintReport(void)
{
    UARTprintf("\n\r=== ORACLE REPORT ===\n\r");
    UARTprintf("Host profile:  %s\n\r", PortGremlinHostName(g_sOracle.eHost));
    UARTprintf("Brain:         %s (%s)\n\r",
               g_sOracle.bBrainActive ? "ACTIVE" : "off",
               PortGremlinBrainPhaseName(g_sOracle.eBrainPhase));
    UARTprintf("Contradiction: %s", g_sOracle.bContradictionMode ? "ON" : "OFF");
    if (g_sOracle.bContradictionMode)
    {
        UARTprintf(" (VID=0x%04X PID=0x%04X)", g_sOracle.ui16PinnedVID, g_sOracle.ui16PinnedPID);
    }
    UARTprintf("\n\r");
    UARTprintf("Config latency:%u ticks (%u ms)\n\r",
               g_sOracle.ui32ConfigLatencyTicks,
               g_sOracle.ui32ConfigLatencyTicks * 10U);
    UARTprintf("USB resets:    %u\n\r", g_sOracle.ui32ResetCount);
    UARTprintf("Stable enums:  %u\n\r", g_sOracle.ui32StableEnums);
    UARTprintf("Disconnects:   %u\n\r", g_sOracle.ui32Disconnects);
    UARTprintf("Tolerance:     %u/100\n\r", g_sOracle.ui32ToleranceScore);
    UARTprintf("=====================\n\r");
}
