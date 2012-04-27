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
#define MIOS32_LCD_BOOT_MSG_LINE1 "MIDIboxKB V1.003"
#define MIOS32_LCD_BOOT_MSG_LINE2 "(C) 2012 T.Klose"

// define a unique VID/PID for this application
#define MIOS32_USB_VENDOR_ID    0x16c0        // sponsored by voti.nl! see http://www.voti.nl/pids
#define MIOS32_USB_VENDOR_STR   "midibox.org" // you will see this in the USB device description
#define MIOS32_USB_PRODUCT_STR  "MIDIboxKB"   // you will see this in the MIDI device list
#define MIOS32_USB_PRODUCT_ID   1005          // 1000..1009 are free for lab use

#define MIOS32_USB_MIDI_NUM_PORTS 4           // we provide 4 USB ports

// disables the default SRIO scan routine in programming_models/traditional/main.c
// allows to implement an own handler
// -> see app.c, TASK_MatrixScan()
#define MIOS32_DONT_SERVICE_SRIO_SCAN 1


// EEPROM emulation
// SIZE == 4096 halfwords -> 8192 bytes
#ifdef MIOS32_BOARD_MBHP_CORE_LPC17
# define EEPROM_EMULATED_SIZE 4096
#else
# error "Application requires external EEPROM - not prepared for this board"
#endif

// magic number in EEPROM - if it doesn't exist at address 0x00..0x03, the EEPROM will be cleared
#define EEPROM_MAGIC_NUMBER 0x47114221

// AIN configuration:
// Pin mapping on MBHP_CORE_LPC17 module:
//   7        6       5      4      3      2      1       0
// J5B.A7  J5B.A6  J5B.A5 J5B.A4 J5A.A3 J5A.A2 J5A.A1  J5A.A0
#define MIOS32_AIN_CHANNEL_MASK 0x003f

// define the deadband (min. difference to report a change to the application hook)
// typically set to (2^(12-desired_resolution)-1)
// e.g. for a resolution of 7 bit, it's set to (2^(12-7)-1) = (2^5 - 1) = 31
#define MIOS32_AIN_DEADBAND 31


#endif /* _MIOS32_CONFIG_H */
