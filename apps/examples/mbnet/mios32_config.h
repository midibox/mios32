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
#define MIOS32_LCD_BOOT_MSG_LINE1 "MBNET Example"
#define MIOS32_LCD_BOOT_MSG_LINE2 "(c) 2013 T.Klose"

// function used to output debug messages (must be printf compatible!)
#define DEBUG_MSG MIOS32_MIDI_SendDebugMessage


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
#define MBNET_SLAVE_NODES_END   0x07

#endif /* _MIOS32_CONFIG_H */
