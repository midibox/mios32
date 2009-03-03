// $Id: mios32_config.h 194 2008-12-18 01:47:21Z tk $
/*
 * Local MIOS32 configuration file
 *
 * this file allows to disable (or re-configure) default functions of MIOS32
 * available switches are listed in $MIOS32_PATH/modules/mios32/MIOS32_CONFIG.txt
 *
 */

#ifndef _MIOS32_CONFIG_H
#define _MIOS32_CONFIG_H

// The boot message which is on a SysEx query (LCD not enabled, therefore not print on startup)
#define MIOS32_LCD_BOOT_MSG_LINE1 "MIOS32 Bootloader"
#define MIOS32_LCD_BOOT_MSG_LINE2 "(c) 2009 T. Klose"

// disable code modules
//#define MIOS32_DONT_USE_SYS
//#define MIOS32_DONT_USE_IRQ
#define MIOS32_DONT_USE_SPI
#define MIOS32_DONT_USE_SRIO
#define MIOS32_DONT_USE_DIN
#define MIOS32_DONT_USE_DOUT
#define MIOS32_DONT_USE_ENC
#define MIOS32_DONT_USE_AIN
#define MIOS32_DONT_USE_MF
#define MIOS32_DONT_USE_LCD
//#define MIOS32_DONT_USE_MIDI
#define MIOS32_DONT_USE_OSC
#define MIOS32_DONT_USE_COM
//#define MIOS32_DONT_USE_USB
//#define MIOS32_DONT_USE_USB_MIDI
//#define MIOS32_USE_USB_COM
//#define MIOS32_DONT_USE_UART
//#define MIOS32_DONT_USE_UART_MIDI
#define MIOS32_DONT_USE_IIC
#define MIOS32_DONT_USE_IIC_MIDI
//#define MIOS32_USE_I2S
//#define MIOS32_DONT_USE_BOARD
#define MIOS32_DONT_USE_TIMER
//#define MIOS32_DONT_USE_STOPWATCH
#define MIOS32_DONT_USE_DELAY
#define MIOS32_DONT_USE_SDCARD
#define MIOS32_DONT_USE_ENC28J60

// calls to FreeRTOS required? (e.g. to disable tasks on critical sections)
#define MIOS32_DONT_USE_FREERTOS


// Following settings allow to customize the USB device descriptor
#define MIOS32_USB_VENDOR_ID    0x16c0        // sponsored by voti.nl! see http://www.voti.nl/pids
#define MIOS32_USB_VENDOR_STR   "midibox.org" // you will see this in the USB device description
#define MIOS32_USB_PRODUCT_STR  "MIOS32 Bootloader"  // you will see this in the MIDI device list
#define MIOS32_USB_PRODUCT_ID   0x0400        // ==1024; 1020-1029 reserved for T.Klose, 1000 - 1009 free for lab use
#define MIOS32_USB_VERSION_ID   0x0100        // v1.00


// 1 to stay compatible to USB MIDI spec, 0 as workaround for some windows versions...
#define MIOS32_USB_MIDI_USE_AC_INTERFACE 1

// allowed number of USB MIDI ports: 1..8
#define MIOS32_USB_MIDI_NUM_PORTS 1

// buffer size (should be at least >= MIOS32_USB_MIDI_DATA_*_SIZE/4)
#define MIOS32_USB_MIDI_RX_BUFFER_SIZE   64 // packages
#define MIOS32_USB_MIDI_TX_BUFFER_SIZE   64 // packages

// size of IN/OUT pipe
#define MIOS32_USB_MIDI_DATA_IN_SIZE           64
#define MIOS32_USB_MIDI_DATA_OUT_SIZE          64


// enable BSL enhancements in MIOS32 SysEx parser
#define MIOS32_MIDI_BSL_ENHANCEMENTS 1

// exclude default BSL image from MIOS32
#define MIOS32_DONT_INCLUDE_BSL

#endif /* _MIOS32_CONFIG_H */
