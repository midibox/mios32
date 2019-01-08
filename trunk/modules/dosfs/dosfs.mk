# $Id$
# defines additional rules for integrating DOSFS

# enhance include path
C_INCLUDE += -I $(MIOS32_PATH)/modules/dosfs


# add modules to thumb sources (TODO: provide makefile option to add code to ARM sources)
THUMB_SOURCE += \
	$(MIOS32_PATH)/modules/dosfs/dosfs.c \
	$(MIOS32_PATH)/modules/dosfs/dfs_sdcard.c


# directories and files that should be part of the distribution (release) package
DIST += $(MIOS32_PATH)/modules/dosfs
