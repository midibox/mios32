/***************************************************************************
 *   mcu_synthesize.cpp                                                    *
 *   Copyright (C) 2007 by Nic Birsan & Ionut Tarsa                        *
 *                                                                         *
 *   -------------------------------------------------------------------   *
 *   reduced version of synthesize.cpp from eSpeak project                 *
 *   Copyright (C) 2005, 2006 by Jonathan Duddington                       *
 *   jsd@clara.co.uk                                                       *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

//#include "StdAfx.h"

#include <stdio.h>
#include <ctype.h>
//#include <wctype.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/*
#include "speak_lib.h"
#include "speech.h"
#include "voice.h"
#include "phoneme.h"
#include "synthesize.h"
#include "translate.h"
*/

#include "mcu_otherdef.h"
#include "mcu_phoneme.h"
#include "mcu_synthesize.h"

/*Allready defined in synthesize.h
#define    PITCHfall   0
#define    PITCHrise   1
*/

extern FILE *f_log;
static void MCU_SmoothSpect(void);


// list of phonemes in a clause
int MCU_n_phoneme_list=0;       //??? is external?
MCU_PHONEME_LIST MCU_phoneme_list[MCU_N_PHONEME_LIST];


/*int speed_factor1;
int speed_factor2;
*/


static int  last_pitch_cmd;
static int  last_amp_cmd;
static MCU_frame_t  *last_frame;
static int  MCU_last_wcmdq;
static int  pitch_length;
static int  amp_length;
static int  modn_flags;

static int  syllable_start;
static int  syllable_end;
static int  syllable_centre;

/*don't use multiple voices
static voice_t *new_voice=NULL;
*/
/*don't use sound icons
int n_soundicon_tab=0;
SOUND_ICON soundicon_tab[N_SOUNDICON_TAB];
*/

#define RMS1  16      //
#define RMS2  20      // 16 - 20
#define RMS_GLOTTAL1 35   // vowel before glottal stop
#define RMS_START  25  // 14 - 30

#define VOWEL_FRONT_LENGTH  50



// a dummy phoneme_list entry which looks like a pause
static MCU_PHONEME_LIST next_pause;

//definition from voices
int MCU_formant_rate[9] = {137, 286, 454, 634, 606, 606, 606, 606, 606};  // values for 8kHz sample rate

/*WordToString defined in synthesize.cpp
	
const char *WordToString(unsigned int word)
{//========================================
// Convert a phoneme mnemonic word into a string
	int  ix;
	static char buf[5];

	for(ix=0; ix<4; ix++)
		buf[ix] = word >> (ix*8);
	buf[4] = 0;
	return(buf);
}
*/

void MCU_SynthesizeInit()
{//==================
	last_pitch_cmd = 0;
	last_amp_cmd = 0;
	last_frame = NULL;
	syllable_centre = -1;

	// initialise next_pause, a dummy phoneme_list entry
//	next_pause.ph = phoneme_tab[phonPAUSE];   // this must be done after voice selection
	next_pause.type = phPAUSE;
	next_pause.newword = 0;
}

static void MCU_EndAmplitude(void)
{//===========================
	if(amp_length > 0)
	{
		if(MCU_wcmdq[last_amp_cmd][1] == 0)
			MCU_wcmdq[last_amp_cmd][1] = amp_length;
		amp_length = 0;
	}
}

static void MCU_EndPitch(int voice_break)
{//==================================
	// posssible end of pitch envelope, fill in the length
	if((pitch_length > 0) && (last_pitch_cmd >= 0))
	{
		if(MCU_wcmdq[last_pitch_cmd][1] == 0)
			MCU_wcmdq[last_pitch_cmd][1] = pitch_length;
		pitch_length = 0;
	}

	if(voice_break)
	{
		MCU_last_wcmdq = -1;
		last_frame = NULL;
		syllable_end = MCU_wcmdq_tail;
		MCU_SmoothSpect();
		syllable_centre = -1;
		memset(MCU_vowel_transition,0,sizeof(MCU_vowel_transition));
	}
}  // end of Synthesize::EndPitch



static void MCU_DoAmplitude(int amp, unsigned char *amp_env)
{//=====================================================
	long *q;

	last_amp_cmd = MCU_wcmdq_tail;
	amp_length = 0;       // total length of vowel with this amplitude envelope

	q = MCU_wcmdq[MCU_wcmdq_tail];
	q[0] = WCMD_AMPLITUDE;
	q[1] = 0;        // fill in later from amp_length
	q[2] = (long)amp_env;
	q[3] = amp;
	MCU_WcmdqInc();
}  // end of Synthesize::DoAmplitude



static void MCU_DoPitch(const unsigned char *env, int pitch1, int pitch2)
{//============================================================
	long *q;

	MCU_EndPitch(0);

	if(pitch1 == 127)
	{
		// pitch was not set
		pitch1 = 24;
		pitch2 = 33;
		env = MCU_envelope_data[PITCHfall];
	}
	last_pitch_cmd = MCU_wcmdq_tail;
	pitch_length = 0;       // total length of spect with this pitch envelope

	if(pitch2 < 0)
		pitch2 = 0;

	q = MCU_wcmdq[MCU_wcmdq_tail];
	q[0] = WCMD_PITCH;
	q[1] = 0;   // length, fill in later from pitch_length
	q[2] = (long)env;
	q[3] = (pitch1 << 16) + pitch2;
	MCU_WcmdqInc();
}  //  end of Synthesize::DoPitch



