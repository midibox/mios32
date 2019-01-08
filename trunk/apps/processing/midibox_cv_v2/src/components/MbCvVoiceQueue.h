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

#ifndef _MB_CV_VOICE_QUEUE_H
#define _MB_CV_VOICE_QUEUE_H

#include <mios32.h>
#include "MbCvStructs.h"

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
} mbcv_voice_queue_item_t;


typedef struct {
  mbcv_voice_queue_item_t item[6]; // CV_SE_NUM_VOICES
} mbcv_voice_queue_t;


class MbCvVoiceQueue
{
public:
    // Constructor
    MbCvVoiceQueue();

    // Destructor
    ~MbCvVoiceQueue();

    // init functions
    void init(cv_patch_t *patch);
    void initExclusive(cv_patch_t *patch);

    // get/release functions
    u8 get(u8 instrument, u8 voice_asg, u8 num_voices);
    u8 getLast(u8 instrument, u8 voice_asg, u8 num_voices, u8 search_voice);
    u8 release(u8 release_voice);

    // debug functions
    void sendDebugMessage(void);

private:
    mbcv_voice_queue_t queue;

};

#endif /* _MB_CV_VOICE_QUEUE_H */
