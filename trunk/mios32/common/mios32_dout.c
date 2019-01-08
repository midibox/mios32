// $Id$
//! \defgroup MIOS32_DOUT
//!
//! DOUT functions for MIOS32
//!
//! \{
/* ==========================================================================
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

// this module can be optionally disabled in a local mios32_config.h file (included from mios32.h)
#if !defined(MIOS32_DONT_USE_DOUT)


/////////////////////////////////////////////////////////////////////////////
//! temporary help array to mirror a byte
//! (could be provided as MIOS32 help function later)
/////////////////////////////////////////////////////////////////////////////

// DOUT bits are uploaded in reversed order compared to PIC based MIOS, and
// especially compared to the order on DIN registers
// Most applications would have to convert this - therefore it's already done
// by MIOS32_DOUT_* functions

// mirroring could work with:
// u8 mirror_u8(u8 b)
// { return  ((b&0x01)<<7) | ((b&0x02)<<5) | ((b&0x04)<<3) | ((b&0x08)<<1) | ((b&0x10)>>1) | ((b&0x20)>>3) | ((b&0x40)>>5) | ((b&0x80)>>7); }
// bit this would be a bit slow, e.g. for SR copy routines - therefore we use a table based approached:

// the table has been generated with:
// perl -e 'for($b=0; $b<256; ++$b) { printf("0x%02x,", (($b&0x01)<<7) | (($b&0x02)<<5) | (($b&0x04)<<3) | (($b&0x08)<<1) | (($b&0x10)>>1) | (($b&0x20)>>3) | (($b&0x40)>>5) | (($b&0x80)>>7)); }; printf("\n");'

const u8 mios32_dout_reverse_tab[256] = {
  0x00,0x80,0x40,0xc0,0x20,0xa0,0x60,0xe0,0x10,0x90,0x50,0xd0,0x30,0xb0,0x70,0xf0,
  0x08,0x88,0x48,0xc8,0x28,0xa8,0x68,0xe8,0x18,0x98,0x58,0xd8,0x38,0xb8,0x78,0xf8,
  0x04,0x84,0x44,0xc4,0x24,0xa4,0x64,0xe4,0x14,0x94,0x54,0xd4,0x34,0xb4,0x74,0xf4,
  0x0c,0x8c,0x4c,0xcc,0x2c,0xac,0x6c,0xec,0x1c,0x9c,0x5c,0xdc,0x3c,0xbc,0x7c,0xfc,
  0x02,0x82,0x42,0xc2,0x22,0xa2,0x62,0xe2,0x12,0x92,0x52,0xd2,0x32,0xb2,0x72,0xf2,
  0x0a,0x8a,0x4a,0xca,0x2a,0xaa,0x6a,0xea,0x1a,0x9a,0x5a,0xda,0x3a,0xba,0x7a,0xfa,
  0x06,0x86,0x46,0xc6,0x26,0xa6,0x66,0xe6,0x16,0x96,0x56,0xd6,0x36,0xb6,0x76,0xf6,
  0x0e,0x8e,0x4e,0xce,0x2e,0xae,0x6e,0xee,0x1e,0x9e,0x5e,0xde,0x3e,0xbe,0x7e,0xfe,
  0x01,0x81,0x41,0xc1,0x21,0xa1,0x61,0xe1,0x11,0x91,0x51,0xd1,0x31,0xb1,0x71,0xf1,
  0x09,0x89,0x49,0xc9,0x29,0xa9,0x69,0xe9,0x19,0x99,0x59,0xd9,0x39,0xb9,0x79,0xf9,
  0x05,0x85,0x45,0xc5,0x25,0xa5,0x65,0xe5,0x15,0x95,0x55,0xd5,0x35,0xb5,0x75,0xf5,
  0x0d,0x8d,0x4d,0xcd,0x2d,0xad,0x6d,0xed,0x1d,0x9d,0x5d,0xdd,0x3d,0xbd,0x7d,0xfd,
  0x03,0x83,0x43,0xc3,0x23,0xa3,0x63,0xe3,0x13,0x93,0x53,0xd3,0x33,0xb3,0x73,0xf3,
  0x0b,0x8b,0x4b,0xcb,0x2b,0xab,0x6b,0xeb,0x1b,0x9b,0x5b,0xdb,0x3b,0xbb,0x7b,0xfb,
  0x07,0x87,0x47,0xc7,0x27,0xa7,0x67,0xe7,0x17,0x97,0x57,0xd7,0x37,0xb7,0x77,0xf7,
  0x0f,0x8f,0x4f,0xcf,0x2f,0xaf,0x6f,0xef,0x1f,0x9f,0x5f,0xdf,0x3f,0xbf,0x7f,0xff
};


/////////////////////////////////////////////////////////////////////////////
//! Initializes DOUT driver
//! \param[in] mode currently only mode 0 supported
//! \return < 0 if initialisation failed
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_DOUT_Init(u32 mode)
{
  int i;

  // currently only mode 0 supported
  if( mode != 0 )
    return -1; // unsupported mode

  // clear DOUT part of SRIO chain
  // TODO: here we could provide an option to invert the default value
  for(i=0; i<MIOS32_SRIO_NUM_SR; ++i) {
#if MIOS32_SRIO_NUM_DOUT_PAGES < 2
    mios32_srio_dout[0][i] = 0;
#else
    int j;
    for(j=0; j<MIOS32_SRIO_NUM_DOUT_PAGES; ++j)
      mios32_srio_dout[j][i] = 0;
#endif
  }

  return 0;
}


/////////////////////////////////////////////////////////////////////////////
//! Returns value from a DOUT Pin (always page 0)
//! \param[in] pin number (0..127)
//! \return 1 if pin is Vss (ie. 5V)
//! \return 0 if pin is 0V
//! \return -1 if pin not available
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_DOUT_PinGet(u32 pin)
{
  u8 num_sr = MIOS32_SRIO_ScanNumGet();

  // check if pin available
  if( (pin/8) >= num_sr )
    return -1;

  // NOTE: DOUT SR registers in reversed (!) order (since DMA doesn't provide a decrement address function)
  return (mios32_srio_dout[0][num_sr - (pin>>3) - 1] & (1 << ((pin&7)^7))) ? 1 : 0;
}

/////////////////////////////////////////////////////////////////////////////
//! Sets value of a DOUT Pin\n
//! With MIOS32_SRIO_NUM_DOUT_PAGES > 1, the value will be set in all pages at once!
//! \param[in] pin number (0..127)
//! \param[in] value of the pin (0 or 1)
//! \return -1 if pin not available
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_DOUT_PinSet(u32 pin, u32 value)
{
  u8 num_sr = MIOS32_SRIO_ScanNumGet();

  // check if pin available
  if( (pin/8) >= num_sr )
    return -1;

  int i;
  u8 *dout = (u8 *)&mios32_srio_dout[0][num_sr - (pin>>3) - 1];
  u8 mask = (1 << ((pin&7)^7));

  MIOS32_IRQ_Disable(); // this should be atomic

  if( value ) {
    for(i=0; i<MIOS32_SRIO_NUM_DOUT_PAGES; ++i, dout += MIOS32_SRIO_NUM_SR)
      *dout |= mask;
  } else {
    for(i=0; i<MIOS32_SRIO_NUM_DOUT_PAGES; ++i, dout += MIOS32_SRIO_NUM_SR)
      *dout &= ~mask;
  }

  MIOS32_IRQ_Enable();

  return 0;
}

/////////////////////////////////////////////////////////////////////////////
//! Returns value from a DOUT Pin of given page
//! \param[in] page number (0..MIOS32_SRIO_NUM_DOUT_PAGES-1)
//! \param[in] pin number (0..127)
//! \return 1 if pin is Vss (ie. 5V)
//! \return 0 if pin is 0V
//! \return -1 if pin not available
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_DOUT_PagePinGet(u8 page, u32 pin)
{
  if( page >= MIOS32_SRIO_NUM_DOUT_PAGES )
    return -2; // invalid page

  u8 num_sr = MIOS32_SRIO_ScanNumGet();

  // check if pin available
  if( (pin/8) >= num_sr )
    return -1;

  // NOTE: DOUT SR registers in reversed (!) order (since DMA doesn't provide a decrement address function)
  return (mios32_srio_dout[page][num_sr - (pin>>3) - 1] & (1 << ((pin&7)^7))) ? 1 : 0;
}

/////////////////////////////////////////////////////////////////////////////
//! Sets value of a DOUT Pin in given page
//! \param[in] page number (0..MIOS32_SRIO_NUM_DOUT_PAGES-1)
//! \param[in] pin number (0..127)
//! \param[in] value of the pin (0 or 1)
//! \return -1 if pin not available
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_DOUT_PagePinSet(u8 page, u32 pin, u32 value)
{
  if( page >= MIOS32_SRIO_NUM_DOUT_PAGES )
    return -2; // invalid page

  u8 num_sr = MIOS32_SRIO_ScanNumGet();

  // check if pin available
  if( (pin/8) >= num_sr )
    return -1;

  u8 *dout = (u8 *)&mios32_srio_dout[page][num_sr - (pin>>3) - 1];
  u8 mask = (1 << ((pin&7)^7));

  MIOS32_IRQ_Disable(); // this should be atomic

  if( value )
    *dout |= mask;
  else
    *dout &= ~mask;

  MIOS32_IRQ_Enable();

  return 0;
}

/////////////////////////////////////////////////////////////////////////////
//! Returns 8bit value of a DOUT shift register (always from page 0)
//! \param[in] sr shift register number (0..15)
//! \return 8bit value of shift register
//! \return -1 if shift register not available
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_DOUT_SRGet(u32 sr)
{
  u8 num_sr = MIOS32_SRIO_ScanNumGet();

  // check if SR available
  if( sr >= num_sr )
    return -1;

  return mios32_dout_reverse_tab[mios32_srio_dout[0][num_sr - sr - 1]];
}

/////////////////////////////////////////////////////////////////////////////
//! Sets 8bit value of a DOUT shift register\n
//! With MIOS32_SRIO_NUM_DOUT_PAGES > 1, the value will be set in all pages at once!
//! \param[in] sr shift register number (0..15)
//! \param[in] value 8bit value of shift register
//! \return -1 if shift register not available
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_DOUT_SRSet(u32 sr, u8 value)
{
  u8 num_sr = MIOS32_SRIO_ScanNumGet();

  // check if SR available
  if( sr >= num_sr )
    return -1;

  u8 *dout = (u8 *)&mios32_srio_dout[0][num_sr - sr - 1];
  u8 mapped_value = mios32_dout_reverse_tab[value];

#if MIOS32_SRIO_NUM_DOUT_PAGES < 2
  *dout = mapped_value;
#else
  int i;
  for(i=0; i<MIOS32_SRIO_NUM_DOUT_PAGES; ++i, dout += MIOS32_SRIO_NUM_SR)
    *dout = mapped_value;
#endif

  return 0;
}


/////////////////////////////////////////////////////////////////////////////
//! Returns 8bit value of a DOUT shift register of the given page
//! \param[in] sr shift register number (0..15)
//! \return 8bit value of shift register
//! \return -1 if shift register not available
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_DOUT_PageSRGet(u8 page, u32 sr)
{
  if( page >= MIOS32_SRIO_NUM_DOUT_PAGES )
    return -2; // invalid page

  u8 num_sr = MIOS32_SRIO_ScanNumGet();

  // check if SR available
  if( sr >= num_sr )
    return -1;

  return mios32_dout_reverse_tab[mios32_srio_dout[page][num_sr - sr - 1]];
}

/////////////////////////////////////////////////////////////////////////////
//! Sets 8bit value of a DOUT shift register in the given page.
//! \param[in] sr shift register number (0..15)
//! \param[in] value 8bit value of shift register
//! \return -1 if shift register not available
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_DOUT_PageSRSet(u8 page, u32 sr, u8 value)
{
  if( page >= MIOS32_SRIO_NUM_DOUT_PAGES )
    return -2; // invalid page

  u8 num_sr = MIOS32_SRIO_ScanNumGet();

  // check if SR available
  if( sr >= num_sr )
    return -1;

  u8 *dout = (u8 *)&mios32_srio_dout[page][num_sr - sr - 1];
  u8 mapped_value = mios32_dout_reverse_tab[value];

  *dout = mapped_value;

  return 0;
}

//! \}

#endif /* MIOS32_DONT_USE_DOUT */