static void MCU_DoPause(int length)
{//============================
	int len;

	len = length * MCU_SPEED_FACTOR1;//speed_factor1;
	if(len < 1000) len = 1000;      // limit the amount to which pauses can be shortened
	len = len / 256;

	//len = (len * samplerate) / 1000;  // convert from mS to number of samples
	len = len * 8;	//fixed samplerate = 8000

	MCU_EndPitch(1);
	MCU_wcmdq[MCU_wcmdq_tail][0] = WCMD_PAUSE;
	MCU_wcmdq[MCU_wcmdq_tail][1] = len;
	MCU_WcmdqInc();
	last_frame = NULL;
}  // end of Synthesize::DoPause



static void MCU_DoSample2(int index, int which, int length_mod, int amp)
{//=================================================================
	int length;
	int length1;
	int format;
	long *q;
	unsigned char *p;

	index = index & 0x7fffff;
	p = &MCU_wavefile_data[index];
	format = p[2];          //TODO: remove format from wave files how many bytes?
	length1 = (p[1] * 256);
	length1 += p[0];    //  length in bytes
	if(length_mod > 0)
		length = (length1 * length_mod) / 256;
	else
		length = length1;

	length = (length * MCU_SPEED_FACTOR2 /*speed_factor2*/)/256;
	if(length > length1)
		length = length1;  // don't exceed wavefile length

	if(format==0)
		length /= 2;     // 2 byte samples

	index += 4;

	MCU_last_wcmdq = MCU_wcmdq_tail;
	q = MCU_wcmdq[MCU_wcmdq_tail];
	if(which & 0x100)
		q[0] = WCMD_WAVE2;    // mix this with synthesised wave
	else
		q[0] = WCMD_WAVE;
	q[1] = length;   // length in samples
	q[2] = (long) (&MCU_wavefile_data[index]);
	q[3] = format + (amp << 8);
	MCU_WcmdqInc();

}  // end of Synthesize::DoSample2



static void MCU_DoSample(MCU_PHONEME_TAB *ph1, MCU_PHONEME_TAB *ph2,
						int which, int length_mod, int amp)
{//=========================================================================================
	int index;
	int match_level;
	int amp2;

	MCU_EndPitch(1);
	index = MCU_LookupSound(ph1,ph2,which & 0xff,&match_level,0);
	if((index & 0x800000) == 0)
		return;             // not wavefile data

	amp2 = MCU_wavefile_amp;    //???where
	if(amp != 0)
		amp2 = (amp * MCU_wavefile_amp)/20;

	MCU_DoSample2(index,which,length_mod,amp2);
	last_frame = NULL;
}  // end of MCUDoSample




static MCU_frame_t *MCU_AllocFrame()
{//=========================
	// Allocate a temporary spectrum frame for the wavegen queue. Use a pool which is big
	// enough to use a round-robin without checks.
	// Only needed for modifying spectra for blending to consonants

#define N_FRAME_POOL  MCU_N_WCMDQ
	static int ix=0;
	static MCU_frame_t frame_pool[N_FRAME_POOL];

	ix++;
	if(ix >= N_FRAME_POOL)
		ix = 0;
	return(&frame_pool[ix]);
}


static void set_frame_rms(MCU_frame_t *fr, int new_rms)
{//=================================================
// Each frame includes its RMS amplitude value, so to set a new
// RMS just adjust the formant amplitudes by the appropriate ratio

	int x;
	int h;
	int ix;

	static const short sqrt_tab[100] = {
     0, 64, 90,110,128,143,156,169,181,192,202,212,221,230,239,247,
   256,263,271,278,286,293,300,306,313,320,326,332,338,344,350,356,
   362,367,373,378,384,389,394,399,404,409,414,419,424,429,434,438,
   443,448,452,457,461,465,470,474,478,483,487,491,495,499,503,507,
   512,515,519,523,527,531,535,539,543,546,550,554,557,561,565,568,
   572,576,579,583,586,590,593,596,600,603,607,610,613,617,620,623,
   627,630,633,636 };

	if(fr->rms == 0) return;    // check for divide by zero
	x = (new_rms * 64)/fr->rms;
	if(x >= 100) x = 99;

	x = sqrt_tab[x];   // sqrt(new_rms/fr->rms)*0x200;

	for(ix=0; ix<MCU_N_PEAKS; ix++)
	{
		h = fr->fheight[ix] * x;
		fr->fheight[ix] = h/0x200;
	}
}   /* end of set_frame_rms */



static void formants_reduce_hf(MCU_frame_t *fr, int level)
{//====================================================
//  change height of peaks 2 to 8, percentage
	int  ix;
	int  x;

	for(ix=2; ix<MCU_N_PEAKS; ix++)
	{
		x = fr->fheight[ix] * level;
		fr->fheight[ix] = x/100;
	}
}


static MCU_frame_t *MCU_CopyFrame(MCU_frame_t *frame1, int copy)
{//=================================================
//  create a copy of the specified frame in temporary buffer
	MCU_frame_t *frame2;

	if((copy==0) && (frame1->frflags & FRFLAG_COPIED))
	{
		// this frame has already been copied in temporary rw memory
		return(frame1);
	}

	frame2 = MCU_AllocFrame();
	if(frame2 != NULL)
	{
		memcpy(frame2,frame1,sizeof(MCU_frame_t));
		frame2->length = 0;
		frame2->frflags |= FRFLAG_COPIED;
	}
	return(frame2);
}


static MCU_frame_t *MCU_DuplicateLastFrame(MCU_frameref_t *seq, int n_frames, int length)
{//==========================================================================
	MCU_frame_t *fr;    //???frameref

	seq[n_frames-1].length = length;
	fr = MCU_CopyFrame(seq[n_frames-1].frame,1);
	seq[n_frames].frame = fr;
	seq[n_frames].length = 0;
	return fr;
}


