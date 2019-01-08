// $Id$
/*
 * MIDIbox LC V2
 * VPot Handler
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
#include "lc_lcd.h"
#include "lc_vpot.h"
#include "lc_gpc.h"
#include "lc_mf.h"
#include "lc_meters.h"

/////////////////////////////////////////////////////////////////////////////
// Local Variables
/////////////////////////////////////////////////////////////////////////////

// the "vpot" display type and values (virtual VPot positions)
static u8 display_type;
static u8 vpot_abs_value[8];

// the LEDring patterns
static u16 ledring_pattern[8]; // 16 bit words for 8 v-pots
static u8 ledring_update_req; // 8 update request flags

// this table defines the ledring patterns for 16 steps and 4 modes
// note that the 12th LED (the center LED below the encoder) is set by
// LC_VPOT_LEDRing_CheckUpdates seperately if the V-Pot pointer hasn't been received from host
static const u16 preset_patterns[4*16] = {
  // 16 entries for LED pattern #1
  0x0000, //   b'0000000000000000'
  0x0001, //   b'0000000000000001'
  0x0002, //   b'0000000000000010'
  0x0004, //   b'0000000000000100'
  0x0008, //   b'0000000000001000'
  0x0010, //   b'0000000000010000'
  0x0020, //   b'0000000000100000'
  0x0040, //   b'0000000001000000'
  0x0080, //   b'0000000010000000'
  0x0100, //   b'0000000100000000'
  0x0200, //   b'0000001000000000'
  0x0400, //   b'0000010000000000'
  0x0400, //   b'0000010000000000'
  0x0400, //   b'0000010000000000'
  0x0400, //   b'0000010000000000'
  0x0400, //   b'0000010000000000'

	//	16 entries for LED pattern #2	
  0x0000, //   b'0000000000000000'
  0x003f, //   b'0000000000111111'
  0x003e, //   b'0000000000111110'
  0x003c, //   b'0000000000111100'
  0x0038, //   b'0000000000111000'
  0x0030, //   b'0000000000110000'
  0x0020, //   b'0000000000100000'
  0x0060, //   b'0000000001100000'
  0x00e0, //   b'0000000011100000'
  0x01e0, //   b'0000000111100000'
  0x03e0, //   b'0000001111100000'
  0x07e0, //   b'0000011111100000'
  0x07e0, //   b'0000011111100000'
  0x07e0, //   b'0000011111100000'
  0x07e0, //   b'0000011111100000'
  0x07e0, //   b'0000011111100000'

	// 16 entries for LED pattern #3
  0x0000, //   b'0000000000000000'
  0x0001, //   b'0000000000000001'
  0x0003, //   b'0000000000000011'
  0x0007, //   b'0000000000000111'
  0x000f, //   b'0000000000001111'
  0x001f, //   b'0000000000011111'
  0x003f, //   b'0000000000111111'
  0x007f, //   b'0000000001111111'
  0x00ff, //   b'0000000011111111'
  0x01ff, //   b'0000000111111111'
  0x03ff, //   b'0000001111111111'
  0x07ff, //   b'0000011111111111'
  0x07ff, //   b'0000011111111111'
  0x07ff, //   b'0000011111111111'
  0x07ff, //   b'0000011111111111'
  0x07ff, //   b'0000011111111111'

	// 16 entries for LED pattern #4
  0x0000, //   b'0000000000000000'
  0x0020, //   b'0000000000100000'
  0x0070, //   b'0000000001110000'
  0x00f8, //   b'0000000011111000'
  0x01fc, //   b'0000000111111100'
  0x03fe, //   b'0000001111111110'
  0x07ff, //   b'0000011111111111'
  0x07ff, //   b'0000011111111111'
  0x07ff, //   b'0000011111111111'
  0x07ff, //   b'0000011111111111'
  0x07ff, //   b'0000011111111111'
  0x07ff, //   b'0000011111111111'
  0x07ff, //   b'0000011111111111'
  0x07ff, //   b'0000011111111111'
  0x07ff, //   b'0000011111111111'
  0x07ff, //   b'0000011111111111'
  };

// this table defines the ledring patterns for 32 steps
// not that the 12th LED (the center LED below the encoder) is set by
// LC_VPOT_LEDRingHandler seperately if the V-Pot pointer hasn't been received from host
static const u16 preset_patterns_gpc[32] = {
  0x0001, //   b'0000000000000001'
  0x0001, //   b'0000000000000001'
  0x0003, //   b'0000000000000011'
  0x0003, //   b'0000000000000011'
  0x0003, //   b'0000000000000011'
  0x0007, //   b'0000000000000111'
  0x0007, //   b'0000000000000111'
  0x0007, //   b'0000000000000111'
  0x000f, //   b'0000000000001111'
  0x000f, //   b'0000000000001111'
  0x000f, //   b'0000000000001111'
  0x001f, //   b'0000000000011111'
  0x001f, //   b'0000000000011111'
  0x001f, //   b'0000000000011111'
  0x003f, //   b'0000000000111111'
  0x003f, //   b'0000000000111111'
  0x003f, //   b'0000000000111111'
  0x003f, //   b'0000000000111111'
  0x007f, //   b'0000000001111111'
  0x007f, //   b'0000000001111111'
  0x007f, //   b'0000000001111111'
  0x00ff, //   b'0000000011111111'
  0x00ff, //   b'0000000011111111'
  0x00ff, //   b'0000000011111111'
  0x01ff, //   b'0000000111111111'
  0x01ff, //   b'0000000111111111'
  0x01ff, //   b'0000000111111111'
  0x03ff, //   b'0000001111111111'
  0x03ff, //   b'0000001111111111'
  0x03ff, //   b'0000001111111111'
  0x07ff, //   b'0000011111111111'
  0x07ff, //   b'0000011111111111'
  };


/////////////////////////////////////////////////////////////////////////////
// This function initializes the LEDring handler and the encoder speed modes
/////////////////////////////////////////////////////////////////////////////
s32 LC_VPOT_Init(u32 mode)
{
  int i;

  // request update of all LEDring patterns
  display_type = 0x00;
  ledring_update_req = 0xff;

  // initialize rotary encoders
  for(i=0; i<LC_HWCFG_ENC_NUM; ++i)
    MIOS32_ENC_ConfigSet(i, lc_hwcfg_encoders[i]);

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// This function returns the vpot display type
/////////////////////////////////////////////////////////////////////////////
u8 LC_VPOT_DisplayTypeGet(void)
{
  return display_type;
}

/////////////////////////////////////////////////////////////////////////////
// This function sets the vpot display type
/////////////////////////////////////////////////////////////////////////////
s32 LC_VPOT_DisplayTypeSet(u8 type)
{
  display_type = type;
  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// This function returns the absolute vpot value
/////////////////////////////////////////////////////////////////////////////
u8 LC_VPOT_ValueGet(u8 vpot)
{
  return vpot_abs_value[vpot];
}

/////////////////////////////////////////////////////////////////////////////
// This function sets the absolute vpot value
/////////////////////////////////////////////////////////////////////////////
s32 LC_VPOT_ValueSet(u8 vpot, u8 value)
{
  vpot_abs_value[vpot] = value;

  MIOS32_IRQ_Disable();
  ledring_update_req |= (1 << vpot); // request update of ledring
  MIOS32_IRQ_Enable();

  LC_LCD_Update_Knob(vpot); // request update on LCD

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// This function requests a LEDring update for the given VPots
/////////////////////////////////////////////////////////////////////////////
s32 LC_VPOT_LEDRingUpdateSet(u8 rings)
{
  MIOS32_IRQ_Disable();
  ledring_update_req |= rings;
  MIOS32_IRQ_Enable();

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// This function should be called from ENC_NotifyChange() in main.c
// when a V-Pot has been moved
/////////////////////////////////////////////////////////////////////////////
s32 LC_VPOT_SendENCEvent(u8 encoder, s32 incrementer)
{
  if( lc_hwcfg_verbose_level >= 2 ) {
    MIOS32_MIDI_SendDebugMessage("  -> LC_VPOT_SendEncEvent(%d, %d)\n", encoder, incrementer);
  }

  if( lc_flags.GPC_SEL ) {
    // in GPC mode: change GPC value
    LC_GPC_SendENCEvent(encoder, incrementer);
  } else {
    // otherwise send relative event to host application
    u8 cc_number = 0x10 | (encoder & 0x07);
    u8 value = (incrementer < 0) ? ((-incrementer & 0x3f) | 0x40) : ((incrementer & 0x3f) | 0x00);
    MIOS32_MIDI_SendCC(DEFAULT, Chn1, cc_number, value);
  }

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// This function should be called from ENC_NotifyChange() in main.c
// when the jogwheel has been moved
/////////////////////////////////////////////////////////////////////////////
s32 LC_VPOT_SendJogWheelEvent(s32 incrementer)
{
  if( lc_hwcfg_verbose_level >= 2 ) {
    MIOS32_MIDI_SendDebugMessage("  -> LC_VPOT_SendJogWheelEvent(%d)\n", incrementer);
  }

  if( lc_flags.GPC_SEL ) {
    // in GPC mode: select GPC offset
    LC_GPC_SendJogWheelEvent(incrementer);
  } else {
    // otherwise send relative event to host application
    u8 cc_number = 0x3c;
    u8 value = (incrementer < 0) ? ((-incrementer & 0x3f) | 0x40) : ((incrementer & 0x3f) | 0x00);
    MIOS32_MIDI_SendCC(DEFAULT, Chn1, cc_number, value);
  }

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// This function should be called from ENC_NotifyChange() in main.c
// when a V-Pot has been moved which is assigned to a fader function
/////////////////////////////////////////////////////////////////////////////
s32 LC_VPOT_SendFADEREvent(u8 encoder, s32 incrementer)
{
  if( lc_hwcfg_verbose_level >= 2 ) {
    MIOS32_MIDI_SendDebugMessage("  -> LC_VPOT_SendFADEREvent(%d, %d)\n", encoder, incrementer);
  }

  // add incrementer to absolute value
  int prev_value = LC_MF_FaderPosGet(encoder);
  int new_value = prev_value + incrementer;
  
  if( new_value < 0 ) { new_value = 0; }
  else if( new_value >= 0x3ff ) { new_value = 0x3ff; }

  if( new_value != prev_value ) {
    // send fader event
    LC_MF_FaderEvent(encoder, new_value);
  }

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// This function should be called from Tick() in main.c
// it updates the LEDring patterns for which an update has been requested
/////////////////////////////////////////////////////////////////////////////
s32 LC_VPOT_LEDRing_CheckUpdates(void)
{
  u8 ledring;
  u8 ledring_select;
  u8 value;
  u8 center_led;

  for(ledring=0, ledring_select=0x01; ledring<8; ++ledring) {

    // check if update has been requested
    if( ledring_update_req & ledring_select ) {
      MIOS32_IRQ_Disable();
      ledring_update_req &= ~ledring_select; // clear request flag
      MIOS32_IRQ_Enable();
      
      // branch depending on mode
      if( lc_flags.GPC_SEL ) {
	value = LC_GPC_VPotValueGet(ledring);
	ledring_pattern[ledring] = preset_patterns_gpc[value >> 2];
      } else {
	value = vpot_abs_value[ledring];
	center_led = (value & (1 << 6)) ? 1 : 0;
	ledring_pattern[ledring] = preset_patterns[value & 0x3f] | (center_led ? (1 << (12-1)) : 0);
      }
    }

    ledring_select <<= 1; // shift left the select bit
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// This function should be called from SR_Service_Prepare in main.c
// it copies the current LEDring pattern to the DOUT registers
/////////////////////////////////////////////////////////////////////////////
s32 LC_VPOT_LEDRing_SRHandler(void)
{
  static u8 sr_ctr = 0;
  u16 anode_pattern;

  // we have different code depending on LEDRINGS_ENABLED and METERS_ENABLED
  if( lc_hwcfg_ledrings.cathodes_ledrings_sr == 0 && lc_hwcfg_ledrings.cathodes_meters_sr == 0 ) {
    return 0; // nothing to do here...
  } else if( lc_hwcfg_ledrings.cathodes_ledrings_sr && lc_hwcfg_ledrings.cathodes_meters_sr == 0 ) {
    // increment the counter which selects the ledring/meter that will be visible during
    // the next SRIO update cycle --- wrap at 8 (0, 1, 2, 3, 4, 5, 6, 7, 0, 1, 2, ...)
    sr_ctr = (sr_ctr + 1) & 0x07;

    // the cathode selection pattern
    u8 select_mask = ~(1 << sr_ctr);

    // select the cathode of the LEDring (set the appr. pin to 0, and all others to 1)
    if( lc_hwcfg_ledrings.cathodes_ledrings_sr )
      MIOS32_DOUT_SRSet(lc_hwcfg_ledrings.cathodes_ledrings_sr-1, select_mask);

    // set the LEDring pattern on the anodes
    anode_pattern = lc_flags.LEDRING_METERS ? meter_pattern[sr_ctr] : ledring_pattern[sr_ctr];

  } else if( lc_hwcfg_ledrings.cathodes_ledrings_sr == 0 && lc_hwcfg_ledrings.cathodes_meters_sr ) {

    // increment the counter which selects the ledring/meter that will be visible during
    // the next SRIO update cycle --- wrap at 8 (0, 1, 2, 3, 4, 5, 6, 7, 0, 1, 2, ...)
    sr_ctr = (sr_ctr + 1) & 0x07;

    // the cathode selection pattern
    u8 select_mask = ~(1 << sr_ctr);

    // select the cathode of the meter (set the appr. pin to 0, and all others to 1)
    if( lc_hwcfg_ledrings.cathodes_meters_sr )
      MIOS32_DOUT_SRSet(lc_hwcfg_ledrings.cathodes_meters_sr-1, select_mask);

    // set the meter pattern on the anodes
    anode_pattern = lc_flags.LEDRING_METERS ? ledring_pattern[sr_ctr] : meter_pattern[sr_ctr];

  } else {

    // increment the counter which selects the ledring/meter that will be visible during
    // the next SRIO update cycle --- wrap at 8 (0, 1, 2, 3, ..., 15, 0, 1, 2, ...)
    sr_ctr = (sr_ctr + 1) & 0x0f;

    // the cathode selection pattern
    u8 select_mask = ~(1 << (sr_ctr & 0x7));

    if( sr_ctr < 8 ) {
      // select the cathode of the ledring (set the appr. pin to 0, and all others to 1)
      if( lc_hwcfg_ledrings.cathodes_ledrings_sr )
	MIOS32_DOUT_SRSet(lc_hwcfg_ledrings.cathodes_ledrings_sr-1, select_mask);
      if( lc_hwcfg_ledrings.cathodes_meters_sr )
	MIOS32_DOUT_SRSet(lc_hwcfg_ledrings.cathodes_meters_sr-1, 0xff);

      // set the LEDring pattern on the anodes
      anode_pattern = lc_flags.LEDRING_METERS ? meter_pattern[sr_ctr] : ledring_pattern[sr_ctr];
    } else {
      // select the cathode of the meter (set the appr. pin to 0, and all others to 1)
      if( lc_hwcfg_ledrings.cathodes_ledrings_sr )
	MIOS32_DOUT_SRSet(lc_hwcfg_ledrings.cathodes_ledrings_sr-1, 0xff);
      if( lc_hwcfg_ledrings.cathodes_meters_sr )
	MIOS32_DOUT_SRSet(lc_hwcfg_ledrings.cathodes_meters_sr-1, select_mask);

      // set the meter pattern on the anodes
      anode_pattern = lc_flags.LEDRING_METERS ? ledring_pattern[sr_ctr] : meter_pattern[sr_ctr];
    }
  }

  if( lc_hwcfg_ledrings.anodes_sr1 )
    MIOS32_DOUT_SRSet(lc_hwcfg_ledrings.anodes_sr1-1, anode_pattern & 0xff);
  if( lc_hwcfg_ledrings.anodes_sr2 )
    MIOS32_DOUT_SRSet(lc_hwcfg_ledrings.anodes_sr2-1, (anode_pattern >> 8) & 0xff);

  return 0; // no error
}
