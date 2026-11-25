// Force-included in every file of the emulator build (emu_macos): things the MIOS32 build normally gets from
// FreeRTOS / the Makefile. All "CPU" activity of the emulator runs under one global lock (see host/emu_os.c).
#ifndef _EMU_PRELUDE_H
#define _EMU_PRELUDE_H
#define MIOS32_BOARD_STR     "MBHP_CORE_STM32F4 (emulated on macOS)"
#define MIOS32_FAMILY_STR    "EMULATION"
#define MIOS32_PROCESSOR_STR "host"
extern void EMU_CriticalEnter(void);
extern void EMU_CriticalExit(void);
extern void EMU_Yield(void);
#define portENTER_CRITICAL() EMU_CriticalEnter()
#define portEXIT_CRITICAL()  EMU_CriticalExit()
#define taskYIELD()          EMU_Yield()
// modules/file/file.c calls unlink() in its emulation branch: route it to FatFs (the SD card image), never to the host
extern int EMU_Unlink(const char *path);
#define unlink EMU_Unlink
// newlib (STM32) leaves strtok_r's rest pointer at "" after the last token, macOS sets it to NULL; the SEQ terminal
// relies on the newlib behaviour (strlen(brkt))
extern char *EMU_strtok_r(char *str, const char *sep, char **lasts);
#define strtok_r EMU_strtok_r
#endif
