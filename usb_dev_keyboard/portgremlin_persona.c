#include <stdlib.h>
#include "portgremlin_persona.h"
#include "portgremlin_oracle.h"
#include "portgremlin_mimic.h"
#include "portgremlin_config.h"
#include "portgremlin_telemetry.h"
#include "utils/uartstdio.h"

GremlinPersona g_ePersona = PERSONA_MANUAL;

typedef struct
{
    GremlinPersona ePersona;
    uint32_t ui32DwellTicks;
} ChoreoStep;

static const ChoreoStep g_psChoreoRedTeam[] =
{
    { PERSONA_PHANTOM, 80 },
    { PERSONA_MIMIC, 40 },
    { PERSONA_STORM, 20 },
    { PERSONA_CHIMERA, 15 },
    { PERSONA_HAUNTED, 10 },
};

static const ChoreoStep g_psChoreoStealth[] =
{
    { PERSONA_MIMIC, 120 },
    { PERSONA_PHANTOM, 60 },
    { PERSONA_MIMIC, 60 },
};

static const ChoreoStep g_psChoreoBlitz[] =
{
    { PERSONA_STORM, 10 },
    { PERSONA_HAUNTED, 5 },
    { PERSONA_CHIMERA, 5 },
};

typedef struct
{
    const ChoreoStep *psSteps;
    uint32_t ui32StepCount;
    const char *pcName;
} ChoreoScript;

static const ChoreoScript g_psChoreoScripts[] =
{
    { g_psChoreoRedTeam, sizeof(g_psChoreoRedTeam) / sizeof(ChoreoStep), "RedTeam" },
    { g_psChoreoStealth, sizeof(g_psChoreoStealth) / sizeof(ChoreoStep), "Stealth" },
    { g_psChoreoBlitz, sizeof(g_psChoreoBlitz) / sizeof(ChoreoStep), "Blitz" },
};

static struct
{
    bool bActive;
    uint32_t ui32ScriptId;
    uint32_t ui32StepIndex;
    uint32_t ui32StepTicks;
} g_sChoreo;

static const char * const g_ppcPersonaNames[] =
{
    "Manual",
    "Chimera",
    "Mimic",
    "Storm",
    "Haunted",
    "Phantom",
    "Spectre"
};

void PortGremlinPersonaInit(void)
{
    g_ePersona = PERSONA_MANUAL;
    g_sChoreo.bActive = false;
}

const char *PortGremlinPersonaName(GremlinPersona ePersona)
{
    if (ePersona >= PERSONA_NUM)
    {
        return "Unknown";
    }
    return g_ppcPersonaNames[ePersona];
}

void PortGremlinPersonaApply(GremlinPersona ePersona)
{
    g_ePersona = ePersona;

    switch (ePersona)
    {
        case PERSONA_MANUAL:
            break;

        case PERSONA_CHIMERA:
            g_sOracle.bIdentityLocked = false;
            g_sConfig.bAutoCycle = true;
            g_sConfig.bMalformedMode = true;
            g_sConfig.bRandomStrings = true;
            g_sConfig.bRealVIDPID = true;
            g_sConfig.ui32CycleIntervalTicks = 3;
            g_sOracle.bContradictionMode = false;
            break;

        case PERSONA_MIMIC:
            g_sOracle.bIdentityLocked = false;
            g_sConfig.bAutoCycle = true;
            g_sConfig.bMalformedMode = false;
            g_sConfig.bRandomStrings = false;
            g_sConfig.bRealVIDPID = true;
            g_sConfig.ui32CycleIntervalTicks = 15;
            g_sOracle.bContradictionMode = false;
            PortGremlinMimicApply(rand() % PortGremlinMimicCount(), NULL);
            break;

        case PERSONA_STORM:
            g_sOracle.bIdentityLocked = false;
            g_sConfig.bAutoCycle = true;
            g_sConfig.bMalformedMode = false;
            g_sConfig.bRandomStrings = true;
            g_sConfig.bRealVIDPID = false;
            g_sConfig.ui32CycleIntervalTicks = PORTGREMLIN_CYCLE_INTERVAL_MIN;
            g_sOracle.bContradictionMode = false;
            for (int i = 0; i < (int)NUM_DEVICE_TYPES; i++)
            {
                g_sConfig.bClassEnabled[i] = true;
            }
            break;

        case PERSONA_HAUNTED:
            g_sConfig.bAutoCycle = true;
            g_sConfig.bMalformedMode = true;
            g_sConfig.bRandomStrings = false;
            g_sConfig.bRealVIDPID = false;
            g_sConfig.ui32CycleIntervalTicks = 4;
            g_sOracle.bContradictionMode = true;
            g_sOracle.ui16PinnedVID = (uint16_t)(0x1000 + (rand() % 0xEFFF));
            g_sOracle.ui16PinnedPID = (uint16_t)(0x1000 + (rand() % 0xEFFF));
            UARTprintf("[HAUNTED] Contradiction lock VID=0x%04X PID=0x%04X\n\r",
                       g_sOracle.ui16PinnedVID, g_sOracle.ui16PinnedPID);
            break;

        case PERSONA_PHANTOM:
            g_sOracle.bIdentityLocked = false;
            g_sConfig.bAutoCycle = true;
            g_sConfig.bMalformedMode = false;
            g_sConfig.bRandomStrings = false;
            g_sConfig.bRealVIDPID = true;
            g_sConfig.ui32CycleIntervalTicks = 50;
            g_sOracle.bContradictionMode = false;
            break;

        case PERSONA_SPECTRE:
            g_sConfig.bAutoCycle = true;
            g_sOracle.bContradictionMode = false;
            PortGremlinPersonaForHost();
            break;

        default:
            break;
    }

    UARTprintf("[PERSONA] %s engaged\n\r", PortGremlinPersonaName(ePersona));
    PortGremlinTelemetryPersona(ePersona);
}

