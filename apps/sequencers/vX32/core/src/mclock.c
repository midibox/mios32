/* $Id$ */
/*
vX32 pre-alpha
not for any use whatsoever
copyright stryd_one
bite me corp 2008

big props to nILS for being my fourth eye and TK for obvious reasons
stay tuned for UI prototyping courtesy of lucem!

*/


/////////////////////////////////////////////////////////////////////////////
// Include files
/////////////////////////////////////////////////////////////////////////////

#include <FreeRTOS.h>
#include <portmacro.h>

#include <mios32.h>


#include "tasks.h"
#include "app.h"
#include "graph.h"
#include "mclock.h"
#include "modules.h"
#include "patterns.h"
#include "utils.h"
#include "ui.h"

#include <seq_midi_out.h>
#include <seq_bpm.h>



/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

mclock_t mClock;                                                                // Allocate the Master Clock struct

u32 mod_Tick_Timestamp = DEAD_TIMESTAMP;                                        // initial global clock timestamp
static u32 SEQ_BPM_Timestamp = DEAD_TIMESTAMP;                                  // real timestamp as per the sequencer engine
static unsigned char reset_Req = 0;                                          // keeps track of the first tick

unsigned int midiclockcounter;                                                  // counts SEQ_BPM module ticks at VXPPQN resolution
unsigned int midiclockdivider;                                                  // divider value used to generate 24ppqn output


/////////////////////////////////////////////////////////////////////////////
// local prototypes
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Initialise the clocks
/////////////////////////////////////////////////////////////////////////////

void Clocks_Init(void) {
    
    SEQ_BPM_TickSet(0);                                                         // reset sequencer
    
    SEQ_BPM_Init(0);                                                            // initialize SEQ module sequencer
    
    SEQ_BPM_PPQN_Set(VXPPQN);                                                   // set internal clock resolution as per the define (usually 384)
    MClock_SetBPM(100.0);                                                       // Master BPM
    
    
    mClock.status.all = 0;                                                      // bit 7 is run/stop
    mClock.ticked = 0;                                                          // we haven't gotten that far yet
    mClock.timesigu = 4;                                                        // Upper value of the Master Time Signature, min 2
    mClock.timesigl = 4;                                                        // Lower value of the Master Time Signature, min 2
    mClock.res = SEQ_BPM_PPQN_Get();                                            // fill this from the SEQ_BPM module to be safe
    mClock.rt_latency = VXLATENCY;                                              // initialise from define
    
    MClock_Init();                                                              // init the vX master clock
    MClock_Reset();
}



/////////////////////////////////////////////////////////////////////////////
// Initialise the vX master clock
/////////////////////////////////////////////////////////////////////////////

void MClock_Init(void) {
    mClock.cyclelen = (unsigned int) 
    (((SEQ_BPM_PPQN_Get() * 4) * mClock.timesigu) / mClock.timesigl);           // Length of master track measured in ticks.

    midiclockdivider = (SEQ_BPM_PPQN_Get())/24;                                 // Set up the divider for 24ppqn output
}



/////////////////////////////////////////////////////////////////////////////
// Functions for user control
/////////////////////////////////////////////////////////////////////////////


void MClock_User_SetBPM(float bpm) {
    MClock_SetBPM(bpm);
}

void MClock_User_Start(void) {
    SEQ_BPM_Start();
}

void MClock_User_Continue(void) {
    SEQ_BPM_Cont();
}

void MClock_User_Stop(void) {
    SEQ_BPM_Stop();
}


/////////////////////////////////////////////////////////////////////////////
// Set the BPM of the vX master clock
/////////////////////////////////////////////////////////////////////////////

void MClock_SetBPM(float bpm) {
    if (SEQ_BPM_Set(bpm) < 0) {
        mClock.status.run = 0;                                                  // handle failure by pausing master clock
    } else {
        mClock.bpm = bpm;                                                       // store the tempo
    }
    
}



/////////////////////////////////////////////////////////////////////////////
// Called as often as possible to check for master clock ticks
/////////////////////////////////////////////////////////////////////////////

