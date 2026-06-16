# PortGremlin

PortGremlin is firmware for the **EK-TM4C123GXL LaunchPad** (TM4C123GH6PM, ARM Cortex-M4F) that continuously changes its USB identity by dynamically switching USB device descriptors on the fly. It cycles through various USB device classes (HID keyboard, audio device, printer, gamepad, MIDI controller) while spoofing random VID/PID values every ~50ms. The objective is to confuse host OS USB stacks, break device-fingerprinting, and disrupt assumptions made by security software.

## Features

- Dynamic USB descriptor switching every ~50ms
- Randomized VID/PID spoofing (seeded on boot)
- Cycles through 5 device classes: Keyboard, Audio, Printer, Gamepad, MIDI
- Randomized serial number string per cycle
- UART debug output at 115200 baud
- SW1 button: cycle device type
- SW2 button: type test string

## Hardware

- **EK-TM4C123GXL** LaunchPad with TM4C123GH6PM
- USB connection to host PC via the Device USB port (USB micro-B)
- UART debug via the ICDI virtual COM port at 115200 baud

## Build Requirements

- **arm-none-eabi-gcc** (GNU Arm Embedded Toolchain)
- **TivaWare C Series 2.2.0.295** (driverlib + usblib)
- **lm4flash** or **OpenOCD** for flashing (optional)

## Building

```sh
# Set TivaWare path (adjust as needed)
export TIVAWARE_PATH=/opt/ti/TivaWare_C_Series-2.2.0.295

# Build
cd usb_dev_keyboard
make

# Flash
make flash
```

## How It Works

The firmware starts as a USB keyboard. A SysTick interrupt fires at 100 Hz; every 5 ticks (50ms) it:

1. Disconnects from USB
2. Randomizes the VID and PID of the current device
3. Re-initializes the USB device stack
4. Reconnects to the host
5. Advances to the next device class

The host sees a continuous stream of different USB devices being plugged in and out with different identifiers.

## License

MIT - see LICENSE file.

This project contains original application code. It depends on TI's TivaWare peripheral driver library and USB library, which are separately licensed by Texas Instruments.
