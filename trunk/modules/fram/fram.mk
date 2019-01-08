# defines additional rules for integrating the FRAM module

# enhance include path
C_INCLUDE += -I $(MIOS32_PATH)/modules/fram


# add modules to thumb sources (TODO: provide makefile option to add code to ARM sources)
THUMB_SOURCE += \
	$(MIOS32_PATH)/modules/fram/fram.c


# directories and files that should be part of the distribution (release) package
DIST += $(MIOS32_PATH)/modules/fram
