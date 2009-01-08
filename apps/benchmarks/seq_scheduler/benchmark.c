// $Id$
/*
 * Benchmark for MIDI Out Scheduler
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
#include <seq_bpm.h>
#include <seq_midi_out.h>

#include <mid_parser.h>

#include "benchmark.h"
#include "mid_file.h"


/////////////////////////////////////////////////////////////////////////////
// Local prototypes
/////////////////////////////////////////////////////////////////////////////

static s32 BENCHMARK_PlayEvent(u8 track, mios32_midi_package_t midi_package, u32 tick);
static s32 BENCHMARK_PlayMeta(u8 track, u8 meta, u32 len, u8 *buffer, u32 tick);


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 BENCHMARK_Init(u32 mode)
{
  // init MIDI file handler
  MID_FILE_Init(0);

  // init MIDI parser module
  MID_PARSER_Init(0);

  // initialize MIDI handler
  SEQ_MIDI_OUT_Init(0);

  // install callback functions
  MID_PARSER_InstallFileCallbacks(&MID_FILE_read, &MID_FILE_eof, &MID_FILE_seek);
  MID_PARSER_InstallEventCallbacks(&BENCHMARK_PlayEvent, &BENCHMARK_PlayMeta);

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// this function resets the benchmark
/////////////////////////////////////////////////////////////////////////////
s32 BENCHMARK_Reset(void)
{
  // get control over BPM generator with following sequence:
  // enter master mode
  SEQ_BPM_ModeSet(SEQ_BPM_MODE_Master);
  // start clock
  SEQ_BPM_Start();
  // enter slave mode
  SEQ_BPM_ModeSet(SEQ_BPM_MODE_Slave);
  // the generator won't generate ticks anymore, as it doesn't receive clock events!
  // now bpm_tick is under direct control!
  SEQ_BPM_TickSet(0);

  // play "off" events for the case that the queue wasn't empty
  SEQ_MIDI_OUT_FlushQueue();

  // read the MIDI file
  MID_FILE_open("dummy");
  MID_PARSER_Read();

  // clear MIDI scheduler analysis variables
  seq_midi_out_allocated = 0;
  seq_midi_out_max_allocated = 0;
  seq_midi_out_dropouts = 0;

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// this function performs the benchmark
/////////////////////////////////////////////////////////////////////////////
s32 BENCHMARK_Start(void)
{
  u32 bpm_tick = 0;
#if 1
  // step through song until last position reached
  // wait additional BPM ticks to ensure that all events have been played
  while( MID_PARSER_FetchEvents(bpm_tick, 1) > 0 || seq_midi_out_allocated ) {
    // increment tick
    ++bpm_tick;

    // forward to BPM handler
    SEQ_BPM_TickSet(bpm_tick);

    // send timestamped MIDI events immediately
    SEQ_MIDI_OUT_Handler();
  }
#else
  int i;
  mios32_midi_package_t midi_package;
  for(i=0; i<1000 || seq_midi_out_allocated; ++i) {
    SEQ_MIDI_OUT_Send(0xff, midi_package, NoteOn, bpm_tick);

    // increment tick
    ++bpm_tick;

    // forward to BPM handler
    SEQ_BPM_TickSet(bpm_tick);

    // send timestamped MIDI events immediately
    SEQ_MIDI_OUT_Handler();
  };
#endif

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// called when a MIDI event should be played at a given tick
/////////////////////////////////////////////////////////////////////////////
static s32 BENCHMARK_PlayEvent(u8 track, mios32_midi_package_t midi_package, u32 tick)
{
  seq_midi_out_event_type_t event_type = SEQ_MIDI_OUT_OnEvent;
  if( midi_package.event == NoteOff || (midi_package.event == NoteOn && midi_package.velocity == 0) )
    event_type = SEQ_MIDI_OUT_OffEvent;

  // output events to a dummy port (so that the interface doesn't falsify the benchmark results)
#if 1
  SEQ_MIDI_OUT_Send(0xff, midi_package, event_type, tick);
#else
  SEQ_MIDI_OUT_Send(USB0, midi_package, event_type, tick);
#endif

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// called when a Meta event should be played/processed at a given tick
/////////////////////////////////////////////////////////////////////////////
static s32 BENCHMARK_PlayMeta(u8 track, u8 meta, u32 len, u8 *buffer, u32 tick)
{
  return 0; // no error
}
