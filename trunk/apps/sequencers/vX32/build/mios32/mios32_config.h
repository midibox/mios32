/* $Id$ */
/*
 * Local MIOS32 configuration file
 *
 * this file allows to disable (or re-configure) default functions of MIOS32
 * available switches are listed in $MIOS32_PATH/modules/mios32/MIOS32_CONFIG.txt
 *
 */

#ifndef _MIOS32_CONFIG_H
#define _MIOS32_CONFIG_H




#define vX_DEBUG_VERBOSE_LEVEL 0

#define DEBUG_MSG MIOS32_MIDI_SendDebugMessage




// Following settings allow to customize the USB device descriptor
#define MIOS32_USB_VENDOR_STR   "midibox.org"                       // you will see this in the USB device description
#define MIOS32_USB_PRODUCT_STR  "vX32"                              // you will see this in the MIDI device list
#define MIOS32_USB_VERSION_ID   0x0100                              // v1.00

#define MIOS32_LCD_BOOT_MSG_LINE1 "vX32 alpha-28"


#define MIOS32_LCD_BOOT_MSG_LINE2 "www.midibox.org"

#define MIOS32_HEAP_SIZE 14*1024

// Windows drivers suck so disable USB COM
//#define MIOS32_USE_USB_COM

//#define MIOS32_DONT_USE_BOARD // FIXME TESTING

#endif /* _MIOS32_CONFIG_H */