void MClock_Tick(void) {
    u8 num_loops = 0;
    u8 again = 0;

    u16 new_song_pos;

    do {                                                                        // let's check to see what SEQ_BPM has for us
        ++num_loops;
        
        if( SEQ_BPM_ChkReqStop() ) {
            if (SEQ_BPM_IsMaster()) MIOS32_MIDI_SendStop(DEFAULT);
            MClock_Stop();
        }
        
        if( SEQ_BPM_ChkReqCont() ) {
            if (SEQ_BPM_IsMaster()) MIOS32_MIDI_SendContinue(DEFAULT);
            MClock_Continue();
        }
        
        
        if( SEQ_BPM_ChkReqStart() ) {
            if (SEQ_BPM_IsMaster()) MIOS32_MIDI_SendStart(DEFAULT);
            MClock_Continue();
            reset_Req = 1;                                                      // reset and then catch up to the correct time at high speed
        }
                
        if( SEQ_BPM_ChkReqSongPos(&new_song_pos) ) {
            mClock.status.spp_hunt = 1;                                         // start hunting for song position
        }
        
        
        if( SEQ_BPM_ChkReqClk(&SEQ_BPM_Timestamp) > 0 ) {
            mClock.ticked++;
            again = 1;                                                          // check all requests again after execution of this part
        }
        
    } while (again && num_loops < 10);                                          // check a few times to be sure none are lost
    
    
    
    if (SEQ_BPM_Timestamp == mod_Tick_Timestamp) {                              // if we caught up with SPP
        mClock.status.spp_hunt = 0;                                             // continue as normal
    }
    
    
    if (SEQ_BPM_Timestamp < mod_Tick_Timestamp) {                               // if the SEQ_BPM engine says the timestamp has gone backwards, we got SPP and it's jumped back
        reset_Req = 1;                                                          // reset and then 
        mClock.status.spp_hunt = 1;                                             // catch up to the correct time at high speed
    }
    
    if (SEQ_BPM_Timestamp >= DEAD_TIMESTAMP) {
        reset_Req = 1;                                                          // dude get some sleep it's been playing for days!
    }
    
    
    
    if ((mClock.status.run > 0) ||                                              // if we're running
        (mClock.status.spp_hunt > 0)) {                                           // or just hunting for song position
        
        if (reset_Req == 0) {                                                   // if we've already seen the first tick
            if (SEQ_BPM_Timestamp > mod_Tick_Timestamp) {                       // if the SEQ_BPM engine timestamp has advanced beyond the vX's
                mod_Tick_Timestamp++;                                           // let's see the next one
            }
            
        } else {                                                                // otherwise
            MClock_Reset();                                                     // so start from zero and process like crazy until we catch up!
        }
        
        
        if (mClock.status.run > 0) {
            if (SEQ_BPM_IsMaster()) {                                           // send MIDI clocks if needed
                while (mClock.ticked > 0) {                                     // and the master clock has ticked
                    if (midiclockcounter == 0) {
                        MIOS32_MIDI_SendClock(DEFAULT);
                        midiclockcounter = midiclockdivider;
                    }
                    
                    midiclockcounter--;
                    mClock.ticked--;
                }
                
            }
            
        }
        
        
        
    }
    
}



/////////////////////////////////////////////////////////////////////////////
// Called to respond to MIDI Continue messages (play!)
/////////////////////////////////////////////////////////////////////////////

void MClock_Continue(void) {
    mClock.status.run = 1;                                                      // just set the run bit and bounce
}


/////////////////////////////////////////////////////////////////////////////
// Called to respond to MIDI Stop messages (pause/stop are the same)
/////////////////////////////////////////////////////////////////////////////

void MClock_Stop(void) {
    mClock.status.run = 0;                                                      // clear the bit
    
    SEQ_MIDI_OUT_FlushQueue();                                                  // and flush the queues
}



/////////////////////////////////////////////////////////////////////////////
// Called to respond to MIDI Start messages (play from the beginning) and SPP
/////////////////////////////////////////////////////////////////////////////

void MClock_Reset(void) {
    SEQ_MIDI_OUT_FlushQueue();                                                  // flush the SEQ_MIDI queues which should be empty anyways
    midiclockcounter = 0;                                                       // reset the counter
    reset_Req = 0;                                                              // we just did that
    mClock.ticked = 0;                                                          // and we need to reset this too
    
    SEQ_BPM_TickSet(0);                                                         // reset BPM tick

    SEQ_BPM_Timestamp = mod_Tick_Timestamp = 0;                                 // we'll force this to zero in case it's already advanced
    
    mClock.status.reset_req = 1;                                                // Set global reset flag
    Mod_PreProcess(DEAD_NODEID);                                                // preprocess all modules, so they can catch the reset
    mClock.status.reset_req = 0;                                                // Clear the flag
    
}



// FIXME todo
// 'strict' and 'nice' MIDI clocks - send as per timer, or send as per rack ticks
