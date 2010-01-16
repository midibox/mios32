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
//                                <------------------------>
#define MIOS32_LCD_BOOT_MSG_LINE1 "Virtual MIDIbox SID V0.1"
#define MIOS32_LCD_BOOT_MSG_LINE2 "(C) 2009 T. Klose       "

// function used to output debug messages (must be printf compatible!)
#define DEBUG_MSG MIOS32_MIDI_SendDebugMessage


// not supported by MacOS emulation:
//#define MIOS32_DONT_USE_SYS
#define MIOS32_DONT_USE_IRQ
//#define MIOS32_DONT_USE_SRIO
//#define MIOS32_DONT_USE_DIN
//#define MIOS32_DONT_USE_DOUT
//#define MIOS32_DONT_USE_ENC
#define MIOS32_DONT_USE_AIN
#define MIOS32_DONT_USE_MF
//#define MIOS32_DONT_USE_LCD
//#define MIOS32_DONT_USE_MIDI
//#define MIOS32_DONT_USE_COM
#define MIOS32_DONT_USE_USB
#define MIOS32_DONT_USE_USB_MIDI
#define MIOS32_USE_USB_COM
//#define MIOS32_DONT_USE_UART
//#define MIOS32_DONT_USE_UART_MIDI
#define MIOS32_DONT_USE_IIC
#define MIOS32_DONT_USE_IIC_MIDI
#define MIOS32_USE_I2S
//#define MIOS32_DONT_USE_BOARD
//#define MIOS32_DONT_USE_TIMER
#define MIOS32_DONT_USE_DELAY
//#define MIOS32_DONT_USE_SDCARD


#define MIOS32_MIDI_DEFAULT_PORT UART0

#define MIOS32_UART_NUM 4
#define MIOS32_UART_RX_BUFFER_SIZE 4096
#define MIOS32_UART_TX_BUFFER_SIZE 4096

// maximum idle counter value to be expected
#define MAX_IDLE_CTR 100

#define SIDPHYS_DISABLED

#endif /* _MIOS32_CONFIG_H */
