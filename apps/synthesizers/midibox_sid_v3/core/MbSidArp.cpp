/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
// $Id$
/*
 * MIDIbox SID Arpeggiator
 *
 * ==========================================================================
 *
 *  Copyright (C) 2010 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#include <string.h>
#include "MbSidArp.h"
#include "MbSidSe.h"
#include "MbSidVoice.h"


/////////////////////////////////////////////////////////////////////////////
// for optional debugging messages via DEBUG_MSG (defined in mios32_config.h)
// should be at least 1 for sending error messages
/////////////////////////////////////////////////////////////////////////////
#define DEBUG_VERBOSE_LEVEL 1



/////////////////////////////////////////////////////////////////////////////
// Constructor
/////////////////////////////////////////////////////////////////////////////
MbSidArp::MbSidArp()
{
    init();
}


/////////////////////////////////////////////////////////////////////////////
// Destructor
/////////////////////////////////////////////////////////////////////////////
MbSidArp::~MbSidArp()
{
}



/////////////////////////////////////////////////////////////////////////////
// Arp init function
/////////////////////////////////////////////////////////////////////////////
void MbSidArp::init(void)
{
    restartReq = false;
    clockReq = false;

    arpActive = false;
    arpUp = false;
    arpHoldSaved = false;

    arpDivCtr = 0;
    arpGlCtr = 0;
    arpNoteCtr = 0;
    arpOctCtr = 0;
}


/////////////////////////////////////////////////////////////////////////////
// Arpeggiator handler
/////////////////////////////////////////////////////////////////////////////
void MbSidArp::tick(MbSidVoice *v, MbSidSe *mbSidSe)
{
    sid_se_midi_voice_t *mv = (sid_se_midi_voice_t *)v->midiVoice;
    sid_se_voice_arp_mode_t arpMode;
    arpMode.ALL = v->voicePatch->arp_mode;
    sid_se_voice_arp_speed_div_t arpSpeedDiv;
    arpSpeedDiv.ALL = v->voicePatch->arp_speed_div;
    sid_se_voice_arp_gl_rng_t arpGlRng;
    arpGlRng.ALL = v->voicePatch->arp_gl_rng;

    bool newNoteReq = false;
    bool firstNoteReq = false;
    bool gateClrReq = false;

    // check if arp sync requested
    if( restartReq ) {
        // set arp counters to max values (forces proper reset)
        arpNoteCtr = ~0;
        arpOctCtr = ~0;
        // reset ARP Up flag (will be set to 1 with first note)
        arpUp = 0;
        // request first note (for oneshot function)
        firstNoteReq = true;
        // reset divider if not disabled or if arp synch on MIDI clock start event
        if( restartReq || !arpMode.SYNC ) {
            arpDivCtr = ~0;
            arpGlCtr = ~0;
            // request new note
            newNoteReq = true;
        }

        restartReq = false;
    }

    // if clock sync event:
    if( clockReq ) {
        clockReq = false;

        // increment clock divider
        // reset divider if it already has reached the target value
        int inc = 1;
        if( arpMode.CAC ) {
            // in CAC mode: increment depending on number of pressed keys
            inc = mv->notestack.len;
            if( !inc ) // at least one increment
                inc = 1;
        }
        // handle divider
        // TODO: improve this!
        u8 divCtr = arpDivCtr + inc;
        u8 speedDiv = arpSpeedDiv.DIV + 1;
        while( divCtr >= speedDiv ) {
            divCtr -= speedDiv;
            // request new note
            newNoteReq = true;
            // request gate clear if voice not active anymore (required for Gln=>Speed)
            if( !v->voiceActive )
                gateClrReq = true;
        }
        arpDivCtr = divCtr;

        // increment gatelength counter
        // reset counter if it already has reached the target value
        if( ++arpGlCtr > arpGlRng.GATELENGTH ) {
            // reset counter
            arpGlCtr = 0;
            // request gate clear
            gateClrReq = true;
        }
    }

    // check if HOLD mode has been deactivated - disable notes in this case
    u8 disableNotes = !arpMode.HOLD && arpHoldSaved;
    // store HOLD flag in MIDI voice record
    arpHoldSaved = arpMode.HOLD;

    // skip the rest if arp is disabled
    if( disableNotes || !arpMode.ENABLE ) {
        // check if arp was active before (for proper 1->0 transition when ARP is disabled)
        if( arpActive ) {
            // notify that arp is not active anymore
            arpActive = false;
            // clear note stack (especially important in HOLD mode!)
            NOTESTACK_Clear(&mv->notestack);
            // propagate Note Off through trigger matrix
            mbSidSe->triggerNoteOff(v, 0);
            // request gate clear
            v->gateSetReq = 0;
            v->gateClrReq = 1;
        }
    } else {
        // notify that arp is active (for proper 1->0 transition when ARP is disabled)
        arpActive = true;

        // check if voice not active anymore (not valid in HOLD mode) or gate clear has been requested
        // skip voice active check in hold mode and voice sync mode
        if( gateClrReq || (!arpMode.HOLD && !arpMode.SYNC && !v->voiceActive) ) {
            // forward this to note handler if gate is not already deactivated
            if( v->gateActive ) {
                // propagate Note Off through trigger matrix
                mbSidSe->triggerNoteOff(v, 0);
                // request gate clear
                v->gateSetReq = 0;
                v->gateClrReq = 1;
            }
        }

        // check if a new arp note has been requested
        // skip if note counter is 0xaa (oneshot mode)
        if( newNoteReq && arpNoteCtr != 0xaa ) {
            // reset gatelength counter
            arpGlCtr = 0;
            // increment note counter
            // if max value of arp note counter reached, reset it
            if( ++arpNoteCtr >= mv->notestack.len )
                arpNoteCtr = 0;
            // if note is zero, reset arp note counter
            if( !mv->notestack_items[arpNoteCtr].note )
                arpNoteCtr = 0;


            // dir modes
            u8 noteNumber = 0;
            if( arpMode.DIR >= 6 ) { // Random
                if( mv->notestack.len > 0 )
                    noteNumber = randomGen.value(0, mv->notestack.len-1);
                else
                    noteNumber = 0;
            } else {
                bool newNoteUp = true;
                if( arpMode.DIR >= 2 && arpMode.DIR <= 5 ) { // Alt Mode 1 and 2
                    // toggle ARP_UP flag each time the arp note counter is zero
                    if( !arpNoteCtr ) {
                        arpUp ^= 1;
                        // increment note counter to prevent double played notes
                        if( arpMode.DIR >= 2 && arpMode.DIR <= 3 ) // only in Alt Mode 1
                            if( ++arpNoteCtr >= mv->notestack.len )
                                arpNoteCtr = 0;
                    }

                    // direction depending on Arp Up/Down and Alt Up/Down flag
                    if( (arpMode.DIR & 1) == 0 ) // UP modes
                        newNoteUp = arpUp;
                    else // DOWN modes
                        newNoteUp = !arpUp;
                } else {
                    // direction depending on arp mode 0 or 1
                    newNoteUp = (arpMode.DIR & 1) == 0;
                }

                if( newNoteUp )
                    noteNumber = arpNoteCtr;
                else
                    if( mv->notestack.len )
                        noteNumber = mv->notestack.len - arpNoteCtr - 1;
                    else
                        noteNumber = 0;
            }

            int newNote = mv->notestack_items[noteNumber].note;

            // now check for oneshot mode: if note is 0, or if note counter and oct counter is 0, stop here
            if( !firstNoteReq && arpSpeedDiv.ONESHOT &&
                (!newNote || (noteNumber == 0 && arpOctCtr == 0)) ) {
                // set note counter to 0xaa to stop ARP until next reset
                arpNoteCtr = 0xaa;
                // don't play new note
                newNote = 0;
            }

            // only play if note is not zero
            if( newNote ) {
                // if first note has been selected, increase octave until max value is reached
                if( firstNoteReq )
                    arpOctCtr = 0;
                else if( noteNumber == 0 ) {
                    ++arpOctCtr;
                }
                if( arpOctCtr > arpGlRng.OCTAVE_RANGE )
                    arpOctCtr = 0;

                // transpose note
                newNote += 12*arpOctCtr;

                // saturate octave-wise
                while( newNote >= 0x6c ) // use 0x6c instead of 128, since range 0x6c..0x7f sets frequency to 0xffff...
                    newNote -= 12;

                // store new arp note
                v->arpNote = newNote;

                // forward gate set request if voice is active and gate not active
                if( v->voiceActive ) {
                    v->gateClrReq = 0; // ensure that gate won't be cleared by previous CLR_REQ
                    if( !v->gateActive ) {
                        // set gate
                        v->gateSetReq = 1;

                        // propagate Note On through trigger matrix
                        mbSidSe->triggerNoteOn(v, 0);
                    }
                }
            }
        }
    }
}
