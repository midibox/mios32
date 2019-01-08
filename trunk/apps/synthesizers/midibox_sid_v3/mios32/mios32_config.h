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
// xTaskCreate(TASK_Period1mS, "Period1mS", configMINIMAL_STACK_SIZE, NULL, PRIORITY_TASK_PERIOD1MS, NULL);
#define MIOS32_MINIMAL_STACK_SIZE 1024

// reserved memory for FreeRTOS pvPortMalloc function
#define MIOS32_HEAP_SIZE 19*1024


// optional performance measuring
// see documentation under http://www.midibox.org/mios32/manual/group___f_r_e_e_r_t_o_s___u_t_i_l_s.html
#define configGENERATE_RUN_TIME_STATS           0
#if configGENERATE_RUN_TIME_STATS
#define portCONFIGURE_TIMER_FOR_RUN_TIME_STATS  FREERTOS_UTILS_PerfCounterInit
#define portGET_RUN_TIME_COUNTER_VALUE          FREERTOS_UTILS_PerfCounterGet
#endif


// maximum idle counter value to be expected
#define MAX_IDLE_CTR 223000


// to enable SID emulation
//#define SIDEMU_ENABLED
#define SIDEMU_EXTERNAL_DAC
#define SIDEMU_INTERNAL_DAC

#ifdef SIDEMU_ENABLED
#define SIDPHYS_DISABLED
#define MIOS32_DONT_USE_SRIO
#define MIOS32_DONT_USE_ENC
#define MIOS32_DONT_USE_AIN
#define MIOS32_DONT_USE_COM

#define MIOS32_I2S_AUDIO_FREQ 25000

#ifdef SIDEMU_EXTERNAL_DAC
  // I2S device connected to J8 (-> SPI1), therefore we have to use SPI0 (-> J16) for SRIO chain
# define MIOS32_SRIO_SPI 0

  // I2S support has to be enabled explicitely
# define MIOS32_USE_I2S

// enable MCLK pin (not for STM32 primer)
#ifdef MIOS32_BOARD_STM32_PRIMER
# define MIOS32_I2S_MCLK_ENABLE  0
#else
# define MIOS32_I2S_MCLK_ENABLE  1
#endif

#endif

#endif

// MSD not enabled yet...
#undef USE_MSD


// MBNet Config:
// relevant if configured as master: how many nodes should be scanned maximum
#define SID_USE_MBNET           1
#define MBNET_SLAVE_NODES_MAX   4
#define MBNET_SLAVE_NODES_BEGIN 0x00
#define MBNET_SLAVE_NODES_END   0x03
#define MBNET_NODE_SCAN_RETRY   32


#endif /* _MIOS32_CONFIG_H */
