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
#   - THUMB_CPP_SOURCE e.g.: main.cp (.cpp only)
#   - THUMB_AS_SOURCE e.g.: assembly.s (.s only)
#   - ARM_SOURCE e.g.: my_startup.c (.c only)
#   - ARM_CPP_SOURCE e.g.: my_startup.cpp (.cpp only)
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
# The usage of arm-elf isn't recommented due to compatibility issues!!!
MIOS32_GCC_PREFIX ?= arm-none-eabi

CC      = $(MIOS32_GCC_PREFIX)-gcc
CPP     = $(MIOS32_GCC_PREFIX)-g++
OBJCOPY = $(MIOS32_GCC_PREFIX)-objcopy
OBJDUMP = $(MIOS32_GCC_PREFIX)-objdump
NM      = $(MIOS32_GCC_PREFIX)-nm
SIZE    = $(MIOS32_GCC_PREFIX)-size

# where should the output files be located
PROJECT_OUT ?= $(PROJECT)_build

# default linker flags
#LDFLAGS += -T $(LD_FILE) -mthumb -Xlinker -u _start -Wl,--gc-section  -Xlinker -M -Xlinker -Map=$(PROJECT).map -nostartfiles
LDFLAGS += -T $(LD_FILE) -mthumb -u _start -Wl,--gc-section  -Xlinker -M -Xlinker -Map=$(PROJECT_OUT)/$(PROJECT).map -lstdc++

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

# define CPP flags
CPPFLAGS += $(CFLAGS) -fno-rtti -fno-exceptions -Wno-write-strings


# convert .c/.s -> .o
THUMB_OBJS = $(THUMB_SOURCE:.c=.o)
THUMB_CPP_OBJS = $(THUMB_CPP_SOURCE:.cpp=.o)
THUMB_AS_OBJS = $(THUMB_AS_SOURCE:.s=.o)
ARM_OBJS = $(ARM_SOURCE:.c=.o)
ARM_CPP_OBJS = $(ARM_CPP_SOURCE:.cpp=.o)
ARM_AS_OBJS = $(ARM_AS_SOURCE:.s=.o)

# convert .s -> .lst
THUMB_AS_LST = $(THUMB_AS_SOURCE:.s=.lst)
ARM_AS_LST = $(ARM_AS_SOURCE:.s=.lst)

# list of all objects
ALL_OBJS = $(addprefix $(PROJECT_OUT)/, $(THUMB_OBJS) $(THUMB_CPP_OBJS) $(THUMB_AS_OBJS) $(ARM_OBJS) $(ARM_CPP_OBJS) $(ARM_AS_OBJS))

# list of all dependency files
ALL_DFILES = $(ALL_OBJS:.o=.d)

# which directories contain source files?
DIRS = $(dir $(THUMB_OBJS) $(THUMB_CPP_OBJS) $(THUMB_AS_OBJS) $(ARM_OBJS) $(ARM_CPP_OBJS) $(ARM_AS_OBJS))

# add files for distribution
DIST += $(MIOS32_PATH)/include/makefile/common.mk $(MIOS32_PATH)/include/c
DIST += $(LD_FILE)

# default rule
all: dirs cleanhex $(PROJECT).hex $(PROJECT_OUT)/$(PROJECT).bin $(PROJECT_OUT)/$(PROJECT).lss $(PROJECT_OUT)/$(PROJECT).sym projectinfo

# define debug/release target for easier use in codeblocks
debug: all
Debug: all
release: all
Release: all

# create the output directories
dirs:
	@-if [ ! -e $(PROJECT_OUT) ]; then mkdir $(PROJECT_OUT); fi;
	@-$(foreach DIR,$(DIRS), if [ ! -e $(PROJECT_OUT)/$(DIR) ]; \
	 then mkdir -p $(PROJECT_OUT)/$(DIR); fi; )


# rule to create a .hex and .bin file
%.bin : $(PROJECT_OUT)/$(PROJECT).elf
	@$(OBJCOPY) $< -O binary $@
%.hex : $(PROJECT_OUT)/$(PROJECT).elf
	@$(OBJCOPY) $< -O ihex $@

# rule to create a listing file from .elf
%.lss: $(PROJECT_OUT)/$(PROJECT).elf
	@$(OBJDUMP) -h -S -C $< > $@

# rule to create a symbol table from .elf
%.sym: $(PROJECT_OUT)/$(PROJECT).elf
	@$(NM) -n $< > $@

# rule to create .elf file
$(PROJECT_OUT)/$(PROJECT).elf: $(ALL_OBJS)
	@$(CC) $(CFLAGS) $(ALL_OBJS) $(LIBS) $(LDFLAGS) -o$@


