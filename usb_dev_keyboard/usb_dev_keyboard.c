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

#define SYSTICKS_PER_SECOND     100
#define USB_RESUME_DURATION_MS  15
#define MAX_SEND_DELAY          50

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
    { 0, HID_KEYB_USAGE_ENTER },                       // LF 0x0A
};

volatile bool g_bConnected = false;
volatile bool g_bSuspended = false;
volatile uint32_t g_ui32SysTickCount;

typedef enum {
    STATE_UNCONFIGURED,
    STATE_IDLE,
    STATE_SENDING
} DeviceState;

volatile DeviceState g_eKeyboardState = STATE_UNCONFIGURED;
volatile DeviceState g_eAudioState = STATE_UNCONFIGURED;
volatile DeviceState g_eMidiState = STATE_UNCONFIGURED;
volatile DeviceState g_eGamepadState = STATE_UNCONFIGURED;
volatile DeviceState g_ePrinterState = STATE_UNCONFIGURED;

DeviceType g_eCurrentDevice = DEVICE_KEYBOARD;
VIDPIDDeviceType g_eCurrentDeviceType = VIDPID_TYPE_KEYBOARD;
void *g_pActiveDevice = NULL;

#ifdef DEBUG
void __error__(char *pcFilename, uint32_t ui32Line) { }
#endif

static void SendString(char *pcStr);
static bool WaitForSendIdle(uint_fast32_t ui32TimeoutTicks);

uint32_t KeyboardHandler(void *pvCBData, uint32_t ui32Event,
                          uint32_t ui32MsgData, void *pvMsgData)
{
    switch (ui32Event)
    {
        case USB_EVENT_CONNECTED:
            g_bConnected = true;
            g_bSuspended = false;
            break;

        case USB_EVENT_DISCONNECTED:
            g_bConnected = false;
            break;

        case USB_EVENT_TX_COMPLETE:
            g_eKeyboardState = STATE_IDLE;
            break;

        case USB_EVENT_SUSPEND:
            g_bSuspended = true;
            break;

        case USB_EVENT_RESUME:
            g_bSuspended = false;
            break;

        case USBD_HID_KEYB_EVENT_SET_LEDS:
            MAP_GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2,
                             (ui32MsgData & HID_KEYB_CAPS_LOCK) ? GPIO_PIN_2 : 0);
            break;

        default:
            break;
    }
    return 0;
}

uint32_t GamepadHandler(void *pvCBData, uint32_t ui32Event,
                         uint32_t ui32MsgParam, void *pvMsgData)
{
    switch (ui32Event)
    {
        case USB_EVENT_CONNECTED:
            g_bConnected = true;
            g_bSuspended = false;
            UARTprintf("Gamepad connected.\n");
            break;
        case USB_EVENT_DISCONNECTED:
            g_bConnected = false;
            UARTprintf("Gamepad disconnected.\n");
            break;
        case USB_EVENT_TX_COMPLETE:
            g_eGamepadState = STATE_IDLE;
            break;
        case USB_EVENT_SUSPEND:
            g_bSuspended = true;
            UARTprintf("Gamepad suspended.\n");
            break;
        case USB_EVENT_RESUME:
            g_bSuspended = false;
            UARTprintf("Gamepad resumed.\n");
            break;
        default:
            UARTprintf("Gamepad event: 0x%x\n", ui32Event);
            break;
    }
    return 0;
}

uint32_t AudioHandler(void *pvCBData, uint32_t ui32Event,
                       uint32_t ui32MsgParam, void *pvMsgData)
{
    switch (ui32Event)
    {
        case USB_EVENT_CONNECTED:
            g_bConnected = true;
            g_bSuspended = false;
            UARTprintf("Audio connected.\n");
            break;
        case USB_EVENT_DISCONNECTED:
            g_bConnected = false;
            UARTprintf("Audio disconnected.\n");
            break;
        case USB_EVENT_TX_COMPLETE:
            g_eAudioState = STATE_IDLE;
            break;
        case USB_EVENT_SUSPEND:
            g_bSuspended = true;
            UARTprintf("Audio suspended.\n");
            break;
        case USB_EVENT_RESUME:
            g_bSuspended = false;
            UARTprintf("Audio resumed.\n");
            break;
        default:
            UARTprintf("Audio event: 0x%x\n", ui32Event);
            break;
    }
    return 0;
}

