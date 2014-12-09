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
#define MIOS32_LCD_BOOT_MSG_LINE1 "MIDIbox NG V1.033"
#define MIOS32_LCD_BOOT_MSG_LINE2 "(C) 2014 T.Klose"

// define a unique VID/PID for this application
#define MIOS32_USB_PRODUCT_STR  "MIDIbox NG"

// enable 4 USB ports
#define MIOS32_USB_MIDI_NUM_PORTS 4

// increased number of SRs
#define MIOS32_SRIO_NUM_SR 32

// use 32 DOUT pages
#define MIOS32_SRIO_NUM_DOUT_PAGES 32

// disables the default SRIO scan routine in programming_models/traditional/main.c
// allows to implement an own handler
// -> see app.c, APP_SRIO_*
#define MIOS32_DONT_SERVICE_SRIO_SCAN 1

// special callback which will be called for DIN pin emulation
#define MIOS32_SRIO_CALLBACK_BEFORE_DIN_COMPARE APP_SRIO_ServiceFinishBeforeDINCompare

// enable 4 MIDI ports (note: MIDI3 only used if AIN ports disabled)
#if defined(MIOS32_FAMILY_STM32F10x)
// enable third UART
# define MIOS32_UART_NUM 3
#else
// enable third and fourth UART
# define MIOS32_UART_NUM 4
#endif


// enable 128 encoders (because SCS allocates one encoder as well)
#define MIOS32_ENC_NUM_MAX 129


// the SCS is entered by pressing the EXIT button
#define SCS_MENU_ENTERED_VIA_EXIT_BUTTON 1

// max. number of menu items (configurable with SCS num_items=...)
#define SCS_NUM_MENU_ITEMS 8


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
#define MIOS32_AIN_DEADBAND 31


#define DEBUG_MSG MIOS32_MIDI_SendDebugMessage

// enable two AINSER modules
#define AINSER_NUM_MODULES 2

// enable 32 AOUT channels
#define AOUT_NUM_CHANNELS 32
// configurable chip select pin
extern char mbng_patch_aout_spi_rc_pin;
#define AOUT_SPI_RC_PIN mbng_patch_aout_spi_rc_pin

// enable 1 MAX72xx chain with up to 16 devices
#define MAX72XX_NUM_CHAINS 1
#define MAX72XX_NUM_DEVICES_PER_CHAIN 16
// configurable chip select pin
extern char mbng_patch_max72xx_spi_rc_pin;
#define MAX72XX_SPI_RC_PIN_CHAIN1 mbng_patch_max72xx_spi_rc_pin


// reserved memory for FreeRTOS pvPortMalloc function
#define MIOS32_HEAP_SIZE 10*1024
// UMM heap located in default section (means for LPC17: not in AHB memory, because we are using it for the event pool)
#define UMM_HEAP_SECTION

// stack sizes which are used by various tasks (see APP_Init() in app.c)
#define APP_BIG_STACK_SIZE     (2048)
#define APP_REDUCED_STACK_SIZE (1024)
// for the MIOS32 hooks in main.c
#define MIOS32_MINIMAL_STACK_SIZE APP_BIG_STACK_SIZE
// for the UIP task
#define UIP_TASK_STACK_SIZE       APP_BIG_STACK_SIZE

// optionally for task analysis - if enabled, the stats can be displayed with the "system" command in MIOS Terminal
#if 0
#define configUSE_TRACE_FACILITY                1
#define configGENERATE_RUN_TIME_STATS           0
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

// Keyboard Handler
#define KEYBOARD_NOTIFY_TOGGLE_HOOK MBNG_KB_NotifyToggle
#define KEYBOARD_DONT_USE_MIDI_CFG 1
#define KEYBOARD_DONT_USE_AIN 1


// enable 4 SPI MIDI ports
// Note: SPI MIDI port has to be explicitely enabled by the user
// He has to enter "set spi_midi 1" in the bootloader update application
// Without this measure, the ENC28J60 based Ethernet Extension will be accessed via J16:RC2
// With "spi_midi" enable, the ENC28J60 driver will be disabled and OSC won't be available on board != LPC17 (which has an inbuilt ETH port)
#define MIOS32_SPI_MIDI_NUM_PORTS 4

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

// Mutex for J16 access
extern void APP_J16SemaphoreTake(void);
extern void APP_J16SemaphoreGive(void);
#define MIOS32_SDCARD_MUTEX_TAKE   { APP_J16SemaphoreTake(); }
#define MIOS32_SDCARD_MUTEX_GIVE   { APP_J16SemaphoreGive(); }
#define MIOS32_ENC28J60_MUTEX_TAKE { APP_J16SemaphoreTake(); }
#define MIOS32_ENC28J60_MUTEX_GIVE { APP_J16SemaphoreGive(); }
#define MIOS32_SPI_MIDI_MUTEX_TAKE { APP_J16SemaphoreTake(); }
#define MIOS32_SPI_MIDI_MUTEX_GIVE { APP_J16SemaphoreGive(); }

#endif /* _MIOS32_CONFIG_H */
