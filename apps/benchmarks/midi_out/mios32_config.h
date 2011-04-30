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
#define MIOS32_LCD_BOOT_MSG_LINE1 "MIDI Out Benchmark"
#define MIOS32_LCD_BOOT_MSG_LINE2 "(c) 2009 T.Klose"


// function used to output debug messages (must be printf compatible!)
#define DEBUG_MSG MIOS32_MIDI_SendDebugMessage

// ENC28J60 settings
#define MIOS32_ENC28J60_FULL_DUPLEX 1
#define MIOS32_ENC28J60_MAX_FRAME_SIZE 1504

// a unique MAC address in your network (6 bytes are required)
// If all bytes are 0, the serial number of STM32 will be taken instead,
// which should be unique in your private network.
#define MIOS32_ENC28J60_MY_MAC_ADDR1 0
#define MIOS32_ENC28J60_MY_MAC_ADDR2 0
#define MIOS32_ENC28J60_MY_MAC_ADDR3 0
#define MIOS32_ENC28J60_MY_MAC_ADDR4 0
#define MIOS32_ENC28J60_MY_MAC_ADDR5 0
#define MIOS32_ENC28J60_MY_MAC_ADDR6 0


// By default the IP will be configured via DHCP
// By defining DONT_USE_DHCP we can optionally use static addresses

// #define DONT_USE_DHCP

#ifdef DONT_USE_DHCP
// Ethernet configuration:
//                      192        .  168        .    2       .  100
# define MY_IP_ADDRESS (192 << 24) | (168 << 16) | (  2 << 8) | (100 << 0)
//                      255        .  255        .  255       .    0
# define MY_NETMASK    (255 << 24) | (255 << 16) | (255 << 8) | (  0 << 0)
//                      192        .  168        .    2       .    1
# define MY_GATEWAY    (192 << 24) | (168 << 16) | (  2 << 8) | (  1 << 0)
#endif


// not automatically configured yet!
//                     192        .  168        .    2       .    1
#define OSC_REMOTE_IP (192 << 24) | (168 << 16) | (  2 << 8) | (  1 << 0)

#define OSC_SERVER_PORT 8888


// max number of IIC MIDI interfaces (0..8)
// has to be set to >0 in mios32_config.h to enable IIC MIDI!
#define MIOS32_IIC_MIDI_NUM 1
// don't poll receive status (OUT only)
#define MIOS32_IIC_MIDI0_ENABLED    1


#endif /* _MIOS32_CONFIG_H */
