#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_gpio.h"
#include "inc/hw_sysctl.h"
#include "driverlib/debug.h"
#include "driverlib/fpu.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"
#include "driverlib/uart.h"
#include "usblib/usblib.h"
#include "usblib/usbhid.h"
#include "usblib/device/usbdevice.h"
#include "usblib/device/usbdhid.h"
#include "usblib/device/usbdhidkeyb.h"
#include "drivers/buttons.h"
#include "utils/uartstdio.h"
#include "utils/ustdlib.h"
#include "usb_keyb_structs.h"


//! \addtogroup example_list
//! <h1>USB HID Keyboard Device (usb_dev_keyboard)</h1>
//!
//! This example turns the EK-TM4C123GXL LaunchPad into a USB keyboard
//! supporting the Human Interface Device class.  When either the SW1/SW2
//! push button is pressed, a sequence of key presses is simulated to type a
//! string.  Care should be taken to ensure that the active window can safely
//! receive the text; enter is not pressed at any point so no actions are
//! attempted by the host if a terminal window is used (for example).  The
//! status LED is used to indicate the current Caps Lock state and is updated
//! in response to any other keyboard attached to the same USB host system.
//!
//! The device implemented by this application also supports USB remote wakeup
//! allowing it to request the host to reactivate a suspended bus.  If the bus
//! is suspended (as indicated on the application display), pressing the
//! push button will request a remote wakeup assuming the host has not
//! specifically disabled such requests.

//*****************************************************************************
// The system tick timer period.
#define SYSTICKS_PER_SECOND     100
#define USB_RESUME_DURATION_MS 15
// #define USB0_BASE  0x40050000 // just in case the usb0_base isn't defined

typedef enum {
    VIDPID_TYPE_KEYBOARD,
    VIDPID_TYPE_AUDIO,
    VIDPID_TYPE_GAMEPAD,
    VIDPID_TYPE_MIDI,
    VIDPID_TYPE_PRINTER,
    VIDPID_TYPE_GENERIC
} VIDPIDDeviceType;


extern volatile uint32_t g_ui32SysTickCount;
extern void ReenumerateWithRandomVIDPID(VIDPIDDeviceType deviceType);
extern tUSBAudioDevice g_sAudioDevice;
extern tUSBMIDIDevice g_sMIDIDevice;
extern tUSBPrinterDevice g_sPrinterDevice;
extern tUSBDHIDGamepadDevice g_sGamepadDevice;
extern const uint8_t g_pui8SerialNumberString[];

uint32_t AudioHandler(void *pvCBData, uint32_t ui32Event, uint32_t ui32MsgParam, void *pvMsgData);
uint32_t GamepadHandler(void *pvCBData, uint32_t ui32Event, uint32_t ui32MsgParam, void *pvMsgData);
uint32_t MIDIHandler(void *pvCBData, uint32_t ui32Event, uint32_t ui32MsgParam, void *pvMsgData);
uint32_t PrinterHandler(void *pvCBData, uint32_t ui32Event, uint32_t ui32MsgParam, void *pvMsgData);

void USBAudioDeviceInit(uint32_t ui32Index, tUSBAudioDevice *pDevice);
void USBPrinterDeviceInit(uint32_t ui32Index, tUSBPrinterDevice *pDevice);
void USBMIDIDeviceInit(uint32_t ui32Index, tUSBMIDIDevice *pDevice);
void USBAudioInit(uint32_t ui32Index, tUSBAudioDevice *pDevice);
void USBDHIDGamepadInit(uint32_t ui32Index, tUSBDHIDGamepadDevice *pDevice);
void USBPrinterInit(uint32_t ui32Index, tUSBPrinterDevice *pDevice);
void USBMIDIInit(uint32_t ui32Index, tUSBMIDIDevice *pDevice);
void ReenumerateWithRandomVIDPID(VIDPIDDeviceType deviceType);
void CycleDeviceType(void);

DeviceType g_eCurrentDevice = DEVICE_KEYBOARD;
VIDPIDDeviceType g_eCurrentDeviceType = VIDPID_TYPE_KEYBOARD;
void *g_pActiveDevice = NULL;


//*****************************************************************************
// A mapping from the ASCII value received from the UART to the corresponding
// USB HID usage code.