uint32_t PrinterHandler(void *pvCBData, uint32_t ui32Event,
                         uint32_t ui32MsgParam, void *pvMsgData)
{
    switch (ui32Event)
    {
        case USB_EVENT_CONNECTED:
            g_bConnected = true;
            g_bSuspended = false;
            UARTprintf("Printer connected.\n");
            break;
        case USB_EVENT_DISCONNECTED:
            g_bConnected = false;
            UARTprintf("Printer disconnected.\n");
            break;
        case USB_EVENT_TX_COMPLETE:
            g_ePrinterState = STATE_IDLE;
            break;
        case USB_EVENT_SUSPEND:
            g_bSuspended = true;
            UARTprintf("Printer suspended.\n");
            break;
        case USB_EVENT_RESUME:
            g_bSuspended = false;
            UARTprintf("Printer resumed.\n");
            break;
        default:
            UARTprintf("Printer event: 0x%x\n", ui32Event);
            break;
    }
    return 0;
}

uint32_t MIDIHandler(void *pvCBData, uint32_t ui32Event,
                      uint32_t ui32MsgParam, void *pvMsgData)
{
    switch (ui32Event)
    {
        case USB_EVENT_CONNECTED:
            g_bConnected = true;
            g_bSuspended = false;
            UARTprintf("MIDI connected.\n");
            break;
        case USB_EVENT_DISCONNECTED:
            g_bConnected = false;
            UARTprintf("MIDI disconnected.\n");
            break;
        case USB_EVENT_TX_COMPLETE:
            g_eMidiState = STATE_IDLE;
            break;
        case USB_EVENT_SUSPEND:
            g_bSuspended = true;
            UARTprintf("MIDI suspended.\n");
            break;
        case USB_EVENT_RESUME:
            g_bSuspended = false;
            UARTprintf("MIDI resumed.\n");
            break;
        default:
            UARTprintf("MIDI event: 0x%x\n", ui32Event);
            break;
    }
    return 0;
}

static bool WaitForSendIdle(uint_fast32_t ui32TimeoutTicks)
{
    uint32_t ui32Start = g_ui32SysTickCount;
    uint32_t ui32Elapsed = 0;

    while (ui32Elapsed < ui32TimeoutTicks)
    {
        if (g_eKeyboardState == STATE_IDLE)
            return true;

        uint32_t ui32Now = g_ui32SysTickCount;
        ui32Elapsed = (ui32Start < ui32Now)
            ? (ui32Now - ui32Start)
            : (0xFFFFFFFF - ui32Start + ui32Now + 1);
    }
    return false;
}

static void SendString(char *pcStr)
{
    while (*pcStr)
    {
        uint32_t ui32Char = (uint32_t)*pcStr++;

        if ((ui32Char < ' ') || (ui32Char > '~'))
        {
            if (ui32Char != '\n')
                continue;
        }

        uint32_t tableIdx;
        if (ui32Char == '\n')
            tableIdx = 0x5f;
        else
            tableIdx = ui32Char - ' ';

        g_eKeyboardState = STATE_SENDING;
        if (USBDHIDKeyboardKeyStateChange((void *)&g_sKeyboardDevice,
                                           g_ppi8KeyUsageCodes[tableIdx][0],
                                           g_ppi8KeyUsageCodes[tableIdx][1],
                                           true) != KEYB_SUCCESS)
            return;

        if (!WaitForSendIdle(MAX_SEND_DELAY))
        {
            g_bConnected = false;
            return;
        }

        g_eKeyboardState = STATE_SENDING;
        if (USBDHIDKeyboardKeyStateChange((void *)&g_sKeyboardDevice,
                                           0, g_ppi8KeyUsageCodes[tableIdx][1],
                                           false) != KEYB_SUCCESS)
            return;

        if (!WaitForSendIdle(MAX_SEND_DELAY))
        {
            g_bConnected = false;
            return;
        }
    }
}

