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
#define MIOS32_LCD_BOOT_MSG_DELAY 0 // we delay the boot and print a message inside the app
//                                <---------------------->
#define MIOS32_LCD_BOOT_MSG_LINE1 "MIDIbox SEQ V4.0Beta2 "
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
#define MIOS32_HEAP_SIZE 19*1024


// optional performance measuring
// see documentation under $MIOS32_PATH/modules/freertos_utils/freertos_utils.c
#define configGENERATE_RUN_TIME_STATS           0
#define portCONFIGURE_TIMER_FOR_RUN_TIME_STATS  FREERTOS_UTILS_PerfCounterInit
#define portGET_RUN_TIME_COUNTER_VALUE          FREERTOS_UTILS_PerfCounterGet


// maximum idle counter value to be expected
#define MAX_IDLE_CTR 223000


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


// configure IIC_MIDI
#define MIOS32_IIC_MIDI_NUM 4
// all interfaces are "OUT only"
#define MIOS32_IIC_MIDI0_ENABLED    1
#define MIOS32_IIC_MIDI1_ENABLED    1
#define MIOS32_IIC_MIDI2_ENABLED    1
#define MIOS32_IIC_MIDI3_ENABLED    1
#define MIOS32_IIC_MIDI4_ENABLED    1
#define MIOS32_IIC_MIDI5_ENABLED    1
#define MIOS32_IIC_MIDI6_ENABLED    1
#define MIOS32_IIC_MIDI7_ENABLED    1


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


// configure BLM_X driver
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
