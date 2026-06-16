#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_memmap.h"
#include "driverlib/rom_map.h"
#include "driverlib/uart.h"
#include "utils/uartstdio.h"
#include "portgremlin_config.h"
#include "portgremlin_uart.h"
#include "portgremlin_oracle.h"
#include "portgremlin_persona.h"
#include "portgremlin_mimic.h"
#include "portgremlin_telemetry.h"
#include "portgremlin_evolve.h"
#include "usb_keyb_structs.h"

static void PrintOnOff(bool bValue)
{
    UARTprintf(bValue ? "ON\n\r" : "OFF\n\r");
}

void PortGremlinUARTPrintHelp(void)
{
    UARTprintf("\n\r=== PortGremlin Command Interface ===\n\r");
    UARTprintf("--- Core ---\n\r");
    UARTprintf("  h  - help          s  - status\n\r");
    UARTprintf("  a  - auto cycle    m  - malformed mode\n\r");
    UARTprintf("  r  - real VID DB    t  - random strings\n\r");
    UARTprintf("  1-5- toggle class  +/- - interval\n\r");
    UARTprintf("  c  - force cycle    e  - re-enumerate\n\r");
    UARTprintf("--- ORACLE (novel) ---\n\r");
    UARTprintf("  b  - Gremlin Brain (autonomous escalation)\n\r");
    UARTprintf("  p  - next attack persona\n\r");
    UARTprintf("  o  - oracle host fingerprint report\n\r");
    UARTprintf("  d  - driver confusion (same VID, diff class)\n\r");
    UARTprintf("  v  - print mimic vault\n\r");
    UARTprintf("  0-9- deploy mimic profile N\n\r");
    UARTprintf("  [  - choreo RedTeam  ]  - Stealth  \\  - Blitz\n\r");
    UARTprintf("--- NEXUS ---\n\r");
    UARTprintf("  g  - genetic evolution engine\n\r");
    UARTprintf("  x  - full nexus mode (brain+telemetry+evolve)\n\r");
    UARTprintf("  l  - toggle JSON telemetry stream\n\r");
    UARTprintf("=====================================\n\r");
}

void PortGremlinUARTPrintStatus(void)
{
    UARTprintf("\n\r--- PortGremlin Status ---\n\r");
    UARTprintf("Device:      %s\n\r", PortGremlinDeviceName(g_eCurrentDevice));
    UARTprintf("Auto cycle:  "); PrintOnOff(g_sConfig.bAutoCycle);
    UARTprintf("Malformed:   "); PrintOnOff(g_sConfig.bMalformedMode);
    UARTprintf("Real VID:    "); PrintOnOff(g_sConfig.bRealVIDPID);
    UARTprintf("Rand strings:"); PrintOnOff(g_sConfig.bRandomStrings);
    UARTprintf("Interval:    %u ticks (%u ms)\n\r",
               g_sConfig.ui32CycleIntervalTicks,
               g_sConfig.ui32CycleIntervalTicks * 10U);
    UARTprintf("Enums:       %u  Cycles: %u\n\r",
               g_sConfig.ui32EnumCount, g_sConfig.ui32CycleCount);
    UARTprintf("Persona:     %s\n\r", PortGremlinPersonaName(g_ePersona));
    UARTprintf("Classes:     ");
    for (int i = 0; i < (int)NUM_DEVICE_TYPES; i++)
    {
        UARTprintf("%s=%s ", PortGremlinDeviceName((DeviceType)i),
                   g_sConfig.bClassEnabled[i] ? "Y" : "N");
    }
    UARTprintf("\n\r--------------------------\n\r");
}

static void ToggleClass(DeviceType eDevice)
{
    g_sConfig.bClassEnabled[eDevice] = !g_sConfig.bClassEnabled[eDevice];
    UARTprintf("%s: ", PortGremlinDeviceName(eDevice));
    PrintOnOff(g_sConfig.bClassEnabled[eDevice]);
}

