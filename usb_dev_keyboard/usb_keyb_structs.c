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

//#define USB_PID_AUDIO    0x0201
#define USB_PID_PRINTER  0x0202
#define USB_PID_MIDI     0x0203

extern uint32_t KeyboardHandler(void *pvCBData, uint32_t ui32Event, uint32_t ui32MsgParam, void *pvMsgData);
extern uint32_t AudioHandler(void *pvCBData, uint32_t ui32Event, uint32_t ui32MsgParam, void *pvMsgData);
extern uint32_t GamepadHandler(void *pvCBData, uint32_t ui32Event, uint32_t ui32MsgParam, void *pvMsgData);
extern uint32_t PrinterHandler(void *pvCBData, uint32_t ui32Event, uint32_t ui32MsgParam, void *pvMsgData);
extern uint32_t MIDIHandler(void *pvCBData, uint32_t ui32Event, uint32_t ui32MsgParam, void *pvMsgData);

//****************************************************************************
//
// The languages supported by this device.
//
//****************************************************************************
const uint8_t g_pui8LangDescriptor[] =
{
    4,
    USB_DTYPE_STRING,
    USBShort(USB_LANG_EN_US)
};

//****************************************************************************
//
// The manufacturer string.
//
//****************************************************************************
const uint8_t g_pui8ManufacturerString[] =
{
    (17 + 1) * 2,
    USB_DTYPE_STRING,
    'T', 0, 'e', 0, 'x', 0, 'a', 0, 's', 0, ' ', 0, 'I', 0, 'n', 0, 's', 0,
    't', 0, 'r', 0, 'u', 0, 'm', 0, 'e', 0, 'n', 0, 't', 0, 's', 0,
};

//****************************************************************************
//
// The product string.
//
//****************************************************************************
// Keyboard product string
const uint8_t g_pui8ProductStringKeyboard[] =
{
    (16 + 1) * 2,
    USB_DTYPE_STRING,
    'K', 0, 'e', 0, 'y', 0, 'b', 0, 'o', 0, 'a', 0, 'r', 0, 'd', 0, ' ',
    'D', 0, 'e', 0, 'v', 0, 'i', 0, 'c', 0, 'e', 0
};

// Audio product string
const uint8_t g_pui8ProductStringAudio[] =
{
    (12 + 1) * 2,
    USB_DTYPE_STRING,
    'A', 0, 'u', 0, 'd', 0, 'i', 0, 'o', 0, ' ', 0, 'D', 0, 'e', 0, 'v', 0,
    'i', 0, 'c', 0, 'e', 0
};

// Gamepad product string
const uint8_t g_pui8ProductStringGamepad[] =
{
    (12 + 1) * 2,
    USB_DTYPE_STRING,
    'G', 0, 'a', 0, 'm', 0, 'e', 0, 'p', 0, 'a', 0, 'd', 0, ' ', 0, 'D', 0,
    'e', 0, 'v', 0, 'i', 0, 'c', 0, 'e', 0
};

// Printer product string
const uint8_t g_pui8ProductStringPrinter[] =
{
    (13 + 1) * 2,
    USB_DTYPE_STRING,
    'P', 0, 'r', 0, 'i', 0, 'n', 0, 't', 0, 'e', 0, 'r', 0, ' ', 0, 'D', 0,
    'e', 0, 'v', 0, 'i', 0, 'c', 0, 'e', 0
};

// MIDI product string
const uint8_t g_pui8ProductStringMIDI[] =
{
    (14 + 1) * 2,
    USB_DTYPE_STRING,
    'M', 0, 'I', 0, 'D', 0, 'I', 0, ' ', 0, 'C', 0, 'o', 0, 'n', 0, 't', 0,
    'r', 0, 'o', 0, 'l', 0, 'l', 0, 'e', 0, 'r', 0
};



//****************************************************************************
//
// The serial number string.
//
//****************************************************************************
const uint8_t g_pui8SerialNumberString[] =
{
    (8 + 1) * 2,
    USB_DTYPE_STRING,
    '1', 0, '2', 0, '3', 0, '4', 0, '5', 0, '6', 0, '7', 0, '8', 0
};

//*****************************************************************************
//
// The interface description string.
//
//*****************************************************************************
const uint8_t g_pui8HIDInterfaceString[] =
{
    (22 + 1) * 2,
    USB_DTYPE_STRING,
    'H', 0, 'I', 0, 'D', 0, ' ', 0, 'K', 0, 'e', 0, 'y', 0, 'b', 0,
    'o', 0, 'a', 0, 'r', 0, 'd', 0, ' ', 0, 'I', 0, 'n', 0, 't', 0,
    'e', 0, 'r', 0, 'f', 0, 'a', 0, 'c', 0, 'e', 0
};

