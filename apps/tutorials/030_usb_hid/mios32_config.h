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

// Keyboard is AZERTY by default, uncomment this line for QWERTY
//#define QWERTY_KEYBOARD

// to disable HID features
//#define MIOS32_DONT_USE_USB_HID

// if you need to process the USB Host in your own task,
// just uncomment this define,
// then add MIOS32_USB_HOST_Process() in your task.
//#define MIOS32_DONT_PROCESS_USB_HOST


#endif /* _MIOS32_CONFIG_H */
