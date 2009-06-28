// $Id$
/*
 * Local MIOS32 configuration file
 *
 * this file allows to disable (or re-configure) default functions of MIOS32
 * available switches are listed in $MIOS32_PATH/modules/mios32/MIOS32_CONFIG.txt
 *
 */

#ifndef _MIOS32_CONFIG_H
#define _MIOS32_CONFIG_H

// The boot message which is print during startup and returned on a SysEx query
#define MIOS32_LCD_BOOT_MSG_LINE1 "MIDIbox SEQ V4.0Beta"
#define MIOS32_LCD_BOOT_MSG_LINE2 "(C) 2009 T. Klose"

// function used to output debug messages (must be printf compatible!)
#define DEBUG_MSG UI_printf


#define MID_PLAYER_TEST 0

// not supported by MacOS emulation:
//#define MIOS32_DONT_USE_SYS
#define MIOS32_DONT_USE_IRQ
//#define MIOS32_DONT_USE_SRIO
//#define MIOS32_DONT_USE_DIN
//#define MIOS32_DONT_USE_DOUT
//#define MIOS32_DONT_USE_ENC
#define MIOS32_DONT_USE_AIN
#define MIOS32_DONT_USE_MF
//#define MIOS32_DONT_USE_LCD
//#define MIOS32_DONT_USE_MIDI
//#define MIOS32_DONT_USE_COM
#define MIOS32_DONT_USE_USB
#define MIOS32_DONT_USE_USB_MIDI
#define MIOS32_USE_USB_COM
//#define MIOS32_DONT_USE_UART
//#define MIOS32_DONT_USE_UART_MIDI
#define MIOS32_DONT_USE_IIC
#define MIOS32_DONT_USE_IIC_MIDI
#define MIOS32_USE_I2S
//#define MIOS32_DONT_USE_BOARD
//#define MIOS32_DONT_USE_TIMER
#define MIOS32_DONT_USE_DELAY
//#define MIOS32_DONT_USE_SDCARD


#define MIOS32_MIDI_DEFAULT_PORT UART0

#define MIOS32_UART_NUM 4
#define MIOS32_UART_RX_BUFFER_SIZE 4096
#define MIOS32_UART_TX_BUFFER_SIZE 4096


// memory alloccation method:
// 0: internal static allocation with one byte for each flag
// 1: internal static allocation with 8bit flags
// 2: internal static allocation with 16bit flags
// 3: internal static allocation with 32bit flags
// 4: FreeRTOS based pvPortMalloc
// 5: malloc provided by library
#define SEQ_MIDI_OUT_MALLOC_METHOD 0

// max number of scheduled events which will allocate memory
// each event allocates 12 bytes
// MAX_EVENTS must be a power of two! (e.g. 64, 128, 256, 512, ...)
#define SEQ_MIDI_OUT_MAX_EVENTS 8192

// enable seq_midi_out_max_allocated and seq_midi_out_dropouts
#define SEQ_MIDI_OUT_MALLOC_ANALYSIS 1

// configure BLM driver
#define BLM_DOUT_L1_SR	255 // dummy, will be changed in seq_file_hw.c
#define BLM_DOUT_R1_SR	255 // dummy, will be changed in seq_file_hw.c
#define BLM_DOUT_CATHODES_SR1	255 // dummy, will be changed in seq_file_hw.c
#define BLM_DOUT_CATHODES_SR2	255 // dummy, will be changed in seq_file_hw.c
#define BLM_CATHODES_INV_MASK	0x00
#define BLM_DOUT_L2_SR	255 // dummy, will be changed in seq_file_hw.c
#define BLM_DOUT_R2_SR	255 // dummy, will be changed in seq_file_hw.c
#define BLM_DOUT_L3_SR	0 // not used
#define BLM_DOUT_R3_SR	0 // not used
#define BLM_DIN_L_SR	255 // dummy, will be changed in seq_file_hw.c
#define BLM_DIN_R_SR	255 // dummy, will be changed in seq_file_hw.c
#define BLM_NUM_COLOURS 2
#define BLM_NUM_ROWS    8
#define BLM_DEBOUNCE_MODE 1


// configure BLM_X driver (not supported by emulation!)
#define BLM_X_NUM_ROWS            8
#define BLM_X_BTN_NUM_COLS        8
#define BLM_X_LED_NUM_COLS        8
#define BLM_X_LED_NUM_COLORS      1
#define BLM_X_ROWSEL_DOUT_SR      255 // dummy, will be changed in seq_file_hw.c
#define BLM_X_LED_FIRST_DOUT_SR   255 // dummy, will be changed in seq_file_hw.c
#define BLM_X_BTN_FIRST_DIN_SR    255 // dummy, will be changed in seq_file_hw.c
#define BLM_X_ROWSEL_INV_MASK     0   // dummy, will be changed in seq_file_hw.c
#define BLM_X_DEBOUNCE_MODE       0


#endif /* _MIOS32_CONFIG_H */