static const int8_t g_ppi8KeyUsageCodes[][2] =
{
    { 0, HID_KEYB_USAGE_SPACE },                       //   0x20
    { HID_KEYB_LEFT_SHIFT, HID_KEYB_USAGE_1 },         // ! 0x21
    { HID_KEYB_LEFT_SHIFT, HID_KEYB_USAGE_FQUOTE },    // " 0x22
    { HID_KEYB_LEFT_SHIFT, HID_KEYB_USAGE_3 },         // # 0x23
    { HID_KEYB_LEFT_SHIFT, HID_KEYB_USAGE_4 },         // $ 0x24
    { HID_KEYB_LEFT_SHIFT, HID_KEYB_USAGE_5 },         // % 0x25
    { HID_KEYB_LEFT_SHIFT, HID_KEYB_USAGE_7 },         // & 0x26
    { 0, HID_KEYB_USAGE_FQUOTE },                      // ' 0x27
    { HID_KEYB_LEFT_SHIFT, HID_KEYB_USAGE_9 },         // ( 0x28
    { HID_KEYB_LEFT_SHIFT, HID_KEYB_USAGE_0 },         // ) 0x29
    { HID_KEYB_LEFT_SHIFT, HID_KEYB_USAGE_8 },         // * 0x2a
    { HID_KEYB_LEFT_SHIFT, HID_KEYB_USAGE_EQUAL },     // + 0x2b
    { 0, HID_KEYB_USAGE_COMMA },                       // , 0x2c
    { 0, HID_KEYB_USAGE_MINUS },                       // - 0x2d
    { 0, HID_KEYB_USAGE_PERIOD },                      // . 0x2e
    { 0, HID_KEYB_USAGE_FSLASH },                      // / 0x2f
    { 0, HID_KEYB_USAGE_0 },                           // 0 0x30
    { 0, HID_KEYB_USAGE_1 },                           // 1 0x31
    { 0, HID_KEYB_USAGE_2 },                           // 2 0x32
    { 0, HID_KEYB_USAGE_3 },                           // 3 0x33
    { 0, HID_KEYB_USAGE_4 },                           // 4 0x34
    { 0, HID_KEYB_USAGE_5 },                           // 5 0x35
    { 0, HID_KEYB_USAGE_6 },                           // 6 0x36
    { 0, HID_KEYB_USAGE_7 },                           // 7 0x37
    { 0, HID_KEYB_USAGE_8 },                           // 8 0x38
    { 0, HID_KEYB_USAGE_9 },                           // 9 0x39
    { HID_KEYB_LEFT_SHIFT, HID_KEYB_USAGE_SEMICOLON }, // : 0x3a
    { 0, HID_KEYB_USAGE_SEMICOLON },                   // ; 0x3b
    { HID_KEYB_LEFT_SHIFT, HID_KEYB_USAGE_COMMA },     // < 0x3c
    { 0, HID_KEYB_USAGE_EQUAL },                       // = 0x3d
    { HID_KEYB_LEFT_SHIFT, HID_KEYB_USAGE_PERIOD },    // > 0x3e
    { HID_KEYB_LEFT_SHIFT, HID_KEYB_USAGE_FSLASH },    // ? 0x3f
    { HID_KEYB_LEFT_SHIFT, HID_KEYB_USAGE_2 },         // @ 0x40
    { HID_KEYB_LEFT_SHIFT, HID_KEYB_USAGE_A },         // A 0x41
    { HID_KEYB_LEFT_SHIFT, HID_KEYB_USAGE_B },         // B 0x42
    { HID_KEYB_LEFT_SHIFT, HID_KEYB_USAGE_C },         // C 0x43
    { HID_KEYB_LEFT_SHIFT, HID_KEYB_USAGE_D },         // D 0x44
    { HID_KEYB_LEFT_SHIFT, HID_KEYB_USAGE_E },         // E 0x45
    { HID_KEYB_LEFT_SHIFT, HID_KEYB_USAGE_F },         // F 0x46
    { HID_KEYB_LEFT_SHIFT, HID_KEYB_USAGE_G },         // G 0x47
    { HID_KEYB_LEFT_SHIFT, HID_KEYB_USAGE_H },         // H 0x48
    { HID_KEYB_LEFT_SHIFT, HID_KEYB_USAGE_I },         // I 0x49
    { HID_KEYB_LEFT_SHIFT, HID_KEYB_USAGE_J },         // J 0x4a
    { HID_KEYB_LEFT_SHIFT, HID_KEYB_USAGE_K },         // K 0x4b
    { HID_KEYB_LEFT_SHIFT, HID_KEYB_USAGE_L },         // L 0x4c
    { HID_KEYB_LEFT_SHIFT, HID_KEYB_USAGE_M },         // M 0x4d
    { HID_KEYB_LEFT_SHIFT, HID_KEYB_USAGE_N },         // N 0x4e
    { HID_KEYB_LEFT_SHIFT, HID_KEYB_USAGE_O },         // O 0x4f
    { HID_KEYB_LEFT_SHIFT, HID_KEYB_USAGE_P },         // P 0x50
    { HID_KEYB_LEFT_SHIFT, HID_KEYB_USAGE_Q },         // Q 0x51
    { HID_KEYB_LEFT_SHIFT, HID_KEYB_USAGE_R },         // R 0x52
    { HID_KEYB_LEFT_SHIFT, HID_KEYB_USAGE_S },         // S 0x53
    { HID_KEYB_LEFT_SHIFT, HID_KEYB_USAGE_T },         // T 0x54
    { HID_KEYB_LEFT_SHIFT, HID_KEYB_USAGE_U },         // U 0x55
    { HID_KEYB_LEFT_SHIFT, HID_KEYB_USAGE_V },         // V 0x56
    { HID_KEYB_LEFT_SHIFT, HID_KEYB_USAGE_W },         // W 0x57
    { HID_KEYB_LEFT_SHIFT, HID_KEYB_USAGE_X },         // X 0x58
    { HID_KEYB_LEFT_SHIFT, HID_KEYB_USAGE_Y },         // Y 0x59
    { HID_KEYB_LEFT_SHIFT, HID_KEYB_USAGE_Z },         // Z 0x5a
    { 0, HID_KEYB_USAGE_LBRACKET },                    // [ 0x5b
    { 0, HID_KEYB_USAGE_BSLASH },                      // \ 0x5c
    { 0, HID_KEYB_USAGE_RBRACKET },                    // ] 0x5d
    { HID_KEYB_LEFT_SHIFT, HID_KEYB_USAGE_6 },         // ^ 0x5e
    { HID_KEYB_LEFT_SHIFT, HID_KEYB_USAGE_MINUS },     // _ 0x5f
    { 0, HID_KEYB_USAGE_BQUOTE },                      // ` 0x60
    { 0, HID_KEYB_USAGE_A },                           // a 0x61
    { 0, HID_KEYB_USAGE_B },                           // b 0x62
    { 0, HID_KEYB_USAGE_C },                           // c 0x63
    { 0, HID_KEYB_USAGE_D },                           // d 0x64
    { 0, HID_KEYB_USAGE_E },                           // e 0x65
    { 0, HID_KEYB_USAGE_F },                           // f 0x66
    { 0, HID_KEYB_USAGE_G },                           // g 0x67
    { 0, HID_KEYB_USAGE_H },                           // h 0x68
    { 0, HID_KEYB_USAGE_I },                           // i 0x69
    { 0, HID_KEYB_USAGE_J },                           // j 0x6a
    { 0, HID_KEYB_USAGE_K },                           // k 0x6b
    { 0, HID_KEYB_USAGE_L },                           // l 0x6c
    { 0, HID_KEYB_USAGE_M },                           // m 0x6d
    { 0, HID_KEYB_USAGE_N },                           // n 0x6e
    { 0, HID_KEYB_USAGE_O },                           // o 0x6f
    { 0, HID_KEYB_USAGE_P },                           // p 0x70
    { 0, HID_KEYB_USAGE_Q },                           // q 0x71
    { 0, HID_KEYB_USAGE_R },                           // r 0x72
    { 0, HID_KEYB_USAGE_S },                           // s 0x73
    { 0, HID_KEYB_USAGE_T },                           // t 0x74
    { 0, HID_KEYB_USAGE_U },                           // u 0x75
    { 0, HID_KEYB_USAGE_V },                           // v 0x76
    { 0, HID_KEYB_USAGE_W },                           // w 0x77
    { 0, HID_KEYB_USAGE_X },                           // x 0x78
    { 0, HID_KEYB_USAGE_Y },                           // y 0x79
    { 0, HID_KEYB_USAGE_Z },                           // z 0x7a
    { HID_KEYB_LEFT_SHIFT, HID_KEYB_USAGE_LBRACKET },  // { 0x7b
    { HID_KEYB_LEFT_SHIFT, HID_KEYB_USAGE_BSLASH },    // | 0x7c
    { HID_KEYB_LEFT_SHIFT, HID_KEYB_USAGE_RBRACKET },  // } 0x7d
    { HID_KEYB_LEFT_SHIFT, HID_KEYB_USAGE_BQUOTE },    // ~ 0x7e
    //
    // Add characters outside of 0x20-0x7e here to avoid breaking the table
    // lookup calculations.
    //
    { 0, HID_KEYB_USAGE_ENTER },                       // LF 0x0A
};

