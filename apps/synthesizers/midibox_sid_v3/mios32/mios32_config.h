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
#define MIOS32_LCD_BOOT_MSG_LINE1 "MIDIbox SID V3 Alpha"
#define MIOS32_LCD_BOOT_MSG_LINE2 "(C) 2009 T. Klose"

// USB settings
#define MIOS32_USB_PRODUCT_STR  "MIDIbox SID V3"
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
// see documentation under http://www.midibox.org/mios32/manual/group___f_r_e_e_r_t_o_s___u_t_i_l_s.html
#define configGENERATE_RUN_TIME_STATS           0
#define portCONFIGURE_TIMER_FOR_RUN_TIME_STATS  FREERTOS_UTILS_PerfCounterInit
#define portGET_RUN_TIME_COUNTER_VALUE          FREERTOS_UTILS_PerfCounterGet


// maximum idle counter value to be expected
#define MAX_IDLE_CTR 223000

#endif /* _MIOS32_CONFIG_H */