static void MCU_AdjustFormants(MCU_frame_t *fr, int target, int min, int max,
							   int f1_adj, int f3_adj, int hf_reduce, int flags)
{//====================================================================================================================
	int x;

//hf_reduce = 70;      // ?? using fixed amount rather than the parameter??

	x = (target - fr->ffreq[2]) / 2;
	if(x > max) x = max;
	if(x < min) x = min;
	fr->ffreq[2] += x;
	fr->ffreq[3] += f3_adj;

	if(flags & 0x20)
	{
		f3_adj = -f3_adj;   //. reverse direction for f4,f5 change
	}
	fr->ffreq[4] += f3_adj;
	fr->ffreq[5] += f3_adj;

	if(f1_adj==1)
	{
		x = (235 - fr->ffreq[1]);
		if(x < -100) x = -100;
		if(x > -60) x = -60;
		fr->ffreq[1] += x;
	}
	if(f1_adj==2)
	{
		x = (235 - fr->ffreq[1]);
		if(x < -300) x = -300;
		if(x > -150) x = -150;
		fr->ffreq[1] += x;
		fr->ffreq[0] += x;
	}
	if(f1_adj==3)
	{
		x = (100 - fr->ffreq[1]);
		if(x < -400) x = -400;
		if(x > -300) x = -400;
		fr->ffreq[1] += x;
		fr->ffreq[0] += x;
	}
	formants_reduce_hf(fr,hf_reduce); 
}


int MCU_VowelCloseness(MCU_frame_t *fr)
{//============================
// return a value 0-3 depending on the vowel's f1
	int f1;

	if((f1 = fr->ffreq[1]) < 300)
		return(3);
	if(f1 < 400)
		return(2);
	if(f1 < 500)
		return(1);
	return(0);
}


int MCU_FormantTransition2(MCU_frameref_t *seq, int* n_frames, unsigned int data1,
						   unsigned int data2, MCU_PHONEME_TAB *other_ph, int which)
{//==============================================================================================================================
	int ix;
	int formant;

	int len;
	int rms;
	int f1;
	int f2;
	int f2_min;
	int f2_max;
	int f3_adj;
	int f3_amp;
	int flags;
	int vcolour;

#define N_VCOLOUR  2
// percentage change for each formant in 256ths
static short vcolouring[N_VCOLOUR][5] = {
	{243,272,256,256,256},         // palatal consonant follows
	{256,256,240,240,240},         // retroflex
};

	MCU_frame_t *fr = NULL;

	if(*n_frames < 2)
		return(0);

	len = (data1 & 0x3f) * 2;
	rms = ((data1 >> 6) & 0x3f) * 2;
	flags = (data1 >> 12);

	f2 = (data2 & 0x3f) * 50;
	f2_min = (((data2 >> 6) & 0x1f) - 15) * 50;
	f2_max = (((data2 >> 11) & 0x1f) - 15) * 50;
	f3_adj = (((data2 >> 16) & 0x1f) - 15) * 50;
	f3_amp = ((data2 >> 21) & 0x1f) * 8;
	f1 = ((data2 >> 26) & 0x7);
	vcolour = (data2 >> 29);

//	fprintf(stderr,"FMT%d %3s  %3d-%3d f1=%d  f2=%4d %4d %4d  f3=%4d %3d\n",
//		which,WordToString(other_ph->mnemonic),len,rms,f1,f2,f2_min,f2_max,f3_adj,f3_amp);

	if(other_ph->mnemonic == '?')
		flags |= 8;

	if(which == 1)
	{
		/* entry to vowel */
		fr = MCU_CopyFrame(seq[0].frame,0);
		seq[0].frame = fr;
		seq[0].length = VOWEL_FRONT_LENGTH;
		if(len > 0)
			seq[0].length = len;
		seq[0].frflags |= FRFLAG_LEN_MOD;              // reduce length modification
		fr->frflags |= FRFLAG_LEN_MOD;

		if(f2 != 0)
		{
			MCU_AdjustFormants(fr, f2, f2_min, f2_max, f1, f3_adj, f3_amp, flags);
			set_frame_rms(fr,rms);
		}
		else
		{
			set_frame_rms(fr,RMS_START);
		}

		if(flags & 8)
		{
			set_frame_rms(fr,seq[1].frame->rms - 5);
			modn_flags = 0x800 + (MCU_VowelCloseness(fr) << 8);
		}
	}
	else
	{
		// exit from vowel

		if((f2 != 0) || (flags != 0))
		{

			if(flags & 8)
			{
				fr = MCU_CopyFrame(seq[*n_frames-1].frame,0);
				seq[*n_frames-1].frame = fr;
				rms = RMS_GLOTTAL1;
	
				// degree of glottal-stop effect depends on closeness of vowel (indicated by f1 freq)
				modn_flags = 0x400 + (MCU_VowelCloseness(fr) << 8);
			}
			else
			{
				fr = MCU_DuplicateLastFrame(seq,*n_frames++,len);
	
				if(f2 != 0)
				{
					MCU_AdjustFormants(fr, f2, f2_min, f2_max, f1, f3_adj, f3_amp, flags);
				}
			}

			set_frame_rms(fr,rms);

			if((vcolour > 0) && (vcolour <= N_VCOLOUR))
			{
				for(ix=0; ix<*n_frames; ix++)
				{
					fr = MCU_CopyFrame(seq[ix].frame,0);
					seq[ix].frame = fr;
					
					for(formant=1; formant<=5; formant++)
					{
						int x;
						x = fr->ffreq[formant] * vcolouring[vcolour-1][formant-1];
						fr->ffreq[formant] = x / 256;
					}
				}
			}
		}
	}

	if(fr != NULL)
	{
		if(flags & 4)
			fr->frflags |= FRFLAG_FORMANT_RATE;
		if(flags & 2)
			fr->frflags |= FRFLAG_BREAK;       // don't merge with next frame
	}

	if(flags & 0x40)
		MCU_DoPause(12);  // add a short pause after the consonant

	if(flags & 16)
		return(len);
	return(0);
} //  end of FormantTransition2



