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
// function used to output debug messages via UART0 (must be printf compatible!)
#define DEBUG_MSG MIOS32_MIDI_SendDebugMessage
//#define MIOS32_MIDI_USBH_DEBUG
#define APP_DEBUG_VERBOSE

// The boot message which is print during startup and returned on a SysEx query
#define MIOS32_LCD_BOOT_MSG_LINE1 "Tutorial #030"
#define MIOS32_LCD_BOOT_MSG_LINE2 "(C) 2020 TK/Antichambre"

//#define MIOS32_DONT_USE_USB_HOST
//#define MIOS32_DONT_USE_USB_HID

#endif /* _MIOS32_CONFIG_H */
