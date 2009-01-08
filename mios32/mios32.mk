# $Id$
# defines additional rules for MIOS32

# enhance include path
C_INCLUDE +=	-I $(MIOS32_PATH)/include/mios32

# forward MIOS32 environment variables to preprocessor
CFLAGS    +=    -DMIOS32_PROCESSOR_$(PROCESSOR) \
		-DMIOS32_FAMILY_$(FAMILY) \
		-DMIOS32_FAMILY_STR=\"$(FAMILY)\" \
		-DMIOS32_BOARD_$(BOARD) \
		-DMIOS32_BOARD_STR=\"$(BOARD)\"


# add modules to thumb sources
# TODO: provide makefile option to add code to ARM sources
THUMB_SOURCE += \
	$(MIOS32_PATH)/mios32/common/mios32_din.c \
	$(MIOS32_PATH)/mios32/common/mios32_dout.c \
	$(MIOS32_PATH)/mios32/common/mios32_enc.c \
	$(MIOS32_PATH)/mios32/common/mios32_lcd.c \
	$(MIOS32_PATH)/mios32/common/mios32_midi.c \
	$(MIOS32_PATH)/mios32/common/mios32_com.c \
	$(MIOS32_PATH)/mios32/common/mios32_uart_midi.c \
	$(MIOS32_PATH)/mios32/common/mios32_iic_bs.c \
	$(MIOS32_PATH)/mios32/$(FAMILY)/mios32_bsl.c \
	$(MIOS32_PATH)/mios32/$(FAMILY)/mios32_sys.c \
	$(MIOS32_PATH)/mios32/$(FAMILY)/mios32_irq.c \
	$(MIOS32_PATH)/mios32/$(FAMILY)/mios32_srio.c \
	$(MIOS32_PATH)/mios32/$(FAMILY)/mios32_iic_midi.c \
	$(MIOS32_PATH)/mios32/$(FAMILY)/mios32_i2s.c \
	$(MIOS32_PATH)/mios32/$(FAMILY)/mios32_board.c \
	$(MIOS32_PATH)/mios32/$(FAMILY)/mios32_timer.c \
	$(MIOS32_PATH)/mios32/$(FAMILY)/mios32_stopwatch.c \
	$(MIOS32_PATH)/mios32/$(FAMILY)/mios32_delay.c \
	$(MIOS32_PATH)/mios32/$(FAMILY)/mios32_sdcard.c \
	$(MIOS32_PATH)/mios32/$(FAMILY)/mios32_ain.c \
	$(MIOS32_PATH)/mios32/$(FAMILY)/mios32_mf.c \
	$(MIOS32_PATH)/mios32/$(FAMILY)/mios32_usb.c \
	$(MIOS32_PATH)/mios32/$(FAMILY)/mios32_usb_midi.c \
	$(MIOS32_PATH)/mios32/$(FAMILY)/mios32_usb_com.c \
	$(MIOS32_PATH)/mios32/$(FAMILY)/mios32_uart.c \
	$(MIOS32_PATH)/mios32/$(FAMILY)/mios32_iic.c \
	$(MIOS32_PATH)/mios32/common/printf-stdarg.c


# MEMO: the gcc linker is clever enough to exclude functions from the final memory image
# if they are not references from the main routine - accordingly we can savely include
# the USB drivers without the danger that this increases the project size of applications,
# which don't use the USB peripheral at all :-)


# add family specific files
include $(MIOS32_PATH)/mios32/$(FAMILY)/mios32_family.mk


# directories and files that should be part of the distribution (release) package
DIST += $(MIOS32_PATH)/mios32/common \
        $(MIOS32_PATH)/mios32/mios32.mk \
        $(MIOS32_PATH)/include/mios32 \
        $(MIOS32_PATH)/doc/mios32