static void MCU_SmoothSpect(void)
{//==========================
	// Limit the rate of frequence change of formants, to reduce chirping

	long *q;
	MCU_frame_t *frame;
	MCU_frame_t *frame2;
	MCU_frame_t *frame1;
	MCU_frame_t *frame_centre;
	int ix;
	int len;
	int pk;
	int modified;
	int allowed;
	int diff;

	if(syllable_start == syllable_end)
		return;

	if((syllable_centre < 0) || (syllable_centre == syllable_start))
	{
		syllable_start = syllable_end;
		return;
	}

	q = MCU_wcmdq[syllable_centre];
	frame_centre = (MCU_frame_t *)q[2];

	// backwards
	ix = syllable_centre -1;
	frame = frame2 = frame_centre;
	for(;;)
	{
		if(ix < 0) ix = MCU_N_WCMDQ-1;
		q = MCU_wcmdq[ix];

		if(q[0] == WCMD_PAUSE || q[0] == WCMD_WAVE)
			break;

		if(q[0] == WCMD_SPECT || q[0] == WCMD_SPECT2)
		{
			len = q[1] & 0xffff;

			frame1 = (MCU_frame_t *)q[3];
			if(frame1 == frame)
			{
				q[3] = (long)frame2;
				frame1 = frame2;
			}
			else
				break;  // doesn't follow on from previous frame

			frame = frame2 = (MCU_frame_t *)q[2];
			modified = 0;

			if(frame->frflags & FRFLAG_FORMANT_RATE)
				len = (len * 13)/10;      // allow slightly greater rate of change for this frame (was 12/10)

			for(pk=0; pk<6; pk++)
			{
				allowed = (MCU_formant_rate[pk] * len)/256;

				diff = frame->ffreq[pk] - frame1->ffreq[pk];
				if(diff > allowed)
				{
					if(modified == 0)
					{
						frame2 = MCU_CopyFrame(frame,0);
						modified = 1;
					}
					frame2->ffreq[pk] = frame1->ffreq[pk] + allowed;
					q[2] = (long)frame2;
				}
				else
				if(diff < -allowed)
				{
					if(modified == 0)
					{
						frame2 = MCU_CopyFrame(frame,0);
						modified = 1;
					}
					frame2->ffreq[pk] = frame1->ffreq[pk] - allowed;
					q[2] = (long)frame2;
				}
			}
		}

		if(ix == syllable_start)
			break;
		ix--;
	}

	// forwards
	ix = syllable_centre;

	frame = NULL;
	for(;;)
	{
		q = MCU_wcmdq[ix];

		if(q[0] == WCMD_PAUSE || q[0] == WCMD_WAVE)
			break;

		if(q[0] == WCMD_SPECT || q[0] == WCMD_SPECT2)
		{

			len = q[1] & 0xffff;

			frame1 = (MCU_frame_t *)q[2];
			if(frame != NULL)
			{
				if(frame1 == frame)
				{
					q[2] = (long)frame2;
					frame1 = frame2;
				}
				else
					break;  // doesn't follow on from previous frame
			}

			frame = frame2 = (MCU_frame_t *)q[3];
			modified = 0;

			if(frame->frflags & FRFLAG_FORMANT_RATE)
				len = (len *6)/5;      // allow slightly greater rate of change for this frame

			for(pk=0; pk<6; pk++)
			{
				allowed = (MCU_formant_rate[pk] * len)/256;

				diff = frame->ffreq[pk] - frame1->ffreq[pk];
				if(diff > allowed)
				{
					if(modified == 0)
					{
						frame2 = MCU_CopyFrame(frame,0);
						modified = 1;
					}
					frame2->ffreq[pk] = frame1->ffreq[pk] + allowed;
					q[3] = (long)frame2;
				}
				else
				if(diff < -allowed)
				{
					if(modified == 0)
					{
						frame2 = MCU_CopyFrame(frame,0);
						modified = 1;
					}
					frame2->ffreq[pk] = frame1->ffreq[pk] - allowed;
					q[3] = (long)frame2;
				}
			}
		}

		ix++;
		if(ix >= MCU_N_WCMDQ) ix = 0;
		if(ix == syllable_end)
			break;
	}

	syllable_start = syllable_end;
}


static void MCU_StartSyllable(void)
{//============================
	// start of syllable, if not already started
	if(syllable_end == syllable_start)
		syllable_end = MCU_wcmdq_tail;
}


