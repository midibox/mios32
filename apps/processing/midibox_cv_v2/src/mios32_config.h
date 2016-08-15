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
#define MIOS32_LCD_BOOT_MSG_LINE1 "MIDIboxCV V2.000"
#define MIOS32_LCD_BOOT_MSG_LINE2 "(C) 2014 T.Klose"

// define a unique VID/PID for this application
#define MIOS32_USB_PRODUCT_STR  "MIDIboxCV"

// only enable a single USB port by default to avoid USB issue under Win7 64bit
#define MIOS32_USB_MIDI_NUM_PORTS 1


#define DEBUG_MSG MIOS32_MIDI_SendDebugMessage

// reserved memory for FreeRTOS pvPortMalloc function
#define MIOS32_MINIMAL_STACK_SIZE 1500
#define MIOS32_HEAP_SIZE 13*1024


// use 16 DOUT pages
#define MIOS32_SRIO_NUM_DOUT_PAGES 16

// disables the default SRIO scan routine in programming_models/traditional/main.c
// allows to implement an own handler
// -> see app.c, APP_SRIO_*
#define MIOS32_DONT_SERVICE_SRIO_SCAN 1

// increased number of SRs
// (we are not using so many SRs... the intention is to enlarge the SRIO update cycle
// so that an update takes place ca. each 250 uS)
#define MIOS32_SRIO_NUM_SR 32


// for LPC17: simplify allocation of large arrays
#if defined(MIOS32_FAMILY_LPC17xx)
# define AHB_SECTION __attribute__ ((section (".bss_ahb")))
#else
# define AHB_SECTION
#endif


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
#if defined(MIOS32_FAMILY_STM32F10x) || defined(MIOS32_FAMILY_LPC17xx)
// reduced to 6 pins due to conflict with MIDI OUT3/IN3
#define MIOS32_AIN_CHANNEL_MASK 0x003f
#else
#define MIOS32_AIN_CHANNEL_MASK 0x00ff
#endif

// define the deadband (min. difference to report a change to the application hook)
// typically set to (2^(12-desired_resolution)-1)
// e.g. for a resolution of 7 bit, it's set to (2^(12-7)-1) = (2^5 - 1) = 31
#define MIOS32_AIN_DEADBAND 3

// Normally the ADC channels are converted each mS from the programming
// model (main.c) - optionally this can be skipped with
// #define MIOS32_DONT_SERVICE_AIN 1 in mios32_config.h
// 
// In this case, the MIOS32_AIN_StartConversions() function has to be called
// periodically from the application (e.g. from a timer), and conversion values
// can be retrieved with MIOS32_AIN_PinGet()
#define MIOS32_DONT_SERVICE_AIN 1


// enable ESP8266 support for OSC Server/Client
#define OSC_SERVER_ESP8266_ENABLED 1

// support direct send command
#define ESP8266_TERMINAL_DIRECT_SEND_CMD 1

// Mutex assignments
//#define ESP8266_MUTEX_MIDIOUT_TAKE { TASKS_MUTEX_MIDIOUT_Take(); }
//#define ESP8266_MUTEX_MIDIOUT_GIVE { TASKS_MUTEX_MIDIOUT_Give(); }

#endif /* _MIOS32_CONFIG_H */