void PortGremlinUARTPoll(void)
{
    int32_t i32Char;

    while ((i32Char = UARTCharGetNonBlocking(UART0_BASE)) >= 0)
    {
        switch (i32Char)
        {
            case 'h':
            case 'H':
                PortGremlinUARTPrintHelp();
                break;

            case 's':
            case 'S':
                PortGremlinUARTPrintStatus();
                break;

            case 'a':
            case 'A':
                g_sConfig.bAutoCycle = !g_sConfig.bAutoCycle;
                UARTprintf("Auto cycle: ");
                PrintOnOff(g_sConfig.bAutoCycle);
                break;

            case 'm':
            case 'M':
                g_sConfig.bMalformedMode = !g_sConfig.bMalformedMode;
                UARTprintf("Malformed mode: ");
                PrintOnOff(g_sConfig.bMalformedMode);
                break;

            case 'r':
            case 'R':
                g_sConfig.bRealVIDPID = !g_sConfig.bRealVIDPID;
                UARTprintf("Real VID database: ");
                PrintOnOff(g_sConfig.bRealVIDPID);
                break;

            case 't':
            case 'T':
                g_sConfig.bRandomStrings = !g_sConfig.bRandomStrings;
                UARTprintf("Random strings: ");
                PrintOnOff(g_sConfig.bRandomStrings);
                break;

            case '1':
                ToggleClass(DEVICE_KEYBOARD);
                break;
            case '2':
                ToggleClass(DEVICE_AUDIO);
                break;
            case '3':
                ToggleClass(DEVICE_PRINTER);
                break;
            case '4':
                ToggleClass(DEVICE_MIDI);
                break;
            case '5':
                ToggleClass(DEVICE_GAMEPAD);
                break;

            case '+':
            case '=':
                if (g_sConfig.ui32CycleIntervalTicks > PORTGREMLIN_CYCLE_INTERVAL_MIN)
                {
                    g_sConfig.ui32CycleIntervalTicks--;
                    UARTprintf("Interval: %u ticks (%u ms)\n\r",
                               g_sConfig.ui32CycleIntervalTicks,
                               g_sConfig.ui32CycleIntervalTicks * 10U);
                }
                break;

            case '-':
            case '_':
                if (g_sConfig.ui32CycleIntervalTicks < PORTGREMLIN_CYCLE_INTERVAL_MAX)
                {
                    g_sConfig.ui32CycleIntervalTicks++;
                    UARTprintf("Interval: %u ticks (%u ms)\n\r",
                               g_sConfig.ui32CycleIntervalTicks,
                               g_sConfig.ui32CycleIntervalTicks * 10U);
                }
                break;

            case 'c':
            case 'C':
                g_sConfig.bForceCycle = true;
                UARTprintf("Force cycle queued\n\r");
                break;

            case 'e':
            case 'E':
                g_sConfig.bForceReenum = true;
                UARTprintf("Force re-enumerate queued\n\r");
                break;

            case 'b':
            case 'B':
                g_sOracle.bBrainActive = !g_sOracle.bBrainActive;
                if (g_sOracle.bBrainActive)
                {
                    g_sOracle.eBrainPhase = BRAIN_IDLE;
                    PortGremlinChoreoStop();
                    UARTprintf("Gremlin Brain: ACTIVE\n\r");
                }
                else
                {
                    g_sOracle.eBrainPhase = BRAIN_IDLE;
                    UARTprintf("Gremlin Brain: off\n\r");
                }
                break;

            case 'p':
            case 'P':
                PortGremlinPersonaNext();
                break;

            case 'o':
            case 'O':
                PortGremlinOraclePrintReport();
                break;

            case 'd':
            case 'D':
                g_sOracle.bContradictionMode = !g_sOracle.bContradictionMode;
                if (g_sOracle.bContradictionMode)
                {
                    g_sOracle.ui16PinnedVID = (uint16_t)(0x1000 + (rand() % 0xEFFF));
                    g_sOracle.ui16PinnedPID = (uint16_t)(0x1000 + (rand() % 0xEFFF));
                    g_sOracle.bIdentityLocked = true;
                    UARTprintf("Driver confusion ON VID=0x%04X PID=0x%04X\n\r",
                               g_sOracle.ui16PinnedVID, g_sOracle.ui16PinnedPID);
                }
                else
                {
                    g_sOracle.bIdentityLocked = false;
                    UARTprintf("Driver confusion OFF\n\r");
                }
                break;

            case 'v':
            case 'V':
                PortGremlinMimicPrintVault();
                break;

            case '[':
                PortGremlinChoreoStart(0);
                break;
            case ']':
                PortGremlinChoreoStart(1);
                break;
            case '\\':
                PortGremlinChoreoStart(2);
                break;

            case 'g':
            case 'G':
                PortGremlinEvolveToggle();
                break;

            case 'x':
            case 'X':
                g_bTelemetryEnabled = true;
                g_sOracle.bBrainActive = true;
                g_sOracle.eBrainPhase = BRAIN_IDLE;
                if (!g_bEvolveActive)
                {
                    PortGremlinEvolveToggle();
                }
                PortGremlinChoreoStart(0);
                UARTprintf("[NEXUS] Full autonomous nexus mode engaged\n\r");
                break;

            case 'l':
            case 'L':
                PortGremlinTelemetryToggle();
                break;

            default:
                if (i32Char >= '0' && i32Char <= '9')
                {
                    PortGremlinMimicApply((uint32_t)(i32Char - '0'), NULL);
                }
                break;
        }
    }
}
