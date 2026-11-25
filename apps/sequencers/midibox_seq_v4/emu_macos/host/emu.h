// MIDIbox SEQ emulator - shared declarations of the host layer
#ifndef _EMU_H
#define _EMU_H
#define EMU_PORT_NAME "MBSEQ EMU Port"
extern void EMU_CpuLock(void);
extern void EMU_CpuUnlock(void);
extern s32 EMU_Setup(int argc, char **argv);      // emu_mios32.c: options, SD card image, CoreMIDI
extern void EMU_Shutdown(void);
extern s32 EMU_MIDI_Init(void);
extern void EMU_MIDI_Flush(void);                  // hand the collected MIDI output to CoreMIDI (every ms)                    // emu_coremidi.c
extern void EMU_CpuSleepUs(unsigned us);                // sleep with the CPU lock released (blocking waits)
#endif
