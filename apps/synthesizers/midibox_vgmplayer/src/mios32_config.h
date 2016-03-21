/*
 * MIOS32 configuration file for MIDIbox VGM Player
 */

#ifndef _MIOS32_CONFIG_H
#define _MIOS32_CONFIG_H

#define DBG MIOS32_MIDI_SendDebugMessage

// Make sure only STM32F4
#if !defined(MIOS32_BOARD_STM32F4DISCOVERY) && !defined(MIOS32_BOARD_MBHP_CORE_STM32F4)
#error "MIDIbox VGM Player only supported on STM32F4 MCU!"
#endif

// The boot message which is print during startup and returned on a SysEx query
#define MIOS32_LCD_BOOT_MSG_LINE1 "MIDIbox VGM Player"
#define MIOS32_LCD_BOOT_MSG_LINE2 "(C) 2016 Sauraen"

// USB settings
#define MIOS32_USB_PRODUCT_STR  "MIDIbox VGM Player"
#define MIOS32_USB_MIDI_NUM_PORTS 1

// MBHP_Genesis
#define GENESIS_COUNT 4

// VGM timers
#define MIOS32_DONT_USE_TIMER
#define MIOS32_IRQ_PRIO_INSANE 3

// The heap (amount of RAM for realtime allocation)
#define MIOS32_HEAP_SIZE 64*1024

// Front panel
#define MIOS32_SRIO_NUM_SR 12
#define MIOS32_SRIO_NUM_DOUT_PAGES 1
#define MIOS32_ENC_NUM_MAX 32

// BLM_X configuration
#define BLM_X_NUM_ROWS 8
#define BLM_X_BTN_NUM_COLS 56
#define BLM_X_LED_NUM_COLS 88
#define BLM_X_LED_NUM_COLORS 1
#define BLM_X_ROWSEL_DOUT_SR 1
#define BLM_X_LED_FIRST_DOUT_SR 2
#define BLM_X_BTN_FIRST_DIN_SR 2
#define BLM_X_ROWSEL_INV_MASK 0xFF
#define BLM_X_DEBOUNCE_MODE 0
#define BLM_X_COLOR_MODE 0

#define FRONTPANEL_REVERSE_ROWS 1

#endif /* _MIOS32_CONFIG_H */
