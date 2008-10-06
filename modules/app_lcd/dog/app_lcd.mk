# $Id$
# defines additional rules for application specific LCD driver

# we include the CLCD driver, and enable the special DOG initialisation sequence
CFLAGS += -DAPP_LCD_INIT_DOG


# enhance include path
C_INCLUDE +=	-I $(MIOS32_PATH)/modules/app_lcd/clcd

# add modules to thumb sources
THUMB_SOURCE += \
	$(MIOS32_PATH)/modules/app_lcd/clcd/app_lcd.c

# include fonts
#include $(MIOS32_PATH)/modules/glcd_font/glcd_font.mk
# (not for CLCDs)

# directories and files that should be part of the distribution (release) package
DIST += $(MIOS32_PATH)/modules/app_lcd/clcd \
        $(MIOS32_PATH)/modules/app_lcd/dog
