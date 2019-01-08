# $Id$
# defines additional rules for application specific LCD driver

# enhance include path
C_INCLUDE +=	-I $(MIOS32_PATH)/modules/app_lcd/t6963_v

# add modules to thumb sources
THUMB_SOURCE += \
	$(MIOS32_PATH)/modules/app_lcd/t6963_v/app_lcd.c

# include fonts
include $(MIOS32_PATH)/modules/glcd_font/glcd_font.mk

# directories and files that should be part of the distribution (release) package
DIST += $(MIOS32_PATH)/modules/app_lcd/t6963_v

