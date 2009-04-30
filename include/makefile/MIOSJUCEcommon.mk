# $Id: common.mk 400 2009-03-24 08:47:37Z stryd_one $
#
# following variables should be set before including this file:
#   - PROCESSOR e.g.: STM32F103RE
#   - FAMILY    e.g.: STM32F10x
#   - BOARD     e.g.: MBHP_CORE_STM32
#   - LCD       e.g.: clcd
#   - PROJECT   e.g.: project   # (.exe, etc... will be added automatically)
#   - THUMB_SOURCE e.g.: main.c (.c or .cpp only)
#   - C_INCLUDE     e.g.: -I./ui  # (more include pathes will be added by .mk files)
#   - DIST      e.g.: ./
#   - JUCE_PATH  e.g.: $(MIOS32_PATH)/drivers/MIOSJUCE/juced/juce/
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
MIOS32_GCC_PREFIX = 


CPP = g++
CC = gcc
LD = g++
OBJCOPY = objcopy
OBJDUMP = objdump
NM      = nm
LIB = ar
WINDRES =
SIZE    = size

# default linker flags
# Linker flags
LDFLAGS += -lfreetype -lpthread -lX11 -lXinerama -lGL -lasound
# Link libs
# LDFLAGS += /usr/lib/libfreetype.a /usr/lib/libpthread.a /usr/lib/libX11.a /usr/lib/libXinerama.a /usr/lib/libGL.so

# define C flags
CFLAGS += $(C_DEFINES) $(C_INCLUDE) 

# add family specific arguments


# add files for distribution
DIST += $(MIOS32_PATH)/include/makefile/MIOSJUCEcommon.mk $(MIOS32_PATH)/include/c
DIST += $(LD_FILE)




##################################################################
# Build script
#
# No changes need to be made by the user below this line!!
#
##################################################################

# convert .c/.cpp -> .o
#THUMB_OBJS = $(patsubst %.c,%.o,$(THUMB_SOURCE))
THUMB_OBJS = $(patsubst %.c,%.o,$(patsubst %.cpp,%.o,$(THUMB_SOURCE)))
#THUMB_OBJS = $(THUMB_SOURCE:.c=.o)

# JUCE libraries
LIBJUCE_PATH = $(JUCE_PATH)/bin
LIBJUCE_RELEASE = libjuce.a 
LIBJUCE_DEBUG = libjuce_debug.a 

# JUCE includes
C_INCLUDE += -I $(JUCE_PATH)/



# Output dirs
OBJS_DIR = obj
BIN_DIR = bin

DEBUG_DIR = Debug
RELEASE_DIR = Release



#################################
# Make targets
#################################

all: Release


# setup
setup:
	@echo Creating output directories
	@mkdir -p $(OUTDIR)
	@mkdir -p $(BINDIR)



# .cpp files
%.o: %.cpp setup
	@echo Compiling: $<
	$(CXX) $(CFLAGS) -c $< -o $(OUTDIR)/$(notdir $@ )

# .c files
%.o: %.c setup
	@echo Compiling: $<
	$(CC) $(CFLAGS) -c $< -o $(OUTDIR)/$(notdir $@ )

# linker
link: setup
	@echo Building executable $(BINDIR)/$(PROJECT)
	$(LD) -L$(LIBJUCE_PATH) -L/usr/lib -o $(BINDIR)/$(PROJECT) $(patsubst %.o,$(OUTDIR)/%.o,$(notdir $(THUMB_OBJS))) $(LDFLAGS) $(JUCELIB)




# rule to output project informations
projectinfo:
	@echo "-------------------------------------------------------------------------------"
	@echo "Application successfully built for:"
	@echo "Processor: $(PROCESSOR)"
	@echo "Family:    $(FAMILY)"
	@echo "Board:     $(BOARD)"
	@echo "LCD:       $(LCD)"
	@echo "-------------------------------------------------------------------------------"
	$(SIZE) $(BINDIR)/$(PROJECT)




# Target: Release
Release: LDFLAGS += -s
Release: CFLAGS += $(OPTIMIZE)
Release: OUTDIR = $(OBJS_DIR)/$(RELEASE_DIR)
Release: BINDIR = $(BIN_DIR)/$(RELEASE_DIR)
Release: JUCELIB = $(LIBJUCE_PATH)/$(LIBJUCE_RELEASE)
Release: cleanRelease $(THUMB_OBJS) link projectinfo


# Target: Debug
Debug: CFLAGS += $(DEBUG)
Debug: OUTDIR = $(OBJS_DIR)/$(DEBUG_DIR)
Debug: BINDIR = $(BIN_DIR)/$(DEBUG_DIR)
Debug: JUCELIB = $(LIBJUCE_PATH)/$(LIBJUCE_DEBUG)
Debug: cleanDebug $(THUMB_OBJS) link projectinfo





#################################
# Clean
#################################

clean: cleanall
cleanall: cleanDebug cleanRelease

cleanDebug: 
	@echo Deleting $(BIN_DIR)/$(DEBUG_DIR)/$(PROJECT)
	@rm -f $(BIN_DIR)/$(DEBUG_DIR)/$(PROJECT)
	@echo Deleting $(OBJS_DIR)/$(DEBUG_DIR)/*.o
	@rm -f $(OBJS_DIR)/$(DEBUG_DIR)/*.o

cleanRelease: 
	@echo Deleting $(BIN_DIR)/$(RELEASE_DIR)/$(PROJECT)
	@rm -f $(BIN_DIR)/$(RELEASE_DIR)/$(PROJECT)
	@echo Deleting $(OBJS_DIR)/$(RELEASE_DIR)/*.o
	@rm -f $(OBJS_DIR)/$(RELEASE_DIR)/*.o
