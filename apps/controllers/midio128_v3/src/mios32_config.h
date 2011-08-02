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
#define MIOS32_USB_VENDOR_ID    0x16c0        // sponsored by voti.nl! see http://www.voti.nl/pids
#define MIOS32_USB_VENDOR_STR   "midibox.org" // you will see this in the USB device description
#define MIOS32_USB_PRODUCT_STR  MIOS32_LCD_BOOT_MSG_LINE1
#define MIOS32_USB_PRODUCT_ID   1005          // 1000..1009 are free for lab use

#define MIOS32_USB_MIDI_NUM_PORTS 2           // we provide 2 USB ports


#define DEBUG_MSG MIOS32_MIDI_SendDebugMessage

#endif /* _MIOS32_CONFIG_H */
