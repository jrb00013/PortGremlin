#ifndef PORTGREMLIN_EVOLVE_H
#define PORTGREMLIN_EVOLVE_H

#include <stdint.h>
#include <stdbool.h>

typedef struct
{
    uint8_t ui8Interval;
    uint8_t ui8Malformed;
    uint8_t ui8RealVid;
    uint8_t ui8Contradiction;
    uint32_t ui32Fitness;
} AttackGenome;

extern bool g_bEvolveActive;
extern AttackGenome g_sGenome;
extern uint32_t g_ui32EvolveGeneration;

void PortGremlinEvolveInit(void);
void PortGremlinEvolveToggle(void);
void PortGremlinEvolveApply(void);
void PortGremlinEvolveReward(void);
void PortGremlinEvolvePunish(void);
void PortGremlinEvolveTick(void);

#endif
