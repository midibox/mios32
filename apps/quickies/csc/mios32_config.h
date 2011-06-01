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
#define MIOS32_LCD_BOOT_MSG_LINE1 "CSC Controller"
#define MIOS32_LCD_BOOT_MSG_LINE2 "(C) 2011 T.Klose"


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
#define MIOS32_AIN_CHANNEL_MASK 0xffff

// define the deadband (min. difference to report a change to the application hook)
// typically set to (2^(12-desired_resolution)-1)
// e.g. for a resolution of 7 bit, it's set to (2^(12-7)-1) = (2^5 - 1) = 31
#define MIOS32_AIN_DEADBAND 31

// muxed or unmuxed mode (0..3)?
// 0 == unmuxed mode
// 1 == 1 mux control line -> *2 channels
// 2 == 2 mux control line -> *4 channels
// 3 == 3 mux control line -> *8 channels
#define MIOS32_AIN_MUX_PINS 3

// control pins to select the muxed channel
#define MIOS32_AIN_MUX0_PIN   GPIO_Pin_13 // J19.RC1
#define MIOS32_AIN_MUX0_PORT  GPIOC
#define MIOS32_AIN_MUX1_PIN   GPIO_Pin_14 // J19.RC2
#define MIOS32_AIN_MUX1_PORT  GPIOC
#define MIOS32_AIN_MUX2_PIN   GPIO_Pin_6 // J19.SC
#define MIOS32_AIN_MUX2_PORT  GPIOB


// first SR to which encoders are connected
// we have 40 encoders, they will allocate 1..10 (counted from 1)
#define FIRST_ENC_DIN_SR 1

// DINs of first 16x16 matrix (counted from 1)
#define DIN_16x16_L0 11
#define DIN_16x16_R0 12

// DINs of second 16x16 matrix (counted from 1)
#define DIN_16x16_L1 13
#define DIN_16x16_R1 14

// DOUTs of both matrices
#define DOUT_16x16_L 1
#define DOUT_16x16_R 2

// DOUTs of LED rings
#define DOUT_LEDRINGS_CATHODES 3
#define DOUT_LEDRINGS_1_8      4
#define DOUT_LEDRINGS_9_16     5
#define DOUT_LEDRINGS_17_24    6
#define DOUT_LEDRINGS_25_32    7

#endif /* _MIOS32_CONFIG_H */
