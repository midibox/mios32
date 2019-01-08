// $Id$
/*
 * MIDIbox MM V3
 * Digital IO Handler
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

#include "app.h"
#include "mm_hwcfg.h"
#include "mm_dio.h"
#include "mm_dio_table.h"
#include "mm_vpot.h"
#include "mm_gpc.h"
#include "mm_lcd.h"

/////////////////////////////////////////////////////////////////////////////
// Local Variables
/////////////////////////////////////////////////////////////////////////////

// the status of the 256 LED functions (each byte contains the status of 8 functions)
static u8 led_status[256/8];


/////////////////////////////////////////////////////////////////////////////
// This function is called from the DIN_NotifyToggle hook in main.c
// when a button has been pressed/depressed
/////////////////////////////////////////////////////////////////////////////
s32 MM_DIO_ButtonHandler(u8 button, u8 pin_value)
{
  u8 button_id;

  // read button ID from table
  switch( mm_flags.LAYER_SEL ) {
    case 1:  button_id = mm_dio_table_layer1[button << 1]; break;
    case 2:  button_id = mm_dio_table_layer2[button << 1]; break;
    default: button_id = mm_dio_table_layer0[button << 1];
  }

  if( mm_hwcfg_verbose_level >= 2 ) {
    MIOS32_MIDI_SendDebugMessage("MM_DIO_ButtonHandler: button ID 0x%02x\n", button_id);
  }

  // branch to special function handler if id >= 0x80
  if( button_id >= 0x80 ) {
    return MM_DIO_SFBHandler(button_id, pin_value);
  }

  // send button event
  // branch depending on button ID range

  // 0x00..0x57 are defined in mm_dio_table.h as MM events
  if( button_id < 0x58 ) {

    // select third and fith byte depending on button id
    u8 cc_value2 = button_id & 0x07;

    u8 cc_value1;
    if( button_id <= 0x2f ) {        // Range #1: Fader/Select/Mute/Solo/Multi/RecRdy
      cc_value1 = button_id & 0x07;
      cc_value2 = (button_id >> 3) & 0x07;
    } else if( button_id <= 0x37 ) { // Range #2: 0x08 group
      cc_value1 = 0x08;
    } else if( button_id <= 0x3f ) { // Range #2: 0x09 group
      cc_value1 = 0x09;
    } else if( button_id <= 0x4f ) { // Range #3: 0x0a group
      cc_value1 = 0x0a;
    } else {                         // Range #3: 0x0b group
      cc_value1 = 0x0b;
    }

    // button pressed/depressed?
    cc_value2 |=  (pin_value ? 0x00 : 0x40);

    u8 chn = (mm_hwcfg_midi_channel-1) & 0x0f;
    MIOS32_MIDI_SendCC(DEFAULT, chn, 0x0f, cc_value1);
    MIOS32_MIDI_SendCC(DEFAULT, chn, 0x2f, cc_value2);

    if( mm_hwcfg_verbose_level >= 2 ) {
      MIOS32_MIDI_SendDebugMessage("  -> sent 0F %02X 2F %02X", cc_value1, cc_value2);
    }

  // 0x58..0x7f could be used for other purposes
  } else if( button_id < 0x80 ) {
    // nothing to do (yet)
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// This function is called from MM_DIO_ButtonHandler if a special function
// button has been pressed/depressed
/////////////////////////////////////////////////////////////////////////////
s32 MM_DIO_SFBHandler(u8 button_id, u8 pin_value)
{
  u8 gpc_hold_flag = 0;

  // set various flags depending on the IDs which are defined in mm_dio_tables.h
  switch( button_id ) {
    case ID_MBMM_SWITCH_LAYER0:
      mm_flags.LAYER_SEL = 0;
      break;

    case ID_MBMM_SWITCH_LAYER1:
      mm_flags.LAYER_SEL = 1;
      break;

    case ID_MBMM_SWITCH_LAYER2:
      mm_flags.LAYER_SEL = 2;
      break;

    case ID_MBMM_TOGGLE_LAYER1:
      if( !pin_value ) {
	mm_flags.LAYER_SEL = mm_flags.LAYER_SEL == 1 ? 0 : 1;
      }
      break;

    case ID_MBMM_TOGGLE_LAYER2:
      if( !pin_value ) {
	mm_flags.LAYER_SEL = mm_flags.LAYER_SEL == 2 ? 0 : 2;
      }
      break;

    case ID_MBMM_HOLD_LAYER1:
      mm_flags.LAYER_SEL = pin_value ? 0 : 1;
      break;

    case ID_MBMM_HOLD_LAYER2:
      mm_flags.LAYER_SEL = pin_value ? 0 : 2;
      break;

    case ID_MBMM_SWITCH_MM:
      mm_flags.GPC_SEL = 0;
      break;

    case ID_MBMM_SWITCH_GPC:
      mm_flags.GPC_SEL = 1;
      break;

    case ID_MBMM_TOGGLE_GPC:
      if( !pin_value ) {
	mm_flags.GPC_SEL = !mm_flags.GPC_SEL;
      }
      break;

    case ID_MBMM_HOLD_GPC:
      mm_flags.GPC_SEL = pin_value ? 0 : 1;
      gpc_hold_flag = 1;
      break;

    case ID_MBMM_SWITCH_LEDMETER0:
      mm_flags.LEDRING_METERS = 0;
      break;

    case ID_MBMM_SWITCH_LEDMETER1:
      mm_flags.LEDRING_METERS = 1;
      break;

    case ID_MBMM_TOGGLE_LEDMETER:
      if( !pin_value ) {
	mm_flags.LEDRING_METERS = !mm_flags.LEDRING_METERS;
      }
      break;

    case ID_MBMM_HOLD_LEDMETER:
      mm_flags.LEDRING_METERS = pin_value ? 0 : 1;
      break;

  }

  // request message message, LED and ledring update
  mm_flags.MSG_UPDATE_REQ = 1;
  mm_flags.LED_UPDATE_REQ = 1;
  MM_VPOT_LEDRingUpdateSet(0xff);

  // optional: send MIDI event which notifies if GPC mode is activated or not
  if( mm_hwcfg_gpc.enabled ) {
    if( !pin_value || gpc_hold_flag ) {
      u8 evnt0 = mm_flags.GPC_SEL ? mm_hwcfg_gpc.on_evnt[0] : mm_hwcfg_gpc.off_evnt[0];
      u8 evnt1 = mm_flags.GPC_SEL ? mm_hwcfg_gpc.on_evnt[1] : mm_hwcfg_gpc.off_evnt[1];
      u8 evnt2 = mm_flags.GPC_SEL ? mm_hwcfg_gpc.on_evnt[2] : mm_hwcfg_gpc.off_evnt[2];

      mios32_midi_package_t p;
      p.ALL = 0;
      p.type = (evnt0 >> 4) & 0xf;
      p.evnt0 = evnt0;
      p.evnt1 = evnt1;
      p.evnt2 = evnt2;
      MIOS32_MIDI_SendPackage(DEFAULT, p);
    }
  }

  // update message if button pressed and GPC selected
  if( !pin_value && mm_flags.GPC_SEL )
    mm_flags.MSG_UPDATE_REQ = 1;

  // update the special function LEDs depending on the new flags:
  MM_DIO_SFBLEDUpdate();

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// This function updates all LEDs which have been assigned to SFBs
/////////////////////////////////////////////////////////////////////////////
s32 MM_DIO_SFBLEDUpdate(void)
{
  MM_DIO_LEDSet(ID_MBMM_SWITCH_LAYER0,  mm_flags.LAYER_SEL == 0);
  MM_DIO_LEDSet(ID_MBMM_SWITCH_LAYER1,  mm_flags.LAYER_SEL == 1);
  MM_DIO_LEDSet(ID_MBMM_SWITCH_LAYER2,  mm_flags.LAYER_SEL == 2);
  MM_DIO_LEDSet(ID_MBMM_TOGGLE_LAYER1,  mm_flags.LAYER_SEL == 1);
  MM_DIO_LEDSet(ID_MBMM_TOGGLE_LAYER2,  mm_flags.LAYER_SEL == 2);
  MM_DIO_LEDSet(ID_MBMM_HOLD_LAYER1,    mm_flags.LAYER_SEL == 1);
  MM_DIO_LEDSet(ID_MBMM_HOLD_LAYER2,    mm_flags.LAYER_SEL == 2);

  MM_DIO_LEDSet(ID_MBMM_SWITCH_MM,     !mm_flags.GPC_SEL);
  MM_DIO_LEDSet(ID_MBMM_SWITCH_GPC,     mm_flags.GPC_SEL);
  MM_DIO_LEDSet(ID_MBMM_TOGGLE_GPC,     mm_flags.GPC_SEL);
  MM_DIO_LEDSet(ID_MBMM_HOLD_GPC,       mm_flags.GPC_SEL);

  MM_DIO_LEDSet(ID_MBMM_SWITCH_LEDMETER0, !mm_flags.LEDRING_METERS);
  MM_DIO_LEDSet(ID_MBMM_SWITCH_LEDMETER1,  mm_flags.LEDRING_METERS);
  MM_DIO_LEDSet(ID_MBMM_TOGGLE_LEDMETER,   mm_flags.LEDRING_METERS);
  MM_DIO_LEDSet(ID_MBMM_HOLD_LEDMETER,     mm_flags.LEDRING_METERS);

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// This function is called from the Tick() function in main.c when 
// all LEDs should be updated
/////////////////////////////////////////////////////////////////////////////
s32 MM_DIO_LED_CheckUpdate(void)
{
  u8 *table_layer;
  int led_ctr;

  if( mm_flags.LED_UPDATE_REQ ) {
    mm_flags.LED_UPDATE_REQ = 0;

    if( mm_flags.LAYER_SEL == 1 )
      table_layer = (u8 *)&mm_dio_table_layer1[0];
    else if( mm_flags.LAYER_SEL == 2 )
      table_layer = (u8 *)&mm_dio_table_layer2[0];
    else
      table_layer = (u8 *)&mm_dio_table_layer0[0];

    for(led_ctr=0; led_ctr<0x80; ++led_ctr) {
      u8 led_id = table_layer[2*led_ctr + 1];

      if( led_id != ID_IGNORE )
	MIOS32_DOUT_PinSet(led_ctr, (led_status[led_id >> 3] & (1 << (led_id & 0x7))) ? 1 : 0);
    }
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// This function is called from MM_MIDI_Received in mm_midi.c when
// a LED status message has been received
/////////////////////////////////////////////////////////////////////////////
s32 MM_DIO_LEDSet(u8 mm_id, u8 led_on) 
{
  u8 ix = mm_id >> 3;

  // note that led_status is a packed array, each byte contains the status of 8 LEDs
  if( led_on ) {
    led_status[ix] |= (1 << (mm_id & 0x07));
  } else {
    led_status[ix] &= ~(1 << (mm_id & 0x07));
  }

  // request update of all LEDs
  mm_flags.LED_UPDATE_REQ = 1;

  return 0;
}


/////////////////////////////////////////////////////////////////////////////
// This function is called from MM_MIDI_Received in mm_midi.c when
// a LED status message has been received
/////////////////////////////////////////////////////////////////////////////
const u8 mm_id_map_0a[8] = { ID_BANK, ID_GROUP, ID_RECRDY_x_FUNCTA, ID_FUNCTA_LED, ID_WRITE_x_FUNCTB, ID_FUNCTB_LED, ID_OTHER_x_FUNCTC, ID_FUNCTC_LED };
const u8 mm_id_map_0b[8] = { ID_FX_BYPASS_x_EFFECT1, ID_EFFECT1_LED, ID_SEND_MUTE_x_EFFECT2, ID_EFFECT2_LED, ID_PRE_POST_x_EFFECT3, ID_EFFECT3_LED, ID_SELECT_x_EFFECT4, ID_EFFECT4_LED };

s32 MM_DIO_LEDHandler(u8 led_id_l, u8 led_id_h)
{
  u8 mm_id = 0;
  u8 ix = led_id_l & 0x07;

  // map LSB/MSB to MIDIbox MM ID
  if( led_id_h <= 0x07 ) {
    mm_id = ((led_id_l << 3) & 0x38) | led_id_h;
  } else if( led_id_h == 0x08 ) {
    mm_id = ix | 0x30;
  } else if( led_id_h == 0x09 ) {
    mm_id = ix | 0x38;
  } else if( led_id_h == 0x0a ) {
    mm_id = mm_id_map_0a[ix];
  } else if( led_id_h == 0x0b ) {
    mm_id = mm_id_map_0b[ix];
  } else {
    return -1; // invalid ID
  }

  u8 led_on = (led_id_l & (1 << 6 )) ? 1 : 0;

  if( mm_hwcfg_verbose_level >= 2 ) {
    MIOS32_MIDI_SendDebugMessage("MM_DIO_LedHandler(0x%02x, 0x%02x) -> mm_id 0x%02x %s\n",
				 led_id_l, led_id_h,
				 mm_id, led_on ? "on" : "off");
  }

  // set LED status
  return MM_DIO_LEDSet(mm_id, led_on);
}

