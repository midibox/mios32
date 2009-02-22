# $Id$
# defines additional rules for integrating DOSFS

# enhance include path
C_INCLUDE += -I $(MIOS32_PATH)/modules/uip/uip \
	     -I $(MIOS32_PATH)/modules/uip/lib \
	     -I $(MIOS32_PATH)/modules/uip/mios32


# add modules to thumb sources (TODO: provide makefile option to add code to ARM sources)
THUMB_SOURCE += \
	$(MIOS32_PATH)/modules/uip/uip/uip.c \
	$(MIOS32_PATH)/modules/uip/uip/uip_arp.c \
	$(MIOS32_PATH)/modules/uip/uip/uiplib.c \
	$(MIOS32_PATH)/modules/uip/uip/psock.c \
	$(MIOS32_PATH)/modules/uip/uip/timer.c \
	$(MIOS32_PATH)/modules/uip/uip/uip-neighbor.c \
	$(MIOS32_PATH)/modules/uip/lib/memb.c \
	$(MIOS32_PATH)/modules/uip/mios32/clock-arch.c \
	$(MIOS32_PATH)/modules/uip/mios32/network-device.c


# directories and files that should be part of the distribution (release) package
DIST += $(MIOS32_PATH)/modules/uip
