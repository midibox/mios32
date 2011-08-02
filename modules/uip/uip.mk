# $Id$
# defines additional rules

# enhance include path
C_INCLUDE += -I $(MIOS32_PATH)/modules/uip/uip \
	     -I $(MIOS32_PATH)/modules/uip/lib \
	     -I $(MIOS32_PATH)/modules/uip/mios32 \
	     -I $(MIOS32_PATH)/modules/uip/mios32/$(MIOS32_FAMILY)


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
	$(MIOS32_PATH)/modules/uip/mios32/$(MIOS32_FAMILY)/network-device.c

ifeq ($(FAMILY),LPC17xx)
THUMB_SOURCE += $(MIOS32_PATH)/modules/uip/mios32/$(MIOS32_FAMILY)/lpc17xx_emac.c
endif


# directories and files that should be part of the distribution (release) package
DIST += $(MIOS32_PATH)/modules/uip
