// $Id$
/*
 * Groove Functions
 *
 * TODO: customized groove templates stored on SD Card
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

#include "seq_groove.h"
#include "seq_cc.h"
#include "seq_layer.h"


/////////////////////////////////////////////////////////////////////////////
// Local types
/////////////////////////////////////////////////////////////////////////////

typedef union {
  struct {
    unsigned s1:4;
    unsigned s2:4;
    unsigned s3:4;
    unsigned s4:4;
    unsigned s5:4;
    unsigned s6:4;
    unsigned s7:4;
    unsigned s8:4;
    unsigned s9:4;
    unsigned s10:4;
    unsigned s11:4;
    unsigned s12:4;
    unsigned s13:4;
    unsigned s14:4;
    unsigned s15:4;
    unsigned s16:4;
  };

  struct {
    u8 steps[8]; // unfortunately "unsigned steps:4[8]" doesn't work
  };
} seq_groove_steps_t;


typedef struct {
  char name[13];
  seq_groove_steps_t delay;
  seq_groove_steps_t length;
  seq_groove_steps_t velocity;
} seq_groove_entry_t;


/////////////////////////////////////////////////////////////////////////////
// Local definitions to improve readability
/////////////////////////////////////////////////////////////////////////////

#define D__0    0
#define D__1    1
#define D__2    2
#define D__3    3
#define D__4    4
#define D__5    5
#define D__6    6
#define D__7    7
#define D__8    8
#define D__9    9
#define D_10   10
#define D_11   11
#define D_12   12
#define D_13   13
#define D_14   14
#define DVAR   15

#define V020    0
#define V030    1
#define V040    2
#define V050    3
#define V060    4
#define V070    5
#define V080    6
#define V090    7
#define V100    8
#define V110    9
#define V120   10
#define V130   11
#define V140   12
#define V150   13
#define V160   14
#define VVAR   15

#define L020    0
#define L030    1
#define L040    2
#define L050    3
#define L060    4
#define L070    5
#define L080    6
#define L090    7
#define L100    8
#define L110    9
#define L120   10
#define L130   11
#define L140   12
#define L150   13
#define L160   14
#define LVAR   15


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

static const seq_groove_entry_t seq_groove_table[] = {
  { " -- off --  ", // dummy entry, changes here have no effect!
    { D__0, D__0, D__0, D__0, D__0, D__0, D__0, D__0, D__0, D__0, D__0, D__0, D__0, D__0, D__0, D__0 },
    { L100, L100, L100, L100, L100, L100, L100, L100, L100, L100, L100, L100, L100, L100, L100, L100 },
    { V100, V100, V100, V100, V100, V100, V100, V100, V100, V100, V100, V100, V100, V100, V100, V100 }
  },

  { "  Shuffle   ",
    { D__0, DVAR, D__0, DVAR, D__0, DVAR, D__0, DVAR, D__0, DVAR, D__0, DVAR, D__0, DVAR, D__0, DVAR },
    { L100, L100, L100, L100, L100, L100, L100, L100, L100, L100, L100, L100, L100, L100, L100, L100 },
    { V100, V100, V100, V100, V100, V100, V100, V100, V100, V100, V100, V100, V100, V100, V100, V100 }
  },

  { "Inv. Shuffle",
    { DVAR, D__0, DVAR, D__0, DVAR, D__0, DVAR, D__0, DVAR, D__0, DVAR, D__0, DVAR, D__0, DVAR, D__0 },
    { L100, L100, L100, L100, L100, L100, L100, L100, L100, L100, L100, L100, L100, L100, L100, L100 },
    { V100, V100, V100, V100, V100, V100, V100, V100, V100, V100, V100, V100, V100, V100, V100, V100 }
  },

  { "  Shuffle2  ",
    { D__0, D__8, D__0, DVAR, D__0, D__8, D__0, DVAR, D__0, D__8, D__0, DVAR, D__0, D__8, D__0, DVAR },
    { L100, L100, L100, L100, L100, L100, L100, L100, L100, L100, L100, L100, L100, L100, L100, L100 },
    { V100, V120, V100, V080, V100, V120, V100, V080, V100, V120, V100, V080, V100, V120, V100, V080 }
  },

  { "  Shuffle3  ",
    { D__0, D__8, D__4, DVAR, D__0, D__8, D__4, DVAR, D__0, D__8, D__4, DVAR, D__0, D__8, D__4, DVAR },
    { L100, L100, L100, L100, L100, L100, L100, L100, L100, L100, L100, L100, L100, L100, L100, L100 },
    { V100, V080, V060, V080, V100, V080, V060, V080, V100, V080, V060, V080, V100, V080, V060, V080 }
  },

  { "  Shuffle4  ",
    { D__0, D__8, D__0, D__8, D__0, D__8, D__0, D__8, D__0, D__8, D__0, D__8, D__0, D__8, D__0, D__8 },
    { L100, L100, L100, L100, L100, L100, L100, L100, L100, L100, L100, L100, L100, L100, L100, L100 },
    { V100, DVAR, V100, DVAR, V100, DVAR, V100, DVAR, V100, DVAR, V100, DVAR, V100, DVAR, V100, DVAR }
  },

  { "  Shuffle5  ",
    { D__0, D__8, D__0, D__8, D__0, D__8, D__0, D__8, D__0, D__8, D__0, D__8, D__0, D__8, D__0, D__8 },
    { DVAR, L100, DVAR, L100, DVAR, L100, DVAR, L100, DVAR, L100, DVAR, L100, DVAR, L100, DVAR, L100 },
    { V100, V100, V100, V100, V100, V100, V100, V100, V100, V100, V100, V100, V100, V100, V100, V100 }
  }
};


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_GROOVE_Init(u32 mode)
{
  // here we will read customized groove pattern later

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// returns number of available grooves
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_GROOVE_NumGet(void)
{
  return sizeof(seq_groove_table)/sizeof(seq_groove_entry_t);
}


/////////////////////////////////////////////////////////////////////////////
// returns pointer to the name of a groove
// Length: 12 characters + zero terminator
/////////////////////////////////////////////////////////////////////////////
char *SEQ_GROOVE_NameGet(u8 groove)
{
  if( groove >= SEQ_GROOVE_NumGet() )
    return "Invld Groove";

  return seq_groove_table[groove].name;
}


/////////////////////////////////////////////////////////////////////////////
// returns 0..95 for the number of ticks at which the step should be delayed
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_GROOVE_DelayGet(u8 track, u8 step)
{
  seq_cc_trk_t *tcc = &seq_cc_trk[track];
  u8 groove = tcc->groove_style;

  // check if within allowed range
  if( !groove || groove >= SEQ_GROOVE_NumGet() )
    return 0; // no groove

  // 16 steps are supported
  step %= 16;

  // get delay value
  u8 delay = seq_groove_table[groove].delay.steps[step>>1];
  delay = (step & 1) ? ((delay >> 4) & 0x0f) : (delay & 0x0f);

  if( delay >= 15 )
    delay = tcc->groove_value;

  return 4 * delay; // currently 0, 4, 8, ... 60
}


/////////////////////////////////////////////////////////////////////////////
// modifies a MIDI event depending on selected groove
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_GROOVE_Event(u8 track, u8 step, seq_layer_evnt_t *e)
{
  seq_cc_trk_t *tcc = &seq_cc_trk[track];
  u8 groove = tcc->groove_style;

  // check if within allowed range
  if( !groove || groove >= SEQ_GROOVE_NumGet() )
    return 0; // no groove

  // 16 steps are supported
  step %= 16;

  // get velocity modifier
  u8 velocity = seq_groove_table[groove].velocity.steps[step>>1];
  velocity = (step & 1) ? ((velocity >> 4) & 0x0f) : (velocity & 0x0f);

  if( velocity >= 15 )
    velocity = tcc->groove_value;

  if( velocity != 8 ) { // 8 == 100%
    u16 value = ((20 + 10*velocity) * e->midi_package.velocity) / 100;
    e->midi_package.velocity = (value >= 127) ? 127 : value;
  }

  // get gatelength modifier
  u8 length = seq_groove_table[groove].length.steps[step>>1];
  length = (step & 1) ? ((length >> 4) & 0x0f) : (length & 0x0f);

  if( length >= 15 )
    length = tcc->groove_value;

  if( length != 8 ) { // 8 == 100%
    u16 value = ((20 + 10*length) * e->len) / 100;
    e->len = (value >= 95) ? 95 : value;
  }

  return 0; // no error
}

