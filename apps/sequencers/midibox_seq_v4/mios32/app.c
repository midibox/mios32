// $Id$
/*
 * MIDIbox SEQ V4
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
#include <seq_demo.h>

#include "app.h"


/////////////////////////////////////////////////////////////////////////////
// Encoder Configuration
/////////////////////////////////////////////////////////////////////////////
#define NUM_ENCODERS 8
// (tmp. mapping of MIDIbox LC... just for beta-testing)
const mios32_enc_config_t encoders[NUM_ENCODERS] = {
  { .cfg.type=NON_DETENTED, .cfg.speed=NORMAL, .cfg.speed_par=0, .cfg.sr=12, .cfg.pos=0 },
  { .cfg.type=NON_DETENTED, .cfg.speed=NORMAL, .cfg.speed_par=0, .cfg.sr=12, .cfg.pos=2 },
  { .cfg.type=NON_DETENTED, .cfg.speed=NORMAL, .cfg.speed_par=0, .cfg.sr=12, .cfg.pos=4 },
  { .cfg.type=NON_DETENTED, .cfg.speed=NORMAL, .cfg.speed_par=0, .cfg.sr=12, .cfg.pos=6 },
  { .cfg.type=NON_DETENTED, .cfg.speed=NORMAL, .cfg.speed_par=0, .cfg.sr=13, .cfg.pos=0 },
  { .cfg.type=NON_DETENTED, .cfg.speed=NORMAL, .cfg.speed_par=0, .cfg.sr=13, .cfg.pos=2 },
  { .cfg.type=NON_DETENTED, .cfg.speed=NORMAL, .cfg.speed_par=0, .cfg.sr=13, .cfg.pos=4 },
  { .cfg.type=NON_DETENTED, .cfg.speed=NORMAL, .cfg.speed_par=0, .cfg.sr=13, .cfg.pos=6 },
};


/////////////////////////////////////////////////////////////////////////////
// This hook is called after startup to initialize the application
/////////////////////////////////////////////////////////////////////////////
void APP_Init(void)
{
  s32 i;

  // initialize all LEDs
  MIOS32_BOARD_LED_Init(0xffffffff);

  // initialize encoders
  for(i=0; i<NUM_ENCODERS; ++i)
    MIOS32_ENC_ConfigSet(i, encoders[i]);

  // init MBSEQ Core
  Init(0);
}


/////////////////////////////////////////////////////////////////////////////
// This task is running endless in background
/////////////////////////////////////////////////////////////////////////////
//void APP_Background(void)
//{
//}
// located in seq_demo


/////////////////////////////////////////////////////////////////////////////
//  This hook is called when a complete MIDI event has been received
/////////////////////////////////////////////////////////////////////////////
void APP_NotifyReceivedEvent(u8 port, mios32_midi_package_t midi_package)
{
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called when a SysEx byte has been received
/////////////////////////////////////////////////////////////////////////////
void APP_NotifyReceivedSysEx(u8 port, u8 sysex_byte)
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
//void APP_DIN_NotifyToggle(u32 pin, u32 pin_value)
//{
//}
// located in seq_demo


/////////////////////////////////////////////////////////////////////////////
// This hook is called when an encoder has been moved
// incrementer is positive when encoder has been turned clockwise, else
// it is negative
/////////////////////////////////////////////////////////////////////////////
//void APP_ENC_NotifyChange(u32 encoder, s32 incrementer)
//{
//}
// located in seq_demo


/////////////////////////////////////////////////////////////////////////////
// This hook is called when a pot has been moved
/////////////////////////////////////////////////////////////////////////////
void APP_AIN_NotifyChange(u32 pin, u32 pin_value)
{
}