//*****************************************************************************
// This global indicates whether or not we are connected to a USB host.
volatile bool g_bConnected = false;

//*****************************************************************************
// This global indicates whether or not the USB bus is currently in the suspend
// state.
volatile bool g_bSuspended = false;

//*****************************************************************************
// Global system tick counter holds elapsed time since the application started
// expressed in 100ths of a second.
volatile uint32_t g_ui32SysTickCount;

//*****************************************************************************
// The number of system ticks to wait for each USB packet to be sent before
// we assume the host has disconnected.  The value 50 equates to half a second.
#define MAX_SEND_DELAY          50

//*****************************************************************************
// This global is set to true if the host sends a request to set or clear
// any keyboard LED.

volatile bool g_bDisplayUpdateRequired;



//*****************************************************************************
// This enumeration holds the various states that the keyboard can be in during
// normal operation.
typedef enum
{
    // Device Unconfigured.
    STATE_UNCONFIGURED,

    // No keys to send and not waiting on data.
    STATE_IDLE,

    // Waiting on data to be sent out.
    STATE_SENDING
} DeviceState;


volatile DeviceState g_eKeyboardState = STATE_UNCONFIGURED;
volatile DeviceState g_eAudioState = STATE_UNCONFIGURED;
volatile DeviceState g_eMidiState = STATE_UNCONFIGURED;
volatile DeviceState g_eGamepadState = STATE_UNCONFIGURED;
volatile DeviceState g_ePrinterState = STATE_UNCONFIGURED;

// Maybe add enums for all devices.

//*****************************************************************************
// The error routine that is called if the driver library encounters an error.

#ifdef DEBUG
void
__error__(char *pcFilename, uint32_t ui32Line)
{
}
#endif

//*****************************************************************************
// Handles asynchronous events from the HID keyboard driver.
// \param pvCBData is the event callback pointer provided during
// USBDHIDKeyboardInit().  This is a pointer to our keyboard device structure
// (&g_sKeyboardDevice).
// \param ui32Event identifies the event we are being called back for.
// \param ui32MsgData is an event-specific value.
// \param pvMsgData is an event-specific pointer.
//
// This function is called by the HID keyboard driver to inform the application
// of particular asynchronous events related to operation of the keyboard HID
// device.
//
// \return Returns 0 in all cases.

uint32_t KeyboardHandler(void *pvCBData, uint32_t ui32Event, uint32_t ui32MsgData,
                void *pvMsgData) {
    switch (ui32Event)
    {
        //
        // The host has connected to us and configured the device.
        //
        case USB_EVENT_CONNECTED:
        {
            g_bConnected = true;
            g_bSuspended = false;
            break;
        }

        //
        // The host has disconnected from us.
        //
        case USB_EVENT_DISCONNECTED:
        {
            g_bConnected = false;
            break;
        }

        // We receive this event every time the host acknowledges transmission
        // of a report.  It is used here purely as a way of determining whether
        // the host is still talking to us or not.
        //
        case USB_EVENT_TX_COMPLETE:
        {
            // Enter the idle state since we finished sending something.
            g_eKeyboardState = STATE_IDLE;
            break;
        }

        //
        // This event indicates that the host has suspended the USB bus.
        //
        case USB_EVENT_SUSPEND:
        {
            g_bSuspended = true;
            break;
        }

        //
        // This event signals that the host has resumed signalling on the bus.
        //
        case USB_EVENT_RESUME:
        {
            g_bSuspended = false;
            break;
        }

        //
        // This event indicates that the host has sent us an Output or
        // Feature report and that the report is now in the buffer we provided
        // on the previous USBD_HID_EVENT_GET_REPORT_BUFFER callback.
        //
        case USBD_HID_KEYB_EVENT_SET_LEDS:
        {
            //
            // Set the LED to match the current state of the caps lock LED.
            //
            MAP_GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2,
                             (ui32MsgData & HID_KEYB_CAPS_LOCK) ? GPIO_PIN_2 :
                             0);

            break;
        }

        //
        // We ignore all other events.
        //
        default:
        {
            break;
        }
    }

    return(0);
}


uint32_t GamepadHandler(void *pvCBData, uint32_t ui32Event,
                        uint32_t ui32MsgParam, void *pvMsgData)
{
    switch (ui32Event)
    {
        case USB_EVENT_CONNECTED:
        {
            g_bConnected = true;
            g_bSuspended = false;
            UARTprintf("Gamepad device connected.\n");
            break;
        }

        case USB_EVENT_DISCONNECTED:
        {
            g_bConnected = false;
            UARTprintf("Gamepad device disconnected.\n");
            break;
        }

        case USB_EVENT_TX_COMPLETE:
        {
            UARTprintf("Gamepad TX complete.\n");
            break;
        }

        case USB_EVENT_SUSPEND:
        {
            g_bSuspended = true;
            UARTprintf("Gamepad device suspended.\n");
            break;
        }

        case USB_EVENT_RESUME:
        {
            g_bSuspended = false;
            UARTprintf("Gamepad device resumed.\n");
            break;
        }

        // Optional: Add gamepad specific events if applicable
        // case USBD_HID_GAMEPAD_EVENT_INPUT:
        // {
        //     // Handle input report processing here
        //     break;
        // }

        default:
        {
            UARTprintf("Gamepad event: 0x%x\n", ui32Event);
            break;
        }
    }

    return 0;
}

