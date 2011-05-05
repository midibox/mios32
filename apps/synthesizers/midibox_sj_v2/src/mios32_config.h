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
#define MIOS32_LCD_BOOT_MSG_LINE1 "MIDIbox SJ"
#define MIOS32_LCD_BOOT_MSG_LINE2 "(C) 2011 AC & TK"

// function used to output debug messages (must be printf compatible!)
#define DEBUG_MSG MIOS32_MIDI_SendDebugMessage

// enable third UART, no open drain (not possible with LPC17 anyhow), baudrate and COM assignment
#define MIOS32_UART_NUM 3
#define MIOS32_UART2_TX_OD 0
#define MIOS32_UART2_BAUDRATE 19200
#define MIOS32_UART2_ASSIGNMENT 2

// Dedicated SpeakJet pin assignments
#if defined(MIOS32_BOARD_MBHP_CORE_STM32)
# error "TODO!"
#elif defined(MIOS32_BOARD_MBHP_CORE_LPC17) || defined(MIOS32_BOARD_LPCXPRESSO)
# define SPEAKJET_PORT_RST_N 2
# define SPEAKJET_PIN_RST_N  2

# define SPEAKJET_PORT_M1    2
# define SPEAKJET_PIN_M1     3

# define SPEAKJET_PORT_M0    2
# define SPEAKJET_PIN_M0     4

# define SPEAKJET_PORT_D2    2
# define SPEAKJET_PIN_D2     5
#else
# error "this MIOS32_BOARD hasn't been prepared yet!"
#endif


// AIN configuration
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

// define the deadband (min. difference to report a change to the application hook)
// typically set to (2^(12-desired_resolution)-1)
// e.g. for a resolution of 7 bit, it's set to (2^(12-7)-1) = (2^5 - 1) = 31
#define MIOS32_AIN_DEADBAND 31


#endif /* _MIOS32_CONFIG_H */
