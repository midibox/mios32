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
#define MIOS32_LCD_BOOT_MSG_LINE1 "MIDIO 128 V3.0"
#define MIOS32_LCD_BOOT_MSG_LINE2 "(C) 2011 T.Klose"

// define a unique VID/PID for this application
#define MIOS32_USB_PRODUCT_STR  MIOS32_LCD_BOOT_MSG_LINE1

// only enable a single USB port by default to avoid USB issue under Win7 64bit
#define MIOS32_USB_MIDI_NUM_PORTS 1


#define DEBUG_MSG MIOS32_MIDI_SendDebugMessage

#endif /* _MIOS32_CONFIG_H */
