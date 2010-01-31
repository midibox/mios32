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
#include "MbSidMidiVoice.h"
#include "MbSidVoice.h"


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
    arpEnabled = false;
    arpDown = false;
    arpUpAndDown = false;
    arpPingPong = false;
    arpRandomNotes = false;
    arpSortedNotes = false;
    arpHoldMode = false;
    arpSyncMode = false;
    arpConstantCycle = false;
    arpEasyChordMode = false;
    arpOneshotMode = false;
    arpSpeed = 0;
    arpGatelength = 0;
    arpOctaveRange = 0;

    restartReq = false;
    clockReq = false;

    arpActive = false;
    arpUp = false;
    arpHoldModeSaved = false;

    arpDivCtr = 0;
    arpGatelengthCtr = 0;
    arpNoteCtr = 0;
    arpOctaveCtr = 0;
}


/////////////////////////////////////////////////////////////////////////////
// Arpeggiator handler
/////////////////////////////////////////////////////////////////////////////
void MbSidArp::tick(MbSidVoice *v, MbSidSe *mbSidSe)
{
    MbSidMidiVoice *mv = (MbSidMidiVoice *)v->midiVoicePtr;

    bool newNoteReq = false;
    bool firstNoteReq = false;
    bool gateClrReq = false;

    // check if arp sync requested
    if( restartReq ) {
        // set arp counters to max values (forces proper reset)
        arpNoteCtr = ~0;
        arpOctaveCtr = ~0;
        // reset ARP Up flag (will be set to 1 with first note)
        arpUp = 0;
        // request first note (for oneshot function)
        firstNoteReq = true;
        // reset divider if not disabled or if arp synch on MIDI clock start event
        if( restartReq || !arpSyncMode ) {
            arpDivCtr = ~0;
            arpGatelengthCtr = ~0;
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
        if( arpConstantCycle ) {
            // in CAC mode: increment depending on number of pressed keys
            inc = mv->midivoiceNotestack.len;
            if( !inc ) // at least one increment
                inc = 1;
        }
        // handle divider
        // TODO: improve this!
        u8 divCtr = arpDivCtr + inc;
        u8 speedDiv = arpSpeed + 1;
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
        if( ++arpGatelengthCtr > arpGatelength ) {
            // reset counter
            arpGatelengthCtr = 0;
            // request gate clear
            gateClrReq = true;
        }
    }

    // check if HOLD mode has been deactivated - disable notes in this case
    u8 disableNotes = !arpHoldMode && arpHoldModeSaved;
    // store HOLD flag in MIDI voice record
    arpHoldModeSaved = arpHoldMode;

    // skip the rest if arp is disabled
    if( disableNotes || !arpEnabled ) {
        // check if arp was active before (for proper 1->0 transition when ARP is disabled)
        if( arpActive ) {
            // notify that arp is not active anymore
            arpActive = false;
            // clear note stack (especially important in HOLD mode!)
            NOTESTACK_Clear(&mv->midivoiceNotestack);
            // propagate Note Off through trigger matrix
            mbSidSe->triggerNoteOff(v, 0);
            // request gate clear
            v->voiceGateSetReq = 0;
            v->voiceGateClrReq = 1;
        }
    } else {
        // notify that arp is active (for proper 1->0 transition when ARP is disabled)
        arpActive = true;

        // check if voice not active anymore (not valid in HOLD mode) or gate clear has been requested
        // skip voice active check in hold mode and voice sync mode
        if( gateClrReq || (!arpHoldMode && !arpSyncMode && !v->voiceActive) ) {
            // forward this to note handler if gate is not already deactivated
            if( v->voiceGateActive ) {
                // propagate Note Off through trigger matrix
                mbSidSe->triggerNoteOff(v, 0);
                // request gate clear
                v->voiceGateSetReq = 0;
                v->voiceGateClrReq = 1;
            }
        }

        // check if a new arp note has been requested
        // skip if note counter is 0xaa (oneshot mode)
        if( newNoteReq && arpNoteCtr != 0xaa ) {
            // reset gatelength counter
            arpGatelengthCtr = 0;
            // increment note counter
            // if max value of arp note counter reached, reset it
            if( ++arpNoteCtr >= mv->midivoiceNotestack.len )
                arpNoteCtr = 0;
            // if note is zero, reset arp note counter
            if( !mv->midivoiceNotestackItems[arpNoteCtr].note )
                arpNoteCtr = 0;


            // dir modes
            u8 noteNumber = 0;
            if( arpRandomNotes ) {
                if( mv->midivoiceNotestack.len > 0 )
                    noteNumber = randomGen.value(0, mv->midivoiceNotestack.len-1);
                else
                    noteNumber = 0;
            } else {
                bool newNoteUp = true;
                if( arpUpAndDown ) { // Alt Mode 1 and 2
                    // toggle ARP_UP flag each time the arp note counter is zero
                    if( !arpNoteCtr ) {
                        arpUp ^= 1;
                        // increment note counter to prevent double played notes
                        if( !arpPingPong ) // only in Alt Mode 1
                            if( ++arpNoteCtr >= mv->midivoiceNotestack.len )
                                arpNoteCtr = 0;
                    }

                    // direction depending on Arp Up/Down and Alt Up/Down flag
                    if( arpDown == 0 ) // UP modes
                        newNoteUp = arpUp;
                    else // DOWN modes
                        newNoteUp = !arpUp;
                } else {
                    // direction depending on arp mode 0 or 1
                    newNoteUp = arpDown;
                }

                if( newNoteUp )
                    noteNumber = arpNoteCtr;
                else
                    if( mv->midivoiceNotestack.len )
                        noteNumber = mv->midivoiceNotestack.len - arpNoteCtr - 1;
                    else
                        noteNumber = 0;
            }

            int newNote = mv->midivoiceNotestackItems[noteNumber].note;

            // now check for oneshot mode: if note is 0, or if note counter and oct counter is 0, stop here
            if( !firstNoteReq && arpOneshotMode &&
                (!newNote || (noteNumber == 0 && arpOctaveCtr == 0)) ) {
                // set note counter to 0xaa to stop ARP until next reset
                arpNoteCtr = 0xaa;
                // don't play new note
                newNote = 0;
            }

            // only play if note is not zero
            if( newNote ) {
                // if first note has been selected, increase octave until max value is reached
                if( firstNoteReq )
                    arpOctaveCtr = 0;
                else if( noteNumber == 0 ) {
                    ++arpOctaveCtr;
                }
                if( arpOctaveCtr > arpOctaveRange )
                    arpOctaveCtr = 0;

                // transpose note
                newNote += 12*arpOctaveCtr;

                // saturate octave-wise
                while( newNote >= 0x6c ) // use 0x6c instead of 128, since range 0x6c..0x7f sets frequency to 0xffff...
                    newNote -= 12;

                // store new note
                v->voiceNote = newNote;

                // forward gate set request if voice is active and gate not active
                if( v->voiceActive ) {
                    v->voiceGateClrReq = 0; // ensure that gate won't be cleared by previous CLR_REQ
                    if( !v->voiceGateActive ) {
                        // set gate
                        v->voiceGateSetReq = 1;

                        // propagate Note On through trigger matrix
                        mbSidSe->triggerNoteOn(v, 0);
                    }
                }
            }
        }
    }
}


/////////////////////////////////////////////////////////////////////////////
// Arpeggiator Notestack Handling
// Note On
/////////////////////////////////////////////////////////////////////////////
void MbSidArp::noteOn(MbSidVoice *v, u8 note, u8 velocity)
{
    MbSidMidiVoice *mv = (MbSidMidiVoice *)v->midiVoicePtr;

    // store current notestack mode
    notestack_mode_t saved_mode = mv->midivoiceNotestack.mode;

    if( arpEasyChordMode && !arpHoldMode ) {
        // easy chord entry:
        // even when HOLD mode not active, a note off doesn't remove notes in stack
        // the notes of released keys will be removed from stack once a *new* note is played
        NOTESTACK_RemoveNonActiveNotes(&mv->midivoiceNotestack);
    }

    // if no note is played anymore, clear stack again (so that new notes can be added in HOLD mode)
    if( NOTESTACK_CountActiveNotes(&mv->midivoiceNotestack) == 0 ) {
        // clear stack
        NOTESTACK_Clear(&mv->midivoiceNotestack);
#if 0
        // synchronize the arpeggiator
        // TODO
        mbSidArp[v->voiceNum].restartReq = true;
#endif
    }

    // push note into stack - select mode depending on sort/hold mode
    if( arpHoldMode )
        mv->midivoiceNotestack.mode = arpSortedNotes ? NOTESTACK_MODE_SORT_HOLD : NOTESTACK_MODE_PUSH_TOP_HOLD;
    else
        mv->midivoiceNotestack.mode = arpSortedNotes ? NOTESTACK_MODE_SORT : NOTESTACK_MODE_PUSH_TOP;
    NOTESTACK_Push(&mv->midivoiceNotestack, note, velocity);

    // activate note
    v->voiceActive = 1;

    // remember note
    v->voicePlayedNote = note;

    // restore notestack mode
    mv->midivoiceNotestack.mode = saved_mode;
}


/////////////////////////////////////////////////////////////////////////////
// Arpeggiator Notestack Handling
// Note Off
/////////////////////////////////////////////////////////////////////////////
void MbSidArp::noteOff(MbSidVoice *v, u8 note)
{
    MbSidMidiVoice *mv = (MbSidMidiVoice *)v->midiVoicePtr;

    // store current notestack mode
    notestack_mode_t saved_mode = mv->midivoiceNotestack.mode;

    if( arpEasyChordMode && !arpHoldMode ) {
        // select mode depending on arp flags
        // always pop note in hold mode
        mv->midivoiceNotestack.mode = arpSortedNotes ? NOTESTACK_MODE_SORT_HOLD : NOTESTACK_MODE_PUSH_TOP_HOLD;
    } else {
        // select mode depending on arp flags
        if( arpHoldMode )
            mv->midivoiceNotestack.mode = arpSortedNotes ? NOTESTACK_MODE_SORT_HOLD : NOTESTACK_MODE_PUSH_TOP_HOLD;
        else
            mv->midivoiceNotestack.mode = arpSortedNotes ? NOTESTACK_MODE_SORT : NOTESTACK_MODE_PUSH_TOP;
    }

    // remove note from stack
    NOTESTACK_Pop(&mv->midivoiceNotestack, note);

    // release voice if no note in queue anymore
    if( NOTESTACK_CountActiveNotes(&mv->midivoiceNotestack) == 0 )
        v->voiceActive = 0;

    // restore notestack mode
    mv->midivoiceNotestack.mode = saved_mode;
}

