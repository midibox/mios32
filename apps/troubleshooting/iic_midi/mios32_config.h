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
#define MIOS32_LCD_BOOT_MSG_LINE1 "IIC_MIDI Test"
#define MIOS32_LCD_BOOT_MSG_LINE2 "(C) 2013 T.Klose"


// enable 4 interfaces
#define MIOS32_IIC_MIDI_NUM 4

// Interface and RI_N port configuration
// _ENABLED:   0 = interface disabled
//             1 = interface enabled
//             2 = interface enabled, check RI_N pin instead of polling receive status
// _RI_N_PORT: Port to which RI_N is connected
// _RI_N_PIN:  pin to which RI_N is connected
#define MIOS32_IIC_MIDI0_ENABLED 1
#define MIOS32_IIC_MIDI0_RI_N_PORT  GPIOC        // not used due to mode #1
#define MIOS32_IIC_MIDI0_RI_N_PIN   GPIO_Pin_0

#define MIOS32_IIC_MIDI1_ENABLED 1
#define MIOS32_IIC_MIDI1_RI_N_PORT  GPIOC        // not used due to mode #1
#define MIOS32_IIC_MIDI1_RI_N_PIN   GPIO_Pin_1

#define MIOS32_IIC_MIDI2_ENABLED 1
#define MIOS32_IIC_MIDI2_RI_N_PORT  GPIOC        // not used due to mode #1
#define MIOS32_IIC_MIDI2_RI_N_PIN   GPIO_Pin_2

#define MIOS32_IIC_MIDI3_ENABLED 1
#define MIOS32_IIC_MIDI3_RI_N_PORT  GPIOC        // not used due to mode #1
#define MIOS32_IIC_MIDI3_RI_N_PIN   GPIO_Pin_3


#endif /* _MIOS32_CONFIG_H */
