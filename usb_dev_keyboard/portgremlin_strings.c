#include <stdlib.h>
#include <string.h>
#include "portgremlin_config.h"
#include "portgremlin_strings.h"
#include "portgremlin_persona.h"
#include "portgremlin_mimic.h"
#include "usb_keyb_structs.h"
#include "usblib/usblib.h"
#include "usblib/usb-ids.h"
#include "usblib/device/usbdhidkeyb.h"

#define STR_BUF_CHARS   PORTGREMLIN_STRING_MAX_CHARS
#define STR_BUF_BYTES   ((STR_BUF_CHARS + 1) * 2)

static uint8_t g_pui8ManufacturerString[STR_BUF_BYTES];
static uint8_t g_pui8ProductStringKeyboard[STR_BUF_BYTES];
static uint8_t g_pui8ProductStringAudio[STR_BUF_BYTES];
static uint8_t g_pui8ProductStringGamepad[STR_BUF_BYTES];
static uint8_t g_pui8ProductStringPrinter[STR_BUF_BYTES];
static uint8_t g_pui8ProductStringMIDI[STR_BUF_BYTES];

static const uint8_t g_pui8LangDescriptor[] =
{
    4,
    USB_DTYPE_STRING,
    USBShort(USB_LANG_EN_US)
};

static const uint8_t g_pui8ManufacturerDefault[] =
{
    (17 + 1) * 2,
    USB_DTYPE_STRING,
    'P', 0, 'o', 0, 'r', 0, 't', 0, 'G', 0, 'r', 0, 'e', 0, 'm', 0,
    'l', 0, 'i', 0, 'n', 0, ' ', 0, 'C', 0, 'o', 0, 'r', 0, 'p', 0
};

static const uint8_t g_pui8ProductDefaultKeyboard[] =
{
    (16 + 1) * 2,
    USB_DTYPE_STRING,
    'K', 0, 'e', 0, 'y', 0, 'b', 0, 'o', 0, 'a', 0, 'r', 0, 'd', 0, ' ',
    'D', 0, 'e', 0, 'v', 0, 'i', 0, 'c', 0, 'e', 0
};

static const uint8_t g_pui8ProductDefaultAudio[] =
{
    (12 + 1) * 2,
    USB_DTYPE_STRING,
    'A', 0, 'u', 0, 'd', 0, 'i', 0, 'o', 0, ' ', 0, 'D', 0, 'e', 0, 'v', 0,
    'i', 0, 'c', 0, 'e', 0
};

static const uint8_t g_pui8ProductDefaultGamepad[] =
{
    (12 + 1) * 2,
    USB_DTYPE_STRING,
    'G', 0, 'a', 0, 'm', 0, 'e', 0, 'p', 0, 'a', 0, 'd', 0, ' ', 0, 'D', 0,
    'e', 0, 'v', 0, 'i', 0, 'c', 0, 'e', 0
};

static const uint8_t g_pui8ProductDefaultPrinter[] =
{
    (13 + 1) * 2,
    USB_DTYPE_STRING,
    'P', 0, 'r', 0, 'i', 0, 'n', 0, 't', 0, 'e', 0, 'r', 0, ' ', 0, 'D', 0,
    'e', 0, 'v', 0, 'i', 0, 'c', 0, 'e', 0
};

static const uint8_t g_pui8ProductDefaultMIDI[] =
{
    (14 + 1) * 2,
    USB_DTYPE_STRING,
    'M', 0, 'I', 0, 'D', 0, 'I', 0, ' ', 0, 'C', 0, 'o', 0, 'n', 0, 't', 0,
    'r', 0, 'o', 0, 'l', 0, 'l', 0, 'e', 0, 'r', 0
};

static const uint8_t g_pui8HIDInterfaceString[] =
{
    (22 + 1) * 2,
    USB_DTYPE_STRING,
    'H', 0, 'I', 0, 'D', 0, ' ', 0, 'K', 0, 'e', 0, 'y', 0, 'b', 0,
    'o', 0, 'a', 0, 'r', 0, 'd', 0, ' ', 0, 'I', 0, 'n', 0, 't', 0,
    'e', 0, 'r', 0, 'f', 0, 'a', 0, 'c', 0, 'e', 0
};

