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
#define MIOS32_LCD_BOOT_MSG_LINE1 "MBSID CAN OSC Proxy"
#define MIOS32_LCD_BOOT_MSG_LINE2 "(c) 2009 T.Klose"

// function used to output debug messages (must be printf compatible!)
#define DEBUG_MSG MIOS32_MIDI_SendDebugMessage

// ENC28J60 settings
#define MIOS32_ENC28J60_FULL_DUPLEX 1
#define MIOS32_ENC28J60_MAX_FRAME_SIZE 420

// a unique MAC address in your network (6 bytes are required)
// If all bytes are 0, the serial number of STM32 will be taken instead,
// which should be unique in your private network.
#define MIOS32_ENC28J60_MY_MAC_ADDR1 0
#define MIOS32_ENC28J60_MY_MAC_ADDR2 0
#define MIOS32_ENC28J60_MY_MAC_ADDR3 0
#define MIOS32_ENC28J60_MY_MAC_ADDR4 0
#define MIOS32_ENC28J60_MY_MAC_ADDR5 0
#define MIOS32_ENC28J60_MY_MAC_ADDR6 0


// Ethernet configuration:
//                     192        .  168        .    1       .    3
#define MY_IP_ADDRESS (192 << 24) | (168 << 16) | (  1 << 8) | (  3 << 0)

//                     255        .  255        .  255       .    0
#define MY_NETMASK    (255 << 24) | (255 << 16) | (255 << 8) | (  0 << 0)

//                     192        .  168        .    1       .    1
#define MY_GATEWAY    (192 << 24) | (168 << 16) | (  1 << 8) | (  1 << 0)

//                     192        .  168        .    1       .  104
#define OSC_REMOTE_IP (192 << 24) | (168 << 16) | (  1 << 8) | (104 << 0)

#define OSC_SERVER_PORT 10000



#if defined(MIOS32_FAMILY_STM32F10x)
// only for STM32: disable USB, enable CAN (it isn't possible to use both peripherals at the same time)
# define MIOS32_DONT_USE_USB
# define MIOS32_USE_CAN
// forward debugging messages to UART based MIDI output (since USB is disabled)
#define MIOS32_MIDI_DEBUG_PORT UART0
#endif

// relevant if configured as master: how many nodes should be scanned maximum
#define MBNET_SLAVE_NODES_MAX 8

// first node to be scanned
#define MBNET_SLAVE_NODES_BEGIN 0x00

// last node to be scanned
#define MBNET_SLAVE_NODES_END   0x03


// disable SRIO, AIN, Encoders, COM (not used - prevent that it consumes CPU time)
#define MIOS32_DONT_USE_SRIO
#define MIOS32_DONT_USE_AIN
#define MIOS32_DONT_USE_ENC
#define MIOS32_DONT_USE_COM


#endif /* _MIOS32_CONFIG_H */
