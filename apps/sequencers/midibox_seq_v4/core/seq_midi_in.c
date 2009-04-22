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

#include "seq_ui.h"
#include "seq_midi_in.h"
#include "seq_cc.h"
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
// Local types
/////////////////////////////////////////////////////////////////////////////

// defines a notestack
typedef struct {
  u8                    len;
  u8                    note[SEQ_MIDI_IN_NOTESTACK_SIZE];
} seq_midi_in_notestack_t;


/////////////////////////////////////////////////////////////////////////////
// Local prototypes
/////////////////////////////////////////////////////////////////////////////

static s32 SEQ_MIDI_IN_Receive_Note(u8 note, u8 velocity);

static s32 SEQ_MIDI_IN_Notestack_Pop(seq_midi_in_notestack_t *n, u8 old_note);
static s32 SEQ_MIDI_IN_Notestack_Push(seq_midi_in_notestack_t *n, u8 new_note, u8 sort);


/////////////////////////////////////////////////////////////////////////////
// Global variables
/////////////////////////////////////////////////////////////////////////////

// 0 disables MIDI In, 1..16 define the MIDI channel which should be used
u8 seq_midi_in_channel;

// which IN port should be used? (0: All)
mios32_midi_port_t seq_midi_in_port;

// which port is used for MIDI clock input? (0: All)
mios32_midi_port_t seq_midi_in_mclk_port;

// Transposer/Arpeggiator split note
// (bit 7 enables/disables split)
u8 seq_midi_in_ta_split_note;


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

// we have two notestacks: one for transposer, another for arpeggiator
static seq_midi_in_notestack_t notestack[NOTESTACK_NUM];

// last note which has been played
static u8 transposer_hold_note;

// last arp notes which have been played
static u8 arp_sorted_hold[4];
static u8 arp_unsorted_hold[4];

