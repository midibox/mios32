/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
// $Id$
/*
 * MIDIbox CV Voice Queue
 *
 * ==========================================================================
 *
 *  Copyright (C) 2010 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#include "MbCvVoiceQueue.h"
#include <string.h>

// tmp-.
#define CV_SE_NUM_VOICES 6


/////////////////////////////////////////////////////////////////////////////
// for optional debugging messages via DEBUG_MSG (defined in mios32_config.h)
// should be at least 1 for sending error messages
/////////////////////////////////////////////////////////////////////////////
#define DEBUG_VERBOSE_LEVEL 1



/////////////////////////////////////////////////////////////////////////////
// Constructor
/////////////////////////////////////////////////////////////////////////////
MbCvVoiceQueue::MbCvVoiceQueue()
{
}


/////////////////////////////////////////////////////////////////////////////
// Destructor
/////////////////////////////////////////////////////////////////////////////
MbCvVoiceQueue::~MbCvVoiceQueue()
{
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
void MbCvVoiceQueue::init(cv_patch_t *patch)
{
    mbcv_voice_queue_item_t *item = (mbcv_voice_queue_item_t *)&queue.item[0];
    for(int voice=0; voice<CV_SE_NUM_VOICES; ++voice, ++item) {
        item->voice = voice;
        item->ASSIGNED = 0;
        item->EXCLUSIVE = 0;
        item->instrument = 0xff; // invalid instrument
    }

    // initialize exclusive flags
    initExclusive(patch);
}


/////////////////////////////////////////////////////////////////////////////
// Initializes the exclusive voice allocation flags
/////////////////////////////////////////////////////////////////////////////
void MbCvVoiceQueue::initExclusive(cv_patch_t *patch)
{
    // by default, allow non-exclusive access
    mbcv_voice_queue_item_t *item = (mbcv_voice_queue_item_t *)&queue.item[0];
    for(int voice=0; voice<CV_SE_NUM_VOICES; ++voice, ++item)
        item->EXCLUSIVE = 0;
}


// get/release functions
/////////////////////////////////////////////////////////////////////////////
// This function searches for a voice which is not allocated, or drops the
// voice which played the longest note.
// Note: this function will always return a valid voice, and never a negative
// result (therefore u8)
/////////////////////////////////////////////////////////////////////////////
u8 MbCvVoiceQueue::get(u8 instrument, u8 voice_asg, u8 num_voices)
{
    if( num_voices > CV_SE_NUM_VOICES )
        num_voices = CV_SE_NUM_VOICES;

    // determine allowed voices (each voice has a dedicated flag)
    u32 allowed_voice_mask;
    switch( voice_asg ) {
    case 0: // all voices
        allowed_voice_mask = (1 << CV_SE_NUM_VOICES)-1; // all voices
        break;

    case 1: // only left voices
        allowed_voice_mask = (1 << (CV_SE_NUM_VOICES/2))-1;
        break;

    case 2: // only right voices
        if( num_voices <= (CV_SE_NUM_VOICES/2) )
            allowed_voice_mask = (1 << (CV_SE_NUM_VOICES/2))-1; // Mono mode: take left voices
        else
            allowed_voice_mask = ((1 << (CV_SE_NUM_VOICES/2))-1) << (CV_SE_NUM_VOICES/2); // only right voices
        break;

    default: { // dedicated voice
        u8 dedicated_voice = (voice_asg-3) % num_voices; // if mono mode: take left voice
        allowed_voice_mask = (1 << dedicated_voice);
    }
    }

    // search for voice and allocate it
    mbcv_voice_queue_item_t *item = (mbcv_voice_queue_item_t *)&queue.item[0];
    for(int voice=0; voice<CV_SE_NUM_VOICES; ++voice, ++item) {
        if( allowed_voice_mask & (1 << item->voice) &&
            (!item->EXCLUSIVE || (item->instrument == instrument) || (voice_asg >= 3)) ) {
            // assign voice and save instrument number
            item->ASSIGNED = 1;
            item->EXCLUSIVE = (voice_asg >= 3) ? 1 : 0; // exclusive assignment?
            item->instrument = instrument;

            // if this isn't already the last voice, put this item to the end of the queue
            if( voice < (CV_SE_NUM_VOICES-1) ) {
                mbcv_voice_queue_item_t stored_item = *item;
                int i;
                for(i=voice; i<(CV_SE_NUM_VOICES-1); ++i)
                    queue.item[i] = queue.item[i+1];
                queue.item[CV_SE_NUM_VOICES-1] = stored_item;
            }

#if DEBUG_VERBOSE_LEVEL >= 2
            sendDebugMessage(cv);
#endif

            // return with voice number
            return queue.item[CV_SE_NUM_VOICES-1].voice;
        }
    }

    // we should never reach this part!
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[VoiceQueueGet] voice not in queue anymore (voice_asg: 0x%02x, allowed_mask: 0x%02x)\n", voice_asg, allowed_voice_mask);
#endif

    return 0; // take first voice on this error case
}


/////////////////////////////////////////////////////////////////////////////
// This function searches for a givem voice. If it is already allocated,
// it continues at get(), otherwise it returns current_voice
// Note: this function will always return a valid voice, and never a negative
// result (therefore u8)
/////////////////////////////////////////////////////////////////////////////
u8 MbCvVoiceQueue::getLast(u8 instrument, u8 voice_asg, u8 num_voices, u8 search_voice)
{
    // search in queue for selected voice
    mbcv_voice_queue_item_t *item = (mbcv_voice_queue_item_t *)&queue.item[0];
    for(int voice=0; voice<CV_SE_NUM_VOICES; ++voice, ++item)
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
            if( voice < (CV_SE_NUM_VOICES-1) ) {
                mbcv_voice_queue_item_t stored_item = *item;
                int i;
                for(i=voice; i<(CV_SE_NUM_VOICES-1); ++i)
                    queue.item[i] = queue.item[i+1];
                queue.item[CV_SE_NUM_VOICES-1] = stored_item;
            }

#if DEBUG_VERBOSE_LEVEL >= 2
            sendDebugMessage(cv);
#endif

            // return with voice number
            return queue.item[CV_SE_NUM_VOICES-1].voice;

        }

    // voice not found, continue at get()
    return get(instrument, voice_asg, num_voices);
}


/////////////////////////////////////////////////////////////////////////////
// This function releases a voice, so that it is free for get()
// Note: this function will always return a valid voice, and never a negative
// result (therefore u8)
/////////////////////////////////////////////////////////////////////////////
u8 MbCvVoiceQueue::release(u8 release_voice)
{
    // search in voice queue for the voice
    mbcv_voice_queue_item_t *item = (mbcv_voice_queue_item_t *)&queue.item[0];
    for(int voice=0; voice<CV_SE_NUM_VOICES; ++voice, ++item)
        if( item->voice == release_voice ) {
            item->ASSIGNED = 0;

#if DEBUG_VERBOSE_LEVEL >= 2
            sendDebugMessage();
#endif

            return item->voice;
        }

    // we should never reach this part!
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[voiceRelease] voice %d not in queue anymore!\n", release_voice);
#endif

    return 0; // take first voice on this error case

}


/////////////////////////////////////////////////////////////////////////////
// Sends the content of the voice queue to the MIOS Terminal
/////////////////////////////////////////////////////////////////////////////
void MbCvVoiceQueue::sendDebugMessage(void)
{
    DEBUG_MSG("Voice Queue content:\n");
    mbcv_voice_queue_item_t *item = (mbcv_voice_queue_item_t *)&queue.item[0];
    for(int voice=0; voice<CV_SE_NUM_VOICES; ++voice, ++item)
        DEBUG_MSG("  [%d] V:%d  A:%d  E:%d  I:%d\n",
                  voice, item->voice, item->ASSIGNED, item->EXCLUSIVE, item->instrument);
}
