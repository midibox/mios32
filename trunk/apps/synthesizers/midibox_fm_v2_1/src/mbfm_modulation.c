// $Id: mbfm_modulation.c $
/*
 * MBHP_MBFM MIDIbox FM V2.0 synth engine parameter modulation
 *
 * ==========================================================================
 *
 *  Copyright (C) 2014 Sauraen (sauraen@gmail.com)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

/////////////////////////////////////////////////////////////////////////////
// Include files
/////////////////////////////////////////////////////////////////////////////

#include <mios32.h>
#include "mbfm_modulation.h"

#include <opl3.h>
#include <jsw_rand.h>
#include "mbfm.h"
#include "mbfm_tables.h"
#include "mbfm_temperament.h"


//Global variables
mbfm_opparams_t pre_opparams[36*OPL3_COUNT];
mbfm_voiceparams_t pre_voiceparams[18*OPL3_COUNT];
mbfm_modlparams_t pre_modlparams[18*OPL3_COUNT];
mbfm_voiceparams_t voiceparams[18*OPL3_COUNT];
mbfm_modlparams_t modlparams[18*OPL3_COUNT];
mbfm_modlentry_t modllists[18*OPL3_COUNT][MBFM_MODL_NUMCONN];
u8 modl_lastsrc;
u8 modl_lastdest;
notevel_t delaytape[18*OPL3_COUNT][MBFM_DLY_STEPS];
u8 msecperdelaystep = 1;
u8 delayrechead[18*OPL3_COUNT];
s8 wavetable[18*OPL3_COUNT][MBFM_WT_LEN];
u8 modl_mods[18*OPL3_COUNT];
u8 modl_varis[18*OPL3_COUNT];
s8 pitchbends[18*OPL3_COUNT];
u8 pitchbendrange;
u8 tuningrange;
u8 portastartnote[18*OPL3_COUNT];
u32 portastarttime[18*OPL3_COUNT];
u32 egstarttime[18*OPL3_COUNT];
u8  egmode[18*OPL3_COUNT];
s8  egrelstartval[18*OPL3_COUNT];
u32 lfostarttime[18*OPL3_COUNT];
s8  lforandval[18*OPL3_COUNT][2];
u8  lforandstate[18*OPL3_COUNT];
u32 last_time;
u32 dly_last_time;

//Local variables
u8 modl_been_processed[8];
s8 modl_outputs[8];

typedef union {
  u16 data;
  struct{
    s16 depth:15;
    u8 changed:1;
  };
} dest_deltas_t;

dest_deltas_t dest_deltas[0x90];


/////////////////////////////////////////////////////////////////////////////
// Functions
/////////////////////////////////////////////////////////////////////////////

void MBFM_InitVoiceValues(u8 voice){
  if(voice >= 18*OPL3_COUNT) return;
  u8 i;
  //4-op
  //I know some of these are going to be rejected, but whatever, it's init only
  OPL3_SetFourOp(voice, 0);
  //Op params
  i=voice*2;
  pre_opparams[i].frqLFO = 1;
  pre_opparams[i].ampLFO = 0;
  pre_opparams[i].fmult = 2;
  i++;
  pre_opparams[i].frqLFO = 0;
  pre_opparams[i].ampLFO = 1;
  pre_opparams[i].fmult = 1;
  for(i=voice*2; i<(voice*2)+2; i++){
    pre_opparams[i].ampKSCL = 0;
    pre_opparams[i].rateKSCL = 0;
    pre_opparams[i].dosus = 1;
    pre_opparams[i].mute = 0;
    //pre_opparams[i].unused = 0;
    pre_opparams[i].wave = 0;
    pre_opparams[i].atk = 0;
    pre_opparams[i].dec = 15;
    pre_opparams[i].sus = 15;
    pre_opparams[i].rel = 0;
    pre_opparams[i].vol = 63;
  }
  //Voice
  pre_voiceparams[voice].tp = 0;
  pre_voiceparams[voice].tune = 0;
  pre_voiceparams[voice].porta = 0;
  pre_voiceparams[voice].dlytime = 0;
  pre_voiceparams[voice].feedback = 0;
  pre_voiceparams[voice].alg = 1;
  pre_voiceparams[voice].retrig = 1;
  pre_voiceparams[voice].dlyscale = 0;
  pre_voiceparams[voice].dest = 15;
  voiceparams[voice].data1 = pre_voiceparams[voice].data1;
  voiceparams[voice].data2 = pre_voiceparams[voice].data2;
  voiceparams[voice].data3 = pre_voiceparams[voice].data3;
  //Modl
  pre_modlparams[voice].EGatk = 0;
  pre_modlparams[voice].EGdec1 = 64;
  pre_modlparams[voice].EGlvl = 64;
  pre_modlparams[voice].EGdec2 = 128;
  pre_modlparams[voice].EGsus = 0;
  pre_modlparams[voice].EGrel = 0;
  pre_modlparams[voice].LFOfrq[0] = 128;
  pre_modlparams[voice].LFOfrq[1] = 128;
  pre_modlparams[voice].LFOdly[0] = 0;
  pre_modlparams[voice].LFOdly[1] = 0;
  pre_modlparams[voice].LFOwave[0] = 0;
  pre_modlparams[voice].LFOwave[1] = 0;
  pre_modlparams[voice].WTfrq = 128;
  for(i=0; i<13; i++){
    modlparams[voice].data[i] = pre_modlparams[voice].data[i];
  }
  //Matrix
  for(i=0; i<MBFM_MODL_NUMCONN; i++){
    modllists[voice][i].src = 0;
    modllists[voice][i].dest = 0;
    modllists[voice][i].pre_depth = 0;
  }
}

void MBFM_InitPercValues(u8 voice){
  if(voice >= 18*OPL3_COUNT) return;
  if(voice%18 < 15) return;
  u8 i;
  //Properties common to all percussion voices
  for(i=voice*2; i<(voice*2)+2; i++){
    //Common to all ops
    pre_opparams[i].frqLFO = 0;
    pre_opparams[i].ampLFO = 0;
    pre_opparams[i].ampKSCL = 0;
    pre_opparams[i].rateKSCL = 0;
    pre_opparams[i].dosus = 0;
    pre_opparams[i].mute = 0;
    pre_opparams[i].atk = 0;
  }
  //Voice
  pre_voiceparams[voice].porta = 0;
  pre_voiceparams[voice].dlytime = 0;
  pre_voiceparams[voice].feedback = 0;
  pre_voiceparams[voice].alg = 0;
  pre_voiceparams[voice].retrig = 1;
  pre_voiceparams[voice].dlyscale = 0;
  pre_voiceparams[voice].dest = 3;
  voiceparams[voice].data1 = pre_voiceparams[voice].data1;
  voiceparams[voice].data2 = pre_voiceparams[voice].data2;
  voiceparams[voice].data3 = pre_voiceparams[voice].data3;
  //Modl
  pre_modlparams[voice].EGatk = 0;
  pre_modlparams[voice].EGdec1 = 64;
  pre_modlparams[voice].EGlvl = 64;
  pre_modlparams[voice].EGdec2 = 128;
  pre_modlparams[voice].EGsus = 0;
  pre_modlparams[voice].EGrel = 0;
  pre_modlparams[voice].LFOfrq[0] = 128;
  pre_modlparams[voice].LFOfrq[1] = 128;
  pre_modlparams[voice].LFOdly[0] = 0;
  pre_modlparams[voice].LFOdly[1] = 0;
  pre_modlparams[voice].LFOwave[0] = 0;
  pre_modlparams[voice].LFOwave[1] = 0;
  for(i=0; i<13; i++){
    modlparams[voice].data[i] = pre_modlparams[voice].data[i];
  }
  //Matrix
  for(i=0; i<MBFM_MODL_NUMCONN; i++){
    modllists[voice][i].src = 0;
    modllists[voice][i].dest = 0;
    modllists[voice][i].pre_depth = 0;
  }
  //Customization for preset percussion patch
  i = voice*2;
  switch(voice%18){
  case 15:
    //Bass drum
    pre_opparams[i].fmult = 0;
    pre_opparams[i].wave = 6;
    pre_opparams[i].dec = 7;
    pre_opparams[i].sus = 5;
    pre_opparams[i].rel = 12;
    pre_opparams[i].vol = 55;
    i++;
    pre_opparams[i].fmult = 1;
    pre_opparams[i].wave = 0;
    pre_opparams[i].dec = 9;
    pre_opparams[i].sus = 0;
    pre_opparams[i].rel = 8;
    pre_opparams[i].vol = 57;
    pre_voiceparams[voice].tp = -14;
    pre_voiceparams[voice].tune = 4;
    break;
  case 16:
    //Snare/Hi-Hat
    pre_opparams[i].fmult = 1;
    pre_opparams[i].wave = 0;
    pre_opparams[i].dec = 7;
    pre_opparams[i].sus = 0;
    pre_opparams[i].rel = 5;
    pre_opparams[i].vol = 57;
    i++;
    pre_opparams[i].fmult = 0;
    pre_opparams[i].wave = 0;
    pre_opparams[i].dec = 8;
    pre_opparams[i].sus = 0;
    pre_opparams[i].rel = 8;
    pre_opparams[i].vol = 57;
    pre_voiceparams[voice].tp = -20;
    pre_voiceparams[voice].tune = 11;
    //Open hi-hat
    modllists[voice][0].src = 11;
    modllists[voice][0].dest = 3;
    modllists[voice][0].pre_depth = 3;
    modllists[voice][1].src = 11;
    modllists[voice][1].dest = 6;
    modllists[voice][1].pre_depth = -5;
    break;
  case 17:
    //Tom/Cymbal
    pre_opparams[i].fmult = 5;
    pre_opparams[i].wave = 0;
    pre_opparams[i].dec = 10;
    pre_opparams[i].sus = 0;
    pre_opparams[i].rel = 8;
    pre_opparams[i].vol = 57;
    i++;
    pre_opparams[i].fmult = 15;
    pre_opparams[i].wave = 0;
    pre_opparams[i].dec = 10;
    pre_opparams[i].sus = 0;
    pre_opparams[i].rel = 10;
    pre_opparams[i].vol = 57;
    pre_voiceparams[voice].tp = -52;
    pre_voiceparams[voice].tune = 8;
    //Two-tom-tuning
    modllists[voice][0].src = 11;
    modllists[voice][0].dest = 1;
    modllists[voice][0].pre_depth = 2;
    break;
  default:
    //shouldn't be here
    return;
  }
  
}

void MBFM_ModValuesToOPL3(u8 voice){
  if(voice >= 18*OPL3_COUNT) return;
  u8 i;
  //Op params
  for(i=voice*2; i<(voice*2)+2; i++){
    OPL3_SetVibrato(i, pre_opparams[i].frqLFO);
    OPL3_SetTremelo(i, pre_opparams[i].ampLFO);
    OPL3_SetKSL(i, pre_opparams[i].ampKSCL);
    OPL3_SetKSR(i, pre_opparams[i].rateKSCL);
    OPL3_SetFMult(i, pre_opparams[i].fmult);
    OPL3_SetWaveform(i, pre_opparams[i].wave);
    OPL3_SetAttack(i, pre_opparams[i].atk);
    OPL3_SetDecay(i, pre_opparams[i].dec);
    OPL3_DoSustain(i, pre_opparams[i].dosus);
    OPL3_SetSustain(i, pre_opparams[i].sus);
    OPL3_SetRelease(i, pre_opparams[i].rel);
    OPL3_SetVolume(i, pre_opparams[i].mute ? 0 : pre_opparams[i].vol);
  }
  //Voice
  OPL3_SetAlgorithm(voice, voiceparams[voice].alg);
  OPL3_SetDest(voice, voiceparams[voice].dest);
  OPL3_SetFeedback(voice, voiceparams[voice].feedback);
}

void MBFM_Modulation_Init(){
  //RNG
  jsw_seed(MIOS32_TIMESTAMP_Get());
  //Internal options
  msecperdelaystep = 4;
  tuningrange = 5;
  pitchbendrange = 4;
  //Voices
  u8 i;
  for(i=0; i<18*OPL3_COUNT; i++){
    MBFM_InitVoiceValues(i);
    MBFM_ModValuesToOPL3(i);
  }
}

void StartModulatorsNow(u8 voice, u32 time){
  egmode[voice] = 0;
  egstarttime[voice] = time;
  lfostarttime[voice] = time;
  if(OPL3_IsChannel4Op(voice) == 1){
    egmode[voice+1] = 1;
    lfostarttime[voice+1] = time;
  }
}

void ProcessDelay(u8 voice, u8 steps, u32 time){
  if(voice >= 18*OPL3_COUNT) return;
  //Write to delay line between last record position and this
  s16 head, startpos, endpos, playdelta, initnote, nownote;
  u8 flag = 0, rflag = 0, vel;
  startpos = (delayrechead[voice] % MBFM_DLY_STEPS);
  endpos = (startpos + steps) % MBFM_DLY_STEPS;
  for(head = startpos; head != endpos; head = (head+1) % MBFM_DLY_STEPS){
    if(head >= MBFM_DLY_STEPS){
      DEBUG_MSG("Writing to delay line overran! head=%d", head);
    }
    vel = voicemidistate[voice].velocity;
    delaytape[voice][head].note = vel ? voicemidistate[voice].note : 0;
    delaytape[voice][head].velocity = vel;
    delaytape[voice][head].retrig = voicemidistate[voice].retrig;
  }
  //Write current position
  if(endpos >= MBFM_DLY_STEPS){
    DEBUG_MSG("Writing to delay line overran! endpos=%d", endpos);
  }
  vel = voicemidistate[voice].velocity;
  delaytape[voice][endpos].note = vel ? voicemidistate[voice].note : 0;
  delaytape[voice][endpos].velocity = vel;
  delaytape[voice][endpos].retrig = voicemidistate[voice].retrig;
  //Move recording head
  delayrechead[voice] = endpos;
  //Clear retrig input from MIDI engine
  voicemidistate[voice].retrig = 0;
  //Position play head
  playdelta = voiceparams[voice].dlytime;
  if(playdelta >= MBFM_DLY_STEPS - 1) playdelta = MBFM_DLY_STEPS - 1;
  startpos = startpos - playdelta;
  if(startpos < 0) startpos += MBFM_DLY_STEPS;
  endpos   = endpos   - playdelta;
  if(endpos   < 0) endpos   += MBFM_DLY_STEPS;
  initnote = delaytape[voice][startpos].note;
  for(head = startpos; head != endpos; head = (head+1) % MBFM_DLY_STEPS){
    if(head >= MBFM_DLY_STEPS){
      DEBUG_MSG("Reading from delay line overran! head=%d", head);
    }
    if(delaytape[voice][head].note != initnote){
      flag = 1;
      break;
    }
    if(delaytape[voice][head].retrig){
      rflag = 1;
      break;
    }
  }
  //For current position
  nownote = delaytape[voice][endpos].note;
  if(nownote != initnote) flag = 1;
  if(nownote != voiceactualnotes[voice].note && flag == 0){
    flag = 1;
    initnote = voiceactualnotes[voice].note;
  }
  if(delaytape[voice][endpos].retrig) rflag = 1;
  if(flag || rflag){
    //DEBUG_MSG("t=%d: Delay voice %d changed to %d", time, voice, nownote);
    //Note has changed on delay line since last update
    voiceactualnotes[voice].note = nownote;
    voiceactualnotes[voice].velocity = delaytape[voice][endpos].velocity;
    voiceactualnotes[voice].update = 1;
    //Start portamento if applicable
    if(nownote){
      portastarttime[voice] = time;
      if(initnote){
        //If it went from one non-zero note to another, start from old one
        portastartnote[voice] = initnote;
        //If desired, set retrig
        if(voiceparams[voice].retrig || rflag){
          voiceactualnotes[voice].retrig = 1;
          StartModulatorsNow(voice, time);
        }
      }else{
        //Note just turned on, glide from here to here
        portastartnote[voice] = nownote;
        StartModulatorsNow(voice, time);
      }
    }else if(initnote){
      //Note just turned off, glide from nowhere to nowhere
      portastartnote[voice] = 0;
    }
    //DEBUG_MSG("From ProcessDelay: portastartnote=%d, portastarttime=%d",
    //                 portastartnote[voice], portastarttime[voice]);
  }//else do nothing;
}



//Calculated in 128ths of a half step
void CalcAndSendNote(u8 voice, u32 time){
  //DEBUG_MSG("Refreshing note for voice %d", voice);
  if(voiceactualnotes[voice].retrig){
    OPL3_Gate(voice, 0);
    egmode[voice] = 1;
    egstarttime[voice] = time;
    if(OPL3_IsChannel4Op(voice) == 1){
      egmode[voice+1] = 1;
      egstarttime[voice+1] = time;
    }
    return;
    //Next frame the right note will be sent
  }
  s32 startnote, portanote, destnote = ((s32)voiceactualnotes[voice].note) << 7;
  //DEBUG_MSG("Before porta note is %d.%d", destnote >> 7, destnote & 0x7F);
  s32 deltat, denom;
  u8 finalnote, block, perc = OPL3_IsChannelPerc(voice);
  u16 fhere, fnext, fout;
  if(perc){
    portanote = destnote;
  }else if(portastartnote[voice]){
    startnote = ((s16)portastartnote[voice]) << 7;
    deltat = (s32)((u32)time - (u32)portastarttime[voice]);
    denom = TIME_MAPPING[voiceparams[voice].porta];
    //DEBUG_MSG("Porta: startnote=%d, portastarttime=%d, deltat=%d, denom=%d", startnote, portastarttime[voice], deltat, denom);
    if(deltat >= denom || startnote == destnote){
      //DEBUG_MSG("Porta done");
      portanote = destnote; //All done
      portastartnote[voice] = voiceactualnotes[voice].note; //Mark for not update later
    }else{
      portanote = startnote + (((destnote - startnote) * deltat) / denom);
    }
  }else{
    OPL3_Gate(voice, 0);
    egmode[voice] = 1;
    egstarttime[voice] = time;
    if(OPL3_IsChannel4Op(voice) == 1){
      egmode[voice+1] = 1;
      egstarttime[voice+1] = time;
    }
    return;
  }
  //DEBUG_MSG("After porta note is %d.%d", portanote >> 7, portanote & 0x7F);
  //Add in transpose
  portanote += (s32)voiceparams[voice].tp << 7;
  //Add in tuning
  portanote += (s32)voiceparams[voice].tune * tuningrange;
  //Add in pitch bend
  portanote += (s32)pitchbends[voice] * pitchbendrange;
  //Trim range
  if(portanote < 0) portanote = 0;
  if(portanote >= 127*128) portanote = 127*128;
  //Convert to note and proportion
  finalnote = portanote >> 7;
  portanote &= 127;
  //DEBUG_MSG("V%d final note is %d.%d", voice, finalnote, portanote);
  //Get tuning
  block = GetOPL3Block(finalnote);
  fhere = GetOPL3Frequency(finalnote);
  fnext = GetOPL3NextFrequency(finalnote);
  //Get in-between tuning
  fout = fhere + ((portanote * (s32)(fnext - fhere)) >> 7);
  //Send data
  OPL3_SetFrequency(voice, fout, block);
  if(!perc){
    OPL3_Gate(voice, 1);
  }
}

s8 ProcessModulator(u8 voice, u8 modulator, u32 time){
  s32 deltat, tmap, modt;
  s16 res;
  u8 k = 1;
  if(modulator&1){
    voice++;
    modulator--;
  }
  switch(modulator){
  case 0:
    //EG
    deltat = time - egstarttime[voice];
    if(egmode[voice]){
      //Release
      tmap = TIME_MAPPING[modlparams[voice].EGrel];
      if(deltat >= tmap){
        res = 0; //All done
      }else{
        res = egrelstartval[voice] - (u8)((s32)egrelstartval[voice] * deltat / tmap);
      }
    }else{
      //ADLDS
      tmap = TIME_MAPPING[modlparams[voice].EGatk];
      if(deltat < tmap){
        //Atk
        res = (deltat << 7) / tmap;
      }else{
        deltat -= tmap;
        tmap = TIME_MAPPING[modlparams[voice].EGdec1];
        if(deltat < tmap){
          //Dec1
          res = (s8)((s32)127 + (((s32)modlparams[voice].EGlvl - 127) * deltat / tmap));
        }else{
          deltat -= tmap;
          tmap = TIME_MAPPING[modlparams[voice].EGdec2];
          if(deltat < tmap){
            //Dec2
            res = (s8)((s32)modlparams[voice].EGlvl + (((s32)modlparams[voice].EGsus -
                              (s32)modlparams[voice].EGlvl) * deltat / tmap));
          }else{
            //Sus
            res = modlparams[voice].EGsus;
          }
        }
      }
      egrelstartval[voice] = res;
    }
    if(res > 127) res = 127;
    if(res < -127) res = -127;
    return res;
  case 2:
    k = 0;
  case 4:
    //LFO 1 or 2
    if(modlparams[voice].LFOwave[k] & 128){
      //Free-running
      deltat = time;
    }else{
      deltat = time - lfostarttime[voice];
    }
    tmap = TIME_MAPPING[modlparams[voice].LFOdly[k]];
    if(deltat < tmap){
      return 0;
    }else{
      deltat -= tmap;
      tmap = TIME_MAPPING[255 - modlparams[voice].LFOfrq[k]];
      modt = deltat % tmap;
      lfostarttime[voice] += deltat - modt;
      deltat = modt;
      switch(modlparams[voice].LFOwave[k] & 127){
      case 0:
        //Sine
        res = SINE_TABLE[(deltat << 10) / tmap];
        break;
      case 1:
        //Triangle
        if(deltat < (tmap >> 2)){
          res = (deltat << 9) / tmap;
        }else if(deltat > 3 * (tmap >> 2)){
          res = (s16)((deltat << 9) / tmap) - 512;
        }else{
          res = 256 - (s16)((deltat << 9) / tmap);
        }
        break;
      case 2:
        //Exp-saw
        res = (TIME_MAPPING[(u8)(255 - (s16)((deltat << 8) / tmap))] / 39) - 128;
        break;
      case 3:
        //Saw
        res = (s16)127 - (s16)((deltat << 8) / tmap);
        break;
      case 4:
        //Square
        if(deltat < (tmap >> 1)){
          res = 127;
        }else{
          res = -127;
        }
        break;
      case 5:
        //Pseudo-random
        if(deltat < (tmap >> 1)){
          if(lforandstate[voice] & (1 << k)){
            //Last time we were on the second half, now first, so new random number
            lforandval[voice][k] = (s32)(jsw_rand() & 255) - 128;
            lforandstate[voice] &= ~(1 << k);
          }
        }else{
          lforandstate[voice] |= 1 << k;
        }
        res = lforandval[voice][k];
        break;
      //More?
      default:
        res = 0;
      }
      if(res > 127) res = 127;
      if(res < -127) res = -127;
      return res;
    }
  case 6:
    //WT
    deltat = time - lfostarttime[voice];
    tmap = TIME_MAPPING[255 - modlparams[voice].WTfrq];
    deltat /= tmap;
    deltat %= MBFM_WT_LEN;
    return wavetable[voice][deltat];
  default:
    return 0;
  }
}

void MBFM_Modulation_Tick(u32 time){
  //if((time & 0x000000FF) == 0){
  //  DEBUG_MSG("t=%d: Modulation Tick", time);
  //}
  u8 voice, conn, i, j, class, modl, src, dest, fourop, perc;
  s8 chan;
  s32 res = 0;
  u32 deltat = time - last_time;
  last_time = time;
  u32 dly_deltat = time - dly_last_time;
  u8 delaysteps = (dly_deltat / msecperdelaystep) % MBFM_DLY_STEPS;
  if(delaysteps){
    dly_last_time = time; //This is a dirty solution,
    //the delay will get out of synch with the clock by up to msecperdelaystep every time 
    //deltat > msecperdelaystep
  }
  for(voice=0; voice<18*OPL3_COUNT; voice++){
    fourop = OPL3_IsChannel4Op(voice);
    perc = OPL3_IsChannelPerc(voice);
    if(fourop != 2){ //Don't do anything for second half of fourop
      //Transfer previous frame's retrig to update
      voiceactualnotes[voice].update |= voiceactualnotes[voice].retrig;
      voiceactualnotes[voice].retrig = 0;
      //Delay line to get actual notes and velocities and set update
      if(!perc){
        ProcessDelay(voice, delaysteps, time);
      }
      //Clear list for modulators having been processed
      for(i=0; i<8; i++){
        modl_been_processed[i] = 0;
      }
      //Process modulators
      for(conn=0; conn<MBFM_MODL_NUMCONN; conn++){
        src = modllists[voice][conn].src;
        dest = modllists[voice][conn].dest;
        if(src == 0) break;
        if(dest >= 0x90) break; //error
        src -= 1;
        if(src < 8){
          if(!modl_been_processed[src]){
            res = ProcessModulator(voice, src, time);
            if(res <= -127) res = -127; //Do not extend to -128
            modl_outputs[src] = res;
            modl_been_processed[src] = 1;
          }
          res = modl_outputs[src];
        }else{
          switch(src){
          case 8: 
            res = voiceactualnotes[voice].velocity;
            //if((time & 0xFF) == 1)
            //  DEBUG_MSG("Setting vel modl to %d", res);
            break;
          case 9: res = modl_mods[voice]; break;
          case 10: res = modl_varis[voice]; break;
          default: res = 0;
          }
        }
        dest_deltas[dest].depth += ((s32)(res * (s32)modllists[voice][conn].depth) + 64) >> 7;
        //if((time & 0xFF) == 1)
        //  DEBUG_MSG("Depth dest %d now %d", dest, dest_deltas[dest].depth);
        dest_deltas[dest].changed = 1;
      }
      
      //Combine deltas with parameters and send to OPL3
      for(class=0; class<9; class++){
        i = class << 4;
        for(modl=0; modl<0x10; modl++, i++){
          if(!fourop && (class == 2 || class == 3 || class == 6 || class == 8)){
            modl = 0x10; //Go to next class
          }else if(voiceactualnotes[voice].update || //Update all
                   dest_deltas[i].changed ||
                   (voicemisc[voice].refreshvol && modl == 6)){ //Update only volume
            if(class < 4){
              //Op parameters
              if(modl >= 14){
                modl = 0x10; //Continue with next class
              }else{
                //Op parameter
                j = (voice*2) + class;
                if(modl < 8){
                  res = pre_opparams[j].data[modl];
                  res += dest_deltas[i].depth;
                  if(res < 0) res = 0;
                }
                switch(modl){
                case 0:
                  //Wave
                  if(res > 7) res = 7;
                  OPL3_SetWaveform(j, (u8)res);
                  break;
                case 1:
                  //FMult
                  if(res > 15) res = 15;
                  OPL3_SetFMult(j, (u8)res);
                  break;
                case 2:
                  //Atk
                  if(res > 15) res = 15;
                  OPL3_SetAttack(j, (u8)res);
                  break;
                case 3:
                  //Dec
                  if(res > 15) res = 15;
                  OPL3_SetDecay(j, (u8)res);
                  break;
                case 4:
                  //Sus
                  if(res > 15) res = 15;
                  OPL3_SetSustain(j, (u8)res);
                  break;
                case 5:
                  //Rel
                  if(res > 15) res = 15;
                  OPL3_SetRelease(j, (u8)res);
                  break;
                case 6:
                  //Vol
                  //DEBUG_MSG("Vol wants to be %d", res);
                  if(res > 63) res = 63;
                  //Mute
                  if(pre_opparams[j].mute) res = 0;
                  if(OPL3_IsOperatorCarrier(j)){
                    //Volume and expression
                    chan = midichanmapping[voice];
                    if(chan < 0){
                      //If this voice is not mapped to a channel, look at DUPL and LINK
                      if(voicedupl[voice].voice >= 0){
                        chan = midichanmapping[voicedupl[voice].voice];
                      }else if(voicelink[voice].voice >= 0){
                        chan = midichanmapping[voicelink[voice].voice];
                      }
                    }
                    if(chan >= 0){
                      //If enabled, scale by MIDI volume
                      if(midivol[chan].enable){
                        res *= midivol[chan].level;
                        res >>= 7;
                      }
                      //If enabled, scale by MIDI expression
                      if(midiexpr[chan].enable){
                        res *= midiexpr[chan].level;
                        res >>= 7;
                      }
                    }
                  }
                  //Write volume
                  OPL3_SetVolume(j, (u8)res);
                  break;
                case 7:
                  //Bits
                  //TODO
                  break;
                case 8:
                  //Frq$
                  res = pre_opparams[j].frqLFO;
                  res += dest_deltas[i].depth;
                  if(res < 0) res = 0;
                  if(res > 1) res = 1;
                  OPL3_SetVibrato(j, (u8)res);
                  break;
                case 9:
                  //Amp$
                  res = pre_opparams[j].ampLFO;
                  res += dest_deltas[i].depth;
                  if(res < 0) res = 0;
                  if(res > 1) res = 1;
                  OPL3_SetTremelo(j, (u8)res);
                  break;
                case 10:
                  //KSL
                  res = pre_opparams[j].ampKSCL;
                  res += dest_deltas[i].depth;
                  if(res < 0) res = 0;
                  if(res > 3) res = 3;
                  OPL3_SetKSL(j, (u8)res);
                  break;
                case 11:
                  //KSR
                  res = pre_opparams[j].rateKSCL;
                  res += dest_deltas[i].depth;
                  if(res < 0) res = 0;
                  if(res > 1) res = 1;
                  OPL3_SetKSR(j, (u8)res);
                  break;
                case 12:
                  //DoSus
                  res = pre_opparams[j].dosus;
                  res += dest_deltas[i].depth;
                  if(res < 0) res = 0;
                  if(res > 1) res = 1;
                  OPL3_DoSustain(j, (u8)res);
                  break;
                case 13:
                  //Mute
                  //TODO
                  break;
                //default: do nothing
                }
              }
            }else if(class == 4){
              //Voice parameter
              switch(modl){
              case 0:
                //TP
                res = (s16)(pre_voiceparams[voice].tp) + dest_deltas[i].depth;
                if(res < -128) res = -128;
                if(res > 127) res = 127;
                voiceparams[voice].tp = res;
                voiceactualnotes[voice].update = 1; //Need to refresh all the time
                break;
              case 1:
                //Tune
                res = (s16)(pre_voiceparams[voice].tune) + dest_deltas[i].depth;
                if(res < -128) res = -128;
                if(res > 127) res = 127;
                voiceparams[voice].tune = res;
                voiceactualnotes[voice].update = 1; //Need to refresh all the time
                break;
              case 2:
                //Porta
                res = (s16)(pre_voiceparams[voice].porta) + dest_deltas[i].depth;
                if(res < 0) res = 0;
                if(res > 255) res = 255;
                voiceparams[voice].porta = res;
                break;
              case 3:
                //DlyTime
                res = (s16)(pre_voiceparams[voice].dlytime) + dest_deltas[i].depth;
                if(res < 0) res = 0;
                if(res > 255) res = 255;
                voiceparams[voice].dlytime = res;
                break;
              case 4:
                //Feedback
                res = (s16)(pre_voiceparams[voice].feedback) + dest_deltas[i].depth;
                if(res < 0) res = 0;
                if(res > 7) res = 7;
                voiceparams[voice].feedback = res;
                OPL3_SetFeedback(voice, (u8)res);
                break;
              default:
                modl = 0x10; //Go to next op
              }
            }else if(class <= 6){
              //Modulator parameter
              if(modl >= 0xA){
                modl = 0x10;
              }else{
                j = voice + class - 5;
                res = (s16)(pre_modlparams[j].data[modl]) + dest_deltas[i].depth;
                if(res < 0) res = 0;
                if(res > 255) res = 255;
                if(modl == 2 || modl == 4){
                  if(res > 127) res = 127;
                }
                modlparams[j].data[modl] = (u8)res;
              }
            }else{
              //Modulation depth
              j = voice + class - 7;
              res = modllists[j][modl].pre_depth;
              res += dest_deltas[i].depth;
              if(res < -127) res = -127;
              if(res > 127) res = 127;
              modllists[j][modl].depth = (u8)res;
            }
            //Clear changed status
            dest_deltas[dest].changed = 0;
            dest_deltas[i].depth = 0;
          }
        }
      }
      
      //More things to update
      if(voiceactualnotes[voice].update){
        //Op
        for(i=0; i<(fourop ? 4 : 2); i++){
          res = (2*voice)+i;
          OPL3_SetVibrato(res, pre_opparams[res].frqLFO);
          OPL3_SetTremelo(res, pre_opparams[res].ampLFO);
          OPL3_SetKSL(res, pre_opparams[res].ampKSCL);
          OPL3_SetKSR(res, pre_opparams[res].rateKSCL);
          OPL3_DoSustain(res, pre_opparams[res].dosus);
        }
        //Voice
        OPL3_SetAlgorithm(voice, voiceparams[voice].alg);
        OPL3_SetDest(voice, voiceparams[voice].dest);
        if(fourop){
          OPL3_SetDest(voice+1, voiceparams[voice+1].dest);
        }
      }
      //Portamento active
      if(voiceactualnotes[voice].note != portastartnote[voice] && !perc){
        voiceactualnotes[voice].update = 1;
      }
      //Update note
      if(voiceactualnotes[voice].update){
        CalcAndSendNote(voice, time);
        voiceactualnotes[voice].update = 0;
      }
      voicemisc[voice].refreshvol = 0;
    }
  }
  //Refresh OPL3
  OPL3_OnFrame();
}

u8 MBFM_GetNumModulationConnections(u8 voice){
  u8 conn;
  for(conn=0; conn<MBFM_MODL_NUMCONN; conn++){
    if(!modllists[voice][conn].src) break;
  }
  return conn;
}

s8 MBFM_GetModDepth(u8 voice, u8 src, u8 dest){
  u8 conn, csrc;
  if(dest > 0x90) return 0;
  for(conn=0; conn<MBFM_MODL_NUMCONN; conn++){
    csrc = modllists[voice][conn].src;
    if(!csrc) break;
    if(csrc == src && modllists[voice][conn].dest == dest){
      return modllists[voice][conn].pre_depth;
    }
  }
  return 0;
}

u8 MBFM_FindLastModDest(u8 voice, u8 src){
  u8 conn, csrc, dest=0xFF;
  for(conn=0; conn<MBFM_MODL_NUMCONN; conn++){
    csrc = modllists[voice][conn].src;
    if(!csrc) break;
    if(csrc == src){
      dest = modllists[voice][conn].dest;
    }
  }
  return dest;
}

s8 MBFM_TrySetModulation(u8 voice, u8 src, u8 dest, s8 depth){
  u8 conn, csrc;
  if(depth){
    //Find if it exists
    for(conn=0; conn<MBFM_MODL_NUMCONN; conn++){
      csrc = modllists[voice][conn].src;
      if(!csrc) break;
      if(csrc == src && modllists[voice][conn].dest == dest){
        //Change value
        modllists[voice][conn].pre_depth = depth;
        return 0;
      }
    }
    //If we ran off the top, out of space
    if(conn == MBFM_MODL_NUMCONN) return -1;
    //Otherwise we're where there's space
    modllists[voice][conn].src = src;
    modllists[voice][conn].dest = dest;
    modllists[voice][conn].pre_depth = depth;
    return 1;
  }else{
    //Find it if it exists
    for(conn=0; conn<MBFM_MODL_NUMCONN; conn++){
      csrc = modllists[voice][conn].src;
      if(!csrc) return 0; //Was 0, still is 0
      if(csrc == src && modllists[voice][conn].dest == dest){
        //Remove
        for(; conn<MBFM_MODL_NUMCONN-1; conn++){
          modllists[voice][conn].src = modllists[voice][conn+1].src;
          modllists[voice][conn].dest = modllists[voice][conn+1].dest;
          modllists[voice][conn].pre_depth = modllists[voice][conn+1].pre_depth;
          if(modllists[voice][conn].src == 0) return 1;
        }
        modllists[voice][conn].src = 0;
        return 1;
      }
    }
    return 0;
  }
}

void MBFM_SetVoiceOutputVolume(u8 voice, u8 midivalue){
  if(voice >= 18*OPL3_COUNT) return;
  midivalue >>= 1;
  u8 stop, i;
  switch(OPL3_IsChannel4Op(voice)){
  case 2:
    voice--;
  case 1:
    stop = 4;
    break;
  default:
    stop = 2;
  }
  for(i=2*voice; i<(2*voice)+stop; i++){
    if(OPL3_IsOperatorCarrier(i)){
      pre_opparams[i].vol = midivalue;
    }
  }
  voiceactualnotes[voice].update = 1;
}

static const char* const mbfm_modsourcenames[12] = {
  "None",
  "EG 1",
  "EG 2",
  "LFO 1",
  "LFO 2",
  "LFO 3",
  "LFO 4",
  "WT 1",
  "WT 2",
  "Velocity",
  "Mod",
  "Variation"
};

const char* MBFM_GetModSourceName(u8 source){
  if(source >= 12) return "None";
  return mbfm_modsourcenames[source];
}

static const char* const mbfm_paramnames[0x90] = {
  //0x00
  "OpA Wave",
  "OpA FMult",
  "OpA Atk",
  "OpA Dec",
  "OpA Sus",
  "OpA Rel",
  "OpA Vol",
  "(OpA Bits)",
  "OpA Frq$",
  "OpA Amp$",
  "OpA KSL",
  "OpA KSR",
  "OpA DoSus",
  "(OpA Mute)",
  "(none)",
  "(none)",
  //0x10
  "OpB Wave",
  "OpB FMult",
  "OpB Atk",
  "OpB Dec",
  "OpB Sus",
  "OpB Rel",
  "OpB Vol",
  "(OpB Bits)",
  "OpB Frq$",
  "OpB Amp$",
  "OpB KSL",
  "OpB KSR",
  "OpB DoSus",
  "(OpB Mute)",
  "(none)",
  "(none)",
  //0x20
  "OpC Wave",
  "OpC FMult",
  "OpC Atk",
  "OpC Dec",
  "OpC Sus",
  "OpC Rel",
  "OpC Vol",
  "(OpC Bits)",
  "OpC Frq$",
  "OpC Amp$",
  "OpC KSL",
  "OpC KSR",
  "OpC DoSus",
  "(OpC Mute)",
  "(none)",
  "(none)",
  //0x30
  "OpD Wave",
  "OpD FMult",
  "OpD Atk",
  "OpD Dec",
  "OpD Sus",
  "OpD Rel",
  "OpD Vol",
  "(OpD Bits)",
  "OpD Frq$",
  "OpD Amp$",
  "OpD KSL",
  "OpD KSR",
  "OpD DoSus",
  "(OpD Mute)",
  "(none)",
  "(none)",
  //0x40
  "Transpose",
  "Tune",
  "Portamento",
  "DlyTime",
  "Feedback",
  "Voice Bits",
  "Algorithm",
  "Retrig",
  "DlyScale",
  "Dest",
  "(none)",
  "(none)",
  "(none)",
  "(none)",
  "(none)",
  "(none)",
  //0x50
  "EG1 Atk",
  "EG1 Dec1",
  "EG1 Lvl",
  "EG1 Dec2",
  "EG1 Sus",
  "EG1 Rel",
  "LFO1 Frq",
  "LFO2 Frq",
  "LFO1 Dly",
  "LFO2 Dly",
  "LFO1 Wave",
  "LFO2 Wave",
  "WT1 Frq",
  "(none)",
  "(none)",
  "(none)",
  //0x60
  "EG2 Atk",
  "EG2 Dec1",
  "EG2 Lvl",
  "EG2 Dec2",
  "EG2 Sus",
  "EG2 Rel",
  "LFO3 Frq",
  "LFO4 Frq",
  "LFO3 Dly",
  "LFO4 Dly",
  "LFO3 Wave",
  "LFO4 Wave",
  "WT2 Frq",
  "(none)",
  "(none)",
  "(none)",
  //0x70
  "Mod Conn 1",
  "Mod Conn 2",
  "Mod Conn 3",
  "Mod Conn 4",
  "Mod Conn 5",
  "Mod Conn 6",
  "Mod Conn 7",
  "Mod Conn 8",
  "Mod Conn 9",
  "Mod Conn 10",
  "Mod Conn 11",
  "Mod Conn 12",
  "Mod Conn 13",
  "Mod Conn 14",
  "Mod Conn 15",
  "Mod Conn 16",
  //0x80
  "Mod Conn 17",
  "Mod Conn 18",
  "Mod Conn 19",
  "Mod Conn 20",
  "Mod Conn 21",
  "Mod Conn 22",
  "Mod Conn 23",
  "Mod Conn 24",
  "Mod Conn 25",
  "Mod Conn 26",
  "Mod Conn 27",
  "Mod Conn 28",
  "Mod Conn 29",
  "Mod Conn 30",
  "Mod Conn 31",
  "Mod Conn 32"
};

const char* MBFM_GetParamName(u8 param){
  if(param >= 0x90){
    return "(none)";
  }
  return mbfm_paramnames[param];
}

s16 MBFM_GetParamValue(u8 voice, u8 param){
  if(voice >= 18*OPL3_COUNT) return 0;
  u8 class = param >> 4, sub = param & 15, op;
  if(class > 9) return 0;
  if(!OPL3_IsChannel4Op(voice) && 
      (class == 2 || class == 3 || class == 6 || class == 9)) return 0;
  if(class < 4){
    //Op param
    op = (2*voice)+class;
    switch(sub){
    case 0x8: return pre_opparams[op].frqLFO;
    case 0x9: return pre_opparams[op].ampLFO;
    case 0xA: return pre_opparams[op].ampKSCL;
    case 0xB: return pre_opparams[op].rateKSCL;
    case 0xC: return pre_opparams[op].dosus;
    case 0xD: return pre_opparams[op].mute;
    case 0xE: case 0xF: return 0;
    default:  return pre_opparams[op].data[sub];
    }
  }else if(class >= 7){
    //Modulation depth
    voice += class - 7; //If second half of 4-op
    return modllists[voice][sub].pre_depth;
  }else if(class >= 5){
    //Modulators' parameters
    voice += class - 5; //If second half of 4-op
    if(sub > 0xC) return 0;
    return pre_modlparams[voice].data[sub];
  }else{ //class == 4
    //Voice parameters
    if(sub > 9) return 0;
    switch(sub){
    case 0: return pre_voiceparams[voice].tp;
    case 1: return pre_voiceparams[voice].tune;
    case 6: return pre_voiceparams[voice].alg;
    case 7: return pre_voiceparams[voice].retrig;
    case 8: return pre_voiceparams[voice].dlyscale;
    case 9: return pre_voiceparams[voice].dest;
    default: return pre_voiceparams[voice].data[sub];
    }
  }
}

s16 MBFM_SetParamValue(u8 voice, u8 param, s16 value){
  if(voice >= 18*OPL3_COUNT) return 0;
  u8 class = param >> 4, sub = param & 15, op;
  //DEBUG_MSG("SetParamValue v%d class.sub %d.%d value %d", voice, class, sub, value);
  if(class > 9) return 0;
  if(!OPL3_IsChannel4Op(voice) && 
      (class == 2 || class == 3 || class == 6 || class == 9)) return 0;
  if(class < 4){
    //Op param
    op = (2*voice)+class;
    if(value < 0) value = 0;
    if(sub <= 7){
      switch(sub){
      case 0: //Wave
        if(value > 7) value = 7; break;
      case 6: //Vol
        if(value > 63) value = 63; break;
      case 7: //Bits
        if(value > 0xFF) value = 0xFF; break;
      default: //FMult, ADSR
        if(value > 15) value = 15;
      }
      pre_opparams[op].data[sub] = value;
    }else{
      if(sub == 0xA){
        if(value > 3) value = 3;
      }else{
        value = !(!value);
      }
      switch(sub){
      case 0x8: pre_opparams[op].frqLFO = value;
      case 0x9: pre_opparams[op].ampLFO = value;
      case 0xA: pre_opparams[op].ampKSCL = value;
      case 0xB: pre_opparams[op].rateKSCL = value;
      case 0xC: pre_opparams[op].dosus = value;
      case 0xD: pre_opparams[op].mute = value;
      default:  value = 0;
      }
    }
  }else if(class >= 7){
    //Modulation depth
    voice += class - 7; //If second half of 4-op
    if(value > 0x7F) value = 0x7F;
    if(value < -0x7F) value = -0x7F;
    modllists[voice][sub].pre_depth = value;
  }else if(class >= 5){
    //Modulators' parameters
    voice += class - 5; //If second half of 4-op
    if(sub > 0xC) return 0;
    if(value < 0) value = 0;
    if(sub == 2 || sub == 4){
      if(value > 0x7F) value = 0x7F;
    }else{
      if(value > 0xFF) value = 0xFF;
    }
    pre_modlparams[voice].data[sub] = value;
  }else{ //class == 4
    //Voice parameters
    if(sub <= 5){
      switch(sub){
      case 0:
        if(value > 0x7F) value = 0x7F;
        if(value < -0x7F) value = -0x7F;
        pre_voiceparams[voice].tp = value;
        break;
      case 1:
        if(value > 0x7F) value = 0x7F;
        if(value < -0x7F) value = -0x7F;
        pre_voiceparams[voice].tune = value;
        break;
      default:
        if(value > 0xFF) value = 0xFF;
        if(value < 0) value = 0;
        pre_voiceparams[voice].data[sub] = value;
      }
    }else{
      if(value < 0) value = 0;
      switch(sub){
      case 6: 
        if(value > 3) value = 3;
        pre_voiceparams[voice].alg = value;
        break;
      case 7: 
        value = !(!value);
        pre_voiceparams[voice].retrig = value;
        break;
      case 8:
        value = !(!value);
        pre_voiceparams[voice].dlyscale = value;
        break;
      case 9: 
        if(value > 15) value = 15;
        pre_voiceparams[voice].dest = value;
        break;
      }
    }
  }
  voiceactualnotes[voice].update = 1;
  return value;
}
