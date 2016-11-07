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
#include <seq_bpm.h>

#include "seq_ui.h"
#include "seq_midi_in.h"
#include "seq_core.h"
#include "seq_cv.h"
#include "seq_cc.h"
#include "seq_pattern.h"
#include "seq_song.h"
#include "seq_mixer.h"
#include "seq_morph.h"
#include "seq_core.h"
#include "seq_record.h"
#include "seq_live.h"
#include "seq_hwcfg.h"
#include "seq_file_b.h"


/////////////////////////////////////////////////////////////////////////////
// for optional debugging messages via DEBUG_MSG (defined in mios32_config.h)
/////////////////////////////////////////////////////////////////////////////
#define DEBUG_VERBOSE_LEVEL 0


/////////////////////////////////////////////////////////////////////////////
// Local definitions
/////////////////////////////////////////////////////////////////////////////

// Too complicated for the world? Pattern has to be stored before this feature is used to avoid data loss
#define PATCH_CHANGER_ENABLED 0

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
static s32 SEQ_MIDI_IN_Receive_ExtCtrlCC(u8 cc, u8 value);
static s32 SEQ_MIDI_IN_Receive_ExtCtrlPC(u8 value);
static s32 SEQ_MIDI_IN_Receive_NoteSC(u8 note, u8 velocity);
#if PATCH_CHANGER_ENABLED
static s32 SEQ_MIDI_IN_Receive_NotePC(u8 note, u8 velocity);
#endif

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


// For External Control functions:
// 0 disables MIDI In, 1..16 define the MIDI channel which should be used
u8 seq_midi_in_ext_ctrl_channel;
// which IN port should be used? (0: Off, 0xff == All)
mios32_midi_port_t seq_midi_in_ext_ctrl_port;
// and which optional out port? (0: Off)
mios32_midi_port_t seq_midi_in_ext_ctrl_out_port;
// external controller assignments
u8 seq_midi_in_ext_ctrl_asg[SEQ_MIDI_IN_EXT_CTRL_NUM];

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

// first and last note which has been played
static u8 transposer_hold_first_note[SEQ_MIDI_IN_NUM_BUSSES];
static u8 transposer_hold_last_note[SEQ_MIDI_IN_NUM_BUSSES];

// last arp notes which have been played
static notestack_item_t arp_sorted_hold[SEQ_MIDI_IN_NUM_BUSSES][4];
static notestack_item_t arp_unsorted_hold[SEQ_MIDI_IN_NUM_BUSSES][4];

// for MIDI remote keyboard function
static u8 remote_active;

