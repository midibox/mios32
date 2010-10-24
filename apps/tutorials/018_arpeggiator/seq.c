// $Id$
/*
 * Sequencer Routines
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
#include <notestack.h>

#include "seq.h"


/////////////////////////////////////////////////////////////////////////////
// Local definitions
/////////////////////////////////////////////////////////////////////////////

#define NOTESTACK_SIZE 16


/////////////////////////////////////////////////////////////////////////////
// Local prototypes
/////////////////////////////////////////////////////////////////////////////

static s32 SEQ_PlayOffEvents(void);
static s32 SEQ_Tick(u32 bpm_tick);


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

// the arpeggiator position
static u8 arp_counter;

// pause mode (will be controlled from user interface)
static u8 seq_pause = 0;

// notestack
static notestack_t notestack;
static notestack_item_t notestack_items[NOTESTACK_SIZE];


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_Init(u32 mode)
{
  // initialize the Notestack
#if 0
  NOTESTACK_Init(&notestack, NOTESTACK_MODE_PUSH_TOP, &notestack_items[0], NOTESTACK_SIZE);
#else
  // for an arpeggiator we prefer sorted mode
  // activate hold mode as well. Stack will be cleared whenever no note is played anymore
  NOTESTACK_Init(&notestack, NOTESTACK_MODE_SORT_HOLD, &notestack_items[0], NOTESTACK_SIZE);
#endif

  // and the arp counter
  arp_counter = 0;

  // reset sequencer
  SEQ_Reset();

  // init BPM generator
  SEQ_BPM_Init(0);

  SEQ_BPM_PPQN_Set(384);
  SEQ_BPM_Set(120.0);

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// this sequencer handler is called periodically to check for new requests
// from BPM generator
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_Handler(void)
{
  // handle requests

  u8 num_loops = 0;
  u8 again = 0;
  do {
    ++num_loops;

    // note: don't remove any request check - clocks won't be propagated
    // so long any Stop/Cont/Start/SongPos event hasn't been flagged to the sequencer
    if( SEQ_BPM_ChkReqStop() ) {
      SEQ_PlayOffEvents();
    }

    if( SEQ_BPM_ChkReqCont() ) {
      // release pause mode
      seq_pause = 0;
    }

    if( SEQ_BPM_ChkReqStart() ) {
      SEQ_Reset();
    }

    u16 new_song_pos;
    if( SEQ_BPM_ChkReqSongPos(&new_song_pos) ) {
      SEQ_PlayOffEvents();
    }

    u32 bpm_tick;
    if( SEQ_BPM_ChkReqClk(&bpm_tick) > 0 ) {
      again = 1; // check all requests again after execution of this part

      SEQ_Tick(bpm_tick);
    }
  } while( again && num_loops < 10 );

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// This function plays all "off" events
// Should be called on sequencer reset/restart/pause to avoid hanging notes
/////////////////////////////////////////////////////////////////////////////
static s32 SEQ_PlayOffEvents(void)
{
  // play "off events"
  SEQ_MIDI_OUT_FlushQueue();

  // here you could send additional events, e.g. "All Notes Off" CC

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Resets song position of sequencer
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_Reset(void)
{
  // since timebase has been changed, ensure that Off-Events are played 
  // (otherwise they will be played much later...)
  SEQ_PlayOffEvents();

  // release pause mode
  seq_pause = 0;

  // reset BPM tick
  SEQ_BPM_TickSet(0);

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// performs a single bpm tick
/////////////////////////////////////////////////////////////////////////////
static s32 SEQ_Tick(u32 bpm_tick)
{
  // whenever we reach a new 16th note (96 ticks @384 ppqn):
  if( (bpm_tick % (SEQ_BPM_PPQN_Get()/4)) == 0 ) {
    // ensure that arp is reseted on first bpm_tick
    if( bpm_tick == 0 )
      arp_counter = 0;
    else {
      // increment arpeggiator counter
      ++arp_counter;

      // reset once we reached length of notestack
      if( arp_counter >= notestack.len )
	arp_counter = 0;
    }

    if( notestack.len > 0 ) {
      // get note/velocity/length from notestack
      u8 note      = notestack_items[arp_counter].note;
      u8 velocity  = notestack_items[arp_counter].tag;
      u8 length    = 72; // always the same, could be varied, e.g. via CC

      // put note into queue if all values are != 0
      if( note && velocity && length ) {
	mios32_midi_package_t midi_package;
	midi_package.type     = NoteOn; // package type must match with event!
	midi_package.event    = NoteOn;
	midi_package.chn      = Chn1;
	midi_package.note     = note;
	midi_package.velocity = velocity;
	
	SEQ_MIDI_OUT_Send(DEFAULT, midi_package, SEQ_MIDI_OUT_OnOffEvent, bpm_tick, length);
      }
    }
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Should be called whenever a Note event has been received.
// We expect, that velocity is 0 on a Note Off event
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_NotifyNoteOn(u8 note, u8 velocity)
{
  u8 clear_stack = 0;

  if( velocity )
    // push note into note stack
    NOTESTACK_Push(&notestack, note, velocity);
  else {
    // remove note from note stack
    // function returns 2 if no note played anymore (all keys depressed)
    if( NOTESTACK_Pop(&notestack, note) == 2 )
      clear_stack = 1;
  }

  // At least one note played?
  if( !clear_stack && notestack.len > 0 ) {
    // start sequencer if it isn't already running
    if( !SEQ_BPM_IsRunning() )
      SEQ_BPM_Start();
  } else {
    // clear stack
    NOTESTACK_Clear(&notestack);

    // no key is pressed anymore: stop sequencer
    SEQ_BPM_Stop();
  }


#if 1
  // optional debug messages
  NOTESTACK_SendDebugMessage(&notestack);
#endif

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Should be called whenever a Note Off event has been received.
// forwards to SEQ_NotifyNoteOn with velocity 0
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_NotifyNoteOff(u8 note)
{
  return SEQ_NotifyNoteOn(note, 0);
}
