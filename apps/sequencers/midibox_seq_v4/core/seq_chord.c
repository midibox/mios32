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
  s8   keys[6];
  char name[7];
} seq_chord_entry_t;


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

  // note: chords are used together with the forced-to-scale feature in seq_scale.c
  // if no key should be played, add -1
static const seq_chord_entry_t seq_chord_table[2][32] = {
  {
    // 1   2   3   4   5   6     <----> (6 chars!)
    {{ 0,  4,  7, -1, -1, -1 }, "Maj.I " },
    {{ 4,  7, 12, -1, -1, -1 }, "Maj.II" },
    {{ 7, 12, 16, -1, -1, -1 }, "MajIII" },
    {{ 0, -1, -1, -1, -1, -1 }, "Root  " },
    {{ 4, -1, -1, -1, -1, -1 }, "3rd   " },
    {{ 7, -1, -1, -1, -1, -1 }, "5th   " },
    {{ 0,  4, -1, -1, -1, -1 }, "R.+3rd" },
    {{ 0,  7, -1, -1, -1, -1 }, "R.+5th" },
    {{ 0,  4,  7,  9, -1, -1 }, "Maj.6 " },
    {{ 0,  4,  7, 11, -1, -1 }, "Maj.7 " },
    {{ 0,  4,  7, 12, -1, -1 }, "Maj.8 " },
    {{ 0,  4,  7, 14, -1, -1 }, "Maj.9 " },
    {{ 0,  7, 12, 16, -1, -1 }, "Maj.10" },
    {{ 0,  7, 12, 19, -1, -1 }, "Maj.12" },
    {{ 0,  5,  7, -1, -1, -1 }, "Sus.4 " },
    {{ 0,  4,  8, -1, -1, -1 }, "Maj.+ " },

    {{ 0,  3,  7, -1, -1, -1 }, "Min.I " },
    {{ 3,  7, 12, -1, -1, -1 }, "Min.II" },
    {{ 7, 12, 15, -1, -1, -1 }, "MinIII" },
    {{ 0, -1, -1, -1, -1, -1 }, "Root  " },
    {{ 3, -1, -1, -1, -1, -1 }, "3rdMin" },
    {{ 7, -1, -1, -1, -1, -1 }, "5th   " },
    {{ 0,  3, -1, -1, -1, -1 }, "R.+3rd" },
    {{ 0,  7, -1, -1, -1, -1 }, "R.+5th" },
    {{ 0,  3,  7,  9, -1, -1 }, "Min.6 " },
    {{ 0,  3,  7, 11, -1, -1 }, "Min.7 " },
    {{ 0,  3,  7, 12, -1, -1 }, "Min.8 " },
    {{ 0,  3,  7, 14, -1, -1 }, "Min.9 " },
    {{ 0,  7, 12, 16, -1, -1 }, "Min.10" },
    {{ 0,  7, 12, 18, -1, -1 }, "Min.12" },
    {{ 0,  3,  6,  9, -1, -1 }, "Co7   " },
    {{ 0,  3,  8, -1, -1, -1 }, "Min.+ " }
  }, {
    // see http://midibox.org/forums/topic/19886-extended-chords-dominants-tensions
    // corrected version at http://midibox.org/forums/topic/13137-midibox-seq-v4-release-feedback/?do=findComment&comment=174313
    {{ 0,  7, -1, -1, -1, -1 }, "Pwr5  " },
    {{ 0,  7, 12, -1, -1, -1 }, "Pwr8  " },
    {{ 0,  4, -1, -1, -1, -1 }, "R+mj3 " },
    {{ 0,  3, -1, -1, -1, -1 }, "R+min3" },
    // major chords
    {{ 0,  4,  7, -1, -1, -1 }, "Maj   " },
    {{ 0,  5,  7, -1, -1, -1 }, "Sus4  " },
    {{ 0,  4,  8, -1, -1, -1 }, "Maj+  " },
    {{ 0,  4,  7,  9, -1, -1 }, "Maj6  " },
    {{ 0,  4,  7, 11, -1, -1 }, "Maj7  " },
    {{ 0,  4,  7, 14, -1, -1 }, "add9  " },
    {{ 0,  4,  7, 11, 14, -1 }, "Maj9  " },
    {{ 0,  4,  7, 11, 14, 17 }, "Maj11 " },
    {{ 0,  4,  7, 11, 14, 21 }, "Maj13 " },
    // minor chords
    {{ 0,  3,  7, -1, -1, -1 }, "Min   " },
    {{ 0,  3,  7,  9, -1, -1 }, "Min6  " },
    {{ 0,  3,  7, 10, -1, -1 }, "Min7  " },
    {{ 0,  3,  7, 14, -1, -1 }, "Minad9" },
    {{ 0,  3,  7, 10, 14, -1 }, "Min9  " },
    {{ 0,  3,  7, 10, 14, 17 }, "Min11 " },
    {{ 0,  3,  7, 10, 14, 21 }, "Min13 " },
    // dominant chords
    {{ 0,  4,  7, 10, -1, -1 }, "Dom7  " },
    {{ 0,  5,  7, 10, -1, -1 }, "7Sus4 " },
    {{ 0,  4,  7, 10, 14, -1 }, "Dom9  " },
    {{ 0,  4,  7, 10, 14, 17 }, "Dom11 " },
    {{ 0,  4,  7, 10, 14, 21 }, "Dom13 " },
    // dominant chords with tensions
    {{ 0,  4,  6, 10, -1, -1 }, "7b5   " },
    {{ 0,  4,  8, 10, -1, -1 }, "7#5   " },
    {{ 0,  4,  7, 10, 13, -1 }, "7b9   " },
    {{ 0,  4,  7, 10, 15, -1 }, "7#9   " },
    // diminished
    {{ 0,  3,  6, -1, -1, -1 }, "DimTri" },
    {{ 0,  3,  6,  9, -1, -1 }, "Dim   " },
    // half diminished aka m7b5
    {{ 0,  3,  6, 10, -1, -1 }, "m7b5  " },
  }
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
  return sizeof(seq_chord_table[0])/sizeof(seq_chord_entry_t);
}


/////////////////////////////////////////////////////////////////////////////
// returns pointer to the name of a chord
// Length: 6 characters + zero terminator
/////////////////////////////////////////////////////////////////////////////
char *SEQ_CHORD_NameGet(u8 chord_set, u8 chord_ix)
{
  if( chord_ix >= SEQ_CHORD_NumGet() )
    return "------";

  return (char *)seq_chord_table[chord_set][chord_ix].name;
}


/////////////////////////////////////////////////////////////////////////////
// This function returns the transpose value of a chord with the given
// key number (0-3)
// IN: key number (0-3)
//     chord number (bit 4:0) and octave transpose value (7:5)
// returns note number if >= 0
// returns < 0 if no note defined for the given key
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_CHORD_NoteGet(u8 key_num, u8 chord_set, u8 chord)
{
  if( key_num >= 6 )
    return -2; // key number too high

  if( chord_set >= 2 )
    return -3; // invalid chord set

  u8 chord_ix = chord & 0x1f;
  s32 oct_transpose = (chord >> 5) - 2;

  s32 note = (s32)seq_chord_table[chord_set][chord_ix].keys[key_num];
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
