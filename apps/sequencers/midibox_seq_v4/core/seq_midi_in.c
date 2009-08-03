// $Id$
/*
 * MIDI IN Routines
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
#include "tasks.h"

#include "notestack.h"

#include "seq_ui.h"
#include "seq_midi_in.h"
#include "seq_cc.h"
#include "seq_morph.h"
#include "seq_core.h"


/////////////////////////////////////////////////////////////////////////////
// for optional debugging messages via DEBUG_MSG (defined in mios32_config.h)
/////////////////////////////////////////////////////////////////////////////
#define DEBUG_VERBOSE_LEVEL 0


/////////////////////////////////////////////////////////////////////////////
// Local definitions
/////////////////////////////////////////////////////////////////////////////

// the notestack size (can be overruled in mios32_config.h)
#ifndef SEQ_MIDI_IN_NOTESTACK_SIZE
#define SEQ_MIDI_IN_NOTESTACK_SIZE 10
#endif


// number of notestacks and assignments
#define NOTESTACK_NUM             3
#define NOTESTACK_TRANSPOSER      0
#define NOTESTACK_ARP_SORTED      1
#define NOTESTACK_ARP_UNSORTED    2


/////////////////////////////////////////////////////////////////////////////
// Local prototypes
/////////////////////////////////////////////////////////////////////////////

static s32 SEQ_MIDI_IN_Receive_Note(u8 note, u8 velocity);
static s32 SEQ_MIDI_IN_Receive_CC(u8 cc, u8 value);


/////////////////////////////////////////////////////////////////////////////
// Global variables
/////////////////////////////////////////////////////////////////////////////

// 0 disables MIDI In, 1..16 define the MIDI channel which should be used
u8 seq_midi_in_channel;

// which IN port should be used? (0: All)
mios32_midi_port_t seq_midi_in_port;

// Transposer/Arpeggiator split note
// (bit 7 enables/disables split)
u8 seq_midi_in_ta_split_note;


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

// we have three notestacks: one for transposer, two other for arpeggiator
static notestack_t notestack[NOTESTACK_NUM];
static notestack_item_t notestack_items[NOTESTACK_NUM][SEQ_MIDI_IN_NOTESTACK_SIZE];

// last note which has been played
static u8 transposer_hold_note;

// last arp notes which have been played
static notestack_item_t arp_sorted_hold[4];
static notestack_item_t arp_unsorted_hold[4];

/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_MIDI_IN_Init(u32 mode)
{
  SEQ_MIDI_IN_ResetNoteStacks();

  // stored in global config:
  seq_midi_in_channel = 1; // Channel #1 (0 disables MIDI IN)
  seq_midi_in_port = DEFAULT; // All ports
  seq_midi_in_ta_split_note = 0x3c; // C-3, bit #7 = 0 (split disabled!)

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// resets the note stacks
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_MIDI_IN_ResetNoteStacks(void)
{
  int i;

  NOTESTACK_Init(&notestack[NOTESTACK_TRANSPOSER],
		 NOTESTACK_MODE_PUSH_TOP,
		 &notestack_items[NOTESTACK_TRANSPOSER][0],
		 SEQ_MIDI_IN_NOTESTACK_SIZE);

  NOTESTACK_Init(&notestack[NOTESTACK_ARP_SORTED],
		 NOTESTACK_MODE_SORT,
		 &notestack_items[NOTESTACK_ARP_SORTED][0],
		 SEQ_MIDI_IN_NOTESTACK_SIZE);

  NOTESTACK_Init(&notestack[NOTESTACK_ARP_UNSORTED],
		 NOTESTACK_MODE_PUSH_TOP,
		 &notestack_items[NOTESTACK_ARP_UNSORTED][0],
		 SEQ_MIDI_IN_NOTESTACK_SIZE);

  // initial hold notes
  transposer_hold_note = 0x3c; // C-3

  for(i=0; i<4; ++i)
    arp_sorted_hold[0].ALL = arp_unsorted_hold[0].ALL = 0;

  arp_sorted_hold[0].note = arp_unsorted_hold[0].note = 0x3c; // C-3
  arp_sorted_hold[1].note = arp_unsorted_hold[1].note = 0x40; // E-3
  arp_sorted_hold[2].note = arp_unsorted_hold[2].note = 0x43; // G-3
  arp_sorted_hold[3].note = arp_unsorted_hold[3].note = 0x48; // C-4

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Receives a MIDI package from APP_NotifyReceivedEvent (-> app.c)
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_MIDI_IN_Receive(mios32_midi_port_t port, mios32_midi_package_t midi_package)
{
  s32 status = 0;

  // filter MIDI port (if 0: no filter, listen to all ports)
#if 0
  if( seq_midi_in_port && port != seq_midi_in_port )
#else
  // Loopback port not filtered!
  if( seq_midi_in_port && port != seq_midi_in_port && ((port & 0xf0) != 0xf0) )
#endif
    return status;

  // if not loopback and MIDI channel matching: forward to record function in record page
  if( ui_page == SEQ_UI_PAGE_TRKREC && !((port & 0xf0) == 0xf0) && midi_package.chn == (seq_midi_in_channel-1) )
    return SEQ_RECORD_Receive(midi_package, SEQ_UI_VisibleTrackGet());

  // Access to MIDI IN functions controlled by Mutex, since this function is access
  // by different tasks (APP_NotifyReceivedEvent() for received MIDI events, and 
  // SEQ_CORE_* for loopbacks)

  // Note Events: ignore channel if loopback
  if( ((port & 0xf0) == 0xf0) || midi_package.chn == (seq_midi_in_channel-1) ) {
    switch( midi_package.event ) {

      case NoteOff: 
	MUTEX_MIDIIN_TAKE;
	status = SEQ_MIDI_IN_Receive_Note(midi_package.note, 0x00);
	MUTEX_MIDIIN_GIVE;
	break;

      case NoteOn:
	MUTEX_MIDIIN_TAKE;
	status = SEQ_MIDI_IN_Receive_Note(midi_package.note, midi_package.velocity);
	MUTEX_MIDIIN_GIVE;
	break;

      case CC:
	MUTEX_MIDIIN_TAKE;
	status = SEQ_MIDI_IN_Receive_CC(midi_package.cc_number, midi_package.value);
	MUTEX_MIDIIN_GIVE;
	break;
    }
  }

  // TODO CC Events: channel won't be ignored on loopback

  return status;
}


/////////////////////////////////////////////////////////////////////////////
// If velocity == 0, Note Off event has been received, otherwise Note On event
/////////////////////////////////////////////////////////////////////////////
static s32 SEQ_MIDI_IN_Receive_Note(u8 note, u8 velocity)
{
  notestack_t *n;

  u8 ta_split_enabled = seq_midi_in_ta_split_note & 0x80;
  u8 ta_split_note = seq_midi_in_ta_split_note & 0x7f;


  ///////////////////////////////////////////////////////////////////////////
  // Transposer
  ///////////////////////////////////////////////////////////////////////////

  if( !ta_split_enabled || (note < ta_split_note) ) {
    n = &notestack[NOTESTACK_TRANSPOSER];
    if( velocity ) { // Note On
      NOTESTACK_Push(n, note, velocity);
      transposer_hold_note = n->note_items[0].note;

      // will only be used if enabled in OPT menu
      seq_core_keyb_scale_root = note % 12;
    } else { // Note Off
      if( NOTESTACK_Pop(n, note) > 0 && n->len ) {
	transposer_hold_note = n->note_items[0].note;
      }
    }
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("NOTESTACK_TRANSPOSER:\n");
    NOTESTACK_SendDebugMessage(&notestack[NOTESTACK_TRANSPOSER]);
#endif
  }


  ///////////////////////////////////////////////////////////////////////////
  // Arpeggiator
  ///////////////////////////////////////////////////////////////////////////

  if( !ta_split_enabled || (note >= ta_split_note) ) {
    if( velocity ) { // Note On
      // if no note in note stack anymore, reset position of all tracks with RESTART flag set
      if( !notestack[NOTESTACK_ARP_UNSORTED].len ) {
        u8 track;
        for(track=0; track<SEQ_CORE_NUM_TRACKS; ++track)
	  if( seq_cc_trk[track].mode.RESTART ) {
	    portENTER_CRITICAL();
	    seq_core_trk[track].state.POS_RESET = 1;
	    portEXIT_CRITICAL();
	  }

	// and invalidate hold stacks
	int i;
	for(i=0; i<4; ++i)
	  arp_sorted_hold[i].ALL = arp_unsorted_hold[i].ALL = 0;
      }

      // add to stacks
      NOTESTACK_Push(&notestack[NOTESTACK_ARP_SORTED], note, velocity);
      NOTESTACK_Push(&notestack[NOTESTACK_ARP_UNSORTED], note, velocity);

      // copy to hold stack
      int i;
      for(i=0; i<4; ++i) {
	arp_unsorted_hold[i].ALL = (i < notestack[NOTESTACK_ARP_UNSORTED].len) ? notestack[NOTESTACK_ARP_UNSORTED].note_items[i].ALL : 0;
	arp_sorted_hold[i].ALL = (i < notestack[NOTESTACK_ARP_SORTED].len) ? notestack[NOTESTACK_ARP_SORTED].note_items[i].ALL : 0;
      }

    } else { // Note Off
      // remove note from sorted/unsorted stack (not hold stacks)
      NOTESTACK_Pop(&notestack[NOTESTACK_ARP_SORTED], note);
      NOTESTACK_Pop(&notestack[NOTESTACK_ARP_UNSORTED], note);
    }

#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("NOTESTACK_ARP_SORTED:\n");
    NOTESTACK_SendDebugMessage(&notestack[NOTESTACK_ARP_SORTED]);
    DEBUG_MSG("NOTESTACK_ARP_UNSORTED:\n");
    NOTESTACK_SendDebugMessage(&notestack[NOTESTACK_ARP_UNSORTED]);
#endif
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// CC has been received over selected port and channel
/////////////////////////////////////////////////////////////////////////////
static s32 SEQ_MIDI_IN_Receive_CC(u8 cc, u8 value)
{
  switch( cc ) {
    case 0x01: // ModWheel -> Morph Value
      // update screen immediately if in morph page
      if( ui_page == SEQ_UI_PAGE_TRKMORPH )
	seq_ui_display_update_req = 1;
      // forward morph value
      return SEQ_MORPH_ValueSet(value);
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Returns the note for transpose mode
// if -1, the stack is empty
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_MIDI_IN_TransposerNoteGet(u8 hold)
{
  if( hold )
    return transposer_hold_note;

  return notestack[NOTESTACK_TRANSPOSER].len ? notestack[NOTESTACK_TRANSPOSER].note_items[0].note : -1;
}

/////////////////////////////////////////////////////////////////////////////
// Returns the note for arp mode, expected key_num range: 0..3
// if -1, the stack is empty
// if bit 7 is set, the last key of the stack has been played
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_MIDI_IN_ArpNoteGet(u8 hold, u8 sorted, u8 key_num)
{
  notestack_item_t *note_ptr;

  if( sorted )
    note_ptr = hold ? arp_sorted_hold : notestack[NOTESTACK_ARP_SORTED].note_items;
  else
    note_ptr = hold ? arp_unsorted_hold : notestack[NOTESTACK_ARP_UNSORTED].note_items;

  // arp note selection is based on following algorithm:
  // 1 note is played, return
  //    1: first note  (#0)
  //    2: first note  (#0)
  //    3: first note  (#0)
  //    4: first note  (#0)

  // 2 notes are played, return
  //    1: first note  (#0)
  //    2: second note (#1)
  //    3: first note  (#0)
  //    4: second note (#1)

  // 3 notes are played, return
  //    1: first note  (#0)
  //    2: second note (#1)
  //    3: third note  (#2)
  //    4: first note  (#0)

  // 4 notes are played, return
  //    1: first note  (#0)
  //    2: second note (#1)
  //    3: third note  (#2)
  //    4: fourth note (#3)

  // (stack size is identical for sorted/unsorted)
  u8 num_notes;
  if( hold ) {
    for(num_notes=0; num_notes<4; ++num_notes)
      if( !note_ptr[num_notes].note )
	break;
  } else
    num_notes = notestack[NOTESTACK_ARP_SORTED].len;

  // btw.: compare with lines of code in PIC based solution! :)
  return note_ptr[key_num % num_notes].note | ((!num_notes || key_num >= (num_notes-1)) ? 0x80 : 0);
}