static const uint8_t g_pui8ConfigString[] =
{
    (26 + 1) * 2,
    USB_DTYPE_STRING,
    'H', 0, 'I', 0, 'D', 0, ' ', 0, 'K', 0, 'e', 0, 'y', 0, 'b', 0,
    'o', 0, 'a', 0, 'r', 0, 'd', 0, ' ', 0, 'C', 0, 'o', 0, 'n', 0,
    'f', 0, 'i', 0, 'g', 0, 'u', 0, 'r', 0, 'a', 0, 't', 0, 'i', 0,
    'o', 0, 'n', 0
};

static uint8_t g_pui8SerialNumberString[] =
{
    (8 + 1) * 2,
    USB_DTYPE_STRING,
    '0', 0, '0', 0, '0', 0, '0', 0, '0', 0, '0', 0, '0', 0, '0', 0
};

const uint8_t * const g_ppui8StringDescriptorsKeyboard[] =
{
    g_pui8LangDescriptor,
    g_pui8ManufacturerString,
    g_pui8ProductStringKeyboard,
    g_pui8SerialNumberString,
    g_pui8HIDInterfaceString,
    g_pui8ConfigString
};

const uint8_t * const g_ppui8StringDescriptorsAudio[] =
{
    g_pui8LangDescriptor,
    g_pui8ManufacturerString,
    g_pui8ProductStringAudio,
    g_pui8SerialNumberString
};

const uint8_t * const g_ppui8StringDescriptorsGamepad[] =
{
    g_pui8LangDescriptor,
    g_pui8ManufacturerString,
    g_pui8ProductStringGamepad,
    g_pui8SerialNumberString
};

const uint8_t * const g_ppui8StringDescriptorsPrinter[] =
{
    g_pui8LangDescriptor,
    g_pui8ManufacturerString,
    g_pui8ProductStringPrinter,
    g_pui8SerialNumberString
};

const uint8_t * const g_ppui8StringDescriptorsMIDI[] =
{
    g_pui8LangDescriptor,
    g_pui8ManufacturerString,
    g_pui8ProductStringMIDI,
    g_pui8SerialNumberString
};

#define NUM_STR_DESC_KB      (sizeof(g_ppui8StringDescriptorsKeyboard) / sizeof(uint8_t *))
#define NUM_STR_DESC_AUDIO   (sizeof(g_ppui8StringDescriptorsAudio) / sizeof(uint8_t *))
#define NUM_STR_DESC_GAMEPAD (sizeof(g_ppui8StringDescriptorsGamepad) / sizeof(uint8_t *))
#define NUM_STR_DESC_PRINTER (sizeof(g_ppui8StringDescriptorsPrinter) / sizeof(uint8_t *))
#define NUM_STR_DESC_MIDI    (sizeof(g_ppui8StringDescriptorsMIDI) / sizeof(uint8_t *))

static void CopyDefaultString(uint8_t *pui8Dst, const uint8_t *pui8Src, uint32_t ui32Size)
{
    memcpy(pui8Dst, pui8Src, ui32Size);
}

