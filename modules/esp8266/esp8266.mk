# $Id$

# enhance include path
C_INCLUDE += -I $(MIOS32_PATH)/modules/esp8266


# add modules to thumb sources (TODO: provide makefile option to add code to ARM sources)
THUMB_SOURCE += \
	$(MIOS32_PATH)/modules/esp8266/esp8266.c




# directories and files that should be part of the distribution (release) package
DIST += $(MIOS32_PATH)/modules/esp8266