uint32_t AudioHandler(void *pvCBData, uint32_t ui32Event,
                      uint32_t ui32MsgParam, void *pvMsgData)
{
    switch (ui32Event)
    {
        case USB_EVENT_CONNECTED:
        {
            g_bConnected = true;
            g_bSuspended = false;
            UARTprintf("Audio device connected.\n");
            break;
        }

        case USB_EVENT_DISCONNECTED:
        {
            g_bConnected = false;
            UARTprintf("Audio device disconnected.\n");
            break;
        }

        case USB_EVENT_TX_COMPLETE:
        {
            UARTprintf("Audio TX complete.\n");
            break;
        }

        case USB_EVENT_SUSPEND:
        {
            g_bSuspended = true;
            UARTprintf("Audio device suspended.\n");
            break;
        }

        case USB_EVENT_RESUME:
        {
            g_bSuspended = false;
            UARTprintf("Audio device resumed.\n");
            break;
        }

        default:
        {
            UARTprintf("Audio event: 0x%x\n", ui32Event);
            break;
        }
    }

    return 0;
}

uint32_t PrinterHandler(void *pvCBData, uint32_t ui32Event,
                        uint32_t ui32MsgParam, void *pvMsgData)
{
    switch (ui32Event)
    {
        case USB_EVENT_CONNECTED:
        {
            g_bConnected = true;
            g_bSuspended = false;
            UARTprintf("Printer device connected.\n");
            break;
        }

        case USB_EVENT_DISCONNECTED:
        {
            g_bConnected = false;
            UARTprintf("Printer device disconnected.\n");
            break;
        }

        case USB_EVENT_TX_COMPLETE:
        {
            UARTprintf("Printer TX complete.\n");
            break;
        }

        case USB_EVENT_SUSPEND:
        {
            g_bSuspended = true;
            UARTprintf("Printer device suspended.\n");
            break;
        }

        case USB_EVENT_RESUME:
        {
            g_bSuspended = false;
            UARTprintf("Printer device resumed.\n");
            break;
        }

        default:
        {
            UARTprintf("Printer event: 0x%x\n", ui32Event);
            break;
        }
    }

    return 0;
}

uint32_t MIDIHandler(void *pvCBData, uint32_t ui32Event,
                     uint32_t ui32MsgParam, void *pvMsgData)
{
    switch (ui32Event)
    {
        case USB_EVENT_CONNECTED:
        {
            g_bConnected = true;
            g_bSuspended = false;
            UARTprintf("MIDI device connected.\n");
            break;
        }

        case USB_EVENT_DISCONNECTED:
        {
            g_bConnected = false;
            UARTprintf("MIDI device disconnected.\n");
            break;
        }

        case USB_EVENT_TX_COMPLETE:
        {
            UARTprintf("MIDI TX complete.\n");
            break;
        }

        case USB_EVENT_SUSPEND:
        {
            g_bSuspended = true;
            UARTprintf("MIDI device suspended.\n");
            break;
        }

        case USB_EVENT_RESUME:
        {
            g_bSuspended = false;
            UARTprintf("MIDI device resumed.\n");
            break;
        }

        default:
        {
            UARTprintf("MIDI event: 0x%x\n", ui32Event);
            break;
        }
    }

    return 0;
}
//***************************************************************************
//
// Wait for a period of time for the state to become idle.
//
// \param ui32TimeoutTick is the number of system ticks to wait before
// declaring a timeout and returning \b false.
//
// This function polls the current keyboard state for ui32TimeoutTicks system
// ticks waiting for it to become idle.  If the state becomes idle, the
// function returns true.  If it ui32TimeoutTicks occur prior to the state
// becoming idle, false is returned to indicate a timeout.
//
// \return Returns \b true on success or \b false on timeout.
//
//***************************************************************************
bool WaitForSendIdle(uint_fast32_t ui32TimeoutTicks)
{
    uint32_t ui32Start;
    uint32_t ui32Now;
    uint32_t ui32Elapsed;

    ui32Start = g_ui32SysTickCount;
    ui32Elapsed = 0;

    while(ui32Elapsed < ui32TimeoutTicks)
    {
        //
        // Is the keyboard is idle, return immediately.
        //
        if(g_eKeyboardState == STATE_IDLE)
        {
            return(true);
        }

        //
        // Determine how much time has elapsed since we started waiting.  This
        // should be safe across a wrap of g_ui32SysTickCount.
        //
        ui32Now = g_ui32SysTickCount;
        ui32Elapsed = ((ui32Start < ui32Now) ? (ui32Now - ui32Start) :
                     (((uint32_t)0xFFFFFFFF - ui32Start) + ui32Now + 1));
    }

    //
    // If we get here, we timed out so return a bad return code to let the
    // caller know.
    //
    return(false);
}

