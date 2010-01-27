/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
// $Id$
/*
 * MIDIbox SID Sound Engine Toplevel
 *
 * ==========================================================================
 *
 *  Copyright (C) 2010 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#include "MbSidSe.h"
#include "MbSidTables.h"
#include <string.h>


/////////////////////////////////////////////////////////////////////////////
// for optional debugging messages via DEBUG_MSG (defined in mios32_config.h)
// should be at least 1 for sending error messages
/////////////////////////////////////////////////////////////////////////////
#define DEBUG_VERBOSE_LEVEL 1


/////////////////////////////////////////////////////////////////////////////
// Constructor
/////////////////////////////////////////////////////////////////////////////
MbSidSe::MbSidSe()
{
    physSidL = 0;
    physSidR = 1;
    updateSpeedFactor = 1;
    updatePatch();

    // note: this initialisation would be more secure if we would use new(),
    // such as mbSidPar = new MbSidPar(this)... but malloc()s are bad for
    // deterministic applications...
    mbSidPar.mbSidSePtr = this;
}


/////////////////////////////////////////////////////////////////////////////
// Destructor
/////////////////////////////////////////////////////////////////////////////
MbSidSe::~MbSidSe()
{
}

/////////////////////////////////////////////////////////////////////////////
// Initialises the structures of a SID sound engine
/////////////////////////////////////////////////////////////////////////////
void MbSidSe::initStructs(void)
{
    int i;
    sid_se_engine_t engine = (sid_se_engine_t)mbSidPatch.body.engine;

    for(i=0; i<SID_SE_NUM_MIDI_VOICES; ++i) {
        u8 midi_voice = i;
        sid_se_midi_voice_t *mv = (sid_se_midi_voice_t *)&sid_se_midi_voice[midi_voice];
        memset(mv, 0, sizeof(sid_se_midi_voice_t));

        mv->midi_voice = midi_voice; // assigned MIDI voice

        NOTESTACK_Init(&mv->notestack,
                       NOTESTACK_MODE_PUSH_TOP,
                       &mv->notestack_items[0],
                       SID_SE_NOTESTACK_SIZE);
        mv->split_upper = 0x7f;
        mv->transpose = 0x40;
        mv->pitchbender = 0x80;

        // tmp.
        switch( engine ) {
        case SID_SE_BASSLINE:
            switch( midi_voice ) {
            case 0:
                mv->midi_channel = 0;
                mv->split_lower = 0x00;
                mv->split_upper = 0x3b;
                break;
  
            case 1:
                mv->midi_channel = 0;
                mv->split_lower = 0x3c;
                mv->split_upper = 0x7f;
                break;
  
            case 2:
                mv->midi_channel = 1;
                mv->split_lower = 0x00;
                mv->split_upper = 0x3b;
                break;
  
            case 3:
                mv->midi_channel = 1;
                mv->split_lower = 0x3c;
                mv->split_upper = 0x7f;
                break;
            }
            break;

        case SID_SE_MULTI:
            mv->midi_channel = midi_voice;
	
        case SID_SE_LEAD:
        case SID_SE_DRUM:
            break; // not relevant
        }
    }

    MbSidVoice *v = &mbSidVoice[0];
    for(int voice=0; voice<SID_SE_NUM_VOICES; ++voice, ++v) {
        u8 physVoice = voice % 3;
        sid_voice_t *physSidVoice;
        if( voice >= 3 )
            physSidVoice = (sid_voice_t *)&sidRegRPtr->v[physVoice];
        else
            physSidVoice = (sid_voice_t *)&sidRegLPtr->v[physVoice];

        switch( engine ) {
        case SID_SE_BASSLINE:
            v->init((sid_se_voice_patch_t *)&mbSidPatch.body.B.voice[(voice < 3) ? 0 : 1],
                    voice, physVoice, physSidVoice,
                    (sid_se_midi_voice_t *)&sid_se_midi_voice[(voice < 3) ? 0 : 1]);
            v->trgMaskNoteOn = NULL;
            v->trgMaskNoteOff = NULL;
            break;

        case SID_SE_DRUM:
            v->init(NULL, // dynamically assigned
                    voice, physVoice, physSidVoice,
                    (sid_se_midi_voice_t *)&sid_se_midi_voice[0]); // always use first midi voice
            v->trgMaskNoteOn = NULL;
            v->trgMaskNoteOff = NULL;
            break;

        case SID_SE_MULTI:
            v->init((sid_se_voice_patch_t *)&mbSidPatch.body.M.voice[0], // dynamically assigned
                    voice, physVoice, physSidVoice,
                    (sid_se_midi_voice_t *)&sid_se_midi_voice[0]); // dynamically assigned
            v->trgMaskNoteOn = NULL;
            v->trgMaskNoteOff = NULL;
            break;

        default: // SID_SE_LEAD
            v->init((sid_se_voice_patch_t *)&mbSidPatch.body.L.voice[voice],
                    voice, physVoice, physSidVoice,
                    (sid_se_midi_voice_t *)&sid_se_midi_voice[voice]);
            v->trgMaskNoteOn = (sid_se_trg_t *)&mbSidPatch.body.L.trg_matrix[SID_SE_TRG_NOn];
            v->trgMaskNoteOff = (sid_se_trg_t *)&mbSidPatch.body.L.trg_matrix[SID_SE_TRG_NOff];
        }

        v->modDstPitch = &mbSidMod.modDst[SID_SE_MOD_DST_PITCH1 + voice];
        v->modDstPw = &mbSidMod.modDst[SID_SE_MOD_DST_PW1 + voice];
    }

    for(i=0; i<SID_SE_NUM_FILTERS; ++i) {
        u8 filter = i;
        sid_se_filter_t *f = (sid_se_filter_t *)&sid_se_filter[filter];
        memset(f, 0, sizeof(sid_se_filter_t));

        f->engine = engine; // engine type
        f->filter = filter; // cross-reference to assigned filter
        if( filter ) {
            f->phys_sid = 1; // reference to physical SID (chip)
            f->phys_sid_regs = sidRegRPtr;
        } else {
            f->phys_sid = 0; // reference to physical SID (chip)
            f->phys_sid_regs = sidRegLPtr;
        }
        f->filter_patch = (sid_se_filter_patch_t *)&mbSidPatch.body.L.filter[filter];
        f->mod_dst_filter = &mbSidMod.modDst[SID_SE_MOD_DST_FIL1 + filter];
        f->mod_dst_volume = &mbSidMod.modDst[SID_SE_MOD_DST_VOL1 + filter];
    }

    MbSidLfo *l = &mbSidLfo[0];
    for(int lfo=0; lfo<SID_SE_NUM_LFO; ++lfo, ++l) {
        switch( engine ) {
        case SID_SE_BASSLINE: {
            u8 patch_voice = (lfo >= 2) ? 1 : 0;
            u8 mod_voice = patch_voice * 3;
            u8 lfo_voice = lfo & 1;
            sid_se_voice_patch_t *voicePatch = (sid_se_voice_patch_t *)&mbSidPatch.body.B.voice[patch_voice];
            l->init((sid_se_lfo_patch_t *)&voicePatch->B.lfo[lfo_voice]);

            l->modDstPitch = &mbSidMod.modDst[SID_SE_MOD_DST_PITCH1 + mod_voice];
            l->modDstPw = &mbSidMod.modDst[SID_SE_MOD_DST_PW1 + mod_voice];
            l->modDstFilter = &mbSidMod.modDst[SID_SE_MOD_DST_FIL1 + patch_voice];
        } break;

        case SID_SE_DRUM:
            // not used
            l->init(NULL);
            break;

        case SID_SE_MULTI: {
            u8 mod_voice = lfo >> 1;
            u8 lfo_voice = lfo & 1;
            sid_se_voice_patch_t *voicePatch = (sid_se_voice_patch_t *)&mbSidPatch.body.M.voice[0];
            l->init((sid_se_lfo_patch_t *)&voicePatch->M.lfo[lfo_voice]);
            l->modDstPitch = &mbSidMod.modDst[SID_SE_MOD_DST_PITCH1 + mod_voice];
            l->modDstPw = &mbSidMod.modDst[SID_SE_MOD_DST_PW1 + mod_voice];
            l->modDstFilter = &mbSidMod.modDst[(mod_voice >= 3) ? SID_SE_MOD_DST_FIL2 : SID_SE_MOD_DST_FIL1];
        } break;

        default: // SID_SE_LEAD
            l->init((sid_se_lfo_patch_t *)&mbSidPatch.body.L.lfo[lfo]);
            l->modSrcLfo = &mbSidMod.modSrc[SID_SE_MOD_SRC_LFO1 + lfo];
            l->modDstLfoDepth = &mbSidMod.modDst[SID_SE_MOD_DST_LD1 + lfo];
            l->modDstLfoRate = &mbSidMod.modDst[SID_SE_MOD_DST_LR1 + lfo];
        }
    }

    MbSidEnv *e = &mbSidEnv[0];
    for(int env=0; env<SID_SE_NUM_ENV; ++env, ++e) {
        switch( engine ) {
        case SID_SE_BASSLINE: {
            u8 patch_voice = env ? 1 : 0;
            u8 mod_voice = patch_voice * 3;
            sid_se_voice_patch_t *voicePatch = (sid_se_voice_patch_t *)&mbSidPatch.body.B.voice[patch_voice];
            e->init((sid_se_env_patch_t *)&voicePatch->B.env);
            e->modDstPitch = &mbSidMod.modDst[SID_SE_MOD_DST_PITCH1 + mod_voice];
            e->modDstPw = &mbSidMod.modDst[SID_SE_MOD_DST_PW1 + mod_voice];
            e->modDstFilter = &mbSidMod.modDst[SID_SE_MOD_DST_FIL1 + patch_voice];
            e->decayA = &voicePatch->B.env_decay_a;
            e->accent = &voicePatch->accent;
        } break;

        case SID_SE_DRUM:
            // not used
            e->init(NULL);
            break;

        case SID_SE_MULTI: {
            u8 mod_voice = env;
            sid_se_voice_patch_t *voicePatch = (sid_se_voice_patch_t *)&mbSidPatch.body.M.voice[0];
            e->init((sid_se_env_patch_t *)&voicePatch->M.env);
            e->modDstPitch = &mbSidMod.modDst[SID_SE_MOD_DST_PITCH1 + mod_voice];
            e->modDstPw = &mbSidMod.modDst[SID_SE_MOD_DST_PW1 + mod_voice];
            e->modDstFilter = &mbSidMod.modDst[(mod_voice >= 3) ? SID_SE_MOD_DST_FIL2 : SID_SE_MOD_DST_FIL1];
            e->decayA = &voicePatch->B.env_decay_a;
            e->accent = &voicePatch->accent;
        } break;

        default: // SID_SE_LEAD
            e->init((sid_se_env_patch_t *)&mbSidPatch.body.L.env[env]);
            e->modSrcEnv = &mbSidMod.modSrc[SID_SE_MOD_SRC_ENV1 + env];
        }
    }

    MbSidWt *w = &mbSidWt[0];
    for(int wt=0; wt<SID_SE_NUM_WT; ++wt, ++w) {
        switch( engine ) {
        case SID_SE_BASSLINE:
            // not used
            w->init(NULL, wt, NULL);
            break;

        case SID_SE_DRUM:
            // no reference to patch, w->tick() will be called with drum model as argument
            w->init(NULL, wt, &mbSidPar);
            break;

        case SID_SE_MULTI: {
            sid_se_voice_patch_t *voicePatch = (sid_se_voice_patch_t *)&mbSidPatch.body.M.voice[0];
            w->init((sid_se_wt_patch_t *)&voicePatch->M.wt, wt, &mbSidPar);
            w->wtMemory = (u8 *)mbSidPatch.body.M.wt_memory;
        } break;

        default: // SID_SE_LEAD
            w->init((sid_se_wt_patch_t *)&mbSidPatch.body.L.wt[wt], wt, &mbSidPar);
            w->wtMemory = (u8 *)mbSidPatch.body.L.wt_memory;
        }

        w->modSrcWt = &mbSidMod.modSrc[SID_SE_MOD_SRC_WT1 + wt];
        w->modDstWt = &mbSidMod.modDst[SID_SE_MOD_DST_WT1 + wt];
    }

    MbSidArp *a = &mbSidArp[0];
    for(int arp=0; arp<SID_SE_NUM_ARP; ++arp, ++a)
        a->init();

    for(i=0; i<SID_SE_NUM_SEQ; ++i) {
        u8 seq = i;
        sid_se_seq_t *s = (sid_se_seq_t *)&sid_se_seq[seq];
        memset(s, 0, sizeof(sid_se_seq_t));

        s->seq = seq;

        switch( engine ) {
        case SID_SE_BASSLINE: {
            s->v = &mbSidVoice[seq ? 3 : 0];
            sid_se_voice_patch_t *voice_patch = (sid_se_voice_patch_t *)&mbSidPatch.body.B.voice[seq];
            sid_se_voice_patch_t *voice_patch_shadow = (sid_se_voice_patch_t *)&mbSidPatch.bodyShadow.B.voice[seq];
            s->seq_patch = (sid_se_seq_patch_t *)voice_patch->B.seq;
            s->seq_patch_shadow = (sid_se_seq_patch_t *)voice_patch_shadow->B.seq;
        } break;

        case SID_SE_DRUM:
            // sequencer is used, but these variables are not relevant
            s->v = NULL;
            s->seq_patch = NULL;;
            s->seq_patch_shadow = NULL;;
            break;

        default: // SID_SE_LEAD
            s->v = NULL;
            s->seq_patch = NULL;;
            s->seq_patch_shadow = NULL;;
        }
    }

    // modulation matrix
    switch( engine ) {
    case SID_SE_BASSLINE:
    case SID_SE_DRUM:
    case SID_SE_MULTI:
        mbSidMod.init(NULL); // matrix not part of patch

    default: // SID_SE_LEAD
        mbSidMod.init((sid_se_mod_patch_t *)&mbSidPatch.body.L.mod[0]);
    }
}


/////////////////////////////////////////////////////////////////////////////
// Sound Engine Update Cycle
/////////////////////////////////////////////////////////////////////////////
bool MbSidSe::updateSe(void)
{
    sid_se_engine_t engine = (sid_se_engine_t)mbSidPatch.body.engine;

    // Clear all modulation destinations
    mbSidMod.clearDestinations();

    // Clock
    if( mbSidClockPtr->eventStart ) {
        if( engine == SID_SE_LEAD )
            triggerLead((sid_se_trg_t *)&mbSidPatch.body.L.trg_matrix[SID_SE_TRG_MST]);
        else {
            // reset WTs
            MbSidWt *w = &mbSidWt[0];
            for(int wt=0; wt<SID_SE_NUM_WT; ++wt, ++w)
                w->restartReq = true;

            // reset Arps
            MbSidArp *a = &mbSidArp[0];
            for(int arp=0; arp<SID_SE_NUM_ARP; ++arp, ++a)
                a->restartReq = true;
        }
    }

    if( mbSidClockPtr->eventClock ) {
        if( engine == SID_SE_LEAD )
            triggerLead((sid_se_trg_t *)&mbSidPatch.body.L.trg_matrix[SID_SE_TRG_CLK]);
        else {
            // clock WTs
            MbSidWt *w = &mbSidWt[0];
            for(int wt=0; wt<SID_SE_NUM_WT; ++wt, ++w)
                w->clockReq = true;
        }

        // clock ARPs for all engines (not part of trigger matrix)
        MbSidArp *a = &mbSidArp[0];
        for(int arp=0; arp<SID_SE_NUM_ARP; ++arp, ++a)
            a->clockReq = true;

        // propagate clock/4 event to trigger matrix on each 6th clock
        if( mbSidClockPtr->clkCtr6 == 0 ) {
            if( engine == SID_SE_LEAD )
                triggerLead((sid_se_trg_t *)&mbSidPatch.body.L.trg_matrix[SID_SE_TRG_CL6]);

            // sync LFOs and ENVs
            MbSidLfo *l = &mbSidLfo[0];
            for(int lfo=0; lfo<SID_SE_NUM_LFO; ++lfo, ++l)
                l->syncClockReq = true;

            MbSidEnv *e = &mbSidEnv[0];
            for(int env=0; env<SID_SE_NUM_ENV; ++env, ++e)
                e->syncClockReq = true;
        }

        // propagate clock/16 event to trigger matrix on each 24th clock
        if( mbSidClockPtr->clkCtr24 == 0 ) {
            if( engine == SID_SE_LEAD )
                triggerLead((sid_se_trg_t *)&mbSidPatch.body.L.trg_matrix[SID_SE_TRG_C24]);
        }
    }

    // engine specific code
    switch( engine ) {
    case SID_SE_LEAD: {
        // LFOs
        MbSidLfo *l = &mbSidLfo[0];
        for(int lfo=0; lfo<6; ++lfo, ++l) {
            if( l->tick(engine, updateSpeedFactor) ) // returns true on overrun
                triggerLead((sid_se_trg_t *)&mbSidPatch.body.L.trg_matrix[SID_SE_TRG_L1P + lfo]);
        }

        // ENVs
        MbSidEnv *e = &mbSidEnv[0];
        for(int env=0; env<2; ++env, ++e) {
            if( e->tick(engine, updateSpeedFactor) ) // returns true if sustain phase reached
                triggerLead((sid_se_trg_t *)&mbSidPatch.body.L.trg_matrix[SID_SE_TRG_E1S + env]);
        }

        // Modulation Matrix
        // since this isn't done anywhere else:
        // convert linear frequency of voice1 to 15bit signed value (only positive direction)
        mbSidMod.modSrc[SID_SE_MOD_SRC_KEY] = mbSidVoice[0].linearFrq >> 1;

        // do calculations
        mbSidMod.tick();

        // Wavetables
        MbSidWt *w = &mbSidWt[0];
        for(int wt=0; wt<4; ++wt, ++w)
            w->tick(engine, updateSpeedFactor);

        // Arps
        MbSidVoice *v = &mbSidVoice[0];
        for(int arp=0; arp<6; ++arp, ++v)
            mbSidArp[arp].tick(v, this);

        // Voices
        v = &mbSidVoice[0];
        for(int i=0; i<6; ++i, ++v) {
            if( v->gate(engine, updateSpeedFactor, this) > 0 )
                v->pitch(engine, updateSpeedFactor, this);
            v->pw(engine, updateSpeedFactor, this);
        }

        // Filters and Volume
        sid_se_filter_t *f = &sid_se_filter[0];
        seFilterAndVolume(f++);
        seFilterAndVolume(f);

        // Tmp: copy register values directly into SID registers
        v = &mbSidVoice[0];
        for(int i=0; i<6; ++i, ++v) {
            sid_se_voice_waveform_t waveform;
            waveform.ALL = v->voicePatch->waveform;
            v->physSidVoice->waveform = waveform.WAVEFORM;
            v->physSidVoice->sync = waveform.SYNC;
            v->physSidVoice->ringmod = waveform.RINGMOD;

            // don't change ADSR so long delay is active (also important for ABW - ADSR bug workaround)
            if( !v->setDelayCtr ) {
                v->physSidVoice->ad = v->voicePatch->ad;
                v->physSidVoice->sr = v->voicePatch->sr;
            }
        }
    } break;


    case SID_SE_BASSLINE: {
        // LFOs
        MbSidLfo *l = &mbSidLfo[0];
        for(int lfo=0; lfo<2*2; ++lfo, ++l)
            l->tick(engine, updateSpeedFactor);

        // ENVs
        MbSidEnv *e = &mbSidEnv[0];
        for(int env=0; env<2; ++env, ++e) {
            e->accentReq = mbSidVoice[3*env].accent;
            e->tick(engine, updateSpeedFactor);
        }

        // Sequencer
        sid_se_seq_t *s = &sid_se_seq[0];
        for(int i=0; i<2; ++i, ++s)
            seSeqBassline(s);

        // Arps
        MbSidVoice *v = &mbSidVoice[0];
        for(int arp=0; arp<6; arp+=3, v+=3)
            mbSidArp[arp].tick(v, this);

        // Voices
        // transfer note values and state flags from OSC1 (master) to OSC2/3 (slaves)
        for(int i=0; i<6; i+=3) {
            // TODO: provide "copy voice" method
            mbSidVoice[i+1].voiceActive = mbSidVoice[i+2].voiceActive = mbSidVoice[i+0].voiceActive;
            mbSidVoice[i+1].voiceDisabled = mbSidVoice[i+2].voiceDisabled = mbSidVoice[i+0].voiceDisabled;
            mbSidVoice[i+1].gateActive = mbSidVoice[i+2].gateActive = mbSidVoice[i+0].gateActive;
            mbSidVoice[i+1].gateSetReq = mbSidVoice[i+2].gateSetReq = mbSidVoice[i+0].gateSetReq;
            mbSidVoice[i+1].gateClrReq = mbSidVoice[i+2].gateClrReq = mbSidVoice[i+0].gateClrReq;
            mbSidVoice[i+1].portaActive = mbSidVoice[i+2].portaActive = mbSidVoice[i+0].portaActive;
            mbSidVoice[i+1].accent = mbSidVoice[i+2].accent = mbSidVoice[i+0].accent;
            mbSidVoice[i+1].slide = mbSidVoice[i+2].slide = mbSidVoice[i+0].slide;
            // etc...
            mbSidVoice[i+1].note = mbSidVoice[i+2].note = mbSidVoice[i+0].note;
            mbSidVoice[i+1].arpNote = mbSidVoice[i+2].arpNote = mbSidVoice[i+0].arpNote;
            mbSidVoice[i+1].playedNote = mbSidVoice[i+2].playedNote = mbSidVoice[i+0].playedNote;
        }

        // process voices
        v = &mbSidVoice[0];
        for(int i=0; i<6; ++i, ++v) {
            if( v->gate(engine, updateSpeedFactor, this) )
                v->pitch(engine, updateSpeedFactor, this);
            v->pw(engine, updateSpeedFactor, this);
        }

        // Filters and Volume
        sid_se_filter_t *f = &sid_se_filter[0];
        seFilterAndVolume(f++);
        seFilterAndVolume(f);

        // Tmp: copy register values directly into SID registers
        for(int i=0; i<6; i+=3) {
            MbSidVoice *v1 = &mbSidVoice[i+0];
            MbSidVoice *v2 = &mbSidVoice[i+1];
            MbSidVoice *v3 = &mbSidVoice[i+2];

            // Voice 1 oscillator
            sid_se_voice_waveform_t v1_waveform;
            v1_waveform.ALL = v1->voicePatch->waveform;
            v1->physSidVoice->waveform = v1_waveform.WAVEFORM;
            v1->physSidVoice->sync = v1_waveform.SYNC;
            v1->physSidVoice->ringmod = v1_waveform.RINGMOD;

            // Voice 2 oscillator has own waveform and voice enable flag
            // no gate/sync if waveform disabled
            sid_se_voice_waveform_t v2_waveform;
            v2_waveform.ALL = v2->voicePatch->B.v2_waveform;
            if( !v2_waveform.WAVEFORM ) {
                v2->physSidVoice->waveform_reg = 0x10; // select triangle waveform to keep the oscillator silent
            } else {
                v2->physSidVoice->waveform = v2_waveform.WAVEFORM;
                v2->physSidVoice->sync = v2_waveform.SYNC;
                v2->physSidVoice->ringmod = v2_waveform.RINGMOD;
            }

            // Voice 3 oscillator has own waveform and voice enable flag
            // no gate/sync if waveform disabled
            sid_se_voice_waveform_t v3_waveform;
            v3_waveform.ALL = v3->voicePatch->B.v3_waveform;
            if( !v3_waveform.WAVEFORM ) {
                v3->physSidVoice->waveform_reg = 0x10; // select triangle waveform to keep the oscillator silent
            } else {
                v3->physSidVoice->waveform = v3_waveform.WAVEFORM;
                v3->physSidVoice->sync = v3_waveform.SYNC;
                v3->physSidVoice->ringmod = v3_waveform.RINGMOD;
            }

            // don't change ADSR so long delay is active (also important for ABW - ADSR bug workaround)
            if( !v->setDelayCtr ) {
                u8 ad = v1->voicePatch->ad;
                u8 sr = v1->voicePatch->sr;

                // force sustain to maximum if accent flag active
                if( v1->accent )
                    sr |= 0xf0;

                v1->physSidVoice->ad = ad;
                v1->physSidVoice->sr = sr;
                v2->physSidVoice->ad = ad;
                v2->physSidVoice->sr = sr;
                v3->physSidVoice->ad = ad;
                v3->physSidVoice->sr = sr;
            }
        }
    } break;

    case SID_SE_DRUM: {
        // Sequencer
        sid_se_seq_t *s = &sid_se_seq[0];
        seSeqDrum(s);


        // Voices
        // process voices
        MbSidVoice *v = &mbSidVoice[0];
        MbSidWt *w = &mbSidWt[0];
        for(int i=0; i<6; ++i, ++v, ++w) {
            // handle drum model with wavetable sequencer
            // wait until initial gate delay has passed (for ABW feature)
            // TK: checking for v->setDelayCtr here is probably not a good idea,
            // it leads to jitter. However, we let it like it is for compatibility reasons with V2
            if( !v->setDelayCtr )
                w->tick(engine, updateSpeedFactor, v->drumModel, v->note, v->drumWaveform);

            // handle gate
            if( v->gate(engine, updateSpeedFactor, this) )
                v->pitchDrum(engine, updateSpeedFactor, this);

            // control the delayed gate clear request
            if( v->gateActive && !v->setDelayCtr && v->clrDelayCtr ) {
                int clrDelayCtr = v->clrDelayCtr + mbSidEnvTable[v->drumGatelength] / updateSpeedFactor;
                if( clrDelayCtr >= 0xffff ) {
                    // overrun: clear counter to disable delay
                    v->clrDelayCtr = 0;
                    // request gate clear and deactivate voice active state (new note can be played)
                    v->gateSetReq = 0;
                    v->gateClrReq = 1;
                    v->voiceActive = 0;
                } else
                    v->clrDelayCtr = clrDelayCtr;
            }

            // Pulsewidth handler
            v->pwDrum(engine, updateSpeedFactor, this);
        }

        // Filters and Volume
        sid_se_filter_t *f = &sid_se_filter[0];
        seFilterAndVolume(f++);
        seFilterAndVolume(f);

        // Tmp: copy register values directly into SID registers
        v = &mbSidVoice[0];
        for(int i=0; i<6; ++i, ++v) {
            // wait until initial gate delay has passed (hard-sync, for ABW feature)
            if( !v->setDelayCtr ) {
                sid_se_voice_waveform_t waveform;
                waveform.ALL = v->drumWaveform;
                v->physSidVoice->test = 0;
                v->physSidVoice->waveform = waveform.WAVEFORM;
                v->physSidVoice->sync = waveform.SYNC;
                v->physSidVoice->ringmod = waveform.RINGMOD;

                if( v->assignedInstrument < 16 ) {
                    sid_se_voice_patch_t *voice_patch = (sid_se_voice_patch_t *)&mbSidPatch.body.D.voice[v->assignedInstrument];

                    u8 ad = voice_patch->D.ad;
                    u8 sr = voice_patch->D.sr;

                    // force sustain to maximum if accent flag active
                    if( v->accent )
                        sr |= 0xf0;

                    v->physSidVoice->ad = ad;
                    v->physSidVoice->sr = sr;
                }
            }
        }
    } break;

    case SID_SE_MULTI: {
        // LFOs
        MbSidLfo *l = &mbSidLfo[0];
        for(int lfo=0; lfo<2*6; ++lfo, ++l)
            l->tick(engine, updateSpeedFactor);

        // ENVs
        MbSidEnv *e = &mbSidEnv[0];
        for(int env=0; env<6; ++env, ++e) {
            e->accentReq = mbSidVoice[env].accent;
            e->tick(engine, updateSpeedFactor);
        }

        // Wavetables
        MbSidWt *w = &mbSidWt[0];
        for(int wt=0; wt<6; ++wt, ++w)
            w->tick(engine, updateSpeedFactor);

        // Arps
        // In difference to other engines, each MIDI voice controls an arpeggiator
        // for the last assigned voice
        sid_se_midi_voice_t *mv = &sid_se_midi_voice[0];
        for(int arp=0; arp<6; ++arp, ++mv) {
            MbSidVoice *v = &mbSidVoice[mv->last_voice];
            if( v->assignedInstrument == arp )
                mbSidArp[arp].tick(v, this);
        }


        // Voices
        MbSidVoice *v = &mbSidVoice[0];
        for(int i=0; i<6; ++i, ++v) {
            if( v->gate(engine, updateSpeedFactor, this) )
                v->pitch(engine, updateSpeedFactor, this);
            v->pw(engine, updateSpeedFactor, this);
        }


        // Filters and Volume
        sid_se_filter_t *f = &sid_se_filter[0];
        seFilterAndVolume(f++);
        seFilterAndVolume(f);


        // Tmp: copy register values directly into SID registers
        v = &mbSidVoice[0];
        for(int i=0; i<6; ++i, ++v) {
            sid_se_voice_waveform_t waveform;
            waveform.ALL = v->voicePatch->waveform;
            v->physSidVoice->waveform = waveform.WAVEFORM;
            v->physSidVoice->sync = waveform.SYNC;
            v->physSidVoice->ringmod = waveform.RINGMOD;

            // don't change ADSR so long delay is active (also important for ABW - ADSR bug workaround)
            if( !v->setDelayCtr ) {
                v->physSidVoice->ad = v->voicePatch->ad;
                v->physSidVoice->sr = v->voicePatch->sr;
            }
        }
    } break;

    }

    // currently no detection if SIDs have to be updated
    return true;
}


/////////////////////////////////////////////////////////////////////////////
// Note On/Off Triggers
/////////////////////////////////////////////////////////////////////////////
void MbSidSe::triggerNoteOn(MbSidVoice *v, u8 no_wt)
{
    switch( (sid_se_engine_t)mbSidPatch.body.engine ) {
    case SID_SE_LEAD: {
        sid_se_trg_t trg = *v->trgMaskNoteOn;
        trg.ALL[0] &= 0xc0 | (1 << v->voiceNum); // only the dedicated voice should trigger
        if( no_wt ) // optional ly WT triggers are masked out
            trg.ALL[2] = 0;
        triggerLead(&trg);
    } break;

    case SID_SE_BASSLINE: {
        if( v->voiceNum < 3 ) {
            mbSidVoice[0].noteRestartReq = 1;
            mbSidVoice[1].noteRestartReq = 1;
            mbSidVoice[2].noteRestartReq = 1;

            sid_se_lfo_mode_t lfo_mode;
            for(int lfo=0; lfo<2; ++lfo) {
                lfo_mode.ALL = mbSidLfo[lfo].lfoPatch->mode;
                if( lfo_mode.KEYSYNC )
                    mbSidLfo[lfo].restartReq = true;
            }

            mbSidEnv[0].restartReq = true;
        } else {
            mbSidVoice[3].noteRestartReq = 1;
            mbSidVoice[4].noteRestartReq = 1;
            mbSidVoice[5].noteRestartReq = 1;

            sid_se_lfo_mode_t lfo_mode;
            for(int lfo=2; lfo<4; ++lfo) {
                lfo_mode.ALL = mbSidLfo[lfo].lfoPatch->mode;
                if( lfo_mode.KEYSYNC )
                    mbSidLfo[lfo].restartReq = true;
            }

            mbSidEnv[1].restartReq = true;
        }
    } break;

    case SID_SE_DRUM: {
        v->noteRestartReq = 1;
        mbSidWt[v->voiceNum].restartReq = true;
    } break;

    case SID_SE_MULTI: {
        v->noteRestartReq = 1;

        sid_se_lfo_mode_t lfo_mode;
        int lfo_offset = 2*v->voiceNum;
        for(int lfo=0; lfo<2; ++lfo) {
            lfo_mode.ALL = mbSidLfo[lfo_offset + lfo].lfoPatch->mode;
            if( lfo_mode.KEYSYNC )
                mbSidLfo[lfo_offset + lfo].restartReq = true;
        }

        mbSidEnv[v->voiceNum].restartReq = true;

        //mbSidWt[v->voiceNum].restartReq = true;
        // (no good idea, WTs will quickly get out of sync - see also "A107 Poly Trancegate" patch)
    } break;
    }
}


void MbSidSe::triggerNoteOff(MbSidVoice *v, u8 no_wt)
{
    switch( (sid_se_engine_t)mbSidPatch.body.engine ) {
    case SID_SE_LEAD: {
        sid_se_trg_t trg = *v->trgMaskNoteOff;
        trg.ALL[0] &= 0xc0; // mask out all gate trigger flags
        if( no_wt ) // optionally WT triggers are masked out
            trg.ALL[2] = 0;
        triggerLead(&trg);
    } break;

    case SID_SE_BASSLINE: {
        mbSidEnv[(v->voiceNum >= 3) ? 1 : 0].releaseReq = 1;
    } break;

    case SID_SE_DRUM: {
        // nothing to do
    } break;

    case SID_SE_MULTI: {
        mbSidEnv[v->voiceNum].releaseReq = 1;
    } break;
    }
}


/////////////////////////////////////////////////////////////////////////////
// Input function for trigger matrix of Lead Engine
/////////////////////////////////////////////////////////////////////////////
void MbSidSe::triggerLead(sid_se_trg_t *trg)
{
    if( trg->ALL[0] ) {
        if( trg->O1L ) mbSidVoice[0].noteRestartReq = true;
        if( trg->O2L ) mbSidVoice[1].noteRestartReq = true;
        if( trg->O3L ) mbSidVoice[2].noteRestartReq = true;
        if( trg->O1R ) mbSidVoice[3].noteRestartReq = true;
        if( trg->O2R ) mbSidVoice[4].noteRestartReq = true;
        if( trg->O3R ) mbSidVoice[5].noteRestartReq = true;
        if( trg->E1A ) mbSidEnv[0].restartReq = true;
        if( trg->E2A ) mbSidEnv[1].restartReq = true;
    }

    if( trg->ALL[1] ) {
        if( trg->E1R ) mbSidEnv[0].releaseReq = true;
        if( trg->E2R ) mbSidEnv[1].releaseReq = true;
        if( trg->L1  ) mbSidLfo[0].restartReq = true;
        if( trg->L2  ) mbSidLfo[1].restartReq = true;
        if( trg->L3  ) mbSidLfo[2].restartReq = true;
        if( trg->L4  ) mbSidLfo[3].restartReq = true;
        if( trg->L5  ) mbSidLfo[4].restartReq = true;
        if( trg->L6  ) mbSidLfo[5].restartReq = true;
    }

    if( trg->ALL[2] ) {
        if( trg->W1R ) mbSidWt[0].restartReq = true;
        if( trg->W2R ) mbSidWt[1].restartReq = true;
        if( trg->W3R ) mbSidWt[2].restartReq = true;
        if( trg->W4R ) mbSidWt[3].restartReq = true;
        if( trg->W1S ) mbSidWt[0].clockReq = true;
        if( trg->W2S ) mbSidWt[1].clockReq = true;
        if( trg->W3S ) mbSidWt[2].clockReq = true;
        if( trg->W4S ) mbSidWt[3].clockReq = true;
    }
}


/////////////////////////////////////////////////////////////////////////////
// SID Filter and Volume
/////////////////////////////////////////////////////////////////////////////
void MbSidSe::seFilterAndVolume(sid_se_filter_t *f)
{
    int cutoff = ((f->filter_patch->cutoff_h & 0x0f) << 8) | f->filter_patch->cutoff_l;

    // cutoff modulation (/8 instead of /16 to simplify extreme modulation results)
    cutoff += *f->mod_dst_filter / 8;
    if( cutoff > 0xfff ) cutoff = 0xfff; else if( cutoff < 0 ) cutoff = 0;

    // lead and bassline engine: add keytracking * linear frequency value (of first voice)
    if( f->engine == SID_SE_LEAD || f->engine == SID_SE_BASSLINE ) {
        if( f->filter_patch->keytrack ) {
            s32 linearFrq = mbSidVoice[f->filter*3].linearFrq;
            s32 delta = (linearFrq * f->filter_patch->keytrack) / 0x1000; // 24bit -> 12bit
            // bias at C-3 (0x3c)
            delta -= (0x3c << 5);

            cutoff += delta;
            if( cutoff > 0xfff ) cutoff = 0xfff; else if( cutoff < 0 ) cutoff = 0;
        }
    }

    // calibration
    // TODO: take calibration values from ensemble
    u16 cali_min = 0;
    u16 cali_max = 1536;
    cutoff = cali_min + ((cutoff * (cali_max-cali_min)) / 4096);

    // map 12bit value to 11 value of SID register
    f->phys_sid_regs->filter_l = (cutoff >> 1) & 0x7;
    f->phys_sid_regs->filter_h = (cutoff >> 4);

    // resonance (4bit only)
    int resonance = f->filter_patch->resonance;
    f->phys_sid_regs->resonance = resonance >> 4;

    // filter channel/mode selection
    u8 chn_mode = f->filter_patch->chn_mode;
    f->phys_sid_regs->filter_select = chn_mode & 0xf;
    f->phys_sid_regs->filter_mode = chn_mode >> 4;

    // volume
    int volume = mbSidPatch.body.L.volume << 9;

    // volume modulation
    volume += *f->mod_dst_volume;
    if( volume > 0xffff ) volume = 0xffff; else if( volume < 0 ) volume = 0;

    f->phys_sid_regs->volume = volume >> 12;
}


/////////////////////////////////////////////////////////////////////////////
// Bassline Sequencer
/////////////////////////////////////////////////////////////////////////////
void MbSidSe::seSeqBassline(sid_se_seq_t *s)
{
    MbSidVoice *v = s->v;
    sid_se_v_flags_t v_flags;
    v_flags.ALL = v->voicePatch->B.v_flags;

    // clear gate and deselect sequence if MIDI clock stop requested
    if( mbSidClockPtr->eventStop ) {
        v->voiceActive = 0;
        v->gateClrReq = 1;
        v->gateSetReq = 0;
        triggerNoteOff(v, 0);
        return; // nothing else to do
    }

    // exit if WTO mode not active
    // clear gate if WTO just has been disabled (for proper transitions)
    if( !v_flags.WTO ) {
        // check if sequencer was disabled before
        if( s->state.ENABLED ) {
            // clear gate
            v->gateClrReq = 1;
            v->gateSetReq = 0;
            triggerNoteOff(v, 0);
        }
        s->state.ENABLED = 0;
        return; // nothing else to do
    }

    // exit if arpeggiator is enabled
    sid_se_voice_arp_mode_t arp_mode;
    arp_mode.ALL = v->voicePatch->arp_mode;
    if( arp_mode.ENABLE )
        return;

    // check if reset requested for FA event or sequencer was disabled before (transition Seq off->on)
    if( !s->state.ENABLED || mbSidClockPtr->eventStart || s->restart_req ) {
        s->restart_req = 0;
        // next clock event will increment to 0
        s->div_ctr = ~0;
        s->sub_ctr = ~0;
        // next step will increment to start position
        s->pos = ((s->seq_patch->num & 0x7) << 4) - 1;
        // ensure that slide flag is cleared
        v->slide = 0;
    }

    if( mbSidClockPtr->eventStart || mbSidClockPtr->eventContinue ) {
        // enable voice (again)
        v->voiceActive = 1;
    }

    // sequencer enabled
    s->state.ENABLED = 1;

    // check for clock sync event
    if( mbSidClockPtr->eventClock ) {
        sid_se_seq_speed_par_t speed_par;
        speed_par.ALL = s->seq_patch->speed;

        // increment clock divider
        // reset divider if it already has reached the target value
        if( ++s->div_ctr == 0 || s->div_ctr > speed_par.CLKDIV ) {
            s->div_ctr = 0;

            // increment subcounter and check for state
            // 0: new note & gate set
            // 4: gate clear
            // >= 6: reset to 0, new note & gate set
            if( ++s->sub_ctr >= 6 )
                s->sub_ctr = 0;

            if( s->sub_ctr == 0 ) { // set gate
                // increment position counter, reset at end position
                u8 next_step = (s->pos & 0x0f) + 1;
                if( next_step > s->seq_patch->length )
                    next_step = 0;
                else
                    next_step &= 0x0f; // just to ensure...

                // change to new sequence number immediately if SYNCH_TO_MEASURE flag not set, or first step reached
                u8 next_pattern = s->pos >> 4;
                if( !speed_par.SYNCH_TO_MEASURE || next_step == 0 )
                    next_pattern = s->seq_patch->num & 0x7;

                s->pos = (next_pattern << 4) | next_step;

                // play the step

                // gate off (without slide) if invalid song number (stop feature: seq >= 8)
                if( s->seq_patch->num >= 8 ) {
                    if( v->gateActive ) {
                        v->gateClrReq = 1;
                        v->gateSetReq = 0;
                        triggerNoteOff(v, 0);
                    }
                } else {
                    // Sequence Storage - Structure:
                    //   2 bytes for each step (selected with address bit #7)
                    //   lower byte: [3:0] note, [5:4] octave, [6] glide, [7] gate
                    //   upper byte: [6:0] parameter value, [7] accent
                    // 16 Steps per sequence (offset 0x00..0x0f)
                    // 8 sequences:
                    //  0x100..0x10f/0x180..0x18f: sequence #1
                    //  0x110..0x11f/0x190..0x19f: sequence #2
                    //  0x120..0x12f/0x1a0..0x1af: sequence #3
                    //  0x130..0x13f/0x1b0..0x1bf: sequence #4
                    //  0x140..0x14f/0x1c0..0x1cf: sequence #5
                    //  0x150..0x15f/0x1d0..0x1df: sequence #6
                    //  0x160..0x16f/0x1e0..0x1ef: sequence #7
                    //  0x170..0x17f/0x1f0..0x1ff: sequence #8

                    // get note/par value
                    sid_se_seq_note_item_t note_item;
                    note_item.ALL = mbSidPatch.body.B.seq_memory[s->pos];
                    sid_se_seq_asg_item_t asg_item;
                    asg_item.ALL = mbSidPatch.body.B.seq_memory[s->pos + 0x80];

#if DEBUG_VERBOSE_LEVEL >= 2
                    DEBUG_MSG("SEQ %d@%d/%d: 0x%02x 0x%02x\n", s->seq, s->pos >> 4, s->pos & 0xf, note_item.ALL, asg_item.ALL);
#endif

                    // determine note
                    u8 note = note_item.NOTE + 0x3c;

                    // add octave
                    switch( note_item.OCTAVE ) {
                    case 1: note += 12; break; // Up
                    case 2: note -= 12; break; // Down
                    case 3: note += 24; break; // Up2
                    }

                    // transfer to voice
                    v->note = note;

                    // set accent
                    // ignore if slide has been set by previous step
                    // (important for SID sustain: transition from sustain < 0xf to 0xf will reset the VCA)
                    if( !v->slide ) {
                        // take over accent
                        v->accent = asg_item.ACCENT;
                    }

                    // activate portamento if slide has been set by previous step
                    v->portaActive = v->slide;

                    // set slide flag of current flag
                    v->slide = note_item.SLIDE;

                    // set gate if flag is set
                    if( note_item.GATE && v->voiceActive ) {
                        v->gateClrReq = 0; // ensure that gate won't be cleared by previous CLR_REQ
                        if( !v->gateActive ) {
                            v->gateSetReq = 1;

                            // request ENV attack and LFO sync
                            triggerNoteOn(v, 0);
                        }
                    }

                    // parameter track:
                    // determine SID channels which should be modified
                    if( s->seq_patch->assign ) {
                        u8 sidlr = 1 << s->seq; // SIDL/R selection
                        u8 ins = 0;
                        mbSidPar.parSetWT(s->seq_patch->assign, asg_item.PAR_VALUE + 0x80, sidlr, ins);
                    }
                }
            } else if( s->sub_ctr == 4 ) { // clear gate
                // don't clear if slide flag is set!
                if( !v->slide ) {
                    v->gateClrReq = 1;
                    v->gateSetReq = 0;
                    triggerNoteOff(v, 0);
                }
            }
        }
    }
}


/////////////////////////////////////////////////////////////////////////////
// Drum Sequencer
/////////////////////////////////////////////////////////////////////////////
void MbSidSe::seSeqDrum(sid_se_seq_t *s)
{
    // clear gate and deselect sequence if MIDI clock stop requested
    if( mbSidClockPtr->eventStop ) {
        // stop sequencer
        s->state.RUNNING = 0;
        // clear gates
        seAllNotesOff();

        return; // nothing else to do
    }

    // exit if not in sequencer mode
    // clear gates if sequencer just has been disabled (for proper transitions)
    sid_se_seq_speed_par_t speed_par;
    speed_par.ALL = mbSidPatch.body.D.seq_speed;
    if( !speed_par.SEQ_ON ) {
        // check if sequencer was disabled before
        if( s->state.ENABLED ) {
            // clear gates
            seAllNotesOff();
        }
        s->state.ENABLED = 0;
        return; // nothing else to do
    }

    // check if reset requested for FA event or sequencer was disabled before (transition Seq off->on)
    if( !s->state.ENABLED || s->restart_req || mbSidClockPtr->eventStart ) {
        s->restart_req = 0;
        // next clock event will increment to 0
        s->div_ctr = ~0;
        s->sub_ctr = ~0;
        // next step will increment to start position
        s->pos = ((mbSidPatch.body.D.seq_num & 0x7) << 4) - 1;
    }

    if( mbSidClockPtr->eventStart || mbSidClockPtr->eventContinue ) {
        // start sequencer
        s->state.RUNNING = 1;
    }

    // sequencer enabled
    s->state.ENABLED = 1;

    // check for clock sync event
    if( mbSidClockPtr->eventClock ) {
        // increment clock divider
        // reset divider if it already has reached the target value
        if( ++s->div_ctr == 0 || s->div_ctr > speed_par.CLKDIV ) {
            s->div_ctr = 0;

            // increment subcounter and check for state
            // 0: new note & gate set
            // 4: gate clear
            // >= 6: reset to 0, new note & gate set
            if( ++s->sub_ctr >= 6 )
                s->sub_ctr = 0;

            if( s->sub_ctr == 0 ) { // set gate
                // increment position counter, reset at end position
                u8 next_step = (s->pos & 0x0f) + 1;
                if( next_step > mbSidPatch.body.D.seq_length )
                    next_step = 0;
                else
                    next_step &= 0x0f; // just to ensure...

                // change to new sequence number immediately if SYNCH_TO_MEASURE flag not set, or first step reached
                u8 next_pattern = s->pos >> 4;
                if( !speed_par.SYNCH_TO_MEASURE || next_step == 0 )
                    next_pattern = mbSidPatch.body.D.seq_num & 0x7;

                s->pos = (next_pattern << 4) | next_step;

                // play the step

                // gate off if invalid song number (stop feature: seq >= 8)
                if( mbSidPatch.body.D.seq_num >= 8 ) {
                    // clear gates
                    seAllNotesOff();
                } else {
                    // Sequence Storage - Structure:
                    // 4 bytes for 16 steps:
                    //  - first byte: [0] gate step #1 ... [7] gate step #8
                    //  - second byte: [0] accent step #1 ... [7] accent step #8
                    //  - third byte: [0] gate step #9 ... [7] gate step #16
                    //  - fourth byte: [0] accent step #9 ... [7] accent step #16
                    // 
                    // 8 tracks per sequence:
                    //  offset 0x00-0x03: track #1
                    //  offset 0x04-0x07: track #2
                    //  offset 0x08-0x0b: track #3
                    //  offset 0x0c-0x0f: track #4
                    //  offset 0x00-0x03: track #5
                    //  offset 0x04-0x07: track #6
                    //  offset 0x08-0x0b: track #7
                    //  offset 0x0c-0x0f: track #8
                    // 8 sequences:
                    //  0x100..0x11f: sequence #1
                    //  0x120..0x13f: sequence #2
                    //  0x140..0x15f: sequence #3
                    //  0x160..0x17f: sequence #4
                    //  0x180..0x19f: sequence #5
                    //  0x1a0..0x1bf: sequence #6
                    //  0x1c0..0x1df: sequence #7
                    //  0x1e0..0x1ff: sequence #8
                    u8 *pattern = (u8 *)&mbSidPatch.body.D.seq_memory[(s->pos & 0xf0) << 1];

                    // loop through 8 tracks
                    u8 track;
                    u8 step = s->pos & 0x0f;
                    u8 step_offset = (step & (1 << 3)) ? 2 : 0;
                    u8 step_mask = (1 << (step & 0x7));
                    for(track=0; track<8; ++track) {
                        u8 mode = 0;
                        if( pattern[4*track + step_offset + 0] & step_mask )
                            mode |= 1;
                        if( pattern[4*track + step_offset + 1] & step_mask )
                            mode |= 2;

                        // coding:
                        // Gate  Accent  Result
                        //    0       0  Don't play note
                        //    1       0  Play Note w/o accent
                        //    0       1  Play Note w/ accent
                        //    1       1  Play Secondary Note
                        u8 drum = track*2;
                        u8 gate = 0;
                        u8 velocity = 0x3f; // >= 0x40 selects accent
                        switch( mode ) {
                        case 1: gate = 1; break;
                        case 2: gate = 1; velocity = 0x7f; break;
                        case 3: gate = 1; ++drum; break;
                        }

                        if( gate )
                            seNoteOnDrum(drum, velocity);
                    }
                }
            } else if( s->sub_ctr == 4 ) { // clear gates
                seAllNotesOff();
            }
        }
    }
}


/////////////////////////////////////////////////////////////////////////////
// Note On for Drums
/////////////////////////////////////////////////////////////////////////////
void MbSidSe::seNoteOnDrum(u8 drum, u8 velocity)
{
    if( drum >= 16 )
        return; // unsupported drum

    // get voice assignment
    sid_se_voice_patch_t *voice_patch = (sid_se_voice_patch_t *)&mbSidPatch.body.D.voice[drum];
    sid_se_v_flags_t v_flags;
    v_flags.ALL = voice_patch->D.v_flags;
    u8 voice_asg = v_flags.D.VOICE_ASG;

    // number of voices depends on mono/stereo mode (TODO: once ensemble available...)
    u8 num_voices = 6;

    // check if drum instrument already played, in this case use the old voice
    MbSidVoice *v = &mbSidVoice[0];
    int voice;
    for(voice=0; voice<num_voices; ++voice, ++v) {
        if( v->assignedInstrument == drum )
            break;
    }

    u8 voice_found = 0;
    if( voice < num_voices ) {
        // check if voice assignment still valid (could have been changed meanwhile)
        switch( voice_asg ) {
        case 0: voice_found = 1; break; // All
        case 1: if( voice < 3 ) voice_found = 1; break; // Left Only
        case 2: if( voice >= 3 ) voice_found = 1; break; // Right Only
        default: if( voice == (voice_asg-3) ) voice_found = 1; break; // direct assignment
        }
    }

    // get new voice if required
    if( !voice_found )
        voiceQueue.get(drum, voice_asg, num_voices);

#if DEBUG_VERBOSE_LEVEL >= 2
    DEBUG_MSG("[seNoteOnDrum:%d] drum %d takes voice %d\n", sidNum, drum, voice);
#endif

    // get pointer to voice
    v = &mbSidVoice[voice];

    // save instrument number
    v->assignedInstrument = drum;

    // determine drum model and save pointer
    u8 drum_model = voice_patch->D.drum_model;
    if( drum_model >= SID_SE_DRUM_MODEL_NUM )
        drum_model = 0;
    v->drumModel = (sid_drum_model_t *)&mbSidDrumModel[drum_model];

    // vary gatelength depending on PAR1 / 4
    s32 gatelength = v->drumModel->gatelength;
    if( gatelength ) {
        gatelength += ((s32)voice_patch->D.par1 - 0x80) / 4;
        if( gatelength > 127 ) gatelength = 127; else if( gatelength < 2 ) gatelength = 2;
        // gatelength should never be 0, otherwise gate clear delay feature would be deactivated
    }
    v->drumGatelength = gatelength;

    // vary WT speed depending on PAR2 / 4
    s32 wt_speed = v->drumModel->wt_speed;
    wt_speed += ((s32)voice_patch->D.par2 - 0x80) / 4;
    if( wt_speed > 127 ) wt_speed = 127; else if( wt_speed < 0 ) wt_speed = 0;
    mbSidWt[voice].wtDrumSpeed = wt_speed;

    // store the third parameter
    mbSidWt[voice].wtDrumPar = voice_patch->D.par3;

    // store waveform and note
    v->drumWaveform = v->drumModel->waveform;
    v->note = v->drumModel->base_note;

    // set/clear ACCENT depending on velocity
    v->accent = velocity >= 64 ? 1 : 0;

    // activate voice and request gate
    v->voiceActive = 1;
    triggerNoteOn(v, 0);
}


/////////////////////////////////////////////////////////////////////////////
// Note On for Drums
/////////////////////////////////////////////////////////////////////////////
void MbSidSe::seNoteOffDrum(u8 drum)
{
    if( drum >= 16 )
        return; // unsupported drum

    // go through all voices which are assigned to the current instrument
    MbSidVoice *v = &mbSidVoice[0];
    int voice;
    for(voice=0; voice<SID_SE_NUM_VOICES; ++voice, ++v) {
        if( v->assignedInstrument == drum ) {
            voiceQueue.release(voice);

#if DEBUG_VERBOSE_LEVEL >= 2
            DEBUG_MSG("[seNoteOffDrum:%d] drum %d releases voice %d\n", sidNum, drum, voice);
#endif

            // the rest is only required if gatelength not controlled from drum model
            if( v->drumModel != NULL && v->drumGatelength ) {
                v->voiceActive = 0;
                v->gateSetReq = 0;
                v->gateClrReq = 1;
            }
        }
    }
}


/////////////////////////////////////////////////////////////////////////////
// All Notes Off
/////////////////////////////////////////////////////////////////////////////
void MbSidSe::seAllNotesOff()
{
    sid_se_engine_t engine = (sid_se_engine_t)mbSidPatch.body.engine;

    switch( engine ) {
    case SID_SE_LEAD:
    case SID_SE_BASSLINE:
    case SID_SE_MULTI: {
        // TODO
    } break;

    case SID_SE_DRUM: {
        MbSidVoice *v = &mbSidVoice[0];

        // disable all active voices
        for(int voice=0; voice<SID_SE_NUM_VOICES; ++voice, ++v) {
            if( v->voiceActive ) {
                voiceQueue.release(voice);

#if DEBUG_VERBOSE_LEVEL >= 2
                DEBUG_MSG("[SID_SE_D_NoteOff:%d] drum %d releases voice %d\n", sid, drum, voice);
#endif

                // the rest is only required if gatelength not controlled from drum model
                if( !v->drumGatelength ) {
                    v->voiceActive = 0;
                    v->gateSetReq = 0;
                    v->gateClrReq = 1;
                }
            }
        }
    } break;
    }
}


/////////////////////////////////////////////////////////////////////////////
// should be called whenver the patch has been changed
/////////////////////////////////////////////////////////////////////////////
void MbSidSe::updatePatch(void)
{
    // disable interrupts to ensure atomic change
    MIOS32_IRQ_Disable();

    // remember previous engine and get new engine
    sid_se_engine_t prevEngine = (sid_se_engine_t)mbSidPatch.bodyShadow.engine;
    sid_se_engine_t engine = (sid_se_engine_t)mbSidPatch.body.engine;

    // copy patch into shadow buffer
    mbSidPatch.updateShadow();

    // re-initialize structures if engine has changed
    if( prevEngine != engine )
        initStructs();

    // clear voice queue
    voiceQueue.init(&mbSidPatch.body);

    // enable interrupts again
    MIOS32_IRQ_Enable();
}
