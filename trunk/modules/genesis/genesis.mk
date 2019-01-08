# defines additional rules for integrating the MBHP_Genesis module
# Modified from opl3 module code by Sauraen

# enhance include path
C_INCLUDE += -I $(MIOS32_PATH)/modules/genesis


# add modules to thumb sources (TODO: provide makefile option to add code to ARM sources)
THUMB_SOURCE += \
	$(MIOS32_PATH)/modules/genesis/genesis.c \


# directories and files that should be part of the distribution (release) package
DIST += $(MIOS32_PATH)/modules/genesis
