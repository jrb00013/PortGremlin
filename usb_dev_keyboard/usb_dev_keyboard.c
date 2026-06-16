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
    { 0, HID_KEYB_USAGE_SPACE },
    { HID_KEYB_LEFT_SHIFT, HID_KEYB_USAGE_1 },
    { HID_KEYB_LEFT_SHIFT, HID_KEYB_USAGE_FQUOTE },
    { HID_KEYB_LEFT_SHIFT, HID_KEYB_USAGE_3 },
    { HID_KEYB_LEFT_SHIFT, HID_KEYB_USAGE_4 },
    { HID_KEYB_LEFT_SHIFT, HID_KEYB_USAGE_5 },
    { HID_KEYB_LEFT_SHIFT, HID_KEYB_USAGE_7 },
    { 0, HID_KEYB_USAGE_FQUOTE },
    { HID_KEYB_LEFT_SHIFT, HID_KEYB_USAGE_9 },
    { HID_KEYB_LEFT_SHIFT, HID_KEYB_USAGE_0 },
    { HID_KEYB_LEFT_SHIFT, HID_KEYB_USAGE_8 },
    { HID_KEYB_LEFT_SHIFT, HID_KEYB_USAGE_EQUAL },
    { 0, HID_KEYB_USAGE_COMMA },
    { 0, HID_KEYB_USAGE_MINUS },
    { 0, HID_KEYB_USAGE_PERIOD },
    { 0, HID_KEYB_USAGE_FSLASH },
    { 0, HID_KEYB_USAGE_0 },
    { 0, HID_KEYB_USAGE_1 },
    { 0, HID_KEYB_USAGE_2 },
    { 0, HID_KEYB_USAGE_3 },
    { 0, HID_KEYB_USAGE_4 },
    { 0, HID_KEYB_USAGE_5 },
    { 0, HID_KEYB_USAGE_6 },
    { 0, HID_KEYB_USAGE_7 },
    { 0, HID_KEYB_USAGE_8 },
    { 0, HID_KEYB_USAGE_9 },
    { HID_KEYB_LEFT_SHIFT, HID_KEYB_USAGE_SEMICOLON },
    { 0, HID_KEYB_USAGE_SEMICOLON },
    { HID_KEYB_LEFT_SHIFT, HID_KEYB_USAGE_COMMA },
    { 0, HID_KEYB_USAGE_EQUAL },
    { HID_KEYB_LEFT_SHIFT, HID_KEYB_USAGE_PERIOD },
    { HID_KEYB_LEFT_SHIFT, HID_KEYB_USAGE_FSLASH },
    { HID_KEYB_LEFT_SHIFT, HID_KEYB_USAGE_2 },
    { HID_KEYB_LEFT_SHIFT, HID_KEYB_USAGE_A },
    { HID_KEYB_LEFT_SHIFT, HID_KEYB_USAGE_B },
    { HID_KEYB_LEFT_SHIFT, HID_KEYB_USAGE_C },
    { HID_KEYB_LEFT_SHIFT, HID_KEYB_USAGE_D },
    { HID_KEYB_LEFT_SHIFT, HID_KEYB_USAGE_E },
    { HID_KEYB_LEFT_SHIFT, HID_KEYB_USAGE_F },
    { HID_KEYB_LEFT_SHIFT, HID_KEYB_USAGE_G },
    { HID_KEYB_LEFT_SHIFT, HID_KEYB_USAGE_H },
    { HID_KEYB_LEFT_SHIFT, HID_KEYB_USAGE_I },
    { HID_KEYB_LEFT_SHIFT, HID_KEYB_USAGE_J },
    { HID_KEYB_LEFT_SHIFT, HID_KEYB_USAGE_K },
    { HID_KEYB_LEFT_SHIFT, HID_KEYB_USAGE_L },
    { HID_KEYB_LEFT_SHIFT, HID_KEYB_USAGE_M },
    { HID_KEYB_LEFT_SHIFT, HID_KEYB_USAGE_N },
    { HID_KEYB_LEFT_SHIFT, HID_KEYB_USAGE_O },
    { HID_KEYB_LEFT_SHIFT, HID_KEYB_USAGE_P },
    { HID_KEYB_LEFT_SHIFT, HID_KEYB_USAGE_Q },
    { HID_KEYB_LEFT_SHIFT, HID_KEYB_USAGE_R },
    { HID_KEYB_LEFT_SHIFT, HID_KEYB_USAGE_S },
    { HID_KEYB_LEFT_SHIFT, HID_KEYB_USAGE_T },
    { HID_KEYB_LEFT_SHIFT, HID_KEYB_USAGE_U },
    { HID_KEYB_LEFT_SHIFT, HID_KEYB_USAGE_V },
    { HID_KEYB_LEFT_SHIFT, HID_KEYB_USAGE_W },
    { HID_KEYB_LEFT_SHIFT, HID_KEYB_USAGE_X },
    { HID_KEYB_LEFT_SHIFT, HID_KEYB_USAGE_Y },
    { HID_KEYB_LEFT_SHIFT, HID_KEYB_USAGE_Z },
    { 0, HID_KEYB_USAGE_LBRACKET },
    { 0, HID_KEYB_USAGE_BSLASH },
    { 0, HID_KEYB_USAGE_RBRACKET },
    { HID_KEYB_LEFT_SHIFT, HID_KEYB_USAGE_6 },
    { HID_KEYB_LEFT_SHIFT, HID_KEYB_USAGE_MINUS },
    { 0, HID_KEYB_USAGE_BQUOTE },
    { 0, HID_KEYB_USAGE_A },
    { 0, HID_KEYB_USAGE_B },
    { 0, HID_KEYB_USAGE_C },
    { 0, HID_KEYB_USAGE_D },
    { 0, HID_KEYB_USAGE_E },
    { 0, HID_KEYB_USAGE_F },
    { 0, HID_KEYB_USAGE_G },
    { 0, HID_KEYB_USAGE_H },
    { 0, HID_KEYB_USAGE_I },
    { 0, HID_KEYB_USAGE_J },
    { 0, HID_KEYB_USAGE_K },
    { 0, HID_KEYB_USAGE_L },
    { 0, HID_KEYB_USAGE_M },
    { 0, HID_KEYB_USAGE_N },
    { 0, HID_KEYB_USAGE_O },
    { 0, HID_KEYB_USAGE_P },
    { 0, HID_KEYB_USAGE_Q },
    { 0, HID_KEYB_USAGE_R },
    { 0, HID_KEYB_USAGE_S },
    { 0, HID_KEYB_USAGE_T },
    { 0, HID_KEYB_USAGE_U },
    { 0, HID_KEYB_USAGE_V },
    { 0, HID_KEYB_USAGE_W },
    { 0, HID_KEYB_USAGE_X },
    { 0, HID_KEYB_USAGE_Y },
    { 0, HID_KEYB_USAGE_Z },
    { HID_KEYB_LEFT_SHIFT, HID_KEYB_USAGE_LBRACKET },
    { HID_KEYB_LEFT_SHIFT, HID_KEYB_USAGE_BSLASH },
    { HID_KEYB_LEFT_SHIFT, HID_KEYB_USAGE_RBRACKET },
    { HID_KEYB_LEFT_SHIFT, HID_KEYB_USAGE_BQUOTE },
    { 0, HID_KEYB_USAGE_ENTER },
};