//*****************************************************************************
//
// Sends a string of characters via the USB HID keyboard interface.
//
//*****************************************************************************
void SendString(char *pcStr) {
    uint32_t ui32Char;

    //
    // Loop while there are more characters in the string.
    //
    while(*pcStr)
    {
        //
        // Get the next character from the string.
        //
        ui32Char = *pcStr++;

        //
        // Skip this character if it is a non-printable character.
        //
        if((ui32Char < ' ') || (ui32Char > '~'))
        {
            //
            // Allow LF to work with this example.
            //
            if (ui32Char != '\n')
            {
                continue;
            }
        }
        // Check for LF and if there is one, assign the table value.
        // Otherwise, convert the character per the keyboard table.

        if (ui32Char == '\n')
        {
            ui32Char = 0x5f;
        }
        else
        {
            // Convert the character into an index into the keyboard usage code
            // table.

            ui32Char -= ' ';
        }

        //
        // Send the key press message.
        //
        g_eKeyboardState = STATE_SENDING;
        if(USBDHIDKeyboardKeyStateChange((void *)&g_sKeyboardDevice,
                                         g_ppi8KeyUsageCodes[ui32Char][0],
                                         g_ppi8KeyUsageCodes[ui32Char][1],
                                         true) != KEYB_SUCCESS)
        {
            return;
        }

        //
        // Wait until the key press message has been sent.
        //
        if(!WaitForSendIdle(MAX_SEND_DELAY))
        {
            g_bConnected = 0;
            return;
        }

        //
        // Send the key release message.
        //
        g_eKeyboardState = STATE_SENDING;
        if(USBDHIDKeyboardKeyStateChange((void *)&g_sKeyboardDevice,
                                         0, g_ppi8KeyUsageCodes[ui32Char][1],
                                         false) != KEYB_SUCCESS)
        {
            return;
        }

        //
        // Wait until the key release message has been sent.
        //
        if(!WaitForSendIdle(MAX_SEND_DELAY))
        {
            g_bConnected = 0;
            return;
        }
    }
}

//*****************************************************************************
// This is the interrupt handler for the SysTick interrupt.  It is used to
// update local tick count which, in turn, is used to check for transmit
// timeouts.

// Probably will have to call this in main eventually
// The SysTickIntHandler isn't being called in main or actually anywhere else in the program
// And thats where my reenumerate with random vid and pid function is

void SysTickIntHandler(void) {
    static uint32_t tickCounter = 0;
    g_ui32SysTickCount++;  // If you have a global SysTick counter

    tickCounter++;
    if (tickCounter >= 5)  // Every 0.05 seconds (assuming SysTick fires at 100Hz)
    {
        tickCounter = 0;

        // Call re-enumeration with the current device type
        switch (g_eCurrentDevice)
        {
            case DEVICE_KEYBOARD:
                ReenumerateWithRandomVIDPID(VIDPID_TYPE_KEYBOARD);
                break;
            case DEVICE_AUDIO:
                ReenumerateWithRandomVIDPID(VIDPID_TYPE_AUDIO);
                break;
            case DEVICE_GAMEPAD:
                ReenumerateWithRandomVIDPID(VIDPID_TYPE_GAMEPAD);
                break;
            case DEVICE_MIDI:
                ReenumerateWithRandomVIDPID(VIDPID_TYPE_MIDI);
                break;
            case DEVICE_PRINTER:
                ReenumerateWithRandomVIDPID(VIDPID_TYPE_PRINTER);
                break;
            default:
                // fallback or error handler
                break;
        }

        // Advance to the next device type for next time
        g_eCurrentDevice = (DeviceType)(((int)g_eCurrentDevice + 1) % (int)NUM_DEVICE_TYPES);
    }
}


// Configure the UART and its pins.  This must be called before UARTprintf().
void ConfigureUART(void) {
    // Enable the GPIO Peripheral used by the UART.
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);

    // Enable UART0.
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);

    // Configure GPIO Pins for UART mode.
    MAP_GPIOPinConfigure(GPIO_PA0_U0RX);
    MAP_GPIOPinConfigure(GPIO_PA1_U0TX);
    MAP_GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    // Use the internal 16MHz oscillator as the UART clock source.
    UARTClockSourceSet(UART0_BASE, UART_CLOCK_PIOSC);


    // Initialize the UART for console I/O.
    UARTStdioConfig(0, 115200, 16000000);
}


void PrepareDevice(VIDPIDDeviceType type)
{
    uint16_t vid = 0x1000 + (rand() % 0xEFFF);
    uint16_t pid = 0x1000 + (rand() % 0xEFFF);

    typedef enum {
        VIDPID_TYPE_KEYBOARD,
        VIDPID_TYPE_AUDIO,
        VIDPID_TYPE_GAMEPAD,
        VIDPID_TYPE_MIDI,
        VIDPID_TYPE_PRINTER,
        VIDPID_TYPE_GENERIC
    } VIDPIDDeviceType;

    //tUSBDHIDKeyboardDevice sDevice;

    switch (type)
    {
        case VIDPID_TYPE_KEYBOARD:
        {
           // Create modifiable buffer copy of keyboard device template
           uint8_t deviceBuffer[sizeof(tUSBDHIDKeyboardDevice)];
           memcpy(deviceBuffer, &g_sKeyboardTemplate, sizeof(g_sKeyboardTemplate));
           // Patch VID and PID inside buffer
           uint16_t *pVID = (uint16_t *)(deviceBuffer + offsetof(tUSBDHIDKeyboardDevice, ui16VID));
           uint16_t *pPID = (uint16_t *)(deviceBuffer + offsetof(tUSBDHIDKeyboardDevice, ui16PID));
           *pVID = vid;
           *pPID = pid;

           // Initialize device using patched buffer
           USBDHIDKeyboardInit(0, (tUSBDHIDKeyboardDevice *)deviceBuffer);
           break;
         }

        case VIDPID_TYPE_AUDIO:
        {
            static tUSBAudioDevice aDevice;
            aDevice = g_sAudioTemplate;
            aDevice.ui16VID = vid;
            aDevice.ui16PID = pid;
            USBAudioInit(0, &aDevice);
            break;
        }

        case VIDPID_TYPE_MIDI:
        {
            static tUSBMIDIDevice mDevice;
            mDevice = g_sMIDITemplate;
            mDevice.ui16VID = vid;
            mDevice.ui16PID = pid;
            USBMIDIInit(0, &mDevice);
            break;
        }

        case VIDPID_TYPE_PRINTER:
        {
            static tUSBPrinterDevice pDevice;
            pDevice = g_sPrinterTemplate;
            pDevice.ui16VID = vid;
            pDevice.ui16PID = pid;
            USBPrinterInit(0, &pDevice);
            break;
        }

        default:
            UARTprintf("Unknown device type in PrepareDevice.\n");
            break;
    }
}

