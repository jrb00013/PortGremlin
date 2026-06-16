#include <stdlib.h>
#include <stddef.h>
#include "portgremlin_config.h"
#include "portgremlin_oracle.h"
#include "portgremlin_vidpid.h"
#include "usblib/device/usbdhidkeyb.h"

static const uint16_t g_pui16KnownVIDs[] =
{
    0x046D, /* Logitech */
    0x045E, /* Microsoft */
    0x05AC, /* Apple */
    0x8087, /* Intel */
    0x1D6B, /* Linux Foundation */
    0x04F2, /* Chicony */
    0x0BDA, /* Realtek */
    0x413C, /* Dell */
    0x0951, /* Kingston */
    0x0781, /* SanDisk */
    0x1058, /* Western Digital */
    0x0E8D, /* MediaTek */
    0x0489, /* Foxconn */
    0x04CA, /* Lite-On */
    0x1BCF, /* Sunplus */
    0x5986, /* Acer */
    0x17EF, /* Lenovo */
    0x03F0, /* HP */
    0x054C, /* Sony */
    0x0B05, /* ASUS */
    0x0408, /* Quanta */
    0x1A40, /* Terminus */
    0x2109, /* VIA Labs */
    0x05E3, /* Genesys Logic */
};

static const uint16_t g_pui16MalformedVIDs[] = { 0x0000, 0xFFFF };
static const uint16_t g_pui16MalformedPIDs[] = { 0x0000, 0xFFFF };

static uint16_t PickKnownVID(void)
{
    return g_pui16KnownVIDs[rand() % (sizeof(g_pui16KnownVIDs) / sizeof(uint16_t))];
}

static uint16_t PickRandomVID(void)
{
    return (uint16_t)(0x1000 + (rand() % 0xEFFF));
}

static uint16_t PickRandomPID(void)
{
    return (uint16_t)(0x0001 + (rand() % 0xFFFE));
}

static void SetVIDPID(void *pDevice, VIDPIDDeviceType eType, uint16_t ui16VID, uint16_t ui16PID)
{
    switch (eType)
    {
        case VIDPID_TYPE_KEYBOARD:
        {
            tUSBDHIDKeyboardDevice *pKb = (tUSBDHIDKeyboardDevice *)pDevice;
            pKb->ui16VID = ui16VID;
            pKb->ui16PID = ui16PID;
            break;
        }
        case VIDPID_TYPE_AUDIO:
        {
            tUSBAudioDevice *pAudio = (tUSBAudioDevice *)pDevice;
            pAudio->ui16VID = ui16VID;
            pAudio->ui16PID = ui16PID;
            break;
        }
        case VIDPID_TYPE_MIDI:
        {
            tUSBMIDIDevice *pMIDI = (tUSBMIDIDevice *)pDevice;
            pMIDI->ui16VID = ui16VID;
            pMIDI->ui16PID = ui16PID;
            break;
        }
        case VIDPID_TYPE_PRINTER:
        {
            tUSBPrinterDevice *pPrinter = (tUSBPrinterDevice *)pDevice;
            pPrinter->ui16VID = ui16VID;
            pPrinter->ui16PID = ui16PID;
            break;
        }
        case VIDPID_TYPE_GAMEPAD:
        {
            tUSBDHIDGamepadDevice *pPad = (tUSBDHIDGamepadDevice *)pDevice;
            pPad->ui16VID = ui16VID;
            pPad->ui16PID = ui16PID;
            break;
        }
        default:
            break;
    }
}

static void SetPowerFields(void *pDevice, VIDPIDDeviceType eType,
                           uint16_t ui16MaxPowermA, uint8_t ui8PwrAttributes)
{
    switch (eType)
    {
        case VIDPID_TYPE_KEYBOARD:
        {
            tUSBDHIDKeyboardDevice *pKb = (tUSBDHIDKeyboardDevice *)pDevice;
            pKb->ui16MaxPowermA = ui16MaxPowermA;
            pKb->ui8PwrAttributes = ui8PwrAttributes;
            break;
        }
        case VIDPID_TYPE_AUDIO:
        {
            tUSBAudioDevice *pAudio = (tUSBAudioDevice *)pDevice;
            pAudio->ui16MaxPowermA = ui16MaxPowermA;
            pAudio->ui8PwrAttributes = ui8PwrAttributes;
            break;
        }
        case VIDPID_TYPE_MIDI:
        {
            tUSBMIDIDevice *pMIDI = (tUSBMIDIDevice *)pDevice;
            pMIDI->ui16MaxPowermA = ui16MaxPowermA;
            pMIDI->ui8PwrAttributes = ui8PwrAttributes;
            break;
        }
        case VIDPID_TYPE_PRINTER:
        {
            tUSBPrinterDevice *pPrinter = (tUSBPrinterDevice *)pDevice;
            pPrinter->ui16MaxPowermA = ui16MaxPowermA;
            pPrinter->ui8PwrAttributes = ui8PwrAttributes;
            break;
        }
        case VIDPID_TYPE_GAMEPAD:
        {
            tUSBDHIDGamepadDevice *pPad = (tUSBDHIDGamepadDevice *)pDevice;
            pPad->ui16MaxPowermA = ui16MaxPowermA;
            pPad->ui8PwrAttributes = ui8PwrAttributes;
            break;
        }
        default:
            break;
    }
}

void PortGremlinRandomizeVIDPID(void *pDevice, VIDPIDDeviceType eType)
{
    uint16_t ui16VID;
    uint16_t ui16PID;

    if (g_sOracle.bContradictionMode || g_sOracle.bIdentityLocked)
    {
        ui16VID = g_sOracle.ui16PinnedVID;
        ui16PID = g_sOracle.ui16PinnedPID;
    }
    else if (g_sConfig.bMalformedMode)
    {
        ui16VID = g_pui16MalformedVIDs[rand() % 2];
        ui16PID = g_pui16MalformedPIDs[rand() % 2];
    }
    else if (g_sConfig.bRealVIDPID)
    {
        ui16VID = PickKnownVID();
        ui16PID = PickRandomPID();
    }
    else
    {
        ui16VID = PickRandomVID();
        ui16PID = PickRandomPID();
    }

    SetVIDPID(pDevice, eType, ui16VID, ui16PID);
    PortGremlinApplyPowerAttributes(pDevice, eType);
}

void PortGremlinSetPinnedVIDPID(uint16_t ui16VID, uint16_t ui16PID)
{
    g_sOracle.ui16PinnedVID = ui16VID;
    g_sOracle.ui16PinnedPID = ui16PID;
    g_sOracle.bIdentityLocked = true;
}

void PortGremlinApplyPowerAttributes(void *pDevice, VIDPIDDeviceType eType)
{
    if (g_sConfig.bMalformedMode)
    {
        static const uint16_t g_pui16BadPower[] = { 0, 5000, 9999 };
        static const uint8_t g_pui8BadAttr[] = { 0x00, 0xFF, 0x80 };

        SetPowerFields(pDevice, eType,
                       g_pui16BadPower[rand() % 3],
                       g_pui8BadAttr[rand() % 3]);
    }
}
