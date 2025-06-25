//*****************************************************************************
//
// usb_keyb_structs.h - Data structures defining the keyboard USB device.
//
// Copyright (c) 2008-2020 Texas Instruments Incorporated.  All rights reserved.
// Software License Agreement
// 
// Texas Instruments (TI) is supplying this software for use solely and
// exclusively on TI's microcontroller products. The software is owned by
// TI and/or its suppliers, and is protected under applicable copyright
// laws. You may not combine this software with "viral" open-source
// software in order to form a larger program.
// 
// THIS SOFTWARE IS PROVIDED "AS IS" AND WITH ALL FAULTS.
// NO WARRANTIES, WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT
// NOT LIMITED TO, IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE. TI SHALL NOT, UNDER ANY
// CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL, OR CONSEQUENTIAL
// DAMAGES, FOR ANY REASON WHATSOEVER.
// 
// This is part of revision 2.2.0.295 of the EK-TM4C123GXL Firmware Package.
//
//*****************************************************************************

#ifndef _USB_KEYB_STRUCTS_H_
#define _USB_KEYB_STRUCTS_H_

typedef enum {
    DEVICE_KEYBOARD = 0,
    DEVICE_AUDIO,
    DEVICE_PRINTER,
    DEVICE_MIDI,
    DEVICE_GAMEPAD,
    NUM_DEVICE_TYPES
} DeviceType;

// My HID Keyboard Struct Wrapper

typedef struct
{
    uint16_t ui16VID;
    uint16_t ui16PID;
    tUSBDHIDKeyboardDevice device;
} tMyKeyboardDevice;


// Gamepad Device Structure
typedef struct
{
    uint16_t ui16VID;   // Vendor ID
    uint16_t ui16PID;   // Product ID
    uint16_t ui16MaxPowermA;  // Max power in mA
    uint8_t  ui8PwrAttributes; // Power attributes (e.g. self-powered, remote wakeup)
    uint32_t (*pfnHandler)(void *pvCBData, uint32_t ui32Event, uint32_t ui32MsgParam, void *pvMsgData); // Event handler
    void *pvCBData; // Callback data (pointer to instance struct)
    const uint8_t * const *ppui8StringDescriptors; // Pointer to string descriptor table
    uint32_t ui32NumStringDescriptors; // Number of string descriptors
} tUSBDHIDGamepadDevice;


// Audio Device Structure
typedef struct
{
    uint16_t ui16VID;
    uint16_t ui16PID;
    uint16_t ui16MaxPowermA;
    uint8_t  ui8PwrAttributes;
    uint32_t (*pfnHandler)(void *pvCBData, uint32_t ui32Event, uint32_t ui32MsgParam, void *pvMsgData);
    void *pvCBData;
    const uint8_t * const *ppui8StringDescriptors;
    uint32_t ui32NumStringDescriptors;
} tUSBAudioDevice;


// Printer Device Structure
typedef struct
{
    uint16_t ui16VID;
    uint16_t ui16PID;
    uint16_t ui16MaxPowermA;
    uint8_t  ui8PwrAttributes;
    uint32_t (*pfnHandler)(void *pvCBData, uint32_t ui32Event, uint32_t ui32MsgParam, void *pvMsgData);
    void *pvCBData;
    const uint8_t * const *ppui8StringDescriptors;
    uint32_t ui32NumStringDescriptors;
} tUSBPrinterDevice;


// MIDI Device Structure
typedef struct
{
    uint16_t ui16VID;
    uint16_t ui16PID;
    uint16_t ui16MaxPowermA;
    uint8_t  ui8PwrAttributes;
    uint32_t  (*pfnHandler)(void *pvCBData, uint32_t ui32Event, uint32_t ui32MsgParam, void *pvMsgData);
    void *pvCBData;
    const uint8_t * const *ppui8StringDescriptors;
    uint32_t ui32NumStringDescriptors;
} tUSBMIDIDevice;


extern DeviceType g_eCurrentDevice;
extern void *g_pActiveDevice;


extern uint32_t KeyboardHandler(void *pvCBData,
                                     uint32_t ui32Event,
                                     uint32_t ui32MsgData,
                                     void *pvMsgData);

extern tUSBDHIDKeyboardDevice g_sKeyboardDevice;

extern tUSBDHIDKeyboardDevice g_sKeyboardTemplate;
extern tUSBAudioDevice g_sAudioTemplate;
extern tUSBPrinterDevice g_sPrinterTemplate;
extern tUSBMIDIDevice g_sMIDITemplate;
extern tUSBDHIDGamepadDevice g_sGamepadTemplate;


extern const uint8_t * const g_ppui8StringDescriptors[];
extern const uint8_t * const g_ppui8StringDescriptorsKeyboard[];


#endif