static void MCU_DoSpect(MCU_PHONEME_TAB *this_ph, MCU_PHONEME_TAB *prev_ph, MCU_PHONEME_TAB *next_ph,
		int which, MCU_PHONEME_LIST *plist, int modulation)
{//===================================================================================
	// which  1  start of phoneme,   2 body and end
	// length_mod: 256 = 100%

	int  n_frames;
	MCU_frameref_t *frames;
	int  frameix;
	MCU_frame_t *frame1;
	MCU_frame_t *frame2;
	MCU_frame_t *fr;
	int  ix;
	long *q;
	int  len;
	int  match_level;
	int  frame_length;
	int  frame1_length;
	int  frame2_length;
	int  length_factor;
	int  length_mod;
	static int wave_flag = 0;
	int wcmd_spect = WCMD_SPECT;

	length_mod = plist->length;
	if(length_mod==0) length_mod=256;

if(which==1)
{
	// limit the shortening of sonorants before shortened (eg. unstressed vowels)
	if((this_ph->type==phLIQUID) || (prev_ph->type==phLIQUID) || (prev_ph->type==phNASAL))
	{
		if(length_mod < (len = 20) )//(len = translator->langopts.param[LOPT_SONORANT_MIN]))
		{
			length_mod = len;
		}
	}
}

	modn_flags = 0;
	frames = MCU_LookupSpect(this_ph,prev_ph,next_ph,which,&match_level,&n_frames, plist);
	if(frames == NULL)
		return;   // not found

	if(MCU_wavefile_ix == 0)
	{
		if(wave_flag)
		{
			// cancel any wavefile that was playing previously
			wcmd_spect = WCMD_SPECT2;
			wave_flag = 0;
		}
		else
		{
			wcmd_spect = WCMD_SPECT;
		}
	}

	frame1 = frames[0].frame;
	frame1_length = frames[0].length;
	if(last_frame != NULL)
	{
		if(((last_frame->length < 2) || (last_frame->frflags & FRFLAG_VOWEL_CENTRE))
			&& !(last_frame->frflags & FRFLAG_BREAK))
		{
			// last frame of previous sequence was zero-length, replace with first of this sequence
			MCU_wcmdq[MCU_last_wcmdq][3] = (long)frame1;

			if(last_frame->frflags & FRFLAG_BREAK_LF)
			{
				// but flag indicates keep HF peaks in last segment
				fr = MCU_CopyFrame(frame1,1);
				for(ix=3; ix<MCU_N_PEAKS; ix++)
				{
					fr->ffreq[ix] = last_frame->ffreq[ix];
					fr->fheight[ix] = last_frame->fheight[ix];
				}
				MCU_wcmdq[MCU_last_wcmdq][3] = (long)fr;
			}
		}
	}

	if((this_ph->type == phVOWEL) && (which == 2))
	{
		MCU_SmoothSpect();    // process previous syllable

		// remember the point in the output queue of the centre of the vowel
		syllable_centre = MCU_wcmdq_tail;
	}

	frame_length = frame1_length;
	for(frameix=1; frameix<n_frames; frameix++)
	{
		frame2 = frames[frameix].frame;
		frame2_length = frames[frameix].length;

		if((MCU_wavefile_ix != 0) && ((frame1->frflags & FRFLAG_DEFER_WAV)==0))
		{
			// there is a wave file to play along with this synthesis
			MCU_DoSample2(MCU_wavefile_ix,which+0x100,0,MCU_wavefile_amp);
			wave_flag = 1;
			MCU_wavefile_ix = 0;
		}

		length_factor = length_mod;
		if(frame1->frflags & FRFLAG_LEN_MOD)     // reduce effect of length mod
		{
			length_factor = (length_mod + 256)/2;
		}
		if(frame1->frflags & FRFLAG_MODULATE)
		{
			modulation = 6;
		}
		len = frame_length*8;		//(frame_length * samplerate)/1000;
		len = (len * length_factor)/256;

		if((frameix == n_frames-1) && (modn_flags & 0xf00))
			modulation |= modn_flags;   // before or after a glottal stop

		pitch_length += len;
		amp_length += len;

		if(frame_length < 2)
		{
			last_frame = NULL;
			frame_length = frame2_length;
			frame1 = frame2;
		}
		else
		{
			MCU_last_wcmdq = MCU_wcmdq_tail;
			q = MCU_wcmdq[MCU_wcmdq_tail];
			q[0] = wcmd_spect;
			q[1] = len + (modulation << 16);
			q[2] = (long) (frame1);
			q[3] = (long) (frame2);
			MCU_WcmdqInc();
			last_frame = frame1 = frame2;
			frame_length = frame2_length;
		}
	}
}  // end of Synthesize::MCUDoSpect


static void MCU_DoMarker(int type, int char_posn, int length, int value)
{//=================================================================
// This could be used to return an index to the word currently being spoken
// Type 1=word, 2=sentence, 3=named marker, 4=play audio, 5=end
	MCU_wcmdq[MCU_wcmdq_tail][0] = WCMD_MARKER;
	MCU_wcmdq[MCU_wcmdq_tail][1] = type;
	MCU_wcmdq[MCU_wcmdq_tail][2] = (char_posn & 0xffffff) | (length << 24);
	MCU_wcmdq[MCU_wcmdq_tail][3] = value;
	MCU_WcmdqInc();

}  // end of MCU_DoMarker



/*static void DoVoice(voice_t *v)
{//============================
	MCUwcmdq[MCUwcmdq_tail][0] = WCMD_VOICE;
	MCUwcmdq[MCUwcmdq_tail][1] = (long)v;
	MCUWcmdqInc();
}
*/


/*static void DoEmbedded(int &embix, int sourceix)
{//=============================================
	// There were embedded commands in the text at this point
	unsigned int word;  // bit 7=last command for this word, bits 5,6 sign, bits 0-4 command
	unsigned int value;
	int command;

	do {
		word = embedded_list[embix++];
		value = word >> 8;
		command = word & 0x7f;

		switch(command & 0x1f)
		{
		case EMBED_S:   // speed
			SetEmbedded((command & 0x60) + EMBED_S2,value);   // adjusts embedded_value[EMBED_S2]
			SetSpeed(2);
			break;

		case EMBED_I:   // play dynamically loaded wav data (sound icon)
			if((int)value < n_soundicon_tab)
			{
				if((MCUwcmdq[MCUwcmdq_tail][1] = soundicon_tab[value].length) != 0)
				{
					DoPause(10);   // ensure a break in the speech
					MCUwcmdq[MCUwcmdq_tail][0] = WCMD_WAVE;
					MCUwcmdq[MCUwcmdq_tail][1] = soundicon_tab[value].length;
					MCUwcmdq[MCUwcmdq_tail][2] = (long)soundicon_tab[value].data;
					MCUwcmdq[MCUwcmdq_tail][3] = 0;   // 16 bit data
					MCUWcmdqInc();
				}
			}
			break;

		case EMBED_M:   // named marker
			DoMarker(espeakEVENT_MARK, (sourceix & 0x7ff) + clause_start_char, 0, value);
			break;

		case EMBED_U:   // play sound
			DoMarker(espeakEVENT_PLAY, count_characters+1, 0, value);  // always occurs at end of clause
			break;

		default:
			DoPause(10);   // ensure a break in the speech
			MCUwcmdq[MCUwcmdq_tail][0] = WCMD_EMBEDDED;
			MCUwcmdq[MCUwcmdq_tail][1] = command;
			MCUwcmdq[MCUwcmdq_tail][2] = value;
			MCUWcmdqInc();
			break;
		}
	} while ((word & 0x80) == 0);
}
*/