# rule to output project informations
projectinfo:
	@echo "-------------------------------------------------------------------------------"
	@echo "Application successfully built for:"
	@echo "Processor: $(PROCESSOR)"
	@echo "Family:    $(FAMILY)"
	@echo "Board:     $(BOARD)"
	@echo "LCD:       $(LCD)"
	@echo "-------------------------------------------------------------------------------"
	$(SIZE) $(PROJECT_OUT)/$(PROJECT).elf

# default rule for compiling .c programs
# inspired from the "super makefile" published at http://gpwiki.org/index.php/Make
# Rule for creating object file and .d file, the sed magic is to add
# the object path at the start of the file because the files gcc
# outputs assume it will be in the same dir as the source file.
$(PROJECT_OUT)/%.o: %.c
	@echo Creating object file for $(notdir $<)
	@$(CC) -Wp,-MMD,$(PROJECT_OUT)/$*.dd $(CFLAGS) -mthumb -c $< -o $@
	@sed -e '1s/^\(.*\)$$/$(subst /,\/,$(dir $@))\1/' $(PROJECT_OUT)/$*.dd > $(PROJECT_OUT)/$*.d
	@rm -f $(PROJECT_OUT)/$*.dd

$(PROJECT_OUT)/%.o: %.cpp
	@echo Creating object file for $(notdir $<)
	@$(CC) -Wp,-MMD,$(PROJECT_OUT)/$*.dd $(CPPFLAGS) -mthumb -c $< -o $@
	@sed -e '1s/^\(.*\)$$/$(subst /,\/,$(dir $@))\1/' $(PROJECT_OUT)/$*.dd > $(PROJECT_OUT)/$*.d
	@rm -f $(PROJECT_OUT)/$*.dd

$(PROJECT_OUT)/%.o: %.s
	@echo Creating object file for $(notdir $<)
	@$(CC) -Wp,-MMD,$(PROJECT_OUT)/$*.dd $(ASFLAGS) -mthumb -c $< -o $@
	@sed -e '1s/^\(.*\)$$/$(subst /,\/,$(dir $@))\1/' $(PROJECT_OUT)/$*.dd > $(PROJECT_OUT)/$*.d
	@rm -f $(PROJECT_OUT)/$*.dd

# Includes the .d files so it knows the exact dependencies for every
# source.
-include $(ALL_DFILES)

# TODO: solution to differ between THUMB and ARM objects!
# we could search in the ARM*OBJS list and prevent the usage of -mthumb in this case
#$(ARM_OBJS) : %.o : %.c
#	$(CC) -c $(CFLAGS) $< -o $@

#$(ARM_CPP_OBJS) : %.o : %.cpp
#	$(CPP) -c $(CPPFLAGS) $< -o $@

#$(ARM_AS_OBJS) : %.o : %.s
#	$(CC) -c $(AFLAGS) $< -o $@


# clean temporary files
clean:
	rm -rf $(PROJECT_OUT)

# clean project image
cleanhex:
	rm -f $(PROJECT).hex

# clean temporary files + project image
cleanall: clean cleanhex


# for use with graphviz and egypt
callgraph: egyptall egypt_tidy egypt_all egypt_project


egyptall: CFLAGS += -dr
egyptall: all

egypt_tidy:
	@rm -fR egypt
	@echo "-------------------------------------------------------------------------------"
	@echo "Generating call graphs in .dot files"
	@echo "-------------------------------------------------------------------------------"
	@mkdir egypt
	@mv *.expand egypt/

egypt_all:
	@perl $(MIOS32_PATH)/etc/egypt/egypt egypt/*.expand > egypt/$(PROJECT).dot


egypt_project: EGYPTFILES = find egypt/*.expand -maxdepth 1 ! -name "mios32_*.expand" ! -name "stm32f10x*.expand" ! -name "usb_*.expand" ! -name "printf-stdarg.c*.expand" ! -name "crt0_STM32x.c*.expand" ! -name "app_lcd.c*.expand" ! -name "heap_*.expand" ! -name "list.c*.expand" ! -name "port.c*.expand" ! -name "queue.c*.expand" ! -name "main.c*.expand"
egypt_project:
	@perl $(MIOS32_PATH)/etc/egypt/egypt `$(EGYPTFILES)` > egypt/$(PROJECT)_NoMIOS.dot


callgraph_convert:
	@$(MIOS32_SHELL) $(MIOS32_PATH)/etc/egypt/dot_output.sh

callgraph_all: callgraph callgraph_convert

callgraph_clean: 
	@rm -fR egypt
