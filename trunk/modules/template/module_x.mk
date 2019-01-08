# defines additional rules for integrating the module_x

# enhance include path
C_INCLUDE += -I $(MIOS32_PATH)/modules/module_x


# add modules to thumb sources (TODO: provide makefile option to add code to ARM sources)
THUMB_SOURCE += \
	$(MIOS32_PATH)/modules/module_x/module_x.c


# directories and files that should be part of the distribution (release) package
DIST += $(MIOS32_PATH)/modules/module_x