static char RandomChar(void)
{
    static const char g_pcCharset[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    return g_pcCharset[rand() % (sizeof(g_pcCharset) - 1)];
}

static void BuildRandomString(uint8_t *pui8Buf, uint32_t ui32MaxChars)
{
    uint32_t ui32Len = 4U + (uint32_t)(rand() % (ui32MaxChars - 3U));

    pui8Buf[0] = (uint8_t)((ui32Len + 1U) * 2U);
    pui8Buf[1] = USB_DTYPE_STRING;

    for (uint32_t i = 0; i < ui32Len; i++)
    {
        char cChar = RandomChar();
        pui8Buf[2U + i * 2U] = (uint8_t)cChar;
        pui8Buf[3U + i * 2U] = 0;
    }
}

static void CorruptString(uint8_t *pui8Buf)
{
    switch (rand() % 4)
    {
        case 0:
            pui8Buf[0] = 0;
            break;
        case 1:
            pui8Buf[0] = 0xFF;
            break;
        case 2:
            pui8Buf[1] = 0xFF;
            break;
        default:
            pui8Buf[1] = 0x00;
            break;
    }
}

void SetSerialNumberString(uint32_t ui32Value)
{
    for (int i = 0; i < 8; i++)
    {
        uint8_t ui8Nibble = (uint8_t)((ui32Value >> (28 - i * 4)) & 0xF);
        uint8_t ui8Char = (ui8Nibble < 10) ? ('0' + ui8Nibble) : ('A' + ui8Nibble - 10);
        g_pui8SerialNumberString[2 + i * 2] = ui8Char;
    }

    if (g_sConfig.bMalformedMode && (rand() % 2))
    {
        CorruptString(g_pui8SerialNumberString);
    }
}

static uint8_t *ProductBufferForDevice(DeviceType eDevice)
{
    switch (eDevice)
    {
        case DEVICE_KEYBOARD: return g_pui8ProductStringKeyboard;
        case DEVICE_AUDIO:    return g_pui8ProductStringAudio;
        case DEVICE_GAMEPAD:  return g_pui8ProductStringGamepad;
        case DEVICE_PRINTER:  return g_pui8ProductStringPrinter;
        case DEVICE_MIDI:     return g_pui8ProductStringMIDI;
        default:              return g_pui8ProductStringKeyboard;
    }
}

static const uint8_t *DefaultProductForDevice(DeviceType eDevice)
{
    switch (eDevice)
    {
        case DEVICE_KEYBOARD: return g_pui8ProductDefaultKeyboard;
        case DEVICE_AUDIO:    return g_pui8ProductDefaultAudio;
        case DEVICE_GAMEPAD:  return g_pui8ProductDefaultGamepad;
        case DEVICE_PRINTER:  return g_pui8ProductDefaultPrinter;
        case DEVICE_MIDI:     return g_pui8ProductDefaultMIDI;
        default:              return g_pui8ProductDefaultKeyboard;
    }
}

static uint32_t DefaultProductSizeForDevice(DeviceType eDevice)
{
    switch (eDevice)
    {
        case DEVICE_KEYBOARD: return sizeof(g_pui8ProductDefaultKeyboard);
        case DEVICE_AUDIO:    return sizeof(g_pui8ProductDefaultAudio);
        case DEVICE_GAMEPAD:  return sizeof(g_pui8ProductDefaultGamepad);
        case DEVICE_PRINTER:  return sizeof(g_pui8ProductDefaultPrinter);
        case DEVICE_MIDI:     return sizeof(g_pui8ProductDefaultMIDI);
        default:              return sizeof(g_pui8ProductDefaultKeyboard);
    }
}

void PortGremlinStringsInit(void)
{
    CopyDefaultString(g_pui8ManufacturerString, g_pui8ManufacturerDefault,
                      sizeof(g_pui8ManufacturerDefault));
    CopyDefaultString(g_pui8ProductStringKeyboard, g_pui8ProductDefaultKeyboard,
                      sizeof(g_pui8ProductDefaultKeyboard));
    CopyDefaultString(g_pui8ProductStringAudio, g_pui8ProductDefaultAudio,
                      sizeof(g_pui8ProductDefaultAudio));
    CopyDefaultString(g_pui8ProductStringGamepad, g_pui8ProductDefaultGamepad,
                      sizeof(g_pui8ProductDefaultGamepad));
    CopyDefaultString(g_pui8ProductStringPrinter, g_pui8ProductDefaultPrinter,
                      sizeof(g_pui8ProductDefaultPrinter));
    CopyDefaultString(g_pui8ProductStringMIDI, g_pui8ProductDefaultMIDI,
                      sizeof(g_pui8ProductDefaultMIDI));
}

static void BuildAsciiString(uint8_t *pui8Buf, const char *pcAscii)
{
    uint32_t ui32Len = 0;

    while (pcAscii[ui32Len] && ui32Len < STR_BUF_CHARS)
    {
        ui32Len++;
    }

    pui8Buf[0] = (uint8_t)((ui32Len + 1U) * 2U);
    pui8Buf[1] = USB_DTYPE_STRING;

    for (uint32_t i = 0; i < ui32Len; i++)
    {
        pui8Buf[2U + i * 2U] = (uint8_t)pcAscii[i];
        pui8Buf[3U + i * 2U] = 0;
    }
}

void PortGremlinSetIdentityStrings(const char *pcManufacturer, const char *pcProduct,
                                   DeviceType eDevice)
{
    BuildAsciiString(g_pui8ManufacturerString, pcManufacturer);
    BuildAsciiString(ProductBufferForDevice(eDevice), pcProduct);
}

void PortGremlinRandomizeIdentity(DeviceType eDevice)
{
    if (g_ePersona == PERSONA_MIMIC)
    {
        PortGremlinMimicApply(rand() % PortGremlinMimicCount(), NULL);
        SetSerialNumberString((uint32_t)(rand() ^ (rand() << 16)));
        return;
    }

    if (g_sConfig.bRandomStrings)
    {
        BuildRandomString(g_pui8ManufacturerString, STR_BUF_CHARS);
        BuildRandomString(ProductBufferForDevice(eDevice), STR_BUF_CHARS);
    }
    else
    {
        PortGremlinRestoreIdentity(eDevice);
    }

    SetSerialNumberString((uint32_t)(rand() ^ (rand() << 16)));

    if (g_sConfig.bMalformedMode)
    {
        if (rand() % 2)
        {
            CorruptString(g_pui8ManufacturerString);
        }
        if (rand() % 2)
        {
            CorruptString(ProductBufferForDevice(eDevice));
        }
    }
}

void PortGremlinRestoreIdentity(DeviceType eDevice)
{
    CopyDefaultString(g_pui8ManufacturerString, g_pui8ManufacturerDefault,
                      sizeof(g_pui8ManufacturerDefault));
    CopyDefaultString(ProductBufferForDevice(eDevice), DefaultProductForDevice(eDevice),
                      DefaultProductSizeForDevice(eDevice));
    memset(ProductBufferForDevice(eDevice) + DefaultProductSizeForDevice(eDevice), 0,
           STR_BUF_BYTES - DefaultProductSizeForDevice(eDevice));
}

tUSBDHIDKeyboardDevice g_sKeyboardDevice;
tUSBDHIDKeyboardDevice g_sKeyboardTemplate =
{
    USB_VID_TI_1CBE,
    USB_PID_KEYBOARD,
    500,
    USB_CONF_ATTR_SELF_PWR | USB_CONF_ATTR_RWAKE,
    KeyboardHandler,
    (void *)&g_sKeyboardDevice,
    g_ppui8StringDescriptorsKeyboard,
    NUM_STR_DESC_KB
};

tUSBDHIDGamepadDevice g_sGamepadDevice;
tUSBDHIDGamepadDevice g_sGamepadTemplate =
{
    USB_VID_TI_1CBE,
    0x0204,
    500,
    USB_CONF_ATTR_SELF_PWR | USB_CONF_ATTR_RWAKE,
    GamepadHandler,
    (void *)&g_sGamepadDevice,
    g_ppui8StringDescriptorsGamepad,
    NUM_STR_DESC_GAMEPAD
};

tUSBAudioDevice g_sAudioDevice;
tUSBAudioDevice g_sAudioTemplate =
{
    USB_VID_TI_1CBE,
    0x0201,
    500,
    USB_CONF_ATTR_SELF_PWR | USB_CONF_ATTR_RWAKE,
    AudioHandler,
    (void *)&g_sAudioDevice,
    g_ppui8StringDescriptorsAudio,
    NUM_STR_DESC_AUDIO
};

tUSBPrinterDevice g_sPrinterDevice;
tUSBPrinterDevice g_sPrinterTemplate =
{
    USB_VID_TI_1CBE,
    0x0202,
    500,
    USB_CONF_ATTR_SELF_PWR | USB_CONF_ATTR_RWAKE,
    PrinterHandler,
    (void *)&g_sPrinterDevice,
    g_ppui8StringDescriptorsPrinter,
    NUM_STR_DESC_PRINTER
};

tUSBMIDIDevice g_sMIDIDevice;
tUSBMIDIDevice g_sMIDITemplate =
{
    USB_VID_TI_1CBE,
    0x0203,
    500,
    USB_CONF_ATTR_SELF_PWR | USB_CONF_ATTR_RWAKE,
    MIDIHandler,
    (void *)&g_sMIDIDevice,
    g_ppui8StringDescriptorsMIDI,
    NUM_STR_DESC_MIDI
};
