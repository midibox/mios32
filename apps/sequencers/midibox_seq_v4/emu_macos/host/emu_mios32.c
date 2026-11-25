// MIDIbox SEQ emulator - MIOS32 hardware layer for macOS: SYS, DELAY, BOARD, SPI (SRIO chain), UART (none), STOPWATCH,
// SD card (FAT disk image), application LCD (output goes to the virtual panel via the SEQ remote protocol), WS2812, OSC.
#include <mios32.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/time.h>
#include "emu.h"
typedef unsigned long long u64;

static struct timespec t0;
static FILE *sd = NULL;
static char sd_path[1024] = "sdcard.img";
static u32 sd_sectors = 0;

static u64 now_us(void)
{
  struct timespec t; clock_gettime(CLOCK_MONOTONIC, &t);
  return (u64)(t.tv_sec - t0.tv_sec) * 1000000ULL + (t.tv_nsec - t0.tv_nsec) / 1000;
}

s32 EMU_Setup(int argc, char **argv)
{
  clock_gettime(CLOCK_MONOTONIC, &t0);
  for(int i=1; i<argc; ++i) {
    if( !strcmp(argv[i], "--sd") && i+1 < argc ) strncpy(sd_path, argv[++i], sizeof(sd_path)-1);
    else { fprintf(stderr, "usage: %s [--sd sdcard.img]\n", argv[0]); return -1; }
  }
  sd = fopen(sd_path, "r+b");
  if( sd ) {
    fseek(sd, 0, SEEK_END); sd_sectors = (u32)(ftell(sd) / 512);
    printf("[EMU] SD card: %s (%u MB)\n", sd_path, sd_sectors / 2048);
  } else {
    printf("[EMU] no SD card image '%s' - run ./make_sdcard.sh first (SEQ runs without SD card)\n", sd_path);
  }
  return EMU_MIDI_Init();
}
void EMU_Shutdown(void) { if( sd ) fclose(sd); }

// ---- SYS / DELAY / STOPWATCH ----
s32 MIOS32_SYS_Init(u32 mode) { return 0; }
s32 MIOS32_SYS_Reset(void) { printf("[EMU] MIOS32_SYS_Reset() - ignored\n"); return 0; }
u32 MIOS32_SYS_ChipIDGet(void) { return 0x0413; }
u32 MIOS32_SYS_FlashSizeGet(void) { return 1024*1024; }
u32 MIOS32_SYS_RAMSizeGet(void) { return 192*1024; }
s32 MIOS32_SYS_SerialNumberGet(char *str) { strcpy(str, "EMU00000000000000000000"); return 0; }
mios32_sys_time_t MIOS32_SYS_TimeGet(void)
{
  u64 us = now_us();
  mios32_sys_time_t t = { .seconds = (u32)(us / 1000000ULL), .fraction_ms = (u16)((us / 1000) % 1000) };
  return t;
}
s32 MIOS32_SYS_TimeSet(mios32_sys_time_t t) { return 0; }
s32 MIOS32_DELAY_Init(u32 mode) { return 0; }
s32 MIOS32_DELAY_Wait_uS(u16 uS) { usleep(uS); return 0; }
static u64 sw_start;
s32 MIOS32_STOPWATCH_Init(u32 resolution) { sw_start = now_us(); return 0; }
s32 MIOS32_STOPWATCH_Reset(void) { sw_start = now_us(); return 0; }
u32 MIOS32_STOPWATCH_ValueGet(void) { return (u32)(now_us() - sw_start); }

// ---- BOARD: no LEDs, J5/J15/DAC pins go nowhere ----
s32 MIOS32_BOARD_Init(u32 mode) { return 0; }
static u32 board_leds;
s32 MIOS32_BOARD_LED_Init(u32 leds) { return 0; }
s32 MIOS32_BOARD_LED_Set(u32 leds, u32 value) { board_leds = (board_leds & ~leds) | (value & leds); return 0; }
u32 MIOS32_BOARD_LED_Get(void) { return board_leds; }
s32 MIOS32_BOARD_J5_PinInit(u8 pin, mios32_board_pin_mode_t mode) { return 0; }
s32 MIOS32_BOARD_J5_PinSet(u8 pin, u8 value) { return 0; }
s32 MIOS32_BOARD_J5_PinGet(u8 pin) { return 1; }
s32 MIOS32_BOARD_J15_PortInit(u32 mode) { return 0; }
s32 MIOS32_BOARD_J15_DataSet(u8 data) { return 0; }
s32 MIOS32_BOARD_J15_SerDataShift(u8 data) { return 0; }
s32 MIOS32_BOARD_J15_RS_Set(u8 rs) { return 0; }
s32 MIOS32_BOARD_DAC_PinInit(u8 chn, u8 enable) { return 0; }
s32 MIOS32_BOARD_DAC_PinSet(u8 chn, u16 value) { return 0; }

