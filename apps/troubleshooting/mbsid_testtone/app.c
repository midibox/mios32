// $Id$
/*
 * SID Testtone generator
 *
 * ==========================================================================
 *
 *  Copyright (C) 2008 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

/////////////////////////////////////////////////////////////////////////////
// Include files
/////////////////////////////////////////////////////////////////////////////

#include <mios32.h>
#include <sid.h>

#include "app.h"


/////////////////////////////////////////////////////////////////////////////
// This hook is called after startup to initialize the application
/////////////////////////////////////////////////////////////////////////////
void APP_Init(void)
{
  MIOS32_BOARD_LED_Init(0xffffffff); // initialize all LEDs

  SID_Init(0); // initialize SID module
}


/////////////////////////////////////////////////////////////////////////////
// This task is running endless in background
/////////////////////////////////////////////////////////////////////////////
void APP_Background(void)
{
  // clear LCD
  MIOS32_LCD_Clear();

  MIOS32_LCD_CursorSet(0, 0);
  MIOS32_LCD_PrintString("SID Testtone    ");
  MIOS32_LCD_CursorSet(0, 1);
  MIOS32_LCD_PrintString("Generator       ");

  // set initial SID values
  int sid;
  for(sid=0; sid<SID_NUM; ++sid) {
    sid_regs[sid].volume = 15;

    sid_regs[sid].v1.frq_l = (16777 & 0xff); // Fout = Fn * Fclk/16777216
    sid_regs[sid].v1.frq_h = (16777 >> 8);
    sid_regs[sid].v1.waveform = 1;
    sid_regs[sid].v1.attack = 0;
    sid_regs[sid].v1.decay = 0;
    sid_regs[sid].v1.sustain = 15;
    sid_regs[sid].v1.release = 0;
  }

  // update SID registers
  SID_Update(0);

  // set gate
  for(sid=0; sid<SID_NUM; ++sid) {
    sid_regs[sid].v1.gate = 1;
  }

  // update SID registers again
  SID_Update(0);


  // run endless
  while( 1 ) {
    // toggle the state of all LEDs (sign of life)
    MIOS32_BOARD_LED_Set(0xffffffff, ~MIOS32_BOARD_LED_Get());

    // update SID registers
    // (this is to measure the update time in best case (no register updates) with a scope)
    SID_Update(0);
  }
}


/////////////////////////////////////////////////////////////////////////////
//  This hook is called when a complete MIDI event has been received
/////////////////////////////////////////////////////////////////////////////
void APP_NotifyReceivedEvent(mios32_midi_port_t port, mios32_midi_package_t midi_package)
{
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called when a SysEx byte has been received
/////////////////////////////////////////////////////////////////////////////
void APP_NotifyReceivedSysEx(mios32_midi_port_t port, u8 sysex_byte)
{
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called when a byte has been received via COM interface
/////////////////////////////////////////////////////////////////////////////
void APP_NotifyReceivedCOM(mios32_com_port_t port, u8 byte)
{
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called before the shift register chain is scanned
/////////////////////////////////////////////////////////////////////////////
void APP_SRIO_ServicePrepare(void)
{
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called after the shift register chain has been scanned
/////////////////////////////////////////////////////////////////////////////
void APP_SRIO_ServiceFinish(void)
{
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called when a button has been toggled
// pin_value is 1 when button released, and 0 when button pressed
/////////////////////////////////////////////////////////////////////////////
void APP_DIN_NotifyToggle(u32 pin, u32 pin_value)
{
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called when an encoder has been moved
// incrementer is positive when encoder has been turned clockwise, else
// it is negative
/////////////////////////////////////////////////////////////////////////////
void APP_ENC_NotifyChange(u32 encoder, s32 incrementer)
{
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called when a pot has been moved
/////////////////////////////////////////////////////////////////////////////
void APP_AIN_NotifyChange(u32 pin, u32 pin_value)
{
}