/*void SwitchDictionary()
{//====================
}
*/


int MCU_Generate(MCU_PHONEME_LIST *phoneme_list, int *n_ph, int resume)
{//============================================================
	static int  ix;
//	static int  embedded_ix;
	MCU_PHONEME_LIST *prev;
	MCU_PHONEME_LIST *next;
	MCU_PHONEME_LIST *next2;
	MCU_PHONEME_LIST *p;
	int  released;
	int  stress;
	int  modulation;
	int  pre_voiced;
//	int  word_count = 0;
	unsigned char *pitch_env=NULL;
	unsigned char *amp_env;
	MCU_PHONEME_TAB *ph;

	if(resume == 0)
	{
		ix = 1;
//		embedded_ix=0;
		pitch_length = 0;
		amp_length = 0;
		last_frame = NULL;
		MCU_last_wcmdq = -1;
		syllable_start = MCU_wcmdq_tail;
		syllable_end = MCU_wcmdq_tail;
		syllable_centre = -1;
		last_pitch_cmd = -1;
		memset(MCU_vowel_transition,0,sizeof(MCU_vowel_transition));
	}

	while(ix < (*n_ph))
	{
		if(MCU_WcmdqFree() <= MCU_MIN_WCMDQ)
			return(1);  // wait

		prev = &phoneme_list[ix-1];
		p = &phoneme_list[ix];
		next = &phoneme_list[ix+1];
		next2 = &phoneme_list[ix+2];
/*
		if(p->synthflags & SFLAG_EMBEDDED)
		{
			DoEmbedded(embedded_ix, p->sourceix);
		}
*/
		if(p->newword)
		{
			last_frame = NULL;

/*			if(p->newword & 4)
				DoMarker(espeakEVENT_SENTENCE, (p->sourceix & 0x7ff) + clause_start_char, 0, count_sentences);  // start of sentence

//			if(p->newword & 2)
//				DoMarker(espeakEVENT_END, count_characters, 0, count_sentences);  // end of clause
*/
			if(p->newword & 1)
				MCU_DoMarker(MCU_espeakEVENT_WORD, p->sourceix & 0x7ff, 0, 0);

		}

		if(( /*translator->langopts.word_gap & 1) || (translator->langopts.vowel_pause && */
			(next->type == phVOWEL)))
		{
			// prevent word merging into next, make it look as though next is a pause
			if((next->newword) && (next->type != phPAUSE))
			{
				next_pause.ph = phonPAUSE;
				next = &next_pause;
			}
		}

		MCU_EndAmplitude();

		if(p->prepause > 0)
			MCU_DoPause(p->prepause);

		switch(p->type)
		{
		case phPAUSE:
			MCU_DoPause(p->length);
			break;

		case phSTOP:
			released = 0;
			if(next->type==phVOWEL) released = 1;
			if(next->type==phLIQUID && !next->newword) released = 1;

			if(released)
				MCU_DoSample(&MCU_phoneme_tab[p->ph],&MCU_phoneme_tab[next->ph],2,0,0);
			else
				MCU_DoSample(&MCU_phoneme_tab[p->ph],&MCU_phoneme_tab[phonPAUSE],2,0,0);
			break;

		case phFRICATIVE:
			if(p->synthflags & SFLAG_LENGTHEN)
				MCU_DoSample(&MCU_phoneme_tab[p->ph],&MCU_phoneme_tab[next->ph],2,p->length,0);  // play it twice for [s:] etc.
			MCU_DoSample(&MCU_phoneme_tab[p->ph],&MCU_phoneme_tab[next->ph],2,p->length,0);
			break;

		case phVSTOP:
			pre_voiced = 0;
			if(next->type==phVOWEL)
			{
				MCU_DoAmplitude(p->amp,NULL);
				MCU_DoPitch(MCU_envelope_data[p->env],p->pitch1,p->pitch2);
				pre_voiced = 1;
			}
			else
			if((next->type==phLIQUID) && !next->newword)
			{
				MCU_DoAmplitude(next->amp,NULL);
				MCU_DoPitch(MCU_envelope_data[next->env],next->pitch1,next->pitch2);
				pre_voiced = 1;
			}
			else
			{
				if(last_pitch_cmd < 0)
				{
					MCU_DoAmplitude(next->amp,NULL);
					MCU_DoPitch(MCU_envelope_data[p->env],p->pitch1,p->pitch2);
				}
			}

			if((MCU_phoneme_tab[prev->ph].type==phVOWEL) ||
				(MCU_phoneme_tab[prev->ph].phflags & phVOWEL2))
			{
				// a period of voicing before the release
				MCU_DoSpect(&MCU_phoneme_tab[p->ph],&MCU_phoneme_tab[phonSCHWA],
					&MCU_phoneme_tab[next->ph],1,p,0);
				if(p->synthflags & SFLAG_LENGTHEN)
				{
					MCU_DoPause(20);
					MCU_DoSpect(&MCU_phoneme_tab[p->ph],
						&MCU_phoneme_tab[phonSCHWA],&MCU_phoneme_tab[next->ph],1,p,0);
				}
			}

			if(pre_voiced)
			{
				// followed by a vowel, or liquid + vowel
				MCU_StartSyllable();
				MCU_DoSpect(&MCU_phoneme_tab[p->ph],&MCU_phoneme_tab[prev->ph],
					&MCU_phoneme_tab[next->ph],2,p,0);
			}
			else
			{
//				if((prev->type != phVOWEL) && ((prev->ph->phflags & phVOICED)==0) && ((next->ph->phflags & phVOICED)==0))
//					MCUDoSpect(p->ph,prev->ph,phoneme_tab[phonPAUSE_SHORT],2,p,0);
//				else
					MCU_DoSpect(&MCU_phoneme_tab[p->ph],&MCU_phoneme_tab[prev->ph],
						&MCU_phoneme_tab[phonPAUSE],2,p,0);
//				MCUDoSpect(p->ph,prev->ph,next->ph,2,p,0);
			}
			break;

		case phVFRICATIVE:
			if(next->type==phVOWEL)
			{
				MCU_DoAmplitude(p->amp,NULL);
				MCU_DoPitch(MCU_envelope_data[p->env],p->pitch1,p->pitch2);
			}
			else
			if(next->type==phLIQUID)
			{
				MCU_DoAmplitude(next->amp,NULL);
				MCU_DoPitch(MCU_envelope_data[next->env],next->pitch1,next->pitch2);
			}
			else
			{
				if(last_pitch_cmd < 0)
				{
					MCU_DoAmplitude(p->amp,NULL);
					MCU_DoPitch(MCU_envelope_data[p->env],p->pitch1,p->pitch2);
				}
			}

			if((next->type==phVOWEL) || (next->type==phLIQUID))
			{
				MCU_StartSyllable();
				if(p->synthflags & SFLAG_LENGTHEN)
					MCU_DoSpect(&MCU_phoneme_tab[p->ph],&MCU_phoneme_tab[prev->ph],
					&MCU_phoneme_tab[next->ph],2,p,0);
				MCU_DoSpect(&MCU_phoneme_tab[p->ph],&MCU_phoneme_tab[prev->ph],
					&MCU_phoneme_tab[next->ph],2,p,0);
			}
			else
			{
				if(p->synthflags & SFLAG_LENGTHEN)
					MCU_DoSpect(&MCU_phoneme_tab[p->ph],&MCU_phoneme_tab[prev->ph],
					&MCU_phoneme_tab[phonPAUSE],2,p,0);
				MCU_DoSpect(&MCU_phoneme_tab[p->ph],&MCU_phoneme_tab[prev->ph],
					&MCU_phoneme_tab[phonPAUSE],2,p,0);
			}
			break;

		case phNASAL:
			if(!(p->synthflags & SFLAG_SEQCONTINUE))
			{
				MCU_DoAmplitude(p->amp,NULL);
				MCU_DoPitch(MCU_envelope_data[p->env],p->pitch1,p->pitch2);
			}

			if(prev->type==phNASAL)
			{
				last_frame = NULL;
			}

			if(next->type==phVOWEL)
			{
				MCU_StartSyllable();
				MCU_DoSpect(&MCU_phoneme_tab[p->ph],&MCU_phoneme_tab[prev->ph],
					&MCU_phoneme_tab[next->ph],1,p,0);
			}
			else
			if(prev->type==phVOWEL && (p->synthflags & SFLAG_SEQCONTINUE))
			{
				MCU_DoSpect(&MCU_phoneme_tab[p->ph],&MCU_phoneme_tab[prev->ph],
					&MCU_phoneme_tab[phonPAUSE],2,p,0);
			}
			else
			{
				last_frame = NULL;  // only for nasal ?
				if(next->type == phLIQUID)
					MCU_DoSpect(&MCU_phoneme_tab[p->ph],&MCU_phoneme_tab[prev->ph],
					&MCU_phoneme_tab[phonSONORANT],2,p,0);
				else
					MCU_DoSpect(&MCU_phoneme_tab[p->ph],&MCU_phoneme_tab[prev->ph],
					&MCU_phoneme_tab[phonPAUSE],2,p,0);
				last_frame = NULL;
			}

			break;

		case phLIQUID:
			modulation = 0;
			if(MCU_phoneme_tab[p->ph].phflags & phTRILL)
				modulation = 5;

			if(!(p->synthflags & SFLAG_SEQCONTINUE))
			{
				MCU_DoAmplitude(p->amp,NULL);
				MCU_DoPitch(MCU_envelope_data[p->env],p->pitch1,p->pitch2);
			}

			if(prev->type==phNASAL)
			{
				last_frame = NULL;
			}

			if(next->type==phVOWEL)
			{
				MCU_StartSyllable();
				MCU_DoSpect(&MCU_phoneme_tab[p->ph],&MCU_phoneme_tab[prev->ph],
					&MCU_phoneme_tab[next->ph],1,p,modulation);  // (,)r
			}
			else
			if(prev->type==phVOWEL && (p->synthflags & SFLAG_SEQCONTINUE))
			{
				MCU_DoSpect(&MCU_phoneme_tab[p->ph],
					&MCU_phoneme_tab[prev->ph],&MCU_phoneme_tab[next->ph],1,p,modulation);
			}
			else
			{
				MCU_DoSpect(&MCU_phoneme_tab[p->ph],&MCU_phoneme_tab[prev->ph],
					&MCU_phoneme_tab[next->ph],1,p,modulation);
			}

			break;

		case phVOWEL:
			ph = &MCU_phoneme_tab[p->ph];
			stress = p->tone & 0xf;

			// vowel transition from the preceding phoneme
			MCU_vowel_transition0 = MCU_vowel_transition[0];
			MCU_vowel_transition1 = MCU_vowel_transition[1];

			pitch_env = (unsigned char*) MCU_envelope_data[p->env];
			amp_env = NULL;
/*			if(p->tone_ph != 0)
			{
				pitch_env = MCU_LookupEnvelope(MCU_phoneme_tab[p->tone_ph].spect);
				amp_env = MCU_LookupEnvelope(MCU_phoneme_tab[p->tone_ph].after);
			}*/

			MCU_StartSyllable();

			modulation = 2;
			if(stress <= 1)
				modulation = 1;  // 16ths
			else
			if(stress >= 7)
				modulation = 3;

			if(prev->type == phVSTOP || prev->type == phVFRICATIVE)
			{
				MCU_DoAmplitude(p->amp,amp_env);
				MCU_DoPitch(pitch_env,p->pitch1,p->pitch2);  // don't use prevocalic rising tone
				MCU_DoSpect(ph,&MCU_phoneme_tab[prev->ph],&MCU_phoneme_tab[next->ph],1,p,modulation);
			}
			else
			if(prev->type==phLIQUID || prev->type==phNASAL)
			{
				MCU_DoAmplitude(p->amp,amp_env);
				MCU_DoSpect(ph,&MCU_phoneme_tab[prev->ph],&MCU_phoneme_tab[next->ph],1,p,modulation);   // continue with pre-vocalic rising tone
				MCU_DoPitch(pitch_env,p->pitch1,p->pitch2);
			}
			else
			{
				if(!(p->synthflags & SFLAG_SEQCONTINUE))
				{
					MCU_DoAmplitude(p->amp,amp_env);
					MCU_DoPitch(pitch_env,p->pitch1,p->pitch2);
				}

				MCU_DoSpect(ph,&MCU_phoneme_tab[prev->ph],&MCU_phoneme_tab[next->ph],1,p,modulation);
			}

			MCU_DoSpect(&MCU_phoneme_tab[p->ph],&MCU_phoneme_tab[prev->ph],
				&MCU_phoneme_tab[next->ph],2,p,modulation);

			memset(MCU_vowel_transition,0,sizeof(MCU_vowel_transition));
			break;
		}
		ix++;
	}
	MCU_EndPitch(1);
/*
	if(*n_ph > 0)
	{
		MCU_DoMarker(espeakEVENT_END, count_characters, 0, count_sentences);  // end of clause
		*n_ph = 0;
	}

	if(new_voice)
	{
		// finished the current clause, now change the voice if there was an embedded
		// change voice command at the end of it (i.e. clause was broken at the change voice command)
		MCU_DoVoice(new_voice);
		new_voice = NULL;
	}
*/
	return(0);  // finished the phoneme list
}  //  end of Generate





