# $Id$
#
# following variables should be set before including this file:
#   - PROCESSOR e.g.: STM32F103RE
#   - FAMILY    e.g.: STM32F10x
#   - BOARD     e.g.: MBHP_CORE_STM32
#   - LCD       e.g.: clcd
#   - LD_FILE   e.g.: $(MIOS32_PATH)/etc/ld/$(FAMILY)/$(PROCESSOR).ld
#   - PROJECT   e.g.: project   # (.lst, .hex, .map, etc... will be added automatically)
#   - THUMB_SOURCE e.g.: main.c (.c only)
#   - THUMB_AS_SOURCE e.g.: assembly.s (.s only)
#   - ARM_SOURCE e.g.: my_startup.c (.c only)
#   - ARM_AS_SOURCE e.g.: assembly.s (.s only)
#   - C_INCLUDE     e.g.: -I./ui  # (more include pathes will be added by .mk files)
#   - A_INCLUDE     same for assembly code
#   - DIST      e.g.: ./
#
# Modules can be added by including .mk files from $MIOS32_PATH/modules/*/*.mk
#

# if MIOS32_SHELL environment variable hasn't been set by the user, set it here
# Ubuntu users should set it to /bin/bash from external (-> "export MIOS32_SHELL /bin/bash")
MIOS32_SHELL ?= sh
export MIOS32_SHELL

# select GCC tools
# can be optionally overruled via environment variable
# e.g. for Cortex M3 support provided by CodeSourcery, use MIOS32_GCC_PREFIX=arm-none-eabi
MIOS32_GCC_PREFIX ?= arm-elf

CC      = $(MIOS32_GCC_PREFIX)-gcc
OBJCOPY = $(MIOS32_GCC_PREFIX)-objcopy
OBJDUMP = $(MIOS32_GCC_PREFIX)-objdump
NM      = $(MIOS32_GCC_PREFIX)-nm
SIZE    = $(MIOS32_GCC_PREFIX)-size

# default linker flags
LDFLAGS += -T $(LD_FILE) -mthumb -Xlinker -o$(PROJECT).elf -u _start -Wl,--gc-section  -Xlinker -M -Xlinker -Map=$(PROJECT).map -nostartfiles

# default assembler flags
AFLAGS += $(A_DEFINES) $(A_INCLUDE) -Wa,-adhlns=$(<:.s=.lst)

# define C flags
CFLAGS += $(C_DEFINES) $(C_INCLUDE) 

# add family specific arguments
ifeq ($(FAMILY),STM32F10x)
CFLAGS += -mcpu=cortex-m3 -mlittle-endian -ffunction-sections
endif

ifeq ($(FAMILY),STR9x)
CFLAGS += -mcpu=arm7tdmi -D PACK_STRUCT_END=__attribute\(\(packed\)\) -D ALIGN_STRUCT_END=__attribute\(\(aligned\(4\)\)\) -fomit-frame-pointer -ffunction-sections -mthumb-interwork
endif


# convert .c/.s -> .o
THUMB_OBJS = $(THUMB_SOURCE:.c=.o)
THUMB_AS_OBJS = $(THUMB_AS_SOURCE:.s=.o)
ARM_OBJS = $(ARM_SOURCE:.c=.o)
ARM_AS_OBJS = $(ARM_AS_SOURCE:.s=.o)

# convert .s -> .lst
THUMB_AS_LST = $(THUMB_AS_SOURCE:.s=.lst)
ARM_AS_LST = $(ARM_AS_SOURCE:.s=.lst)

# add files for distribution
DIST += $(MIOS32_PATH)/include/makefile/common.mk $(MIOS32_PATH)/include/c
DIST += $(LD_FILE)

# default rule
# note: currently we always require a "cleanall", since dependencies (e.g. on .h files) are not properly declared
# later we could try it w/o "cleanall", and propose the usage of this step to the user
all: cleanall $(PROJECT).hex $(PROJECT).bin $(PROJECT).lss $(PROJECT).sym projectinfo

# define debug/release target for easier use in codeblocks
debug: all
Debug: all
release: all
Release: all

# rule to create a .hex and .bin file
%.bin : $(PROJECT).elf
	$(OBJCOPY) $< -O binary $@
%.hex : $(PROJECT).elf
	$(OBJCOPY) $< -O ihex $@

# rule to create a listing file from .elf
%.lss: $(PROJECT).elf
	$(OBJDUMP) -h -S -C $< > $@

# rule to create a symbol table from .elf
%.sym: $(PROJECT).elf
	$(NM) -n $< > $@

# rule to create .elf file
$(PROJECT).elf: $(THUMB_OBJS) $(THUMB_AS_OBJS) $(ARM_OBJS) $(ARM_AS_OBJS)
	$(CC) $(CFLAGS) $(ARM_OBJS) $(THUMB_OBJS) $(ARM_AS_OBJS) $(THUMB_AS_OBJS) $(LIBS) $(LDFLAGS) 

# rule to output project informations
projectinfo:
	@echo "-------------------------------------------------------------------------------"
	@echo "Application successfully built for:"
	@echo "Processor: $(PROCESSOR)"
	@echo "Family:    $(FAMILY)"
	@echo "Board:     $(BOARD)"
	@echo "LCD:       $(LCD)"
	@echo "-------------------------------------------------------------------------------"
	$(SIZE) $(PROJECT).elf

# default rule for compiling .c programs
$(THUMB_OBJS) : %.o : %.c
	$(CC) -c $(CFLAGS) -mthumb $< -o $@

$(ARM_OBJS) : %.o : %.c
	$(CC) -c $(CFLAGS) $< -o $@

# default rule for compiling assembly programs
$(THUMB_AS_OBJS) : %.o : %.s
	$(CC) -c $(AFLAGS) -mthumb $< -o $@

$(ARM_AS_OBJS) : %.o : %.s
	$(CC) -c $(AFLAGS) $< -o $@

# clean temporary files
clean:
	rm -f *.lss *.sym *.map *.elf
	rm -f $(THUMB_OBJS) $(THUMB_AS_OBJS) $(THUMB_AS_LST) $(ARM_OBJS) $(ARM_AS_OBJS)  $(ARM_AS_LST)

# clean temporary files + project image
cleanall: clean
	rm -f *.hex *.bin
