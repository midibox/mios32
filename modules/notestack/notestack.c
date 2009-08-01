// $Id$
//! \defgroup NOTESTACK
//!
//! Generic Notestack Module
//!
//! Usage Examples:
//!   $MIOS32_PATH/apps/tutorial/013_aout
//!   $MIOS32_PATH/apps/tutorial/014_sequencer
//!   $MIOS32_PATH/apps/sequencers/midibox_seq_v4/core/seq_midi_in.c
//!
//! \{
/* ==========================================================================
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
#include "notestack.h"


/////////////////////////////////////////////////////////////////////////////
//! Initializes a Notestack
//!
//! Has to be called before NOTESTACK_Push/Pop/Clear functions are used!
//! \param[in] *n pointer to notestack structure
//! \param[in] mode one of following modes:
//! <UL>
//!   <LI>NOTESTACK_MODE_PUSH_TOP: new notes are added to the top of the note stack
//!   <LI>NOTESTACK_MODE_PUSH_BOTTOM: new notes are added to the bottom of the note stack
//!   <LI>NOTESTACK_MODE_PUSH_TOP_HOLD: same like above, but notes won't be removed so long
//!       there is a free item in note stack. Instead, they will be marked with .depressed=1
//!   <LI>NOTESTACK_MODE_PUSH_BOTTOM_HOLD,: same like above, but new notes will be added to
//!       the bottom of the note stack
//!   <LI>NOTESTACK_MODE_SORT: Notes will be sorted
//!   <LI>NOTESTACK_MODE_SORT_HOLD: Like above, but notes won't be removed so long there is a free item in note stack.
//!       Instead, they will be marked with .depressed=1
//! </UL>
//! \param[in] *note_items pointer to notestack_item_t array which stores the notes and related informations
//! \param[in] size number of note items stored in the array
//! \return < 0 if initialisation failed
/////////////////////////////////////////////////////////////////////////////
s32 NOTESTACK_Init(notestack_t *n, notestack_mode_t mode, notestack_item_t *note_items, u8 size)
{
  n->mode = mode;
  n->size = size;
  n->len = 0;
  n->note_items = note_items;

  return NOTESTACK_Clear(n);
}


/////////////////////////////////////////////////////////////////////////////
//! Pushes a new note, bundled with a tag, to the note stack
//! \param[in] *n pointer to notestack structure
//! \param[in] new_note the note number which should be added (1..127)
//! \param[in] tag an optional tag which is bundled with the note. It can
//!            contain a voice number, the velocity, or...
//! \return < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 NOTESTACK_Push(notestack_t *n, u8 new_note, u8 tag)
{
  int i;

  u8 hold_mode =
    n->mode == NOTESTACK_MODE_PUSH_TOP_HOLD ||
    n->mode == NOTESTACK_MODE_PUSH_BOTTOM_HOLD ||
    n->mode == NOTESTACK_MODE_SORT_HOLD;

  if( hold_mode ) {
    // check if note already in stack - in this case, replace it with the new tag and exit
    for(i=0; i < n->len; ++i) {
      if( n->note_items[i].note == new_note ) {
	n->note_items[i].depressed = 0;
	n->note_items[i].tag = tag;
	return 0; // no error
      }
    }
  } else {
    // not in hold mode:
    // check if note already in stack - in this case, remove it
    NOTESTACK_Pop(n, new_note);
  }

  int insertion_point = 0;
  if( n->mode == NOTESTACK_MODE_PUSH_BOTTOM || n->mode == NOTESTACK_MODE_PUSH_BOTTOM_HOLD ) {
    // add note to the end of the stack (FIFO)
    if( n->len < n->size ) {
      insertion_point = n->len;
    } else {
      // corner case: stack is full, and new note is greater than all others:
      // replace last note by new one and exit
      n->note_items[n->size-1].note = new_note;
      n->note_items[n->size-1].depressed = 0;
      n->note_items[n->size-1].tag = tag;
      return 0; // no error
    }
    insertion_point = n->len;
  } else {
    // add note to the beginning of the stack (FILO)
    // if sort flag set: search for insertion point
    int sort = n->mode == NOTESTACK_MODE_SORT || n->mode == NOTESTACK_MODE_SORT_HOLD;
    if( sort && n->len ) {
      for(i=0; i<n->len; ++i)
	if( n->note_items[i].note > new_note ) {
	  insertion_point = i;
	  break;
	}
    }

    if( i == n->len ) {
      // corner case: stack is full, and new note is greater than all others:
      // replace last note by new one and exit
      if( n->len >= n->size ) {
	n->note_items[n->size-1].note = new_note;
	n->note_items[n->size-1].depressed = 0;
	n->note_items[n->size-1].tag = tag;
	return 0; // no error
      }
      insertion_point = n->len;
    }
  }
  
  // increment length so long it hasn't reached the notestack size
  if( n->len < n->size )
    ++n->len;
  
  // add note at insertion point
  for(i=n->len-1; i > insertion_point; --i)
    n->note_items[i] = n->note_items[i-1];
  n->note_items[insertion_point].note = new_note;
  n->note_items[insertion_point].depressed = 0;
  n->note_items[insertion_point].tag = tag;

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
//! Removes a note from the stack
//! \param[in] *n pointer to notestack structure
//! \param[in] old_note the note number which should be removed (1..127)
//! \return < 0 on errors
//! \return 0 if note hasn't been found
//! \return 1 if note has been found and removed from stack (in hold mode: marked as depressed)
//! \return 2 only in hold mode: if all notes are depressed now
/////////////////////////////////////////////////////////////////////////////
s32 NOTESTACK_Pop(notestack_t *n, u8 old_note)
{
  int i, j;

  u8 hold_mode =
    n->mode == NOTESTACK_MODE_PUSH_TOP_HOLD ||
    n->mode == NOTESTACK_MODE_PUSH_BOTTOM_HOLD ||
    n->mode == NOTESTACK_MODE_SORT_HOLD;

  // search for note with same value and remove it
  // (SEQ_MIDI_IN_Notestack_Push ensures, that a note value only exists one time in stack)
  for(i=0; i < n->len; ++i) {
    if( n->note_items[i].note == old_note ) {
      if( hold_mode ) {
	n->note_items[i].depressed = 1;
	
	// check if any note is still pressed
	u8 any_note_pressed = 0;
	for(j=0; !any_note_pressed && j<n->len; ++j)
	  if( !n->note_items[j].depressed )
	    any_note_pressed = 1;

	return any_note_pressed ? 2 : 1;
      } else {
	for(j=i; j < n->len-1; ++j)
	  n->note_items[j] = n->note_items[j+1];
	--n->len;
	n->note_items[n->len].ALL = 0x00;

	return 1; // note has been found and removed
      }
    }
  }

  // note hasn't been found
  return 0;
}


/////////////////////////////////////////////////////////////////////////////
//! Clears the note stack
//! \param[in] *n pointer to notestack structure
//! \return < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 NOTESTACK_Clear(notestack_t *n)
{
  int i;

  for(i=0; i<n->size; ++i)
    n->note_items[i].ALL = 0;

  return 0; // no error
}



/////////////////////////////////////////////////////////////////////////////
//! Sends the content of the Notestack to the MIOS Terminal
//! \param[in] *n pointer to notestack structure
//! \return < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 NOTESTACK_SendDebugMessage(notestack_t *n)
{
  static const char note_name[12][3] = { "C-", "C#", "D-", "D#", "E-", "F-", "F#", "G-", "G#", "A-", "A#", "B-" };
  int i;

  MIOS32_MIDI_SendDebugMessage("Notestack Content (len=%d, size=%d)\n", n->len, n->size);
  for(i=0; i<n->len; ++i) {
    MIOS32_MIDI_SendDebugMessage("%02d: %s%-3d (0x%02x) %s tag:0x%02x\n",
				 i,
				 note_name[n->note_items[i].note%12], (int)(n->note_items[i].note/12)-2,
				 n->note_items[i].note,
				 n->note_items[i].depressed ? "depressed" : "pressed  ",
				 n->note_items[i].tag);
  }

  return 0; // no error
}

//! \}
