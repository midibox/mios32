/***************************************************************************
 *   mcu_translator.cpp                                                    *
 *   Copyright (C) 2007 by Nic Birsan & Ionut Tarsa                        *
 *                                                                         *
 *   -------------------------------------------------------------------   *
 *   high level translator/synthesis for embedded application              *
 *   this is a simplified version of Traslator class from eSpeak project   *
 *   -------------------------------------------------------------------   *
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

#include <string.h>
//#include <ctype.h>
#include "en_list.h"


#include "mcu_otherdef.h"
#include "mcu_phoneme.h"
#include "mcu_synthesize.h"

//definitions 
#define MCU_N_WORD_PHONEMES  30

const char *MCUWordToString(unsigned int word)
{//========================================
// Convert a phoneme mnemonic word into a string
	int  ix;
	static char buf[5];

	for(ix=0; ix<4; ix++)
		buf[ix] = word >> (ix*8);
	buf[4] = 0;
	return(buf);
}

int MCUHashDictionary(const char *string)
//====================================
/* Generate a hash code from the specified string
	This is used to access the dictionary_2 word-lookup dictionary
*/
{
   int  c;
   int  chars=0;
   int  hash=0;

   while((c = (*string++ & 0xff)) != 0)
   {
      hash = hash * 8 + c;
      hash = (hash & 0x3ff) ^ (hash >> 8);    /* exclusive or */
		chars++;
   }

   return((hash+chars) & 0x3ff);  // a 10 bit hash code
}   //  end of HashDictionary

#define FLAG_STRESS_END      0x800  /* full stress if at end of clause */

int MCULookupDictList(char *word, char *word2, char *phonetic, unsigned int *flags, int end_flags)
//======================================================================================================
/* Find an entry in the word_dict file for a specified word.
   Returns 1 if an entry is found

	word   zero terminated word to match
	word2  pointer to next word(s) in the input text (terminated by space)

	flags:  returns dictionary flags which are associated with a matched word

	end_flags:  indicates whether this is a retranslation after removing a suffix
*/
{
	char *p;
	char *next;
	int  hash;
	int  phoneme_len;
	int  wlen;
	unsigned char flag;
	unsigned int  dictionary_flags;
	int  condition_failed=0;
	int  n_chars;
	int  no_phonemes;
	char *word_end;
	char *word1;
	char word_buf[MCU_N_WORD_PHONEMES];

	word1 = word;
	wlen = strlen(word);

	hash = MCUHashDictionary(word);
	p = (char*) &dict_list_en[hash_index_en[hash]];

	if(p == NULL)
	{
		if(flags != NULL)
			*flags = 0;
		return(0);
	}

	while(*p != 0)
	{
		next = p + p[0];

		if(((p[1] & 0x7f) != wlen) || (memcmp(word,&p[2],wlen & 0x3f) != 0) || (p[1] != wlen))
		{
			// bit 6 of wlen indicates whether the word has been compressed; so we need to match on this also.
			p = next;
			continue;
		}

		/* found matching entry. Decode the phonetic string */
//		word_end = word2;

		dictionary_flags = 0;
		no_phonemes = p[1] & 0x80;

		p += ((p[1] & 0x3f) + 2);

		if(no_phonemes)
		{
			phonetic[0] = 0;
			phoneme_len = 0;
		}
		else
		{
			strcpy(phonetic,p);
			phoneme_len = strlen(p);
			p += (phoneme_len + 1);
		}
		while(p < next)
		{
			// examine the flags which follow the phoneme string

			flag = *p++;
			if(flag >= 100)
			{
				// conditional rule
//				if((dict_condition & (1 << (flag-100))) == 0)
					condition_failed = 1;
			}
			else
			if(flag > 64)
			{
				// stressed syllable information, put in bits 0-3
				dictionary_flags = (dictionary_flags & ~0xf) | (flag & 0xf);
				if((flag & 0xc) == 0xc)
					dictionary_flags |= FLAG_STRESS_END;
			}
			else
			if(flag > 40)
			{
				// flags 41,42,or 43  match more than one word
				// This comes after the other flags
				n_chars = next - p;
				if(memcmp(word2,p,n_chars)==0)
				{
					dictionary_flags |= ((flag-40) << 5);   // set (bits 5-7) to 1,2,or 3
					p = next;
					word_end = word2 + n_chars;
				}
				else
				{
					p = next;
					condition_failed = 1;
					break;
				}
			}
			else
			{
				dictionary_flags |= (1L << flag);
			}
		}

		if(condition_failed)
		{
			condition_failed=0;
			continue;
		}
		return(1);

	}
	return(0);
}   //  end of MCULookupDictList

