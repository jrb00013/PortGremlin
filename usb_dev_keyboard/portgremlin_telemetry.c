#include <stdbool.h>
#include "portgremlin_telemetry.h"
#include "portgremlin_config.h"
#include "usb_keyb_structs.h"
#include "usblib/device/usbdhidkeyb.h"
#include "utils/uartstdio.h"

bool g_bTelemetryEnabled = true;

void PortGremlinTelemetryInit(void)
{
    g_bTelemetryEnabled = true;
}

void PortGremlinTelemetryToggle(void)
{
    g_bTelemetryEnabled = !g_bTelemetryEnabled;
    UARTprintf("Telemetry: %s\n\r", g_bTelemetryEnabled ? "ON" : "OFF");
}

void PortGremlinTelemetryHost(HostProfile eHost, uint32_t ui32Latency, uint32_t ui32Resets)
{
    if (!g_bTelemetryEnabled)
    {
        return;
    }

    UARTprintf("@PG{\"e\":\"host\",\"os\":\"%s\",\"lat\":%u,\"rst\":%u}\n\r",
               PortGremlinHostName(eHost), ui32Latency, ui32Resets);
}

void PortGremlinTelemetryEnum(uint16_t ui16VID, uint16_t ui16PID, const char *pcClass)
{
    if (!g_bTelemetryEnabled)
    {
        return;
    }

    UARTprintf("@PG{\"e\":\"enum\",\"vid\":\"%04X\",\"pid\":\"%04X\",\"cls\":\"%s\","
               "\"n\":%u}\n\r",
               ui16VID, ui16PID, pcClass, g_sConfig.ui32EnumCount);
}

void PortGremlinTelemetryPersona(GremlinPersona ePersona)
{
    if (!g_bTelemetryEnabled)
    {
        return;
    }

    UARTprintf("@PG{\"e\":\"persona\",\"name\":\"%s\"}\n\r",
               PortGremlinPersonaName(ePersona));
}

void PortGremlinTelemetryBrain(BrainPhase ePhase, uint32_t ui32Tolerance)
{
    if (!g_bTelemetryEnabled)
    {
        return;
    }

    UARTprintf("@PG{\"e\":\"brain\",\"phase\":\"%s\",\"tol\":%u}\n\r",
               PortGremlinBrainPhaseName(ePhase), ui32Tolerance);
}

void PortGremlinTelemetryDisconnect(uint32_t ui32Total)
{
    if (!g_bTelemetryEnabled)
    {
        return;
    }

    UARTprintf("@PG{\"e\":\"disconnect\",\"total\":%u}\n\r", ui32Total);
}

void PortGremlinTelemetryEvolve(uint32_t ui32Gen, uint32_t ui32Fitness)
{
    if (!g_bTelemetryEnabled)
    {
        return;
    }

    UARTprintf("@PG{\"e\":\"evolve\",\"gen\":%u,\"fit\":%u}\n\r", ui32Gen, ui32Fitness);
}

void PortGremlinTelemetryCurrentIdentity(void)
{
    uint16_t ui16VID = 0;
    uint16_t ui16PID = 0;

    switch (g_eCurrentDevice)
    {
        case DEVICE_KEYBOARD:
            ui16VID = g_sKeyboardDevice.ui16VID;
            ui16PID = g_sKeyboardDevice.ui16PID;
            break;
        case DEVICE_AUDIO:
            ui16VID = g_sAudioDevice.ui16VID;
            ui16PID = g_sAudioDevice.ui16PID;
            break;
        case DEVICE_GAMEPAD:
            ui16VID = g_sGamepadDevice.ui16VID;
            ui16PID = g_sGamepadDevice.ui16PID;
            break;
        case DEVICE_MIDI:
            ui16VID = g_sMIDIDevice.ui16VID;
            ui16PID = g_sMIDIDevice.ui16PID;
            break;
        case DEVICE_PRINTER:
            ui16VID = g_sPrinterDevice.ui16VID;
            ui16PID = g_sPrinterDevice.ui16PID;
            break;
        default:
            break;
    }

    PortGremlinTelemetryEnum(ui16VID, ui16PID, PortGremlinDeviceName(g_eCurrentDevice));
}
