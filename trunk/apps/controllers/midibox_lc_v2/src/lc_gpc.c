// $Id$
/*
 * MIDIbox LC V2
 * General Purpose Controller
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
#include <string.h>

#include "app.h"
#include "lc_hwcfg.h"
#include "lc_gpc.h"
#include "lc_gpc_lables.h"
#include "lc_vpot.h"
#include "lc_lcd.h"


/////////////////////////////////////////////////////////////////////////////
// Global Variables
/////////////////////////////////////////////////////////////////////////////

char gpc_msg[128];


/////////////////////////////////////////////////////////////////////////////
// Local Variables
/////////////////////////////////////////////////////////////////////////////

static u8 gpc_offset;               // offset for general purpose controller list
static u8 gpc_abs_value[128];       // the absolute values of Vpots in GPC mode

/////////////////////////////////////////////////////////////////////////////
// This function initializes the GPC mode
/////////////////////////////////////////////////////////////////////////////
s32 LC_GPC_Init(u32 mode)
{
  // start at 0 offset
  gpc_offset = 0;

  // update GPC message array
  LC_GPC_Msg_Update();

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// This function returns the absolute value which is assigned to the given vpot
/////////////////////////////////////////////////////////////////////////////
u8 LC_GPC_VPotValueGet(u8 vpot)
{
  return gpc_abs_value[gpc_offset + vpot];
}



/////////////////////////////////////////////////////////////////////////////
// This function is called from APP_MIDI_NotifyPackage
// when a MIDI package has been received
/////////////////////////////////////////////////////////////////////////////
s32 LC_GPC_Received(mios32_midi_package_t package)
{
#if 0
  // TODO: GPC events
  LC_GPC_AbsValue_Received(entry, evnt2);
#endif

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// This function is called from LC_GPC_Received
// when a GPC value has been received via MIDI
/////////////////////////////////////////////////////////////////////////////
s32 LC_GPC_AbsValue_Received(u8 entry, u8 value)
{
  // store value
  gpc_abs_value[entry & 0x7f] = value;

  // request LEDring update
  LC_VPOT_LEDRingUpdateSet(1 << entry);

  // in GPC mode: update GPC message and request display refresh
  if( lc_flags.GPC_SEL ) {
    LC_GPC_Msg_UpdateValues();
    LC_LCD_Update_HostMsg();
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// This function copies lables and values into the GPC message buffer
/////////////////////////////////////////////////////////////////////////////
s32 LC_GPC_Msg_Update(void)
{
  int i, j;
  char *gpc_msg_ptr = (char *)&gpc_msg[0x00];

  for(i=0; i<8; ++i) {
    char *gpc_label_ptr = (char *)&lc_gpc_lables[gpc_offset*5 + i];

    *gpc_msg_ptr++ = ' ';
    for(j=0; j<5; ++j)
      *gpc_msg_ptr++ = *gpc_label_ptr++;
    *gpc_msg_ptr++ = ' ';
  }

  // now copy the values to the second line
  LC_GPC_Msg_UpdateValues(); 

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// This function copies only the values into the GPC message buffer
/////////////////////////////////////////////////////////////////////////////
s32 LC_GPC_Msg_UpdateValues(void)
{
  int i;

  // TODO: provide alternative formats
  for(i=0; i<8; ++i)
    sprintf((char *)&gpc_msg[0x40 + i*7], "  %03d  ", gpc_abs_value[gpc_offset + i]);

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// This function is called from LC_VPOT_SendENCEvent() in lc_vpot.c
// when GPC mode is activated and a V-Pot has been moved
/////////////////////////////////////////////////////////////////////////////
s32 LC_GPC_SendENCEvent(u8 encoder, s32 incrementer)
{
  u8 entry = gpc_offset + encoder;
  int prev_value = gpc_abs_value[entry];
  int new_value = prev_value;
  int min = 0;
  int max = 127;

  // add incrementer to absolute value
  if( incrementer >= 0 ) {
    if( (new_value += incrementer) >= max )
      new_value = max;
  } else {
    if( (new_value += incrementer < min ) )
      new_value = min;
  }

  if( new_value == prev_value )
    return 0; // no change

  // store new value
  LC_GPC_AbsValue_Received(entry, (u8)new_value);

  // send value
#if 0
  u8 evnt0 = MIOS_MPROC_EVENT_TABLE[2*entry+0];
  u8 evnt1 = MIOS_MPROC_EVENT_TABLE[2*entry+1];
#else
  // TODO: define new table
  u8 evnt0 = 0x90;
  u8 evnt1 = entry;
#endif
  u8 evnt2 = (u8)new_value;

  mios32_midi_package_t p;
  p.ALL = 0;
  p.type = (evnt0 >> 4) & 0xf;
  p.evnt0 = evnt0;
  p.evnt1 = evnt1;
  p.evnt2 = evnt2;
  MIOS32_MIDI_SendPackage(DEFAULT, p);

  return 1; // new value sent
}

/////////////////////////////////////////////////////////////////////////////
// This function is called from LC_VPOT_SendJogWheelEvent() in lc_vpot.c
// when GPC mode is activated and the jogwheel has been moved
/////////////////////////////////////////////////////////////////////////////
s32 LC_GPC_SendJogWheelEvent(s32 incrementer)
{
  int prev_value = gpc_offset;
  int new_value = prev_value;
  int min = 0;
  int max = 127 - 8;

  if( incrementer >= 0 ) {
    if( (new_value += incrementer) >= max )
      new_value = max;
  } else {
    if( (new_value += incrementer < min ) )
      new_value = min;
  }

  if( new_value == prev_value )
    return 0; // no change

  gpc_offset = (u8)new_value;

  // update screen
  LC_GPC_Msg_Update();
  LC_LCD_Update_HostMsg();

  // request LEDring update
  LC_VPOT_LEDRingUpdateSet(0xff);

  return 1; // value changed
}