extern int MCU_n_phoneme_list;
extern MCU_PHONEME_LIST MCU_phoneme_list[MCU_N_PHONEME_LIST];

//Useful ctype functions
char Tolower(char c){
	if((c >='A') && (c <='Z') ) return c |= 0x20;
	return c;
}
int Isalpha(char c){
	if( (c>='A') && c<='z') return 1;
	return 0;
}
int Isspace(char c){
	if( (c>= 0x09) && c<=0x0D) return 1;
	if(c == 0x20) return 1;
	return 0;
}
int Isdigit(char c){
	if( (c>= '0') && c<='9') return 1;
	return 0;
}
volatile char* first_letter;

int MCUTranslate(char* src){
	static char* psrc = NULL;
	char phonemes[30];
	char* p;
	unsigned int flags;
	char word[30];
	char* next;
	unsigned char cap_on;
//	int i;
	int wlen=0;
	int old_ph_list = 0;
	MCU_n_phoneme_list = 0;
	if(NULL != src){
		first_letter = src;
		psrc = src;
	}
	if(psrc == NULL)	return 0;	//done

	while (*psrc != 0){
		word[0]=0;
		if(Isspace(*psrc)){
			psrc++;
		}else if( Isalpha(*psrc) || ( '_' == *psrc )){
			wlen = 0;
			cap_on = ( '_' == *psrc )? 1:0;
			for(p = psrc;(wlen<29) && (0 != *p);wlen++){
				if(Isspace(*p) ) break;
				word[wlen] = (cap_on)? *p++:Tolower(*p++);
			}
			word[wlen]=0;
			next = &word[wlen];

			if(MCULookupDictList(word, next, phonemes, &flags, 0) ){
				old_ph_list = MCU_n_phoneme_list;
				MCU_phoneme_list[MCU_n_phoneme_list].synthflags = 0;
				MCU_phoneme_list[MCU_n_phoneme_list].ph = phonPAUSE;
				
				MCU_phoneme_list[MCU_n_phoneme_list].newword = 1;
				MCU_phoneme_list[MCU_n_phoneme_list].sourceix = 
					(unsigned short) (psrc - first_letter);
				MCU_n_phoneme_list++;
				for(wlen=0;MCU_n_phoneme_list<MCU_N_PHONEME_LIST;wlen++){
					if(phonemes[wlen] == 0) break;
					MCU_phoneme_list[MCU_n_phoneme_list].newword = 0;
					MCU_phoneme_list[MCU_n_phoneme_list].ph = phonemes[wlen];
					MCU_n_phoneme_list++;
				}
			}
			if(MCU_n_phoneme_list < MCU_N_PHONEME_LIST) psrc+=strlen(word);
			else{
				MCU_n_phoneme_list = old_ph_list;	//don't play unterminated words
				return 1;			//not finished to translate
			}
		}else if( Isdigit(*psrc) ){
			//Translate numbers
			word[0] = '_'; word[1] = *psrc;word[2]=0;
			next = &word[2];
			if(MCULookupDictList(word, next, phonemes, &flags, 0) ){
				old_ph_list = MCU_n_phoneme_list;
				MCU_phoneme_list[MCU_n_phoneme_list].ph = phonPAUSE;
				MCU_phoneme_list[MCU_n_phoneme_list].newword = 0;
				MCU_n_phoneme_list++;
				for(wlen=0;MCU_n_phoneme_list<MCU_N_PHONEME_LIST;wlen++){
					if(phonemes[wlen] == 0) break;
					MCU_phoneme_list[MCU_n_phoneme_list].newword = 0;
					MCU_phoneme_list[MCU_n_phoneme_list].ph = phonemes[wlen];
					MCU_n_phoneme_list++;
				}
			}
			if(MCU_n_phoneme_list < MCU_N_PHONEME_LIST) psrc++;
			else{
				MCU_n_phoneme_list = old_ph_list;	//don't play unterminated words
				return 1;			//not finished to translate
			}
		}else{
			//nothing to to
			psrc++;
		}
		
	}
	psrc = NULL;
	return 0;	//done
}

