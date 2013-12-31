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
#define MIOS32_LCD_BOOT_MSG_LINE2 "(C) 2011 T.Klose"

// define a unique VID/PID for this application
#define MIOS32_USB_PRODUCT_STR  "MIDIboxCV"

// only enable a single USB port by default to avoid USB issue under Win7 64bit
#define MIOS32_USB_MIDI_NUM_PORTS 1


#define DEBUG_MSG MIOS32_MIDI_SendDebugMessage

// reserved memory for FreeRTOS pvPortMalloc function
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


#endif /* _MIOS32_CONFIG_H */
