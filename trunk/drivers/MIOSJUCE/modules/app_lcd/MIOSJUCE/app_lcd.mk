# $Id: app_lcd.mk 43 2008-09-30 23:30:38Z tk $
# defines additional rules for application specific LCD driver

# enhance include path
C_INCLUDE +=	-I $(MIOS32_PATH)/drivers/MIOSJUCE/modules/app_lcd/$(LCD)/

# add modules to thumb sources
THUMB_SOURCE += \
	$(MIOS32_PATH)/drivers/MIOSJUCE/modules/app_lcd/$(LCD)/app_lcd.c

# include fonts
#include $(MIOS32_PATH)/modules/glcd_font/glcd_font.mk
# (not for CLCDs)

# directories and files that should be part of the distribution (release) package
DIST += $(MIOS32_PATH)/drivers/MIOSJUCE/modules/app_lcd/$(LCD)/
