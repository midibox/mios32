// $Id: benchmark.c 534 2009-05-18 21:08:36Z tk $
/*
 * Benchmark for MIDI Parser
 * See README.txt for details
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
#include "benchmark.h"


// for LPC17: simplify allocation of large arrays
#if defined(MIOS32_FAMILY_LPC17xx)
# define AHB_SECTION __attribute__ ((section (".bss_ahb")))
#else
# define AHB_SECTION
#endif


/////////////////////////////////////////////////////////////////////////////
// Local Variables
/////////////////////////////////////////////////////////////////////////////

#define NUM_ENTRIES  256
#define STORAGE_SIZE (NUM_ENTRIES*10) 
static u8 search_storage[STORAGE_SIZE];

static u8 search_storage_ahb[STORAGE_SIZE] AHB_SECTION;

static u8 search_string[10];


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 BENCHMARK_Init(u32 mode)
{
  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
s32 BENCHMARK_Reset_LinearRAM(u32 par)
{
  int i;

  u8 *storage = par ? (u8 *)&search_storage_ahb[0] : (u8 *)&search_storage[0];

  for(i=0; i<NUM_ENTRIES; ++i) {
    u8 *entry = (u8 *)&storage[i*10];
    entry[0] = 0xf0;
    entry[1] = 0x01;
    entry[2] = 0x02;
    entry[3] = 0x03;
    entry[4] = 0x04;
    entry[5] = 0x05;
    entry[6] = 0x06;
    entry[7] = i & 0x7f;
    entry[8] = i >> 7;
    entry[9] = 0xff;
  }

  int n = NUM_ENTRIES-1;
  search_string[0] = 0xf0;
  search_string[1] = 0x01;
  search_string[2] = 0x02;
  search_string[3] = 0x03;
  search_string[4] = 0x04;
  search_string[5] = 0x05;
  search_string[6] = 0x06;
  search_string[7] = n & 0x7f;
  search_string[8] = n >> 7;
  search_string[9] = 0xff;

  return 0; // no error
}

s32 BENCHMARK_Start_LinearRAM(u32 par)
{
  int i;

  u8 *storage = par ? (u8 *)&search_storage_ahb[0] : (u8 *)&search_storage[0];

  for(i=0; i<NUM_ENTRIES; ++i) {
    u8 *s1 = (u8 *)&storage[i*10];
    u8 *s2 = (u8 *)&search_string[0];

    while( *s1 == *s2 && *s1 != 0xff && *s2 != 0xff ) {
      ++s1;
      ++s2;
    }

    if( *s1 == *s2 ) {
#if 0
      MIOS32_MIDI_SendDebugMessage("Found %d\n", i);
#endif
      return 0; // found (no error)
    }
  }

  return -1; // search string not found (not intended)
}


/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
s32 BENCHMARK_Reset_LinearRAM_KnownLen(u32 par)
{
  int i;

  u8 *storage = par ? (u8 *)&search_storage_ahb[0] : (u8 *)&search_storage[0];

  for(i=0; i<NUM_ENTRIES; ++i) {
    u8 *entry = (u8 *)&storage[i*10];
    entry[0] = 9;
    entry[1] = 0xf0;
    entry[2] = 0x01;
    entry[3] = 0x02;
    entry[4] = 0x03;
    entry[5] = 0x04;
    entry[6] = 0x05;
    entry[7] = 0x06;
    entry[8] = i & 0x7f;
    entry[9] = i >> 7;
  }

  int n = NUM_ENTRIES-1;
  search_string[0] = 9;
  search_string[1] = 0xf0;
  search_string[2] = 0x01;
  search_string[3] = 0x02;
  search_string[4] = 0x03;
  search_string[5] = 0x04;
  search_string[6] = 0x05;
  search_string[7] = 0x06;
  search_string[8] = n & 0x7f;
  search_string[9] = n >> 7;

  return 0; // no error
}

s32 BENCHMARK_Start_LinearRAM_KnownLen(u32 par)
{
  int i;

  u8 *storage = par ? (u8 *)&search_storage_ahb[0] : (u8 *)&search_storage[0];

  u8 *s1 = (u8 *)&storage[0];
  u8 *s1_end = (u8 *)(s1 + STORAGE_SIZE - 1);
  u8 len_s2 = search_string[0];

  for(i=0; i<NUM_ENTRIES && s1 < s1_end; ++i) {
    u8 len_s1 = *s1++;

    if( len_s1 != len_s2 ) {
      s1 += len_s1;
    } else {
      int j;
      u8 *s2 = (u8 *)&search_string[1];
      for(j=0; j<len_s2 && *s1++ == *s2++; ++j);

      if( j == len_s2 ) {
#if 0
	MIOS32_MIDI_SendDebugMessage("Found %d\n", i);
#endif
	return 0; // found (no error)
      } else {
	s1 += len_s1 - j - 1;
      }
    }
  }

  return -1; // search string not found (not intended)
}
