// $Id: mios32_config.h 285 2009-01-11 17:25:57Z tk $
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
#define MIOS32_LCD_BOOT_MSG_LINE1 "MIDIbox SEQ V4.0Alpha"
#define MIOS32_LCD_BOOT_MSG_LINE2 "(C) 2009 T. Klose"


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


// the speed value for the datawheel (#0) which is used when the "FAST" button is activated:
#define DEFAULT_DATAWHEEL_SPEED_VALUE	3

// the speed value for the additional encoders (#1-#16) which is used when the "FAST" button is activated:
#define DEFAULT_ENC_SPEED_VALUE		3

// Auto FAST mode: if a layer is assigned to velocity or CC, the fast button will be automatically
// enabled - in other cases (e.g. Note or Length), the fast button will be automatically disabled
#define DEFAULT_AUTO_FAST_BUTTON        1


// Toggle behaviour of various buttons
// 0: active mode so long button pressed
// 1: pressing button toggles the mode
#define DEFAULT_BEHAVIOUR_BUTTON_FAST	1
#define DEFAULT_BEHAVIOUR_BUTTON_ALL	1
#define DEFAULT_BEHAVIOUR_BUTTON_SOLO	1
#define DEFAULT_BEHAVIOUR_BUTTON_METRON	1
#define DEFAULT_BEHAVIOUR_BUTTON_SCRUB	1
#define DEFAULT_BEHAVIOUR_BUTTON_MENU	1


// include SRIO setup here, so that we can propagate values to external modules
#include "srio_mapping.h"

// forward to BLM8x8 driver
#define BLM8X8_DOUT	          DEFAULT_SRM_DOUT_M
#define BLM8X8_DOUT_CATHODES	  DEFAULT_SRM_DOUT_CATHODESM
#define BLM8X8_CATHODES_INV_MASK  DEFAULT_SRM_CATHODES_INV_MASK_M
#define BLM8X8_DIN	          DEFAULT_SRM_DIN_M

#endif /* _MIOS32_CONFIG_H */
