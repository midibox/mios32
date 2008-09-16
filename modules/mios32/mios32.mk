# $Id$
# defines additional rules for MIOS32

# enhance include path
C_INCLUDE += -I $(MIOS32_PATH)/modules/mios32


# add modules to thumb sources (TODO: provide makefile option to add code to ARM sources)
THUMB_SOURCE += \
	$(MIOS32_PATH)/modules/mios32/mios32_sys.c \
	$(MIOS32_PATH)/modules/mios32/mios32_srio.c \
	$(MIOS32_PATH)/modules/mios32/mios32_din.c \
	$(MIOS32_PATH)/modules/mios32/mios32_dout.c \
	$(MIOS32_PATH)/modules/mios32/mios32_usb.c \
	$(MIOS32_PATH)/modules/mios32/mios32_usb_desc.c \
	$(DRIVER_LIB)/src/usb_core.c \
	$(DRIVER_LIB)/src/usb_init.c \
	$(DRIVER_LIB)/src/usb_int.c \
	$(DRIVER_LIB)/src/usb_mem.c \
	$(DRIVER_LIB)/src/usb_regs.c

# MEMO: the gcc linker is clever enough to exclude functions from the final memory image
# if they are not references from the main routine - accordingly we can savely include
# the USB drivers without the danger that this increases the project size of applications,
# which don't use the USB peripheral at all :-)


# directories and files that should be part of the distribution (release) package
DIST += $(MIOS32_PATH)/modules/mios32