// for automatic note-stack reset
static u8 last_bus_received_from_loopback;


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
    seq_midi_in_options[bus].MODE_PLAY = (bus == 0); // first bus in Play mode by default (for recording)
    seq_midi_in_port[bus] = DEFAULT; // All ports
    seq_midi_in_lower[bus] = 0x00; // lowest note
    seq_midi_in_upper[bus] = 0x7f; // highest note
    seq_midi_in_options[bus].ALL = 0; // disable all options
    
  }

  seq_midi_in_ext_ctrl_channel = 0; // 0 disables MIDI IN
  seq_midi_in_ext_ctrl_port = 0; // off
  seq_midi_in_ext_ctrl_out_port = 0; // off

  seq_midi_in_ext_ctrl_asg[SEQ_MIDI_IN_EXT_CTRL_MORPH] = 1;
  seq_midi_in_ext_ctrl_asg[SEQ_MIDI_IN_EXT_CTRL_SCALE] = 3;
  seq_midi_in_ext_ctrl_asg[SEQ_MIDI_IN_EXT_CTRL_PATTERN_G1] = 112;
  seq_midi_in_ext_ctrl_asg[SEQ_MIDI_IN_EXT_CTRL_PATTERN_G2] = 113;
  seq_midi_in_ext_ctrl_asg[SEQ_MIDI_IN_EXT_CTRL_PATTERN_G3] = 114;
  seq_midi_in_ext_ctrl_asg[SEQ_MIDI_IN_EXT_CTRL_PATTERN_G4] = 115;
  seq_midi_in_ext_ctrl_asg[SEQ_MIDI_IN_EXT_CTRL_SONG] = 102;
  seq_midi_in_ext_ctrl_asg[SEQ_MIDI_IN_EXT_CTRL_PHRASE] = 103;
  seq_midi_in_ext_ctrl_asg[SEQ_MIDI_IN_EXT_CTRL_MIXER_MAP] = 111;
  seq_midi_in_ext_ctrl_asg[SEQ_MIDI_IN_EXT_CTRL_BANK_G1] = 116;
  seq_midi_in_ext_ctrl_asg[SEQ_MIDI_IN_EXT_CTRL_BANK_G2] = 117;
  seq_midi_in_ext_ctrl_asg[SEQ_MIDI_IN_EXT_CTRL_BANK_G3] = 118;
  seq_midi_in_ext_ctrl_asg[SEQ_MIDI_IN_EXT_CTRL_BANK_G4] = 119;
  seq_midi_in_ext_ctrl_asg[SEQ_MIDI_IN_EXT_CTRL_ALL_NOTES_OFF] = 123;
  seq_midi_in_ext_ctrl_asg[SEQ_MIDI_IN_EXT_CTRL_NRPN_ENABLED] = 1;
  seq_midi_in_ext_ctrl_asg[SEQ_MIDI_IN_EXT_CTRL_PC_MODE] = SEQ_MIDI_IN_EXT_CTRL_PC_MODE_OFF;
  seq_midi_in_ext_ctrl_asg[SEQ_MIDI_IN_EXT_CTRL_MUTES] = 128;
  seq_midi_in_ext_ctrl_asg[SEQ_MIDI_IN_EXT_CTRL_STEPS] = 128;

  seq_midi_in_sect_channel = 0; // disabled by default
  seq_midi_in_sect_port = DEFAULT; // All ports
  seq_midi_in_sect_fwd_port = DEFAULT; // 0 = disable

  int i;
  for(i=0; i<4; ++i)
    seq_midi_in_sect_note[i] = 0x30 + 12*i;

  remote_active = 0;
  last_bus_received_from_loopback = 0x00;

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Returns the name of an external controller (up to 15 chars)
/////////////////////////////////////////////////////////////////////////////
const char *SEQ_MIDI_IN_ExtCtrlStr(u8 ext_ctrl)
{
  const char* ext_ctrl_str[SEQ_MIDI_IN_EXT_CTRL_NUM] = {
  //<--------------->
    "Morph Value",
    "Scale",
    "Song Number",
    "Song Phrase",
    "Mixer Map",
    "Pattern G1",
    "Pattern G2",
    "Pattern G3",
    "Pattern G4",
    "Bank G1",
    "Bank G2",
    "Bank G3",
    "Bank G4",
    "All Notes Off",
    "Play/Stop",
    "Record",
    "NRPNs",
    "PrgChange Mode",
    "Mutes(first CC)",
    "Steps(first CC)",
  };

  if( ext_ctrl >= SEQ_MIDI_IN_EXT_CTRL_NUM )
    return "Invalid Ctrl.";

  return ext_ctrl_str[ext_ctrl];
}


/////////////////////////////////////////////////////////////////////////////
// Returns the name of an external controller program change mode (up to 9 chars)
/////////////////////////////////////////////////////////////////////////////
const char *SEQ_MIDI_IN_ExtCtrlPcModeStr(u8 pc_mode)
{
  const char* ext_ctrl_pc_mode_str[SEQ_MIDI_IN_EXT_CTRL_PC_MODE_NUM] = {
  //<---------->
    "off",
    "Patterns",
    "Song",
    "Phrase",
  };

  if( pc_mode >= SEQ_MIDI_IN_EXT_CTRL_PC_MODE_NUM )
    return "UnknwnMde";

  return ext_ctrl_pc_mode_str[pc_mode];
}


