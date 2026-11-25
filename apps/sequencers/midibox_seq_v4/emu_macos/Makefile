# MIDIbox SEQ V4+ emulator for macOS: the real SEQ firmware (apps/sequencers/midibox_seq_v4/core) and the real MIOS32
# common code, compiled for the host with MIOS32_FAMILY_EMULATION; the hardware layer is in host/ (pthreads, CoreMIDI,
# SD card = disk image). Build: make      Run: ./run.sh   (see README.md)
SEQ  := ..
R    := ../../../..
M    := $(R)/modules
CC   := clang
DEFS := -DMIOS32_FAMILY_EMULATION -DMIOS32_BOARD_MBHP_CORE_STM32F4 -DMIOS32_PROCESSOR_STM32F407VG
INC  := -I host -I $(SEQ)/core -I $(R)/include/mios32 -I $(M)/sequencer -I $(M)/notestack \
        -I $(M)/midifile -I $(M)/fatfs/src -I $(M)/file -I $(M)/random -I $(M)/blm -I $(M)/blm_scalar_master -I $(M)/aout \
        -I $(M)/glcd_font -I $(M)/app_lcd/universal -I $(M)/uip_task_standard -I $(M)/uip/uip -I $(M)/ws2812
CFLAGS := -O1 -g -w -include host/emu_prelude.h $(DEFS) $(INC)
LDFLAGS := -framework CoreMIDI -framework CoreFoundation -framework CoreServices -lpthread

SEQ_SRC := $(addprefix $(SEQ)/,$(shell grep -oE 'core/[a-z0-9_]+\.c' $(SEQ)/Makefile))
MOD_SRC := $(M)/sequencer/seq_bpm.c $(M)/sequencer/seq_midi_out.c $(M)/notestack/notestack.c $(M)/midifile/mid_parser.c \
           $(M)/fatfs/src/ff.c $(M)/fatfs/src/option/ccsbcs.c $(M)/fatfs/src/diskio.c $(M)/file/file.c $(M)/random/jsw_rand.c \
           $(M)/blm/blm.c $(M)/blm_scalar_master/blm_scalar_master.c $(M)/aout/aout.c \
           $(wildcard $(M)/glcd_font/glcd_font_*.c)
MIOS_SRC := $(addprefix $(R)/mios32/common/,mios32_midi.c mios32_din.c mios32_dout.c mios32_enc.c mios32_srio.c \
            mios32_timestamp.c mios32_com.c mios32_lcd.c mios32_uart_midi.c mios32_spi_midi.c mios32_osc.c mios32_mf.c)
HOST_SRC := $(wildcard host/*.c)

OBJ := $(patsubst %.c,build/%.o,$(notdir $(SEQ_SRC) $(MOD_SRC) $(MIOS_SRC) $(HOST_SRC)))
VPATH := $(sort $(dir $(SEQ_SRC) $(MOD_SRC) $(MIOS_SRC) $(HOST_SRC)))

mbseq_emu: $(OBJ)
	$(CC) -o $@ $(OBJ) $(LDFLAGS)

build/%.o: %.c host/emu_prelude.h | build
	$(CC) $(CFLAGS) -c -o $@ $<

build:
	mkdir -p build

clean:
	rm -rf build mbseq_emu
.PHONY: clean