typedef enum {
    STATE_UNCONFIGURED,
    STATE_IDLE,
    STATE_SENDING
} DeviceState;

static volatile bool g_bConnected = false;
static volatile bool g_bSuspended = false;
static volatile uint32_t g_ui32SysTickCount;
static volatile DeviceState g_eKeyboardState = STATE_UNCONFIGURED;
static volatile DeviceState g_eAudioState = STATE_UNCONFIGURED;
static volatile DeviceState g_eMidiState = STATE_UNCONFIGURED;
static volatile DeviceState g_eGamepadState = STATE_UNCONFIGURED;
static volatile DeviceState g_ePrinterState = STATE_UNCONFIGURED;

DeviceType g_eCurrentDevice = DEVICE_KEYBOARD;
void *g_pActiveDevice = NULL;

#ifdef DEBUG
void __error__(char *pcFilename, uint32_t ui32Line) { }
#endif

static void SendString(char *pcStr);
static void CycleDeviceType(void);
static void ReenumerateWithRandomVIDPID(void);
static void RandomizeVIDPID(void *pDevice, DeviceType type);
static void ConfigureUART(void);
static void USBRemoteWakeup(void);
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
    while (1)
    {
        uint32_t ui32Now = g_ui32SysTickCount;
        uint32_t ui32Elapsed = (ui32Start < ui32Now)
            ? (ui32Now - ui32Start)
            : (0xFFFFFFFF - ui32Start + ui32Now + 1);

        if (g_eKeyboardState == STATE_IDLE)
            return true;
        if (ui32Elapsed >= ui32TimeoutTicks)
            return false;
    }
}

