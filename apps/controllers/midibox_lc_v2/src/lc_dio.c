// $Id$
/*
 * MIDIbox LC V2
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
#include "lc_hwcfg.h"
#include "lc_dio.h"
#include "lc_dio_table.h"
#include "lc_vpot.h"
#include "lc_gpc.h"
#include "lc_lcd.h"

/////////////////////////////////////////////////////////////////////////////
// Local Variables
/////////////////////////////////////////////////////////////////////////////

// the status of the 256 LED functions (each byte contains the status of 8 functions)
static u8 led_status[256/8];


/////////////////////////////////////////////////////////////////////////////
// This function is called from the DIN_NotifyToggle hook in main.c
// when a button has been pressed/depressed
/////////////////////////////////////////////////////////////////////////////
s32 LC_DIO_ButtonHandler(u8 button, u8 pin_value)
{
  u8 button_id;

  // read button ID from table
  switch( lc_flags.LAYER_SEL ) {
    case 1:  button_id = lc_dio_table_layer1[button << 1]; break;
    case 2:  button_id = lc_dio_table_layer2[button << 1]; break;
    default: button_id = lc_dio_table_layer0[button << 1];
  }

  // branch to special function handler if id >= 0x80
  if( button_id >= 0x80 ) {
    return LC_DIO_SFBHandler(button_id, pin_value);
  }

  // send button event
#if 0
  if( pin_value )
    return MIOS32_MIDI_SendNoteOff(DEFAULT, Chn1, button_id, 0x00);
  else
    return MIOS32_MIDI_SendNoteOn(DEFAULT, Chn1, button_id, 0x7f);
#else
  // Interesting: Logic Pro 9 totally runs crazy if some buttons are sent with NoteOff Event!!! 
  // Use Note On with Velocity 0 instead (and send 0x7f for Note On as usual)
  return MIOS32_MIDI_SendNoteOn(DEFAULT, Chn1, button_id, pin_value ? 0x00 : 0x7f);
#endif
}


/////////////////////////////////////////////////////////////////////////////
// This function is called from LC_DIO_ButtonHandler if a special function
// button has been pressed/depressed
/////////////////////////////////////////////////////////////////////////////
s32 LC_DIO_SFBHandler(u8 button_id, u8 pin_value)
{
  u8 gpc_hold_flag = 0;

  // set various flags depending on the IDs which are defined in lc_dio_tables.h
  switch( button_id ) {
    case ID_MBLC_DISPLAY_PAGE0:
      LC_LCD_DisplayPageSet(0);
      break;

    case ID_MBLC_DISPLAY_PAGE1:
      LC_LCD_DisplayPageSet(1);
      break;

    case ID_MBLC_DISPLAY_PAGE2:
      LC_LCD_DisplayPageSet(2);
      break;

    case ID_MBLC_DISPLAY_PAGE3:
      LC_LCD_DisplayPageSet(3);
      break;

    case ID_MBLC_DISPLAY_NEXT:
      if( !pin_value ) {
	LC_LCD_DisplayPageSet((LC_LCD_DisplayPageGet() + 1) & 0x3); // 4 pages
      }
      break;

    case ID_MBLC_SWITCH_LAYER0:
      lc_flags.LAYER_SEL = 0;
      break;

    case ID_MBLC_SWITCH_LAYER1:
      lc_flags.LAYER_SEL = 1;
      break;

    case ID_MBLC_SWITCH_LAYER2:
      lc_flags.LAYER_SEL = 2;
      break;

    case ID_MBLC_TOGGLE_LAYER1:
      if( !pin_value ) {
	lc_flags.LAYER_SEL = lc_flags.LAYER_SEL == 1 ? 0 : 1;
      }
      break;

    case ID_MBLC_TOGGLE_LAYER2:
      if( !pin_value ) {
	lc_flags.LAYER_SEL = lc_flags.LAYER_SEL == 2 ? 0 : 2;
      }
      break;

    case ID_MBLC_HOLD_LAYER1:
      lc_flags.LAYER_SEL = pin_value ? 0 : 1;
      break;

    case ID_MBLC_HOLD_LAYER2:
      lc_flags.LAYER_SEL = pin_value ? 0 : 2;
      break;

    case ID_MBLC_SWITCH_LC:
      lc_flags.GPC_SEL = 0;
      break;

    case ID_MBLC_SWITCH_GPC:
      lc_flags.GPC_SEL = 1;
      break;

    case ID_MBLC_TOGGLE_GPC:
      if( !pin_value ) {
	lc_flags.GPC_SEL = !lc_flags.GPC_SEL;
      }
      break;

    case ID_MBLC_HOLD_GPC:
      lc_flags.GPC_SEL = pin_value ? 0 : 1;
      gpc_hold_flag = 1;
      break;

    case ID_MBLC_SWITCH_LEDMETER0:
      lc_flags.LEDRING_METERS = 0;
      break;

    case ID_MBLC_SWITCH_LEDMETER1:
      lc_flags.LEDRING_METERS = 1;
      break;

    case ID_MBLC_TOGGLE_LEDMETER:
      if( !pin_value ) {
	lc_flags.LEDRING_METERS = !lc_flags.LEDRING_METERS;
      }
      break;

    case ID_MBLC_HOLD_LEDMETER:
      lc_flags.LEDRING_METERS = pin_value ? 0 : 1;
      break;

  }

  // request message message, LED and ledring update
  LC_LCD_Update_HostMsg();
  lc_flags.LED_UPDATE_REQ = 1;
  LC_VPOT_LEDRingUpdateSet(0xff);

  // optional: send MIDI event which notifies if GPC mode is activated or not
#if GPC_BUTTON_SENDS_MIDI_EVENT
  if( !pin_value || gpc_hold_flag ) {
    u8 evnt0 = lc_flags.GPC_SEL ? (GPC_BUTTON_ON_EVNT0) : (GPC_BUTTON_OFF_EVNT0);
    u8 evnt1 = lc_flags.GPC_SEL ? (GPC_BUTTON_ON_EVNT1) : (GPC_BUTTON_OFF_EVNT1);
    u8 evnt2 = lc_flags.GPC_SEL ? (GPC_BUTTON_ON_EVNT2) : (GPC_BUTTON_OFF_EVNT2);

    mios32_midi_package_t p;
    p.ALL = 0;
    p.type = (evnt0 >> 4) & 0xf;
    p.evnt0 = evnt0;
    p.evnt1 = evnt1;
    p.evnt2 = evnt2;
    MIOS32_MIDI_SendPackage(DEFAULT, p);
  }
#endif

  // update message if button pressed and GPC selected
  if( !pin_value && lc_flags.GPC_SEL )
    LC_LCD_Update_HostMsg();

  // update the special function LEDs depending on the new flags:
  LC_DIO_SFBLEDUpdate();

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// This function updates all LEDs which have been assigned to SFBs
/////////////////////////////////////////////////////////////////////////////
s32 LC_DIO_SFBLEDUpdate(void)
{
  u8 page;
  // add IDs >= 0x80 must be handled here!

  page = LC_LCD_DisplayPageGet();
  LC_DIO_LEDSet(ID_MBLC_DISPLAY_PAGE0,  page == 0);
  LC_DIO_LEDSet(ID_MBLC_DISPLAY_PAGE1,  page == 1);
  LC_DIO_LEDSet(ID_MBLC_DISPLAY_PAGE2,  page == 2);
  LC_DIO_LEDSet(ID_MBLC_DISPLAY_PAGE3,  page == 3);
  LC_DIO_LEDSet(ID_MBLC_DISPLAY_NEXT,   page == 0);

  LC_DIO_LEDSet(ID_MBLC_SWITCH_LAYER0,  lc_flags.LAYER_SEL == 0);
  LC_DIO_LEDSet(ID_MBLC_SWITCH_LAYER1,  lc_flags.LAYER_SEL == 1);
  LC_DIO_LEDSet(ID_MBLC_SWITCH_LAYER2,  lc_flags.LAYER_SEL == 2);
  LC_DIO_LEDSet(ID_MBLC_TOGGLE_LAYER1,  lc_flags.LAYER_SEL == 1);
  LC_DIO_LEDSet(ID_MBLC_TOGGLE_LAYER2,  lc_flags.LAYER_SEL == 2);
  LC_DIO_LEDSet(ID_MBLC_HOLD_LAYER1,    lc_flags.LAYER_SEL == 1);
  LC_DIO_LEDSet(ID_MBLC_HOLD_LAYER2,    lc_flags.LAYER_SEL == 2);

  LC_DIO_LEDSet(ID_MBLC_SWITCH_LC,     !lc_flags.GPC_SEL);
  LC_DIO_LEDSet(ID_MBLC_SWITCH_GPC,     lc_flags.GPC_SEL);
  LC_DIO_LEDSet(ID_MBLC_TOGGLE_GPC,     lc_flags.GPC_SEL);
  LC_DIO_LEDSet(ID_MBLC_HOLD_GPC,       lc_flags.GPC_SEL);

  LC_DIO_LEDSet(ID_MBLC_SWITCH_LEDMETER0, !lc_flags.LEDRING_METERS);
  LC_DIO_LEDSet(ID_MBLC_SWITCH_LEDMETER1,  lc_flags.LEDRING_METERS);
  LC_DIO_LEDSet(ID_MBLC_TOGGLE_LEDMETER,   lc_flags.LEDRING_METERS);
  LC_DIO_LEDSet(ID_MBLC_HOLD_LEDMETER,     lc_flags.LEDRING_METERS);

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// This function is called from the Tick() function in main.c when 
// all LEDs should be updated
/////////////////////////////////////////////////////////////////////////////
s32 LC_DIO_LED_CheckUpdate(void)
{
  u8 *table_layer;
  int led_ctr;

  if( lc_flags.LED_UPDATE_REQ ) {
    lc_flags.LED_UPDATE_REQ = 0;

    if( lc_flags.LAYER_SEL == 1 )
      table_layer = (u8 *)&lc_dio_table_layer1[0];
    else if( lc_flags.LAYER_SEL == 2 )
      table_layer = (u8 *)&lc_dio_table_layer2[0];
    else
      table_layer = (u8 *)&lc_dio_table_layer0[0];

    for(led_ctr=0; led_ctr<0x80; ++led_ctr) {
      u8 led_id = table_layer[2*led_ctr + 1];

      if( led_id != ID_IGNORE )
	MIOS32_DOUT_PinSet(led_ctr, (led_status[led_id >> 3] & (1 << (led_id & 0x7))) ? 1 : 0);
    }
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// This function is called from LC_MIDI_Received in lc_midi.c when
// a LED status message has been received
/////////////////////////////////////////////////////////////////////////////
s32 LC_DIO_LEDSet(u8 lc_id, u8 led_on) 
{
  u8 ix = lc_id >> 3;

  // note that led_status is a packed array, each byte contains the status of 8 LEDs
  if( led_on ) {
    led_status[ix] |= (1 << (lc_id & 0x07));
  } else {
    led_status[ix] &= ~(1 << (lc_id & 0x07));
  }

  // request update of all LEDs
  lc_flags.LED_UPDATE_REQ = 1;

  return 0;
}