/////////////////////////////////////////////////////////////////////////////
// since there is currently no better place (somebody could expect this function in SEQ_MIDI_OUT...)
/////////////////////////////////////////////////////////////////////////////
extern s32 SEQ_MIDI_IN_ExtCtrlSend(u8 ext_ctrl, u8 value, u8 cc_offset)
{
  if( seq_midi_in_ext_ctrl_out_port && seq_midi_in_ext_ctrl_channel ) {
    mios32_midi_port_t port = seq_midi_in_ext_ctrl_out_port;
    mios32_midi_chn_t chn = seq_midi_in_ext_ctrl_channel - 1;

    int cc = seq_midi_in_ext_ctrl_asg[ext_ctrl] + cc_offset;
    if( cc <= 127 ) {
      MUTEX_MIDIOUT_TAKE;
      MIOS32_MIDI_SendCC(port, chn, cc, value);
      MUTEX_MIDIOUT_GIVE;    
    }

    u8 pc_mode = seq_midi_in_ext_ctrl_asg[SEQ_MIDI_IN_EXT_CTRL_PC_MODE];
    if( (pc_mode == SEQ_MIDI_IN_EXT_CTRL_PC_MODE_PATTERNS && ext_ctrl >= SEQ_MIDI_IN_EXT_CTRL_PATTERN_G1 && ext_ctrl <= SEQ_MIDI_IN_EXT_CTRL_PATTERN_G4) ||
	(pc_mode == SEQ_MIDI_IN_EXT_CTRL_PC_MODE_SONG && ext_ctrl == SEQ_MIDI_IN_EXT_CTRL_SONG) ||
	(pc_mode == SEQ_MIDI_IN_EXT_CTRL_PC_MODE_PHRASE && ext_ctrl == SEQ_MIDI_IN_EXT_CTRL_PHRASE) ) {
      MUTEX_MIDIOUT_TAKE;
      MIOS32_MIDI_SendProgramChange(port, chn, value);
      MUTEX_MIDIOUT_GIVE;    
    }
  }

  return 0;
}


