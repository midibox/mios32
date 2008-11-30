// $Id$
/*
 * Sequencer Demo
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

#if DEFAULT_SRM_ENABLED
#include <blm8x8.h>
#endif

#include "seq_demo.h"
#include "seq_core.h"
#include "seq_par.h"
#include "seq_trg.h"
#include "seq_ui.h"


/////////////////////////////////////////////////////////////////////////////
// Local types
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Local prototypes
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Global variables
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 Init(u32 mode)
{
#if DEFAULT_SRM_ENABLED
  // init BLM
  BLM8X8_Init(0);
#endif

  // initialize parameter/trigger layers
  SEQ_PAR_Init(0);
  SEQ_TRG_Init(0);

  // init sequencer core
  SEQ_CORE_Init(0);

  // init user interface
  SEQ_LED_Init(0);
  SEQ_UI_Init(0);

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
//  This hook is called when a complete MIDI event has been received
/////////////////////////////////////////////////////////////////////////////
void APP_NotifyReceivedEvent(u8 port, mios32_midi_package_t midi_package)
{
  MIOS32_MIDI_SendPackage(port, midi_package);
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called when a SysEx byte has been received
/////////////////////////////////////////////////////////////////////////////
void APP_NotifyReceivedSysEx(u8 port, u8 sysex_byte)
{
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called when a button has been toggled
// pin_value is 1 when button released, and 0 when button pressed
/////////////////////////////////////////////////////////////////////////////
void APP_DIN_NotifyToggle(u32 pin, u32 pin_value)
{
#if COM_DEBUG == 1
  // for debugging
  printf("Pin %3d (SR%d:D%d) = %d\n\r", pin, (pin>>3)+1, pin&7, pin_value);
#endif

  // forward to UI button handler
  SEQ_UI_Button_Handler(pin, pin_value);
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called when a BLM8x8 button has been toggled
// (see also SEQ_TASK_Period1mS)
// pin_value is 1 when button released, and 0 when button pressed
/////////////////////////////////////////////////////////////////////////////
#if DEFAULT_SRM_ENABLED
void APP_BLM8X8_NotifyToggle(u32 pin, u32 pin_value)
{
#if COM_DEBUG == 1
  // for debugging
  printf("BLM Pin %3d (SR%d:D%d) = %d\n\r", pin, (pin>>3)+1, pin&7, pin_value);
#endif

  // forward to UI button handler
  SEQ_UI_Button_Handler(pin + 128, pin_value);
}
#endif


/////////////////////////////////////////////////////////////////////////////
// This hook is called when an encoder has been moved
// incrementer is positive when encoder has been turned clockwise, else
// it is negative
/////////////////////////////////////////////////////////////////////////////
void APP_ENC_NotifyChange(u32 encoder, s32 incrementer)
{
#if COM_DEBUG == 1
  // for debugging
  printf("Enc %2d = %d\n\r", encoder, incrementer);
#endif

  // forward to UI encoder handler
  SEQ_UI_Encoder_Handler(encoder, incrementer);
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called before the shift register chain is scanned
/////////////////////////////////////////////////////////////////////////////
void APP_SRIO_ServicePrepare(void)
{
#if DEFAULT_SRM_ENABLED
  // prepare DOUT registers of 8x8 BLM to drive the column
  BLM8X8_PrepareCol();
#endif
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called after the shift register chain has been scanned
/////////////////////////////////////////////////////////////////////////////
void APP_SRIO_ServiceFinish(void)
{
#if DEFAULT_SRM_ENABLED
  // call the BLM8X8_GetRow function after scan is finished to capture the read DIN values
  BLM8X8_GetRow();
#endif
}


/////////////////////////////////////////////////////////////////////////////
// This task is running endless in background
/////////////////////////////////////////////////////////////////////////////
void APP_Background(void)
{
  // toggle the state of all LEDs (allows to measure the execution speed with a scope)
  MIOS32_BOARD_LED_Set(0xffffffff, ~MIOS32_BOARD_LED_Get());

  // call LCD Handler
  SEQ_UI_LCD_Handler();

  // update LEDs
  SEQ_UI_LED_Handler();
}


/////////////////////////////////////////////////////////////////////////////
// This task is called periodically each mS
/////////////////////////////////////////////////////////////////////////////
void SEQ_TASK_Period1mS(void)
{
  // update high-prio LED functions
  SEQ_UI_LED_Handler_Periodic();

#if DEFAULT_SRM_ENABLED
  // check for BLM8x8 pin changes, call button handler of sequencer on each toggled pin
  BLM8X8_ButtonHandler(APP_BLM8X8_NotifyToggle);
#endif
}
