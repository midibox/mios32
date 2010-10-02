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

#include <notestack.h>

#include "seq_ui.h"
#include "seq_midi_in.h"
#include "seq_cv.h"
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

#define SEQ_MIDI_IN_SECTION_CHANGER_NOTESTACK_SIZE 4
#define SEQ_MIDI_IN_PATCH_CHANGER_NOTESTACK_SIZE 4


// number of notestacks and assignments
#define BUS_NOTESTACK_NUM             3
#define BUS_NOTESTACK_TRANSPOSER      0
#define BUS_NOTESTACK_ARP_SORTED      1
#define BUS_NOTESTACK_ARP_UNSORTED    2

// additional notestacks for section and patch changer
#define SECTION_CHANGER_NOTESTACK_NUM  4
#define SECTION_CHANGER_NOTESTACK_G1   0
#define SECTION_CHANGER_NOTESTACK_G2   1
#define SECTION_CHANGER_NOTESTACK_G3   2
#define SECTION_CHANGER_NOTESTACK_G4   3

#define PATCH_CHANGER_NOTESTACK_NUM    4
#define PATCH_CHANGER_NOTESTACK_G1     0
#define PATCH_CHANGER_NOTESTACK_G2     1
#define PATCH_CHANGER_NOTESTACK_G3     2
#define PATCH_CHANGER_NOTESTACK_G4     3


/////////////////////////////////////////////////////////////////////////////
// Local prototypes
/////////////////////////////////////////////////////////////////////////////

static s32 SEQ_MIDI_IN_Receive_Note(u8 bus, u8 note, u8 velocity);
static s32 SEQ_MIDI_IN_Receive_CC(u8 bus, u8 cc, u8 value);
static s32 SEQ_MIDI_IN_Receive_NoteSC(u8 note, u8 velocity);
static s32 SEQ_MIDI_IN_Receive_NotePC(u8 note, u8 velocity);


/////////////////////////////////////////////////////////////////////////////
// Global variables
/////////////////////////////////////////////////////////////////////////////

// For Transposer/Arpeggiator:
// 0 disables MIDI In, 1..16 define the MIDI channel which should be used
u8 seq_midi_in_channel[SEQ_MIDI_IN_NUM_BUSSES];
// which IN port should be used? (0: All)
mios32_midi_port_t seq_midi_in_port[SEQ_MIDI_IN_NUM_BUSSES];

// lower/upper note of bus range
u8 seq_midi_in_lower[SEQ_MIDI_IN_NUM_BUSSES];
u8 seq_midi_in_upper[SEQ_MIDI_IN_NUM_BUSSES];

// options
seq_midi_in_options_t seq_midi_in_options[SEQ_MIDI_IN_NUM_BUSSES];

// For Record function:
// 0 disables MIDI In, 1..16 define the MIDI channel which should be used
u8 seq_midi_in_rec_channel;
// which IN port should be used? (0: All)
mios32_midi_port_t seq_midi_in_rec_port;


// for Sections:
// 0 disables MIDI In, 1..16 define the MIDI channel which should be used
u8 seq_midi_in_sect_channel;
// which IN port should be used? (0: All)
mios32_midi_port_t seq_midi_in_sect_port;
// optional OUT port to forward octaves not taken by section selection (0: disable)
mios32_midi_port_t seq_midi_in_sect_fwd_port;
// Starting note for section selection:
u8 seq_midi_in_sect_note[4];


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

// we have three notestacks per bus: one for transposer, two other for arpeggiator
static notestack_t bus_notestack[SEQ_MIDI_IN_NUM_BUSSES][BUS_NOTESTACK_NUM];
static notestack_item_t bus_notestack_items[SEQ_MIDI_IN_NUM_BUSSES][BUS_NOTESTACK_NUM][SEQ_MIDI_IN_NOTESTACK_SIZE];

static notestack_t section_changer_notestack[SECTION_CHANGER_NOTESTACK_NUM];
static notestack_item_t section_changer_notestack_items[SECTION_CHANGER_NOTESTACK_NUM][SEQ_MIDI_IN_NOTESTACK_SIZE];

