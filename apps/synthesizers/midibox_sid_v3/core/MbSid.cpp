/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
// $Id$
/*
 * MIDIbox SID Unit
 *
 * ==========================================================================
 *
 *  Copyright (C) 2010 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#include "MbSid.h"


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

#define SID_BANK_NUM 1  // currently only a single bank is available in ROM
#include "sid_bank_preset_a.inc"



/////////////////////////////////////////////////////////////////////////////
// Constructor
/////////////////////////////////////////////////////////////////////////////
MbSid::MbSid()
{
    for(int i=0; i<SID_SE_KNOB_NUM; ++i) {
        knob[i].knobNum = i;
        knob[i].mbSidSePtr = this;
    }        
}


/////////////////////////////////////////////////////////////////////////////
// Destructor
/////////////////////////////////////////////////////////////////////////////
MbSid::~MbSid()
{
}


/////////////////////////////////////////////////////////////////////////////
// Test function
/////////////////////////////////////////////////////////////////////////////
void MbSid::sendAlive(void)
{
    MIOS32_MIDI_SendDebugMessage("MbSid%d is alive!\n", sidNum);
}


/////////////////////////////////////////////////////////////////////////////
// Write a patch
/////////////////////////////////////////////////////////////////////////////
s32 MbSid::bankWrite(MbSidPatch *p)
{
    return -1; // not supported yet
}


/////////////////////////////////////////////////////////////////////////////
// Read a patch
/////////////////////////////////////////////////////////////////////////////
s32 MbSid::patchRead(MbSidPatch *p)
{
    if( p->bankNum >= SID_BANK_NUM )
        return -1; // invalid bank

    if( p->patchNum >= 128 )
        return -2; // invalid patch

#if DEBUG_VERBOSE_LEVEL >= 2
    DEBUG_MSG("[SID_BANK_PatchRead] SID %d reads patch %c%03d\n", sidNum, 'A'+p->bankNum, p->patchNum);
#endif

    switch( p->bankNum ) {
    case 0: {
        sid_patch_t *bankPatch = (sid_patch_t *)sid_bank_preset_0[p->patchNum];
        p->copyToPatch(bankPatch);
    } break;

    default:
        return -3; // no bank in ROM
    }

    return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// returns the name of a preset patch as zero-terminated C string (17 chars)
/////////////////////////////////////////////////////////////////////////////
s32 MbSid::patchNameGet(u8 bank, u8 patch, char *buffer)
{
    int i;
    sid_patch_t *p;

    if( bank >= SID_BANK_NUM ) {
        sprintf(buffer, "<Invalid Bank %c>", 'A'+bank);
        return -1; // invalid bank
    }

    if( patch >= 128 ) {
        sprintf(buffer, "<WrongPatch %03d>", patch);
        return -2; // invalid patch
  }

    switch( bank ) {
    case 0:
        p = (sid_patch_t *)&sid_bank_preset_0[patch];
        break;

    default:
        sprintf(buffer, "<Empty Bank %c>  ", 'A'+bank);
        return -3; // no bank in ROM
    }

    for(i=0; i<16; ++i)
        buffer[i] = p->name[i] >= 0x20 ? p->name[i] : ' ';
    buffer[i] = 0;

    return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Receives a MIDI package
/////////////////////////////////////////////////////////////////////////////
void MbSid::midiReceive(mios32_midi_package_t midi_package)
{
    switch( midi_package.event ) {
    case NoteOff:
        midiReceiveNote(midi_package.chn, midi_package.note, 0x00);
        break;

    case NoteOn:
        midiReceiveNote(midi_package.chn, midi_package.note, midi_package.velocity);
        break;

    case CC:
        midiReceiveCC(midi_package.chn, midi_package.cc_number, midi_package.value);
        break;

    case PitchBend: {
        u16 pitchbend_value_14bit = (midi_package.evnt2 << 7) | (midi_package.evnt1 & 0x7f);
        midiReceivePitchBend(midi_package.chn, pitchbend_value_14bit);
    } break;

    case ProgramChange:
        midiProgramChange(midi_package.chn, midi_package.evnt1);
        break;
  }
}


/////////////////////////////////////////////////////////////////////////////
// Receives a MIDI note
/////////////////////////////////////////////////////////////////////////////
void MbSid::midiReceiveNote(u8 chn, u8 note, u8 velocity)
{
    sid_se_engine_t engine = (sid_se_engine_t)mbSidPatch.body.engine;

    if( engine != SID_SE_LEAD )
        return; // currently only lead engine supported

    MbSidVoice *v = &mbSidVoice[0];
    sid_se_midi_voice_t *mv = (sid_se_midi_voice_t *)v->midiVoice;
    sid_patch_t *p = &mbSidPatch.body;
    sid_se_v_flags_t v_flags;
    v_flags.ALL = p->L.v_flags;

    // check if MIDI channel and splitzone matches
    if( chn != mv->midi_channel ||
        note < mv->split_lower || (mv->split_upper && note > mv->split_upper) )
        return; // note filtered

    // operation must be atomic!
    MIOS32_IRQ_Disable();

    if( velocity ) { // Note On
        // copy velocity into mod matrix source
        knob[SID_KNOB_VELOCITY].set(velocity << 1);

        // go through all voices
        int voice;
        for(voice=0; voice<6; ++voice, ++v) {
            sid_se_midi_voice_t *mv = (sid_se_midi_voice_t *)v->midiVoice;
            sid_se_voice_arp_mode_t arp_mode;
            arp_mode.ALL = v->voicePatch->arp_mode;

            // push note into WT stack
            SID_MIDI_PushWT(mv, note);
#if DEBUG_VERBOSE_LEVEL >= 2
            if( mv->midi_voice == 0 )
                DEBUG_MSG("WT_STACK>%02x %02x %02x %02x\n", mv->wt_stack[0], mv->wt_stack[1], mv->wt_stack[2], mv->wt_stack[3]);
#endif

            // branch depending on normal/arp mode
            if( arp_mode.ENABLE ) {
                // call Arp Note On Handler
                SID_MIDI_ArpNoteOn(v, note, velocity);
            } else {
                // push note into stack
                NOTESTACK_Push(&mv->notestack, note, velocity);

                // switch off gate if not in legato or WTO mode
                if( !v_flags.LEGATO && !v_flags.WTO )
                    SID_MIDI_GateOff(v, mv, note);

                // call Note On Handler
                SID_MIDI_NoteOn(v, note, velocity, v_flags);
            }
        }
    } else { // Note Off
        // go through all voices
        int voice;
        MbSidVoice *v = &mbSidVoice[0];
        for(voice=0; voice<6; ++voice, ++v) {
            sid_se_midi_voice_t *mv = (sid_se_midi_voice_t *)v->midiVoice;
            sid_se_voice_arp_mode_t arp_mode;
            arp_mode.ALL = v->voicePatch->arp_mode;

            // pop from WT stack if sustain not active (TODO: sustain switch)
            SID_MIDI_PopWT(mv, note);
#if DEBUG_VERBOSE_LEVEL >= 2
            if( mv->midi_voice == 0 )
                DEBUG_MSG("WT_STACK<%02x %02x %02x %02x\n", mv->wt_stack[0], mv->wt_stack[1], mv->wt_stack[2], mv->wt_stack[3]);
#endif

            // TODO: if sustain active: mark note, but don't stop it (will work in normal and arp mode)
            // if not in arp mode: sustain only relevant if only one active note in stack

            // branch depending on normal/arp mode
            if( arp_mode.ENABLE ) {
                // call Arp Note Off Handler
                SID_MIDI_ArpNoteOff(v, note);
            } else {
                u8 last_first_note = mv->notestack.note_items[0].note;
                // pop note from stack
                if( NOTESTACK_Pop(&mv->notestack, note) > 0 ) {
                    // call Note Off Handler
                    if( SID_MIDI_NoteOff(v, note, last_first_note, v_flags) > 0 ) // retrigger requested?
                        SID_MIDI_NoteOn(v, mv->notestack.note_items[0].note, mv->notestack.note_items[0].tag, v_flags); // new note
                }
            }
        }
    }

    MIOS32_IRQ_Enable();
}


/////////////////////////////////////////////////////////////////////////////
// Receives a MIDI CC
/////////////////////////////////////////////////////////////////////////////
void MbSid::midiReceiveCC(u8 chn, u8 cc_number, u8 value)
{
    // knob values are available for all engines
    if( chn == MbSidSe::mbSidVoice[0].midiVoice->midi_channel ) {
        switch( cc_number ) {
        case  1: knob[SID_KNOB_1].set(value << 1); break;
        case 16: knob[SID_KNOB_2].set(value << 1); break;
        case 17: knob[SID_KNOB_3].set(value << 1); break;
        case 18: knob[SID_KNOB_4].set(value << 1); break;
        case 19: knob[SID_KNOB_5].set(value << 1); break;
        }
    }
}


/////////////////////////////////////////////////////////////////////////////
// Receives a Pitchbend Event
/////////////////////////////////////////////////////////////////////////////
void MbSid::midiReceivePitchBend(u8 chn, u16 pitchbend_value_14bit)
{
    u16 pitchbend_value_8bit = pitchbend_value_14bit >> 6;

    // copy pitchbender value into mod matrix source
    knob[SID_KNOB_PITCHBENDER].set(pitchbend_value_8bit);
}


/////////////////////////////////////////////////////////////////////////////
// Receives a Program Change
/////////////////////////////////////////////////////////////////////////////
void MbSid::midiProgramChange(u8 chn, u8 patch)
{
    mbSidPatch.patchNum = patch;
    patchRead(&mbSidPatch);
    updatePatch();
}


/////////////////////////////////////////////////////////////////////////////
// Wavetable Notestack Handling
/////////////////////////////////////////////////////////////////////////////
void MbSid::SID_MIDI_PushWT(sid_se_midi_voice_t *mv, u8 note)
{
    for(int i=0; i<4; ++i) {
        u8 stack_note = mv->wt_stack[i] & 0x7f;
        u8 push_stack = 0;

        if( !stack_note ) { // last entry?
            push_stack = 1;
        } else {
            // ignore if note is already in stack
            if( stack_note == note )
                return;
            // push into stack if note >= current note
            if( stack_note >= note )
                push_stack = 1;
        }

        if( push_stack ) {
            if( i != 3 ) { // max note: no shift required
                for(int j=3; j>i; --j)
                    mv->wt_stack[j] = mv->wt_stack[j-1];
            }

            // insert note
            mv->wt_stack[i] = note;

            return;
        }
    }

    return;
}

void MbSid::SID_MIDI_PopWT(sid_se_midi_voice_t *mv, u8 note)
{
    // search for note entry with the same number, erase it and push the entries behind
    for(int i=0; i<4; ++i) {
        u8 stack_note = mv->wt_stack[i] & 0x7f;

        if( note == stack_note ) {
            // push the entries behind the found entry
            if( i != 3 ) {
                for(int j=i; j<3; ++j)
                    mv->wt_stack[j] = mv->wt_stack[j+1];
            }

            // clear last entry
            mv->wt_stack[3] = 0;
        }
    }
}


/////////////////////////////////////////////////////////////////////////////
// Arpeggiator Notestack Handling
/////////////////////////////////////////////////////////////////////////////
void MbSid::SID_MIDI_ArpNoteOn(MbSidVoice *v, u8 note, u8 velocity)
{
    sid_se_midi_voice_t *mv = (sid_se_midi_voice_t *)v->midiVoice;
    sid_se_voice_arp_mode_t arp_mode;
    arp_mode.ALL = v->voicePatch->arp_mode;
    sid_se_voice_arp_speed_div_t arp_speed_div;
    arp_speed_div.ALL = v->voicePatch->arp_speed_div;

    // store current notestack mode
    notestack_mode_t saved_mode = mv->notestack.mode;

    if( arp_speed_div.EASY_CHORD && !arp_mode.HOLD ) {
        // easy chord entry:
        // even when HOLD mode not active, a note off doesn't remove notes in stack
        // the notes of released keys will be removed from stack once a *new* note is played
        NOTESTACK_RemoveNonActiveNotes(&mv->notestack);
    }

    // if no note is played anymore, clear stack again (so that new notes can be added in HOLD mode)
    if( NOTESTACK_CountActiveNotes(&mv->notestack) == 0 ) {
        // clear stack
        NOTESTACK_Clear(&mv->notestack);
        // synchronize the arpeggiator
        mbSidArp[v->voiceNum].restartReq = true;
    }

    // push note into stack - select mode depending on sort/hold mode
    if( arp_mode.HOLD )
        mv->notestack.mode = arp_mode.SORTED ? NOTESTACK_MODE_SORT_HOLD : NOTESTACK_MODE_PUSH_TOP_HOLD;
    else
        mv->notestack.mode = arp_mode.SORTED ? NOTESTACK_MODE_SORT : NOTESTACK_MODE_PUSH_TOP;
    NOTESTACK_Push(&mv->notestack, note, velocity);

    // activate note
    v->voiceActive = 1;

    // remember note
    v->playedNote = note;

    // restore notestack mode
    mv->notestack.mode = saved_mode;
}


void MbSid::SID_MIDI_ArpNoteOff(MbSidVoice *v, u8 note)
{
    sid_se_midi_voice_t *mv = (sid_se_midi_voice_t *)v->midiVoice;
    sid_se_voice_arp_mode_t arp_mode;
    arp_mode.ALL = v->voicePatch->arp_mode;
    sid_se_voice_arp_speed_div_t arp_speed_div;
    arp_speed_div.ALL = v->voicePatch->arp_speed_div;

    // store current notestack mode
    notestack_mode_t saved_mode = mv->notestack.mode;

    if( arp_speed_div.EASY_CHORD && !arp_mode.HOLD ) {
        // select mode depending on arp flags
        // always pop note in hold mode
        mv->notestack.mode = arp_mode.SORTED ? NOTESTACK_MODE_SORT_HOLD : NOTESTACK_MODE_PUSH_TOP_HOLD;
    } else {
        // select mode depending on arp flags
        if( arp_mode.HOLD )
            mv->notestack.mode = arp_mode.SORTED ? NOTESTACK_MODE_SORT_HOLD : NOTESTACK_MODE_PUSH_TOP_HOLD;
        else
            mv->notestack.mode = arp_mode.SORTED ? NOTESTACK_MODE_SORT : NOTESTACK_MODE_PUSH_TOP;
    }

    // remove note from stack
    NOTESTACK_Pop(&mv->notestack, note);

    // release voice if no note in queue anymore
    if( NOTESTACK_CountActiveNotes(&mv->notestack) == 0 )
        v->voiceActive = 0;

    // restore notestack mode
    mv->notestack.mode = saved_mode;
}

/////////////////////////////////////////////////////////////////////////////
// Normal Note Handling (valid for Lead/Bassline/Multi engine)
/////////////////////////////////////////////////////////////////////////////
void MbSid::SID_MIDI_NoteOn(MbSidVoice *v, u8 note, u8 velocity, sid_se_v_flags_t v_flags)
{
    sid_se_midi_voice_t *mv = (sid_se_midi_voice_t *)v->midiVoice;

    // save note
    v->playedNote = mv->notestack.note_items[0].note;
    if( !v_flags.WTO )
        v->note = mv->notestack.note_items[0].note;

    // ensure that note is not depressed anymore
    // TODO n->note_items[0].depressed = 0;

    if( v_flags.SUSKEY ) {
        // in SusKey mode, we activate portamento only if at least two keys are played
        // omit portamento if first key played after patch initialisation
        if( mv->notestack.len >= 2 && v->portaInitialized )
            v->portaActive = 1;
    } else {
        // portamento always activated (will immediately finish if portamento value = 0)
        v->portaActive = 1;
    }

    // next key will allow portamento
    v->portaInitialized = 1;

    // gate bit handling
    if( !v_flags.LEGATO || !v->voiceActive ) {
        // enable gate
        SID_MIDI_GateOn(v);

        // set accent flag depending on velocity (set when velocity >= 0x40)
        v->accent = (velocity >= 0x40) ? 1 : 0;

        // trigger matrix
        triggerNoteOn(v, 0);
    }
}

bool MbSid::SID_MIDI_NoteOff(MbSidVoice *v, u8 note, u8 last_first_note, sid_se_v_flags_t v_flags)
{
    sid_se_midi_voice_t *mv = (sid_se_midi_voice_t *)v->midiVoice;

    // if there is still a note in the stack, play new note with NoteOn Function (checked by caller)
    if( mv->notestack.len && !v_flags.POLY ) {
        // if not in legato mode and current note-off number equat to last entry #0: gate off
        if( !v_flags.LEGATO && note == last_first_note )
            SID_MIDI_GateOff(v, mv, note);

        // activate portamento (will be ignored by pitch handler if no portamento active - important for SusKey function to have it here!)
        v->portaActive = 1;
        return true; // request Note On!
    }

    // request gate clear bit
    SID_MIDI_GateOff(v, mv, note);

    // trigger matrix
    triggerNoteOff(v, 0);

    return false; // NO note on!
}


/////////////////////////////////////////////////////////////////////////////
// Gate Handling
/////////////////////////////////////////////////////////////////////////////
void MbSid::SID_MIDI_GateOn(MbSidVoice *v)
{
    v->voiceActive = 1;
    // gate requested via trigger matrix
}


void MbSid::SID_MIDI_GateOff_SingleVoice(MbSidVoice *v)
{
    if( v->voiceActive ) {
        v->voiceActive = 0;
        v->gateSetReq = 0;

        // request gate off if not disabled via trigger matrix
        if( v->trgMaskNoteOff ) {
            u8 *trgMaskNoteOff = (u8 *)&v->trgMaskNoteOff->ALL;
            if( trgMaskNoteOff[0] & (1 << v->voiceNum) )
                v->gateClrReq = 1;
        } else
            v->gateClrReq = 1;

        // remove gate set request
        v->noteRestartReq = 0;
    }
}


void MbSid::SID_MIDI_GateOff(MbSidVoice *v, sid_se_midi_voice_t *mv, u8 note)
{
    sid_se_engine_t engine = (sid_se_engine_t)mbSidPatch.body.engine;
    if( engine == SID_SE_MULTI ) {
        // go through all voices which are assigned to the current instrument and note
        u8 instrument = mv->midi_voice;
        MbSidVoice *v_i = (MbSidVoice *)&mbSidVoice[0];
        int voice;
        for(voice=0; voice<SID_SE_NUM_VOICES; ++voice, ++v_i)
            if( v_i->assignedInstrument == instrument && v_i->playedNote == note ) {
                SID_MIDI_GateOff_SingleVoice(v_i);
                voiceQueue.release(voice);
            }
    } else {
        SID_MIDI_GateOff_SingleVoice(v);
    }
}
