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
#define MIOS32_LCD_BOOT_MSG_LINE1 "MIOS32_IIC_MIDI"
#define MIOS32_LCD_BOOT_MSG_LINE2 "(C) 2011 T.Klose"


#define MIOS32_IIC_MIDI_ADDR_BASE 0x10

// IIC port
#define MIOS32_IIC_MIDI_PORT 0

// max number of IIC MIDI interfaces (0..8)
// has to be set to >0 in mios32_config.h to enable IIC MIDI!
#define MIOS32_IIC_MIDI_NUM 4

// Interface and RI_N port configuration
// _ENABLED:   0 = interface disabled
//             1 = interface enabled, don't poll receive status (OUT only)
//             2 = interface enabled, poll receive status
//             3 = interface enabled, check RI_N pin instead of polling receive status
// _RI_N_PORT: Port to which RI_N is connected
// _RI_N_PIN:  pin to which RI_N is connected
#if defined(MIOS32_FAMILY_STM32F10x)
#define MIOS32_IIC_MIDI0_ENABLED    2
#define MIOS32_IIC_MIDI0_RI_N_PORT  GPIOC
#define MIOS32_IIC_MIDI0_RI_N_PIN   GPIO_Pin_0

#define MIOS32_IIC_MIDI1_ENABLED    2
#define MIOS32_IIC_MIDI1_RI_N_PORT  GPIOC
#define MIOS32_IIC_MIDI1_RI_N_PIN   GPIO_Pin_1

#define MIOS32_IIC_MIDI2_ENABLED    2
#define MIOS32_IIC_MIDI2_RI_N_PORT  GPIOC
#define MIOS32_IIC_MIDI2_RI_N_PIN   GPIO_Pin_2

#define MIOS32_IIC_MIDI3_ENABLED    2
#define MIOS32_IIC_MIDI3_RI_N_PORT  GPIOC
#define MIOS32_IIC_MIDI3_RI_N_PIN   GPIO_Pin_3
#elif defined(MIOS32_FAMILY_LPC17xx)
// available at J10 (digital IO)
// P2.2 is J10.D0
#define MIOS32_IIC_MIDI0_ENABLED    2
#define MIOS32_IIC_MIDI0_RI_N_PORT  2
#define MIOS32_IIC_MIDI0_RI_N_PIN   2

// P2.3 is J10.D1
#define MIOS32_IIC_MIDI1_ENABLED    2
#define MIOS32_IIC_MIDI1_RI_N_PORT  2
#define MIOS32_IIC_MIDI1_RI_N_PIN   3

// P2.4 is J10.D2
#define MIOS32_IIC_MIDI2_ENABLED    2
#define MIOS32_IIC_MIDI2_RI_N_PORT  2
#define MIOS32_IIC_MIDI2_RI_N_PIN   4

// P2.5 is J10.D3
#define MIOS32_IIC_MIDI3_ENABLED    2
#define MIOS32_IIC_MIDI3_RI_N_PORT  2
#define MIOS32_IIC_MIDI3_RI_N_PIN   5
#else
# error "application not prepared for this MIOS32_FAMILY!"
#endif

#endif /* _MIOS32_CONFIG_H */
