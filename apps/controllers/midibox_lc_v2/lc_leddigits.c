// $Id$
/*
 * MIDIbox LC V2
 * LED Digits Handler
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
#include "lc_leddigits.h"
#include "lc_lcd.h"

/////////////////////////////////////////////////////////////////////////////
// Global Variables
/////////////////////////////////////////////////////////////////////////////

// this is a global value (for fastest access)
// the purpose of the 16 digits is defined in lc_leddigits.h
u8 lc_leddigits_mtc[10];
u8 lc_leddigits_status[2];


/////////////////////////////////////////////////////////////////////////////
// Local Variables
/////////////////////////////////////////////////////////////////////////////

// the LED digit patterns
static const u8 digit_patterns[64] = {
  //    a
  //   ---
  //  !   !
  // f! g !b
  //   ---
  //  !   !
  // e!   !c
  //   ---
  //    d   h
  // 0 = on, 1 = off
  // NOTE: the dod (h) will be set automatically by the driver above when bit 6 is 1
              //    habcdefg     habcdefg
  0xff, 0x88, //  b'11111111', b'10001000'
  0x70, 0xb1, //  b'11100000', b'10110001'
  0xc2, 0xb0, //  b'11000010', b'10110000'
  0xb8, 0xa1, //  b'10111000', b'10100001'
  0xe8, 0xcf, //  b'11101000', b'11001111'
  0xc3, 0xf8, //  b'11000011', b'11111000'
  0xf1, 0x89, //  b'11110001', b'10001001'
  0xea, 0xe2, //  b'11101010', b'11100010'

  0x98, 0x8c, //  b'10011000', b'10001100'
  0xfa, 0x92, //  b'11111010', b'10010010'
  0xf0, 0xe3, //  b'11110000', b'11100011'
  0xd8, 0xc1, //  b'11011000', b'11000001'
  0xc8, 0xc4, //  b'11001000', b'11000100'
  0x92, 0xb1, //  b'10010010', b'10110001'
  0xec, 0x87, //  b'11101100', b'10000111'
  0x9d, 0xf7, //  b'10011101', b'11110111'

  0xff, 0xff, //  b'11111111', b'11111111'
  0xdd, 0x9c, //  b'11011101', b'10011100'
  0xa4, 0x98, //  b'10100100', b'10011000'
  0x82, 0xfd, //  b'10000010', b'11111101'
  0xb1, 0x87, //  b'10110001', b'10000111'
  0x9c, 0xce, //  b'10011100', b'11001110'
  0xfb, 0xfe, //  b'11111011', b'11111110'
  0xf7, 0xba, //  b'11110111', b'11011010'

  0x81, 0xcf, //  b'10000001', b'11001111'
  0x92, 0x86, //  b'10010010', b'10000110'
  0xcc, 0xa4, //  b'11001100', b'10100100'
  0xa0, 0x8f, //  b'10100000', b'10001111'
  0x80, 0x84, //  b'10000000', b'10000100'
  0xff, 0xbb, //  b'11111111', b'10111011'
  0xce, 0xf6, //  b'11001110', b'11110110'
  0xf8, 0x9a, //  b'11111000', b'10011010'
};

/////////////////////////////////////////////////////////////////////////////
// This function initializes the LED Digits
/////////////////////////////////////////////////////////////////////////////
s32 LC_LEDDIGITS_Init(u32 mode)
{
  int i;

  for(i=0; i<sizeof(lc_leddigits_mtc); ++i)
    lc_leddigits_mtc[i] = '0';

  for(i=0; i<sizeof(lc_leddigits_status); ++i)
    lc_leddigits_status[i] = '_';

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// This function updates the MTC LED digits
/////////////////////////////////////////////////////////////////////////////
s32 LC_LEDDIGITS_MTCSet(u8 number, u8 pattern)
{
  // store digit
  lc_leddigits_mtc[number] = pattern;

  // print MTC digit
  LC_LCD_Update_MTC();

  //  u8 digit_pattern = digit_patterns[pattern&0x3f];

  //  if( pattern & (1 << 7) ) // set dot depending on bit 7 of pattern
  //    digit_pattern &= 0x7f;

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// This function updates the Status LED digits
/////////////////////////////////////////////////////////////////////////////
s32 LC_LEDDIGITS_StatusSet(u8 number, u8 pattern)
{
  // store digit
  lc_leddigits_status[number] = pattern;

  // print Status digit
  return LC_LCD_Update_Status();
}
