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
#define MIOS32_LCD_BOOT_MSG_LINE1 "MIDIO 128 V3.013"
#define MIOS32_LCD_BOOT_MSG_LINE2 "(C) 2012 T.Klose"

// define a unique VID/PID for this application
#define MIOS32_USB_PRODUCT_STR  "MIDIO128"

// enable 4 USB ports
#define MIOS32_USB_MIDI_NUM_PORTS 4


// enable 4 MIDI ports (note: MIDI3 only used if AIN ports disabled)
#if defined(MIOS32_FAMILY_STM32F10x)
// enable third UART
# define MIOS32_UART_NUM 3
#else
// enable third and fourth UART
# define MIOS32_UART_NUM 4
#endif

// optionally the number of SRs can be increased here
#define MIOS32_SRIO_NUM_SR 16

// AIN configuration:

// bit mask to enable channels
//
// Pin mapping on MBHP_CORE_STM32 module:
//   15       14      13     12     11     10      9      8   
// J16.SO  J16.SI  J16.SC J16.RC J5C.A11 J5C.A10 J5C.A9 J5C.A8
//   7        6       5      4      3      2      1       0
// J5B.A7  J5B.A6  J5B.A5 J5B.A4 J5A.A3 J5A.A2 J5A.A1  J5A.A0
//
// Examples:
//   mask 0x000f will enable all J5A channels
//   mask 0x00f0 will enable all J5B channels
//   mask 0x0f00 will enable all J5C channels
//   mask 0x0fff will enable all J5A/B/C channels
// (all channels are disabled by default)
#define MIOS32_AIN_CHANNEL_MASK 0x003f

// define the deadband (min. difference to report a change to the application hook)
// typically set to (2^(12-desired_resolution)-1)
// e.g. for a resolution of 7 bit, it's set to (2^(12-7)-1) = (2^5 - 1) = 31
#define MIOS32_AIN_DEADBAND 31


#define DEBUG_MSG MIOS32_MIDI_SendDebugMessage

// enable two AINSER modules
#define AINSER_NUM_MODULES 2

// reserved memory for FreeRTOS pvPortMalloc function
#define MIOS32_HEAP_SIZE 14*1024


// combine MIDI router with SEQ module
#define MIDI_ROUTER_COMBINED_WITH_SEQ 1

// map MIDI mutex to UIP task
// located in app.c to access MIDI IN/OUT mutex from external
extern void APP_MUTEX_MIDIOUT_Take(void);
extern void APP_MUTEX_MIDIOUT_Give(void);
extern void APP_MUTEX_MIDIIN_Take(void);
extern void APP_MUTEX_MIDIIN_Give(void);
#define UIP_TASK_MUTEX_MIDIOUT_TAKE { APP_MUTEX_MIDIOUT_Take(); }
#define UIP_TASK_MUTEX_MIDIOUT_GIVE { APP_MUTEX_MIDIOUT_Give(); }
#define UIP_TASK_MUTEX_MIDIIN_TAKE  { APP_MUTEX_MIDIIN_Take(); }
#define UIP_TASK_MUTEX_MIDIIN_GIVE  { APP_MUTEX_MIDIIN_Give(); }

#endif /* _MIOS32_CONFIG_H */