int MCUSpeak(char* src){
//	char* p = src;
	MCUTranslate (src) ;
	while ( ( MCUTranslate (NULL)) != 0);
	return 0;
}

int MCUPhoneticString(char* dest){
	char* p = dest;
	int i;
	*p = 0;
	for(i=0;i<MCU_n_phoneme_list;i++){
		strcat(p,MCUWordToString(MCU_phoneme_tab[MCU_phoneme_list[i].ph].mnemonic));
		p+=strlen(p);
	}
	return 1;
}

void MCUCalcPitches(int clause_tone)
{//==========================================
//  clause_tone: 0=. 1=, 2=?, 3=! 4=none
	MCU_PHONEME_LIST *p;
	int  ix;
	int  x;
	int  st_ix;
	int  tonic_ix=0;
	int  tonic_env = 0;
	int  max_stress=0;
	int  option;
	int  st_ix_changed = -1;
	int  syllable_tab[MCU_N_PHONEME_LIST];
	st_ix=0;
	p = &MCU_phoneme_list[0];
	for(ix=0; ix<MCU_n_phoneme_list; ix++, p++)
	{
		p->type = MCU_phoneme_tab[p->ph].type;
		p->length = MCU_phoneme_tab[p->ph].std_length;
		p->env=0;
		p->pitch1 = 25;
		p->pitch2 = 33;
		if(p->type == phVOWEL){
			p->synthflags |= SFLAG_SYLLABLE;
		}
		if(p->synthflags & SFLAG_SYLLABLE)
		{
			syllable_tab[st_ix] = p->tone;
			st_ix++;
			if(p->tone >= max_stress)
			{
				max_stress = p->tone;
				tonic_ix = ix;
			}
		}
	}

	if(st_ix_changed >= 0)
		syllable_tab[st_ix_changed] = 4;

	if(st_ix == 0)
		return;  // no vowels, nothing to do

	/* transfer vowel data from ph_list to syllable_tab */

//	count_pitch_vowels();

	// unpack pitch data
	st_ix=0;
	p = &MCU_phoneme_list[0];
	for(ix=0; ix<MCU_n_phoneme_list; ix++, p++)
	{
		p->tone = syllable_tab[st_ix] & 0x3f;
		
		if(p->synthflags & SFLAG_SYLLABLE)
		{
			x = ((syllable_tab[st_ix] >> 8) & 0x1ff) - 72;
			if(x < 0) x = 0;
			p->pitch1 = x;

			x = ((syllable_tab[st_ix] >> 17) & 0x1ff) - 72;
			if(x < 0) x = 0;
			p->pitch2 = x;

			p->env = PITCHfall;
			if(syllable_tab[st_ix] & 0x80)
			{
//				if(p->pitch1 > p->pitch2)
//					p->env = PITCHfall;
//				else
					p->env = PITCHrise;
			}

			if(p->pitch1 > p->pitch2)
			{
				// swap so that pitch2 is the higher
				x = p->pitch1;
				p->pitch1 = p->pitch2;
				p->pitch2 = x;
			}
			if(ix==tonic_ix) p->env = tonic_env;
			st_ix++;
		}
	}
}  // end of Translator::CalcPitches


static int speed1 = 170;
static int speed2 = 161;
static int speed3 = 158;

