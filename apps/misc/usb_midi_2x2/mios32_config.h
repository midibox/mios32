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
#define MIOS32_LCD_BOOT_MSG_LINE1 "USB MIDI 2x2 V1.009"
#define MIOS32_LCD_BOOT_MSG_LINE2 "(C) 2014 T.Klose"

// Following settings allow to customize the USB device descriptor
#define MIOS32_USB_VENDOR_STR   "midibox.org" // you will see this in the USB device description
#define MIOS32_USB_PRODUCT_STR  "USB MIDI 2x2" // you will see this in the MIDI device list

#define MIOS32_USB_MIDI_NUM_PORTS 2           // we provide 2 USB ports

#endif /* _MIOS32_CONFIG_H */
