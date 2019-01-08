/***************************************************************************
 *   Copyright (C) 2005,2006 by Jonathan Duddington                        *
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
/*
	armsynthesize.c
	Part of LMxxxx entry
*/
#if defined(__cplusplus)
extern "C"
{
#endif

#define MCU_N_PHONEME_LIST  64    // enough for source[] full of text, else it will truncate

#define MCU_MAX_HARMONIC   80           // 80 * 50Hz = 4 kHz, more than enough
#define N_SEQ_FRAMES   25           // max frames in a spectrum sequence (real max is ablut 8)

// definitions like in eSpeak
#define    PITCHfall   0
#define    PITCHrise   1

// flags set for frames within a spectrum sequence
#define FRFLAG_VOWEL_CENTRE    0x02   // centre point of vowel
#define FRFLAG_LEN_MOD         0x04   // reduce effect of length adjustment
#define FRFLAG_BREAK_LF        0x08   // but keep f3 upwards
#define FRFLAG_BREAK           0x10   // don't merge with next frame
#define FRFLAG_FORMANT_RATE    0x20   // Flag5 allow increased rate of change of formant freq
#define FRFLAG_MODULATE        0x40   // Flag6 modulate amplitude of some cycles to give trill
#define FRFLAG_DEFER_WAV       0x80   // Flag7 defer mixing WAV until the next frame
#define FRFLAG_COPIED        0x8000   // This frame has been copied into temporary rw memory

#define SFLAG_SEQCONTINUE      0x01   // a liquid or nasal after a vowel, but not followed by a vowel
#define SFLAG_EMBEDDED         0x02   // there are embedded commands before this phoneme
#define SFLAG_SYLLABLE         0x04   // vowel or syllabic consonant
#define SFLAG_LENGTHEN         0x08   // lengthen symbol : included after this phoneme
#define SFLAG_DICTIONARY       0x10   // the pronunciation of this word was listed in the xx_list dictionary
// End definitions like in eSpeak

/*not supporting embedded commands
// embedded command numbers
#define EMBED_P     1   // pitch
#define EMBED_S     2   // speed (used in setlengths)
#define EMBED_A     3   // amplitude/volume
#define EMBED_R     4   // pitch range/expression
#define EMBED_H     5   // echo/reverberation
#define EMBED_T     6   // different tone
#define EMBED_I     7   // sound icon
#define EMBED_S2    8   // speed (used in synthesize)
#define EMBED_Y     9   // say-as commands
#define EMBED_M    10   // mark name
#define EMBED_U    11   // audio uri
#define EMBED_B    12   // break
#define EMBED_F    13   // emphasis

#define N_EMBEDDED_VALUES    14
extern int embedded_value[N_EMBEDDED_VALUES];
extern int embedded_default[N_EMBEDDED_VALUES];
*/

typedef float MCU_DOUBLEX;
// formant data used by wavegen
typedef struct {
	int freq;     // Hz<<16
	int height;   // height<<15
	int left;     // Hz<<16
	int right;    // Hz<<16
	MCU_DOUBLEX freq1; // floating point versions of the above
	MCU_DOUBLEX height1;
	MCU_DOUBLEX left1;
	MCU_DOUBLEX right1;
	MCU_DOUBLEX freq_inc;    // increment by this every 64 samples
	MCU_DOUBLEX height_inc;
	MCU_DOUBLEX left_inc;
	MCU_DOUBLEX right_inc;
}  MCU_wavegen_peaks_t;

typedef struct {
   short length;
   unsigned char  n_frames;
   unsigned char  flags;
   MCU_frame_t  frame[MCU_N_SEQ_FRAMES];     // max. frames in a spectrum sequence
} MCU_SPECT_SEQ;

typedef struct {
	short length;
	short frflags;
	MCU_frame_t *frame;
} MCU_frameref_t;


typedef struct {
	unsigned char ph;		//MCU_PHONEME_TAB *ph;
	unsigned char env;    // pitch envelope number
	unsigned char tone;
	unsigned char type;
	unsigned char prepause;
	unsigned char amp;
//	unsigned char tone_ph;   // tone phoneme to use with this vowel
	unsigned char newword;   // bit 0=start of word, bit 1=end of clause, bit 2=start of sentence
	unsigned char synthflags;
	short length;  // length_mod
	char pitch1;  // pitch, 0-127 within the Voice's pitch range
	char pitch2;
	unsigned short sourceix;  // ix into the original source text string, only set at the start of a word
} MCU_PHONEME_LIST;

/*Not supporting icons
typedef struct {
	int name;
	int length;
	char *data;
	char *filename;
} SOUND_ICON;
*/
// phoneme table

#ifdef MCU_USE_MANY_TABLES
extern MCU_PHONEME_TAB* MCU_phoneme_tab[N_PHONEME_TAB];
#else
extern MCU_PHONEME_TAB* MCU_phoneme_tab;//[N_PHONEME_TAB];
#endif

// list of phonemes in a clause
extern int MCU_n_phoneme_list;
extern MCU_PHONEME_LIST MCU_phoneme_list[MCU_N_PHONEME_LIST];

/*not supporting embedded commands
extern unsigned int embedded_list[];
*/

//redefinition of envelopes
//extern unsigned char MCU_env_fall[128];

extern const unsigned char MCU_env_fall[128];
extern const unsigned char MCU_env_rise[128];
extern const unsigned char MCU_env_frise[128];


// queue of commands for wavegen
// like in eSpeak
#define WCMD_AMPLITUDE 1
#define WCMD_PITCH	2
#define WCMD_SPECT	3
#define WCMD_SPECT2	4
#define WCMD_PAUSE	5
#define WCMD_WAVE    6
#define WCMD_WAVE2   7
#define WCMD_MARKER	8
#define WCMD_VOICE   9
#define WCMD_EMBEDDED 10

//redefinition of wave command for reduce sizes
#define MCU_N_WCMDQ    64
#define MCU_MIN_WCMDQ  20   // need this many free entries before adding new phoneme

//TODO - try to define as a structure
extern long MCU_wcmdq[MCU_N_WCMDQ][4];
extern int MCU_wcmdq_head;
extern int MCU_wcmdq_tail;

// from Wavegen file
int  MCU_WcmdqFree(void);
void MCU_WcmdqStop(void);
int  MCU_WcmdqUsed(void);
void MCU_WcmdqInc(void);
/* Not needed in embedded aplications
int  WavegenOpenSound();
int  WavegenCloseSound();
int  WavegenInitSound();
*/
void MCU_WavegenInit(void); // not using int rate, int wavemult_fact);

/* Not needed in embedded aplications
int  OpenWaveFile(const char *path, int rate);
void CloseWaveFile(int rate);
*/

/*polint perhaps is needed only in setlenght
float polint(float xa[],float ya[],int n,float x);
*/
int MCU_WavegenFile(void);
int MCU_WavegenFill(int fill_zeros);

/*
void MarkerEvent(int type, unsigned int char_position, int value, unsigned char *out_ptr);
extern unsigned char *out_ptr;
extern unsigned char *out_end;
*/

extern unsigned char *MCU_wavefile_data;


//extern int samplerate;   fixed samplerate

extern int MCU_wavefile_ix;
extern int MCU_wavefile_amp;
extern int MCU_wavefile_ix2;
extern int MCU_wavefile_amp2;
extern int MCU_vowel_transition[4];
extern int MCU_vowel_transition0, MCU_vowel_transition1;

// from armsynthdata file
unsigned int MCU_LookupSound(MCU_PHONEME_TAB *ph1,
							MCU_PHONEME_TAB *ph2, int which, int *match_level, int control);
MCU_frameref_t *MCU_LookupSpect(MCU_PHONEME_TAB *ph1, MCU_PHONEME_TAB *prev_ph,
							  MCU_PHONEME_TAB *next_ph, int which, int *match_level, int *n_frames,
							  MCU_PHONEME_LIST *plist);

unsigned char *MCU_LookupEnvelope(int ix);
/*
int MCU_LoadPhData();
*/
void MCU_SynthesizeInit(void);
int  MCU_Generate(MCU_PHONEME_LIST *phoneme_list, int *n_ph, int resume);
void MCU_MakeWave2(MCU_PHONEME_LIST *p, int n_ph);
int MCU_LookupPh(const char *string);
/*
int  SynthOnTimer(void);
int  SpeakNextClause(FILE *f_text, const void *text_in, int control);
int  SynthStatus(void);
void SetSpeed(int control);
void SetEmbedded(int control, int value);
void SelectPhonemeTable(int number);
int  SelectPhonemeTableName(const char *name);
*/


//extern unsigned char *MCU_envelope_data[16];
//#define MCU_envelope_data envelope_data

extern const unsigned char *MCU_envelope_data[16];
//extern int MCU_formant_rate[];         // max rate of change of each formant


/*
#define N_SOUNDICON_TAB  100
extern int n_soundicon_tab;
extern SOUND_ICON soundicon_tab[N_SOUNDICON_TAB];

espeak_ERROR SetVoiceByName(const char *name);
espeak_ERROR SetVoiceByProperties(espeak_VOICE *voice_selector);
void SetParameter(int parameter, int value, int relative);
*/
#if defined(__cplusplus)
}
#endif 