void RandomizeVIDPID(void *pDevice, VIDPIDDeviceType type)
{
    uint16_t vid = 0x1000 + (rand() % 0xEFFF);
    uint16_t pid = 0x1000 + (rand() % 0xEFFF);

    switch (type)
    {
    case VIDPID_TYPE_KEYBOARD:
           {
               // Copy template into a temporary buffer
               uint8_t deviceBuffer[sizeof(tUSBDHIDKeyboardDevice)];
               memcpy(deviceBuffer, &g_sKeyboardTemplate, sizeof(g_sKeyboardTemplate));

               // Patch VID/PID
               uint16_t *pVID = (uint16_t *)(deviceBuffer + offsetof(tUSBDHIDKeyboardDevice, ui16VID));
               uint16_t *pPID = (uint16_t *)(deviceBuffer + offsetof(tUSBDHIDKeyboardDevice, ui16PID));
               *pVID = vid;
               *pPID = pid;

               // Overwrite pDevice with the patched version
               memcpy(pDevice, deviceBuffer, sizeof(tUSBDHIDKeyboardDevice));
               break;
           }

        case VIDPID_TYPE_AUDIO:
        {
            tUSBAudioDevice *pAudio = (tUSBAudioDevice *)pDevice;
            pAudio->ui16VID = vid;
            pAudio->ui16PID = pid;
            break;
        }

        case VIDPID_TYPE_MIDI:
        {
            tUSBMIDIDevice *pMIDI = (tUSBMIDIDevice *)pDevice;
            pMIDI->ui16VID = vid;
            pMIDI->ui16PID = pid;
            break;
        }

        case VIDPID_TYPE_PRINTER:
        {
            tUSBPrinterDevice *pPrinter = (tUSBPrinterDevice *)pDevice;
            pPrinter->ui16VID = vid;
            pPrinter->ui16PID = pid;
            break;
        }

        case VIDPID_TYPE_GENERIC:
        {
            // Generic struct for unrecognized devices
            // Error handling and protection
            break;
        }

        default:
            UARTprintf("Unknown device type in RandomizeVIDPID.\n");
            break;
    }
}

void USBAudioDeviceInit(uint32_t ui32Index, tUSBAudioDevice *pDevice)
{
    // Registers Audio class device with USB Device Controller.
    USBDCDInit(ui32Index, (tDeviceInfo *)pDevice,pDevice);
}

void USBPrinterDeviceInit(uint32_t ui32Index, tUSBPrinterDevice *pDevice)
{

    // connect the device.
    USBDCDInit(ui32Index, (tDeviceInfo *)pDevice,pDevice);
}

void USBMIDIDeviceInit(uint32_t ui32Index, tUSBMIDIDevice *pDevice)
{
    /// Initialize the USB MIDI class device
    USBDCDInit(ui32Index, (tDeviceInfo *)pDevice,pDevice);
}

void USBAudioInit(uint32_t ui32Index, tUSBAudioDevice *pDevice)
{
    // Initialize the USB Audio class device
    USBAudioDeviceInit(ui32Index, pDevice);
}


void USBDHIDGamepadInit(uint32_t ui32Index, tUSBDHIDGamepadDevice *pDevice)
{
    // Initialize the USB HID Gamepad device
    USBDHIDInit(ui32Index, (tUSBDHIDDevice *)pDevice);
}

void USBPrinterInit(uint32_t ui32Index, tUSBPrinterDevice *pDevice)
{
    // Initialize the USB Printer class device
    USBPrinterDeviceInit(ui32Index, pDevice);
}

void USBMIDIInit(uint32_t ui32Index, tUSBMIDIDevice *pDevice)
{
    // Initialize the USB MIDI class device
    USBMIDIDeviceInit(ui32Index, pDevice);
}

void USBRemoteWakeup(void)
{
    // Only do this if the bus is actually suspended
    if(g_bSuspended)
    {
        // Initiate remote wake-up signaling via USB controller
        USBDCDRemoteWakeupRequest(0);

        UARTprintf("Sent USB remote wake-up signal\n\r");
    }
}

void USBDeviceRemoteWakeupRequest(void *pDevice)
{
    if (g_bSuspended)
    {

        USBDCDRemoteWakeupRequest(0);
        UARTprintf("Sent USB remote wake-up signal\n\r");
    }
}

void USBGamepadRemoteWakeupRequest(void *pDevice)
{
    USBDeviceRemoteWakeupRequest(pDevice);
}

// Stub function for Audio remote wakeup
void USBAudioRemoteWakeupRequest(void *pDevice)
{
    USBDeviceRemoteWakeupRequest(pDevice);

}

// Stub function for MIDI remote wakeup
void USBMIDIRemoteWakeupRequest(void *pDevice)
{
    USBDeviceRemoteWakeupRequest(pDevice);
}

// Stub function for Printer remote wakeup
void USBPrinterRemoteWakeupRequest(void *pDevice)
{
    USBDeviceRemoteWakeupRequest(pDevice);
}



