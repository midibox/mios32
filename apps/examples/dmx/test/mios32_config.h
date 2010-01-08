// $Id: mios32_config.h 346 2009-02-10 22:10:39Z tk $
/*
 * Local MIOS32 configuration file
 *
 * this file allows to disable (or re-configure) default functions of MIOS32
 * available switches are listed in $MIOS32_PATH/modules/mios32/MIOS32_CONFIG.txt
 *
 */

#ifndef _MIOS32_CONFIG_H
#define _MIOS32_CONFIG_H

// For now disable UART as we will be using DMX 
#define MIOS32_DONT_USE_UART


// The boot message which is print during startup and returned on a SysEx query
#define MIOS32_LCD_BOOT_MSG_LINE1 "MIDIbox Lights"
#define MIOS32_LCD_BOOT_MSG_LINE2 "(C) 2009 Phil Taylor"

// USB settings
#define MIOS32_USB_PRODUCT_STR  "MIDIbox Lights v1"

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
#define MIOS32_AIN_CHANNEL_MASK 0x000f

// define the desired oversampling rate (1..16)
#define MIOS32_AIN_OVERSAMPLING_RATE  1

// define the deadband (min. difference to report a change to the application hook)
// 31 is enough for 7bit resolution at 12bit sampling resolution (1x oversampling)
#define MIOS32_AIN_DEADBAND 15
// muxed or unmuxed mode (0..3)?
// 0 == unmuxed mode
// 1 == 1 mux control line -> *2 channels
// 2 == 2 mux control line -> *4 channels
// 3 == 3 mux control line -> *8 channels
#define MIOS32_AIN_MUX_PINS 3

// control pins to select the muxed channel
// only relevant if MIOS32_AIN_MUX_PINS > 0
// I found this easier with Smash AIN module.
//#define MIOS32_AIN_MUX0_PIN   GPIO_Pin_1 // J5C.A11
//#define MIOS32_AIN_MUX0_PORT  GPIOB
//#define MIOS32_AIN_MUX1_PIN   GPIO_Pin_0 // J5C.A10
//#define MIOS32_AIN_MUX1_PORT  GPIOB
//#define MIOS32_AIN_MUX2_PIN   GPIO_Pin_5 // J5C.A9
//#define MIOS32_AIN_MUX2_PORT  GPIOC
// control pins to select the muxed channel
#define MIOS32_AIN_MUX0_PIN   GPIO_Pin_4 // J5C.A8
#define MIOS32_AIN_MUX0_PORT  GPIOC
#define MIOS32_AIN_MUX1_PIN   GPIO_Pin_5 // J5C.A9
#define MIOS32_AIN_MUX1_PORT  GPIOC
#define MIOS32_AIN_MUX2_PIN   GPIO_Pin_0 // J5C.A10
#define MIOS32_AIN_MUX2_PORT  GPIOB


// ENC28J60 settings
#define MIOS32_ENC28J60_FULL_DUPLEX 1
#define MIOS32_ENC28J60_MAX_FRAME_SIZE 620

// a unique MAC address in your network (6 bytes are required)
// If all bytes are 0, the serial number of STM32 will be taken instead,
// which should be unique in your private network.
#define MIOS32_ENC28J60_MY_MAC_ADDR1 0
#define MIOS32_ENC28J60_MY_MAC_ADDR2 0
#define MIOS32_ENC28J60_MY_MAC_ADDR3 0
#define MIOS32_ENC28J60_MY_MAC_ADDR4 0
#define MIOS32_ENC28J60_MY_MAC_ADDR5 0
#define MIOS32_ENC28J60_MY_MAC_ADDR6 0

#define MIOS32_SDCARD_MUTEX_TAKE    APP_MutexSPI0Take();
#define MIOS32_SDCARD_MUTEX_GIVE    APP_MutexSPI0Give();
#define MIOS32_ENC28J60_MUTEX_TAKE    APP_MutexSPI0Take();
#define MIOS32_ENC28J60_MUTEX_GIVE    APP_MutexSPI0Give();
//#define MIOS32_IIC_MIDI_MUTEX_TAKE    APP_MutexIIC0Take();
//#define MIOS32_IIC_MIDI_MUTEX_GIVE    APP_MutexIIC0Give();
//#define MIOS32_IIC_BS_MUTEX_TAKE    APP_MutexIIC0Take();
//#define MIOS32_IIC_BS_MUTEX_GIVE    APP_MutexIIC0Give();


#endif /* _MIOS32_CONFIG_H */
