// $Id$
/*
 * Benchmark for MIDI Out Transfers
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


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 BENCHMARK_Init(u32 mode)
{
  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// this function resets the benchmark
/////////////////////////////////////////////////////////////////////////////
s32 BENCHMARK_Reset(void)
{
  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// this function performs the benchmark
/////////////////////////////////////////////////////////////////////////////
s32 BENCHMARK_Start(mios32_midi_port_t port)
{
  int i;

  for(i=0; i<128; ++i)
    MIOS32_MIDI_SendNoteOn(port, Chn16, i, 0x7f);

  for(i=0; i<128; ++i)
    MIOS32_MIDI_SendNoteOn(port, Chn16, i, 0x00);

  // if UART: wait until all bytes transmitted
  if( (port & 0xf0) == UART0 )
    while( MIOS32_UART_TxBufferUsed(port&0xf) );

  return 0; // no error
}
