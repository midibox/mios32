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
#define MIOS32_LCD_BOOT_MSG_LINE1 "MIDIbox SEQ V4.0Alpha"
#define MIOS32_LCD_BOOT_MSG_LINE2 "(C) 2009 T. Klose"

// USB settings
#define MIOS32_USB_PRODUCT_STR  "MIDIbox SEQ V4"
#define MIOS32_USB_MIDI_NUM_PORTS 4


// port used for debugging via MIDI
//#define MIOS32_MIDI_DEBUG_PORT USB0

// function used to output debug messages (must be printf compatible!)
#define DEBUG_MSG MIOS32_MIDI_SendDebugMessage


// Stack size for FreeRTOS tasks as defined by the programming model
// Note that each task maintains it's own stack!
// If you want to define a different stack size for your application tasks
// (-> xTaskCreate() function), keep in mind that it has to be divided by 4,
// since the stack width of ARM is 32bit.
// The FreeRTOS define "configMINIMAL_STACK_SIZE" is (MIOS32_MINIMAL_STACK_SIZE/4)
// it can be used in applications as well, e.g.
// xTaskCreate(TASK_Period1mS, (signed portCHAR *)"Period1mS", configMINIMAL_STACK_SIZE, NULL, PRIORITY_TASK_PERIOD1MS, NULL);
#define MIOS32_MINIMAL_STACK_SIZE 1024

// reserved memory for FreeRTOS pvPortMalloc function
#define MIOS32_HEAP_SIZE 18*1024


// memory alloccation method:
// 0: internal static allocation with one byte for each flag
// 1: internal static allocation with 8bit flags
// 2: internal static allocation with 16bit flags
// 3: internal static allocation with 32bit flags
// 4: FreeRTOS based pvPortMalloc
// 5: malloc provided by library
#define SEQ_MIDI_OUT_MALLOC_METHOD 3

// max number of scheduled events which will allocate memory
// each event allocates 12 bytes
// MAX_EVENTS must be a power of two! (e.g. 64, 128, 256, 512, ...)
#define SEQ_MIDI_OUT_MAX_EVENTS 512

// enable seq_midi_out_max_allocated and seq_midi_out_dropouts
#define SEQ_MIDI_OUT_MALLOC_ANALYSIS 1


#define MID_PLAYER_TEST 0

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
#define DEFAULT_BEHAVIOUR_BUTTON_SCRUB	0
#define DEFAULT_BEHAVIOUR_BUTTON_MENU	0
#define DEFAULT_BEHAVIOUR_BUTTON_STEPVIEW 0
#define DEFAULT_BEHAVIOUR_BUTTON_TRG_LAYER 0
#define DEFAULT_BEHAVIOUR_BUTTON_PAR_LAYER 0


// include SRIO setup here, so that we can propagate values to external modules
#include "srio_mapping.h"

// configure BLM_X driver
#define BLM_X_NUM_ROWS            8
#define BLM_X_BTN_NUM_COLS        8
#define BLM_X_LED_NUM_COLS        8
#define BLM_X_LED_NUM_COLORS      1
#define BLM_X_ROWSEL_DOUT_SR      DEFAULT_SRM_DOUT_CATHODESM-1
#define BLM_X_LED_FIRST_DOUT_SR   DEFAULT_SRM_DOUT_M-1
#define BLM_X_BTN_FIRST_DIN_SR    DEFAULT_SRM_DIN_M-1
#define BLM_X_ROWSEL_INV_MASK     DEFAULT_SRM_CATHODES_INV_MASK_M
#define BLM_X_DEBOUNCE_MODE       1 // no mode 0?

#endif /* _MIOS32_CONFIG_H */