void ConfigureUART(void)
{
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
    MAP_GPIOPinConfigure(GPIO_PA0_U0RX);
    MAP_GPIOPinConfigure(GPIO_PA1_U0TX);
    MAP_GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);
    UARTClockSourceSet(UART0_BASE, UART_CLOCK_PIOSC);
    UARTStdioConfig(0, 115200, 16000000);
}

void PrepareDevice(VIDPIDDeviceType type)
{
    uint16_t vid = (uint16_t)(0x1000 + (rand() % 0xEFFF));
    uint16_t pid = (uint16_t)(0x1000 + (rand() % 0xEFFF));

    switch (type)
    {
        case VIDPID_TYPE_KEYBOARD:
        {
            static uint8_t s_DeviceBuffer[sizeof(tUSBDHIDKeyboardDevice)];
            memcpy(s_DeviceBuffer, &g_sKeyboardTemplate, sizeof(g_sKeyboardTemplate));
            uint16_t *pVID = (uint16_t *)(s_DeviceBuffer + offsetof(tUSBDHIDKeyboardDevice, ui16VID));
            uint16_t *pPID = (uint16_t *)(s_DeviceBuffer + offsetof(tUSBDHIDKeyboardDevice, ui16PID));
            *pVID = vid;
            *pPID = pid;
            USBDHIDKeyboardInit(0, (tUSBDHIDKeyboardDevice *)s_DeviceBuffer);
            break;
        }

        case VIDPID_TYPE_AUDIO:
        {
            static tUSBAudioDevice s_AudioDevice;
            s_AudioDevice = g_sAudioTemplate;
            s_AudioDevice.ui16VID = vid;
            s_AudioDevice.ui16PID = pid;
            USBAudioInit(0, &s_AudioDevice);
            break;
        }

        case VIDPID_TYPE_MIDI:
        {
            static tUSBMIDIDevice s_MIDIDevice;
            s_MIDIDevice = g_sMIDITemplate;
            s_MIDIDevice.ui16VID = vid;
            s_MIDIDevice.ui16PID = pid;
            USBMIDIInit(0, &s_MIDIDevice);
            break;
        }

        case VIDPID_TYPE_PRINTER:
        {
            static tUSBPrinterDevice s_PrinterDevice;
            s_PrinterDevice = g_sPrinterTemplate;
            s_PrinterDevice.ui16VID = vid;
            s_PrinterDevice.ui16PID = pid;
            USBPrinterInit(0, &s_PrinterDevice);
            break;
        }

        default:
            UARTprintf("Unknown device type in PrepareDevice.\n");
            break;
    }
}

