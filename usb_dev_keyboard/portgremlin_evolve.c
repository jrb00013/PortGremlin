#include <stdlib.h>
#include "portgremlin_evolve.h"
#include "portgremlin_config.h"
#include "portgremlin_oracle.h"
#include "portgremlin_telemetry.h"
#include "utils/uartstdio.h"

bool g_bEvolveActive = false;
AttackGenome g_sGenome;
uint32_t g_ui32EvolveGeneration;

static AttackGenome g_sBestGenome;
static uint32_t g_ui32EnumsSinceMutate;

static uint8_t RandByte(void)
{
    return (uint8_t)(rand() & 0xFF);
}

static void MutateGenome(AttackGenome *psGenome)
{
    switch (RandByte() % 4)
    {
        case 0:
            psGenome->ui8Interval = (uint8_t)(PORTGREMLIN_CYCLE_INTERVAL_MIN +
                (RandByte() % (PORTGREMLIN_CYCLE_INTERVAL_MAX -
                                PORTGREMLIN_CYCLE_INTERVAL_MIN + 1)));
            break;
        case 1:
            psGenome->ui8Malformed ^= 1;
            break;
        case 2:
            psGenome->ui8RealVid ^= 1;
            break;
        default:
            psGenome->ui8Contradiction ^= 1;
            break;
    }
}

static void GenomeToConfig(const AttackGenome *psGenome)
{
    g_sConfig.ui32CycleIntervalTicks = psGenome->ui8Interval;
    g_sConfig.bMalformedMode = psGenome->ui8Malformed != 0;
    g_sConfig.bRealVIDPID = psGenome->ui8RealVid != 0;
    g_sConfig.bRandomStrings = true;
    g_sConfig.bAutoCycle = true;

    if (psGenome->ui8Contradiction)
    {
        g_sOracle.bContradictionMode = true;
        g_sOracle.bIdentityLocked = true;
        g_sOracle.ui16PinnedVID = (uint16_t)(0x1000 + (rand() % 0xEFFF));
        g_sOracle.ui16PinnedPID = (uint16_t)(0x1000 + (rand() % 0xEFFF));
    }
    else
    {
        g_sOracle.bContradictionMode = false;
        g_sOracle.bIdentityLocked = false;
    }
}

void PortGremlinEvolveInit(void)
{
    g_bEvolveActive = false;
    g_ui32EvolveGeneration = 0;
    g_ui32EnumsSinceMutate = 0;

    g_sGenome.ui8Interval = PORTGREMLIN_CYCLE_INTERVAL_DEF;
    g_sGenome.ui8Malformed = 0;
    g_sGenome.ui8RealVid = 1;
    g_sGenome.ui8Contradiction = 0;
    g_sGenome.ui32Fitness = 0;
    g_sBestGenome = g_sGenome;
}

void PortGremlinEvolveToggle(void)
{
    g_bEvolveActive = !g_bEvolveActive;
    if (g_bEvolveActive)
    {
        g_sOracle.bBrainActive = false;
        PortGremlinEvolveApply();
        UARTprintf("[EVOLVE] Genetic attack engine ACTIVE (gen %u)\n\r",
                   g_ui32EvolveGeneration);
    }
    else
    {
        UARTprintf("[EVOLVE] Genetic attack engine off\n\r");
    }
}

void PortGremlinEvolveApply(void)
{
    GenomeToConfig(&g_sGenome);
    PortGremlinTelemetryEvolve(g_ui32EvolveGeneration, g_sGenome.ui32Fitness);
    UARTprintf("[EVOLVE] interval=%u mal=%u vid=%u contra=%u fit=%u\n\r",
               g_sGenome.ui8Interval, g_sGenome.ui8Malformed,
               g_sGenome.ui8RealVid, g_sGenome.ui8Contradiction,
               g_sGenome.ui32Fitness);
}

void PortGremlinEvolveReward(void)
{
    if (!g_bEvolveActive)
    {
        return;
    }

    g_sGenome.ui32Fitness += 2;
    if (g_sGenome.ui32Fitness > g_sBestGenome.ui32Fitness)
    {
        g_sBestGenome = g_sGenome;
    }
}

void PortGremlinEvolvePunish(void)
{
    if (!g_bEvolveActive)
    {
        return;
    }

    g_ui32EvolveGeneration++;
    g_sGenome = g_sBestGenome;
    MutateGenome(&g_sGenome);
    g_ui32EnumsSinceMutate = 0;
    PortGremlinEvolveApply();
    UARTprintf("[EVOLVE] Host rejected genome - mutated to gen %u\n\r",
               g_ui32EvolveGeneration);
}

void PortGremlinEvolveTick(void)
{
    if (!g_bEvolveActive)
    {
        return;
    }

    g_ui32EnumsSinceMutate++;
    if (g_ui32EnumsSinceMutate >= 25U)
    {
        g_ui32EvolveGeneration++;
        MutateGenome(&g_sGenome);
        g_ui32EnumsSinceMutate = 0;
        PortGremlinEvolveApply();
    }
}
