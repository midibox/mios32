// $Id$
/*
 * MIDIbox SID V3
 *
 * ==========================================================================
 *
 *  Copyright (C) 2009 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

/////////////////////////////////////////////////////////////////////////////
// Include files
/////////////////////////////////////////////////////////////////////////////

#include <mios32.h>

#include "tasks.h"

#include <aout.h>
#include <sid.h>

#include "app.h"
#include "sid_sysex.h"


/////////////////////////////////////////////////////////////////////////////
// for optional debugging messages via DEBUG_MSG (defined in mios32_config.h)
/////////////////////////////////////////////////////////////////////////////
#define DEBUG_VERBOSE_LEVEL 0


/////////////////////////////////////////////////////////////////////////////
// Local prototypes
/////////////////////////////////////////////////////////////////////////////
static s32 NOTIFY_MIDI_Rx(mios32_midi_port_t port, u8 byte);
static s32 NOTIFY_MIDI_Tx(mios32_midi_port_t port, mios32_midi_package_t package);
static s32 NOTIFY_MIDI_TimeOut(mios32_midi_port_t port);


/////////////////////////////////////////////////////////////////////////////
// This hook is called after startup to initialize the application
/////////////////////////////////////////////////////////////////////////////
void APP_Init(void)
{
  // initialize all LEDs
  MIOS32_BOARD_LED_Init(0xffffffff);

  // initialize AOUT driver
  AOUT_Init(0);

  // initialize SID module
  SID_Init(0);

  // start tasks (differs between MIOS32 and MacOS)
  TASKS_Init(0);

  // install MIDI Rx/Tx callback functions
  MIOS32_MIDI_DirectRxCallback_Init(NOTIFY_MIDI_Rx);
  MIOS32_MIDI_DirectTxCallback_Init(NOTIFY_MIDI_Tx);

  // install timeout callback function
  MIOS32_MIDI_TimeOutCallback_Init(NOTIFY_MIDI_TimeOut);

  // init MIDI parsers
  SID_SYSEX_Init(0);


#if 0

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
#endif

}


/////////////////////////////////////////////////////////////////////////////
// This task is running endless in background
/////////////////////////////////////////////////////////////////////////////
void APP_Background(void)
{
  // toggle the state of all LEDs (allows to measure the execution speed with a scope)
#if 0
  MIOS32_BOARD_LED_Set(0xffffffff, ~MIOS32_BOARD_LED_Get());
#endif
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called when a MIDI package has been received
/////////////////////////////////////////////////////////////////////////////
void APP_MIDI_NotifyPackage(mios32_midi_port_t port, mios32_midi_package_t midi_package)
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
#if DEBUG_VERBOSE_LEVEL >= 1
  DEBUG_MSG("Pin %3d (SR%d:D%d) = %d\n", pin, (pin>>3)+1, pin&7, pin_value);
#endif
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called when an encoder has been moved
// incrementer is positive when encoder has been turned clockwise, else
// it is negative
/////////////////////////////////////////////////////////////////////////////
void APP_ENC_NotifyChange(u32 encoder, s32 incrementer)
{
#if DEBUG_VERBOSE_LEVEL >= 1
  DEBUG_MSG("Enc %2d = %d\n", encoder, incrementer);
#endif
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called when a pot has been moved
/////////////////////////////////////////////////////////////////////////////
void APP_AIN_NotifyChange(u32 pin, u32 pin_value)
{
}


/////////////////////////////////////////////////////////////////////////////
// This task is called periodically each mS as well
// it handles SID accesses
/////////////////////////////////////////////////////////////////////////////
void SEQ_TASK_SID(void)
{
  // update SID registers if not in ASID mode
  if( SID_SYSEX_ASID_ModeGet() == SID_SYSEX_ASID_MODE_OFF )
    SID_Update(0);
}


/////////////////////////////////////////////////////////////////////////////
// This task is called periodically each mS
/////////////////////////////////////////////////////////////////////////////
void SEQ_TASK_Period1mS(void)
{
}


/////////////////////////////////////////////////////////////////////////////
// This task is called each mS with lowest priority
/////////////////////////////////////////////////////////////////////////////
void SEQ_TASK_Period1mS_LowPrio(void)
{
}

/////////////////////////////////////////////////////////////////////////////
// This task is called periodically each second
/////////////////////////////////////////////////////////////////////////////
void SEQ_TASK_Period1S(void)
{
  static u8 wait_boot_ctr = 2; // wait 2 seconds before loading from SD Card - this is to increase the time where the boot screen is print!
  u8 load_sd_content = 0;

  // don't check for SD Card if MSD enabled
  if( TASK_MSD_EnableGet() > 0 )
    return;

  // boot phase of 2 seconds finished?
  if( wait_boot_ctr ) {
    --wait_boot_ctr;
    return;
  }

  // check if SD Card connected
  MUTEX_SDCARD_TAKE;

#if 0
  s32 status = SID_FILE_CheckSDCard();
#endif

  MUTEX_SDCARD_GIVE;
}


/////////////////////////////////////////////////////////////////////////////
// Installed via MIOS32_MIDI_DirectRxCallback_Init
/////////////////////////////////////////////////////////////////////////////
static s32 NOTIFY_MIDI_Rx(mios32_midi_port_t port, u8 midi_byte)
{
  return 0; // no error, no filtering
}


/////////////////////////////////////////////////////////////////////////////
// Installed via MIOS32_MIDI_DirectTxCallback_Init
/////////////////////////////////////////////////////////////////////////////
static s32 NOTIFY_MIDI_Tx(mios32_midi_port_t port, mios32_midi_package_t package)
{
  return 0;
}


/////////////////////////////////////////////////////////////////////////////
// Installed via MIOS32_MIDI_TimeoutCallback_Init
/////////////////////////////////////////////////////////////////////////////
static s32 NOTIFY_MIDI_TimeOut(mios32_midi_port_t port)
{
  SID_SYSEX_TimeOut(port);

  return 0;
}
