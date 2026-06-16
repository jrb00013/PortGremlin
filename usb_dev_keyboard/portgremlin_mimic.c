#include <string.h>
#include "portgremlin_mimic.h"
#include "portgremlin_strings.h"
#include "portgremlin_vidpid.h"
#include "portgremlin_config.h"
#include "utils/uartstdio.h"

static const MimicProfile g_psMimicVault[] =
{
    { 0x046D, 0xC52B, DEVICE_KEYBOARD, "Logitech", "USB Receiver" },
    { 0x046D, 0xC077, DEVICE_KEYBOARD, "Logitech", "USB Optical Mouse" },
    { 0x05AC, 0x0250, DEVICE_KEYBOARD, "Apple Inc.", "Apple Keyboard" },
    { 0x05AC, 0x8300, DEVICE_KEYBOARD, "Apple Inc.", "Internal Keyboard" },
    { 0x045E, 0x07B2, DEVICE_KEYBOARD, "Microsoft", "Wired Keyboard 600" },
    { 0x045E, 0x028E, DEVICE_GAMEPAD, "Microsoft", "Xbox360 Controller" },
    { 0x0781, 0x5567, DEVICE_PRINTER, "SanDisk", "Cruzer Blade" },
    { 0x0951, 0x1666, DEVICE_PRINTER, "Kingston", "DataTraveler 3.0" },
    { 0x0BDA, 0x8152, DEVICE_AUDIO, "Realtek", "USB Audio" },
    { 0x1235, 0x0002, DEVICE_MIDI, "Focusrite", "Scarlett 2i2" },
    { 0x04F9, 0x2042, DEVICE_PRINTER, "Brother", "HL-L2350DW" },
    { 0x03F0, 0x094A, DEVICE_PRINTER, "HP", "LaserJet Pro" },
};

uint32_t PortGremlinMimicCount(void)
{
    return sizeof(g_psMimicVault) / sizeof(MimicProfile);
}

const MimicProfile *PortGremlinMimicGet(uint32_t ui32Index)
{
    if (ui32Index >= PortGremlinMimicCount())
    {
        return NULL;
    }
    return &g_psMimicVault[ui32Index];
}

bool PortGremlinMimicApply(uint32_t ui32Index, DeviceType *peTargetClass)
{
    const MimicProfile *psProfile = PortGremlinMimicGet(ui32Index);
    if (!psProfile)
    {
        return false;
    }

    PortGremlinSetIdentityStrings(psProfile->pcManufacturer, psProfile->pcProduct,
                                  psProfile->eClass);
    PortGremlinSetPinnedVIDPID(psProfile->ui16VID, psProfile->ui16PID);

    if (peTargetClass)
    {
        *peTargetClass = psProfile->eClass;
    }

    UARTprintf("[MIMIC] #%u %s %s (0x%04X:0x%04X) -> %s\n\r",
               ui32Index, psProfile->pcManufacturer, psProfile->pcProduct,
               psProfile->ui16VID, psProfile->ui16PID,
               PortGremlinDeviceName(psProfile->eClass));
    return true;
}

void PortGremlinMimicPrintVault(void)
{
    UARTprintf("\n\r=== MIMIC VAULT ===\n\r");
    for (uint32_t i = 0; i < PortGremlinMimicCount(); i++)
    {
        const MimicProfile *ps = &g_psMimicVault[i];
        UARTprintf("  %u: %s %s [0x%04X:0x%04X] %s\n\r",
                   i, ps->pcManufacturer, ps->pcProduct,
                   ps->ui16VID, ps->ui16PID,
                   PortGremlinDeviceName(ps->eClass));
    }
    UARTprintf("===================\n\r");
}
