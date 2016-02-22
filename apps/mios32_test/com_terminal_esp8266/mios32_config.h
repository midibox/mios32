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
#define MIOS32_LCD_BOOT_MSG_LINE1 "ESP8266 COM Terminal"
#define MIOS32_LCD_BOOT_MSG_LINE2 "(C) 2016 T.Klose"

// function used to output debug messages (must be printf compatible!)
#define DEBUG_MSG MIOS32_MIDI_SendDebugMessage

// enable third UART, no open drain, baudrate and COM assignment
#if MIOS32_UART_NUM < 3
#define MIOS32_UART_NUM 3
#endif

#define MIOS32_UART2_TX_OD 0
#define MIOS32_UART2_BAUDRATE 115200
#define MIOS32_UART2_ASSIGNMENT 2

#define ESP8266_UART UART2


#endif /* _MIOS32_CONFIG_H */