// ---- SPI: the SRIO chain. DIN inputs are pulled up (no button pressed: 0xff); buttons come from the remote panel ----
u8 emu_srio_dout[64];
s32 MIOS32_SPI_Init(u32 mode) { return 0; }
s32 MIOS32_SPI_IO_Init(u8 spi, mios32_spi_pin_driver_t spi_pin_driver) { return 0; }
s32 MIOS32_SPI_TransferModeInit(u8 spi, mios32_spi_mode_t spi_mode, mios32_spi_prescaler_t spi_prescaler) { return 0; }
s32 MIOS32_SPI_RC_PinSet(u8 spi, u8 rc_pin, u8 pin_value) { return 0; }
s32 MIOS32_SPI_TransferByte(u8 spi, u8 b) { return 0xff; }
s32 MIOS32_SPI_TransferBlock(u8 spi, u8 *send_buffer, u8 *receive_buffer, u16 len, void *callback)
{
  if( send_buffer ) memcpy(emu_srio_dout, send_buffer, len > sizeof(emu_srio_dout) ? sizeof(emu_srio_dout) : len);
  if( receive_buffer ) memset(receive_buffer, 0xff, len);
  if( callback ) ((void (*)(void))callback)();       // "DMA" finished immediately
  return 0;
}

// ---- UART: no physical MIDI ports in the emulator ----
s32 MIOS32_UART_Init(u32 mode) { return 0; }
s32 MIOS32_UART_IsAssignedToMIDI(u8 uart) { return 0; }
s32 MIOS32_UART_RxBufferGet(u8 uart) { return -1; }
s32 MIOS32_UART_TxBufferPutMore_NonBlocking(u8 uart, u8 *buffer, u16 len) { return 0; }
s32 MIOS32_UART_TxBufferPutMore(u8 uart, u8 *buffer, u16 len) { return 0; }
s32 MIOS32_UART_BaudrateSet(u8 uart, u32 baudrate) { return 0; }
u32 MIOS32_UART_BaudrateGet(u8 uart) { return 31250; }

// ---- SD card = disk image (512-byte sectors) ----
s32 MIOS32_SDCARD_Init(u32 mode) { return 0; }
s32 MIOS32_SDCARD_PowerOn(void) { return sd ? 0 : -1; }
s32 MIOS32_SDCARD_PowerOff(void) { return 0; }
s32 MIOS32_SDCARD_CheckAvailable(u8 was_available) { return sd ? 1 : 0; }
s32 MIOS32_SDCARD_SectorRead(u32 sector, u8 *buffer)
{
  if( !sd || sector >= sd_sectors ) return -1;
  fseek(sd, (long)sector * 512, SEEK_SET);
  return fread(buffer, 1, 512, sd) == 512 ? 0 : -2;
}
s32 MIOS32_SDCARD_SectorWrite(u32 sector, u8 *buffer)
{
  if( !sd || sector >= sd_sectors ) return -1;
  fseek(sd, (long)sector * 512, SEEK_SET);
  s32 r = fwrite(buffer, 1, 512, sd) == 512 ? 0 : -2;
  fflush(sd);
  return r;
}
s32 MIOS32_SDCARD_CIDRead(mios32_sdcard_cid_t *cid) { memset(cid, 0, sizeof(*cid)); strcpy((char *)cid->ProdName, "EMU"); return 0; }
s32 MIOS32_SDCARD_CSDRead(mios32_sdcard_csd_t *csd) { memset(csd, 0, sizeof(*csd)); return 0; }

// ---- LCD: the SEQ sends its screen to the virtual panel itself (remote server mode); no local display ----
s32 APP_LCD_Init(u32 mode) { return 0; }
s32 APP_LCD_Data(u8 data) { return 0; }
s32 APP_LCD_Cmd(u8 cmd) { return 0; }
s32 APP_LCD_Clear(void) { return 0; }
s32 APP_LCD_CursorSet(u16 column, u16 line) { return 0; }
s32 APP_LCD_GCursorSet(u16 x, u16 y) { return 0; }
s32 APP_LCD_SpecialCharInit(u8 num, u8 table[8]) { return 0; }
s32 APP_LCD_BColourSet(u32 rgb) { return 0; }
s32 APP_LCD_FColourSet(u32 rgb) { return 0; }
s32 APP_LCD_BitmapPixelSet(mios32_lcd_bitmap_t bitmap, u16 x, u16 y, u32 colour) { return 0; }
s32 APP_LCD_BitmapPrint(mios32_lcd_bitmap_t bitmap) { return 0; }

// ---- RGB LEDs and OSC/Ethernet: not emulated ----
s32 WS2812_Init(u32 mode) { return 0; }
s32 WS2812_LED_SetRGB(u16 led, u8 colour, u8 value) { return 0; }
s32 OSC_CLIENT_SendMIDIEvent(u8 osc_port, mios32_midi_package_t p) { return -1; }
s32 OSC_SERVER_SendPacket(u8 con, u8 *packet, u32 len) { return -1; }

// ---- C library: newlib-compatible strtok_r (see emu_prelude.h) ----
#undef strtok_r
extern char *strtok_r(char *str, const char *sep, char **lasts);
char *EMU_strtok_r(char *str, const char *sep, char **lasts)
{
  static char empty[1] = "";
  char *r = strtok_r(str, sep, lasts);
  if( *lasts == NULL ) *lasts = empty;
  return r;
}

// FILE_Remove(): delete on the SD card image through FatFs (returns 0 = FR_OK, like f_unlink)
#include "ff.h"
int EMU_Unlink(const char *path) { return (int)f_unlink(path); }

// The firmware's data types must have the sizes they have on the STM32 (session files are written with them)
#include "seq_song.h"
_Static_assert(sizeof(u32) == 4 && sizeof(s32) == 4 && sizeof(DWORD) == 4, "32 bit types");
_Static_assert(sizeof(seq_song_step_t) == 8, "seq_song_step_t must be 8 bytes like on the STM32");