static void SendString(char *pcStr)
{
    while (*pcStr)
    {
        uint32_t ui32Char = (uint32_t)*pcStr++;

        if (ui32Char < ' ' || ui32Char > '~')
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

static void ConfigureUART(void)
{
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
    MAP_GPIOPinConfigure(GPIO_PA0_U0RX);
    MAP_GPIOPinConfigure(GPIO_PA1_U0TX);
    MAP_GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);
    UARTClockSourceSet(UART0_BASE, UART_CLOCK_PIOSC);
    UARTStdioConfig(0, 115200, 16000000);
}

static void RandomizeVIDPID(void *pDevice, DeviceType type)
{
    uint16_t vid = (uint16_t)(0x1000 + (rand() % 0xEFFF));
    uint16_t pid = (uint16_t)(0x1000 + (rand() % 0xEFFF));

    switch (type)
    {
        case DEVICE_KEYBOARD:
            ((tUSBDHIDKeyboardDevice *)pDevice)->ui16VID = vid;
            ((tUSBDHIDKeyboardDevice *)pDevice)->ui16PID = pid;
            break;
        case DEVICE_AUDIO:
            ((tUSBAudioDevice *)pDevice)->ui16VID = vid;
            ((tUSBAudioDevice *)pDevice)->ui16PID = pid;
            break;
        case DEVICE_GAMEPAD:
            ((tUSBDHIDGamepadDevice *)pDevice)->ui16VID = vid;
            ((tUSBDHIDGamepadDevice *)pDevice)->ui16PID = pid;
            break;
        case DEVICE_MIDI:
            ((tUSBMIDIDevice *)pDevice)->ui16VID = vid;
            ((tUSBMIDIDevice *)pDevice)->ui16PID = pid;
            break;
        case DEVICE_PRINTER:
            ((tUSBPrinterDevice *)pDevice)->ui16VID = vid;
            ((tUSBPrinterDevice *)pDevice)->ui16PID = pid;
            break;
        default:
            break;
    }
}

static void USBRemoteWakeup(void)
{
    if (g_bSuspended)
    {
        USBDCDRemoteWakeupRequest(0);
        UARTprintf("Sent remote wake-up signal\n\r");
    }
}

static void ReenumerateWithRandomVIDPID(void)
{
    UARTprintf("Re-enumerating USB with new VID/PID...\n\r");

    USBDevDisconnect(USB0_BASE);
    SysCtlDelay(SysCtlClockGet() / 3);

    switch (g_eCurrentDevice)
    {
        case DEVICE_KEYBOARD:
            USBDHIDKeyboardTerm(&g_sKeyboardDevice);
            RandomizeVIDPID(&g_sKeyboardDevice, DEVICE_KEYBOARD);
            UARTprintf("New VID: 0x%04X, PID: 0x%04X\n\r",
                g_sKeyboardDevice.ui16VID, g_sKeyboardDevice.ui16PID);
            USBDHIDKeyboardInit(0, &g_sKeyboardDevice);
            break;

        case DEVICE_AUDIO:
            RandomizeVIDPID(&g_sAudioDevice, DEVICE_AUDIO);
            UARTprintf("New VID: 0x%04X, PID: 0x%04X\n\r",
                g_sAudioDevice.ui16VID, g_sAudioDevice.ui16PID);
            g_sAudioDevice.pfnHandler = AudioHandler;
            g_sAudioDevice.pvCBData = (void *)&g_sAudioDevice;
            USBDCDInit(0, (tDeviceInfo *)&g_sAudioDevice, &g_sAudioDevice);
            break;

        case DEVICE_GAMEPAD:
            RandomizeVIDPID(&g_sGamepadDevice, DEVICE_GAMEPAD);
            UARTprintf("New VID: 0x%04X, PID: 0x%04X\n\r",
                g_sGamepadDevice.ui16VID, g_sGamepadDevice.ui16PID);
            g_sGamepadDevice.pfnHandler = GamepadHandler;
            g_sGamepadDevice.pvCBData = (void *)&g_sGamepadDevice;
            USBDHIDInit(0, (tUSBDHIDDevice *)&g_sGamepadDevice);
            break;

        case DEVICE_MIDI:
            RandomizeVIDPID(&g_sMIDIDevice, DEVICE_MIDI);
            UARTprintf("New VID: 0x%04X, PID: 0x%04X\n\r",
                g_sMIDIDevice.ui16VID, g_sMIDIDevice.ui16PID);
            g_sMIDIDevice.pfnHandler = MIDIHandler;
            g_sMIDIDevice.pvCBData = (void *)&g_sMIDIDevice;
            USBDCDInit(0, (tDeviceInfo *)&g_sMIDIDevice, &g_sMIDIDevice);
            break;

        case DEVICE_PRINTER:
            RandomizeVIDPID(&g_sPrinterDevice, DEVICE_PRINTER);
            UARTprintf("New VID: 0x%04X, PID: 0x%04X\n\r",
                g_sPrinterDevice.ui16VID, g_sPrinterDevice.ui16PID);
            g_sPrinterDevice.pfnHandler = PrinterHandler;
            g_sPrinterDevice.pvCBData = (void *)&g_sPrinterDevice;
            USBDCDInit(0, (tDeviceInfo *)&g_sPrinterDevice, &g_sPrinterDevice);
            break;
    }

    USBDevConnect(USB0_BASE);
}

static void CycleDeviceType(void)
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
            RandomizeVIDPID(&g_sKeyboardDevice, DEVICE_KEYBOARD);
            USBDHIDKeyboardInit(0, &g_sKeyboardDevice);
            break;

        case DEVICE_AUDIO:
            UARTprintf("Switching to Audio...\n");
            g_sAudioDevice = g_sAudioTemplate;
            g_pActiveDevice = &g_sAudioDevice;
            RandomizeVIDPID(&g_sAudioDevice, DEVICE_AUDIO);
            USBDCDInit(0, (tDeviceInfo *)&g_sAudioDevice, &g_sAudioDevice);
            break;

        case DEVICE_PRINTER:
            UARTprintf("Switching to Printer...\n");
            g_sPrinterDevice = g_sPrinterTemplate;
            g_pActiveDevice = &g_sPrinterDevice;
            RandomizeVIDPID(&g_sPrinterDevice, DEVICE_PRINTER);
            USBDCDInit(0, (tDeviceInfo *)&g_sPrinterDevice, &g_sPrinterDevice);
            break;

        case DEVICE_MIDI:
            UARTprintf("Switching to MIDI...\n");
            g_sMIDIDevice = g_sMIDITemplate;
            g_pActiveDevice = &g_sMIDIDevice;
            RandomizeVIDPID(&g_sMIDIDevice, DEVICE_MIDI);
            USBDCDInit(0, (tDeviceInfo *)&g_sMIDIDevice, &g_sMIDIDevice);
            break;

        case DEVICE_GAMEPAD:
            UARTprintf("Switching to Gamepad...\n");
            g_sGamepadDevice = g_sGamepadTemplate;
            g_pActiveDevice = &g_sGamepadDevice;
            RandomizeVIDPID(&g_sGamepadDevice, DEVICE_GAMEPAD);
            USBDHIDInit(0, (tUSBDHIDDevice *)&g_sGamepadDevice);
            break;
    }

    USBDevConnect(USB0_BASE);
}

