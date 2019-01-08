# $Id: blm_x.mk 132 2008-11-29 18:52:58Z tk $
# defines additional rules for integrating the button/LED matrix

# enhance include path
C_INCLUDE += -I $(MIOS32_PATH)/modules/blm_x


# add modules to thumb sources (TODO: provide makefile option to add code to ARM sources)
THUMB_SOURCE += \
	$(MIOS32_PATH)/modules/blm_x/blm_x.c


# directories and files that should be part of the distribution (release) package
DIST += $(MIOS32_PATH)/modules/blm_x