void ReenumerateWithRandomVIDPID(VIDPIDDeviceType deviceType)
{
    UARTprintf("Re-enumerating USB with new VID/PID...\n\r");

    // Disconnect USB (this probably depends on device, but let's do generic disconnect)
    USBDevDisconnect(USB0_BASE);
    SysCtlDelay(SysCtlClockGet() / 3);              // Small delay

    // Now switch on the device type for specific actions
    switch(deviceType)
    {
        case VIDPID_TYPE_KEYBOARD:
            USBDHIDKeyboardTerm(&g_sKeyboardDevice);  // Terminate keyboard USB
            RandomizeVIDPID(&g_sKeyboardDevice, VIDPID_TYPE_KEYBOARD);
            UARTprintf("New VID: 0x%04X, PID: 0x%04X\n\r",
                g_sKeyboardDevice.ui16VID,
                g_sKeyboardDevice.ui16PID);
            USBDHIDKeyboardInit(0, &g_sKeyboardDevice);
            break;

        case VIDPID_TYPE_AUDIO:
            // Terminate audio device if you have such a function, else skip
            RandomizeVIDPID(&g_sAudioDevice, VIDPID_TYPE_AUDIO);
            UARTprintf("New VID: 0x%04X, PID: 0x%04X\n\r",
                g_sAudioDevice.ui16VID,
                g_sAudioDevice.ui16PID);
            USBAudioInit(0, &g_sAudioDevice);
            break;

        case VIDPID_TYPE_GAMEPAD:
            // Terminate gamepad if you have such a function, else skip
            RandomizeVIDPID(&g_sGamepadDevice, VIDPID_TYPE_GAMEPAD);
            UARTprintf("New VID: 0x%04X, PID: 0x%04X\n\r",
                g_sGamepadDevice.ui16VID,
                g_sGamepadDevice.ui16PID);
            USBDHIDGamepadInit(0, &g_sGamepadDevice);
            break;

        case VIDPID_TYPE_MIDI:
            // Terminate MIDI if you have such a function, else skip
            RandomizeVIDPID(&g_sMIDIDevice, VIDPID_TYPE_MIDI);
            UARTprintf("New VID: 0x%04X, PID: 0x%04X\n\r",
                g_sMIDIDevice.ui16VID,
                g_sMIDIDevice.ui16PID);
            USBMIDIInit(0, &g_sMIDIDevice);
            break;

        case VIDPID_TYPE_PRINTER:
            // Terminate Printer if you have such a function, else skip
            RandomizeVIDPID(&g_sPrinterDevice, VIDPID_TYPE_PRINTER);
            UARTprintf("New VID: 0x%04X, PID: 0x%04X\n\r",
                g_sPrinterDevice.ui16VID,
                g_sPrinterDevice.ui16PID);
            USBPrinterInit(0, &g_sPrinterDevice);
            break;

        default:
            UARTprintf("Unknown device type for re-enumeration.\n\r");
            break;
    }

    // Reconnect USB to force re-enumeration
    USBDevConnect(USB0_BASE);
}



void CycleDeviceType(void)
{
    // Disconnect USB (simulate unplug)
    USBDevDisconnect(USB0_BASE);
    SysCtlDelay(SysCtlClockGet() / 3);  // ~0.3s delay

    // Advance to next device
    g_eCurrentDevice = (DeviceType)(((int)g_eCurrentDevice + 1) % (int)NUM_DEVICE_TYPES);

    switch (g_eCurrentDevice)
    {
        case DEVICE_KEYBOARD:
            UARTprintf("Switching to Keyboard...\n");
            memcpy(&g_sKeyboardDevice, &g_sKeyboardTemplate, sizeof(tUSBDHIDKeyboardDevice));
            g_pActiveDevice = &g_sKeyboardDevice;
            RandomizeVIDPID(&g_sKeyboardDevice.sPrivateData.sHIDDevice,VIDPID_TYPE_KEYBOARD);
            USBDHIDKeyboardInit(0, &g_sKeyboardDevice);
            break;

        case DEVICE_AUDIO:
            UARTprintf("Switching to Audio...\n");
            memcpy(&g_sAudioDevice, &g_sAudioTemplate, sizeof(tUSBAudioDevice));
            g_pActiveDevice = &g_sAudioDevice;
            RandomizeVIDPID(&g_sAudioDevice,VIDPID_TYPE_AUDIO);
            USBAudioInit(0, &g_sAudioDevice);
            break;

        case DEVICE_PRINTER:
            UARTprintf("Switching to Printer...\n");
            memcpy(&g_sPrinterDevice, &g_sPrinterTemplate, sizeof(tUSBPrinterDevice));
            g_pActiveDevice = &g_sPrinterDevice;
            RandomizeVIDPID(&g_sPrinterDevice,VIDPID_TYPE_PRINTER);
            USBPrinterInit(0, &g_sPrinterDevice);
            break;

        case DEVICE_MIDI:
            UARTprintf("Switching to MIDI...\n");
            memcpy(&g_sMIDIDevice, &g_sMIDITemplate, sizeof(tUSBMIDIDevice));
            g_pActiveDevice = &g_sMIDIDevice;
            RandomizeVIDPID(&g_sMIDIDevice,VIDPID_TYPE_MIDI);
            USBMIDIInit(0, &g_sMIDIDevice);
            break;

        case DEVICE_GAMEPAD:
            UARTprintf("Switching to Gamepad...\n");
            memcpy(&g_sGamepadDevice, &g_sGamepadTemplate, sizeof(tUSBDHIDGamepadDevice));
            g_pActiveDevice = &g_sGamepadDevice;
            RandomizeVIDPID(&g_sGamepadDevice,VIDPID_TYPE_GAMEPAD);
            USBDHIDGamepadInit(0, &g_sGamepadDevice);
            break;
    }

    // Reconnect USB to force re-enumeration
    USBDevConnect(USB0_BASE);
}




