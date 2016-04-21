# defines additional rules for integrating the VGM Playback module
# Modified from MBHP_Genesis module code by Sauraen

# enhance include path
C_INCLUDE += -I $(MIOS32_PATH)/modules/vgm


# add modules to thumb sources (TODO: provide makefile option to add code to ARM sources)
THUMB_SOURCE += \
	$(MIOS32_PATH)/modules/vgm/vgm.c \
	$(MIOS32_PATH)/modules/vgm/vgmhead.c \
	$(MIOS32_PATH)/modules/vgm/vgmperfmon.c \
	$(MIOS32_PATH)/modules/vgm/vgmplayer.c \
	$(MIOS32_PATH)/modules/vgm/vgmqueue.c \
	$(MIOS32_PATH)/modules/vgm/vgmram.c \
	$(MIOS32_PATH)/modules/vgm/vgmsdtask.c \
	$(MIOS32_PATH)/modules/vgm/vgmsource.c \
	$(MIOS32_PATH)/modules/vgm/vgmstream.c \
	$(MIOS32_PATH)/modules/vgm/vgmtuning.c \


# directories and files that should be part of the distribution (release) package
DIST += $(MIOS32_PATH)/modules/vgm
