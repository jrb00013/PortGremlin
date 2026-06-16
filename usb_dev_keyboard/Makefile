TIVAWARE_PATH ?= /opt/ti/TivaWare_C_Series-2.2.0.295

PROJECT := portgremlin

CC := arm-none-eabi-gcc
LD := arm-none-eabi-gcc
OBJCOPY := arm-none-eabi-objcopy
SIZE := arm-none-eabi-size
GDB := arm-none-eabi-gdb

MCFLAGS := -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16

DEFS := -DPART_TM4C123GH6PM -DTARGET_IS_TM4C123_RB1 -DUART_BUFFERED

INCLUDES := -I$(TIVAWARE_PATH) -I.

CFLAGS := $(MCFLAGS) $(DEFS) $(INCLUDES)
CFLAGS += -Wall -Wextra -Wshadow -Wstrict-prototypes -Werror
CFLAGS += -Os -g3 -ffunction-sections -fdata-sections
CFLAGS += -fno-strict-aliasing -std=c99

LDFLAGS := $(MCFLAGS)
LDFLAGS += -Wl,--gc-sections -Wl,--script=usb_dev_keyboard_gcc.ld
LDFLAGS += -L$(TIVAWARE_PATH)/driverlib/gcc -L$(TIVAWARE_PATH)/usblib/gcc

LIBS := -ldriverlib -lusblib -lc

SRCS := usb_dev_keyboard.c usb_keyb_structs.c portgremlin_config.c \
        portgremlin_vidpid.c portgremlin_strings.c portgremlin_uart.c \
        portgremlin_oracle.c portgremlin_persona.c portgremlin_mimic.c \
        portgremlin_telemetry.c portgremlin_evolve.c \
        startup_gcc.c
OBJS := $(SRCS:.c=.o)

.PHONY: all clean size flash gdb

all: $(PROJECT).bin

$(PROJECT).bin: $(PROJECT).elf
	$(OBJCOPY) -O binary $< $@

$(PROJECT).elf: $(OBJS)
	$(LD) $(LDFLAGS) -o $@ $^ $(LIBS)
	$(SIZE) $@

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJS) $(PROJECT).elf $(PROJECT).bin

size: $(PROJECT).elf
	$(SIZE) $<

flash: $(PROJECT).bin
	lm4flash $(PROJECT).bin

gdb: $(PROJECT).elf
	$(GDB) -ex "target extended-remote /dev/ttyACM0" -ex "monitor swd" -ex "load" -ex "monitor reset" $<
