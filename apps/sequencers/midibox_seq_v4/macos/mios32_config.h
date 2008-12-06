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


// include SRIO setup here, so that we can propagate values to external modules
#include "srio_mapping.h"

// forward to BLM8x8 driver
#define BLM8X8_DOUT	          DEFAULT_SRM_DOUT_M
#define BLM8X8_DOUT_CATHODES	  DEFAULT_SRM_DOUT_CATHODESM
#define BLM8X8_CATHODES_INV_MASK  DEFAULT_SRM_CATHODES_INV_MASK_M
#define BLM8X8_DIN	          DEFAULT_SRM_DIN_M

#endif /* _MIOS32_CONFIG_H */
