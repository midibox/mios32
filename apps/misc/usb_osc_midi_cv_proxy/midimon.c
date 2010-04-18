// $Id$
/*
 * MIDI Monitor functions
 *
 * ==========================================================================
 *
 *  Copyright (C) 2009 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

/////////////////////////////////////////////////////////////////////////////
// Include files
/////////////////////////////////////////////////////////////////////////////
#include <mios32.h>
#include <string.h>

#include "midimon.h"


/////////////////////////////////////////////////////////////////////////////
// Local defines
/////////////////////////////////////////////////////////////////////////////

// shortcut for printing a message on MIOS Terminal
// could also be replaced by "printf" if source code used in a different environment
#define MSG MIOS32_MIDI_SendDebugMessage

// to determine BPM
#define NUM_TEMPO_SAMPLES (6+1) // to display BPM correctly after one 16th step

#define NUM_TEMPO_PORTS 4 // for USB0/1 and UART0/1 separately

/////////////////////////////////////////////////////////////////////////////
// Local structures
/////////////////////////////////////////////////////////////////////////////

typedef union {
  struct {
    unsigned long long ALL:64;
  };

  struct {
    u32 measure;
    u8  beat;
    u8  step;
    s8  subtick;
  };
} midi_clk_pos_t;


typedef union {
  struct {
    unsigned long long ALL:64;
  };

  struct {
    u8 type;
    u8 hours;
    u8 minutes;
    u8 seconds;
    u8 frames;
  };
} mtc_pos_t;


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

static const char note_name[12][3] = { "C-", "C#", "D-", "D#", "E-", "F-", "F#", "G-", "G#", "A-", "A#", "B-" };

static midi_clk_pos_t midi_clk_pos[NUM_TEMPO_PORTS];
static u32 tempo_samples[NUM_TEMPO_PORTS][NUM_TEMPO_SAMPLES];
static u8 tempo_sample_pos[NUM_TEMPO_PORTS];

static mtc_pos_t mtc_pos[NUM_TEMPO_PORTS];

static u8 midimon_active = 0;
static u8 filter_active = 1;
static u8 tempo_active = 0;


/////////////////////////////////////////////////////////////////////////////
// Initialize the monitor
/////////////////////////////////////////////////////////////////////////////
s32 MIDIMON_Init(u32 mode)
{
  int tempo_port_ix;
  int i;

  if( mode > 0 )
    return -1; // only mode 0 supported yet

  for(tempo_port_ix=0; tempo_port_ix<NUM_TEMPO_PORTS; ++tempo_port_ix) {
    midi_clk_pos[tempo_port_ix].ALL = 0;
    midi_clk_pos[tempo_port_ix].subtick = -1;

    for(i=0; i<NUM_TEMPO_SAMPLES; ++i)
      tempo_samples[tempo_port_ix][i] = 0;
    tempo_sample_pos[tempo_port_ix] = 0;

    mtc_pos[tempo_port_ix].ALL = 0;
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Init function for presets (read before MIDIMON_Init()
/////////////////////////////////////////////////////////////////////////////
s32 MIDIMON_InitFromPresets(u8 _midimon_active, u8 _filter_active, u8 _tempo_active)
{
  midimon_active = _midimon_active;
  filter_active = _filter_active;
  tempo_active = _tempo_active;

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Get/Set functions
/////////////////////////////////////////////////////////////////////////////
s32 MIDIMON_ActiveSet(u8 active)
{
  midimon_active = active;
}

s32 MIDIMON_ActiveGet(void)
{
  return midimon_active;
}


s32 MIDIMON_FilterActiveSet(u8 active)
{
  filter_active = active;
}

s32 MIDIMON_FilterActiveGet(void)
{
  return filter_active;
}


s32 MIDIMON_TempoActiveSet(u8 active)
{
  tempo_active = active;
}

s32 MIDIMON_TempoActiveGet(void)
{
  return tempo_active;
}


/////////////////////////////////////////////////////////////////////////////
// MIDI Packet Receiver function
/////////////////////////////////////////////////////////////////////////////
s32 MIDIMON_Receive(mios32_midi_port_t port, mios32_midi_package_t package, u32 timestamp, u8 filter_sysex_message)
{
  char pre_str[32];
  u8 display_midi_clk = 0;
  u8 display_mtc = 0;

  if( !midimon_active )
    return 0; // MIDImon mode not enabled

  // derive port name and build pre-string
  u8 port_ix = port & 0x0f;
  char port_ix_name = (port_ix < 9) ? ('1'+port_ix) : ('A'+(port_ix-9));
  switch( port & 0xf0 ) {
    case USB0:  sprintf(pre_str, "[USB%c]", port_ix_name); break;
    case UART0: sprintf(pre_str, "[IN%c ]", port_ix_name); break;
    case IIC0:  sprintf(pre_str, "[IIC%c]", port_ix_name); break;
    default:    sprintf(pre_str, "[P.%02X ]", port);
  }

  // for separate MIDI clock/MTC measurements
  int tempo_port_ix = -1;
  switch( port ) {
    case USB0: tempo_port_ix = 0; break;
    case USB1: tempo_port_ix = 1; break;
    case UART0: tempo_port_ix = 2; break;
    case UART1: tempo_port_ix = 3; break;
  }

  // branch depending on package type
  u8 msg_sent = 0;
  switch( package.type ) {
    case 0x2:  // Two-byte System Common messages like MTC, SongSelect, etc.
      if( package.evnt0 == 0xf1 ) {
	if( !filter_active ) {
	  switch( package.evnt1 & 0xf0 ) {
	    case 0x00: MSG("%s MTC Frame   Low:  %X\n", pre_str, package.evnt1 & 0xf); break;
	    case 0x10: MSG("%s MTC Frame   High: %X\n", pre_str, package.evnt1 & 0xf); break;
	    case 0x20: MSG("%s MTC Seconds Low:  %X\n", pre_str, package.evnt1 & 0xf); break;
	    case 0x30: MSG("%s MTC Seconds High: %X\n", pre_str, package.evnt1 & 0xf); break;
	    case 0x40: MSG("%s MTC Minutes Low:  %X\n", pre_str, package.evnt1 & 0xf); break;
	    case 0x50: MSG("%s MTC Minutes High: %X\n", pre_str, package.evnt1 & 0xf); break;
	    case 0x60: MSG("%s MTC Hours   Low:  %X\n", pre_str, package.evnt1 & 0xf); break;
	    case 0x70: MSG("%s MTC Hours   High: %X (SMPTE Type: %d)\n", 
			   pre_str, package.evnt1 & 0x1, (package.evnt1>>1) & 0x7); break;
	    default:
	      MSG("%s MTC Invalid: %02X %02X %02X\n", pre_str, package.evnt0, package.evnt1, package.evnt2);
	  }
	}
	msg_sent = 1;
      } else if( package.evnt0 == 0xf3 ) {
	MSG("%s Song Number #%d\n", pre_str, package.evnt1);
	msg_sent = 1;
      }
      break;

    case 0x3:  // Three-byte System Common messages like SPP, etc.
      if( package.evnt0 == 0xf2 ) {
	u16 song_pos = package.evnt1 | (package.evnt2 >> 7);
	u8 step = song_pos % 4;
	u8 beat = (song_pos / 4) % 4;
	u32 measure = song_pos / 16;

	MSG("%s Song Position %d.%d.%d\n", pre_str, measure+1, beat+1, step+1);
	msg_sent = 1;

	if( tempo_port_ix >= 0 ) {
	  midi_clk_pos_t *mcp = (midi_clk_pos_t *)&midi_clk_pos[tempo_port_ix];

	  mcp->subtick = -1; // ensure that next clock starts with 0
	  mcp->step = step;
	  mcp->beat = beat;
	  mcp->measure = measure;
	}
      }
      break;

    case 0x4:  // SysEx starts or continues (3 bytes)
    case 0x7:  // SysEx ends with following three bytes
      if( !filter_sysex_message )
	MSG("%s SysEx: %02X %02X %02X\n", pre_str, package.evnt0, package.evnt1, package.evnt2);
      msg_sent = 1;
      break;

    case 0x5: // Single-byte System Common Message or SysEx ends with following single bytes
    case 0xf: // Single Byte
      switch( package.evnt0 ) {
        case 0xf6: MSG("%s Tune Request (F6)\n", pre_str); break;
        case 0xf7: if( !filter_sysex_message ) { MSG("%s SysEx End (F7)\n", pre_str); } break;
        case 0xf8: 
	  if( !filter_active ) {
	    MSG("%s MIDI Clock (F8)\n", pre_str);
	  }

	  if( tempo_port_ix >= 0 ) {
	    midi_clk_pos_t *mcp = (midi_clk_pos_t *)&midi_clk_pos[tempo_port_ix];
	    if( ++mcp->subtick >= 6 ) {
	      mcp->subtick = 0;
	      if( ++mcp->step >= 4 ) {
		mcp->step = 0;
		if( ++mcp->beat >= 4 ) {
		  mcp->beat = 0;
		  ++mcp->measure;
		}
	      }
	    }

	    // for tempo measurements
	    tempo_samples[tempo_port_ix][tempo_sample_pos[tempo_port_ix]] = timestamp;
	    if( ++tempo_sample_pos[tempo_port_ix] >= NUM_TEMPO_SAMPLES )
	      tempo_sample_pos[tempo_port_ix] = 0;

	    // will happen on -1 -> 0 (after FA has been received) and after common overrun
	    if( mcp->subtick == 0 )
	      display_midi_clk = 1;
	  }

	  break;

        case 0xf9: MSG("%s MIDI Tick (F9)\n", pre_str); break;
        case 0xfa:
	  MSG("%s MIDI Clock Start (FA)\n", pre_str);
	  if( tempo_port_ix >= 0 ) {
	    midi_clk_pos_t *mcp = (midi_clk_pos_t *)&midi_clk_pos[tempo_port_ix];
	    mcp->ALL = 0;
	    mcp->subtick = -1; // ensure that next clock starts with 0
	  }
	  break;
        case 0xfb: MSG("%s MIDI Clock Continue (FB)\n", pre_str); break;
        case 0xfc: MSG("%s MIDI Clock Stop (FC)\n", pre_str); break;
        case 0xfd: MSG("%s Inspecified Realtime Event (FD)\n", pre_str); break;
        case 0xfe: if( !filter_active ) { MSG("%s Active Sense (FE)\n", pre_str); } break;
        case 0xff: MSG("%s Reset (FF)\n", pre_str); break;
        default:
	  if( package.type == 0xf )
	    MSG("%s Single-Byte Package: %02X\n", pre_str, package.evnt0);
	  else
	    MSG("%s Invalid SysEx Single-Byte Event (%02X)\n", pre_str, package.evnt0);
      }
      msg_sent = 1;
      break;

    case 0x6:  // SysEx ends with following two bytes
      if( !filter_sysex_message )
	MSG("%s SysEx: %02X %02X\n", pre_str, package.evnt0, package.evnt1);
      msg_sent = 1;
      break;

    case 0x8: // Note Off
      MSG("%s Chn%2d  Note Off %s%d  V:%3d\n",
	  pre_str, (package.evnt0 & 0xf)+1, note_name[package.evnt1%12], (int)(package.evnt1/12)-2, package.evnt2);
      msg_sent = 1;
      break;
      
    case 0x9: // Note On
      MSG("%s Chn%2d  Note On  %s%d  V:%3d\n",
	  pre_str, (package.evnt0 & 0xf)+1, note_name[package.evnt1%12], (int)(package.evnt1/12)-2, package.evnt2);
      msg_sent = 1;
      break;
      
    case 0xa: // Poly Aftertouch
      MSG("%s Chn%2d  Poly Aftertouch %s%d  V:%3d\n",
	  pre_str, (package.evnt0 & 0xf)+1, note_name[package.evnt1%12], (int)(package.evnt1/12)-2, package.evnt2);
      msg_sent = 1;
      break;
      
    case 0xb: // CC
      MSG("%s Chn%2d  CC#%3d  V:%3d\n",
	  pre_str, (package.evnt0 & 0xf)+1, package.evnt1, package.evnt2);
      msg_sent = 1;
      break;
      
    case 0xc: // Program Change
      MSG("%s Chn%2d  Program Change #%3d\n",
	  pre_str, (package.evnt0 & 0xf)+1, package.evnt1);
      msg_sent = 1;
      break;
      
    case 0xd: // Channel Aftertouch
      MSG("%s Chn%2d  Channel Aftertouch %s%d\n",
	  pre_str, (package.evnt0 & 0xf)+1, note_name[package.evnt1%12], (int)(package.evnt1/12)-2);
      msg_sent = 1;
      break;
  }

  // unspecified or invalid packages
  if( !msg_sent ) {
    MSG("%s Invalid Package (Type %d: %02X %02X %02X)\n",
	pre_str, package.type, package.evnt0, package.evnt1, package.evnt2);
  }

  if( display_midi_clk && tempo_active && tempo_port_ix >= 0 ) {
    midi_clk_pos_t *mcp = (midi_clk_pos_t *)&midi_clk_pos[tempo_port_ix];

    // calculate tempo based on last samples
    // last samples have higher weight (FIT algorithm)
    int i;
    int valid = 1;

    int pos = tempo_sample_pos[tempo_port_ix];
    u32 sample_timestamp = tempo_samples[tempo_port_ix][pos];

    u32 filter_value = 0;
    int weight = 100;
    int divisor = 0;
    for(i=0; i<(NUM_TEMPO_SAMPLES-1); ++i) {
      if( ++pos >= NUM_TEMPO_SAMPLES )
	pos = 0;      

      u32 delta = tempo_samples[tempo_port_ix][pos] - sample_timestamp;
#if 0
      MSG(">>>Delta %d: %u\n", i, delta);
#endif

      // more than 1000 mS between two ticks would result into 2.5 BPM
      // we assume that measuring results are invalid!
      if( delta >= 1000 )
	valid = 0;

      filter_value += weight * delta;
      divisor += weight;
      weight = (weight*100) / 110; // if divided by 100, we will get an unweighted mean value
      sample_timestamp = tempo_samples[tempo_port_ix][pos];
    }

    if( !valid ) {
      MSG("%s MIDI Clock: %3d.%d.%d  (???.? BPM)\n", 
	  pre_str,
	  mcp->measure+1, mcp->beat+1, mcp->step+1);
    } else {
      float mean_value = (float)filter_value / (float)divisor;
      float bpm = 60.0 / ((mean_value/1000.0) * 24.0);

      // print MIDI Clock position and tempo
      MSG("%s MIDI Clock: %3d.%d.%d  (%3d.%d BPM)\n", 
	  pre_str,
	  mcp->measure+1, mcp->beat+1, mcp->step+1,
	  (int)bpm, (int)(10.0*bpm) % 10);
    }
  }

  return 0; // no error
}