/* SynthOnTimer - could be useful on MCU
static int timer_on = 0;
static int paused = 0;

int SynthOnTimer()
{//===============
	if(!timer_on)
	{
		return(WavegenCloseSound());
	}

	do {
		if(Generate(phoneme_list,&n_phoneme_list,1)==0)
		{
			SpeakNextClause(NULL,NULL,1);
		}
	} while(skipping_text);

	return(0);
}


int SynthStatus()
{//==============
	return(timer_on | paused);
}

*/

/*int SpeakNextClause(FILE *f_in, const void *text_in, int control)
{//==============================================================
// Speak text from file (f_in) or memory (text_in)
// control 0: start
//    either f_in or text_in is set, the other must be NULL

// The other calls have f_in and text_in = NULL
// control 1: speak next text
//         2: stop
//         3: pause (toggle)
//         4: is file being read (0=no, 1=yes)
//         5: interrupt and flush current text.

	int clause_tone;
	char *voice_change;
	static FILE *f_text=NULL;
	static const void *p_text=NULL;

	if(control == 4)
	{
		if((f_text == NULL) && (p_text == NULL))
			return(0);
		else
			return(1);
	}

	if(control == 2)
	{
		// stop speaking
		timer_on = 0;
		p_text = NULL;
		if(f_text != NULL)
		{
			fclose(f_text);
			f_text=NULL;
		}
		n_phoneme_list = 0;
		MCUWcmdqStop();
		return(0);
	}

	if(control == 3)
	{
		// toggle pause
		if(paused == 0)
		{
			timer_on = 0;
			paused = 2;
		}
		else
		{
			WavegenOpenSound();
			timer_on = 1;
			paused = 0;
			Generate(phoneme_list,&n_phoneme_list,0);   // re-start from beginning of clause
		}
		return(0);
	}

	if(control == 5)
	{
		// stop speaking, but continue looking for text
		n_phoneme_list = 0;
		MCUWcmdqStop();
		return(0);
	}

	if((f_in != NULL) || (text_in != NULL))
	{
		f_text = f_in;
		p_text = text_in;
		timer_on = 1;
		paused = 0;
	}

	if((f_text==NULL) && (p_text==NULL))
	{
		skipping_text = 0;
		timer_on = 0;
		return(0);
	}

	if((f_text != NULL) && feof(f_text))
	{
		timer_on = 0;
		fclose(f_text);
		f_text=NULL;
		return(0);
	}

	// read the next clause from the input text file, translate it, and generate
	// entries in the wavegen command queue
	p_text = translator->TranslateClause(f_text,p_text,&clause_tone,&voice_change);

	if(option_phonemes > 0)
	{
		fprintf(f_trans,"%s\n",translator->phon_out);
	}
	translator->CalcPitches(clause_tone);
	translator->CalcLengths();

	if(voice_change != NULL)
	{
		// voice change at the end of the clause (i.e. clause was terminated by a voice change)
		new_voice = LoadVoice(voice_change,0); // add a Voice instruction to wavegen at the end of the clause
		if(new_voice != NULL)
			voice = new_voice; 
	}

	if(skipping_text)
	{
		n_phoneme_list = 0;
		return(1);
	}

	Generate(phoneme_list,&n_phoneme_list,0);
	WavegenOpenSound();

	return(1);
}  //  end of SpeakNextClause
*/