void SysTick_Handler(void)
{
    static uint32_t tickCounter = 0;
    g_ui32SysTickCount++;

    tickCounter++;
    if (tickCounter >= 5)
    {
        tickCounter = 0;
        ReenumerateWithRandomVIDPID();
        g_eCurrentDevice = (DeviceType)(((int)g_eCurrentDevice + 1) % (int)NUM_DEVICE_TYPES);
    }
}

void UART0_Handler(void)
{
    UARTStdioIntHandler();
}

void USB0_Handler(void)
{
    USB0DeviceIntHandler();
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
    g_eCurrentDevice = DEVICE_KEYBOARD;
    g_pActiveDevice = &g_sKeyboardDevice;
    RandomizeVIDPID(&g_sKeyboardDevice, DEVICE_KEYBOARD);
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
                    switch (g_eCurrentDevice)
                    {
                        case DEVICE_KEYBOARD:
                            USBDHIDKeyboardRemoteWakeupRequest(&g_sKeyboardDevice);
                            break;
                        case DEVICE_AUDIO:
                            USBRemoteWakeup();
                            break;
                        case DEVICE_GAMEPAD:
                            USBRemoteWakeup();
                            break;
                        case DEVICE_MIDI:
                            USBRemoteWakeup();
                            break;
                        case DEVICE_PRINTER:
                            USBRemoteWakeup();
                            break;
                    }
                }
                else
                {
                    SendString("PortGremlin: cycling device descriptors.\n");
                    CycleDeviceType();
                }
            }
            else if (BUTTON_PRESSED(RIGHT_BUTTON, ui8Buttons, ui8ButtonsChanged))
            {
                if (g_bSuspended)
                    USBDHIDKeyboardRemoteWakeupRequest((void *)&g_sKeyboardDevice);
                else
                    SendString("PortGremlin active. Press SW1 to cycle.\n");
            }

            while (g_ui32SysTickCount == ui32LastTickCount) { }
        }
    }
}
