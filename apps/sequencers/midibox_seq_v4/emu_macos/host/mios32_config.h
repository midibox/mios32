// Emulator: the SEQ's own mios32_config.h, compiled for MIOS32_FAMILY_EMULATION (emu_macos)
#ifndef _EMU_MIOS32_CONFIG_H
#define _EMU_MIOS32_CONFIG_H
#include "../../../apps/sequencers/midibox_seq_v4/mios32/mios32_config.h"
#endif
// emulator: no IIC MIDI / SPI MIDI / UART hardware (the MIDI ports are CoreMIDI "USB" ports)
#ifndef MIOS32_DONT_USE_IIC_MIDI
#define MIOS32_DONT_USE_IIC_MIDI
#endif