/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_MIDI_IN_Init(u32 mode)
{
  int i;

  for(i=0; i<NOTESTACK_NUM; ++i)
    notestack[i].len = 0;

  // initial hold notes
  transposer_hold_note = 0x3c; // C-3

  arp_sorted_hold[0] = arp_unsorted_hold[0] = 0x3c; // C-3
  arp_sorted_hold[1] = arp_unsorted_hold[1] = 0x40; // E-3
  arp_sorted_hold[2] = arp_unsorted_hold[2] = 0x43; // G-3
  arp_sorted_hold[3] = arp_unsorted_hold[3] = 0x48; // C-4

  // stored in global config:
  seq_midi_in_channel = 1; // Channel #1 (0 disables MIDI IN)
  seq_midi_in_port = DEFAULT; // All ports
  seq_midi_in_mclk_port = DEFAULT; // All ports
  seq_midi_in_ta_split_note = 0x3c; // C-3, bit #7 = 0 (split disabled!)

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Receives a MIDI package from APP_NotifyReceivedEvent (-> app.c)
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_MIDI_IN_Receive(mios32_midi_port_t port, mios32_midi_package_t midi_package)
{
  s32 status = 0;

  // filter MIDI port (if 0: no filter, listen to all ports)
  if( seq_midi_in_port && port != seq_midi_in_port )
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
  seq_midi_in_notestack_t *n;

  u8 ta_split_enabled = seq_midi_in_ta_split_note & 0x80;
  u8 ta_split_note = seq_midi_in_ta_split_note & 0x7f;


  ///////////////////////////////////////////////////////////////////////////
  // Transposer
  ///////////////////////////////////////////////////////////////////////////

  if( !ta_split_enabled || (note < ta_split_note) ) {
    n = &notestack[NOTESTACK_TRANSPOSER];
    if( velocity ) { // Note On
      SEQ_MIDI_IN_Notestack_Push(n, note, 0); // unsorted
      transposer_hold_note = n->note[0];

      // will only be used if enabled in OPT menu
      seq_core_keyb_scale_root = note % 12;
    } else { // Note Off
      if( SEQ_MIDI_IN_Notestack_Pop(n, note) > 0 && n->len ) {
	transposer_hold_note = n->note[0];
      }
    }

#if DEBUG_VERBOSE_LEVEL >= 1
    {
      int i;
      DEBUG_MSG("[T Notestack] IN: %02x %02x  LEN: %d  Notes:", note, velocity, n->len);
      for(i=0; i<SEQ_MIDI_IN_NOTESTACK_SIZE; ++i)
        DEBUG_MSG(" %02x", n->note[i]);
      DEBUG_MSG("\n");
    }
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
	  arp_sorted_hold[i] = arp_unsorted_hold[i] = 0;
      }

      // add to stacks
      SEQ_MIDI_IN_Notestack_Push(&notestack[NOTESTACK_ARP_SORTED], note, 1);
      SEQ_MIDI_IN_Notestack_Push(&notestack[NOTESTACK_ARP_UNSORTED], note, 0);

      // copy to hold stack
      int i;
      for(i=0; i<4; ++i) {
	arp_unsorted_hold[i] = (i < notestack[NOTESTACK_ARP_UNSORTED].len) ? notestack[NOTESTACK_ARP_UNSORTED].note[i] : 0;
	arp_sorted_hold[i] = (i < notestack[NOTESTACK_ARP_SORTED].len) ? notestack[NOTESTACK_ARP_SORTED].note[i] : 0;
      }

    } else { // Note Off
      // remove note from sorted/unsorted stack (not hold stacks)
      SEQ_MIDI_IN_Notestack_Pop(&notestack[NOTESTACK_ARP_SORTED], note);
      SEQ_MIDI_IN_Notestack_Pop(&notestack[NOTESTACK_ARP_UNSORTED], note);
    }

#if DEBUG_VERBOSE_LEVEL >= 1
    {
      int i;
      DEBUG_MSG("[ARP Sorted] IN: %02x %02x  LEN: %d  Notes:", note, velocity, notestack[NOTESTACK_ARP_SORTED].len);
      for(i=0; i<SEQ_MIDI_IN_NOTESTACK_SIZE; ++i)
        DEBUG_MSG(" %02x", notestack[NOTESTACK_ARP_SORTED].note[i]);
      DEBUG_MSG("\n");

      DEBUG_MSG("[ARP Unsorted] IN: %02x %02x  LEN: %d  Notes:", note, velocity, notestack[NOTESTACK_ARP_UNSORTED].len);
      for(i=0; i<SEQ_MIDI_IN_NOTESTACK_SIZE; ++i)
        DEBUG_MSG(" %02x", notestack[NOTESTACK_ARP_UNSORTED].note[i]);
      DEBUG_MSG("\n");
    }
#endif
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Notestack handling
/////////////////////////////////////////////////////////////////////////////
static s32 SEQ_MIDI_IN_Notestack_Pop(seq_midi_in_notestack_t *n, u8 old_note)
{
  int i, j;

  // search for note with same value and remove it
  // (SEQ_MIDI_IN_Notestack_Push ensures, that a note value only exists one time in stack)
  for(i=0; i < n->len; ++i) {
    if( n->note[i] == old_note ) {
      for(j=i; j < n->len-1; ++j)
	n->note[j] = n->note[j+1];
      --n->len;
      n->note[n->len] = 0x00;

      return 1; // note has been found and removed
    }
  }

  // note hasn't been found
  return 0;
}

static s32 SEQ_MIDI_IN_Notestack_Push(seq_midi_in_notestack_t *n, u8 new_note, u8 sort)
{
  int i;

  // check if note already in stack - in this case, remove it
  SEQ_MIDI_IN_Notestack_Pop(n, new_note);

  // add note to the beginning of the stack (FILO)
  // if sort flag set: search for insertion point
  int insertion_point = 0;
  if( sort && n->len ) {
    for(i=0; i<n->len; ++i)
      if( n->note[i] > new_note ) {
	insertion_point = i;
	break;
      }
    if( i == n->len ) {
      // corner case: stack is full, and new note is greater than all others:
      // replace last note by new one and exit
      if( n->len >= SEQ_MIDI_IN_NOTESTACK_SIZE ) {
	n->note[n->len-1] = new_note;
	return 0; // no error
      }
      insertion_point = n->len;
    }
  }
  
  // increment length so long it hasn't reached the notestack size
  if( n->len < SEQ_MIDI_IN_NOTESTACK_SIZE )
    ++n->len;
  
  // add note at insertion point
  for(i=n->len-1; i > insertion_point; --i)
    n->note[i] = n->note[i-1];
  n->note[insertion_point] = new_note;

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

  return notestack[NOTESTACK_TRANSPOSER].len ? notestack[NOTESTACK_TRANSPOSER].note[0] : -1;
}

/////////////////////////////////////////////////////////////////////////////
// Returns the note for arp mode, expected key_num range: 0..3
// if -1, the stack is empty
// if bit 7 is set, the last key of the stack has been played
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_MIDI_IN_ArpNoteGet(u8 hold, u8 sorted, u8 key_num)
{
  u8 *note_ptr;

  if( sorted )
    note_ptr = hold ? arp_sorted_hold : notestack[NOTESTACK_ARP_SORTED].note;
  else
    note_ptr = hold ? arp_unsorted_hold : notestack[NOTESTACK_ARP_UNSORTED].note;

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
      if( !note_ptr[num_notes] )
	break;
  } else
    num_notes = notestack[NOTESTACK_ARP_SORTED].len;

  // btw.: compare with lines of code in PIC based solution! :)
  return note_ptr[key_num % num_notes] | ((!num_notes || key_num >= (num_notes-1)) ? 0x80 : 0);
}
