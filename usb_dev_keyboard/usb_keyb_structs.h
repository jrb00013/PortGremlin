#ifndef PORTGREMLIN_STRUCTS_H
#define PORTGREMLIN_STRUCTS_H

#include <stdint.h>
#include "usblib/device/usbdhidkeyb.h"

typedef enum {
    DEVICE_KEYBOARD = 0,
    DEVICE_AUDIO,
    DEVICE_PRINTER,
    DEVICE_MIDI,
    DEVICE_GAMEPAD,
    NUM_DEVICE_TYPES
} DeviceType;

typedef enum {
    VIDPID_TYPE_KEYBOARD,
    VIDPID_TYPE_AUDIO,
    VIDPID_TYPE_GAMEPAD,
    VIDPID_TYPE_MIDI,
    VIDPID_TYPE_PRINTER,
    VIDPID_TYPE_GENERIC
} VIDPIDDeviceType;

typedef struct {
    uint16_t ui16VID;
    uint16_t ui16PID;
    uint16_t ui16MaxPowermA;
    uint8_t  ui8PwrAttributes;
    uint32_t (*pfnHandler)(void *, uint32_t, uint32_t, void *);
    void     *pvCBData;
    const uint8_t * const *ppui8StringDescriptors;
    uint32_t ui32NumStringDescriptors;
} tUSBDHIDGamepadDevice;

typedef struct {
    uint16_t ui16VID;
    uint16_t ui16PID;
    uint16_t ui16MaxPowermA;
    uint8_t  ui8PwrAttributes;
    uint32_t (*pfnHandler)(void *, uint32_t, uint32_t, void *);
    void     *pvCBData;
    const uint8_t * const *ppui8StringDescriptors;
    uint32_t ui32NumStringDescriptors;
} tUSBAudioDevice;

typedef struct {
    uint16_t ui16VID;
    uint16_t ui16PID;
    uint16_t ui16MaxPowermA;
    uint8_t  ui8PwrAttributes;
    uint32_t (*pfnHandler)(void *, uint32_t, uint32_t, void *);
    void     *pvCBData;
    const uint8_t * const *ppui8StringDescriptors;
    uint32_t ui32NumStringDescriptors;
} tUSBPrinterDevice;

typedef struct {
    uint16_t ui16VID;
    uint16_t ui16PID;
    uint16_t ui16MaxPowermA;
    uint8_t  ui8PwrAttributes;
    uint32_t (*pfnHandler)(void *, uint32_t, uint32_t, void *);
    void     *pvCBData;
    const uint8_t * const *ppui8StringDescriptors;
    uint32_t ui32NumStringDescriptors;
} tUSBMIDIDevice;

extern DeviceType g_eCurrentDevice;
extern VIDPIDDeviceType g_eCurrentDeviceType;
extern void *g_pActiveDevice;

extern tUSBDHIDKeyboardDevice g_sKeyboardDevice;
extern tUSBDHIDKeyboardDevice g_sKeyboardTemplate;
extern tUSBDHIDGamepadDevice g_sGamepadDevice;
extern tUSBDHIDGamepadDevice g_sGamepadTemplate;
extern tUSBAudioDevice g_sAudioDevice;
extern tUSBAudioDevice g_sAudioTemplate;
extern tUSBPrinterDevice g_sPrinterDevice;
extern tUSBPrinterDevice g_sPrinterTemplate;
extern tUSBMIDIDevice g_sMIDIDevice;
extern tUSBMIDIDevice g_sMIDITemplate;

extern uint32_t KeyboardHandler(void *pvCBData, uint32_t ui32Event,
                                 uint32_t ui32MsgData, void *pvMsgData);
extern uint32_t AudioHandler(void *pvCBData, uint32_t ui32Event,
                              uint32_t ui32MsgParam, void *pvMsgData);
extern uint32_t GamepadHandler(void *pvCBData, uint32_t ui32Event,
                                uint32_t ui32MsgParam, void *pvMsgData);
extern uint32_t MIDIHandler(void *pvCBData, uint32_t ui32Event,
                             uint32_t ui32MsgParam, void *pvMsgData);
extern uint32_t PrinterHandler(void *pvCBData, uint32_t ui32Event,
                                uint32_t ui32MsgParam, void *pvMsgData);

extern const uint8_t * const g_ppui8StringDescriptorsKeyboard[];
extern const uint8_t * const g_ppui8StringDescriptorsAudio[];
extern const uint8_t * const g_ppui8StringDescriptorsGamepad[];
extern const uint8_t * const g_ppui8StringDescriptorsPrinter[];
extern const uint8_t * const g_ppui8StringDescriptorsMIDI[];

void SetSerialNumberString(uint32_t value);
void UsbKeybStructsInit(void);

#endif
