# PortGremlin

PortGremlin is a USB device firmware based off the usb_dev_keyboard template by TI's Tiva C series electronics, designed to continuously change its USB identity by dynamically switching USB device descriptors on the fly. It cycles through various USB device classes such as HID keyboard, audio device, printer, gamepad, and MIDI controller while spoofing random Vendor ID (VID) and Product ID (PID) values. The objective is to confuse the host operating system by flooding it with fake USB devices, breaking assumptions made by security software, autorun logic, or device fingerprinting techniques.

- Dynamic USB descriptor switching  
- Randomized VID/PID spoofing  
- Cycling through multiple device classes every few seconds  
- Optional malformed or poisoned descriptors to induce driver instability  
- Targets OS-level assumptions without injecting keystrokes or data  

---

## Hardware / Components

- **EK-TM4C123GXL** — Main MCU with built-in USB Device controller  
- **USB-A Female Connector** — To connect the device to a PC  
- **Optional:** USB OTG cable or breakout board (depending on your testing setup)  
- **Optional:** RGB LEDs or small display for visual feedback of current device mode or VID/PID  
---
## Tools 
- **TI Code Composer Studio (CCS)** — Development environment  
- **TivaWare USB Library** — TI’s USB device stack and example descriptors  
- USB descriptor editing tools or online descriptor builders  
- **Wireshark** with USB capture support for debugging  
- **Theyscon USB Descriptor Dumper** (optional)  
- **Python** — For generating random VID/PID and descriptor payloads dynamically  
- A host PC running Windows, Linux, or macOS for device testing  

