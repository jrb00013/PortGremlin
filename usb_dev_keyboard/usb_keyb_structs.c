#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_types.h"
#include "driverlib/usb.h"
#include "usblib/usblib.h"
#include "usblib/usbhid.h"
#include "usblib/usb-ids.h"
#include "usblib/device/usbdevice.h"
#include "usblib/device/usbdhid.h"
#include "usblib/device/usbdhidkeyb.h"
#include "usb_keyb_structs.h"

static const uint8_t g_pui8LangDescriptor[] =
{
    4,
    USB_DTYPE_STRING,
    USBShort(USB_LANG_EN_US)
};

static const uint8_t g_pui8ManufacturerString[] =
{
    (17 + 1) * 2,
    USB_DTYPE_STRING,
    'P', 0, 'o', 0, 'r', 0, 't', 0, 'G', 0, 'r', 0, 'e', 0, 'm', 0,
    'l', 0, 'i', 0, 'n', 0, ' ', 0, 'C', 0, 'o', 0, 'r', 0, 'p', 0
};

static const uint8_t g_pui8ProductStringKeyboard[] =
{
    (16 + 1) * 2,
    USB_DTYPE_STRING,
    'K', 0, 'e', 0, 'y', 0, 'b', 0, 'o', 0, 'a', 0, 'r', 0, 'd', 0, ' ',
    'D', 0, 'e', 0, 'v', 0, 'i', 0, 'c', 0, 'e', 0
};

static const uint8_t g_pui8ProductStringAudio[] =
{
    (12 + 1) * 2,
    USB_DTYPE_STRING,
    'A', 0, 'u', 0, 'd', 0, 'i', 0, 'o', 0, ' ', 0, 'D', 0, 'e', 0, 'v', 0,
    'i', 0, 'c', 0, 'e', 0
};

static const uint8_t g_pui8ProductStringGamepad[] =
{
    (12 + 1) * 2,
    USB_DTYPE_STRING,
    'G', 0, 'a', 0, 'm', 0, 'e', 0, 'p', 0, 'a', 0, 'd', 0, ' ', 0, 'D', 0,
    'e', 0, 'v', 0, 'i', 0, 'c', 0, 'e', 0
};

static const uint8_t g_pui8ProductStringPrinter[] =
{
    (13 + 1) * 2,
    USB_DTYPE_STRING,
    'P', 0, 'r', 0, 'i', 0, 'n', 0, 't', 0, 'e', 0, 'r', 0, ' ', 0, 'D', 0,
    'e', 0, 'v', 0, 'i', 0, 'c', 0, 'e', 0
};

static const uint8_t g_pui8ProductStringMIDI[] =
{
    (14 + 1) * 2,
    USB_DTYPE_STRING,
    'M', 0, 'I', 0, 'D', 0, 'I', 0, ' ', 0, 'C', 0, 'o', 0, 'n', 0, 't', 0,
    'r', 0, 'o', 0, 'l', 0, 'l', 0, 'e', 0, 'r', 0
};

static uint8_t g_pui8SerialNumberString[] =
{
    (8 + 1) * 2,
    USB_DTYPE_STRING,
    '0', 0, '0', 0, '0', 0, '0', 0, '0', 0, '0', 0, '0', 0, '0', 0
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

const uint8_t * const g_ppui8StringDescriptorsKeyboard[] =
{
    g_pui8LangDescriptor,
    g_pui8ManufacturerString,
    g_pui8ProductStringKeyboard,
    g_pui8SerialNumberString,
    g_pui8HIDInterfaceString,
    g_pui8ConfigString
};

#define NUM_STR_DESC_KB (sizeof(g_ppui8StringDescriptorsKeyboard) / sizeof(uint8_t *))

const uint8_t * const g_ppui8StringDescriptorsAudio[] =
{
    g_pui8LangDescriptor,
    g_pui8ManufacturerString,
    g_pui8ProductStringAudio,
    g_pui8SerialNumberString
};
#define NUM_STR_DESC_AUDIO (sizeof(g_ppui8StringDescriptorsAudio) / sizeof(uint8_t *))

const uint8_t * const g_ppui8StringDescriptorsGamepad[] =
{
    g_pui8LangDescriptor,
    g_pui8ManufacturerString,
    g_pui8ProductStringGamepad,
    g_pui8SerialNumberString
};
#define NUM_STR_DESC_GAMEPAD (sizeof(g_ppui8StringDescriptorsGamepad) / sizeof(uint8_t *))

const uint8_t * const g_ppui8StringDescriptorsPrinter[] =
{
    g_pui8LangDescriptor,
    g_pui8ManufacturerString,
    g_pui8ProductStringPrinter,
    g_pui8SerialNumberString
};
#define NUM_STR_DESC_PRINTER (sizeof(g_ppui8StringDescriptorsPrinter) / sizeof(uint8_t *))

const uint8_t * const g_ppui8StringDescriptorsMIDI[] =
{
    g_pui8LangDescriptor,
    g_pui8ManufacturerString,
    g_pui8ProductStringMIDI,
    g_pui8SerialNumberString
};
#define NUM_STR_DESC_MIDI (sizeof(g_ppui8StringDescriptorsMIDI) / sizeof(uint8_t *))

extern uint32_t KeyboardHandler(void *, uint32_t, uint32_t, void *);
extern uint32_t AudioHandler(void *, uint32_t, uint32_t, void *);
extern uint32_t GamepadHandler(void *, uint32_t, uint32_t, void *);
extern uint32_t PrinterHandler(void *, uint32_t, uint32_t, void *);
extern uint32_t MIDIHandler(void *, uint32_t, uint32_t, void *);

void SetSerialNumberString(uint32_t value)
{
    for (int i = 0; i < 8; i++)
    {
        uint8_t nibble = (uint8_t)((value >> (28 - i * 4)) & 0xF);
        uint8_t c = (nibble < 10) ? ('0' + nibble) : ('A' + nibble - 10);
        g_pui8SerialNumberString[2 + i * 2] = c;
    }
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
