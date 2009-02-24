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
#define MIOS32_LCD_BOOT_MSG_LINE1 "uIP Example"
#define MIOS32_LCD_BOOT_MSG_LINE2 "(c) 2009 T.Klose"

// function used to output debug messages (must be printf compatible!)
#define DEBUG_MSG MIOS32_MIDI_SendDebugMessage

// ENC28J60 settings
#define MIOS32_ENC28J60_FULL_DUPLEX 1
#define MIOS32_ENC28J60_MAX_FRAME_SIZE 420

#define MIOS32_ENC28J60_MY_MAC_ADDR1 0x01
#define MIOS32_ENC28J60_MY_MAC_ADDR2 0x02
#define MIOS32_ENC28J60_MY_MAC_ADDR3 0x03
#define MIOS32_ENC28J60_MY_MAC_ADDR4 0x04
#define MIOS32_ENC28J60_MY_MAC_ADDR5 0x05
#define MIOS32_ENC28J60_MY_MAC_ADDR6 0x06


#endif /* _MIOS32_CONFIG_H */