void MCUCalcLengths(void)
{//===========================
	int ix;
	int ix2;
	MCU_PHONEME_LIST *prev;
	MCU_PHONEME_LIST *next;
	MCU_PHONEME_LIST *next2;
	MCU_PHONEME_LIST *next3;
	MCU_PHONEME_LIST *p;
	MCU_PHONEME_LIST *p2;

	int  stress;
	int  type;
	static int  more_syllables=0;
	int  pre_sonorant=0;
	int  pre_voiced=0;
	int  last_pitch = 0;
	int  pitch_start;
	int  length_mod;
	int  len;
	int  env2;
	int  end_of_clause;
	int  embedded_ix = 0;
	int  min_drop;
	unsigned char *pitch_env=NULL;

	for(ix=1; ix<MCU_n_phoneme_list; ix++)
	{
		prev = &MCU_phoneme_list[ix-1];
		p = &MCU_phoneme_list[ix];
		stress = p->tone & 0xf;

		next = &MCU_phoneme_list[ix+1];

		type = p->type;
		if(p->synthflags & SFLAG_SYLLABLE)
			type = phVOWEL;

		switch(type)
		{
		case phPAUSE:
			last_pitch = 0;
			break;
			
		case phSTOP:
			last_pitch = 0;
			if(prev->type == phSTOP || prev->type == phFRICATIVE)
				p->prepause = 15;
			else
			if((more_syllables > 0) || (stress < 4))
				p->prepause = 20;
			else
				p->prepause = 15;

			if( (p->newword))
				p->prepause = 15;

			if(p->synthflags & SFLAG_LENGTHEN)
				p->prepause += 10;
			break;

		case phVFRICATIVE:
			if(next->type==phVOWEL)
			{
				pre_voiced = 1;
			}
		case phFRICATIVE:
			if(p->newword)
				p->prepause = 15;

			if(next->type==phPAUSE && prev->type==phNASAL &&
				!(MCU_phoneme_tab[p->ph].phflags&phFORTIS))
				p->prepause = 25;

			if((MCU_phoneme_tab[p->ph].phflags & phSIBILANT)
				&& next->type==phSTOP && !next->newword)
			{
				if(prev->type == phVOWEL)
					p->length = 150;      // ?? should do this if it's from a prefix
				else
					p->length = 100;
			}
			else
				p->length = 156;

			if((p->newword))
				p->prepause = 20;

			break;

		case phVSTOP:
			if(prev->type==phVFRICATIVE || prev->type==phFRICATIVE ||
				(MCU_phoneme_tab[prev->ph].phflags & phSIBILANT) || (prev->type == phLIQUID))
				p->prepause = 20;

			if(next->type==phVOWEL || next->type==phLIQUID)
			{
				if((next->type==phVOWEL) || !next->newword)
					pre_voiced = 1;

				p->prepause = 30;

				if((prev->type == phPAUSE) || (prev->type == phVOWEL)) // || (prev->ph->mnemonic == ('/'*256+'r')))
					p->prepause = 0;
				else
				if(p->newword==0)
				{
					if(prev->type==phLIQUID)
						p->prepause = 20;
					if(prev->type==phNASAL)
						p->prepause = 12;

					if(prev->type==phSTOP &&
						!(MCU_phoneme_tab[prev->ph].phflags & phFORTIS))
						p->prepause = 0;
				}
			}
			if( (p->newword) && (p->prepause < 20) )
				p->prepause = 20;

			break;

		case phLIQUID:
		case phNASAL:
			p->amp = 20;//stress_amps[1];  // unless changed later
			p->length = 156;  //  TEMPORARY
			min_drop = 0;
			
			if(p->newword)
			{
				if(prev->type==phLIQUID)
					p->prepause = 25;
				if(prev->type==phVOWEL)
					p->prepause = 12;
			}

			if(next->type==phVOWEL)
			{
				pre_sonorant = 1;
			}
			else
			if((prev->type==phVOWEL) || (prev->type == phLIQUID))
			{
				p->length = prev->length;
				p->pitch2 = last_pitch;
				if(p->pitch2 < 7)
					p->pitch2 = 7;
				p->pitch1 = p->pitch2 - 8;
				p->env = PITCHfall;
				pre_voiced = 0;
				
				if(p->type == phLIQUID)
				{
					p->length = speed1;
					p->pitch1 = p->pitch2 - 20;   // post vocalic [r/]
				}

				if(next->type == phVSTOP)
				{
					p->length = (p->length * 160)/100;
				}
				if(next->type == phVFRICATIVE)
				{
					p->length = (p->length * 120)/100;
				}
			}
			else
			{
				p->pitch2 = last_pitch;
				for(ix2=ix; ix2<MCU_n_phoneme_list; ix2++)
				{
					if(MCU_phoneme_list[ix2].type == phVOWEL)
					{
						p->pitch2 = MCU_phoneme_list[ix2].pitch2;
						break;
					}
				}
				p->pitch1 = p->pitch2-8;
				p->env = PITCHfall;
				pre_voiced = 0;
			}
			break;

		case phVOWEL:
			min_drop = 0;
			next2 = &MCU_phoneme_list[ix+2];
			next3 = &MCU_phoneme_list[ix+3];

			if(stress > 7) stress = 7;

			if(pre_sonorant)
				p->amp = 30;//stress_amps[stress]-1;
			else
				p->amp = 20;//stress_amps[stress];

			if(ix >= (MCU_n_phoneme_list-3))
			{
				// last phoneme of a clause, limit its amplitude
				if(p->amp > 20/*langopts.param[LOPT_MAXAMP_EOC]*/)
					p->amp = 20/*langopts.param[LOPT_MAXAMP_EOC]*/;
			}

			// is the last syllable of a word ?
			more_syllables=0;
			end_of_clause = 0;
			for(p2 = p+1; p2->newword== 0; p2++)
			{
				if(p2->type == phVOWEL)
					more_syllables++;
			}
			if((p2->newword & 2) && (more_syllables==0))
			{
				end_of_clause = 2;
			}

			// calc length modifier
			if(more_syllables==0)
			{
				len = 100;//langopts.length_mods0[next2->ph->length_mod *10+ next->ph->length_mod];

				if((next->newword) /*&& (langopts.word_gap & 0x4)*/)
				{
					// consider as a pause + first phoneme of the next word
					length_mod = 150;//(len + langopts.length_mods0[next->ph->length_mod *10+ 1])/2;
				}
				else
					length_mod = len;
			}
			else
			{
				length_mod = 100;//langopts.length_mods[next2->ph->length_mod *10+ next->ph->length_mod];

				if((next->type == phNASAL) && (next2->type == phSTOP ||
					next2->type == phVSTOP) && (MCU_phoneme_tab[next3->ph].phflags & phFORTIS))
					length_mod -= 15;
			}

			if(more_syllables==0)
				length_mod *= speed1;
			else
			if(more_syllables==1)
				length_mod *= speed2;
			else
				length_mod *= speed3;

			length_mod = length_mod / 128;
			if(length_mod < 24)
				length_mod = 24;     // restrict how much lengths can be reduced

			if(stress >= 7)
			{
				// tonic syllable, include a constant component so it doesn't decrease directly with speed
				length_mod += 20;
			}
			
			length_mod = 150;//(length_mod * stress_lengths[stress])/128;

			if(end_of_clause == 2)
			{
				// this is the last syllable in the clause, lengthen it - more for short vowels
				length_mod = length_mod * (256 + (280 - MCU_phoneme_tab[p->ph].std_length)/3)/256;
			}

if(p->type != phVOWEL)
{
	length_mod = 256;   // syllabic consonant
	min_drop = 8;
}
			p->length = length_mod;

			// pre-vocalic part
			// set last-pitch
			env2 = p->env;
			if(env2 > 1) env2++;   // version for use with preceding semi-vowel


			pitch_start = p->pitch1;// + ((p->pitch2-p->pitch1)*pitch_env[0])/256;

			if(pre_sonorant || pre_voiced)
			{
				// set pitch for pre-vocalic part
				if(pitch_start - last_pitch > 9)
					last_pitch = pitch_start - 9;
				prev->pitch1 = last_pitch;
				prev->pitch2 = pitch_start;
				if(last_pitch < pitch_start)
				{
					prev->env = PITCHrise;
					p->env = env2;
				}
				else
				{
					prev->env = PITCHfall;
				}

				prev->length = length_mod;

				prev->amp = p->amp;
				if((prev->type != phLIQUID) && (prev->amp > 18))
					prev->amp = 18;
			}

			// vowel & post-vocalic part
			next->synthflags &= ~SFLAG_SEQCONTINUE;
			if(next->type == phNASAL && next2->type != phVOWEL)
				next->synthflags |= SFLAG_SEQCONTINUE;
				
			if(next->type == phLIQUID)
			{
				next->synthflags |= SFLAG_SEQCONTINUE;
					
				if(next2->type == phVOWEL)
				{
					next->synthflags &= ~SFLAG_SEQCONTINUE;
				}

				if(next2->type != phVOWEL)
				{
					if(MCU_phoneme_tab[next->ph].mnemonic == ('/'*256+'r'))
					{
						next->synthflags &= ~SFLAG_SEQCONTINUE;
//						min_drop = 15;
					}
				}
			}

			if((min_drop > 0) && ((p->pitch2 - p->pitch1) < min_drop))
			{
				p->pitch1 = p->pitch2 - min_drop;
				if(p->pitch1 < 0)
					p->pitch1 = 0;
			}

			last_pitch = p->pitch1 + ((p->pitch2-p->pitch1)*MCU_envelope_data[p->env][127])/256;
			pre_sonorant = 0;
			pre_voiced = 0;
			break;
		}
	}
}  //  end of CalcLengths
