// $Id: mbfm_temperament.h $
/*
 * MBHP_MBFM MIDIbox FM V2.0 OPL3 equal temperament
 *
 * ==========================================================================
 *
 *  Copyright (C) 2014 Sauraen (sauraen@gmail.com)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#include <mios32.h>
#include "mbfm_temperament.h"

//OPL3 equal temperament table; this will be in flash

static const u16 opl3equaltemper[31] = {
    0, 183, 194, 205, 217, 230, 244, 258, 274, 290, 307, 326, 345, 365, 387, 410, 435, 460,
  488, 517, 547, 580, 614, 651, 690, 731, 774, 820, 869, 921, 975
};

static const u16 equaltemperedoctave[12] = {
  //F#4 (54) through F5 (65)
  1850, 1960, 2077, 2200, 2331, 2469, 2616, 2772, 2937, 3111, 3296, 3492
};

static AHB_SECTION u16 opl3customtemper[31];


//C is for Custom
static u8  c_temper;
static u8  c_basenote;
static u16 c_basefreq;

static AHB_SECTION u8  c_numerators[12]; //Stored internally 0 here = 1 to avoid divide by 0
static AHB_SECTION u8  c_denominators[12]; //Stored internally 0 here = 1 to avoid divide by 0
static AHB_SECTION u16 c_frequencies[12];


u8 GetOPL3Block(u8 note){
  if(note >= 103) return 7;
  if(note < 30) return 0;
  return (note - 18) / 12;
}

u16 GetOPL3Frequency(u8 note){
  u8 index;
  if(note >= 115) return 1023;
  if(note == 114){
    index = 30;
  }else if(note >= 30){
    index = ((note - 30) % 12) + 18;
  }else{
    index = note;
  }
  if(c_temper){
    return opl3customtemper[index];
  }else{
    return opl3equaltemper[index];
  }
}

u16 GetOPL3NextFrequency(u8 note){
  u8 index;
  if(note >= 114) return 1023;
  if(note >= 30){
    index = ((note - 30) % 12) + 19;
  }else{
    index = note+1;
  }
  if(c_temper){
    return opl3customtemper[index];
  }else{
    return opl3equaltemper[index];
  }
}

u8 GetOPL3Temperament(){
  return c_temper;
}
extern void SetOPL3Temperament(u8 temper){
  c_temper = !(!temper);
}

extern void Temperament_Init(){
  c_temper = 0;
  c_numerators  [0]  =  0; //C: 1/1
  c_denominators[0]  =  0;
  c_numerators  [1]  = 24; //C#: 25/24
  c_denominators[1]  = 23;
  c_numerators  [2]  =  8; //D: 9/8
  c_denominators[2]  =  7;
  c_numerators  [3]  = 31; //Eb: 32/27
  c_denominators[3]  = 26;
  c_numerators  [4]  =  4; //E: 5/4
  c_denominators[4]  =  3;
  c_numerators  [5]  =  3; //F: 4/3
  c_denominators[5]  =  2;
  c_numerators  [6]  = 63; //F#: 64/45
  c_denominators[6]  = 44;
  c_numerators  [7]  =  2; //G: 3/2
  c_denominators[7]  =  1;
  c_numerators  [8]  = 24; //G#: 25/16
  c_denominators[8]  = 15;
  c_numerators  [9]  =  4; //A: 5/3
  c_denominators[9]  =  2;
  c_numerators  [10] = 15; //Bb: 16/9
  c_denominators[10] =  8;
  c_numerators  [11] = 14; //B: 15/8
  c_denominators[11] =  7;
  SetOPL3BaseNote(60, GetEqualTemperBaseFreq(60));
}

//Calculated at 1/160 Hz resolution
//Note can be 0 (base) through 11
extern void Temperament_Recalc(u8 notenum){
  if(notenum >= 12) return;
  u32 x = c_basefreq;
  u16 write;
  s8 index;
  x <<= 4;
  x *= (u32)c_numerators[notenum] + 1;
  x /= (u32)c_denominators[notenum] + 1;
  c_frequencies[notenum] = x >> 4;
  //Here comes the music
  x <<= 16 - GetOPL3Block(c_basenote+notenum);
  x /= (u32)49716;
  //x is still 10 times too large
  //Main note, 18-29
  index = ((c_basenote + notenum - 30) % 12) + 18;
  write = (x+5)/10;
  if(write > 1023) write = 1023;
  opl3customtemper[index] = write;
  //Extra high note, 30
  index += 12;
  if(index == 30){
    write = (x+2)/5;
    if(write > 1023) write = 1023;
    opl3customtemper[index] = write;
  }
  //One octave down, 6-17
  index -= 24;
  write = (x+10)/20;
  if(write > 1023) write = 1023;
  opl3customtemper[index] = write;
  //Two octaves down, 1-5
  index -= 12;
  if(index > 0){
    write = (x+20)/40;
    if(write > 1023) write = 1023;
    opl3customtemper[index] = write;
  }
}

u8 GetOPL3BaseNote(){
  return c_basenote;
}
u16 GetOPL3BaseFreq(){
  return c_basefreq;
}
void SetOPL3BaseNote(u8 midinote, u16 freq){
  c_basenote = ((midinote-6) % 12) + 54;
  c_basefreq = freq;
  u8 i;
  for(i=0; i<12; i++){
    Temperament_Recalc(i);
  }
}
u16 GetEqualTemperBaseFreq(u8 midinote){
  return equaltemperedoctave[(midinote-6) % 12];
}

u8 GetTemperNumerator(u8 notenum){
  if(notenum >= 12) return 0;
  return c_numerators[notenum];
}
u8 GetTemperDenominator(u8 notenum){
  if(notenum >= 12) return 0;
  return c_denominators[notenum];
}
u16 GetTemperFrequency(u8 notenum){
  if(notenum >= 12) return 0;
  return c_frequencies[notenum];
}
void SetTemperNumerator(u8 notenum, u8 numerator){
  if(notenum >= 12) return;
  c_numerators[notenum] = numerator;
  Temperament_Recalc(notenum);
}
void SetTemperDenominator(u8 notenum, u8 denominator){
  if(notenum >= 12) return;
  c_denominators[notenum] = denominator;
  Temperament_Recalc(notenum);
}
