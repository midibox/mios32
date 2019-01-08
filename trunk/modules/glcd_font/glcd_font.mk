# $Id$
# defines the rule for creating the glcd_font_*.o objects,

# enhance include path
C_INCLUDE +=	-I $(MIOS32_PATH)/modules/glcd_font

# add modules to thumb sources
THUMB_SOURCE += \
	$(MIOS32_PATH)/modules/glcd_font/glcd_font_normal.c \
	$(MIOS32_PATH)/modules/glcd_font/glcd_font_normal_inv.c \
	$(MIOS32_PATH)/modules/glcd_font/glcd_font_big.c \
	$(MIOS32_PATH)/modules/glcd_font/glcd_font_small.c \
	$(MIOS32_PATH)/modules/glcd_font/glcd_font_tiny.c \
	$(MIOS32_PATH)/modules/glcd_font/glcd_font_knob_icons.c \
	$(MIOS32_PATH)/modules/glcd_font/glcd_font_meter_icons_h.c \
	$(MIOS32_PATH)/modules/glcd_font/glcd_font_meter_icons_v.c

# directories and files that should be part of the distribution (release) package
DIST += $(MIOS32_PATH)/modules/glcd_font