//*****************************************************************************
//
// The configuration description string.
//
//*****************************************************************************
const uint8_t g_pui8ConfigString[] =
{
    (26 + 1) * 2,
    USB_DTYPE_STRING,
    'H', 0, 'I', 0, 'D', 0, ' ', 0, 'K', 0, 'e', 0, 'y', 0, 'b', 0,
    'o', 0, 'a', 0, 'r', 0, 'd', 0, ' ', 0, 'C', 0, 'o', 0, 'n', 0,
    'f', 0, 'i', 0, 'g', 0, 'u', 0, 'r', 0, 'a', 0, 't', 0, 'i', 0,
    'o', 0, 'n', 0
};

//*****************************************************************************
//
// The descriptor string table.
//
//*****************************************************************************
const uint8_t * const g_ppui8StringDescriptorsKeyboard[] =
{
    g_pui8LangDescriptor,
    g_pui8ManufacturerString,
    g_pui8ProductStringKeyboard,
    g_pui8SerialNumberString,
    g_pui8HIDInterfaceString,
    g_pui8ConfigString
};

#define NUM_STRING_DESCRIPTORS_KEYBOARD (sizeof(g_ppui8StringDescriptorsKeyboard) /            \
                                sizeof(uint8_t *))

const uint8_t * const g_ppui8StringDescriptorsAudio[] =
{
    g_pui8LangDescriptor,
    g_pui8ManufacturerString,
    g_pui8ProductStringAudio,
    g_pui8SerialNumberString
};
#define NUM_STRING_DESCRIPTORS_AUDIO (sizeof(g_ppui8StringDescriptorsAudio) / sizeof(uint8_t *))

const uint8_t * const g_ppui8StringDescriptorsGamepad[] =
{
    g_pui8LangDescriptor,
    g_pui8ManufacturerString,
    g_pui8ProductStringGamepad,
    g_pui8SerialNumberString
};
#define NUM_STRING_DESCRIPTORS_GAMEPAD (sizeof(g_ppui8StringDescriptorsGamepad) / sizeof(uint8_t *))

const uint8_t * const g_ppui8StringDescriptorsPrinter[] =
{
    g_pui8LangDescriptor,
    g_pui8ManufacturerString,
    g_pui8ProductStringPrinter,
    g_pui8SerialNumberString
};
#define NUM_STRING_DESCRIPTORS_PRINTER (sizeof(g_ppui8StringDescriptorsPrinter) / sizeof(uint8_t *))

const uint8_t * const g_ppui8StringDescriptorsMIDI[] =
{
    g_pui8LangDescriptor,
    g_pui8ManufacturerString,
    g_pui8ProductStringMIDI,
    g_pui8SerialNumberString
};
#define NUM_STRING_DESCRIPTORS_MIDI (sizeof(g_ppui8StringDescriptorsMIDI) / sizeof(uint8_t *))

//*****************************************************************************
//
// The HID keyboard device initialization and customization structures.
//
//*****************************************************************************
// Keyboard Device Attributes
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
    NUM_STRING_DESCRIPTORS_KEYBOARD
};
// Gamepad Device Attributes
tUSBDHIDGamepadDevice g_sGamepadDevice;
tUSBDHIDGamepadDevice g_sGamepadTemplate =
{
    USB_VID_TI_1CBE,
    USB_PID_GAMEPAD,
    500,
    USB_CONF_ATTR_SELF_PWR | USB_CONF_ATTR_RWAKE,
    GamepadHandler,
    (void *)&g_sGamepadDevice,
    g_ppui8StringDescriptorsGamepad,
    NUM_STRING_DESCRIPTORS_GAMEPAD
};


// Audio Device Attributes
tUSBAudioDevice g_sAudioDevice;
tUSBAudioDevice g_sAudioTemplate =
{
    USB_VID_TI_1CBE,
    USB_PID_AUDIO,
    500,
    USB_CONF_ATTR_SELF_PWR | USB_CONF_ATTR_RWAKE,
    AudioHandler,
    (void *)&g_sAudioDevice,
    g_ppui8StringDescriptorsAudio,
    NUM_STRING_DESCRIPTORS_AUDIO
};

// Printer Device Attributes
tUSBPrinterDevice g_sPrinterDevice;
tUSBPrinterDevice g_sPrinterTemplate =
{
    USB_VID_TI_1CBE,
    USB_PID_PRINTER,
    500,
    USB_CONF_ATTR_SELF_PWR | USB_CONF_ATTR_RWAKE,
    PrinterHandler,
    (void *)&g_sPrinterDevice,
    g_ppui8StringDescriptorsPrinter,
    NUM_STRING_DESCRIPTORS_PRINTER
};

// MIDI Device Attributes
tUSBMIDIDevice g_sMIDIDevice;
tUSBMIDIDevice g_sMIDITemplate =
{
    USB_VID_TI_1CBE,
    USB_PID_MIDI,
    500,
    USB_CONF_ATTR_SELF_PWR | USB_CONF_ATTR_RWAKE,
    MIDIHandler,
    (void *)&g_sMIDIDevice,
    g_ppui8StringDescriptorsMIDI,
    NUM_STRING_DESCRIPTORS_MIDI
};
