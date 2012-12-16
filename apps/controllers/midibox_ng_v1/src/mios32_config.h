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
#define MIOS32_LCD_BOOT_MSG_LINE1 "MIDIbox NG V1.000"
#define MIOS32_LCD_BOOT_MSG_LINE2 "(C) 2012 T.Klose"

// define a unique VID/PID for this application
#define MIOS32_USB_PRODUCT_STR  "MIDIbox NG"

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


// enable 65 encoders (because SCS allocates one encoder as well)
#define MIOS32_ENC_NUM_MAX 65


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
#define MIOS32_HEAP_SIZE 12*1024
// UMM heap located in default section (means for LPC17: not in AHB memory, because we are using it for the event pool)
#define UMM_HEAP_SECTION

// optionally for task analysis - if enabled, the stats can be displayed with the "system" command in MIOS Terminal
#if 0
#define configGENERATE_RUN_TIME_STATS           1
#define portCONFIGURE_TIMER_FOR_RUN_TIME_STATS  FREERTOS_UTILS_PerfCounterInit
#define portGET_RUN_TIME_COUNTER_VALUE          FREERTOS_UTILS_PerfCounterGet
#endif

// for LPC17: simplify allocation of large arrays
#if defined(MIOS32_FAMILY_LPC17xx)
# define AHB_SECTION __attribute__ ((section (".bss_ahb")))
#else
# define AHB_SECTION
#endif

// LPC17 Ethernet driver: locate buffers to lower (default) section
// (no attribute passed to this variable)
#define LPC17XX_EMAC_MEM_SECTION
// reduce number of buffers to save memory
#define LPC17XX_EMAC_NUM_RX_FRAG 2
#define LPC17XX_EMAC_NUM_TX_FRAG 2
#define LPC17XX_EMAC_FRAG_SIZE   1024


// size of SysEx buffers
// if longer SysEx strings are received, they will be forwarded directly
// in this case, multiple strings concurrently sent to the same port won't be merged correctly anymore.
#define MIDI_ROUTER_SYSEX_BUFFER_SIZE 16

// BUFLCD driver should support GLCD Font Selection
#define BUFLCD_NUM_DEVICES          1
#define BUFLCD_COLUMNS_PER_DEVICE  64
#define BUFLCD_MAX_LINES            4
#define BUFLCD_SUPPORT_GLCD_FONTS   1

#endif /* _MIOS32_CONFIG_H */
