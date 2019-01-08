// $Id$
/*
 * Temporary Wrapper code/stubs for MIOS32 functions
 * Only functions used by MBSID applications are listed here!
 * TODO: bring this to $MIOS32_PATH/mios32/JUCE and implemnent them properly!
 *
 * ==========================================================================
 *
 *  Copyright (C) 2009 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#include <mios32.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>


s32 MIOS32_MIDI_SendDebugMessage(const char *format, ...)
{
  char buffer[1024];
  va_list args;

  va_start(args, format);
  vsprintf((char *)buffer, format, args);
  printf(buffer);
	
  return 0; // no error
}

// now temporary in MidiProcessing.cpp
//s32 MIOS32_MIDI_SendSysEx(mios32_midi_port_t port, u8 *stream, u32 count) { return -1; }

s32 MIOS32_MIDI_SysExCallback_Init(s32 (*callback_sysex)(mios32_midi_port_t port, u8 sysex_byte)) { return -1; }
s32 MIOS32_MIDI_TimeOutCallback_Init(s32 (*callback_timeout)(mios32_midi_port_t port)) { return -1; }
s32 MIOS32_MIDI_DirectTxCallback_Init(s32 (*callback_tx)(mios32_midi_port_t port, mios32_midi_package_t package)) { return -1; }
s32 MIOS32_MIDI_DirectRxCallback_Init(s32 (*callback_rx)(mios32_midi_port_t port, u8 midi_byte)) { return -1; }
u8  MIOS32_MIDI_DeviceIDGet(void) { return 0x00; }

s32 MIOS32_STOPWATCH_Init(u32 resolution) { return -1; }
s32 MIOS32_STOPWATCH_Reset(void) { return -1; }
u32 MIOS32_STOPWATCH_ValueGet(void) { return -1; }

s32 MIOS32_BOARD_LED_Init(u32 leds) { return -1; }
s32 MIOS32_BOARD_LED_Set(u32 leds, u32 value) { return -1; }
u32 MIOS32_BOARD_LED_Get(void) { return -1; }

s32 MIOS32_BOARD_DAC_PinInit(u8 chn, u8 enable) { return -1; }
s32 MIOS32_BOARD_DAC_PinSet(u8 chn, u16 value) { return -1; }

s32 MIOS32_TIMER_Init(u8 timer, u32 period, void (*_irq_handler)(void), u8 irq_priority) { return -1; }
s32 MIOS32_TIMER_ReInit(u8 timer, u32 period) { return -1; }
s32 MIOS32_TIMER_DeInit(u8 timer) { return -1; }

s32 MIOS32_IRQ_Disable(void) { return -1; }
s32 MIOS32_IRQ_Enable(void) { return -1; }

s32 MIOS32_SPI_Init(u32 mode);

s32 MIOS32_SPI_IO_Init(u8 spi, mios32_spi_pin_driver_t spi_pin_driver) { return -1; }
s32 MIOS32_SPI_TransferModeInit(u8 spi, mios32_spi_mode_t spi_mode, mios32_spi_prescaler_t spi_prescaler) { return -1; }


s32 MIOS32_SPI_RC_PinSet(u8 spi, u8 rc_pin, u8 pin_value) { return -1; }
s32 MIOS32_SPI_TransferByte(u8 spi, u8 b) { return -1; }
s32 MIOS32_SPI_TransferBlock(u8 spi, u8 *send_buffer, u8 *receive_buffer, u16 len, void *callback) { return -1; }