static notestack_t patch_changer_notestack[SECTION_CHANGER_NOTESTACK_NUM];
static notestack_item_t patch_changer_notestack_items[SECTION_CHANGER_NOTESTACK_NUM][SEQ_MIDI_IN_NOTESTACK_SIZE];

// last note which has been played
static u8 transposer_hold_note[SEQ_MIDI_IN_NUM_BUSSES];

// last arp notes which have been played
static notestack_item_t arp_sorted_hold[SEQ_MIDI_IN_NUM_BUSSES][4];
static notestack_item_t arp_unsorted_hold[SEQ_MIDI_IN_NUM_BUSSES][4];

// for MIDI remote keyboard function
static u8 remote_active;


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_MIDI_IN_Init(u32 mode)
{
  int bus;

  SEQ_MIDI_IN_ResetAllStacks();

  // stored in global config:
  for(bus=0; bus<SEQ_MIDI_IN_NUM_BUSSES; ++bus) {
    seq_midi_in_channel[bus] = bus+1; // Channel #1 (0 disables MIDI IN)
    seq_midi_in_port[bus] = DEFAULT; // All ports
    seq_midi_in_lower[bus] = 0x00; // lowest note
    seq_midi_in_upper[bus] = 0x7f; // highest note
    seq_midi_in_options[bus].ALL = 0; // disable all options
  }

  seq_midi_in_rec_channel = 1; // Channel #1 (0 disables MIDI IN)
  seq_midi_in_rec_port = DEFAULT; // All ports

  seq_midi_in_sect_channel = 0; // disabled by default
  seq_midi_in_sect_port = DEFAULT; // All ports
  seq_midi_in_sect_fwd_port = DEFAULT; // 0 = disable

  int i;
  for(i=0; i<4; ++i)
    seq_midi_in_sect_note[i] = 0x30 + 12*i;

  remote_active = 0;

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// resets note stacks used for transposer/arpeggiator
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_MIDI_IN_ResetTransArpStacks(void)
{
  int bus;
  for(bus=0; bus<SEQ_MIDI_IN_NUM_BUSSES; ++bus) {
    NOTESTACK_Init(&bus_notestack[bus][BUS_NOTESTACK_TRANSPOSER],
		   NOTESTACK_MODE_PUSH_TOP,
		   &bus_notestack_items[bus][BUS_NOTESTACK_TRANSPOSER][0],
		   SEQ_MIDI_IN_NOTESTACK_SIZE);

    NOTESTACK_Init(&bus_notestack[bus][BUS_NOTESTACK_ARP_SORTED],
		   NOTESTACK_MODE_SORT,
		   &bus_notestack_items[bus][BUS_NOTESTACK_ARP_SORTED][0],
		   SEQ_MIDI_IN_NOTESTACK_SIZE);

    NOTESTACK_Init(&bus_notestack[bus][BUS_NOTESTACK_ARP_UNSORTED],
		   NOTESTACK_MODE_PUSH_TOP,
		   &bus_notestack_items[bus][BUS_NOTESTACK_ARP_UNSORTED][0],
		   SEQ_MIDI_IN_NOTESTACK_SIZE);

    transposer_hold_note[bus] = 0x3c; // C-3

    int i;
    for(i=0; i<4; ++i)
      arp_sorted_hold[bus][i].ALL = arp_unsorted_hold[bus][i].ALL = 0;

    arp_sorted_hold[bus][0].note = arp_unsorted_hold[bus][0].note = 0x3c; // C-3
    arp_sorted_hold[bus][1].note = arp_unsorted_hold[bus][1].note = 0x40; // E-3
    arp_sorted_hold[bus][2].note = arp_unsorted_hold[bus][2].note = 0x43; // G-3
    arp_sorted_hold[bus][3].note = arp_unsorted_hold[bus][3].note = 0x48; // C-4
  }
    
  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// resets note stacks used for patch changer
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_MIDI_IN_ResetChangerStacks(void)
{
  int i;

  for(i=0; i<SECTION_CHANGER_NOTESTACK_NUM; ++i)
    NOTESTACK_Init(&section_changer_notestack[i],
		   NOTESTACK_MODE_PUSH_TOP,
		   &section_changer_notestack_items[i][0],
		   SEQ_MIDI_IN_SECTION_CHANGER_NOTESTACK_SIZE);

  for(i=0; i<PATCH_CHANGER_NOTESTACK_NUM; ++i)
    NOTESTACK_Init(&patch_changer_notestack[i],
		   NOTESTACK_MODE_PUSH_TOP,
		   &patch_changer_notestack_items[i][0],
		   SEQ_MIDI_IN_PATCH_CHANGER_NOTESTACK_SIZE);

  // following operation should be atomic!
  u8 track;
  seq_core_trk_t *t = &seq_core_trk[0];
  MIOS32_IRQ_Disable();
  for(track=0; track<SEQ_CORE_NUM_TRACKS; ++track, ++t)
    t->play_section = 0; // don't select section
  MIOS32_IRQ_Enable();

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// resets all note stacks
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_MIDI_IN_ResetAllStacks(void)
{
  s32 status = 0;

  status |= SEQ_MIDI_IN_ResetTransArpStacks();
  status |= SEQ_MIDI_IN_ResetChangerStacks();

  return status;
}


/////////////////////////////////////////////////////////////////////////////
// Receives a MIDI package from APP_NotifyReceivedEvent (-> app.c)
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_MIDI_IN_Receive(mios32_midi_port_t port, mios32_midi_package_t midi_package)
{
  s32 status = 0;

  // check if we should record this event
  u8 should_be_recorded =
    ui_page == SEQ_UI_PAGE_TRKREC &&
    ((!seq_midi_in_rec_port || port == seq_midi_in_rec_port) &&
     midi_package.chn == (seq_midi_in_rec_channel-1));

  // search for matching ports
  // status[0] set if at least one port matched
  // status[1] set if remote function active
  int bus;
  for(bus=0; bus<SEQ_MIDI_IN_NUM_BUSSES; ++bus) {
    // filter MIDI port (if 0: no filter, listen to all ports)
    if( (!seq_midi_in_port[bus] || port == seq_midi_in_port[bus]) &&
	midi_package.chn == (seq_midi_in_channel[bus]-1) ) {

      switch( midi_package.event ) {
      case NoteOff: 
	if( remote_active ) {
	  if( seq_hwcfg_midi_remote.key && midi_package.note == seq_hwcfg_midi_remote.key ) {
	    remote_active = 0;

	    // send "button depressed" state to all remote functions
	    int i;
	    for(i=0; i<128; ++i)
	      SEQ_UI_REMOTE_MIDI_Keyboard(i, 1); // depressed
	  } else {
	    SEQ_UI_REMOTE_MIDI_Keyboard(midi_package.note, 1); // depressed
	  }
	  status |= 2;
	} else {
	  if( !should_be_recorded &&
	      midi_package.note >= seq_midi_in_lower[bus] &&
	      (!seq_midi_in_upper[bus] || midi_package.note <= seq_midi_in_upper[bus]) ) {
	    if( seq_midi_in_options[bus].MODE_PLAY ) {
	      MUTEX_MIDIOUT_TAKE;
	      SEQ_CORE_PlayLive(SEQ_UI_VisibleTrackGet(), midi_package);
	      MUTEX_MIDIOUT_GIVE;
	    } else {
#if 0
	      // octave normalisation - too complicated for normal users...
	      mios32_midi_package_t p = midi_package;
	      if( seq_midi_in_lower[bus] ) { // normalize to first octave
		int normalized_note = 0x30 + p.note - ((int)seq_midi_in_lower[bus]/12)*12;
		while( normalized_note > 127 ) normalized_note -= 12;
		while( normalized_note < 0 ) normalized_note += 12;
		p.note = normalized_note;
	      }
	      SEQ_MIDI_IN_BusReceive(0xf0+bus, p, 0);
#else
	      SEQ_MIDI_IN_BusReceive(0xf0+bus, midi_package, 0);
#endif
	    }
	    status |= 1;
	  }
	}
	break;

      case NoteOn:
	if( remote_active )
	  SEQ_UI_REMOTE_MIDI_Keyboard(midi_package.note, midi_package.velocity ? 0 : 1); // depressed
	else {
	  if( seq_hwcfg_midi_remote.key && midi_package.note == seq_hwcfg_midi_remote.key ) {
	    remote_active = 1;
	    status |= 2;
	  } else {
	    if( !should_be_recorded &&
		midi_package.note >= seq_midi_in_lower[bus] &&
		(!seq_midi_in_upper[bus] || midi_package.note <= seq_midi_in_upper[bus]) ) {
	      if( seq_midi_in_options[bus].MODE_PLAY ) {
		MUTEX_MIDIOUT_TAKE;
		SEQ_CORE_PlayLive(SEQ_UI_VisibleTrackGet(), midi_package);
		MUTEX_MIDIOUT_GIVE;
	      } else {
#if 0
		// octave normalisation - too complicated for normal users...
		mios32_midi_package_t p = midi_package;
		if( seq_midi_in_lower[bus] ) { // normalize to first octave
		  int normalized_note = 0x30 + p.note - ((int)seq_midi_in_lower[bus]/12)*12;
		  while( normalized_note > 127 ) normalized_note -= 12;
		  while( normalized_note < 0 ) normalized_note += 12;
		  p.note = normalized_note;
		}
		SEQ_MIDI_IN_BusReceive(0xf0+bus, p, 0);
#else
		SEQ_MIDI_IN_BusReceive(0xf0+bus, midi_package, 0);
#endif
	      }
	      status |= 1;
	    }
	  }
	}
	break;

      case CC:
	if( !should_be_recorded ) {
	  if( seq_midi_in_options[bus].MODE_PLAY ) {
	    MUTEX_MIDIOUT_TAKE;
	    SEQ_CORE_PlayLive(SEQ_UI_VisibleTrackGet(), midi_package);
	    MUTEX_MIDIOUT_GIVE;
	  } else {
	    SEQ_MIDI_IN_BusReceive(0xf0+bus, midi_package, 0);
	  }
	}
	status |= 1;
	break;

      default:
	if( seq_midi_in_options[bus].MODE_PLAY ) {
	  MUTEX_MIDIOUT_TAKE;
	  SEQ_CORE_PlayLive(SEQ_UI_VisibleTrackGet(), midi_package);
	  MUTEX_MIDIOUT_GIVE;
	}
      }
    }
  }


  // record function
  if( !(status & 2) && should_be_recorded ) {
    SEQ_RECORD_Receive(midi_package, SEQ_UI_VisibleTrackGet());
  }

  // Section Changer
  if( !(status & 2) &&
      (seq_midi_in_sect_port && port == seq_midi_in_sect_port &&
       midi_package.chn == (seq_midi_in_sect_channel-1)) ) {
    u8 forward_event = 1;

    switch( midi_package.event ) {
      case NoteOff: 
	MUTEX_MIDIIN_TAKE;
	if( (status = SEQ_MIDI_IN_Receive_NoteSC(midi_package.note, 0x00)) >= 1 )
	  forward_event = 0;
	MUTEX_MIDIIN_GIVE;
	break;

      case NoteOn:
	MUTEX_MIDIIN_TAKE;
	if( (status = SEQ_MIDI_IN_Receive_NoteSC(midi_package.note, midi_package.velocity)) >= 1 )
	  forward_event = 0;
	MUTEX_MIDIIN_GIVE;
	break;
    }

    if( seq_midi_in_sect_fwd_port && forward_event ) { // octave hasn't been taken, optionally forward to forwarding port
      MUTEX_MIDIOUT_TAKE;
      MIOS32_MIDI_SendPackage(seq_midi_in_sect_fwd_port, midi_package);
      MUTEX_MIDIOUT_GIVE;
    }
  }

#if 0
  // Patch Changer (currently assigned to channel+1)
  // Too complicated for the world? Pattern has to be stored before this feature is used to avoid data loss
  if( !(status & 2) &&
      (seq_midi_in_sect_port && port == seq_midi_in_sect_port &&
       midi_package.chn == (seq_midi_in_sect_channel)) ) {
    u8 forward_event = 1;

    switch( midi_package.event ) {
      case NoteOff: 
	MUTEX_MIDIIN_TAKE;
	if( (status = SEQ_MIDI_IN_Receive_NotePC(midi_package.note, 0x00)) >= 1 )
	  forward_event = 0;
	MUTEX_MIDIIN_GIVE;
	break;

      case NoteOn:
	MUTEX_MIDIIN_TAKE;
	if( (status = SEQ_MIDI_IN_Receive_NotePC(midi_package.note, midi_package.velocity)) >= 1 )
	  forward_event = 0;
	MUTEX_MIDIIN_GIVE;
	break;
    }

    if( seq_midi_in_sect_fwd_port && forward_event ) { // octave hasn't been taken, optionally forward to forwarding port
      MUTEX_MIDIOUT_TAKE;
      MIOS32_MIDI_SendPackage(seq_midi_in_sect_fwd_port, midi_package);
      MUTEX_MIDIOUT_GIVE;
    }
  }
#endif

  return status;
}


/////////////////////////////////////////////////////////////////////////////
// Receives a MIDI package from a loopback port (Bus1..Bus4)
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_MIDI_IN_BusReceive(mios32_midi_port_t port, mios32_midi_package_t midi_package, u8 from_loopback_port)
{
  s32 status = 0;
  u8 bus = (port >= 0xf0) ? (port-0xf0) : 0xf0;

  // Access to MIDI IN functions controlled by Mutex, since this function is access
  // by different tasks (APP_NotifyReceivedEvent() for received MIDI events, and 
  // SEQ_CORE_* for loopbacks)
  MUTEX_MIDIIN_TAKE;

  switch( midi_package.event ) {
  case NoteOff: 
    status = SEQ_MIDI_IN_Receive_Note(bus, midi_package.note, 0x00);
    break;

  case NoteOn:
    status = SEQ_MIDI_IN_Receive_Note(bus, midi_package.note, midi_package.velocity);
    break;

  case CC:
    if( from_loopback_port )
      status = SEQ_CC_MIDI_Set(midi_package.chn, midi_package.cc_number, midi_package.value);
    else
      status = SEQ_MIDI_IN_Receive_CC(bus, midi_package.cc_number, midi_package.value);
    break;
  }

  MUTEX_MIDIIN_GIVE;

  return status;
}


/////////////////////////////////////////////////////////////////////////////
// If velocity == 0, Note Off event has been received, otherwise Note On event
/////////////////////////////////////////////////////////////////////////////
static s32 SEQ_MIDI_IN_Receive_Note(u8 bus, u8 note, u8 velocity)
{
  notestack_t *n;

  if( bus >= SEQ_MIDI_IN_NUM_BUSSES )
    return -1;

  ///////////////////////////////////////////////////////////////////////////
  // Transposer
  ///////////////////////////////////////////////////////////////////////////

  n = &bus_notestack[bus][BUS_NOTESTACK_TRANSPOSER];
  if( velocity ) { // Note On
    NOTESTACK_Push(n, note, velocity);
    transposer_hold_note[bus] = n->note_items[0].note;

    // will only be used for Bus1 and if enabled in OPT menu
    if( bus == 0 )
      seq_core_keyb_scale_root = note % 12;
  } else { // Note Off
    if( NOTESTACK_Pop(n, note) > 0 && n->len ) {
      transposer_hold_note[bus] = n->note_items[0].note;
    }
  }
#if DEBUG_VERBOSE_LEVEL >= 1
  DEBUG_MSG("NOTESTACK_TRANSPOSER[%d]:\n", bus);
  NOTESTACK_SendDebugMessage(&bus_notestack[bus][BUS_NOTESTACK_TRANSPOSER]);
#endif


  ///////////////////////////////////////////////////////////////////////////
  // Arpeggiator
  ///////////////////////////////////////////////////////////////////////////

  if( velocity ) { // Note On
    // if no note in note stack anymore, reset position of all tracks with RESTART flag set
    if( !bus_notestack[bus][BUS_NOTESTACK_ARP_UNSORTED].len ) {
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
	arp_sorted_hold[bus][i].ALL = arp_unsorted_hold[bus][i].ALL = 0;
    }

    // add to stacks
    NOTESTACK_Push(&bus_notestack[bus][BUS_NOTESTACK_ARP_SORTED], note, velocity);
    NOTESTACK_Push(&bus_notestack[bus][BUS_NOTESTACK_ARP_UNSORTED], note, velocity);

    // copy to hold stack
    int i;
    for(i=0; i<4; ++i) {
      arp_unsorted_hold[bus][i].ALL = (i < bus_notestack[bus][BUS_NOTESTACK_ARP_UNSORTED].len) ? bus_notestack[bus][BUS_NOTESTACK_ARP_UNSORTED].note_items[i].ALL : 0;
      arp_sorted_hold[bus][i].ALL = (i < bus_notestack[bus][BUS_NOTESTACK_ARP_SORTED].len) ? bus_notestack[bus][BUS_NOTESTACK_ARP_SORTED].note_items[i].ALL : 0;
    }

  } else { // Note Off
    // remove note from sorted/unsorted stack (not hold stacks)
    NOTESTACK_Pop(&bus_notestack[bus][BUS_NOTESTACK_ARP_SORTED], note);
    NOTESTACK_Pop(&bus_notestack[bus][BUS_NOTESTACK_ARP_UNSORTED], note);
  }

#if DEBUG_VERBOSE_LEVEL >= 1
  DEBUG_MSG("NOTESTACK_ARP_SORTED[%d]:\n", bus);
  NOTESTACK_SendDebugMessage(&bus_notestack[bus][BUS_NOTESTACK_ARP_SORTED]);
  DEBUG_MSG("NOTESTACK_ARP_UNSORTED[%d]:\n", bus);
  NOTESTACK_SendDebugMessage(&bus_notestack[bus][BUS_NOTESTACK_ARP_UNSORTED]);
#endif

  return 0; // no error
}



/////////////////////////////////////////////////////////////////////////////
// For Section Changes
// If velocity == 0, Note Off event has been received, otherwise Note On event
/////////////////////////////////////////////////////////////////////////////
static s32 SEQ_MIDI_IN_Receive_NoteSC(u8 note, u8 velocity)
{
  int octave = note / 12;
  int octave_taken = 0;

  int group;
  for(group=0; group<4; ++group) {
    if( octave == (seq_midi_in_sect_note[group] / 12) ) {
      octave_taken = 1;
      notestack_t *n = &section_changer_notestack[group];

      int section = -1;
      if( velocity ) { // Note On
	NOTESTACK_Push(n, note, velocity);
	section = n->note_items[0].note % 12;
      } else { // Note Off
	if( NOTESTACK_Pop(n, note) > 0 && n->len ) {
	  section = n->note_items[0].note % 12;
	}
      }

      // switch to new section if required
      if( section >= 0 && section < 12 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	DEBUG_MSG("Group %d Section %d\n", group, section);
#endif

	// following operation should be atomic!
	u8 track;
	seq_core_trk_t *t = &seq_core_trk[group*SEQ_CORE_NUM_TRACKS_PER_GROUP];
	MIOS32_IRQ_Disable();
	for(track=0; track<SEQ_CORE_NUM_TRACKS_PER_GROUP; ++track, ++t)
	  t->play_section = section;
	MIOS32_IRQ_Enable();
      }

#if DEBUG_VERBOSE_LEVEL >= 1
      DEBUG_MSG("NOTESTACK_SECTION_CHANGER_G%d:\n", group+1);
      NOTESTACK_SendDebugMessage(n);
#endif
    }
  }

  return octave_taken; // return 1 if octave has been taken, otherwise 0
}


/////////////////////////////////////////////////////////////////////////////
// For Patch Changes (assigned to different MIDI channel)
// If velocity == 0, Note Off event has been received, otherwise Note On event
/////////////////////////////////////////////////////////////////////////////
static s32 SEQ_MIDI_IN_Receive_NotePC(u8 note, u8 velocity)
{
  int octave = note / 12;
  int octave_taken = 0;

  int group;
  for(group=0; group<4; ++group) {
    if( octave == (seq_midi_in_sect_note[group] / 12) ) {
      octave_taken = 1;
      notestack_t *n = &patch_changer_notestack[group];

      int patch = -1;
      if( velocity ) { // Note On
	NOTESTACK_Push(n, note, velocity);
	patch = n->note_items[0].note % 12;
      } else { // Note Off
	if( NOTESTACK_Pop(n, note) > 0 && n->len ) {
	  patch = n->note_items[0].note % 12;
	}
      }

      // switch to new patch if required
      if( patch >= 0 && patch < 8 ) {
	seq_pattern_t pattern = seq_pattern[group];
	if( pattern.num != patch ) {
	  pattern.num = patch;
	  pattern.DISABLED = 0;
	  pattern.SYNCHED = 0;
	  SEQ_PATTERN_Change(group, pattern);
	}
      }

#if DEBUG_VERBOSE_LEVEL >= 0
      DEBUG_MSG("NOTESTACK_PATCH_CHANGER_G%d:\n", group+1);
      NOTESTACK_SendDebugMessage(n);
#endif
    }
  }

  return octave_taken; // return 1 if octave has been taken, otherwise 0
}


/////////////////////////////////////////////////////////////////////////////
// CC has been received over selected port and channel
/////////////////////////////////////////////////////////////////////////////
static s32 SEQ_MIDI_IN_Receive_CC(u8 bus, u8 cc, u8 value)
{
  static nrpn_lsb = 0;
  static nrpn_msb = 0;

  if( bus >= SEQ_MIDI_IN_NUM_BUSSES )
    return -1;

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

    case 0x7b: // all notes off (transposer, arpeggiator, patch changer)
      if( value == 0 ) {
	SEQ_MIDI_IN_ResetAllStacks();
	SEQ_CV_ResetAllChannels();
      }
      break;
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Returns the note for transpose mode
// if -1, the stack is empty
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_MIDI_IN_TransposerNoteGet(u8 bus, u8 hold)
{
  if( hold )
    return transposer_hold_note[bus];

  return bus_notestack[bus][BUS_NOTESTACK_TRANSPOSER].len ? bus_notestack[bus][BUS_NOTESTACK_TRANSPOSER].note_items[0].note : -1;
}

/////////////////////////////////////////////////////////////////////////////
// Returns the note for arp mode, expected key_num range: 0..3
// if -1, the stack is empty
// if bit 7 is set, the last key of the stack has been played
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_MIDI_IN_ArpNoteGet(u8 bus, u8 hold, u8 sorted, u8 key_num)
{
  notestack_item_t *note_ptr;

  if( sorted )
    note_ptr = hold ? arp_sorted_hold[bus] : bus_notestack[bus][BUS_NOTESTACK_ARP_SORTED].note_items;
  else
    note_ptr = hold ? arp_unsorted_hold[bus] : bus_notestack[bus][BUS_NOTESTACK_ARP_UNSORTED].note_items;

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
    num_notes = bus_notestack[bus][BUS_NOTESTACK_ARP_SORTED].len;

  // btw.: compare with lines of code in PIC based solution! :)
  return note_ptr[key_num % num_notes].note | ((!num_notes || key_num >= (num_notes-1)) ? 0x80 : 0);
}
