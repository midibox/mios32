// $Id$
/*
 * MBSID Voice Allocation Routines
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

#include <sid.h>

#include "sid_se.h"
#include "sid_patch.h"
#include "sid_voice.h"


/////////////////////////////////////////////////////////////////////////////
// for optional debugging messages via DEBUG_MSG (defined in mios32_config.h)
// should be at least 1 for sending error messages
/////////////////////////////////////////////////////////////////////////////
#define DEBUG_VERBOSE_LEVEL 1


/////////////////////////////////////////////////////////////////////////////
// local defines
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// local types
/////////////////////////////////////////////////////////////////////////////

typedef union {
  struct {
    u16 ALL;
  };
  struct {
    u8 voice:6;
    u8 ASSIGNED:1;
    u8 EXCLUSIVE:1;
    u8 instrument;
  };
} sid_voice_queue_item_t;


typedef struct {
  sid_voice_queue_item_t item[SID_SE_NUM_VOICES];
} sid_voice_queue_t;


/////////////////////////////////////////////////////////////////////////////
// local variables
/////////////////////////////////////////////////////////////////////////////
static sid_voice_queue_t sid_voice_queue[SID_NUM];


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 SID_VOICE_Init(u32 mode)
{
  int i;

  for(i=0; i<SID_NUM; ++i)
    SID_VOICE_QueueInit(i);

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//  This function initializes the voice queue and the assigned instruments
//  The queue contains a number for each voice. So long the ASSIGNED flag is 
//  set, the voice is assigned to an instrument, if bit ASSIGNED is not set, 
//  the voice can be allocated by a new instrument
// 
//  The instrument number is stored in the queue as well, it is especially
//  important for mono voices
// 
//  The first voice in the queue is the first which will be taken.
//  To realize a "drop longest note first" algorithm, the take number should
//  always be moved to the end of the queue
/////////////////////////////////////////////////////////////////////////////
s32 SID_VOICE_QueueInit(u8 sid)
{
  sid_voice_queue_t *queue = (sid_voice_queue_t *)&sid_voice_queue[sid];
  int voice;

  sid_voice_queue_item_t *item = (sid_voice_queue_item_t *)&queue->item[0];
  for(voice=0; voice<SID_SE_NUM_VOICES; ++voice, ++item) {
    item->voice = voice;
    item->ASSIGNED = 0;
    item->EXCLUSIVE = 0;
    item->instrument = 0xff; // invalid instrument
  }

  // initialize exclusive flags
  SID_VOICE_QueueInitExclusive(sid);

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Initializes the exclusive voice allocation flags
/////////////////////////////////////////////////////////////////////////////
s32 SID_VOICE_QueueInitExclusive(u8 sid)
{
  sid_voice_queue_t *queue = (sid_voice_queue_t *)&sid_voice_queue[sid];
  int voice;

  // by default, allow non-exclusive access
  sid_voice_queue_item_t *item = (sid_voice_queue_item_t *)&queue->item[0];
  for(voice=0; voice<SID_SE_NUM_VOICES; ++voice, ++item)
    item->EXCLUSIVE = 0;

  // engine specific code
  switch( sid_patch[sid].engine ) {
    case SID_SE_LEAD:
    case SID_SE_BASSLINE:
      break; // bot relevant

    case SID_SE_DRUM: {
      u8 drum;
      sid_se_voice_patch_t *voice_patch = (sid_se_voice_patch_t *)&sid_patch[sid].D.voice[0];
      for(drum=0; drum<16; ++drum, ++voice_patch) {
	u8 voice_asg = ((sid_se_v_flags_t)voice_patch->D.v_flags).D.VOICE_ASG;
	int direct_voice_asg = voice_asg - 3;
	if( direct_voice_asg >= 0 && direct_voice_asg < SID_SE_NUM_VOICES ) {
	  // search for drum instrument in queue and set exclusive flag
	  item = (sid_voice_queue_item_t *)&queue->item[0];
	  for(voice=0; voice<SID_SE_NUM_VOICES; ++voice, ++item)
	    if( item->instrument == drum )
	      item->EXCLUSIVE = 1;
	}
      }
    } break;

    case SID_SE_MULTI: {
      u8 ins;
      sid_se_voice_patch_t *voice_patch = (sid_se_voice_patch_t *)&sid_patch[sid].M.voice[0];
      for(ins=0; ins<6; ++ins, ++voice_patch) {
	u8 voice_asg = voice_patch->M.voice_asg;
	int direct_voice_asg = voice_asg - 3;
	if( direct_voice_asg >= 0 && direct_voice_asg < SID_SE_NUM_VOICES ) {
	  // search for drum instrument in queue and set exclusive flag
	  item = (sid_voice_queue_item_t *)&queue->item[0];
	  for(voice=0; voice<SID_SE_NUM_VOICES; ++voice, ++item)
	    if( item->instrument == ins )
	      item->EXCLUSIVE = 1;
	}
      }
    } break;
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// This function searches for a voice which is not allocated, or drops the
// voice which played the longest note.
// Note: this function will always return a valid voice, and never a negative
// result (therefore u8)
/////////////////////////////////////////////////////////////////////////////
u8 SID_VOICE_Get(u8 sid, u8 instrument, u8 voice_asg, u8 num_voices)
{
  sid_voice_queue_t *queue = (sid_voice_queue_t *)&sid_voice_queue[sid];
  int voice;

  if( num_voices > SID_SE_NUM_VOICES )
    num_voices = SID_SE_NUM_VOICES;

  // determine allowed voices (each voice has a dedicated flag)
  u32 allowed_voice_mask;
  switch( voice_asg ) {
    case 0: // all voices
      allowed_voice_mask = (1 << SID_SE_NUM_VOICES)-1; // all voices
      break;

    case 1: // only left voices
      allowed_voice_mask = (1 << (SID_SE_NUM_VOICES/2))-1;
      break;

    case 2: // only right voices
      if( num_voices <= (SID_SE_NUM_VOICES/2) )
	allowed_voice_mask = (1 << (SID_SE_NUM_VOICES/2))-1; // Mono mode: take left voices
      else
	allowed_voice_mask = ((1 << (SID_SE_NUM_VOICES/2))-1) << (SID_SE_NUM_VOICES/2); // only right voices
      break;

    default: { // dedicated voice
      u8 dedicated_voice = (voice_asg-3) % num_voices; // if mono mode: take left voice
      allowed_voice_mask = (1 << dedicated_voice);
    }
  }

  // search for voice and allocate it
  sid_voice_queue_item_t *item = (sid_voice_queue_item_t *)&queue->item[0];
  for(voice=0; voice<SID_SE_NUM_VOICES; ++voice, ++item) {
    if( allowed_voice_mask & (1 << item->voice) &&
	(!item->EXCLUSIVE || (item->instrument == instrument) || (voice_asg >= 3)) ) {
      // assign voice and save instrument number
      item->ASSIGNED = 1;
      item->EXCLUSIVE = (voice_asg >= 3) ? 1 : 0; // exclusive assignment?
      item->instrument = instrument;

      // if this isn't already the last voice, put this item to the end of the queue
      if( voice < (SID_SE_NUM_VOICES-1) ) {
	sid_voice_queue_item_t stored_item = *item;
	int i;
	for(i=voice; i<(SID_SE_NUM_VOICES-1); ++i)
	  queue->item[i] = queue->item[i+1];
	queue->item[SID_SE_NUM_VOICES-1] = stored_item;
      }

#if DEBUG_VERBOSE_LEVEL >= 2
      SID_VOICE_SendDebugMessage(sid);
#endif

      // return with voice number
      return queue->item[SID_SE_NUM_VOICES-1].voice;
    }
  }

  // we should never reach this part!
#if DEBUG_VERBOSE_LEVEL >= 1
  DEBUG_MSG("[SID_VOICE_Get:%d] voice not in queue anymore (voice_asg: 0x%02x, allowed_mask: 0x%02x)\n", sid, voice_asg, allowed_voice_mask);
#endif

  return 0; // take first voice on this error case
}


/////////////////////////////////////////////////////////////////////////////
// This function searches for a givem voice. If it is already allocated,
// it continues at SID_VOICE_Get, otherwise it returns current_voice
// Note: this function will always return a valid voice, and never a negative
// result (therefore u8)
/////////////////////////////////////////////////////////////////////////////
u8 SID_VOICE_GetLast(u8 sid, u8 instrument, u8 voice_asg, u8 num_voices, u8 search_voice)
{
  sid_voice_queue_t *queue = (sid_voice_queue_t *)&sid_voice_queue[sid];
  int voice;

  // search in queue for selected voice
  sid_voice_queue_item_t *item = (sid_voice_queue_item_t *)&queue->item[0];
  for(voice=0; voice<SID_SE_NUM_VOICES; ++voice, ++item)
    if( item->voice == search_voice ) {
      // selected voice found
      // if instrument number not equal, we should get a new voice
      if( item->instrument != instrument )
	break;
      // if number of available voices has changed meanwhile (e.g. Stereo->Mono switch):
      // check that voice number still < n
      if( item->voice >= num_voices )
	break;

      // it's mine!

      // assign voice (again)
      item->ASSIGNED = 1;

      // if this isn't already the last voice, put this item to the end of the queue
      if( voice < (SID_SE_NUM_VOICES-1) ) {
	sid_voice_queue_item_t stored_item = *item;
	int i;
	for(i=voice; i<(SID_SE_NUM_VOICES-1); ++i)
	  queue->item[i] = queue->item[i+1];
	queue->item[SID_SE_NUM_VOICES-1] = stored_item;
      }

#if DEBUG_VERBOSE_LEVEL >= 2
      SID_VOICE_SendDebugMessage(sid);
#endif

      // return with voice number
      return queue->item[SID_SE_NUM_VOICES-1].voice;

    }

  // voice not found, continue at SID_VOICE_Get
  return SID_VOICE_Get(sid, instrument, voice_asg, num_voices);
}


/////////////////////////////////////////////////////////////////////////////
// This function releases a voice, so that it is free for SID_VOICE_Get
// Note: this function will always return a valid voice, and never a negative
// result (therefore u8)
/////////////////////////////////////////////////////////////////////////////
u8 SID_VOICE_Release(u8 sid, u8 release_voice)
{
  sid_voice_queue_t *queue = (sid_voice_queue_t *)&sid_voice_queue[sid];
  int voice;

  // search in voice queue for the voice
  sid_voice_queue_item_t *item = (sid_voice_queue_item_t *)&queue->item[0];
  for(voice=0; voice<SID_SE_NUM_VOICES; ++voice, ++item)
    if( item->voice == release_voice ) {
      item->ASSIGNED = 0;

#if DEBUG_VERBOSE_LEVEL >= 2
      SID_VOICE_SendDebugMessage(sid);
#endif

      return item->voice;
    }

  // we should never reach this part!
#if DEBUG_VERBOSE_LEVEL >= 1
  DEBUG_MSG("[SID_VOICE_Release:%d] voice %d not in queue anymore!\n", sid, release_voice);
#endif

  return 0; // take first voice on this error case
}


/////////////////////////////////////////////////////////////////////////////
// Sends the content of the voice queue to the MIOS Terminal
/////////////////////////////////////////////////////////////////////////////
s32 SID_VOICE_SendDebugMessage(u8 sid)
{
  sid_voice_queue_t *queue = (sid_voice_queue_t *)&sid_voice_queue[sid];
  int voice;

  DEBUG_MSG("Voice Queue content of SID %d:\n", sid);
  sid_voice_queue_item_t *item = (sid_voice_queue_item_t *)&queue->item[0];
  for(voice=0; voice<SID_SE_NUM_VOICES; ++voice, ++item)
    DEBUG_MSG("  [%d] V:%d  A:%d  E:%d  I:%d\n",
	      voice, item->voice, item->ASSIGNED, item->EXCLUSIVE, item->instrument);

  return 0;
}
