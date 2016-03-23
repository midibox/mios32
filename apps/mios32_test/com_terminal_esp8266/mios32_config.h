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

// support direct send command
#define ESP8266_TERMINAL_DIRECT_SEND_CMD 1
// suport for FW parts (bootloader)
#define ESP8266_FW_ACCESS_ENABLED 1

// don't use default firmware, but local version in esp8266_fw.inc instead
#define ESP8266_FW_DEFAULT_FIRMWARE 0
#define ESP8266_FW_FIRMWARE_UPPER_BASE_ADDRESS 0x20000

#endif /* _MIOS32_CONFIG_H */
