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

    for(i=0; i<SID_SE_NUM_VOICES; ++i) {
        u8 voice = i;
        sid_se_voice_t *v = (sid_se_voice_t *)&sid_se_voice[voice];
        memset(v, 0, sizeof(sid_se_voice_t));

        v->engine = engine; // engine type
        v->phys_sid = (voice>=3) ? physSidR : physSidL; // reference to physical SID (chip)
        v->voice = voice; // cross-reference to assigned voice
        v->phys_voice = voice % 3; // reference to physical voice
        if( voice >= 3 )
            v->phys_sid_voice = (sid_voice_t *)&sidRegRPtr->v[v->phys_voice];
        else
            v->phys_sid_voice = (sid_voice_t *)&sidRegLPtr->v[v->phys_voice];

        switch( engine ) {
        case SID_SE_BASSLINE:
            v->mv = (sid_se_midi_voice_t *)&sid_se_midi_voice[(voice < 3) ? 0 : 1];
            v->voice_patch = (sid_se_voice_patch_t *)&mbSidPatch.body.B.voice[(voice < 3) ? 0 : 1];
            v->trg_mask_note_on = NULL;
            v->trg_mask_note_off = NULL;
            break;

        case SID_SE_DRUM:
            v->mv = (sid_se_midi_voice_t *)&sid_se_midi_voice[0]; // always use first midi voice
            v->voice_patch = NULL; // not used!
            v->trg_mask_note_on = NULL;
            v->trg_mask_note_off = NULL;
            break;

        case SID_SE_MULTI:
            v->mv = (sid_se_midi_voice_t *)&sid_se_midi_voice[0]; // will be automatically re-assigned
            v->voice_patch = (sid_se_voice_patch_t *)&mbSidPatch.body.M.voice[0]; // will be automatically re-assigned
            v->trg_mask_note_on = NULL;
            v->trg_mask_note_off = NULL;
            break;

        default: // SID_SE_LEAD
            v->mv = (sid_se_midi_voice_t *)&sid_se_midi_voice[voice];
            v->voice_patch = (sid_se_voice_patch_t *)&mbSidPatch.body.L.voice[voice];
            v->trg_mask_note_on = (sid_se_trg_t *)&mbSidPatch.body.L.trg_matrix[SID_SE_TRG_NOn];
            v->trg_mask_note_off = (sid_se_trg_t *)&mbSidPatch.body.L.trg_matrix[SID_SE_TRG_NOff];
        }

        v->mod_dst_pitch = &modDst[SID_SE_MOD_DST_PITCH1 + voice];
        v->mod_dst_pw = &modDst[SID_SE_MOD_DST_PW1 + voice];

        v->assigned_instrument = ~0; // invalid instrument
        v->dm = NULL; // will be assigned by drum engine
        v->drum_gatelength = 0; // will be configured by drum engine
        v->drum_wt_speed = 0; // will be configured by drum engine
        v->drum_par3 = 0; // will be configured by drum engine
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
        f->mod_dst_filter = &modDst[SID_SE_MOD_DST_FIL1 + filter];
        f->mod_dst_volume = &modDst[SID_SE_MOD_DST_VOL1 + filter];
    }

    MbSidLfo *l = &mbSidLfo[0];
    for(int lfo=0; lfo<SID_SE_NUM_LFO; ++lfo, ++l) {
        switch( engine ) {
        case SID_SE_BASSLINE: {
            u8 patch_voice = (lfo >= 2) ? 1 : 0;
            u8 mod_voice = patch_voice * 3;
            u8 lfo_voice = lfo & 1;
            sid_se_voice_patch_t *voicePatch = (sid_se_voice_patch_t *)&mbSidPatch.body.B.voice[patch_voice];
            l->init((sid_se_lfo_patch_t *)&voicePatch->B.lfo[lfo_voice], mbSidClockPtr);

            l->modDstPitch = &modDst[SID_SE_MOD_DST_PITCH1 + mod_voice];
            l->modDstPw = &modDst[SID_SE_MOD_DST_PW1 + mod_voice];
            l->modDstFilter = &modDst[SID_SE_MOD_DST_FIL1 + patch_voice];
        } break;

        case SID_SE_DRUM:
            // not used
            l->init(NULL, NULL);
            break;

        case SID_SE_MULTI: {
            u8 mod_voice = lfo >> 1;
            u8 lfo_voice = lfo & 1;
            sid_se_voice_patch_t *voicePatch = (sid_se_voice_patch_t *)&mbSidPatch.body.M.voice[0];
            l->init((sid_se_lfo_patch_t *)&voicePatch->M.lfo[lfo_voice], mbSidClockPtr);
            l->modDstPitch = &modDst[SID_SE_MOD_DST_PITCH1 + mod_voice];
            l->modDstPw = &modDst[SID_SE_MOD_DST_PW1 + mod_voice];
            l->modDstFilter = &modDst[(mod_voice >= 3) ? SID_SE_MOD_DST_FIL2 : SID_SE_MOD_DST_FIL1];
        } break;

        default: // SID_SE_LEAD
            l->init((sid_se_lfo_patch_t *)&mbSidPatch.body.L.lfo[lfo], mbSidClockPtr);
            l->modSrcLfo = &modSrc[SID_SE_MOD_SRC_LFO1 + lfo];
            l->modDstLfoDepth = &modDst[SID_SE_MOD_DST_LD1 + lfo];
            l->modDstLfoRate = &modDst[SID_SE_MOD_DST_LR1 + lfo];
        }
    }

    MbSidEnv *e = &mbSidEnv[0];
    for(int env=0; env<SID_SE_NUM_ENV; ++env, ++e) {
        switch( engine ) {
        case SID_SE_BASSLINE: {
            u8 patch_voice = env ? 1 : 0;
            u8 mod_voice = patch_voice * 3;
            sid_se_voice_patch_t *voicePatch = (sid_se_voice_patch_t *)&mbSidPatch.body.B.voice[patch_voice];
            e->init((sid_se_env_patch_t *)&voicePatch->B.env, mbSidClockPtr);
            e->modDstPitch = &modDst[SID_SE_MOD_DST_PITCH1 + mod_voice];
            e->modDstPw = &modDst[SID_SE_MOD_DST_PW1 + mod_voice];
            e->modDstFilter = &modDst[SID_SE_MOD_DST_FIL1 + patch_voice];
            e->decayA = &voicePatch->B.env_decay_a;
            e->accent = &voicePatch->accent;
        } break;

        case SID_SE_DRUM:
            // not used
            e->init(NULL, NULL);
            break;

        case SID_SE_MULTI: {
            u8 mod_voice = env;
            sid_se_voice_patch_t *voicePatch = (sid_se_voice_patch_t *)&mbSidPatch.body.M.voice[0];
            e->init((sid_se_env_patch_t *)&voicePatch->M.env, mbSidClockPtr);
            e->modDstPitch = &modDst[SID_SE_MOD_DST_PITCH1 + mod_voice];
            e->modDstPw = &modDst[SID_SE_MOD_DST_PW1 + mod_voice];
            e->modDstFilter = &modDst[(mod_voice >= 3) ? SID_SE_MOD_DST_FIL2 : SID_SE_MOD_DST_FIL1];
            e->decayA = &voicePatch->B.env_decay_a;
            e->accent = &voicePatch->accent;
        } break;

        default: // SID_SE_LEAD
            e->init((sid_se_env_patch_t *)&mbSidPatch.body.L.env[env], mbSidClockPtr);
            e->modSrcEnv = &modSrc[SID_SE_MOD_SRC_ENV1 + env];
        }
    }

    for(i=0; i<SID_SE_NUM_WT; ++i) {
        u8 wt = i;
        sid_se_wt_t *w = (sid_se_wt_t *)&sid_se_wt[wt];
        memset(w, 0, sizeof(sid_se_wt_t));

        w->wt = wt; // cross-reference to WT number

        switch( engine ) {
        case SID_SE_BASSLINE:
            w->wt_patch = NULL;
            break;

        case SID_SE_DRUM:
            // not relevant
            w->wt_patch = NULL;
            break;

        case SID_SE_MULTI: {
            sid_se_voice_patch_t *voice_patch = (sid_se_voice_patch_t *)&mbSidPatch.body.M.voice[0];
            w->wt_patch = (sid_se_wt_patch_t *)&voice_patch->M.wt; // will be dynamically changed depending on assigned instrument
        } break;

        default: // SID_SE_LEAD
            w->wt_patch = (sid_se_wt_patch_t *)&mbSidPatch.body.L.wt[wt];
        }

        w->mod_src_wt = &modSrc[SID_SE_MOD_SRC_WT1 + wt];
        w->mod_dst_wt = &modDst[SID_SE_MOD_DST_WT1 + wt];
    }

    for(i=0; i<SID_SE_NUM_SEQ; ++i) {
        u8 seq = i;
        sid_se_seq_t *s = (sid_se_seq_t *)&sid_se_seq[seq];
        memset(s, 0, sizeof(sid_se_seq_t));

        s->seq = seq;

        switch( engine ) {
        case SID_SE_BASSLINE: {
            s->v = (sid_se_voice_t *)&sid_se_voice[seq ? 3 : 0];
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
}


/////////////////////////////////////////////////////////////////////////////
// Sound Engine Update Cycle
/////////////////////////////////////////////////////////////////////////////
bool MbSidSe::updateSe(void)
{
    sid_se_engine_t engine = (sid_se_engine_t)mbSidPatch.body.engine;

    // Clear all modulation destinations
    s32 *modDst_clr = (s32 *)&modDst;
    for(int i=0; i<SID_SE_NUM_MOD_DST; ++i)
        *modDst_clr++ = 0; // faster than memset()! (ca. 20 uS) - seems that memset only copies byte-wise

    // Clock
    if( mbSidClockPtr->event.MIDI_START ) {
        if( engine == SID_SE_LEAD )
            triggerLead((sid_se_trg_t *)&mbSidPatch.body.L.trg_matrix[SID_SE_TRG_MST]);
        else {
            // TODO: additional measures required for other engines?
        }
    }

    if( mbSidClockPtr->event.CLK ) {
        if( engine == SID_SE_LEAD )
            triggerLead((sid_se_trg_t *)&mbSidPatch.body.L.trg_matrix[SID_SE_TRG_CLK]);
        else {
            // clock WTs
            sid_se_wt_t *w = &sid_se_wt[0];
            for(int i=0; i<SID_SE_NUM_WT; ++i, ++w)
                w->clk_req = 1;
        }

        // propagate clock/4 event to trigger matrix on each 6th clock
        if( mbSidClockPtr->clkCtr6 == 0 )
            triggerLead((sid_se_trg_t *)&mbSidPatch.body.L.trg_matrix[SID_SE_TRG_CL6]);

        // propagate clock/16 event to trigger matrix on each 24th clock
        if( mbSidClockPtr->clkCtr24 == 0 )
            triggerLead((sid_se_trg_t *)&mbSidPatch.body.L.trg_matrix[SID_SE_TRG_C24]);
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
        modSrc[SID_SE_MOD_SRC_KEY] = sid_se_voice[0].linear_frq >> 1;

        // do calculations
        calcModulationLead();

        // Wavetables
        sid_se_wt_t *w = &sid_se_wt[0];
        for(int i=0; i<4; ++i, ++w)
            seWt(w);

        // Arps
        sid_se_voice_t *v = &sid_se_voice[0];
        for(int i=0; i<6; ++i, ++v)
            seArp(v);

        // Voices
        v = &sid_se_voice[0];
        for(int i=0; i<6; ++i, ++v) {
            if( seGate(v) > 0 )
                sePitch(v);
            sePw(v);
        }

        // Filters and Volume
        sid_se_filter_t *f = &sid_se_filter[0];
        seFilterAndVolume(f++);
        seFilterAndVolume(f);

        // Tmp: copy register values directly into SID registers
        v = &sid_se_voice[0];
        for(int i=0; i<6; ++i, ++v) {
            sid_se_voice_waveform_t waveform;
            waveform.ALL = v->voice_patch->waveform;
            v->phys_sid_voice->waveform = waveform.WAVEFORM;
            v->phys_sid_voice->sync = waveform.SYNC;
            v->phys_sid_voice->ringmod = waveform.RINGMOD;

            // don't change ADSR so long delay is active (also important for ABW - ADSR bug workaround)
            if( !v->set_delay_ctr ) {
                v->phys_sid_voice->ad = v->voice_patch->ad;
                v->phys_sid_voice->sr = v->voice_patch->sr;
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
            e->accentReq = sid_se_voice[3*env].state.ACCENT;
            e->tick(engine, updateSpeedFactor);
        }

        // Sequencer
        sid_se_seq_t *s = &sid_se_seq[0];
        for(int i=0; i<2; ++i, ++s)
            seSeqBassline(s);

        // Arps
        sid_se_voice_t *v = &sid_se_voice[0];
        for(int i=0; i<6; i+=3, v+=3)
            seArp(v);

        // Voices
        // transfer note values and state flags from OSC1 (master) to OSC2/3 (slaves)
        for(int i=0; i<6; i+=3) {
            sid_se_voice[i+1].state = sid_se_voice[i+2].state = sid_se_voice[i+0].state;
            sid_se_voice[i+1].note = sid_se_voice[i+2].note = sid_se_voice[i+0].note;
            sid_se_voice[i+1].arp_note = sid_se_voice[i+2].arp_note = sid_se_voice[i+0].arp_note;
            sid_se_voice[i+1].played_note = sid_se_voice[i+2].played_note = sid_se_voice[i+0].played_note;
        }

        // process voices
        v = &sid_se_voice[0];
        for(int i=0; i<6; ++i, ++v) {
            if( seGate(v) )
                sePitch(v);
            sePw(v);
        }

        // Filters and Volume
        sid_se_filter_t *f = &sid_se_filter[0];
        seFilterAndVolume(f++);
        seFilterAndVolume(f);

        // Tmp: copy register values directly into SID registers
        for(int i=0; i<6; i+=3) {
            sid_se_voice_t *v1 = &sid_se_voice[i+0];
            sid_se_voice_t *v2 = &sid_se_voice[i+1];
            sid_se_voice_t *v3 = &sid_se_voice[i+2];

            // Voice 1 oscillator
            sid_se_voice_waveform_t v1_waveform;
            v1_waveform.ALL = v1->voice_patch->waveform;
            v1->phys_sid_voice->waveform = v1_waveform.WAVEFORM;
            v1->phys_sid_voice->sync = v1_waveform.SYNC;
            v1->phys_sid_voice->ringmod = v1_waveform.RINGMOD;

            // Voice 2 oscillator has own waveform and voice enable flag
            // no gate/sync if waveform disabled
            sid_se_voice_waveform_t v2_waveform;
            v2_waveform.ALL = v2->voice_patch->B.v2_waveform;
            if( !v2_waveform.WAVEFORM ) {
                v2->phys_sid_voice->waveform_reg = 0x10; // select triangle waveform to keep the oscillator silent
            } else {
                v2->phys_sid_voice->waveform = v2_waveform.WAVEFORM;
                v2->phys_sid_voice->sync = v2_waveform.SYNC;
                v2->phys_sid_voice->ringmod = v2_waveform.RINGMOD;
            }

            // Voice 3 oscillator has own waveform and voice enable flag
            // no gate/sync if waveform disabled
            sid_se_voice_waveform_t v3_waveform;
            v3_waveform.ALL = v3->voice_patch->B.v3_waveform;
            if( !v3_waveform.WAVEFORM ) {
                v3->phys_sid_voice->waveform_reg = 0x10; // select triangle waveform to keep the oscillator silent
            } else {
                v3->phys_sid_voice->waveform = v3_waveform.WAVEFORM;
                v3->phys_sid_voice->sync = v3_waveform.SYNC;
                v3->phys_sid_voice->ringmod = v3_waveform.RINGMOD;
            }

            // don't change ADSR so long delay is active (also important for ABW - ADSR bug workaround)
            if( !v->set_delay_ctr ) {
                u8 ad = v1->voice_patch->ad;
                u8 sr = v1->voice_patch->sr;

                // force sustain to maximum if accent flag active
                if( v1->state.ACCENT )
                    sr |= 0xf0;

                v1->phys_sid_voice->ad = ad;
                v1->phys_sid_voice->sr = sr;
                v2->phys_sid_voice->ad = ad;
                v2->phys_sid_voice->sr = sr;
                v3->phys_sid_voice->ad = ad;
                v3->phys_sid_voice->sr = sr;
            }
        }
    } break;

    case SID_SE_DRUM: {
        // Sequencer
        sid_se_seq_t *s = &sid_se_seq[0];
        seSeqDrum(s);


        // Wavetables
        sid_se_wt_t *w = &sid_se_wt[0];
        for(int i=0; i<6; ++i, ++w)
            seWtDrum(w);


        // Voices
        // process voices
        sid_se_voice_t *v = &sid_se_voice[0];
        for(int i=0; i<6; ++i, ++v) {
            if( seGate(v) )
                sePitchDrum(v);

            // control the delayed gate clear request
            if( v->state.GATE_ACTIVE && !v->set_delay_ctr && v->clr_delay_ctr ) {
                int clr_delay_ctr = v->clr_delay_ctr + mbSidEnvTable[v->drum_gatelength] / updateSpeedFactor;
                if( clr_delay_ctr >= 0xffff ) {
                    // overrun: clear counter to disable delay
                    v->clr_delay_ctr = 0;
                    // request gate clear and deactivate voice active state (new note can be played)
                    v->state.GATE_SET_REQ = 0;
                    v->state.GATE_CLR_REQ = 1;
                    v->state.VOICE_ACTIVE = 0;
                } else
                    v->clr_delay_ctr = clr_delay_ctr;
            }

            // Pulsewidth handler
            sePwDrum(v);
        }

        // Filters and Volume
        sid_se_filter_t *f = &sid_se_filter[0];
        seFilterAndVolume(f++);
        seFilterAndVolume(f);

        // Tmp: copy register values directly into SID registers
        v = &sid_se_voice[0];
        for(int i=0; i<6; ++i, ++v) {
            // wait until initial gate delay has passed (hard-sync, for ABW feature)
            if( !v->set_delay_ctr ) {
                sid_se_voice_waveform_t waveform;
                waveform.ALL = v->drum_waveform;
                v->phys_sid_voice->test = 0;
                v->phys_sid_voice->waveform = waveform.WAVEFORM;
                v->phys_sid_voice->sync = waveform.SYNC;
                v->phys_sid_voice->ringmod = waveform.RINGMOD;

                if( v->assigned_instrument < 16 ) {
                    sid_se_voice_patch_t *voice_patch = (sid_se_voice_patch_t *)&mbSidPatch.body.D.voice[v->assigned_instrument];

                    u8 ad = voice_patch->D.ad;
                    u8 sr = voice_patch->D.sr;

                    // force sustain to maximum if accent flag active
                    if( v->state.ACCENT )
                        sr |= 0xf0;

                    v->phys_sid_voice->ad = ad;
                    v->phys_sid_voice->sr = sr;
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
            e->accentReq = sid_se_voice[env].state.ACCENT;
            e->tick(engine, updateSpeedFactor);
        }

        // Wavetables
        sid_se_wt_t *w = &sid_se_wt[0];
        for(int i=0; i<6; ++i, ++w)
            seWt(w);

        // Arps
        // In difference to other engines, each MIDI voice controls an arpeggiator
        // for the last assigned voice
        sid_se_midi_voice_t *mv = &sid_se_midi_voice[0];
        for(int i=0; i<6; ++i, ++mv) {
            sid_se_voice_t *v = &sid_se_voice[mv->last_voice];
            if( v->assigned_instrument == i )
                seArp(v);
        }


        // Voices
        sid_se_voice_t *v = &sid_se_voice[0];
        for(int i=0; i<6; ++i, ++v) {
            if( seGate(v) )
                sePitch(v);
            sePw(v);
        }


        // Filters and Volume
        sid_se_filter_t *f = &sid_se_filter[0];
        seFilterAndVolume(f++);
        seFilterAndVolume(f);


        // Tmp: copy register values directly into SID registers
        v = &sid_se_voice[0];
        for(int i=0; i<6; ++i, ++v) {
            sid_se_voice_waveform_t waveform;
            waveform.ALL = v->voice_patch->waveform;
            v->phys_sid_voice->waveform = waveform.WAVEFORM;
            v->phys_sid_voice->sync = waveform.SYNC;
            v->phys_sid_voice->ringmod = waveform.RINGMOD;

            // don't change ADSR so long delay is active (also important for ABW - ADSR bug workaround)
            if( !v->set_delay_ctr ) {
                v->phys_sid_voice->ad = v->voice_patch->ad;
                v->phys_sid_voice->sr = v->voice_patch->sr;
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
void MbSidSe::triggerNoteOn(sid_se_voice_t *v, u8 no_wt)
{
    switch( v->engine ) {
    case SID_SE_LEAD: {
        sid_se_trg_t trg = *v->trg_mask_note_on;
        trg.ALL[0] &= 0xc0 | (1 << v->voice); // only the dedicated voice should trigger
        if( no_wt ) // optional ly WT triggers are masked out
            trg.ALL[2] = 0;
        triggerLead(&trg);
    } break;

    case SID_SE_BASSLINE: {
        if( v->voice < 3 ) {
            sid_se_voice[0].note_restart_req = 1;
            sid_se_voice[1].note_restart_req = 1;
            sid_se_voice[2].note_restart_req = 1;

            sid_se_lfo_mode_t lfo_mode;
            for(int lfo=0; lfo<2; ++lfo) {
                lfo_mode.ALL = mbSidLfo[lfo].lfoPatch->mode;
                if( lfo_mode.KEYSYNC )
                    mbSidLfo[lfo].restartReq = 1;
            }

            mbSidEnv[0].restartReq = 1;
        } else {
            sid_se_voice[3].note_restart_req = 1;
            sid_se_voice[4].note_restart_req = 1;
            sid_se_voice[5].note_restart_req = 1;

            sid_se_lfo_mode_t lfo_mode;
            for(int lfo=2; lfo<4; ++lfo) {
                lfo_mode.ALL = mbSidLfo[lfo].lfoPatch->mode;
                if( lfo_mode.KEYSYNC )
                    mbSidLfo[lfo].restartReq = 1;
            }

            mbSidEnv[1].restartReq = 1;
        }
    } break;

    case SID_SE_DRUM: {
        v->note_restart_req = 1;
        sid_se_wt[v->voice].restart_req = 1;
    } break;

    case SID_SE_MULTI: {
        v->note_restart_req = 1;

        sid_se_lfo_mode_t lfo_mode;
        int lfo_offset = 2*v->voice;
        for(int lfo=0; lfo<2; ++lfo) {
            lfo_mode.ALL = mbSidLfo[lfo_offset + lfo].lfoPatch->mode;
            if( lfo_mode.KEYSYNC )
                mbSidLfo[lfo_offset + lfo].restartReq = 1;
        }

        mbSidEnv[v->voice].restartReq = 1;

        //sid_se_wt[v->voice].restart_req = 1;
        // (no good idea, WTs will quickly get out of sync - see also "A107 Poly Trancegate" patch)
    } break;
    }
}


void MbSidSe::triggerNoteOff(sid_se_voice_t *v, u8 no_wt)
{
    switch( v->engine ) {
    case SID_SE_LEAD: {
        sid_se_trg_t trg = *v->trg_mask_note_off;
        trg.ALL[0] &= 0xc0; // mask out all gate trigger flags
        if( no_wt ) // optionally WT triggers are masked out
            trg.ALL[2] = 0;
        triggerLead(&trg);
    } break;

    case SID_SE_BASSLINE: {
        mbSidEnv[(v->voice >= 3) ? 1 : 0].releaseReq = 1;
    } break;

    case SID_SE_DRUM: {
        // nothing to do
    } break;

    case SID_SE_MULTI: {
        mbSidEnv[v->voice].releaseReq = 1;
    } break;
    }
}


/////////////////////////////////////////////////////////////////////////////
// Input function for trigger matrix of Lead Engine
/////////////////////////////////////////////////////////////////////////////
void MbSidSe::triggerLead(sid_se_trg_t *trg)
{
  if( trg->ALL[0] ) {
    if( trg->O1L ) sid_se_voice[0].note_restart_req = 1;
    if( trg->O2L ) sid_se_voice[1].note_restart_req = 1;
    if( trg->O3L ) sid_se_voice[2].note_restart_req = 1;
    if( trg->O1R ) sid_se_voice[3].note_restart_req = 1;
    if( trg->O2R ) sid_se_voice[4].note_restart_req = 1;
    if( trg->O3R ) sid_se_voice[5].note_restart_req = 1;
    if( trg->E1A ) mbSidEnv[0].restartReq = 1;
    if( trg->E2A ) mbSidEnv[1].restartReq = 1;
  }

  if( trg->ALL[1] ) {
    if( trg->E1R ) mbSidEnv[0].releaseReq = 1;
    if( trg->E2R ) mbSidEnv[1].releaseReq = 1;
    if( trg->L1  ) mbSidLfo[0].restartReq = 1;
    if( trg->L2  ) mbSidLfo[1].restartReq = 1;
    if( trg->L3  ) mbSidLfo[2].restartReq = 1;
    if( trg->L4  ) mbSidLfo[3].restartReq = 1;
    if( trg->L5  ) mbSidLfo[4].restartReq = 1;
    if( trg->L6  ) mbSidLfo[5].restartReq = 1;
  }

  if( trg->ALL[2] ) {
    if( trg->W1R ) sid_se_wt[0].restart_req = 1;
    if( trg->W2R ) sid_se_wt[1].restart_req = 1;
    if( trg->W3R ) sid_se_wt[2].restart_req = 1;
    if( trg->W4R ) sid_se_wt[3].restart_req = 1;
    if( trg->W1S ) sid_se_wt[0].clk_req = 1;
    if( trg->W2S ) sid_se_wt[1].clk_req = 1;
    if( trg->W3S ) sid_se_wt[2].clk_req = 1;
    if( trg->W4S ) sid_se_wt[3].clk_req = 1;
  }
}


/////////////////////////////////////////////////////////////////////////////
// Calculate Modulation Path of Lead Engine
/////////////////////////////////////////////////////////////////////////////
void MbSidSe::calcModulationLead(void)
{
    int i;

    // calculate modulation pathes
    sid_se_mod_patch_t *mp = (sid_se_mod_patch_t *)&mbSidPatch.body.L.mod[0];
    for(i=0; i<8; ++i, ++mp) {
        if( mp->depth != 0 ) {

            // first source
            s32 mod_src1_value;
            if( !mp->src1 ) {
                mod_src1_value = 0;
            } else {
                if( mp->src1 & (1 << 7) ) {
                    // constant range 0x00..0x7f -> +0x0000..0x38f0
                    mod_src1_value = (mp->src1 & 0x7f) << 7;
                } else {
                    // modulation range +/- 0x3fff
                    mod_src1_value = modSrc[mp->src1-1] / 2;
                }
            }

            // second source
            s32 mod_src2_value;
            if( !mp->src2 ) {
                mod_src2_value = 0;
            } else {
                if( mp->src2 & (1 << 7) ) {
                    // constant range 0x00..0x7f -> +0x0000..0x38f0
                    mod_src2_value = (mp->src2 & 0x7f) << 7;
                } else {
                    // modulation range +/- 0x3fff
                    mod_src2_value = modSrc[mp->src2-1] / 2;
                }
            }

            // apply operator
            s16 mod_result;
            switch( mp->op & 0x0f ) {
            case 0: // disabled
                mod_result = 0;
                break;

            case 1: // SRC1 only
                mod_result = mod_src1_value;
                break;

            case 2: // SRC2 only
                mod_result = mod_src2_value;
                break;

            case 3: // SRC1+SRC2
                mod_result = mod_src1_value + mod_src2_value;
                break;

            case 4: // SRC1-SRC2
                mod_result = mod_src1_value - mod_src2_value;
                break;

            case 5: // SRC1*SRC2 / 8192 (to avoid overrun)
                mod_result = (mod_src1_value * mod_src2_value) / 8192;
                break;

            case 6: // XOR
                mod_result = mod_src1_value ^ mod_src2_value;
                break;

            case 7: // OR
                mod_result = mod_src1_value | mod_src2_value;
                break;

            case 8: // AND
                mod_result = mod_src1_value & mod_src2_value;
                break;

            case 9: // Min
                mod_result = (mod_src1_value < mod_src2_value) ? mod_src1_value : mod_src2_value;
                break;

            case 10: // Max
                mod_result = (mod_src1_value > mod_src2_value) ? mod_src1_value : mod_src2_value;
                break;

            case 11: // SRC1 < SRC2
                mod_result = (mod_src1_value < mod_src2_value) ? 0x7fff : 0x0000;
                break;

            case 12: // SRC1 > SRC2
                mod_result = (mod_src1_value > mod_src2_value) ? 0x7fff : 0x0000;
                break;

            case 13: { // SRC1 == SRC2 (with tolarance of +/- 64
                s32 diff = mod_src1_value - mod_src2_value;
                mod_result = (diff > -64 && diff < 64) ? 0x7fff : 0x0000;
            } break;

            case 14: { // S&H - SRC1 will be sampled whenever SRC2 changes from a negative to a positive value
                // check for SRC2 transition
                u8 old_mod_transition = modTransition;
                if( mod_src2_value < 0 )
                    modTransition &= ~(1 << i);
                else
                    modTransition |= (1 << i);

                if( modTransition != old_mod_transition && mod_src2_value >= 0 ) // only on positive transition
                    mod_result = mod_src1_value; // sample: take new mod value
                else
                    mod_result = modSrc[SID_SE_MOD_SRC_MOD1 + i]; // hold: take old mod value
            } break;

            default:
                mod_result = 0;
            }

            // store in modulator source array for feedbacks
            // use value w/o depth, this has two advantages:
            // - maximum resolution when forwarding the data value
            // - original MOD value can be taken for sample&hold feature
            // bit it also has disadvantage:
            // - the user could think it is a bug when depth doesn't affect the feedback MOD value...
            modSrc[SID_SE_MOD_SRC_MOD1 + i] = mod_result;

            // forward to destinations
            if( mod_result ) {
                s32 scaled_mod_result = ((s32)mp->depth-128) * mod_result / 64; // (+/- 0x7fff * +/- 0x7f) / 128
      
                // invert result if requested
                s32 mod_dst1 = (mp->op & (1 << 6)) ? -scaled_mod_result : scaled_mod_result;
                s32 mod_dst2 = (mp->op & (1 << 7)) ? -scaled_mod_result : scaled_mod_result;

                // add result to modulation target array
                u8 x_target1 = mp->x_target[0];
                if( x_target1 && x_target1 <= SID_SE_NUM_MOD_DST )
                    modDst[x_target1 - 1] += mod_dst1;
	
                u8 x_target2 = mp->x_target[1];
                if( x_target2 && x_target2 <= SID_SE_NUM_MOD_DST )
                    modDst[x_target2 - 1] += mod_dst2;

                // add to additional SIDL/R targets
                u8 direct_target_l = mp->direct_target[0];
                if( direct_target_l ) {
                    if( direct_target_l & (1 << 0) ) modDst[SID_SE_MOD_DST_PITCH1] += mod_dst1;
                    if( direct_target_l & (1 << 1) ) modDst[SID_SE_MOD_DST_PITCH2] += mod_dst1;
                    if( direct_target_l & (1 << 2) ) modDst[SID_SE_MOD_DST_PITCH3] += mod_dst1;
                    if( direct_target_l & (1 << 3) ) modDst[SID_SE_MOD_DST_PW1] += mod_dst1;
                    if( direct_target_l & (1 << 4) ) modDst[SID_SE_MOD_DST_PW2] += mod_dst1;
                    if( direct_target_l & (1 << 5) ) modDst[SID_SE_MOD_DST_PW3] += mod_dst1;
                    if( direct_target_l & (1 << 6) ) modDst[SID_SE_MOD_DST_FIL1] += mod_dst1;
                    if( direct_target_l & (1 << 7) ) modDst[SID_SE_MOD_DST_VOL1] += mod_dst1;
                }

                u8 direct_target_r = mp->direct_target[1];
                if( direct_target_r ) {
                    if( direct_target_r & (1 << 0) ) modDst[SID_SE_MOD_DST_PITCH4] += mod_dst2;
                    if( direct_target_r & (1 << 1) ) modDst[SID_SE_MOD_DST_PITCH5] += mod_dst2;
                    if( direct_target_r & (1 << 2) ) modDst[SID_SE_MOD_DST_PITCH6] += mod_dst2;
                    if( direct_target_r & (1 << 3) ) modDst[SID_SE_MOD_DST_PW4] += mod_dst2;
                    if( direct_target_r & (1 << 4) ) modDst[SID_SE_MOD_DST_PW5] += mod_dst2;
                    if( direct_target_r & (1 << 5) ) modDst[SID_SE_MOD_DST_PW6] += mod_dst2;
                    if( direct_target_r & (1 << 6) ) modDst[SID_SE_MOD_DST_FIL2] += mod_dst2;
                    if( direct_target_r & (1 << 7) ) modDst[SID_SE_MOD_DST_VOL2] += mod_dst2;
                }
            }
        }
    }
}


/////////////////////////////////////////////////////////////////////////////
// Arpeggiator
/////////////////////////////////////////////////////////////////////////////
void MbSidSe::seArp(sid_se_voice_t *v)
{
    sid_se_midi_voice_t *mv = (sid_se_midi_voice_t *)v->mv;
    sid_se_voice_arp_mode_t arp_mode;
    arp_mode.ALL = v->voice_patch->arp_mode;
    sid_se_voice_arp_speed_div_t arp_speed_div;
    arp_speed_div.ALL = v->voice_patch->arp_speed_div;
    sid_se_voice_arp_gl_rng_t arp_gl_rng;
    arp_gl_rng.ALL = v->voice_patch->arp_gl_rng;

    u8 new_note_req = 0;
    u8 first_note_req = 0;
    u8 gate_clr_req = 0;

    // check if arp sync requested
    if( mbSidClockPtr->event.MIDI_START || mv->arp_state.SYNC_ARP ) {
        // set arp counters to max values (forces proper reset)
        mv->arp_note_ctr = ~0;
        mv->arp_oct_ctr = ~0;
        // reset ARP Up flag (will be set to 1 with first note)
        mv->arp_state.ARP_UP = 0;
        // request first note (for oneshot function)
        first_note_req = 1;
        // reset divider if not disabled or if arp synch on MIDI clock start event
        if( mbSidClockPtr->event.MIDI_START || !arp_mode.SYNC ) {
            mv->arp_div_ctr = ~0;
            mv->arp_gl_ctr = ~0;
            // request new note
            new_note_req = 1;
        }
    }

    // if clock sync event:
    if( mbSidClockPtr->event.CLK ) {
        // increment clock divider
        // reset divider if it already has reached the target value
        int inc = 1;
        if( arp_mode.CAC ) {
            // in CAC mode: increment depending on number of pressed keys
            inc = mv->notestack.len;
            if( !inc ) // at least one increment
                inc = 1;
        }
        // handle divider
        // TODO: improve this!
        u8 div_ctr = mv->arp_div_ctr + inc;
        u8 speed_div = arp_speed_div.DIV + 1;
        while( div_ctr >= speed_div ) {
            div_ctr -= speed_div;
            // request new note
            new_note_req = 1;
            // request gate clear if voice not active anymore (required for Gln=>Speed)
            if( !v->state.VOICE_ACTIVE )
                gate_clr_req = 1;
        }
        mv->arp_div_ctr = div_ctr;

        // increment gatelength counter
        // reset counter if it already has reached the target value
        if( ++mv->arp_gl_ctr > arp_gl_rng.GATELENGTH ) {
            // reset counter
            mv->arp_gl_ctr = 0;
            // request gate clear
            gate_clr_req = 1;
        }
    }

    // check if HOLD mode has been deactivated - disable notes in this case
    u8 disable_notes = !arp_mode.HOLD && mv->arp_state.HOLD_SAVED;
    // store HOLD flag in MIDI voice record
    mv->arp_state.HOLD_SAVED = arp_mode.HOLD;

    // skip the rest if arp is disabled
    if( disable_notes || !arp_mode.ENABLE ) {
        // check if arp was active before (for proper 1->0 transition when ARP is disabled)
        if( mv->arp_state.ARP_ACTIVE ) {
            // notify that arp is not active anymore
            mv->arp_state.ARP_ACTIVE = 0;
            // clear note stack (especially important in HOLD mode!)
            NOTESTACK_Clear(&mv->notestack);
            // propagate Note Off through trigger matrix
            triggerNoteOff(v, 0);
            // request gate clear
            v->state.GATE_SET_REQ = 0;
            v->state.GATE_CLR_REQ = 1;
        }
    } else {
        // notify that arp is active (for proper 1->0 transition when ARP is disabled)
        mv->arp_state.ARP_ACTIVE = 1;

        // check if voice not active anymore (not valid in HOLD mode) or gate clear has been requested
        // skip voice active check in hold mode and voice sync mode
        if( gate_clr_req || (!arp_mode.HOLD && !arp_mode.SYNC && !v->state.VOICE_ACTIVE) ) {
            // forward this to note handler if gate is not already deactivated
            if( v->state.GATE_ACTIVE ) {
                // propagate Note Off through trigger matrix
                triggerNoteOff(v, 0);
                // request gate clear
                v->state.GATE_SET_REQ = 0;
                v->state.GATE_CLR_REQ = 1;
            }
        }

        // check if a new arp note has been requested
        // skip if note counter is 0xaa (oneshot mode)
        if( new_note_req && mv->arp_note_ctr != 0xaa ) {
            // reset gatelength counter
            mv->arp_gl_ctr = 0;
            // increment note counter
            // if max value of arp note counter reached, reset it
            if( ++mv->arp_note_ctr >= mv->notestack.len )
                mv->arp_note_ctr = 0;
            // if note is zero, reset arp note counter
            if( !mv->notestack_items[mv->arp_note_ctr].note )
                mv->arp_note_ctr = 0;


            // dir modes
            u8 note_number = 0;
            if( arp_mode.DIR >= 6 ) { // Random
                if( mv->notestack.len > 0 )
                    note_number = randomGen.value(0, mv->notestack.len-1);
                else
                    note_number = 0;
            } else {
                u8 new_note_up;
                if( arp_mode.DIR >= 2 && arp_mode.DIR <= 5 ) { // Alt Mode 1 and 2
                    // toggle ARP_UP flag each time the arp note counter is zero
                    if( !mv->arp_note_ctr ) {
                        mv->arp_state.ARP_UP ^= 1;
                        // increment note counter to prevent double played notes
                        if( arp_mode.DIR >= 2 && arp_mode.DIR <= 3 ) // only in Alt Mode 1
                            if( ++mv->arp_note_ctr >= mv->notestack.len )
                                mv->arp_note_ctr = 0;
                    }

                    // direction depending on Arp Up/Down and Alt Up/Down flag
                    if( (arp_mode.DIR & 1) == 0 ) // UP modes
                        new_note_up = mv->arp_state.ARP_UP;
                    else // DOWN modes
                        new_note_up = !mv->arp_state.ARP_UP;
                } else {
                    // direction depending on arp mode 0 or 1
                    new_note_up = (arp_mode.DIR & 1) == 0;
                }

                if( new_note_up )
                    note_number = mv->arp_note_ctr;
                else
                    if( mv->notestack.len )
                        note_number = mv->notestack.len - mv->arp_note_ctr - 1;
                    else
                        note_number = 0;
            }

            int new_note = mv->notestack_items[note_number].note;

            // now check for oneshot mode: if note is 0, or if note counter and oct counter is 0, stop here
            if( !first_note_req && arp_speed_div.ONESHOT &&
                (!new_note || (note_number == 0 && mv->arp_oct_ctr == 0)) ) {
                // set note counter to 0xaa to stop ARP until next reset
                mv->arp_note_ctr = 0xaa;
                // don't play new note
                new_note = 0;
            }

            // only play if note is not zero
            if( new_note ) {
                // if first note has been selected, increase octave until max value is reached
                if( first_note_req )
                    mv->arp_oct_ctr = 0;
                else if( note_number == 0 ) {
                    ++mv->arp_oct_ctr;
                }
                if( mv->arp_oct_ctr > arp_gl_rng.OCTAVE_RANGE )
                    mv->arp_oct_ctr = 0;

                // transpose note
                new_note += 12*mv->arp_oct_ctr;

                // saturate octave-wise
                while( new_note >= 0x6c ) // use 0x6c instead of 128, since range 0x6c..0x7f sets frequency to 0xffff...
                    new_note -= 12;

                // store new arp note
                v->arp_note = new_note;

                // forward gate set request if voice is active and gate not active
                if( v->state.VOICE_ACTIVE ) {
                    v->state.GATE_CLR_REQ = 0; // ensure that gate won't be cleared by previous CLR_REQ
                    if( !v->state.GATE_ACTIVE ) {
                        // set gate
                        v->state.GATE_SET_REQ = 1;

                        // propagate Note On through trigger matrix
                        triggerNoteOn(v, 0);
                    }
                }
            }
        }
    }

    // clear arp sync flag
    mv->arp_state.SYNC_ARP = 0;
}


/////////////////////////////////////////////////////////////////////////////
// Voice Gate
// returns true if pitch should be changed
/////////////////////////////////////////////////////////////////////////////
bool MbSidSe::seGate(sid_se_voice_t *v)
{
    bool changePitch = true;

    // restart request?
    if( v->note_restart_req ) {
        v->note_restart_req = 0;

        // request gate if voice is active (and request clear for proper ADSR handling)
        v->state.GATE_CLR_REQ = 1;
        if( v->state.VOICE_ACTIVE )
            v->state.GATE_SET_REQ = 1;

        if( v->engine == SID_SE_DRUM ) {
            // initialize clear delay counter (drum engine controls the gate active time)
            v->clr_delay_ctr = v->drum_gatelength ? 1 : 0;
            // no set delay by default
            v->set_delay_ctr = 0;

            // delay if ABW (ADSR bug workaround) option active
            // this feature works different for drum engine: test flag is set and waveform is cleared
            // instead of clearing the ADSR registers - this approach is called "hard-sync"
            sid_se_opt_flags_t opt_flags;
            opt_flags.ALL = mbSidPatch.body.opt_flags;
            if( opt_flags.ABW ) {
                v->set_delay_ctr = 0x0001;

                // clear waveform register and set test+gate flag
                v->phys_sid_voice->waveform_reg = 0x09;
            }
        } else {
            // check if voice should be delayed - set delay counter to 0x0001 in this case, else to 0x0000
            v->set_delay_ctr = v->voice_patch->delay ? 0x0001 : 0x0000;

            // delay also if ABW (ADSR bug workaround) option is active
            sid_se_opt_flags_t opt_flags;
			opt_flags.ALL = mbSidPatch.body.opt_flags;
            if( opt_flags.ABW ) {
                if( !v->set_delay_ctr ) // at least +1 delay
                    v->set_delay_ctr = 0x0001;

                // clear ADSR registers, so that the envelope gets completely released
                v->phys_sid_voice->ad = 0x00;
                v->phys_sid_voice->sr = 0x00;
            }
        }
    }

    // voice disable handling (allows to turn on/off voice via waveform parameter)
    sid_se_voice_waveform_t waveform;
    if( v->engine == SID_SE_DRUM ) {
        if( v->dm )
            waveform.ALL = v->dm->waveform;
        else
			return false; // no drum model selected
    } else
        waveform.ALL = v->voice_patch->waveform;

    if( v->state.VOICE_DISABLED ) {
        if( !waveform.VOICE_OFF ) {
            v->state.VOICE_DISABLED = 0;
            if( v->state.VOICE_ACTIVE )
                v->state.GATE_SET_REQ = 1;
        }
    } else {
        if( waveform.VOICE_OFF ) {
            v->state.VOICE_DISABLED = 1;
            v->state.GATE_CLR_REQ = 1;
        }
    }

    // if gate not active: ignore clear request
    if( !v->state.GATE_ACTIVE )
        v->state.GATE_CLR_REQ = 0;

    // gate set/clear request?
    if( v->state.GATE_CLR_REQ ) {
        v->state.GATE_CLR_REQ = 0;

        // clear SID gate flag if GSA (gate stays active) function not enabled
        sid_se_voice_flags_t voice_flags;
        if( v->engine == SID_SE_DRUM )
            voice_flags.ALL = 0;
        else
            voice_flags.ALL = v->voice_patch->flags;

        if( !voice_flags.GSA )
            v->phys_sid_voice->gate = 0;

        // gate not active anymore
        v->state.GATE_ACTIVE = 0;
    } else if( v->state.GATE_SET_REQ || v->state.OSC_SYNC_IN_PROGRESS ) {
        // don't set gate if oscillator disabled
        if( !waveform.VOICE_OFF ) {
            sid_se_opt_flags_t opt_flags;
			opt_flags.ALL = mbSidPatch.body.opt_flags;

            // delay note so long 16bit delay counter != 0
            if( v->set_delay_ctr ) {
                int delay_inc = 0;
                if( v->engine != SID_SE_DRUM )
                    delay_inc = v->voice_patch->delay;

                // if ABW (ADSR bug workaround) active: use at least 30 mS delay
                if( opt_flags.ABW ) {
                    delay_inc += 25;
                    if( delay_inc > 255 )
                        delay_inc = 255;
                }

                int set_delay_ctr = v->set_delay_ctr + mbSidEnvTable[delay_inc] / updateSpeedFactor;
                if( delay_inc && set_delay_ctr < 0xffff ) {
                    // no overrun
                    v->set_delay_ctr = set_delay_ctr;
                    // don't change pitch so long delay is active
                    changePitch = false;
                } else {
                    // overrun: clear counter to disable delay
                    v->set_delay_ctr = 0x0000;
                    // for ABW (hard-sync)
                    v->phys_sid_voice->test = 0;
                }
            }

            if( !v->set_delay_ctr ) {
                // acknowledge the set request
                v->state.GATE_SET_REQ = 0;

                // optional OSC synchronisation
                u8 skip_gate = 0;
                if( !v->state.OSC_SYNC_IN_PROGRESS ) {
                    switch( v->engine ) {
                    case SID_SE_LEAD: {
                        u8 osc_phase;
                        if( (osc_phase=mbSidPatch.body.L.osc_phase) ) {
                            // notify that OSC synchronisation has been started
                            v->state.OSC_SYNC_IN_PROGRESS = 1;
                            // set test flag for one update cycle
                            v->phys_sid_voice->test = 1;
                            // set pitch depending on selected oscillator phase to achieve an offset between the waveforms
                            // This approach has been invented by Wilba! :-)
                            u32 reference_frq = 16779; // 1kHz
                            u32 osc_sync_frq;
                            switch( v->voice % 3 ) {
                            case 1: osc_sync_frq = (reference_frq * (1000 + 4*(u32)osc_phase)) / 1000; break;
                            case 2: osc_sync_frq = (reference_frq * (1000 + 8*(u32)osc_phase)) / 1000; break;
                            default: osc_sync_frq = reference_frq; // (Voice 0 and 3)
                            }
                            v->phys_sid_voice->frq_l = osc_sync_frq & 0xff;
                            v->phys_sid_voice->frq_h = osc_sync_frq >> 8;
                            // don't change pitch for this update cycle!
                            changePitch = false;
                            // skip gate handling for this update cycle
                            skip_gate = 1;
                        }
                    } break;

                    case SID_SE_BASSLINE: {
                        // set test flag if OSC_PHASE_SYNC enabled
                        sid_se_v_flags_t v_flags;
						v_flags.ALL = v->voice_patch->B.v_flags;
                        if( v_flags.OSC_PHASE_SYNC ) {
                            // notify that OSC synchronisation has been started
                            v->state.OSC_SYNC_IN_PROGRESS = 1;
                            // set test flag for one update cycle
                            v->phys_sid_voice->test = 1;
                            // don't change pitch for this update cycle!
                            changePitch = false;
                            // skip gate handling for this update cycle
                            skip_gate = 1;
                        }
                    } break;

                    case SID_SE_MULTI: {
                        // TODO
                    } break;

                    case SID_SE_DRUM: {
                        // not relevant
                    } break;
                    }
                }

                if( !skip_gate ) { // set by code below if OSC sync is started
                    if( v->phys_sid_voice->test ) {
                        // clear test flag
                        v->phys_sid_voice->test = 0;
                        // don't change pitch for this update cycle!
                        changePitch = false;
                        // ensure that pitch handler will re-calculate pitch frequency on next update cycle
                        v->state.FORCE_FRQ_RECALC = 1;
                    } else {
                        // this code is also executed if OSC synchronisation disabled
                        // set the gate flag
                        v->phys_sid_voice->gate = 1;
                        // OSC sync finished
                        v->state.OSC_SYNC_IN_PROGRESS = 0;
                    }
                }
            }
        }
        v->state.GATE_ACTIVE = 1;
    }

    return changePitch;
}


/////////////////////////////////////////////////////////////////////////////
// Voice Pitch
/////////////////////////////////////////////////////////////////////////////
void MbSidSe::sePitch(sid_se_voice_t *v)
{
    // transpose MIDI note
    sid_se_voice_arp_mode_t arp_mode;
	arp_mode.ALL = v->voice_patch->arp_mode;
    int transposed_note = arp_mode.ENABLE ? v->arp_note : v->note;

    if( v->engine == SID_SE_BASSLINE ) {
        // get transpose value depending on voice number
        switch( v->voice ) {
        case 1:
        case 4:
            if( v->voice_patch->B.v2_static_note )
                transposed_note = v->voice_patch->B.v2_static_note;
            else {
                u8 oct_transpose = v->voice_patch->B.v2_oct_transpose;
                if( oct_transpose & 4 )
                    transposed_note -= 12*(4-(oct_transpose & 3));
                else
                    transposed_note += 12*(oct_transpose & 3);
            }
            break;

        case 2:
        case 5:
            if( v->voice_patch->B.v3_static_note )
                transposed_note = v->voice_patch->B.v3_static_note;
            else {
                u8 oct_transpose = v->voice_patch->B.v3_oct_transpose;
                if( oct_transpose & 4 )
                    transposed_note -= 12*(4-(oct_transpose & 3));
                else
                    transposed_note += 12*(oct_transpose & 3);
            }
            break;

        default: // 0 and 3
            transposed_note += v->voice_patch->transpose - 64;
        }

        // MV transpose: if sequencer running, we can transpose with MIDI voice 3/4
        sid_se_v_flags_t v_flags;
		v_flags.ALL = v->voice_patch->B.v_flags;
        if( v_flags.WTO ) {
            sid_se_midi_voice_t *mv_transpose = (v->voice >= 3)
                ? (sid_se_midi_voice_t *)&sid_se_midi_voice[3]
                : (sid_se_midi_voice_t *)&sid_se_midi_voice[2];

            // check for MIDI channel to ensure compatibility with older ensemble patches
            if( mv_transpose->midi_channel < 16 && mv_transpose->notestack.len )
                transposed_note += mv_transpose->notestack.note_items[0].note - 0x3c + mv_transpose->transpose - 64;
            else
                transposed_note += (int)v->mv->transpose - 64;
        } else {
            transposed_note += (int)v->mv->transpose - 64;
        }    
    } else {
        // Lead & Multi engine
        transposed_note += v->voice_patch->transpose - 64;
        transposed_note += (int)v->mv->transpose - 64;
    }



    // octave-wise saturation
    while( transposed_note < 0 )
        transposed_note += 12;
    while( transposed_note >= 128 )
        transposed_note -= 12;

    // Glissando handling
    sid_se_voice_flags_t voice_flags;
	voice_flags.ALL = v->voice_patch->flags;
    if( transposed_note != v->prev_transposed_note ) {
        v->prev_transposed_note = transposed_note;
        if( voice_flags.PORTA_MODE >= 2 ) // Glissando active?
            v->portamento_ctr = 0xffff; // force overrun
    }

    u8 portamento = v->voice_patch->portamento;
    if( v->state.PORTA_ACTIVE && portamento && voice_flags.PORTA_MODE >= 2 ) {
        // increment portamento counter, amount derived from envelope table  .. make it faster
        int portamento_ctr = v->portamento_ctr + mbSidEnvTable[portamento >> 1] / updateSpeedFactor;
        // next note step?
        if( portamento_ctr >= 0xffff ) {
            // reset counter
            v->portamento_ctr = 0;

            // increment/decrement note
            if( transposed_note > v->glissando_note )
                ++v->glissando_note;
            else if( transposed_note < v->glissando_note )
                --v->glissando_note;

            // target reached?
            if( transposed_note == v->glissando_note )
                v->state.PORTA_ACTIVE = 0;
        } else {
            // take over new counter value
            v->portamento_ctr = portamento_ctr;
        }
        // switch to current glissando note
        transposed_note = v->glissando_note;
    } else {
        // store current transposed note (optionally varried by glissando effect)
        v->glissando_note = transposed_note;
    }

    // transfer note -> linear frequency
    int target_frq = transposed_note << 9;

    // increase/decrease target frequency by pitchrange
    // depending on pitchbender and finetune value
    if( v->voice_patch->pitchrange ) {
        u16 pitchbender = v->mv->pitchbender;
        int delta = (int)pitchbender - 0x80;
        delta += (int)v->voice_patch->finetune-0x80;

        // detuning
        u8 detune = mbSidPatch.body.L.osc_detune;
        if( detune ) {
            // additional detuning depending on SID channel and oscillator
            // Left OSC1: +detune/4   (lead only, 0 in bassline mode)
            // Right OSC1: -detune/4  (lead only, 0 in bassline mode)
            // Left OSC2: +detune
            // Right OSC2: -detune
            // Left OSC3: -detune
            // Right OSC3: +detune
            switch( v->voice ) {
            case 0: if( v->engine == SID_SE_LEAD ) delta += detune/4; break; // makes only sense on stereo sounds
            case 3: if( v->engine == SID_SE_LEAD ) delta -= detune/4; break; // makes only sense on stereo sounds

            case 1:
            case 5: delta += detune; break;

            case 2:
            case 4: delta -= detune; break;
            }
        }

        if( delta ) {
            int scaled = (delta * 4 * (int)v->voice_patch->pitchrange);
            target_frq += scaled;
        }
    }

    // saturate target frequency to 16bit
    if( target_frq < 0 ) target_frq = 0; else if( target_frq > 0xffff ) target_frq = 0xffff;

    // portamento
    // whenever target frequency has been changed, update portamento frequency
    if( v->portamento_end_frq != target_frq ) {
        v->portamento_end_frq = target_frq;
        v->portamento_begin_frq = v->linear_frq;
        // reset portamento counter if not in glissando mode
        if( voice_flags.PORTA_MODE < 2 )
            v->portamento_ctr = 0;

        if( target_frq == v->linear_frq )
            v->state.PORTA_ACTIVE = 0; // nothing to do...
    }

    int linear_frq = target_frq;
    if( v->state.PORTA_ACTIVE && portamento && voice_flags.PORTA_MODE < 2 ) {
        linear_frq = v->linear_frq;

        // get portamento multiplier from envelope table
        // this one is used for "constant time glide" and "normal portamento"
        int porta_multiplier = mbSidEnvTable[portamento] / updateSpeedFactor;

        if( voice_flags.PORTA_MODE >= 1 ) { // constant glide time and glissando
            // increment counter
            int portamento_ctr = v->portamento_ctr + porta_multiplier;

            // target reached on overrun
            if( portamento_ctr >= 0xffff ) {
                linear_frq = target_frq;
                v->state.PORTA_ACTIVE = 0;
            } else {
                v->portamento_ctr = portamento_ctr;

                // scale between new and old frequency
                int delta = v->portamento_end_frq - v->portamento_begin_frq;
                linear_frq = v->portamento_begin_frq + ((delta * portamento_ctr) >> 16);
                if( delta > 0 ) {
                    if( linear_frq >= target_frq ) {
                        linear_frq = target_frq;
                        v->state.PORTA_ACTIVE = 0;
                    }
                } else {
                    if( linear_frq <= target_frq ) {
                        linear_frq = target_frq;
                        v->state.PORTA_ACTIVE = 0;
                    }
                }
            }
        } else { // normal portamento

            // increment/decrement frequency
            int inc = (int)(((u32)linear_frq * (u32)porta_multiplier) >> 16);
            if( !inc )
                inc = 1;

            if( target_frq > linear_frq ) {
                linear_frq += inc;
                if( linear_frq >= target_frq ) {
                    linear_frq = target_frq;
                    v->state.PORTA_ACTIVE = 0;
                }
            } else {
                linear_frq -= inc;
                if( linear_frq <= target_frq ) {
                    linear_frq = target_frq;
                    v->state.PORTA_ACTIVE = 0;
                }
            }
        }
    }

    // pitch modulation
    linear_frq += *v->mod_dst_pitch;
    if( linear_frq < 0 ) linear_frq = 0; else if( linear_frq > 0xffff ) linear_frq = 0xffff;

    // if frequency has been changed
    if( v->state.FORCE_FRQ_RECALC || v->linear_frq != linear_frq ) {
        v->state.FORCE_FRQ_RECALC = 0;
        v->linear_frq = linear_frq;

        // linear frequency -> SID frequency conversion
        // convert bitfield [15:9] of 16bit linear frequency to SID frequency value
        u8 frq_ix = (linear_frq >> 9) + 21;
        if( frq_ix > 126 )
            frq_ix = 126;
        // interpolate between two frequency steps (frq_a and frq_b)
        int frq_a = mbSidFrqTable[frq_ix];
        int frq_b = mbSidFrqTable[frq_ix+1];

        // use bitfield [8:0] of 16bit linear frequency for scaling between two semitones
        int frq_semi = (frq_b-frq_a) * (linear_frq & 0x1ff);
        int frq = frq_a + (frq_semi >> 9);

        // write result into SID frequency register
        v->phys_sid_voice->frq_l = frq & 0xff;
        v->phys_sid_voice->frq_h = frq >> 8;
    }
}


/////////////////////////////////////////////////////////////////////////////
// Voice Pitch for Drum Instruments
/////////////////////////////////////////////////////////////////////////////
void MbSidSe::sePitchDrum(sid_se_voice_t *v)
{
    // get frequency depending on base note
    u8 note = v->note;
    u8 frq_ix = note + 21;
    if( frq_ix > 127 )
        frq_ix = 127;
    s32 target_frq = mbSidFrqTable[frq_ix];

    if( v->assigned_instrument < 16 ) {
        sid_se_voice_patch_t *voice_patch = (sid_se_voice_patch_t *)&mbSidPatch.body.D.voice[v->assigned_instrument];

        // increase/decrease target frequency by pitchrange depending on finetune value
        u8 tune = voice_patch->D.tune;
        if( tune != 0x80 ) {
            u8 pitchrange = 24;
            if( tune > 0x80 ) {
                int dst_note = note + pitchrange + 21;
                if( dst_note > 127 ) dst_note = 127;
                u16 dst_frq = mbSidFrqTable[dst_note];
                target_frq += ((dst_frq-target_frq) * (tune-0x80)) / 128;
                if( target_frq >= 0xffff ) target_frq = 0xffff;
            } else {
                int dst_note = note - pitchrange + 21;
                if( dst_note < 0 ) dst_note = 0;
                u16 dst_frq = mbSidFrqTable[dst_note];
                target_frq -= ((target_frq-dst_frq) * (0x80-tune)) / 128;
                if( target_frq < 0 ) target_frq = 0;
            }
        }
    }

    // copy target frequency into SID registers
    v->phys_sid_voice->frq_l = target_frq & 0xff;
    v->phys_sid_voice->frq_h = target_frq >> 8;
}


/////////////////////////////////////////////////////////////////////////////
// Voice Pulsewidth
/////////////////////////////////////////////////////////////////////////////
void MbSidSe::sePw(sid_se_voice_t *v)
{
    // convert pulsewidth to 12bit signed value
    int pulsewidth;
    if( v->engine == SID_SE_BASSLINE ) {
        // get pulsewidth value depending on voice number
        switch( v->voice ) {
        case 1:
        case 4:
            pulsewidth = ((v->voice_patch->B.v2_pulsewidth_h & 0x0f) << 8) | v->voice_patch->B.v2_pulsewidth_l;
            break;

        case 2:
        case 5:
            pulsewidth = ((v->voice_patch->B.v3_pulsewidth_h & 0x0f) << 8) | v->voice_patch->B.v3_pulsewidth_l;
            break;

        default: // 0 and 3
            pulsewidth = ((v->voice_patch->pulsewidth_h & 0x0f) << 8) | v->voice_patch->pulsewidth_l;
            break;
        }
    } else {
        pulsewidth = ((v->voice_patch->pulsewidth_h & 0x0f) << 8) | v->voice_patch->pulsewidth_l;
    }

    // PW modulation
    pulsewidth += *v->mod_dst_pw / 16;
    if( pulsewidth > 0xfff ) pulsewidth = 0xfff; else if( pulsewidth < 0 ) pulsewidth = 0;

    // transfer to SID registers
    v->phys_sid_voice->pw_l = pulsewidth & 0xff;
    v->phys_sid_voice->pw_h = (pulsewidth >> 8) & 0x0f;
}


/////////////////////////////////////////////////////////////////////////////
// Voice Pulsewidth for Drum Instruments
/////////////////////////////////////////////////////////////////////////////
void MbSidSe::sePwDrum(sid_se_voice_t *v)
{
    // take pulsewidth specified by drum model
    u16 pulsewidth = v->dm ? (v->dm->pulsewidth << 4) : 0x800;

    // transfer to SID registers
    v->phys_sid_voice->pw_l = pulsewidth & 0xff;
    v->phys_sid_voice->pw_h = (pulsewidth >> 8) & 0x0f;
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
            s32 linear_frq = sid_se_voice[f->filter*3].linear_frq;
            s32 delta = (linear_frq * f->filter_patch->keytrack) / 0x1000; // 24bit -> 12bit
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
// Wavetable
/////////////////////////////////////////////////////////////////////////////
void MbSidSe::seWt(sid_se_wt_t *w)
{
    s32 step = -1;

    // if key control flag (END[7]) set, control position from current played key
    if( w->wt_patch->end & (1 << 7) ) {
        // copy currently played note to step position
        step = sid_se_voice[0].played_note;

        // if MOD control flag (BEGIN[7]) set, control step position from modulation matrix
    } else if( w->wt_patch->begin & (1 << 7) ) {
        step = ((s32)*w->mod_dst_wt + 0x8000) >> 9; // 16bit signed -> 7bit unsigned
    }

    if( step >= 0 ) {
        // use modulated step position
        // scale between begin/end range
        if( w->wt_patch->end > w->wt_patch->begin ) {
            s32 range = (w->wt_patch->end - w->wt_patch->begin) + 1;
            step = w->wt_patch->begin + ((step * range) / 128);
        } else {
            // should we invert the waveform?
            s32 range = (w->wt_patch->begin - w->wt_patch->end) + 1;
            step = w->wt_patch->end + ((step * range) / 128);
        }
    } else {
        // don't use modulated position - normal mode
        u8 next_step_req = 0;

        // check if WT reset requested
        if( w->restart_req ) {
            w->restart_req = 0;
            // next clock will increment div to 0
            w->div_ctr = ~0;
            // next step will increment to start position
            w->pos = (w->wt_patch->begin & 0x7f) - 1;
        }

        // check for WT clock event
        if( w->clk_req ) {
            w->clk_req = 0;
            // increment clock divider
            // reset divider if it already has reached the target value
            if( ++w->div_ctr == 0 || (w->div_ctr > (w->wt_patch->speed & 0x3f)) ) {
                w->div_ctr = 0;
                next_step_req = 1;
            }
        }

        // check for next step request
        // skip if position is 0xaa (notifies oneshot -> WT stopped)
        if( next_step_req && w->pos != 0xaa ) {
            // increment position counter, reset at end position
            if( ++w->pos > w->wt_patch->end ) {
                // if oneshot mode: set position to 0xaa, WT is stopped now!
                if( w->wt_patch->loop & (1 << 7) )
                    w->pos = 0xaa;
                else
                    w->pos = w->wt_patch->loop & 0x7f;
            }
            step = w->pos; // step is positive now -> will be played
        }
    }

    // check if step should be played
    if( step >= 0 ) {
        u8 wt_value = mbSidPatch.body.L.wt_memory[step & 0x7f];

        // forward to mod matrix
        if( wt_value < 0x80 ) {
            // relative value -> convert to -0x8000..+0x7fff
            *w->mod_src_wt = ((s32)wt_value - 0x40) * 512;
        } else {
            // absolute value -> convert to 0x0000..+0x7f00
            *w->mod_src_wt = ((s32)wt_value & 0x7f) * 256;
        }
    
        // determine SID channels which should be modified
        u8 sidlr = (w->wt_patch->speed >> 6); // SID selection
        u8 ins = w->wt; // preparation for multi engine

        // call parameter handler
        if( w->wt_patch->assign ) {
#if DEBUG_VERBOSE_LEVEL >= 2
            DEBUG_MSG("WT %d: 0x%02x 0x%02x\n", w->wt, step, wt_value);
#endif
            mbSidPar.parSetWT(w->wt_patch->assign, wt_value, sidlr, ins);
        }
    }
}


/////////////////////////////////////////////////////////////////////////////
// Wavetable for Drums
/////////////////////////////////////////////////////////////////////////////
void MbSidSe::seWtDrum(sid_se_wt_t *w)
{
    u8 next_step_req = 0;

    // only if drum model is selected
    sid_se_voice_t *v = (sid_se_voice_t *)&sid_se_voice[w->wt];
    if( !v->dm )
        return;

    // check if WT reset requested
    if( w->restart_req ) {
        w->restart_req = 0;
        // next clock will increment div to 0
        w->div_ctr = ~0;
        // next step will increment to start position
        w->pos = ~0;
    }

    // clock with each update cycle, so that we are independent from the selected BPM rate
    // increment clock divider
    // reset divider if it already has reached the target value
    if( ++w->div_ctr > (v->drum_wt_speed * updateSpeedFactor) ) {
        w->div_ctr = 0;
        next_step_req = 1;
    }

    // check for next step request
    // skip if position is 0xaa (notifies oneshot -> WT stopped)
    // wait until initial gate delay has passed (for ABW feature)
    // TK: checking for v->set_delay_ctr here is probably not a good idea,
    // it leads to jitter. However, we let it like it is for compatibility reasons with V2
    if( next_step_req && w->pos != 0xaa && !v->set_delay_ctr ) {
        // increment position counter, reset at end position
        ++w->pos;
        if( v->dm->wavetable[2*w->pos] == 0 ) {
            if( v->dm->wt_loop == 0xff )
                w->pos = 0xaa; // oneshot mode
            else
                w->pos = v->dm->wt_loop;
        }

        if( w->pos != 0xaa ) {
            // "play" the step
            int note = v->dm->wavetable[2*w->pos + 0];
            // transfer to voice
            // if bit #7 of note entry is set: add PAR3/2 and saturate
            if( note & (1 << 7) ) {
                note = (note & 0x7f) + (((int)v->drum_par3 - 0x80) / 2);
                if( note > 127 ) note = 127; else if( note < 0 ) note = 0;
            }
            v->note = note;

            // set waveform
            v->drum_waveform = v->dm->wavetable[2*w->pos + 1];

#if DEBUG_VERBOSE_LEVEL >= 3
            DEBUG_MSG("WT %d: %d (%02x %02x)\n", w->wt, w->pos, v->dm->wavetable[2*w->pos + 0], v->dm->wavetable[2*w->pos + 1]);
#endif
        }
    }
}


/////////////////////////////////////////////////////////////////////////////
// Bassline Sequencer
/////////////////////////////////////////////////////////////////////////////
void MbSidSe::seSeqBassline(sid_se_seq_t *s)
{
    sid_se_voice_t *v = s->v;
    sid_se_v_flags_t v_flags;
    v_flags.ALL = v->voice_patch->B.v_flags;

    // clear gate and deselect sequence if MIDI clock stop requested
    if( mbSidClockPtr->event.MIDI_STOP ) {
        v->state.VOICE_ACTIVE = 0;
        v->state.GATE_CLR_REQ = 1;
        v->state.GATE_SET_REQ = 0;
        triggerNoteOff(v, 0);
        return; // nothing else to do
    }

    // exit if WTO mode not active
    // clear gate if WTO just has been disabled (for proper transitions)
    if( !v_flags.WTO ) {
        // check if sequencer was disabled before
        if( s->state.ENABLED ) {
            // clear gate
            v->state.GATE_CLR_REQ = 1;
            v->state.GATE_SET_REQ = 0;
            triggerNoteOff(v, 0);
        }
        s->state.ENABLED = 0;
        return; // nothing else to do
    }

    // exit if arpeggiator is enabled
    sid_se_voice_arp_mode_t arp_mode;
    arp_mode.ALL = v->voice_patch->arp_mode;
    if( arp_mode.ENABLE )
        return;

    // check if reset requested for FA event or sequencer was disabled before (transition Seq off->on)
    if( !s->state.ENABLED || mbSidClockPtr->event.MIDI_START || s->restart_req ) {
        s->restart_req = 0;
        // next clock event will increment to 0
        s->div_ctr = ~0;
        s->sub_ctr = ~0;
        // next step will increment to start position
        s->pos = ((s->seq_patch->num & 0x7) << 4) - 1;
        // ensure that slide flag is cleared
        v->state.SLIDE = 0;
    }

    if( mbSidClockPtr->event.MIDI_START || mbSidClockPtr->event.MIDI_CONTINUE ) {
        // enable voice (again)
        v->state.VOICE_ACTIVE = 1;
    }

    // sequencer enabled
    s->state.ENABLED = 1;

    // check for clock sync event
    if( mbSidClockPtr->event.CLK ) {
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
                    if( v->state.GATE_ACTIVE ) {
                        v->state.GATE_CLR_REQ = 1;
                        v->state.GATE_SET_REQ = 0;
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
                    if( !v->state.SLIDE ) {
                        // take over accent
                        v->state.ACCENT = asg_item.ACCENT;
                    }

                    // activate portamento if slide has been set by previous step
                    v->state.PORTA_ACTIVE = v->state.SLIDE;

                    // set slide flag of current flag
                    v->state.SLIDE = note_item.SLIDE;

                    // set gate if flag is set
                    if( note_item.GATE && v->state.VOICE_ACTIVE ) {
                        v->state.GATE_CLR_REQ = 0; // ensure that gate won't be cleared by previous CLR_REQ
                        if( !v->state.GATE_ACTIVE ) {
                            v->state.GATE_SET_REQ = 1;

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
                if( !v->state.SLIDE ) {
                    v->state.GATE_CLR_REQ = 1;
                    v->state.GATE_SET_REQ = 0;
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
    if( mbSidClockPtr->event.MIDI_STOP ) {
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
    if( !s->state.ENABLED || s->restart_req || mbSidClockPtr->event.MIDI_START ) {
        s->restart_req = 0;
        // next clock event will increment to 0
        s->div_ctr = ~0;
        s->sub_ctr = ~0;
        // next step will increment to start position
        s->pos = ((mbSidPatch.body.D.seq_num & 0x7) << 4) - 1;
    }

    if( mbSidClockPtr->event.MIDI_START || mbSidClockPtr->event.MIDI_CONTINUE ) {
        // start sequencer
        s->state.RUNNING = 1;
    }

    // sequencer enabled
    s->state.ENABLED = 1;

    // check for clock sync event
    if( mbSidClockPtr->event.CLK ) {
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
    sid_se_voice_t *v = (sid_se_voice_t *)&sid_se_voice[0];
    int voice;
    for(voice=0; voice<num_voices; ++voice, ++v) {
        if( v->assigned_instrument == drum )
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
    v = (sid_se_voice_t *)&sid_se_voice[voice];

    // save instrument number
    v->assigned_instrument = drum;

    // determine drum model and save pointer
    u8 drum_model = voice_patch->D.drum_model;
    if( drum_model >= SID_SE_DRUM_MODEL_NUM )
        drum_model = 0;
    v->dm = (sid_drum_model_t *)&mbSidDrumModel[drum_model];

    // vary gatelength depending on PAR1 / 4
    s32 gatelength = v->dm->gatelength;
    if( gatelength ) {
        gatelength += ((s32)voice_patch->D.par1 - 0x80) / 4;
        if( gatelength > 127 ) gatelength = 127; else if( gatelength < 2 ) gatelength = 2;
        // gatelength should never be 0, otherwise gate clear delay feature would be deactivated
    }
    v->drum_gatelength = gatelength;

    // vary WT speed depending on PAR2 / 4
    s32 wt_speed = v->dm->wt_speed;
    wt_speed += ((s32)voice_patch->D.par2 - 0x80) / 4;
    if( wt_speed > 127 ) wt_speed = 127; else if( wt_speed < 0 ) wt_speed = 0;
    v->drum_wt_speed = wt_speed;

    // store the third parameter
    v->drum_par3 = voice_patch->D.par3;

    // store waveform and note
    v->drum_waveform = v->dm->waveform;
    v->note = v->dm->base_note;

    // set/clear ACCENT depending on velocity
    v->state.ACCENT = velocity >= 64 ? 1 : 0;

    // activate voice and request gate
    v->state.VOICE_ACTIVE = 1;
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
    sid_se_voice_t *v = (sid_se_voice_t *)&sid_se_voice[0];
    int voice;
    for(voice=0; voice<SID_SE_NUM_VOICES; ++voice, ++v) {
        if( v->assigned_instrument == drum ) {
            voiceQueue.release(voice);

#if DEBUG_VERBOSE_LEVEL >= 2
            DEBUG_MSG("[seNoteOffDrum:%d] drum %d releases voice %d\n", sidNum, drum, voice);
#endif

            // the rest is only required if gatelength not controlled from drum model
            if( v->dm != NULL && v->drum_gatelength ) {
                v->state.VOICE_ACTIVE = 0;
                v->state.GATE_SET_REQ = 0;
                v->state.GATE_CLR_REQ = 1;
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
        sid_se_voice_t *v = (sid_se_voice_t *)&sid_se_voice[0];

        // disable all active voices
        for(int voice=0; voice<SID_SE_NUM_VOICES; ++voice, ++v) {
            if( v->state.VOICE_ACTIVE ) {
                voiceQueue.release(voice);

#if DEBUG_VERBOSE_LEVEL >= 2
                DEBUG_MSG("[SID_SE_D_NoteOff:%d] drum %d releases voice %d\n", sid, drum, voice);
#endif

                // the rest is only required if gatelength not controlled from drum model
                if( !v->drum_gatelength ) {
                    v->state.VOICE_ACTIVE = 0;
                    v->state.GATE_SET_REQ = 0;
                    v->state.GATE_CLR_REQ = 1;
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