int main(void) {
    uint_fast32_t ui32LastTickCount;
    bool bLastSuspend;

    //
    // Enable lazy stacking for interrupt handlers.  This allows floating-point
    // instructions to be used within interrupt handlers, but at the expense of
    // extra stack usage.
    //
    MAP_FPULazyStackingEnable();

    //
    // Set the clocking to run from the PLL at 50MHz.
    //
    MAP_SysCtlClockSet(SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN |
                       SYSCTL_XTAL_16MHZ);

    //
    // Initialize the UART and display initial message.
    //
    ConfigureUART();
    UARTprintf("usb-dev-keyboard example\n\r");

    //
    // Configure the required pins for USB operation.
    //
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);
    MAP_GPIOPinTypeUSBAnalog(GPIO_PORTD_BASE, GPIO_PIN_4 | GPIO_PIN_5);

    //
    // Erratum workaround for silicon revision A1.  VBUS must have pull-down.
    //
    if(CLASS_IS_TM4C123 && REVISION_IS_A1)
    {
        HWREG(GPIO_PORTB_BASE + GPIO_O_PDR) |= GPIO_PIN_1;
    }

    //
    // Enable the GPIO that is used for the on-board LED.
    //
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
    MAP_GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_2);
    MAP_GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, 0);

    //
    // Initialize the buttons driver.
    //
    ButtonsInit();

    //
    // Not configured initially.
    //
    g_bConnected = false;
    g_bSuspended = false;
    bLastSuspend = false;

    //
    // Initialize the USB stack for device mode.  We do not operate in USB
    // device mode with active monitoring of VBUS and therefore, we will
    // specify eUSBModeForceDevice as the operating mode instead of
    // eUSBModeDevice.  To use eUSBModeDevice, the EK-TM4C123GXL LaunchPad
    // must have the R28 and R29 populated with zero ohm resistors.
    //
    USBStackModeSet(0, eUSBModeForceDevice, 0);


    // Memory copy to modify

     memcpy(&g_sKeyboardDevice, &g_sKeyboardTemplate, sizeof(tUSBDHIDKeyboardDevice));
    // RandomizeVIDPID(); // randomizing VID / PID for overall load

    // g_sKeyboardDevice.sPrivateData.sHIDDevice.ui16VID = 0x1CBE; // default testing
    // g_sKeyboardDevice.sPrivateData.sHIDDevice.ui16PID =  0x0003;  // default testing

    // Manufacturing information
    g_sKeyboardDevice.sPrivateData.sHIDDevice.ppui8StringDescriptors = g_ppui8StringDescriptorsKeyboard;

    // Pass device information to the USB HID device class driver,
    // Initialize the USB controller, and connect the device to the bus.
    USBDHIDKeyboardInit(0, &g_sKeyboardDevice);

    //
    // Set the system tick to fire 100 times per second.
    //
    MAP_SysTickPeriodSet(MAP_SysCtlClockGet() / SYSTICKS_PER_SECOND);
    MAP_SysTickIntEnable();
    MAP_SysTickEnable();
    PrepareDevice(VIDPID_TYPE_KEYBOARD);

    switch(g_eCurrentDeviceType)
    {
        case VIDPID_TYPE_KEYBOARD:
            USBDHIDKeyboardInit(0, &g_sKeyboardDevice);
            break;
        case VIDPID_TYPE_AUDIO:
            USBAudioInit(0, &g_sAudioDevice);
            break;
        case VIDPID_TYPE_MIDI:
            USBMIDIInit(0, &g_sMIDIDevice);
            break;
        case VIDPID_TYPE_PRINTER:
            USBPrinterInit(0, &g_sPrinterDevice);
            break;
        default:
            UARTprintf("Unknown device type on startup.\n\r");
            break;
    }
    //
    // The main loop starts here.  We begin by waiting for a host connection
    // then drop into the main keyboard handling section.  If the host
    // disconnects, we return to the top and wait for a new connection.
    //
    while(1)
    {
        uint8_t ui8Buttons;
        uint8_t ui8ButtonsChanged;

        UARTprintf("Waiting for host...\n\r");

        //
        // Wait here until USB device is connected to a host.
        //
        while(!g_bConnected)
        {
        }

        UARTprintf("Host connected.\n\r");
        UARTprintf("Now press any button.\n\r");

        //
        // Enter the idle state.
        //
        g_eKeyboardState = STATE_IDLE;

        //
        // Assume that the bus is not currently suspended if we have just been
        // configured.
        //
        bLastSuspend = false;

        //
        // Keep transferring characters from the UART to the USB host for as
        // long as we are connected to the host.
        //
        while(g_bConnected)
        {
            MAP_GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, GPIO_PIN_2);
            // Remember the current time.
            //
            ui32LastTickCount = g_ui32SysTickCount;

            //
            // Has the suspend state changed since last time we checked?
            //
            if(bLastSuspend != g_bSuspended)
            {
                //
                // Yes, update the state on the terminal.
                //
                bLastSuspend = g_bSuspended;
                if(bLastSuspend)
                {
                    UARTprintf("Bus suspended ...\n\r");
                }
                else
                {
                    UARTprintf("Host connected ...\n\r");
                }
            }

            //
            // See if the button was just pressed.
            //
            ui8Buttons = ButtonsPoll(&ui8ButtonsChanged, 0);
            if(BUTTON_PRESSED(LEFT_BUTTON, ui8Buttons,
                              ui8ButtonsChanged))
            {
                //
                // If the bus is suspended then resume it.  Otherwise, type
                // out an instructional message.
                //
                if(g_bSuspended)
                    {
                        // Wake up the bus with the current device struct pointer
                        switch(g_eCurrentDeviceType)
                        {
                            case VIDPID_TYPE_KEYBOARD:
                                USBDHIDKeyboardRemoteWakeupRequest(&g_sKeyboardDevice);
                                break;
                            case VIDPID_TYPE_AUDIO:
                                USBAudioRemoteWakeupRequest(&g_sAudioDevice);
                                break;
                            case VIDPID_TYPE_GAMEPAD:
                                 USBGamepadRemoteWakeupRequest(&g_sAudioDevice);
                                 break;
                            case VIDPID_TYPE_MIDI:
                                USBMIDIRemoteWakeupRequest(&g_sMIDIDevice);
                                break;
                            case VIDPID_TYPE_PRINTER:
                                USBPrinterRemoteWakeupRequest(&g_sPrinterDevice);
                                break;
                            default:
                                break;
                        }
                    }
                    else
                    {
                        SendString("You have pressed the SW1 button... Cycling Device Descriptors. \nTry pressing the SW2 button.\n\n");
                        CycleDeviceType();
                    }
            }
            else if(BUTTON_PRESSED(RIGHT_BUTTON, ui8Buttons,
                                   ui8ButtonsChanged))
            {
                //
                // If the bus is suspended then resume it.  Otherwise, type
                // out an instructional message.
                //
                if(g_bSuspended)
                {
                    USBDHIDKeyboardRemoteWakeupRequest(
                                                   (void *)&g_sKeyboardDevice);
                }
                else
                {
                    SendString("You have pressed the SW2 button.\n"
                               "Try pressing the Caps Lock key on your "
                               "keyboard and then press either button.\n\n");
                }
            }

            //
            // Wait for at least 1 system tick to have gone by before we poll
            // the buttons again.
            //
            while(g_ui32SysTickCount == ui32LastTickCount)
            {
            }
        }
    }
}
