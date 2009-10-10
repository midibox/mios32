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
#include "seq_pattern.h"
#include "seq_morph.h"
#include "seq_core.h"
#include "seq_record.h"
#include "seq_hwcfg.h"
#include "seq_file_b.h"


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

// for MIDI remote keyboard function
static u8 remote_active;


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

  remote_active = 0;

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
  u8 loopback_port = port == 0xf0;

  // filter MIDI port (if 0: no filter, listen to all ports)
#if 0
  if( seq_midi_in_port && port != seq_midi_in_port )
#else
  // Loopback port not filtered!
  if( seq_midi_in_port && port != seq_midi_in_port && !loopback_port )
#endif
    return status;

  // if not loopback, no remote and MIDI channel matching: forward to record function in record page
  if( !loopback_port && !remote_active && ui_page == SEQ_UI_PAGE_TRKREC && midi_package.chn == (seq_midi_in_channel-1) )
    return SEQ_RECORD_Receive(midi_package, SEQ_UI_VisibleTrackGet());

  // Access to MIDI IN functions controlled by Mutex, since this function is access
  // by different tasks (APP_NotifyReceivedEvent() for received MIDI events, and 
  // SEQ_CORE_* for loopbacks)

  // Note Events: ignore channel if loopback
  if( loopback_port || midi_package.chn == (seq_midi_in_channel-1) ) {
    switch( midi_package.event ) {

      case NoteOff: 
	if( !loopback_port && remote_active ) {
	  if( seq_hwcfg_midi_remote.key && midi_package.note == seq_hwcfg_midi_remote.key ) {
	    int i;

	    remote_active = 0;
	    // send "button depressed" state to all remote functions
	    for(i=0; i<128; ++i)
	      SEQ_UI_REMOTE_MIDI_Keyboard(i, 1); // depressed

	    status = 1;
	  } else
	    status = SEQ_UI_REMOTE_MIDI_Keyboard(midi_package.note, 1); // depressed
	} else {
	  MUTEX_MIDIIN_TAKE;
	  status = SEQ_MIDI_IN_Receive_Note(midi_package.note, 0x00);
	  MUTEX_MIDIIN_GIVE;
	}
	break;

      case NoteOn:
	if( !loopback_port && remote_active )
	  status = SEQ_UI_REMOTE_MIDI_Keyboard(midi_package.note, midi_package.velocity ? 0 : 1); // depressed
	else {
	  if( seq_hwcfg_midi_remote.key && midi_package.note == seq_hwcfg_midi_remote.key ) {
	    remote_active = 1;
	    status = 1;
	  } else {
	    MUTEX_MIDIIN_TAKE;
	    status = SEQ_MIDI_IN_Receive_Note(midi_package.note, midi_package.velocity);
	    MUTEX_MIDIIN_GIVE;
	  }
	}
	break;

      case CC:
	MUTEX_MIDIIN_TAKE;
	if( loopback_port )
	  status = SEQ_CC_MIDI_Set(midi_package.chn, midi_package.cc_number, midi_package.value);
	else
	  status = SEQ_MIDI_IN_Receive_CC(midi_package.cc_number, midi_package.value);
	MUTEX_MIDIIN_GIVE;
	break;
    }
  }

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
  static nrpn_lsb = 0;
  static nrpn_msb = 0;

  // MIDI Remote Function?
  if( seq_hwcfg_midi_remote.cc && seq_hwcfg_midi_remote.cc == cc ) {
    remote_active = value >= 0x40;
    return 1;
  }

  // Remaining functions
  switch( cc ) {
    case 0x01: // ModWheel -> Morph Value
    case 0x03: // Global Scale
      return SEQ_CC_MIDI_Set(0, cc, value);

    case 0x70: // Pattern Group #1
    case 0x71: // Pattern Group #2
    case 0x72: // Pattern Group #3
    case 0x73: { // Pattern Group #4
      u8 group = cc-0x70;
      seq_pattern_t pattern = seq_pattern[group];
      if( value < SEQ_FILE_B_NumPatterns(pattern.bank) ) {
	pattern.pattern = value;
	pattern.DISABLED = 0;
	pattern.SYNCHED = 0;
	SEQ_PATTERN_Change(group, pattern);
      }
      return 1;
    } break;
      
    case 0x74: // Bank Group #1
    case 0x75: // Bank Group #2
    case 0x76: // Bank Group #3
    case 0x77: { // Bank Group #4
      u8 group = cc-0x74;
      seq_pattern_t pattern = seq_pattern[group];
      if( value < SEQ_FILE_B_NUM_BANKS ) {
	pattern.bank = value;
	pattern.DISABLED = 0;
	pattern.SYNCHED = 0;
	SEQ_PATTERN_Change(group, pattern);
      }
      return 1;
    } break;
      
    case 0x62: // NRPN LSB (selects parameter)
      nrpn_lsb = value;
      return 1;

    case 0x63: // NRPN MSB (selects track)
      nrpn_msb = value;
      return 1;

    case 0x06: // NRPN Value LSB (sets parameter)
      if( nrpn_msb < SEQ_CORE_NUM_TRACKS)
	return SEQ_CC_MIDI_Set(nrpn_msb, nrpn_lsb, value);
      break;
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
