TIVAWARE_PATH ?= /opt/ti/TivaWare_C_Series-2.2.0.295

PROJECT := portgremlin

CC := arm-none-eabi-gcc
LD := arm-none-eabi-gcc
OBJCOPY := arm-none-eabi-objcopy
SIZE := arm-none-eabi-size
GDB := arm-none-eabi-gdb

MCFLAGS := -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16

DEFS := -DPART_TM4C123GH6PM -DTARGET_IS_TM4C123_RB1 -DUART_BUFFERED -Dgcc

INCLUDES := -I$(TIVAWARE_PATH) \
            -I$(TIVAWARE_PATH)/examples/boards/ek-tm4c123gxl \
            -I.

CFLAGS := $(MCFLAGS) $(DEFS) $(INCLUDES)
CFLAGS += -Wall -Wextra -Wshadow -Wstrict-prototypes -Werror
CFLAGS += -Wno-unused-parameter -Wno-missing-field-initializers
CFLAGS += -Os -g3 -ffunction-sections -fdata-sections
CFLAGS += -fno-strict-aliasing -std=c99

LDFLAGS := $(MCFLAGS)
LDFLAGS += -Wl,--gc-sections -Wl,--script=usb_dev_keyboard_gcc.ld
LDFLAGS += -L$(TIVAWARE_PATH)/driverlib/gcc -L$(TIVAWARE_PATH)/usblib/gcc

LIBS := -Wl,--start-group -ldriver -lusb -lc -lm -lgcc -Wl,--end-group

SRCS := usb_dev_keyboard.c usb_keyb_structs.c portgremlin_config.c \
        portgremlin_vidpid.c portgremlin_strings.c portgremlin_uart.c \
        portgremlin_oracle.c portgremlin_persona.c portgremlin_mimic.c \
        portgremlin_telemetry.c portgremlin_evolve.c \
        startup_gcc.c
OBJS := $(SRCS:.c=.o)

# TivaWare utils + LaunchPad board drivers (not redistributable).
TIVA_EXTRA_SRCS := $(TIVAWARE_PATH)/utils/uartstdio.c \
                   $(TIVAWARE_PATH)/utils/ustdlib.c \
                   $(TIVAWARE_PATH)/examples/boards/ek-tm4c123gxl/drivers/buttons.c
TIVA_EXTRA_OBJS := uartstdio.o ustdlib.o buttons.o

.PHONY: all clean size flash gdb

all: $(PROJECT).bin

$(PROJECT).bin: $(PROJECT).elf
	$(OBJCOPY) -O binary $< $@

$(PROJECT).elf: $(OBJS) $(TIVA_EXTRA_OBJS)
	$(LD) $(LDFLAGS) -o $@ $^ $(LIBS)
	$(SIZE) $@

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

uartstdio.o: $(TIVAWARE_PATH)/utils/uartstdio.c
	$(CC) $(CFLAGS) -Wno-error=sign-compare -c -o $@ $<

ustdlib.o: $(TIVAWARE_PATH)/utils/ustdlib.c
	$(CC) $(CFLAGS) -Wno-error=sign-compare -c -o $@ $<

buttons.o: $(TIVAWARE_PATH)/examples/boards/ek-tm4c123gxl/drivers/buttons.c
	$(CC) $(CFLAGS) -Wno-error=sign-compare -c -o $@ $<

clean:
	rm -f $(OBJS) $(TIVA_EXTRA_OBJS) $(PROJECT).elf $(PROJECT).bin

size: $(PROJECT).elf
	$(SIZE) $<

flash: $(PROJECT).bin
	lm4flash $(PROJECT).bin

gdb: $(PROJECT).elf
	$(GDB) -ex "target extended-remote /dev/ttyACM0" -ex "monitor swd" -ex "load" -ex "monitor reset" $<
