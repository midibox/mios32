##################################################################
# Project Variables 
##################################################################

# Project name
PROJECT = vXjuce


# Path to app sources
CORE_DIR = ../../core/src/
VX_DIR = ../../vxmodules/src/


# Path to MIOSJUCE
MIOSJUCE_PATH = ../../MIOSJUCE/


# Operating system
# Choose LINUX MACOS _WIN32 or _WIN64
OS = LINUX


#################################
# Add your MIOS app sources here....

# App sources
OBJS += $(CORE_DIR)tasks.o \
		$(CORE_DIR)app.o \
		$(CORE_DIR)utils.o \
		$(CORE_DIR)ui.o \
		$(CORE_DIR)graph.o \
		$(CORE_DIR)mclock.o \
		$(CORE_DIR)patterns.o \
		$(CORE_DIR)mod_send.o \
		$(CORE_DIR)mod_xlate.o \
		$(CORE_DIR)modules.o \
\
		$(VX_DIR)mod_sclk.o \
		$(VX_DIR)mod_sxh.o \
		$(VX_DIR)mod_seq.o \
		$(VX_DIR)mod_midiout.o

# App include paths
CFLAGS += -I $(CORE_DIR)../inc -I $(VX_DIR)../inc 


# Module sources
OBJS += $(MIOS32_PATH)/modules/sequencer/seq_bpm.o \
		$(MIOS32_PATH)/modules/sequencer/seq_midi_out.o

# Module include paths
CFLAGS += -I $(MIOS32_PATH)/modules/sequencer

# End MIOS app sources
#################################



##################################################################
# MIOSJUCE Emulation
##################################################################

#################################
# Add your emulation UI sources here....

# CS emulation sources
OBJS += ../../vXjuce/src/AppComponent.o


# CS emulation includes
CFLAGS += -I ../../vXjuce/inc/

# End emulation UI sources
#################################





##################################################################
# Build script
#
# No changes need to be made by the user below this line!!
#
##################################################################

#################################
# Emulation wrappers
#################################



# JUCE libraries
LIBJUCE_PATH = $(MIOSJUCE_PATH)juced/juce/bin/
LIBJUCE_RELEASE = libjuce.a 
LIBJUCE_DEBUG = libjuce_debug.a 

# JUCE includes
CFLAGS += -I $(MIOSJUCE_PATH)juced/juce/ 




# MIOSJUCE programming model sources
OBJS += $(MIOSJUCE_PATH)programming_models/MIOSJUCE/Main.o \
		$(MIOSJUCE_PATH)programming_models/MIOSJUCE/MainComponent.o 

# MIOSJUCE programming model flags
CFLAGS += -DENDLESSLOOP=0




# FREERTOS emulation C files
OBJS += $(MIOSJUCE_PATH)FreeRTOS/Source/task.o \
		$(MIOSJUCE_PATH)FreeRTOS/Source/list.o \
		$(MIOSJUCE_PATH)FreeRTOS/Source/queue.o \
		$(MIOSJUCE_PATH)FreeRTOS/Source/portable/GCC/JUCE/port.o \
		$(MIOSJUCE_PATH)FreeRTOS/Source/portable/MemMang/heap_3.o 

# FREERTOS emulation C++ files
OBJS += $(MIOSJUCE_PATH)FreeRTOS/Source/JUCEtask.o \
		$(MIOSJUCE_PATH)FreeRTOS/Source/JUCEqueue.o

# FreeRTOS emulation includes
CFLAGS += -I $(MIOSJUCE_PATH)FreeRTOS/Source/include -I $(MIOSJUCE_PATH)FreeRTOS/Source/portable/GCC/JUCE




# MIOS emulation C files
OBJS += $(MIOSJUCE_PATH)programming_models/MIOSJUCE/mios32_main.o \
		$(MIOSJUCE_PATH)mios32/mios32_midi.o\
		$(MIOSJUCE_PATH)mios32/mios32_timer.o

# MIOS emulation C++ files
OBJS += $(MIOSJUCE_PATH)mios32/JUCEmios32.o \
		$(MIOSJUCE_PATH)mios32/JUCEmidi.o \
		$(MIOSJUCE_PATH)mios32/JUCEtimer.o

# MIOS emulation includes
CFLAGS += -I $(MIOSJUCE_PATH)programming_models/MIOSJUCE/ \
		 -I $(MIOSJUCE_PATH)include/mios32/




# MIOS defines
CFLAGS += -DMIOS32_FAMILY_EMULATION -DJUCED -DMIOS32_FAMILY_STR=\"EMULATION\" -DMIOS32_BOARD_STR=\"JUCED\" -D$(OS)

# MIOS includes
CFLAGS += -I $(MIOS32_PATH)/include/mios32/




# Required GCC flags
CFLAGS += -Wall -I/usr/include/ -I ./

# Linker flags
LDFLAGS += -lfreetype -lpthread -lX11 -lXinerama -lGL -lasound

# Link libs
# LDFLAGS += /usr/lib/libfreetype.a /usr/lib/libpthread.a /usr/lib/libX11.a /usr/lib/libXinerama.a /usr/lib/libGL.so

# Output dirs
OBJS_DIR = obj/
BIN_DIR = bin/

DEBUG_DIR=Debug/
RELEASE_DIR = Release/



# Compiler names if you want to override them
CPP = g++
CC = gcc
LD = g++
LIB = ar
WINDRES =



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
	$(CPP) $(CFLAGS) -c $< -o $(OUTDIR)$(notdir $@ )

# .c files
%.o: %.c setup
	@echo Compiling: $<
	$(CC) $(CFLAGS) -c $< -o $(OUTDIR)$(notdir $@ )

# linker
link:
	@echo Building executable $(BINDIR)$(PROJECT)
	$(CPP) -L$(LIBJUCE_PATH) -L/usr/lib -o $(BINDIR)$(PROJECT) $(patsubst %.o,$(OUTDIR)%.o,$(notdir $(OBJS))) $(LDFLAGS) $(JUCELIB)



# Target: Release
Release: LDFLAGS += -s
Release: CFLAGS += -O2
Release: OUTDIR = $(OBJS_DIR)$(RELEASE_DIR)
Release: BINDIR = $(BIN_DIR)$(RELEASE_DIR)
Release: JUCELIB = $(LIBJUCE_PATH)$(LIBJUCE_RELEASE)
Release: cleanRelease $(OBJS) link


# Target: Debug
Debug: CFLAGS += -g
Debug: OUTDIR = $(OBJS_DIR)$(DEBUG_DIR)
Debug: BINDIR = $(BIN_DIR)$(DEBUG_DIR)
Debug: JUCELIB = $(LIBJUCE_PATH)$(LIBJUCE_DEBUG)
Debug: cleanDebug $(OBJS) link




#################################
# Clean
#################################

clean: cleanall
cleanall: cleanDebug cleanRelease

cleanDebug: 
	@echo Deleting $(BIN_DIR)$(DEBUG_DIR)$(PROJECT)
	@rm -f $(BIN_DIR)$(DEBUG_DIR)$(PROJECT)
	@echo Deleting $(OBJS_DIR)$(DEBUG_DIR)*.o
	@rm -f $(OBJS_DIR)$(DEBUG_DIR)*.o

cleanRelease: 
	@echo Deleting $(BIN_DIR)$(RELEASE_DIR)$(PROJECT)
	@rm -f $(BIN_DIR)$(RELEASE_DIR)$(PROJECT)
	@echo Deleting $(OBJS_DIR)$(RELEASE_DIR)*.o
	@rm -f $(OBJS_DIR)$(RELEASE_DIR)*.o


# Notes:
# with some work this can be used as a programming model like normal mios32 apps
#
# needs variable names altered and to use *.c/*.cpp for sources rather than *.o
# so that users can use the existing .mk include files for including modules
