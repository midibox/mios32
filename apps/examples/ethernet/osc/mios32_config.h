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
#define MIOS32_LCD_BOOT_MSG_LINE1 "uIP OSC Example"
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


// By default the IP will be configured via DHCP
// By defining DONT_USE_DHCP we can optionally use static addresses

// #define DONT_USE_DHCP

#ifndef DONT_USE_DHCP
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

#define OSC_SERVER_PORT 10000



// AIN configuration:

// bit mask to enable channels
//
// Pin mapping on MBHP_CORE_STM32 module:
//   15       14      13     12     11     10      9      8   
// J16.SO  J16.SI  J16.SC J16.RC J5C.A11 J5C.A10 J5C.A9 J5C.A8
//   7        6       5      4      3      2      1       0
// J5B.A7  J5B.A6  J5B.A5 J5B.A4 J5A.A3 J5A.A2 J5A.A1  J5A.A0
//
// Examples:
//   mask 0x000f will enable all J5A channels
//   mask 0x00f0 will enable all J5B channels
//   mask 0x0f00 will enable all J5C channels
//   mask 0x0fff will enable all J5A/B/C channels
// (all channels are disabled by default)
#define MIOS32_AIN_CHANNEL_MASK 0x00ff

// define the desired oversampling rate (1..16)
#define MIOS32_AIN_OVERSAMPLING_RATE  1

// define the deadband (min. difference to report a change to the application hook)
#define MIOS32_AIN_DEADBAND 15

// muxed or unmuxed mode (0..3)?
// 0 == unmuxed mode
// 1 == 1 mux control line -> *2 channels
// 2 == 2 mux control line -> *4 channels
// 3 == 3 mux control line -> *8 channels
#define MIOS32_AIN_MUX_PINS 0

// control pins to select the muxed channel
// only relevant if MIOS32_AIN_MUX_PINS > 0
#define MIOS32_AIN_MUX0_PIN   GPIO_Pin_4 // J5C.A8
#define MIOS32_AIN_MUX0_PORT  GPIOC
#define MIOS32_AIN_MUX1_PIN   GPIO_Pin_5 // J5C.A9
#define MIOS32_AIN_MUX1_PORT  GPIOC
#define MIOS32_AIN_MUX2_PIN   GPIO_Pin_0 // J5C.A10
#define MIOS32_AIN_MUX2_PORT  GPIOB

// number of motorfaders (0-16)
#define MIOS32_MF_NUM 8


#endif /* _MIOS32_CONFIG_H */