void RandomizeVIDPID(void *pDevice, VIDPIDDeviceType type)
{
    uint16_t vid = (uint16_t)(0x1000 + (rand() % 0xEFFF));
    uint16_t pid = (uint16_t)(0x1000 + (rand() % 0xEFFF));

    switch (type)
    {
        case VIDPID_TYPE_KEYBOARD:
        {
            tUSBDHIDKeyboardDevice *pKb = (tUSBDHIDKeyboardDevice *)pDevice;
            pKb->ui16VID = vid;
            pKb->ui16PID = pid;
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

        case VIDPID_TYPE_GAMEPAD:
        {
            tUSBDHIDGamepadDevice *pPad = (tUSBDHIDGamepadDevice *)pDevice;
            pPad->ui16VID = vid;
            pPad->ui16PID = pid;
            break;
        }

        case VIDPID_TYPE_GENERIC:
            break;

        default:
            UARTprintf("Unknown device type in RandomizeVIDPID.\n");
            break;
    }
}

void USBAudioDeviceInit(uint32_t ui32Index, tUSBAudioDevice *pDevice)
{
    USBDCDInit(ui32Index, (tDeviceInfo *)pDevice, pDevice);
}

void USBPrinterDeviceInit(uint32_t ui32Index, tUSBPrinterDevice *pDevice)
{
    USBDCDInit(ui32Index, (tDeviceInfo *)pDevice, pDevice);
}

void USBMIDIDeviceInit(uint32_t ui32Index, tUSBMIDIDevice *pDevice)
{
    USBDCDInit(ui32Index, (tDeviceInfo *)pDevice, pDevice);
}

void USBAudioInit(uint32_t ui32Index, tUSBAudioDevice *pDevice)
{
    USBAudioDeviceInit(ui32Index, pDevice);
}

void USBDHIDGamepadInit(uint32_t ui32Index, tUSBDHIDGamepadDevice *pDevice)
{
    USBDHIDInit(ui32Index, (tUSBDHIDDevice *)pDevice);
}

void USBPrinterInit(uint32_t ui32Index, tUSBPrinterDevice *pDevice)
{
    USBPrinterDeviceInit(ui32Index, pDevice);
}

void USBMIDIInit(uint32_t ui32Index, tUSBMIDIDevice *pDevice)
{
    USBMIDIDeviceInit(ui32Index, pDevice);
}

void USBRemoteWakeup(void)
{
    if (g_bSuspended)
    {
        USBDCDRemoteWakeupRequest(0);
        UARTprintf("Sent USB remote wake-up signal\n\r");
    }
}

void USBDeviceRemoteWakeupRequest(void *pDevice)
{
    (void)pDevice;
    USBRemoteWakeup();
}

void USBGamepadRemoteWakeupRequest(void *pDevice)
{
    USBDeviceRemoteWakeupRequest(pDevice);
}

void USBAudioRemoteWakeupRequest(void *pDevice)
{
    USBDeviceRemoteWakeupRequest(pDevice);
}

void USBMIDIRemoteWakeupRequest(void *pDevice)
{
    USBDeviceRemoteWakeupRequest(pDevice);
}

void USBPrinterRemoteWakeupRequest(void *pDevice)
{
    USBDeviceRemoteWakeupRequest(pDevice);
}

void ReenumerateWithRandomVIDPID(VIDPIDDeviceType deviceType)
{
    UARTprintf("Re-enumerating USB with new VID/PID...\n\r");

    USBDevDisconnect(USB0_BASE);
    SysCtlDelay(SysCtlClockGet() / 3);

    switch (deviceType)
    {
        case VIDPID_TYPE_KEYBOARD:
            USBDHIDKeyboardTerm(&g_sKeyboardDevice);
            RandomizeVIDPID(&g_sKeyboardDevice, VIDPID_TYPE_KEYBOARD);
            UARTprintf("New VID: 0x%04X, PID: 0x%04X\n\r",
                g_sKeyboardDevice.ui16VID, g_sKeyboardDevice.ui16PID);
            USBDHIDKeyboardInit(0, &g_sKeyboardDevice);
            break;

        case VIDPID_TYPE_AUDIO:
            RandomizeVIDPID(&g_sAudioDevice, VIDPID_TYPE_AUDIO);
            UARTprintf("New VID: 0x%04X, PID: 0x%04X\n\r",
                g_sAudioDevice.ui16VID, g_sAudioDevice.ui16PID);
            USBAudioInit(0, &g_sAudioDevice);
            break;

        case VIDPID_TYPE_GAMEPAD:
            RandomizeVIDPID(&g_sGamepadDevice, VIDPID_TYPE_GAMEPAD);
            UARTprintf("New VID: 0x%04X, PID: 0x%04X\n\r",
                g_sGamepadDevice.ui16VID, g_sGamepadDevice.ui16PID);
            USBDHIDGamepadInit(0, &g_sGamepadDevice);
            break;

        case VIDPID_TYPE_MIDI:
            RandomizeVIDPID(&g_sMIDIDevice, VIDPID_TYPE_MIDI);
            UARTprintf("New VID: 0x%04X, PID: 0x%04X\n\r",
                g_sMIDIDevice.ui16VID, g_sMIDIDevice.ui16PID);
            USBMIDIInit(0, &g_sMIDIDevice);
            break;

        case VIDPID_TYPE_PRINTER:
            RandomizeVIDPID(&g_sPrinterDevice, VIDPID_TYPE_PRINTER);
            UARTprintf("New VID: 0x%04X, PID: 0x%04X\n\r",
                g_sPrinterDevice.ui16VID, g_sPrinterDevice.ui16PID);
            USBPrinterInit(0, &g_sPrinterDevice);
            break;

        default:
            UARTprintf("Unknown device type for re-enumeration.\n\r");
            break;
    }

    USBDevConnect(USB0_BASE);
}

void CycleDeviceType(void)
{
    USBDevDisconnect(USB0_BASE);
    SysCtlDelay(SysCtlClockGet() / 3);

    g_eCurrentDevice = (DeviceType)(((int)g_eCurrentDevice + 1) % (int)NUM_DEVICE_TYPES);
    SetSerialNumberString((uint32_t)(rand() ^ (rand() << 16)));

    switch (g_eCurrentDevice)
    {
        case DEVICE_KEYBOARD:
            UARTprintf("Switching to Keyboard...\n");
            g_sKeyboardDevice = g_sKeyboardTemplate;
            g_pActiveDevice = &g_sKeyboardDevice;
            RandomizeVIDPID(&g_sKeyboardDevice, VIDPID_TYPE_KEYBOARD);
            g_eCurrentDeviceType = VIDPID_TYPE_KEYBOARD;
            USBDHIDKeyboardInit(0, &g_sKeyboardDevice);
            break;

        case DEVICE_AUDIO:
            UARTprintf("Switching to Audio...\n");
            g_sAudioDevice = g_sAudioTemplate;
            g_pActiveDevice = &g_sAudioDevice;
            RandomizeVIDPID(&g_sAudioDevice, VIDPID_TYPE_AUDIO);
            g_eCurrentDeviceType = VIDPID_TYPE_AUDIO;
            USBAudioInit(0, &g_sAudioDevice);
            break;

        case DEVICE_PRINTER:
            UARTprintf("Switching to Printer...\n");
            g_sPrinterDevice = g_sPrinterTemplate;
            g_pActiveDevice = &g_sPrinterDevice;
            RandomizeVIDPID(&g_sPrinterDevice, VIDPID_TYPE_PRINTER);
            g_eCurrentDeviceType = VIDPID_TYPE_PRINTER;
            USBPrinterInit(0, &g_sPrinterDevice);
            break;

        case DEVICE_MIDI:
            UARTprintf("Switching to MIDI...\n");
            g_sMIDIDevice = g_sMIDITemplate;
            g_pActiveDevice = &g_sMIDIDevice;
            RandomizeVIDPID(&g_sMIDIDevice, VIDPID_TYPE_MIDI);
            g_eCurrentDeviceType = VIDPID_TYPE_MIDI;
            USBMIDIInit(0, &g_sMIDIDevice);
            break;

        case DEVICE_GAMEPAD:
            UARTprintf("Switching to Gamepad...\n");
            g_sGamepadDevice = g_sGamepadTemplate;
            g_pActiveDevice = &g_sGamepadDevice;
            RandomizeVIDPID(&g_sGamepadDevice, VIDPID_TYPE_GAMEPAD);
            g_eCurrentDeviceType = VIDPID_TYPE_GAMEPAD;
            USBDHIDGamepadInit(0, &g_sGamepadDevice);
            break;
    }

    USBDevConnect(USB0_BASE);
}

void SysTickIntHandler(void)
{
    static uint32_t tickCounter = 0;
    g_ui32SysTickCount++;

    tickCounter++;
    if (tickCounter >= 5)
    {
        tickCounter = 0;

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
                break;
        }

        g_eCurrentDevice = (DeviceType)(((int)g_eCurrentDevice + 1) % (int)NUM_DEVICE_TYPES);
    }
}

