##############################################################################
BUILD = build
BIN = UsbSnifferLite

##############################################################################
.PHONY: all directory clean size release

CC = arm-none-eabi-gcc
OBJCOPY = arm-none-eabi-objcopy
SIZE = arm-none-eabi-size

ifeq ($(OS), Windows_NT)
  MKDIR = gmkdir
else
  MKDIR = mkdir
endif

CFLAGS += -W -Wall --std=gnu11 -Os
CFLAGS += -ffreestanding
CFLAGS += -fno-diagnostics-show-caret
CFLAGS += -fdata-sections -ffunction-sections
CFLAGS += -funsigned-char -funsigned-bitfields
CFLAGS += -mcpu=cortex-m0plus -mthumb
CFLAGS += -MD -MP -MT $(BUILD)/$(*F).o -MF $(BUILD)/$(@F).d

LDFLAGS += -mcpu=cortex-m0plus -mthumb
LDFLAGS += -nostartfiles
LDFLAGS += -Wl,--gc-sections
LDFLAGS += -Wl,--script=../linker/rp2040.ld

INCLUDES += \
  -I../include \
  -I.. \
  -I../pico-pio-usb/src \
  -I$(PICO_SDK_PATH)/src/common/include \
  -I$(PICO_SDK_PATH)/src/rp2_common/hardware_sync/include \
  -I$(PICO_SDK_PATH)/src/rp2_common/hardware_base/include \
  -I$(PICO_SDK_PATH)/src/boards/include \
  -I$(PICO_SDK_PATH)/src/rp2040/hardware_regs/include \
  -I$(PICO_SDK_PATH)/src/rp2_common/pico_platform/include \
  -I$(PICO_SDK_PATH)/src/host/pico_include \
  -I$(PICO_SDK_PATH)/src/rp2_common/pico_stdlib/include \
  -I$(PICO_SDK_PATH)/src/common/pico_base_headers/include \
  -I$(PICO_SDK_PATH)/src/boards/include/boards \

SRCS += \
  ../main.c \
  ../utils.c \
  ../capture.c \
  ../display.c \
  ../usb.c \
  ../usb_std.c \
  ../usb_cdc.c \
  ../usb_descriptors.c \
  ../startup_rp2040.c \
  ../pico-pio-usb/src/pio_usb_host.c \
  ../pico-pio-usb/src/pio_usb.c

DEFINES += \

CFLAGS += $(INCLUDES) $(DEFINES)

OBJS = $(addprefix $(BUILD)/, $(notdir %/$(subst .c,.o, $(SRCS))))

all: directory $(BUILD)/$(BIN).elf $(BUILD)/$(BIN).hex $(BUILD)/$(BIN).bin $(BUILD)/$(BIN).uf2 release size

$(BUILD)/$(BIN).elf: $(OBJS)
	@echo LD $@
	@$(CC) $(LDFLAGS) $(OBJS) $(LIBS) -o $@

$(BUILD)/$(BIN).hex: $(BUILD)/$(BIN).elf
	@echo OBJCOPY $@
	@$(OBJCOPY) -O ihex $^ $@

$(BUILD)/$(BIN).bin: $(BUILD)/$(BIN).elf
	@echo OBJCOPY $@
	@$(OBJCOPY) -O binary $^

$(BUILD)/$(BIN).uf2: $(BUILD)/$(BIN).bin
	@echo UF2 $@
	@elf2uf2 $^ $@

release: $(BUILD)/$(BIN).bin $(BUILD)/$(BIN).uf2
	@echo release
	@cp $(BUILD)/$(BIN).bin ../../bin/
	@cp $(BUILD)/$(BIN).uf2 ../../bin/

directory:
	@$(MKDIR) -p $(BUILD)

size: $(BUILD)/$(BIN).elf
	@echo size:
	@$(SIZE) -t $^

clean:
	@echo clean
	@-rm -rf $(BUILD)

-include $(wildcard $(BUILD)/*.d)

%.o:
