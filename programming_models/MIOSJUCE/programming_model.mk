# $Id: programming_model.mk 229 2009-01-01 13:05:29Z tk $
# defines rules building the programming model


#################################
# Emulation wrappers
#################################


# where is MIOSJUCE FreeRTOS Emulation located
FREE_RTOS = $(MIOS32_PATH)/mios32/MIOSJUCE/FreeRTOS

# required by FreeRTOS to select the port
CFLAGS += -DMIOSJUCE

# FREERTOS emulation C files
THUMB_SOURCE += $(FREE_RTOS)/Source/task.c \
				$(FREE_RTOS)/Source/list.c \
				$(FREE_RTOS)/Source/queue.c \
				$(FREE_RTOS)/Source/portable/GCC/MIOSJUCE/port.c \
				$(FREE_RTOS)/Source/portable/MemMang/heap_3.c 

# FREERTOS emulation C++ files
THUMB_SOURCE += $(FREE_RTOS)/Source/JUCEtask.cpp \
				$(FREE_RTOS)/Source/JUCEqueue.cpp


# FreeRTOS emulation includes
C_INCLUDE += -I $(FREE_RTOS)/Source/include \
			 -I $(FREE_RTOS)/Source/portable/GCC/MIOSJUCE


# MIOSJUCE programming model
THUMB_SOURCE += $(MIOS32_PATH)/programming_models/MIOSJUCE/mios32_main.c \
				$(MIOS32_PATH)/programming_models/MIOSJUCE/Main.cpp \
				$(MIOS32_PATH)/programming_models/MIOSJUCE/MainComponent.cpp 


# MIOS emulation sources
THUMB_SOURCE += $(MIOS32_PATH)/mios32/MIOSJUCE/c++/JUCEmios32.cpp \
				$(MIOS32_PATH)/mios32/MIOSJUCE/c++/JUCEmidi.cpp \
				$(MIOS32_PATH)/mios32/MIOSJUCE/c++/JUCEtimer.cpp


# MIOS emulation includes
C_INCLUDE += -I $(MIOS32_PATH)/programming_models/MIOSJUCE/ \
			 -I $(MIOS32_PATH)/mios32/MIOSJUCE/include/ \
			 -I/usr/include/ \
			 -I ./

# MIOSJUCE programming model flags
CFLAGS += -DENDLESSLOOP=0 -D$(OS)





# MIOS includes
C_INCLUDE += -I $(MIOS32_PATH)/include/mios32/

# add MIOS32 sources and C++ wrappers
include $(MIOS32_PATH)/mios32/mios32.mk



# directories and files that should be part of the distribution (release) package
DIST += $(MIOS32_PATH)/programming_models/MIOSJUCE
