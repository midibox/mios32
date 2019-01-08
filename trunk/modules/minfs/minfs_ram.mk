# defines additional rules for integrating MINFS

# enhance include path
C_INCLUDE += -I $(MIOS32_PATH)/modules/minfs


# add modules to thumb sources (TODO: provide makefile option to add code to ARM sources)
THUMB_SOURCE += \
	$(MIOS32_PATH)/modules/minfs/minfs.c \
	$(MIOS32_PATH)/modules/minfs/minfs_ram.c


# directories and files that should be part of the distribution (release) package
DIST += $(MIOS32_PATH)/modules/minfs