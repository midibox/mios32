/*
 * MIOS32 configuration file for MIDIbox Genesis Tracker
 */

#ifndef _MIOS32_CONFIG_H
#define _MIOS32_CONFIG_H

// Make sure only STM32F4
#if !defined(MIOS32_BOARD_STM32F4DISCOVERY) && !defined(MIOS32_BOARD_MBHP_CORE_STM32F4)
#error "MIDIbox Genesis Tracker only supported on STM32F4 MCU!"
#endif

// The boot message which is print during startup and returned on a SysEx query
#define MIOS32_LCD_BOOT_MSG_LINE1 "MIDIbox Genesis Tracker"
#define MIOS32_LCD_BOOT_MSG_LINE2 "(C) 2016 Sauraen"

// USB settings
#define MIOS32_USB_PRODUCT_STR  "MIDIbox Genesis Tracker"
#define MIOS32_USB_MIDI_NUM_PORTS 1

// VGM timers
#define MIOS32_DONT_USE_TIMER
#define MIOS32_IRQ_PRIO_INSANE 3

#endif /* _MIOS32_CONFIG_H */