void PortGremlinPersonaNext(void)
{
    GremlinPersona eNext = (GremlinPersona)(((int)g_ePersona + 1) % (int)PERSONA_NUM);
    if (eNext == PERSONA_MANUAL)
    {
        eNext = PERSONA_CHIMERA;
    }
    PortGremlinPersonaApply(eNext);
}

void PortGremlinPersonaForHost(void)
{
    switch (g_sOracle.eHost)
    {
        case HOST_WINDOWS:
            PortGremlinPersonaApply(PERSONA_CHIMERA);
            break;
        case HOST_LINUX:
            PortGremlinPersonaApply(PERSONA_HAUNTED);
            break;
        case HOST_MACOS:
            PortGremlinMimicApply(2, NULL);
            PortGremlinPersonaApply(PERSONA_MIMIC);
            break;
        case HOST_EMBEDDED:
            PortGremlinPersonaApply(PERSONA_STORM);
            break;
        default:
            PortGremlinPersonaApply(PERSONA_PHANTOM);
            break;
    }
}

void PortGremlinChoreoStart(uint32_t ui32ScriptId)
{
    if (ui32ScriptId >= (sizeof(g_psChoreoScripts) / sizeof(ChoreoScript)))
    {
        UARTprintf("[CHOREO] Unknown script %u\n\r", ui32ScriptId);
        return;
    }

    g_sChoreo.bActive = true;
    g_sChoreo.ui32ScriptId = ui32ScriptId;
    g_sChoreo.ui32StepIndex = 0;
    g_sChoreo.ui32StepTicks = 0;
    g_sOracle.bBrainActive = false;

    UARTprintf("[CHOREO] Starting '%s'\n\r", g_psChoreoScripts[ui32ScriptId].pcName);
    PortGremlinPersonaApply(g_psChoreoScripts[ui32ScriptId].psSteps[0].ePersona);
}

void PortGremlinChoreoStop(void)
{
    g_sChoreo.bActive = false;
}

void PortGremlinChoreoTick(void)
{
    if (!g_sChoreo.bActive)
    {
        return;
    }

    const ChoreoScript *psScript = &g_psChoreoScripts[g_sChoreo.ui32ScriptId];
    const ChoreoStep *psStep = &psScript->psSteps[g_sChoreo.ui32StepIndex];

    g_sChoreo.ui32StepTicks++;
    if (g_sChoreo.ui32StepTicks < psStep->ui32DwellTicks)
    {
        return;
    }

    g_sChoreo.ui32StepTicks = 0;
    g_sChoreo.ui32StepIndex++;

    if (g_sChoreo.ui32StepIndex >= psScript->ui32StepCount)
    {
        g_sChoreo.bActive = false;
        UARTprintf("[CHOREO] '%s' complete\n\r", psScript->pcName);
        return;
    }

    PortGremlinPersonaApply(psScript->psSteps[g_sChoreo.ui32StepIndex].ePersona);
}