int main(void)
{
    MAP_FPULazyStackingEnable();
    MAP_SysCtlClockSet(SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN |
                       SYSCTL_XTAL_16MHZ);

    srand(MAP_SysCtlClockGet());

    ConfigureUART();
    UARTprintf("PortGremlin - USB Identity Spoofer\n\r");

    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);
    MAP_GPIOPinTypeUSBAnalog(GPIO_PORTD_BASE, GPIO_PIN_4 | GPIO_PIN_5);

    if (CLASS_IS_TM4C123 && REVISION_IS_A1)
    {
        HWREG(GPIO_PORTB_BASE + GPIO_O_PDR) |= GPIO_PIN_1;
    }

    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
    MAP_GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_2);
    MAP_GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, 0);

    ButtonsInit();

    g_bConnected = false;
    g_bSuspended = false;
    bool bLastSuspend = false;

    USBStackModeSet(0, eUSBModeForceDevice, 0);

    g_sKeyboardDevice = g_sKeyboardTemplate;
    g_sKeyboardDevice.sPrivateData.sHIDDevice.ppui8StringDescriptors = g_ppui8StringDescriptorsKeyboard;
    g_eCurrentDevice = DEVICE_KEYBOARD;
    g_eCurrentDeviceType = VIDPID_TYPE_KEYBOARD;
    g_pActiveDevice = &g_sKeyboardDevice;
    RandomizeVIDPID(&g_sKeyboardDevice, VIDPID_TYPE_KEYBOARD);
    SetSerialNumberString((uint32_t)(rand() ^ (rand() << 16)));
    USBDHIDKeyboardInit(0, &g_sKeyboardDevice);

    MAP_SysTickPeriodSet(MAP_SysCtlClockGet() / SYSTICKS_PER_SECOND);
    MAP_SysTickIntEnable();
    MAP_SysTickEnable();

    while (1)
    {
        uint8_t ui8Buttons;
        uint8_t ui8ButtonsChanged;

        UARTprintf("Waiting for host...\n\r");

        while (!g_bConnected) { }

        UARTprintf("Host connected.\n\r");

        g_eKeyboardState = STATE_IDLE;
        bLastSuspend = false;

        while (g_bConnected)
        {
            MAP_GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, GPIO_PIN_2);
            uint32_t ui32LastTickCount = g_ui32SysTickCount;

            if (bLastSuspend != g_bSuspended)
            {
                bLastSuspend = g_bSuspended;
                UARTprintf(bLastSuspend ? "Bus suspended...\n\r" : "Host connected...\n\r");
            }

            ui8Buttons = ButtonsPoll(&ui8ButtonsChanged, 0);
            if (BUTTON_PRESSED(LEFT_BUTTON, ui8Buttons, ui8ButtonsChanged))
            {
                if (g_bSuspended)
                {
                    switch (g_eCurrentDeviceType)
                    {
                        case VIDPID_TYPE_KEYBOARD:
                            USBDHIDKeyboardRemoteWakeupRequest(&g_sKeyboardDevice);
                            break;
                        case VIDPID_TYPE_AUDIO:
                            USBAudioRemoteWakeupRequest(&g_sAudioDevice);
                            break;
                        case VIDPID_TYPE_GAMEPAD:
                            USBGamepadRemoteWakeupRequest(&g_sGamepadDevice);
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
            else if (BUTTON_PRESSED(RIGHT_BUTTON, ui8Buttons, ui8ButtonsChanged))
            {
                if (g_bSuspended)
                    USBDHIDKeyboardRemoteWakeupRequest((void *)&g_sKeyboardDevice);
                else
                    SendString("You have pressed the SW2 button.\n"
                               "Try pressing the Caps Lock key on your "
                               "keyboard and then press either button.\n\n");
            }

            while (g_ui32SysTickCount == ui32LastTickCount) { }
        }
    }
}