/////////////////////////////////////////////////////////////////////////////
// resets note stacks used for transposer/arpeggiator
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_MIDI_IN_ResetSingleTransArpStacks(u8 bus)
{
  if( bus >= SEQ_MIDI_IN_NUM_BUSSES )
    return -1;

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

  transposer_hold_first_note[bus] = 0x3c; // C-3
  transposer_hold_last_note[bus] = 0x3c; // C-3

  int i;
  for(i=0; i<4; ++i)
    arp_sorted_hold[bus][i].ALL = arp_unsorted_hold[bus][i].ALL = 0;

  arp_sorted_hold[bus][0].note = arp_unsorted_hold[bus][0].note = 0x3c; // C-3
  arp_sorted_hold[bus][1].note = arp_unsorted_hold[bus][1].note = 0x40; // E-3
  arp_sorted_hold[bus][2].note = arp_unsorted_hold[bus][2].note = 0x43; // G-3
  arp_sorted_hold[bus][3].note = arp_unsorted_hold[bus][3].note = 0x48; // C-4
    
  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// resets note stacks used for transposer/arpeggiator
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_MIDI_IN_ResetTransArpStacks(void)
{
  int bus;
  for(bus=0; bus<SEQ_MIDI_IN_NUM_BUSSES; ++bus) {
    SEQ_MIDI_IN_ResetSingleTransArpStacks(bus);
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

  // simplify Note On/Off handling
  if( midi_package.event == NoteOff ) {
    midi_package.event = NoteOn;
    midi_package.velocity = 0;
  }

  // check if we should record this event or for remote function
  u8 is_record_port = 0;
  int bus;
  for(bus=0; bus<SEQ_MIDI_IN_NUM_BUSSES; ++bus) {
    mios32_midi_port_t check_port = seq_midi_in_port[bus];
    u8 check_chn = seq_midi_in_channel[bus];

    if( (!check_port || port == check_port) && (check_chn == 17 || midi_package.chn == (check_chn-1)) ) {
      if( seq_midi_in_options[bus].MODE_PLAY ) {
	is_record_port = 1;
      }

      // check for remote key
      if( midi_package.event == NoteOn ) {
	if( midi_package.velocity == 0 ) {
	  if( remote_active ) {
	    if( seq_hwcfg_midi_remote.key && midi_package.note == seq_hwcfg_midi_remote.key ) {
	      remote_active = 0;

	      MUTEX_MIDIOUT_TAKE;
	      DEBUG_MSG("[SEQ_MIDI_IN] MIDI remote access deactivated");
	      MUTEX_MIDIOUT_GIVE;

	      // send "button depressed" state to all remote functions
	      int i;
	      for(i=0; i<128; ++i)
		SEQ_UI_REMOTE_MIDI_Keyboard(i, 1); // depressed
	    } else {
	      SEQ_UI_REMOTE_MIDI_Keyboard(midi_package.note, 1); // depressed
	    }
	    return 0; // stop processing
	  }
	} else {
	  if( remote_active ) {
	    SEQ_UI_REMOTE_MIDI_Keyboard(midi_package.note, 0);
	    return 0; // stop processing
	  } else {
	    if( seq_hwcfg_midi_remote.key && midi_package.note == seq_hwcfg_midi_remote.key ) {
	      MUTEX_MIDIOUT_TAKE;
	      DEBUG_MSG("[SEQ_MIDI_IN] MIDI remote access activated");
	      MUTEX_MIDIOUT_GIVE;
	      remote_active = 1;
	      return 0; // stop processing
	    }
	  }
	}
      }
    }
  }
  u8 should_be_recorded = seq_record_state.ENABLED && is_record_port;

#ifndef MBSEQV4L
  if( is_record_port ) {
    // inform UI MIDI callback
    if( SEQ_UI_NotifyMIDIINCallback(port, midi_package) > 0 )
      return 0; // stop processing
  }
#endif


  // search for matching ports
  // status[0] set if at least one port matched
  for(bus=0; bus<SEQ_MIDI_IN_NUM_BUSSES; ++bus) {
    mios32_midi_port_t check_port = seq_midi_in_port[bus];
    u8 check_chn = seq_midi_in_channel[bus];
    // filter MIDI port (if 0: no filter, listen to all ports)

    if( (!check_port || port == check_port) && (check_chn == 17 || midi_package.chn == (check_chn-1)) ) {

      switch( midi_package.event ) {
      case NoteOff:
      case NoteOn:

	if( midi_package.velocity == 0 ) {
	  if( !should_be_recorded &&
	      midi_package.note >= seq_midi_in_lower[bus] &&
	      (!seq_midi_in_upper[bus] || midi_package.note <= seq_midi_in_upper[bus]) ) {

	    if( !seq_midi_in_options[bus].MODE_PLAY ) {
#if 0
	      // octave normalisation - too complicated for normal users...
	      mios32_midi_package_t p = midi_package;
	      if( seq_midi_in_lower[bus] ) { // normalize to first octave
		int normalized_note = 0x30 + p.note - ((int)seq_midi_in_lower[bus]/12)*12;
		// ensure that note is in the 0..127 range
		normalized_note = SEQ_CORE_TrimNote(normalized_note, 0, 127);
		p.note = normalized_note;
	      }
	      SEQ_MIDI_IN_BusReceive(bus, p, 0);
#else
	      SEQ_MIDI_IN_BusReceive(bus, midi_package, 0);
#endif
	    }

	    if( seq_midi_in_options[bus].MODE_PLAY && seq_record_options.FWD_MIDI && !seq_record_state.ENABLED ) {
	      SEQ_LIVE_PlayEvent(SEQ_UI_VisibleTrackGet(), midi_package);
	    }
	      
	    status |= 1;
	  }
	} else {
	  if( !should_be_recorded &&
	      midi_package.note >= seq_midi_in_lower[bus] &&
	      (!seq_midi_in_upper[bus] || midi_package.note <= seq_midi_in_upper[bus]) ) {

	    if( !seq_midi_in_options[bus].MODE_PLAY ) {
#if 0
	      // octave normalisation - too complicated for normal users...
	      mios32_midi_package_t p = midi_package;
	      if( seq_midi_in_lower[bus] ) { // normalize to first octave
		int normalized_note = 0x30 + p.note - ((int)seq_midi_in_lower[bus]/12)*12;
		// ensure that note is in the 0..127 range
		normalized_note = SEQ_CORE_TrimNote(normalized_note, 0, 127);
		p.note = normalized_note;
	      }
	      SEQ_MIDI_IN_BusReceive(bus, p, 0);
#else
	      SEQ_MIDI_IN_BusReceive(bus, midi_package, 0);
#endif
	    }

	    if( seq_midi_in_options[bus].MODE_PLAY && seq_record_options.FWD_MIDI && !seq_record_state.ENABLED ) {
	      SEQ_LIVE_PlayEvent(SEQ_UI_VisibleTrackGet(), midi_package);
	    }

	    status |= 1;
	  }
	}
	break;

      case CC:
	if( !should_be_recorded ) {

	  if( !seq_midi_in_options[bus].MODE_PLAY ) {
	    SEQ_MIDI_IN_BusReceive(bus, midi_package, 0);
	  }

	  if( seq_midi_in_options[bus].MODE_PLAY && seq_record_options.FWD_MIDI && !seq_record_state.ENABLED ) {
	    SEQ_LIVE_PlayEvent(SEQ_UI_VisibleTrackGet(), midi_package);
	  }

	}
	status |= 1;
	break;

      default:
	if( seq_midi_in_options[bus].MODE_PLAY && seq_record_options.FWD_MIDI && !seq_record_state.ENABLED ) {
	  SEQ_LIVE_PlayEvent(SEQ_UI_VisibleTrackGet(), midi_package);
	}
      }
    }
  }


  // record function
  if( should_be_recorded ) {
    SEQ_RECORD_Receive(midi_package, SEQ_UI_VisibleTrackGet());
    return 0; // stop processing
  }

  // External Control
  if( ((seq_midi_in_ext_ctrl_port == 0xff || port == seq_midi_in_ext_ctrl_port) &&
       (seq_midi_in_ext_ctrl_channel == 17 || midi_package.chn == (seq_midi_in_ext_ctrl_channel-1))) ) {

    switch( midi_package.event ) {
      case CC: 
	MUTEX_MIDIIN_TAKE;
	SEQ_MIDI_IN_Receive_ExtCtrlCC(midi_package.cc_number, midi_package.value);
	MUTEX_MIDIIN_GIVE;
	break;

      case ProgramChange:
	MUTEX_MIDIIN_TAKE;
	SEQ_MIDI_IN_Receive_ExtCtrlPC(midi_package.program_change);
	MUTEX_MIDIIN_GIVE;
	break;
    }
  }


  // Section Changer
  if( (seq_midi_in_sect_port && port == seq_midi_in_sect_port &&
       (seq_midi_in_sect_channel == 17 || midi_package.chn == (seq_midi_in_sect_channel-1))) ) {
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

#if PATCH_CHANGER_ENABLED
  // Patch Changer (currently assigned to channel+1)
  if( (seq_midi_in_sect_port && port == seq_midi_in_sect_port &&
       (seq_midi_in_sect_channel == 17 || midi_package.chn == (seq_midi_in_sect_channel))) ) {
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
s32 SEQ_MIDI_IN_BusReceive(u8 bus, mios32_midi_package_t midi_package, u8 from_loopback_port)
{
  s32 status = 0;

  if( bus >= SEQ_MIDI_IN_NUM_BUSSES )
    return -1;

  if( from_loopback_port ) {
    last_bus_received_from_loopback |= (1 << bus);
  } else if( (last_bus_received_from_loopback & (1 << bus)) ) {
    last_bus_received_from_loopback &= ~(1 << bus);
    SEQ_MIDI_IN_ResetSingleTransArpStacks(bus);
  }

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
    else {
      // MIDI Remote Function?
      if( seq_hwcfg_midi_remote.cc && seq_hwcfg_midi_remote.cc == midi_package.cc_number ) {
	remote_active = midi_package.value >= 0x40;
	MUTEX_MIDIOUT_TAKE;
	DEBUG_MSG("[SEQ_MIDI_IN] MIDI remote access %sactivated", remote_active ? "" : "de");
	MUTEX_MIDIOUT_GIVE;
      }
    }
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
    if( bus_notestack[bus][BUS_NOTESTACK_TRANSPOSER].len == 1 )
      transposer_hold_first_note[bus] = n->note_items[0].note;
    transposer_hold_last_note[bus] = n->note_items[0].note;

    // will only be used for Bus1 and if enabled in OPT menu
    if( bus == 0
#ifdef MBSEQV4L
	&& seq_core_global_transpose_enabled
#endif
	) {
      seq_core_keyb_scale_root = note % 12;
    }
  } else { // Note Off
    if( NOTESTACK_Pop(n, note) > 0 && n->len ) {
      transposer_hold_last_note[bus] = n->note_items[0].note;
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
      portENTER_CRITICAL();
      u8 track;
      for(track=0; track<SEQ_CORE_NUM_TRACKS; ++track)
	if( seq_cc_trk[track].mode.RESTART ) {
	  seq_core_trk[track].state.POS_RESET = 1;
	}
      portEXIT_CRITICAL();

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
#if PATCH_CHANGER_ENABLED
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
#endif

/////////////////////////////////////////////////////////////////////////////
// CC has been received over external control port and channel
/////////////////////////////////////////////////////////////////////////////
static s32 SEQ_MIDI_IN_Receive_ExtCtrlCC(u8 cc, u8 value)
{
  static u8 nrpn_lsb = 0;
  static u8 nrpn_msb = 0;

  // NRPN handling
  switch( cc ) {
  case 0x62: // NRPN LSB (selects parameter)
    nrpn_lsb = value;
    break;

  case 0x63: // NRPN MSB (selects track, if >= SEQ_CORE_NUM_TRACKS, take the currently visible track)
    nrpn_msb = value;
    break;

  case 0x06: // NRPN Value MSB (sets parameter)
    if( nrpn_msb < SEQ_CORE_NUM_TRACKS && seq_midi_in_ext_ctrl_asg[SEQ_MIDI_IN_EXT_CTRL_NRPN_ENABLED] ) {
      u8 track = nrpn_msb;
      if( track >= SEQ_CORE_NUM_TRACKS )
	track = SEQ_UI_VisibleTrackGet();
      SEQ_CC_MIDI_Set(track, nrpn_lsb, value);
    }
    break;
  }


  // search for matching CCs
  int i;
  u8 *asg = (u8 *)&seq_midi_in_ext_ctrl_asg[0];
  for(i=0; i<SEQ_MIDI_IN_EXT_CTRL_NUM_IX_CC; ++i, ++asg) {
    if( cc == *asg ) {
      switch( i ) {
      case SEQ_MIDI_IN_EXT_CTRL_MORPH:
	SEQ_MORPH_ValueSet(value);
	break;

      case SEQ_MIDI_IN_EXT_CTRL_SCALE:
	seq_core_global_scale = value;
	break;

      case SEQ_MIDI_IN_EXT_CTRL_PATTERN_G1:
      case SEQ_MIDI_IN_EXT_CTRL_PATTERN_G2:
      case SEQ_MIDI_IN_EXT_CTRL_PATTERN_G3:
      case SEQ_MIDI_IN_EXT_CTRL_PATTERN_G4: {
	u8 group = i - SEQ_MIDI_IN_EXT_CTRL_PATTERN_G1;
	portENTER_CRITICAL();
	seq_pattern_t pattern = seq_pattern[group];
	if( value < SEQ_FILE_B_NumPatterns(pattern.bank) ) {
	  pattern.pattern = value;
	  pattern.DISABLED = 0;
	  pattern.SYNCHED = 0;
	  SEQ_PATTERN_Change(group, pattern, 0);
	}
	portEXIT_CRITICAL();
      } break;

      case SEQ_MIDI_IN_EXT_CTRL_SONG: {
	if( value < SEQ_SONG_NUM && value != SEQ_SONG_NumGet() ) {
	  SEQ_SONG_Load(value);
	}
      } break;

      case SEQ_MIDI_IN_EXT_CTRL_PHRASE: {
	if( value <= 15 ) {
	  u8 song_pos = value << 3;
	  SEQ_SONG_PosSet(song_pos);
	  SEQ_SONG_FetchPos(0, 0);
	  ui_song_edit_pos = song_pos;
	}
      } break;

      case SEQ_MIDI_IN_EXT_CTRL_MIXER_MAP: {
	SEQ_MIXER_Load(value);
	SEQ_MIXER_SendAll();	
      } break;

      case SEQ_MIDI_IN_EXT_CTRL_BANK_G1:
      case SEQ_MIDI_IN_EXT_CTRL_BANK_G2:
      case SEQ_MIDI_IN_EXT_CTRL_BANK_G3:
      case SEQ_MIDI_IN_EXT_CTRL_BANK_G4: {
	u8 group = i - SEQ_MIDI_IN_EXT_CTRL_BANK_G1;
	portENTER_CRITICAL();
	seq_pattern_t pattern = seq_pattern[group];
	if( value < SEQ_FILE_B_NUM_BANKS ) {
	  pattern.bank = value;
	  pattern.DISABLED = 0;
	  pattern.SYNCHED = 0;
	  SEQ_PATTERN_Change(group, pattern, 0);
	}
	portEXIT_CRITICAL();
      } break;

      case SEQ_MIDI_IN_EXT_CTRL_ALL_NOTES_OFF: {
	if( value == 0 ) {
	  SEQ_MIDI_IN_ResetAllStacks();
	  SEQ_CV_ResetAllChannels();
	}
      } break;

      case SEQ_MIDI_IN_EXT_CTRL_PLAY: {
	if( value ) {
	  if( SEQ_BPM_IsRunning() ) {
	    SEQ_UI_Button_Stop(0); // depressed
	    SEQ_UI_Button_Stop(1); // pressed
	  } else {
	    SEQ_UI_Button_Play(0); // depressed
	    SEQ_UI_Button_Play(1); // pressed
	  }
	}
      } break;

      case SEQ_MIDI_IN_EXT_CTRL_RECORD: {
	if( value ) {
	  SEQ_UI_Button_Record(0); // depressed
	  SEQ_UI_Button_Record(1); // pressed
	}
      } break;
      }
    }
  }

  {
    int mute_cc = seq_midi_in_ext_ctrl_asg[SEQ_MIDI_IN_EXT_CTRL_MUTES];
    if( mute_cc < 128 && cc >= mute_cc && cc < (mute_cc+SEQ_CORE_NUM_TRACKS) ) {
      u8 track = cc - mute_cc;
      portENTER_CRITICAL();
      if( value >= 64 ) {
	seq_core_trk_muted |= (1 << track);
      } else {
	seq_core_trk_muted &= ~(1 << track);
      }
      portEXIT_CRITICAL();      
    }
  }

  {
    int step_cc = seq_midi_in_ext_ctrl_asg[SEQ_MIDI_IN_EXT_CTRL_STEPS];
    if( step_cc < 128 && cc >= step_cc && cc < (step_cc+SEQ_CORE_NUM_TRACKS) ) {
      u8 track = cc - step_cc;
      portENTER_CRITICAL();
      SEQ_CORE_SetTrkPos(track, value, 0); // don't scale, otherwise manual trigger won't work with satisfying results...
      portEXIT_CRITICAL();      
    }
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// PC has been received over external control port and channel
/////////////////////////////////////////////////////////////////////////////
static s32 SEQ_MIDI_IN_Receive_ExtCtrlPC(u8 value)
{
  switch( seq_midi_in_ext_ctrl_asg[SEQ_MIDI_IN_EXT_CTRL_PC_MODE] ) {
  case SEQ_MIDI_IN_EXT_CTRL_PC_MODE_OFF:
    break; // do nothing...

  case SEQ_MIDI_IN_EXT_CTRL_PC_MODE_PATTERNS: {
    portENTER_CRITICAL();
    // switch all patterns (don't switch banks!)
    int group;
    for(group=0; group<SEQ_CORE_NUM_GROUPS; ++group) {
      seq_pattern_t pattern = seq_pattern[group];
      if( value < SEQ_FILE_B_NumPatterns(pattern.bank) ) {
	pattern.pattern = value;
	pattern.DISABLED = 0;
	pattern.SYNCHED = 0;
	SEQ_PATTERN_Change(group, pattern, 0);
      }
    }
    portEXIT_CRITICAL();
  } break;
    
  case SEQ_MIDI_IN_EXT_CTRL_PC_MODE_SONG:
    if( value < SEQ_SONG_NUM && value != SEQ_SONG_NumGet() ) {
      SEQ_SONG_Load(value);
    }
    break;

  case SEQ_MIDI_IN_EXT_CTRL_PC_MODE_PHRASE:
    if( value <= 15 ) {
      u8 song_pos = value << 3;
      SEQ_SONG_PosSet(song_pos);
      SEQ_SONG_FetchPos(0, 0);
      ui_song_edit_pos = song_pos;
    }
    break;
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Returns the note for transpose mode
// if -1, the stack is empty
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_MIDI_IN_TransposerNoteGet(u8 bus, u8 hold, u8 first_note)
{
  u8 len = bus_notestack[bus][BUS_NOTESTACK_TRANSPOSER].len;
  if( first_note ) {
    if( hold )
      return transposer_hold_first_note[bus];

    return len ? bus_notestack[bus][BUS_NOTESTACK_TRANSPOSER].note_items[len-1].note : -1;
  } else {
    if( hold )
      return transposer_hold_last_note[bus];

    return len ? bus_notestack[bus][BUS_NOTESTACK_TRANSPOSER].note_items[0].note : -1;
  }
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
