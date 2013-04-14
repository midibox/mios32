// $Id$
/*
 * Chord Functions
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

#include "seq_chord.h"
#include "seq_core.h"


/////////////////////////////////////////////////////////////////////////////
// Local types
/////////////////////////////////////////////////////////////////////////////

typedef struct {
  s8   keys[4];
  char name[7];
} seq_chord_entry_t;


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

  // note: chords are used together with the forced-to-scale feature in seq_scale.c
  // if no key should be played, add -1
static const seq_chord_entry_t seq_chord_table[] = {
  // 1   2   3   4     <----> (6 chars!)
  {{ 0,  4,  7, -1 }, "Maj.I " },
  {{ 4,  7, 12, -1 }, "Maj.II" },
  {{ 7, 12, 16, -1 }, "MajIII" },
  {{ 0, -1, -1, -1 }, "Root  " },
  {{ 4, -1, -1, -1 }, "3rd   " },
  {{ 7, -1, -1, -1 }, "5th   " },
  {{ 0,  4, -1, -1 }, "R.+3rd" },
  {{ 0,  7, -1, -1 }, "R.+5th" },
  {{ 0,  4,  7,  9 }, "Maj.6 " },
  {{ 0,  4,  7, 11 }, "Maj.7 " },
  {{ 0,  4,  7, 12 }, "Maj.8 " },
  {{ 0,  4,  7, 14 }, "Maj.9 " },
  {{ 0,  7, 12, 16 }, "Maj.10" },
  {{ 0,  7, 12, 19 }, "Maj.12" },
  {{ 0,  5,  7, -1 }, "Sus.4 " },
  {{ 0,  4,  8, -1 }, "Maj.+ " },

  {{ 0,  3,  7, -1 }, "Min.I " },
  {{ 3,  7, 12, -1 }, "Min.II" },
  {{ 7, 12, 15, -1 }, "MinIII" },
  {{ 0, -1, -1, -1 }, "Root  " },
  {{ 3, -1, -1, -1 }, "3rdMin" },
  {{ 7, -1, -1, -1 }, "5th   " },
  {{ 0,  3, -1, -1 }, "R.+3rd" },
  {{ 0,  7, -1, -1 }, "R.+5th" },
  {{ 0,  3,  7,  9 }, "Min.6 " },
  {{ 0,  3,  7, 11 }, "Min.7 " },
  {{ 0,  3,  7, 12 }, "Min.8 " },
  {{ 0,  3,  7, 14 }, "Min.9 " },
  {{ 0,  7, 12, 16 }, "Min.10" },
  {{ 0,  7, 12, 18 }, "Min.12" },
  {{ 0,  3,  6,  9 }, "Co7   " },
  {{ 0,  3,  8, -1 }, "Min.+ " }
};


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_CHORD_Init(u32 mode)
{
  // here we could also generate the chord table in RAM...

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// returns number of available chords
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_CHORD_NumGet(void)
{
  return sizeof(seq_chord_table)/sizeof(seq_chord_entry_t);
}


/////////////////////////////////////////////////////////////////////////////
// returns pointer to the name of a chord
// Length: 6 characters + zero terminator
/////////////////////////////////////////////////////////////////////////////
char *SEQ_CHORD_NameGet(u8 chord_ix)
{
  if( chord_ix >= SEQ_CHORD_NumGet() )
    return "------";

  return (char *)seq_chord_table[chord_ix].name;
}


/////////////////////////////////////////////////////////////////////////////
// This function returns the transpose value of a chord with the given
// key number (0-3)
// selected scale
// IN: key number (0-3)
//     chord number (bit 3:0) and octave transpose value (6:4)
// returns note number if >= 0
// returns < 0 if no note defined for the given key
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_CHORD_NoteGet(u8 key_num, u8 chord)
{
  if( key_num >= 4 )
    return -2; // key number too high

  u8 chord_ix = chord & 0x1f;
  s32 oct_transpose = (chord >> 5) - 2;

  s32 note = (s32)seq_chord_table[chord_ix].keys[key_num];
  if( note < 0 )
    return note;

  // add C-2
  note += 0x30;

  // transpose octave
  note += oct_transpose * 12;

  // ensure that note is in the 0..127 range
  note = SEQ_CORE_TrimNote(note, 0, 127);

  return note;
}
