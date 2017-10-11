// $Id$
/*
 * LED Handlers
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

#include "seq_led.h"
#include "seq_blm8x8.h"
#include "seq_hwcfg.h"
#include "seq_file_hw.h"

#ifdef MBSEQV4L
#include <blm_cheapo.h>
#endif


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_LED_Init(u32 mode)
{
  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Sets a LED, differs between LEDs directly connected to DOUT pins 
// (pin = 0..191) and LEDS which are connected to a 8x8 matrix (pin = 192..255)
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_LED_PinSet(u32 pin, u32 value)
{
#ifdef MBSEQV4L
  if( pin < 64 ) {
    BLM_CHEAPO_DOUT_PinSet(pin, value);

    // extra: directly mirror to SRIO DOUT as long as SD Card not read (dirty workaround to simplify usage of V4L)
    if( SEQ_FILE_HW_Valid() ) {
      return 0;
    } else {
      pin += 184;
    }
  }
#else
  if( pin < 184 )
    return MIOS32_DOUT_PinSet(pin, value);
#endif

  if( pin >= 184 && pin < 248 ) {
    if( seq_hwcfg_blm8x8.dout_gp_mapping == 2 && !SEQ_FILE_HW_Valid() ) {
      // MBSEQ V4L SRIO Board
      if( pin >= 216 ) {
	pin = (pin & 0xf8) | (7 - (pin&7));
      }
    }

    return SEQ_BLM8X8_LEDSet(0, pin-184, value);
  }

  return -1; // pin not available
}


/////////////////////////////////////////////////////////////////////////////
// Sets a DOUT SR (8 LEDs), differs between LEDs directly connected to DOUT SR
// (sr=0..23) and LEDS which are connected to a 8x8 matrix (sr=24..31)
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_LED_SRSet(u32 sr, u8 value)
{
  if( sr >= 128 )
    return -1; // SR disabled

#ifdef MBSEQV4L
  if( sr < 8 ) {
    BLM_CHEAPO_DOUT_SRSet(sr, value);

    // extra: directly mirror to SRIO DOUT as long as SD Card not read (dirty workaround to simplify usage of V4L)
    if( SEQ_FILE_HW_Valid() ) {
      return 0;
    } else {
      sr += 23;
    }
  }

#else
  if( sr < 23 )
    return MIOS32_DOUT_SRSet(sr, value);
#endif

  if( sr >= 23 && sr < 31 ) {
    if( seq_hwcfg_blm8x8.dout_gp_mapping == 2 && sr >= 27 ) {
      // MBSEQ V4L SRIO Board
      value = mios32_dout_reverse_tab[value];
    }

    return SEQ_BLM8X8_LEDSRSet(0, sr-23, value);
  }

  return -1; // SR not available
}

/////////////////////////////////////////////////////////////////////////////
// returns the value of a DOUT SR (8 LEDs), differs between LEDs directly connected to DOUT SR
// (sr=0..23) and LEDS which are connected to a 8x8 matrix (sr=24..31)
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_LED_SRGet(u32 sr)
{
  if( sr >= 128 )
    return 0; // SR disabled

#ifdef MBSEQV4L
  if( sr < 8 )
    return BLM_CHEAPO_DOUT_SRGet(sr);
#else
  if( sr < 23 )
    return MIOS32_DOUT_SRGet(sr);
#endif

  if( sr >= 23 && sr < 31 ) {
    u8 value = SEQ_BLM8X8_LEDSRGet(0, sr-23);

    if( seq_hwcfg_blm8x8.dout_gp_mapping == 2 && sr >= 27 ) {
      // MBSEQ V4L SRIO Board
      value = mios32_dout_reverse_tab[value];
    }

    return value;
  }

  return 0; // SR not available... return 0
}


/////////////////////////////////////////////////////////////////////////////
// Returns the 8bit pattern of a hexadecimal 4bit value
// The dot will be set if bit 7 of this value is set
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_LED_DigitPatternGet(u8 value)
{
  const u8 led_digits_decoded[16] = {
    //    a
    //   ---
    //  !   !
    // f! g !b
    //   ---
    //  !   !
    // e!   !c
    //   ---
    //    d   h
    // 1 = on, 0 = off
    // NOTE: the dod (h) will be set automatically by the driver above when bit 7 is set

    //       habc defg
    0x7e, // 0111 1110
    0x30, // 0011 0000
    0x6d, // 0110 1101
    0x79, // 0111 1001
    0x33, // 0011 0011
    0x5b, // 0101 1011
    0x5f, // 0101 1111
    0x70, // 0111 0000
    0x7f, // 0111 1111
    0x7b, // 0111 1011
    0x77, // 0111 0111
    0x1f, // 0001 1111
    0x4e, // 0100 1110
    0x3d, // 0011 1101
    0x4f, // 0100 1111
    0x47, // 0100 0111
  };  


  return led_digits_decoded[value & 0xf] | (value & 0x80);
}
