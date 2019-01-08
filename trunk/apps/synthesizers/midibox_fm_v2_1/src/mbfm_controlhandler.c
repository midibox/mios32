// $Id: mbfm_controlhandler.c $
/*
 * MBHP_MBFM MIDIbox FM V2.0 synth engine control handler
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
#include "mbfm_controlhandler.h"

#include <opl3.h>
#include <string.h>
#include "mbfm.h"
#include "mbfm_modulation.h"
#include "mbng_event.h"
#include "mbng_lcd.h"
#include "mbfm_sequencer.h"
#include "mbfm_patch.h"
#include "mbfm_temperament.h"
#include "tasks.h"

//Global variables
mbfm_mainmode_t   mainmode;
mbfm_holdmode_t   holdmode;
u8                holdmodeidx;
mbfm_screenmode_t screenmode;
mbfm_screenmode_t screenmodereturn;
u8                screenmodeidx;
u8                screenneedsrefresh;
u8                zeromode;
u8                act_opl3;
u8                act_voice;
u8                act_midich;
u8                act_temper;
u8                act_modconn;
u8                act_port;
u8                ctlrefreshvolume;


static const u8 charset_mbfm_fm[8*8] = {
  //Top half of sine wave
  0b00000100,
  0b00001010,
  0b00010001,
  0b00010001,
  0b00000000,
  0b00000000,
  0b00000000,
  0b00000000,
  //Bottom half of sine wave
  0b00000000,
  0b00000000,
  0b00000000,
  0b00010001,
  0b00010001,
  0b00001010,
  0b00000100,
  0b00000000,
  //Quarter-sine
  0b00000100,
  0b00001100,
  0b00010100,
  0b00010111,
  0b00000000,
  0b00000000,
  0b00000000,
  0b00000000,
  //Half-period sine
  0b00001000,
  0b00010100,
  0b00010100,
  0b00010101,
  0b00000101,
  0b00000101,
  0b00000010,
  0b00000000,
  
  //Half-period abs-sine
  0b00001010,
  0b00001010,
  0b00010101,
  0b00010101,
  0b00000000,
  0b00000000,
  0b00000000,
  0b00000000,
  //Square pulse
  0b00011111,
  0b00010001,
  0b00010001,
  0b00010001,
  0b00000000,
  0b00000000,
  0b00000000,
  0b00000000,
  //Exp-saw
  0b00010000,
  0b00011000,
  0b00010110,
  0b00010001,
  0b00000000,
  0b00000000,
  0b00000000,
  0b00000000,
  //LFO symbol
  0b00010111,
  0b00010100,
  0b00010110,
  0b00011100,
  0b00000010,
  0b00000101,
  0b00000101,
  0b00000010
};




///////////////////////////////////////////////////////////////////////////////
//Helper functions
///////////////////////////////////////////////////////////////////////////////
void MBFM_DrawAlgWidget(u8 enable){
  u8 i, carrier, modlr, op, fourop, unmuted = 1;
  op = act_voice*2;
  fourop = OPL3_IsChannel4Op(act_voice);
  //DEBUG_MSG("DrawAlgWidget for voice %d, enable=%d, fourop=%d", act_voice, enable, fourop);
  //FB light
  MBNG_EVENT_ReceiveMBFM(MBNG_EVENT_MBFM_TYPE_ISALGFB, 0, enable ? pre_voiceparams[act_voice].feedback : 0);
  //Op and connections
  for(i=0; i<4; i++){
    if((fourop || i<2) && enable){
      carrier = OPL3_IsOperatorCarrier(op+i);
      modlr = !carrier;
      unmuted = !pre_opparams[op+i].mute;
    }else{
      modlr = carrier = 0;
    }
    MBNG_EVENT_ReceiveMBFM(MBNG_EVENT_MBFM_TYPE_ALGOPLEDRED, i, modlr && unmuted);
    MBNG_EVENT_ReceiveMBFM(MBNG_EVENT_MBFM_TYPE_ALGOPLEDGREEN, i, carrier && unmuted);
    MBNG_EVENT_ReceiveMBFM(MBNG_EVENT_MBFM_TYPE_ISALGFM, i, modlr);
    MBNG_EVENT_ReceiveMBFM(MBNG_EVENT_MBFM_TYPE_ISALGOUT, i, carrier);
  }
}
///////////////////////////////////////////////////////////////////////////////
void MBFM_DrawVoiceStuff(){
  u8 i;
  //Show channel number
  MBNG_EVENT_ReceiveMBFM(MBNG_EVENT_MBFM_TYPE_DISPVOICE, 0, act_voice+1);
  //Set other lights
  MBFM_DrawAlgWidget(1);
  MBNG_EVENT_ReceiveMBFM(MBNG_EVENT_MBFM_TYPE_ISCURVOICE4OP, 0, OPL3_IsChannel4Op(act_voice));
  MBNG_EVENT_ReceiveMBFM(MBNG_EVENT_MBFM_TYPE_ISRETRIG, 0, voiceparams[act_voice].retrig);
  MBNG_EVENT_ReceiveMBFM(MBNG_EVENT_MBFM_TYPE_ISDLYSCALE, 0, voiceparams[act_voice].dlyscale);
  MBNG_EVENT_ReceiveMBFM(MBNG_EVENT_MBFM_TYPE_ISDUPL, 0, voicedupl[act_voice].voice >= 0);
  MBNG_EVENT_ReceiveMBFM(MBNG_EVENT_MBFM_TYPE_ISLINK, 0, voicelink[act_voice].voice >= 0);
  //4-op indicators for other voices
  for(i=0; i<6; i++){
    MBNG_EVENT_ReceiveMBFM(MBNG_EVENT_MBFM_TYPE_ISSOMEVOICE4OP, i, OPL3_IsChannel4Op((2*i) + (18*act_opl3)));
  }
  //LCD
  MBFM_DrawScreen();
}
///////////////////////////////////////////////////////////////////////////////
void MBFM_ClearVoiceStuff(){
  u8 i;
  //Show channel number
  MBNG_EVENT_ReceiveMBFM(MBNG_EVENT_MBFM_TYPE_CLEARVOICE, 0, 0);
  //Set other lights
  MBFM_DrawAlgWidget(0);
  MBNG_EVENT_ReceiveMBFM(MBNG_EVENT_MBFM_TYPE_ISCURVOICE4OP, 0, 0);
  MBNG_EVENT_ReceiveMBFM(MBNG_EVENT_MBFM_TYPE_ISRETRIG, 0, 0);
  MBNG_EVENT_ReceiveMBFM(MBNG_EVENT_MBFM_TYPE_ISDLYSCALE, 0, 0);
  MBNG_EVENT_ReceiveMBFM(MBNG_EVENT_MBFM_TYPE_ISDUPL, 0, 0);
  MBNG_EVENT_ReceiveMBFM(MBNG_EVENT_MBFM_TYPE_ISLINK, 0, 0);
  //4-op indicators
  for(i=0; i<6; i++){
    MBNG_EVENT_ReceiveMBFM(MBNG_EVENT_MBFM_TYPE_ISSOMEVOICE4OP, i, 0);
  }
}
///////////////////////////////////////////////////////////////////////////////
void MBFM_DrawMIDIChMapping(){
  u8 i, j;
  s8 k;
  //Voice buttons
  j=18*act_opl3;
  for(i=0; i<18; i++, j++){
    k = midichanmapping[j];
    if(k == act_midich){
      MBNG_EVENT_ReceiveMBFM(MBNG_EVENT_MBFM_TYPE_VOICELEDRED, i, 0);
      MBNG_EVENT_ReceiveMBFM(MBNG_EVENT_MBFM_TYPE_VOICELEDGREEN, i, 1);
    }else if(k >= 0){
      MBNG_EVENT_ReceiveMBFM(MBNG_EVENT_MBFM_TYPE_VOICELEDRED, i, 1);
      MBNG_EVENT_ReceiveMBFM(MBNG_EVENT_MBFM_TYPE_VOICELEDGREEN, i, 0);
    }else{
      MBNG_EVENT_ReceiveMBFM(MBNG_EVENT_MBFM_TYPE_VOICELEDRED, i, 0);
      MBNG_EVENT_ReceiveMBFM(MBNG_EVENT_MBFM_TYPE_VOICELEDGREEN, i, 0);
    }
  }
  //4-op indicators for other voices
  for(i=0; i<6; i++){
    MBNG_EVENT_ReceiveMBFM(MBNG_EVENT_MBFM_TYPE_ISSOMEVOICE4OP, i, OPL3_IsChannel4Op((2*i) + (18*act_opl3)));
  }
}
///////////////////////////////////////////////////////////////////////////////
void MBFM_DrawMIDIMain(){
  u8 i;
  for(i=0; i<18; i++){
    MBNG_EVENT_ReceiveMBFM(MBNG_EVENT_MBFM_TYPE_VOICELEDGREEN, i, 
              (i == act_midich));
  }
}
///////////////////////////////////////////////////////////////////////////////
void MBFM_ClearVoiceButtons(){
  u8 i;
  for(i=0; i<18; i++){
    MBNG_EVENT_ReceiveMBFM(MBNG_EVENT_MBFM_TYPE_VOICELEDRED, i, 0);
    MBNG_EVENT_ReceiveMBFM(MBNG_EVENT_MBFM_TYPE_VOICELEDGREEN, i, 0);
  }
}


///////////////////////////////////////////////////////////////////////////////
//Control interface functions
///////////////////////////////////////////////////////////////////////////////
void MBFM_SelectOPL3(u8 value){
  //DEBUG_MSG("MBFM_SelectOPL3 %d", value);
  u8 s, j;
  if(value >= OPL3_COUNT) return;
  act_opl3 = value;
  switch(mainmode){
  case MBFM_MAINMODE_FM:
    switch(holdmode){
    case MBFM_HOLD_NONE:
      MBFM_SelectVoice(act_voice % 18, 1);
      //Draw new voice states
      for(s=0; s<18; s++){
        j = s+(18*act_opl3);
        if(voicemisc[j].activity > 0){
          MBNG_EVENT_ReceiveMBFM(MBNG_EVENT_MBFM_TYPE_VOICELEDRED, s, 1);
          voicemisc[j].activity = 2;
        }else{
          MBNG_EVENT_ReceiveMBFM(MBNG_EVENT_MBFM_TYPE_VOICELEDRED, s, 0);
          voicemisc[j].activity = -1;
        }
      }
      break;
    case MBFM_HOLD_DUPL:
      //Turn on or off DUPL light
      s = voicedupl[act_voice].voice;
      if(s >= 0){
        MBNG_EVENT_ReceiveMBFM(MBNG_EVENT_MBFM_TYPE_VOICELEDRED, s % 18, (s/18 == act_opl3));
      }
      //Turn on or off act_voice light
      MBNG_EVENT_ReceiveMBFM(MBNG_EVENT_MBFM_TYPE_VOICELEDGREEN, act_voice % 18, (act_voice/18 == act_opl3));
      //Redraw 4-op indicators
      for(s=0; s<6; s++){
        MBNG_EVENT_ReceiveMBFM(MBNG_EVENT_MBFM_TYPE_ISSOMEVOICE4OP, s, OPL3_IsChannel4Op((2*s) + (18*act_opl3)));
      }
      break;
    case MBFM_HOLD_LINK:
      //Turn on or off LINK light
      s = voicelink[act_voice].voice;
      if(s >= 0){
        MBNG_EVENT_ReceiveMBFM(MBNG_EVENT_MBFM_TYPE_VOICELEDRED, s % 18, (s/18 == act_opl3));
      }
      //Turn on or off act_voice light
      MBNG_EVENT_ReceiveMBFM(MBNG_EVENT_MBFM_TYPE_VOICELEDGREEN, act_voice % 18, (act_voice/18 == act_opl3));
      //Redraw 4-op indicators
      for(s=0; s<6; s++){
        MBNG_EVENT_ReceiveMBFM(MBNG_EVENT_MBFM_TYPE_ISSOMEVOICE4OP, s, OPL3_IsChannel4Op((2*s) + (18*act_opl3)));
      }
      break;
    }
    break;
  case MBFM_MAINMODE_MIDI:
    switch(holdmode){
    case MBFM_HOLD_SELECT:
      MBFM_DrawMIDIChMapping();
      MBFM_DrawScreen();
      break;
    }
    break;
  }
}
///////////////////////////////////////////////////////////////////////////////
void MBFM_SelectVoice(u8 voice, u8 value){
  //DEBUG_MSG("MBFM_SelectVoice %d, value %d", voice, value);
  if(voice >= 18) return;
  u8 i;
  switch(mainmode){
  case MBFM_MAINMODE_FM:
    switch(holdmode){
    case MBFM_HOLD_NONE:
      if(!value) return;
      i = OPL3_IsChannel4Op(voice + 18*act_opl3);
      if(i == 2){
        voice--; //Direct to lower of 4-op voice
        i--;
      }
      //Turn off old voice light
      MBNG_EVENT_ReceiveMBFM(MBNG_EVENT_MBFM_TYPE_VOICELEDGREEN, act_voice % 18, 0);
      //Set new voice
      act_voice = voice + 18*act_opl3;
      //Turn on new voice light
      MBNG_EVENT_ReceiveMBFM(MBNG_EVENT_MBFM_TYPE_VOICELEDGREEN, act_voice % 18, 1);
      MBFM_DrawVoiceStuff();
      break;
    case MBFM_HOLD_DUPL:
      if(!value) return;
      if(act_voice == voice + 18*act_opl3) return; //can't assign to self
      if(OPL3_IsChannel4Op(voice + 18*act_opl3) == 2){
        voice--; //Direct to lower of 4-op voice
      }
      //Make sure both voices are same number of ops
      if(OPL3_IsChannel4Op(voice + 18*act_opl3) != OPL3_IsChannel4Op(act_voice)){
        MBNG_LCD_CursorSet(0,0,0);
        MBNG_LCD_PrintFormattedString("Not same number of ops!                 ");
        return;
      }
      //Turn on DUPL light
      MBNG_EVENT_ReceiveMBFM(MBNG_EVENT_MBFM_TYPE_ISDUPL, 0, 1);
      //Turn off old voice light
      if(voicedupl[act_voice].voice >= 0){
        MBNG_EVENT_ReceiveMBFM(MBNG_EVENT_MBFM_TYPE_VOICELEDRED, voicedupl[act_voice].voice % 18, 0);
      }
      //Turn on voice light
      MBNG_EVENT_ReceiveMBFM(MBNG_EVENT_MBFM_TYPE_VOICELEDRED, voice, 1);
      //Set value
      voicelink[act_voice].voice = -1; //No dupl+link
      voicedupl[act_voice].voice = voice + 18*act_opl3;
      //Make copy
      MBFM_CopyVoice(voice + 18*act_opl3, act_voice);
      //End
      MBFM_DrawScreen();
      break;
    case MBFM_HOLD_LINK:
      if(!value) return;
      if(act_voice == voice + 18*act_opl3) return; //can't assign to self
      if(OPL3_IsChannel4Op(voice + 18*act_opl3) == 2){
        voice--; //Direct to lower of 4-op voice
      }
      //Make sure both voices are same number of ops
      if(OPL3_IsChannel4Op(voice + 18*act_opl3) != OPL3_IsChannel4Op(act_voice)){
        MBNG_LCD_CursorSet(0,0,0);
        MBNG_LCD_PrintFormattedString("Not same number of ops!                 ");
        return;
      }
      //Turn on LINK light
      MBNG_EVENT_ReceiveMBFM(MBNG_EVENT_MBFM_TYPE_ISLINK, 0, 1);
      //Turn off old voice light
      if(voicelink[act_voice].voice >= 0){
        MBNG_EVENT_ReceiveMBFM(MBNG_EVENT_MBFM_TYPE_VOICELEDRED, voicelink[act_voice].voice % 18, 0);
      }
      //Turn on voice light
      MBNG_EVENT_ReceiveMBFM(MBNG_EVENT_MBFM_TYPE_VOICELEDRED, voice, 1);
      //Set value
      voicedupl[act_voice].voice = -1; //No dupl+link
      voicelink[act_voice].voice = voice + 18*act_opl3;
      //Make copy
      MBFM_CopyVoice(voice + 18*act_opl3, act_voice);
      //End
      MBFM_DrawScreen();
      break;
    }
    MBNG_EVENT_ReceiveMBFM(MBNG_EVENT_MBFM_TYPE_CLEARVALUE, 0, 0);
    break;
  case MBFM_MAINMODE_MIDI:
    switch(holdmode){
    case MBFM_HOLD_SELECT:
      if(!value) return;
      i = voice + 18*act_opl3;
      if(OPL3_IsChannel4Op(i) == 2){
        i--;
      }
      MBFM_AssignVoiceToChannel(i, act_midich);
      MBNG_EVENT_ReceiveMBFM(MBNG_EVENT_MBFM_TYPE_DISPVOICE, 0, i+1);
      MBFM_DrawMIDIChMapping();
      MBFM_DrawScreen();
      break;
    default:
      if(!value) return;
      if(voice >= 16) return;
      act_midich = voice;
      MBFM_DrawMIDIMain();
      MBFM_DrawScreen();
      if(act_midich == 9){
        MBNG_EVENT_ReceiveMBFM(MBNG_EVENT_MBFM_TYPE_DISPVALUE, 0, mididrummapping[seq_seldrum]);
        return;
      }
      break;
    }
    MBNG_EVENT_ReceiveMBFM(MBNG_EVENT_MBFM_TYPE_CLEARVALUE, 0, 0);
    break;
  case MBFM_MAINMODE_SEQ:
    MBFM_SEQ_TrackButton(voice, value);
    break;
  }
}
///////////////////////////////////////////////////////////////////////////////
void MBFM_SelectMode(u8 mode, u8 value){
  //DEBUG_MSG("MBFM_SelectMode %d, value %d", mode, value);
  if(mode > 3) return;
  if(holdmode != MBFM_HOLD_NONE) return;
  if(zeromode) return;
  if(!value) return;
  if(mode == mainmode) return;
  //Turn off mode light
  MBNG_EVENT_ReceiveMBFM(MBNG_EVENT_MBFM_TYPE_ISMODE, mainmode, 0);
  //Set new mode
  mainmode = mode;
  screenmode = MBFM_SCREENMODE_NONE;
  //Turn on new mode light
  MBNG_EVENT_ReceiveMBFM(MBNG_EVENT_MBFM_TYPE_ISMODE, mainmode, 1);
  //Clear stuff
  MBFM_ClearVoiceButtons();
  MBFM_ClearVoiceStuff();
  MBNG_EVENT_ReceiveMBFM(MBNG_EVENT_MBFM_TYPE_CLEARVALUE, 0, 0);
  //LCD
  MBFM_SetUpLCDChars();
  MBFM_DrawScreen();
  //Mode-specific
  if(mainmode == MBFM_MAINMODE_FM){
    //Draw everything as if voice just changed
    MBFM_SelectVoice(act_voice % 18, 1);
  }else if(mainmode == MBFM_MAINMODE_MIDI){
    //Draw selection of MIDI channels
    MBFM_DrawMIDIMain();
  }else if(mainmode == MBFM_MAINMODE_SEQ){
    screenmodeidx = 0;
    MBFM_SEQ_RedrawAll();
  }
}
///////////////////////////////////////////////////////////////////////////////
void MBFM_BtnSoftkey(u8 softkey, u8 value){
  //DEBUG_MSG("MBFM_BtnSoftkey %d, value %d", softkey, value);
  if(softkey > 8) return;
  if(softkey == 0){
    DEBUG_MSG("Config ERROR: Softkeys are indexed 1-8, there is no 0!");
    return;
  }
  s8 s; u8 voice; char c;
  switch(mainmode){
  case MBFM_MAINMODE_FM:
    switch(holdmode){
    case MBFM_HOLD_DUPL:
      if(!value) return;
      switch(softkey){
      case 1:
      case 2:
        //Turn off DUPL light
        MBNG_EVENT_ReceiveMBFM(MBNG_EVENT_MBFM_TYPE_ISDUPL, 0, 0);
        //Turn off voice light, if applicable
        s = voicedupl[act_voice].voice;
        if(s >= 0 && s/18 == act_opl3){
          MBNG_EVENT_ReceiveMBFM(MBNG_EVENT_MBFM_TYPE_VOICELEDRED, s % 18, 0);
        }
        //Set to -1
        voicedupl[act_voice].voice = -1;
        MBFM_DrawScreen();
        break;
      case 3:
      case 4:
        //Toggle follow
        voicedupl[act_voice].follow = !voicedupl[act_voice].follow;
        MBFM_DrawScreen();
        break;
      }
      break;
    case MBFM_HOLD_LINK:
      if(!value) return;
      switch(softkey){
      case 1:
      case 2:
        //Turn off LINK light
        MBNG_EVENT_ReceiveMBFM(MBNG_EVENT_MBFM_TYPE_ISLINK, 0, 0);
        //Turn off voice light, if applicable
        s = voicelink[act_voice].voice;
        if(s >= 0 && s/18 == act_opl3){
          MBNG_EVENT_ReceiveMBFM(MBNG_EVENT_MBFM_TYPE_VOICELEDRED, s % 18, 0);
        }
        //Set to -1
        voicelink[act_voice].voice = -1;
        MBFM_DrawScreen();
        break;
      case 3:
      case 4:
        //Toggle follow
        voicelink[act_voice].follow = !voicelink[act_voice].follow;
        MBFM_DrawScreen();
        break;
      }
      break;
    case MBFM_HOLD_ALG:
      if(value){
        s = (softkey - 1) / 2;
        if(!OPL3_IsChannel4Op(act_voice) && s >= 2) return;
        //Set algorithm
        for(voice=0; voice<18*OPL3_COUNT; voice++){
          if(voicedupl[voice].voice == act_voice || voice == act_voice){
            voiceparams[voice].alg = s;
            //We have to write to OPL3 register here rather than waiting for update
            //because MBFM_DrawAlgWidget() uses OPL3_IsOperatorCarrier() to draw widget
            //and it won't be updated properly otherwise
            OPL3_SetAlgorithm(voice, s);
            voiceactualnotes[voice].update = 1;
          }
        }
        //Draw widget and display
        MBFM_DrawAlgWidget(1);
        MBFM_DrawScreen();
      }
      break;
    case MBFM_HOLD_DEST:
      if(value && softkey >= 5){
        softkey -= 5;
        s = voiceparams[act_voice+holdmodeidx].dest; //This is correct
        if((s >> softkey) & 1){
          s &= ~(1 << softkey);
        }else{
          s |= (1 << softkey);
        }
        for(voice=0; voice<18*OPL3_COUNT; voice++){
          if(voicedupl[voice].voice == act_voice || voice == act_voice){
            voiceparams[voice+holdmodeidx].dest = s;
            voiceactualnotes[voice].update = 1;
          }
        }
        MBFM_DrawScreen();
      }
      break;
    case MBFM_HOLD_OPPARAM:
      if(!value) return;
      for(voice=0; voice<18*OPL3_COUNT; voice++){
        if(voicedupl[voice].voice == act_voice || voice == act_voice){
          s = (voice*2)+holdmodeidx;
          //DEBUG_MSG("Setting OPPARAM for op %d", s);
          switch(softkey){
          case 1:
            pre_opparams[s].frqLFO = !pre_opparams[s].frqLFO;
            break;
          case 2:
            pre_opparams[s].ampLFO = !pre_opparams[s].ampLFO;
            break;
          case 3:
            if(pre_opparams[s].ampKSCL == 3){
              pre_opparams[s].ampKSCL = 0;
            }else{
              pre_opparams[s].ampKSCL++;
            }
            break;
          case 4:
            pre_opparams[s].rateKSCL = !pre_opparams[s].rateKSCL;
            break;
          case 5:
            pre_opparams[s].dosus = !pre_opparams[s].dosus;
            break;
          default:
            return;
          }
          voiceactualnotes[voice].update = 1;
        }
      }
      MBFM_DrawScreen();
      break;
    case MBFM_HOLD_WAVE:
      if(!value) return;
      softkey--;
      for(voice=0; voice<18*OPL3_COUNT; voice++){
        if(voicedupl[voice].voice == act_voice || voice == act_voice){
          s = (voice*2)+holdmodeidx;
          //DEBUG_MSG("Setting WAVE for op %d", s);
          pre_opparams[s].wave = softkey;
          voiceactualnotes[voice].update = 1;
        }
      }
      MBFM_DrawScreen();
      break;
    case MBFM_HOLD_LFOWAVE:
      if(!value) return;
      softkey--;
      for(voice=0; voice<18*OPL3_COUNT; voice++){
        if(voicedupl[voice].voice == act_voice || voice == act_voice){
          if(softkey == 7){
            modlparams[voice+(holdmodeidx>>1)].LFOwave[holdmodeidx&1] ^= 128;
          }else if(softkey <= 5){
            modlparams[voice+(holdmodeidx>>1)].LFOwave[holdmodeidx&1] =
                (modlparams[voice+(holdmodeidx>>1)].LFOwave[holdmodeidx&1] & 128) + softkey;
          }
        }
      }
      MBFM_DrawScreen();
      break;
    case MBFM_HOLD_ASGN:
      if(softkey == 8){
        zeromode = value;
      }
      break;
    case MBFM_HOLD_NONE:
      switch(screenmode){
      case MBFM_SCREENMODE_NONE:
        switch(softkey){
        case 1:
          if(!value) return;
          screenmode = MBFM_SCREENMODE_FM_MOD;
          screenmodeidx = 0;
          MBFM_DrawScreen();
          break;
        case 5:
          if(!value) return;
          screenmode = MBFM_SCREENMODE_FM_TEMPER;
          screenmodeidx = 0;
          act_temper = 0;
          MBFM_DrawScreen();
          break;
        case 6:
          if(!value) return;
          screenmode = MBFM_SCREENMODE_FM_OPT;
          screenmodeidx = 0;
          MBFM_DrawScreen();
          break;
        case 7:
          if(!value) return;
          screenmode = MBFM_SCREENMODE_FM_OPL3;
          MBFM_DrawScreen();
          break;
        case 8:
          zeromode = value;
          break;
        }
        break;
      case MBFM_SCREENMODE_FM_MOD:
        if(!value) return;
        if(softkey == 1){
          if(act_modconn > 0) act_modconn--;
        }else if(softkey == 2){
          if(act_modconn < (OPL3_IsChannel4Op(act_voice) ? 31 : 15)) act_modconn++;
        }else if(softkey <= 4){
          screenmodeidx = 0;
        }else if(softkey <= 6){
          screenmodeidx = 1;
        }else{
          screenmodeidx = 2;
        }
        MBFM_DrawScreen();
        break;
      case MBFM_SCREENMODE_FM_TEMPER:
        if(!value) return;
        if(softkey == 1){
          SetOPL3Temperament(!GetOPL3Temperament());
          screenmodeidx = 0;
        }else if(softkey == 2){
          screenmodeidx = 0;
        }else if(softkey <= 7){
          screenmodeidx = softkey-3;
        }else{
          return;
        }
        MBFM_DrawScreen();
        break;
      case MBFM_SCREENMODE_FM_OPT:
        if(!value) return;
        if(softkey > 3) return;
        screenmodeidx = softkey - 1;
        MBFM_DrawScreen();
        break;
      case MBFM_SCREENMODE_FM_OPL3:
        if(!value) return;
        switch(softkey){
        case 1:
          s = opl3_chip[0].vibratodepth;
          s = !s; //I REALLY want C to allow "s !=;" and "s ~=;"!
          OPL3_SetVibratoDepth(0, s);
          #if OPL3_COUNT
          OPL3_SetVibratoDepth(1, s);
          #endif
          break;
        case 2:
          s = opl3_chip[0].tremelodepth;
          s = !s;
          OPL3_SetTremeloDepth(0, s);
          #if OPL3_COUNT
          OPL3_SetTremeloDepth(1, s);
          #endif
          break;
        case 5:
          s = opl3_chip[0].percussion;
          s = !s;
          OPL3_SetPercussionMode(0, s);
          if(s){
            MBFM_Drum_InitPercussionMode(0);
          }else{
            voiceactualnotes[15].note = 0;
            voiceactualnotes[16].note = 0;
            voiceactualnotes[17].note = 0;
          }
          break;
        case 6:
          #if OPL3_COUNT
          s = opl3_chip[1].percussion;
          s = !s;
          OPL3_SetPercussionMode(1, s);
          if(s){
            MBFM_Drum_InitPercussionMode(1);
          }else{
            voiceactualnotes[33].note = 0;
            voiceactualnotes[34].note = 0;
            voiceactualnotes[35].note = 0;
          }
          #endif
          break;
        }
        MBFM_DrawScreen();
        break;
      }
    }
    break;
  case MBFM_MAINMODE_MIDI:
    switch(holdmode){
    case MBFM_HOLD_SELECT:
      if(!value) return;
      switch(softkey){
      case 1:
        //Clear
        for(voice=0; voice<18*OPL3_COUNT; voice++){
          if(midichanmapping[voice] == act_midich){
            midichanmapping[voice] = -1;
            voicemidistate[voice].note = 0;
            voicemidistate[voice].velocity = 0;
            voicemidistate[voice].update = 0;
            if(chanoptions[act_midich].autodupl){
              voicelink[voice].voice = -1;
              voicedupl[voice].voice = -1;
            }
          }
        }
        midifirstvoice[act_midich] = -1;
        MBNG_EVENT_ReceiveMBFM(MBNG_EVENT_MBFM_TYPE_CLEARVOICE, 0, 0);
        MBFM_DrawMIDIChMapping();
        MBFM_DrawScreen();
        break;
      case 3:
      case 4:
        //Auto-dupl
        chanoptions[act_midich].autodupl = !chanoptions[act_midich].autodupl;
        MBFM_DrawScreen();
        break;
      case 5:
      case 6:
        MBFM_AllNotesOff(act_midich);
        break;
      }
      break;
    case MBFM_HOLD_NONE:
      switch(screenmode){
      case MBFM_SCREENMODE_NONE:
        if(!value) return;
        switch(softkey){
        case 1:
          screenmode = MBFM_SCREENMODE_MIDI_LOAD;
          screenmodeidx = 2;
          patchloadquery = 1;
          if(act_midich == 9){
            prgstate[9].drum = ((seq_seldrum / 6) << 1) + ((seq_seldrum % 6) > 0);
            //DEBUG_MSG("Setting up to load drum %d", prgstate[9].drum);
          }
          break;
        case 2:
          screenmode = MBFM_SCREENMODE_MIDI_SAVE;
          screenmodeidx = 2;
          patchloadquery = 1;
          if(act_midich == 9){
            prgstate[9].drum = ((seq_seldrum / 6) << 1) + ((seq_seldrum % 6) > 0);
            //DEBUG_MSG("Setting up to save drum %d", prgstate[9].drum);
          }
          break;
        case 3:
          screenmode = MBFM_SCREENMODE_MIDI_AUTO;
          screenmodeidx = 0;
          break;
        case 4:
          screenmode = MBFM_SCREENMODE_MIDI_MIXER;
          screenmodeidx = 0;
          break;
        case 5:
          screenmode = MBFM_SCREENMODE_MIDI_IN;
          screenmodeidx = 0;
          break;
        case 6:
          screenmode = MBFM_SCREENMODE_MIDI_OUT;
          screenmodeidx = 0;
          break;
        default:
          return;
        }
        MBFM_DrawScreen();
        break;
      case MBFM_SCREENMODE_MIDI_LOAD:
        if(!value) return;
        switch(softkey){
        case 5:
          screenmodeidx = 0;
          break;
        case 6:
          screenmodeidx = 1;
          break;
        case 7:
          screenmodeidx = 2;
          break;
        case 8:
          //Set to load on next frame
          if(!prgstate[act_midich].command){
            prgstate[act_midich].command = 1;
          }
          break;
        default:
          return;
        }
        MBFM_DrawScreen();
        break;
      case MBFM_SCREENMODE_MIDI_SAVE:
        if(!value) return;
        switch(softkey){
        case 4:
          screenmodereturn = screenmode;
          screenmode = MBFM_SCREENMODE_MIDI_NAME;
          screenmodeidx = 0;
          break;
        case 5:
          screenmodeidx = 0;
          break;
        case 6:
          screenmodeidx = 1;
          break;
        case 7:
          screenmodeidx = 2;
          break;
        case 8:
          //Set to save on next frame
          if(!prgstate[act_midich].command){
            prgstate[act_midich].command = 2;
          }
          break;
        default:
          return;
        }
        MBFM_DrawScreen();
        break;
      case MBFM_SCREENMODE_MIDI_NAME:
        if(!value) return;
        char *nameptr;
        if(act_midich == 9){
          nameptr = drumnames[prgstate[9].drum];
        }else{
          nameptr = patchnames[act_midich];
        }
        switch(softkey){
        case 4:
          //Case
          c = nameptr[screenmodeidx];
          if(c >= 'a' && c <= 'z'){
            c -= 'a' - 'A';
          }else if(c >= 'A' && c <= 'Z'){
            c += 'a' - 'A';
          }else{
            c = 'M';
          }
          nameptr[screenmodeidx] = c;
          break;
        case 5:
          //Backspace
          for(s=screenmodeidx; s<MBFM_PATCHNAME_LEN-1; s++){
            nameptr[s] = nameptr[s+1];
          }
          break;
        case 6:
          //Cursor left
          if(screenmodeidx){
            if(nameptr[screenmodeidx+1] == 0 &&
                  nameptr[screenmodeidx] == ' '){
              nameptr[screenmodeidx] = 0;
            }
            screenmodeidx--;
          }
          break;
        case 7:
          //Cursor right
          if(screenmodeidx < MBFM_PATCHNAME_LEN-2){
            screenmodeidx++;
            if(nameptr[screenmodeidx] == 0){
              nameptr[screenmodeidx] = 
                    nameptr[screenmodeidx-1];
            }
          }
          break;
        case 8:
          //OK
          screenmode = screenmodereturn;
          screenmodeidx = 2;
          break;
        default:
          return;
        }
        MBFM_DrawScreen();
        break;
      case MBFM_SCREENMODE_MIDI_AUTO:
        if(!value) return;
        u8 b;
        switch(softkey){
        case 1: //Chan vs. All
          screenmodeidx = !screenmodeidx;
          break;
        case 2:
          b = !chanoptions[act_midich].autoload;
          if(screenmodeidx){
            for(s=0; s<16; s++){
              chanoptions[s].autoload = b;
            }
            screenmodeidx = 0;
          }else{
            chanoptions[act_midich].autoload = b;
          }
          break;
        case 3:
          b = !chanoptions[act_midich].cc_bmsb;
          if(screenmodeidx){
            for(s=0; s<16; s++){
              chanoptions[s].cc_bmsb = b;
            }
            screenmodeidx = 0;
          }else{
            chanoptions[act_midich].cc_bmsb = b;
          }
          break;
        case 4:
          b = !chanoptions[act_midich].cc_blsb;
          if(screenmodeidx){
            for(s=0; s<16; s++){
              chanoptions[s].cc_blsb = b;
            }
            screenmodeidx = 0;
          }else{
            chanoptions[act_midich].cc_blsb = b;
          }
          break;
        case 5:
          b = !chanoptions[act_midich].cc_prog;
          if(screenmodeidx){
            for(s=0; s<16; s++){
              chanoptions[s].cc_prog = b;
            }
            screenmodeidx = 0;
          }else{
            chanoptions[act_midich].cc_prog = b;
          }
          break;
        case 6:
          b = !chanoptions[act_midich].cc_pan;
          if(screenmodeidx){
            for(s=0; s<16; s++){
              chanoptions[s].cc_pan = b;
            }
            screenmodeidx = 0;
          }else{
            chanoptions[act_midich].cc_pan = b;
          }
          break;
        case 7:
          b = !chanoptions[act_midich].cc_other;
          if(screenmodeidx){
            for(s=0; s<16; s++){
              chanoptions[s].cc_other = b;
            }
            screenmodeidx = 0;
          }else{
            chanoptions[act_midich].cc_other = b;
          }
          break;
        default:
          return;
        }
        MBFM_DrawScreen();
        break;
      case MBFM_SCREENMODE_MIDI_MIXER:
        if(!value) return;
        switch(softkey){
        case 1:
          screenmodeidx = !screenmodeidx;
          break;
        case 2:
          if(screenmodeidx){
            midiexpr[act_midich].enable = !midiexpr[act_midich].enable;
          }else{
            midivol[act_midich].enable = !midivol[act_midich].enable;
          }
          break;
        default:
          return;
        }
        MBFM_DrawScreen();
        break;
      case MBFM_SCREENMODE_MIDI_IN:
        if(!value) return;
        switch(softkey){
        case 1:
          if(act_port > 0) act_port--;
          break;
        case 2:
          if(act_port < 5) act_port++;
          break;
        case 5:
          s = midiportinopts[act_port].mode;
          if(s == 3) s = 0; else s++;
          midiportinopts[act_port].mode = s;
          break;
        default:
          return;
        }
        MBFM_DrawScreen();
        break;
      }
      break;
    }
    break;
  case MBFM_MAINMODE_SEQ:
    MBFM_SEQ_Softkey(softkey, value);
    break;
  }
}
///////////////////////////////////////////////////////////////////////////////
void MBFM_BtnSelect(u8 value){
  //DEBUG_MSG("MBFM_BtnSelect %d", value);
  if(zeromode) return;
  if(holdmode == MBFM_HOLD_SELECT && !value){
    holdmode = MBFM_HOLD_NONE;
    switch(mainmode){
    case MBFM_MAINMODE_MIDI:
      MBFM_ClearVoiceButtons();
      MBFM_ClearVoiceStuff(); //for 4-op indicators
      MBNG_EVENT_ReceiveMBFM(MBNG_EVENT_MBFM_TYPE_CLEARVOICE, 0, 0);
      MBFM_DrawMIDIMain();
      MBFM_DrawScreen();
      break;
    }
  }else if(holdmode == MBFM_HOLD_NONE && value){
    holdmode = MBFM_HOLD_SELECT;
    switch(mainmode){
    case MBFM_MAINMODE_MIDI:
      if(act_midich == 9){
        //Drum channel, yell at user
        MBNG_LCD_CursorSet(0,0,0);
        MBNG_LCD_PrintFormattedString("No, use Datawheel to change MIDI note!  ");
        holdmode = MBFM_HOLD_NONE;
        return;
      }
      MBFM_DrawMIDIChMapping();
      MBFM_DrawScreen();
      break;
    }
  }//else return?
}
///////////////////////////////////////////////////////////////////////////////
void MBFM_BtnMenu(u8 value){
  //DEBUG_MSG("MBFM_BtnMenu %d", value);
  if(!value) return;
  if(screenmode == MBFM_SCREENMODE_NONE) return;
  screenmode = MBFM_SCREENMODE_NONE;
  MBFM_DrawScreen();
}
///////////////////////////////////////////////////////////////////////////////
void MBFM_BtnDupl(u8 value){
  //DEBUG_MSG("MBFM_BtnDupl %d", value);
  s8 voice; u8 i;
  if(mainmode != MBFM_MAINMODE_FM) return;
  if(zeromode) return;
  if(holdmode == MBFM_HOLD_DUPL && !value){
    holdmode = MBFM_HOLD_NONE;
    voice = voicedupl[act_voice].voice;
    //Turn off red light
    if(voice >= 0) MBNG_EVENT_ReceiveMBFM(MBNG_EVENT_MBFM_TYPE_VOICELEDRED, voice % 18, 0);
    //Fake select voice
    MBFM_SelectVoice((voice >= 0) ? (voice % 18) : (act_voice % 18), 1);
  }else if(holdmode == MBFM_HOLD_NONE && value){
    holdmode = MBFM_HOLD_DUPL;
    MBFM_ClearVoiceStuff();
    MBNG_EVENT_ReceiveMBFM(MBNG_EVENT_MBFM_TYPE_ISDUPL, 0, 1);
    MBFM_ClearVoiceButtons();
    MBNG_EVENT_ReceiveMBFM(MBNG_EVENT_MBFM_TYPE_VOICELEDGREEN, act_voice % 18, 1);
    voice = voicedupl[act_voice].voice;
    if(voice >= 0 && voice/18 == act_opl3){
      //Draw red light for voice
      MBNG_EVENT_ReceiveMBFM(MBNG_EVENT_MBFM_TYPE_VOICELEDRED, voice % 18, 1);
    }
    //4-op indicators for other voices
    for(i=0; i<6; i++){
      MBNG_EVENT_ReceiveMBFM(MBNG_EVENT_MBFM_TYPE_ISSOMEVOICE4OP, i, OPL3_IsChannel4Op((2*i) + (18*act_opl3)));
    }
    MBFM_DrawScreen();
  }
}
///////////////////////////////////////////////////////////////////////////////
void MBFM_BtnLink(u8 value){
  //DEBUG_MSG("MBFM_BtnLink %d", value);
  s8 voice; u8 i;
  if(mainmode != MBFM_MAINMODE_FM) return;
  if(zeromode) return;
  if(holdmode == MBFM_HOLD_LINK && !value){
    holdmode = MBFM_HOLD_NONE;
    voice = voicelink[act_voice].voice;
    //Turn off red light
    if(voice >= 0) MBNG_EVENT_ReceiveMBFM(MBNG_EVENT_MBFM_TYPE_VOICELEDRED, voice % 18, 0);
    //Fake select voice
    MBFM_SelectVoice((voice >= 0) ? (voice % 18) : (act_voice % 18), 1);
  }else if(holdmode == MBFM_HOLD_NONE && value){
    holdmode = MBFM_HOLD_LINK;
    MBFM_ClearVoiceStuff();
    MBNG_EVENT_ReceiveMBFM(MBNG_EVENT_MBFM_TYPE_ISLINK, 0, 1);
    MBFM_ClearVoiceButtons();
    MBNG_EVENT_ReceiveMBFM(MBNG_EVENT_MBFM_TYPE_VOICELEDGREEN, act_voice % 18, 1);
    voice = voicelink[act_voice].voice;
    if(voice >= 0 && voice/18 == act_opl3){
      //Draw red light for voice
      MBNG_EVENT_ReceiveMBFM(MBNG_EVENT_MBFM_TYPE_VOICELEDRED, voice % 18, 1);
    }
    //4-op indicators for other voices
    for(i=0; i<6; i++){
      MBNG_EVENT_ReceiveMBFM(MBNG_EVENT_MBFM_TYPE_ISSOMEVOICE4OP, i, OPL3_IsChannel4Op((2*i) + (18*act_opl3)));
    }
    MBFM_DrawScreen();
  }
}
///////////////////////////////////////////////////////////////////////////////
void MBFM_BtnRetrig(u8 value){
  //DEBUG_MSG("MBFM_BtnRetrig %d", value);
  if(mainmode != MBFM_MAINMODE_FM) return;
  if(holdmode != MBFM_HOLD_NONE) return;
  if(zeromode) return;
  if(!value) return;
  if(voicedupl[act_voice].voice >= 0){
    MBNG_LCD_CursorSet(0,0,0);
    MBNG_LCD_PrintFormattedString("Dupl'd to V%d, can't edit!", voicedupl[act_voice].voice+1);
    return;
  }
  u8 state = voiceparams[act_voice].retrig;
  state = !state;
  u8 voice;
  for(voice=0; voice<18*OPL3_COUNT; voice++){
    if(voicedupl[voice].voice == act_voice || voice == act_voice){
      voiceparams[act_voice].retrig = state;
    }
  }
  MBNG_EVENT_ReceiveMBFM(MBNG_EVENT_MBFM_TYPE_ISRETRIG, 0, state);
}
///////////////////////////////////////////////////////////////////////////////
void MBFM_BtnDlyScale(u8 value){
  //DEBUG_MSG("MBFM_BtnDlyScale %d", value);
  if(mainmode != MBFM_MAINMODE_FM) return;
  if(holdmode != MBFM_HOLD_NONE) return;
  if(zeromode) return;
  if(!value) return;
  if(voicedupl[act_voice].voice >= 0){
    MBNG_LCD_CursorSet(0,0,0);
    MBNG_LCD_PrintFormattedString("Dupl'd to V%d, can't edit!", voicedupl[act_voice].voice+1);
    return;
  }
  u8 state = voiceparams[act_voice].dlyscale;
  state = !state;
  u8 voice;
  for(voice=0; voice<18*OPL3_COUNT; voice++){
    if(voicedupl[voice].voice == act_voice || voice == act_voice){
      voiceparams[act_voice].dlyscale = state;
    }
  }
  MBNG_EVENT_ReceiveMBFM(MBNG_EVENT_MBFM_TYPE_ISDLYSCALE, 0, state);
}
///////////////////////////////////////////////////////////////////////////////
void MBFM_Btn4Op(u8 value){
  //DEBUG_MSG("MBFM_Btn4Op %d", value);
  if(mainmode != MBFM_MAINMODE_FM) return;
  if(holdmode != MBFM_HOLD_NONE) return;
  if(zeromode) return;
  if(!value) return;
  if(act_voice & 1) return;
  if((act_voice % 18) > 11) return;
  //Check for dupl'd or linked voices
  u8 i;
  for(i=0; i<18*OPL3_COUNT; i++){
    if(voicedupl[i].voice == act_voice){
      MBNG_LCD_CursorSet(0,0,0);
      MBNG_LCD_PrintFormattedString("V%d dupl'd to this, can't change 4op!", i+1);
      return;
    }
    if(voicedupl[i].voice == act_voice){
      MBNG_LCD_CursorSet(0,0,0);
      MBNG_LCD_PrintFormattedString("V%d linked to this, can't change 4op!", i+1);
      return;
    }
  }
  u8 state = OPL3_IsChannel4Op(act_voice);
  state = !state;
  OPL3_SetFourOp(act_voice, state);
  if(state){
    //Remove second voice's MIDI assignments, LINK, and DUPL
    midichanmapping[act_voice+1] = -1;
    voicedupl[act_voice+1].voice = -1;
    voicelink[act_voice+1].voice = -1;
    //Anything connected to it too
    for(state=0; state<18*OPL3_COUNT; state++){
      if(voicedupl[state].voice == act_voice+1) voicedupl[state].voice = -1;
      if(voicelink[state].voice == act_voice+1) voicelink[state].voice = -1;
    }
  }
  voiceactualnotes[act_voice].update = 1;
  voiceactualnotes[act_voice+1].update = 1;
  MBFM_DrawVoiceStuff();
}
///////////////////////////////////////////////////////////////////////////////
void MBFM_BtnAlg(u8 value){
  //DEBUG_MSG("MBFM_BtnAlg %d", value);
  if(mainmode != MBFM_MAINMODE_FM) return;
  if(zeromode) return;
  if(voicedupl[act_voice].voice >= 0){
    if(value){
      MBNG_LCD_CursorSet(0,0,0);
      MBNG_LCD_PrintFormattedString("Dupl'd to V%d, can't edit!", voicedupl[act_voice].voice+1);
    }
    return;
  }
  if(holdmode == MBFM_HOLD_ALG && !value){
    holdmode = MBFM_HOLD_NONE;
    MBFM_DrawScreen();
  }else if(holdmode == MBFM_HOLD_NONE && value){
    holdmode = MBFM_HOLD_ALG;
    MBFM_DrawScreen();
  }
}
///////////////////////////////////////////////////////////////////////////////
void MBFM_BtnDest(u8 voicehalf, u8 value){
  //DEBUG_MSG("MBFM_BtnDest %d", value);
  if(voicehalf > 1) return;
  if(mainmode != MBFM_MAINMODE_FM) return;
  if(zeromode) return;
  if(voicehalf == 1 && !OPL3_IsChannel4Op(act_voice)) return;
  if(voicedupl[act_voice].voice >= 0){
    if(value){
      MBNG_LCD_CursorSet(0,0,0);
      MBNG_LCD_PrintFormattedString("Dupl'd to V%d, can't edit!", voicedupl[act_voice].voice+1);
    }
    return;
  }
  if(holdmode == MBFM_HOLD_DEST && !value && holdmodeidx == voicehalf){
    holdmode = MBFM_HOLD_NONE;
    MBFM_DrawScreen();
  }else if(holdmode == MBFM_HOLD_NONE && value){
    holdmode = MBFM_HOLD_DEST;
    holdmodeidx = voicehalf;
    MBFM_DrawScreen();
  }
}
///////////////////////////////////////////////////////////////////////////////
void MBFM_BtnOpParam(u8 op, u8 value){
  //DEBUG_MSG("MBFM_BtnOpParam %d, value %d", op, value);
  if(op >= 4) return;
  if(mainmode != MBFM_MAINMODE_FM) return;
  if(zeromode) return;
  if(op >= 2 && !OPL3_IsChannel4Op(act_voice)) return;
  if(voicedupl[act_voice].voice >= 0){
    if(value){
      MBNG_LCD_CursorSet(0,0,0);
      MBNG_LCD_PrintFormattedString("Dupl'd to V%d, can't edit!", voicedupl[act_voice].voice+1);
    }
    return;
  }
  if(holdmode == MBFM_HOLD_OPPARAM && !value && holdmodeidx == op){
    holdmode = MBFM_HOLD_NONE;
    MBFM_DrawScreen();
  }else if((holdmode == MBFM_HOLD_NONE || holdmode == MBFM_HOLD_OPPARAM || holdmode == MBFM_HOLD_WAVE) && value){
    holdmode = MBFM_HOLD_OPPARAM;
    holdmodeidx = op;
    MBFM_DrawScreen();
  }
}
///////////////////////////////////////////////////////////////////////////////
void MBFM_BtnOpWave(u8 op, u8 value){
  //DEBUG_MSG("MBFM_BtnOpWave %d, value %d", op, value);
  if(op >= 4) return;
  if(mainmode != MBFM_MAINMODE_FM) return;
  if(zeromode) return;
  if(op >= 2 && !OPL3_IsChannel4Op(act_voice)) return;
  if(voicedupl[act_voice].voice >= 0){
    if(value){
      MBNG_LCD_CursorSet(0,0,0);
      MBNG_LCD_PrintFormattedString("Dupl'd to V%d, can't edit!", voicedupl[act_voice].voice+1);
    }
    return;
  }
  if(holdmode == MBFM_HOLD_WAVE && !value && holdmodeidx == op){
    holdmode = MBFM_HOLD_NONE;
    MBFM_DrawScreen();
  }else if((holdmode == MBFM_HOLD_NONE || holdmode == MBFM_HOLD_WAVE || holdmode == MBFM_HOLD_OPPARAM) && value){
    holdmode = MBFM_HOLD_WAVE;
    holdmodeidx = op;
    MBFM_DrawScreen();
  }
}
///////////////////////////////////////////////////////////////////////////////

void MBFM_BtnOpMute(u8 op, u8 value){
  //DEBUG_MSG("MBFM_BtnOpMute %d, value %d", op, value);
  if(op >= 4) return;
  if(!value) return;
  if(mainmode != MBFM_MAINMODE_FM) return;
  if(zeromode) return;
  if(op >= 2 && !OPL3_IsChannel4Op(act_voice)) return;
  if(voicedupl[act_voice].voice >= 0){
    if(value){
      MBNG_LCD_CursorSet(0,0,0);
      MBNG_LCD_PrintFormattedString("Dupl'd to V%d, can't edit!", voicedupl[act_voice].voice+1);
    }
    return;
  }
  u8 voice;
  for(voice=0; voice<18*OPL3_COUNT; voice++){
    if(voicedupl[voice].voice == act_voice || voice == act_voice){
      u8 s = (voice*2) + op;
      pre_opparams[s].mute = !pre_opparams[s].mute;
      voicemisc[voice].refreshvol = 1;
    }
  }
  MBFM_DrawAlgWidget(1);
}

void MBFM_BtnALLAssign(u8 modulator, u8 value){
  if(mainmode != MBFM_MAINMODE_FM) return;
  if(holdmode == MBFM_HOLD_ASGN && !value && modl_lastsrc == modulator){
    holdmode = MBFM_HOLD_NONE;
    MBFM_DrawScreen();
  }else if(holdmode == MBFM_HOLD_NONE && value){
    holdmode = MBFM_HOLD_ASGN;
    modl_lastsrc = modulator;
    if(MBFM_GetModDepth(act_voice, modl_lastsrc, modl_lastdest) == 0){
      modl_lastdest = MBFM_FindLastModDest(act_voice, modl_lastsrc);
    }
    MBFM_DrawScreen();
  }
}
///////////////////////////////////////////////////////////////////////////////
void MBFM_BtnEGAssign(u8 eg, u8 value){
  //DEBUG_MSG("MBFM_BtnEGAssign %d, value %d", eg, value);
  if(eg >= 2) return;
  if(eg >= 1 && !OPL3_IsChannel4Op(act_voice)) return;
  MBFM_BtnALLAssign(eg+1, value);
}
///////////////////////////////////////////////////////////////////////////////
void MBFM_BtnLFOAssign(u8 lfo, u8 value){
  //DEBUG_MSG("MBFM_BtnLFOAssign %d, value %d", lfo, value);
  if(lfo >= 4) return;
  if(lfo >= 2 && !OPL3_IsChannel4Op(act_voice)) return;
  MBFM_BtnALLAssign(lfo+3, value);
}
///////////////////////////////////////////////////////////////////////////////
void MBFM_BtnLFOWave(u8 lfo, u8 value){
  //DEBUG_MSG("MBFM_BtnLFOWave %d, value %d", lfo, value);
  if(lfo >= 4) return;
  if(mainmode != MBFM_MAINMODE_FM) return;
  if(zeromode) return;
  if(lfo >= 2 && !OPL3_IsChannel4Op(act_voice)) return;
  if(holdmode == MBFM_HOLD_LFOWAVE && !value && holdmodeidx == lfo){
    holdmode = MBFM_HOLD_NONE;
    MBFM_DrawScreen();
  }else if(holdmode == MBFM_HOLD_NONE && value){
    holdmode = MBFM_HOLD_LFOWAVE;
    holdmodeidx = lfo;
    MBFM_DrawScreen();
  }
}
///////////////////////////////////////////////////////////////////////////////
void MBFM_BtnWTAssign(u8 wt, u8 value){
  //DEBUG_MSG("MBFM_BtnWTAssign %d, value %d", wt, value);
  if(wt >= 2) return;
  if(wt >= 1 && !OPL3_IsChannel4Op(act_voice)) return;
  MBFM_BtnALLAssign(wt+7, value);
}
///////////////////////////////////////////////////////////////////////////////
void MBFM_BtnVelAssign(u8 value){
  //DEBUG_MSG("MBFM_BtnVelAssign, value %d", value);
  MBFM_BtnALLAssign(9, value);
}
///////////////////////////////////////////////////////////////////////////////
void MBFM_BtnModAssign(u8 value){
  //DEBUG_MSG("MBFM_BtnModAssign, value %d", value);
  MBFM_BtnALLAssign(10, value);
}
///////////////////////////////////////////////////////////////////////////////
void MBFM_BtnVariAssign(u8 value){
  //DEBUG_MSG("MBFM_BtnVariAssign, value %d", value);
  MBFM_BtnALLAssign(11, value);
}
///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/*
  Return values:
  -  Normal: value to display, ranged value that was actually set
  - -0x4000: general error (including 4-op error) that should not happen
  - -0x3FFF: no room for new modulation connection
  - -0x3FFE: trying to edit dupl'd voice
  Set assignsrc to negative to disable it
  Set value to -0x4000 to use inc instead
*/
s16 MBFM_ControlHandler(u8 param, s8 assignsrc, u8 origvoice, s16 value, u8 inc){
  if(voicedupl[origvoice].voice >= 0) return -0x3FFE;
  u8 dv, lv;
  s16 origvalue, newvalue;
  s16 realinc = inc;
  realinc -= 0x40;
  //Check 4-op
  dv = OPL3_IsChannel4Op(origvoice);
  if(dv == 2) return -0x4000;
  if(dv == 0){
    lv = param >> 4;
    if(lv == 2 || lv == 3 || lv == 6 || lv == 8) return -0x4000;
  }
  //Find and change value from original voice
  if(assignsrc >= 0){
    //Modulation connection change
    //Show new parameter on screen
    if(modl_lastdest != param){
      modl_lastdest = param;
      screenneedsrefresh = 1;
    }
    //Change value
    origvalue = MBFM_GetModDepth(origvoice, assignsrc, param);
    newvalue  = (value < -0x2000) ? origvalue + realinc : value;
    if(newvalue > 127) newvalue = 127;
    if(newvalue < -127) newvalue = -127;
    if(zeromode) newvalue = 0;
    s8 ret = MBFM_TrySetModulation(origvoice, assignsrc, param, newvalue);
    //See if it worked
    if(ret < 0) return -0x3FFF;
    if(ret == 1){ //New, not just changed value
      screenneedsrefresh = 1;
    }
  }else{
    //Value change
    origvalue = MBFM_GetParamValue(origvoice, param);
    newvalue  = (value < -0x2000) ? origvalue + realinc : value;
    newvalue  = MBFM_SetParamValue(origvoice, param, newvalue); //Fixes it if out of range
    if(origvalue == newvalue) return newvalue; //Don't bother with the rest if no change
  }
  //Find all voices dupl'd to this with follow set
  for(dv=0; dv<18*OPL3_COUNT; dv++){
    if(dv == origvoice || (voicedupl[dv].voice == origvoice && voicedupl[dv].follow)){
      if(dv != origvoice){
        //Only for voices other than self
        if(assignsrc >= 0){
          MBFM_TrySetModulation(dv, assignsrc, param, newvalue);
        }else{
          MBFM_SetParamValue(dv, param, newvalue);
        }
      }
      //For all voices linked to this with follow set
      for(lv=0; lv<18*OPL3_COUNT; lv++){
        if(voicelink[lv].voice == dv && voicelink[lv].follow){
          if(assignsrc >= 0){
            //Modulation connection change
            if(MBFM_GetModDepth(lv, assignsrc, param) == origvalue){
              MBFM_TrySetModulation(lv, assignsrc, param, newvalue);
            }
          }else{
            //Value change--Set parameter
            if(MBFM_GetParamValue(lv, param) == origvalue){
              MBFM_SetParamValue(lv, param, newvalue);
            }
          }
        }
      }
    }
  }
  return newvalue;
}

///////////////////////////////////////////////////////////////////////////////

void MBFM_FrontPanelCtlChange(u8 modl, s16 value, u8 inc){
  if(mainmode != MBFM_MAINMODE_FM) return;
  if(zeromode) value = 0;
  if(holdmode == MBFM_HOLD_ASGN){
    value = MBFM_ControlHandler(modl, modl_lastsrc, act_voice, value, inc);
  }else if(holdmode == MBFM_HOLD_NONE){
    value = MBFM_ControlHandler(modl, -1, act_voice, value, inc);
  }else{
    return;
  }
  if(value == -0x3FFE){
    MBNG_LCD_CursorSet(0,0,0);
    MBNG_LCD_PrintFormattedString("Voice dupl'd to V%d, can't edit!", voicedupl[act_voice].voice+1);
  }else if(value == -0x3FFF){
    MBNG_LCD_CursorSet(0,0,0);
    MBNG_LCD_PrintFormattedString("No empty mod conn slots!                ");
  }else if(value >= -0x2000){
    MBNG_EVENT_ReceiveMBFM(MBNG_EVENT_MBFM_TYPE_DISPVALUE, 0, value);
    if(screenneedsrefresh){
      screenneedsrefresh = 0;
      MBFM_DrawScreen();
    }
  }
}


///////////////////////////////////////////////////////////////////////////////
void MBFM_SetVoiceTranspose(s16 value){
  MBFM_FrontPanelCtlChange(0x40, value, 0);
}
///////////////////////////////////////////////////////////////////////////////
void MBFM_SetVoiceTune(s16 value){
  MBFM_FrontPanelCtlChange(0x41, value, 0);
}
///////////////////////////////////////////////////////////////////////////////
void MBFM_SetVoicePorta(s16 value){
  MBFM_FrontPanelCtlChange(0x42, value, 0);
}
///////////////////////////////////////////////////////////////////////////////
void MBFM_SetVoiceDlyTime(s16 value){
  MBFM_FrontPanelCtlChange(0x43, value, 0);
}
///////////////////////////////////////////////////////////////////////////////
void MBFM_SetVoiceFB(s16 value){
  MBFM_FrontPanelCtlChange(0x44, value, 0);
}
///////////////////////////////////////////////////////////////////////////////
void MBFM_SetOpFMult(u8 op, s16 value){
  if(op >= 4) return;
  MBFM_FrontPanelCtlChange((op<<4)+1, value, 0);
}
///////////////////////////////////////////////////////////////////////////////
void MBFM_SetOpAtk(u8 op, s16 value){
  if(op >= 4) return;
  MBFM_FrontPanelCtlChange((op<<4)+2, value, 0);
}
///////////////////////////////////////////////////////////////////////////////
void MBFM_SetOpDec(u8 op, s16 value){
  if(op >= 4) return;
  MBFM_FrontPanelCtlChange((op<<4)+3, value, 0);
}
///////////////////////////////////////////////////////////////////////////////
void MBFM_SetOpSus(u8 op, s16 value){
  if(op >= 4) return;
  MBFM_FrontPanelCtlChange((op<<4)+4, value, 0);
}
///////////////////////////////////////////////////////////////////////////////
void MBFM_SetOpRel(u8 op, s16 value){
  if(op >= 4) return;
  MBFM_FrontPanelCtlChange((op<<4)+5, value, 0);
}
///////////////////////////////////////////////////////////////////////////////
void MBFM_SetOpVolume(u8 op, s16 value){
  if(op >= 4) return;
  MBFM_FrontPanelCtlChange((op<<4)+6, value, 0);
}
///////////////////////////////////////////////////////////////////////////////
void MBFM_SetEGAtk(u8 eg, s16 value){
  if(eg >= 2) return;
  MBFM_FrontPanelCtlChange(0x50+(eg<<4), value, 0);
}
///////////////////////////////////////////////////////////////////////////////
void MBFM_SetEGDec1(u8 eg, s16 value){
  if(eg >= 2) return;
  MBFM_FrontPanelCtlChange(0x51+(eg<<4), value, 0);
}
///////////////////////////////////////////////////////////////////////////////
void MBFM_SetEGLevel(u8 eg, s16 value){
  if(eg >= 2) return;
  MBFM_FrontPanelCtlChange(0x52+(eg<<4), value, 0);
}
///////////////////////////////////////////////////////////////////////////////
void MBFM_SetEGDec2(u8 eg, s16 value){
  if(eg >= 2) return;
  MBFM_FrontPanelCtlChange(0x53+(eg<<4), value, 0);
}
///////////////////////////////////////////////////////////////////////////////
void MBFM_SetEGSus(u8 eg, s16 value){
  if(eg >= 2) return;
  MBFM_FrontPanelCtlChange(0x54+(eg<<4), value, 0);
}
///////////////////////////////////////////////////////////////////////////////
void MBFM_SetEGRel(u8 eg, s16 value){
  if(eg >= 2) return;
  MBFM_FrontPanelCtlChange(0x55+(eg<<4), value, 0);
}
///////////////////////////////////////////////////////////////////////////////
void MBFM_SetLFOFreq(u8 lfo, s16 value){
  if(lfo >= 4) return;
  MBFM_FrontPanelCtlChange(0x56+((lfo>>1)<<4)+(lfo&1), value, 0);
}
///////////////////////////////////////////////////////////////////////////////
void MBFM_SetLFODelay(u8 lfo, s16 value){
  if(lfo >= 4) return;
  MBFM_FrontPanelCtlChange(0x58+((lfo>>1)<<4)+(lfo&1), value, 0);
}
///////////////////////////////////////////////////////////////////////////////
void MBFM_IncVoiceTranspose(u8 inc){
  MBFM_FrontPanelCtlChange(0x40, -0x4000, inc);
}
///////////////////////////////////////////////////////////////////////////////
void MBFM_IncVoiceTune(u8 inc){
  MBFM_FrontPanelCtlChange(0x41, -0x4000, inc);
}
///////////////////////////////////////////////////////////////////////////////
void MBFM_IncVoicePorta(u8 inc){
  MBFM_FrontPanelCtlChange(0x42, -0x4000, inc);
}
///////////////////////////////////////////////////////////////////////////////
void MBFM_IncVoiceDlyTime(u8 inc){
  MBFM_FrontPanelCtlChange(0x43, -0x4000, inc);
}
///////////////////////////////////////////////////////////////////////////////
void MBFM_IncVoiceFB(u8 inc){
  MBFM_FrontPanelCtlChange(0x44, -0x4000, inc);
}
///////////////////////////////////////////////////////////////////////////////
void MBFM_IncOpFMult(u8 op, u8 inc){
  if(op >= 4) return;
  MBFM_FrontPanelCtlChange((op<<4)+1, -0x4000, inc);
}
///////////////////////////////////////////////////////////////////////////////
void MBFM_IncOpAtk(u8 op, u8 inc){
  if(op >= 4) return;
  MBFM_FrontPanelCtlChange((op<<4)+2, -0x4000, inc);
}
///////////////////////////////////////////////////////////////////////////////
void MBFM_IncOpDec(u8 op, u8 inc){
  if(op >= 4) return;
  MBFM_FrontPanelCtlChange((op<<4)+3, -0x4000, inc);
}
///////////////////////////////////////////////////////////////////////////////
void MBFM_IncOpSus(u8 op, u8 inc){
  if(op >= 4) return;
  MBFM_FrontPanelCtlChange((op<<4)+4, -0x4000, inc);
}
///////////////////////////////////////////////////////////////////////////////
void MBFM_IncOpRel(u8 op, u8 inc){
  if(op >= 4) return;
  MBFM_FrontPanelCtlChange((op<<4)+5, -0x4000, inc);
}
///////////////////////////////////////////////////////////////////////////////
void MBFM_IncOpVolume(u8 op, u8 inc){
  if(op >= 4) return;
  MBFM_FrontPanelCtlChange((op<<4)+6, -0x4000, inc);
}
///////////////////////////////////////////////////////////////////////////////
void MBFM_IncEGAtk(u8 eg, u8 inc){
  if(eg >= 2) return;
  MBFM_FrontPanelCtlChange(0x50+(eg<<4), -0x4000, inc);
}
///////////////////////////////////////////////////////////////////////////////
void MBFM_IncEGDec1(u8 eg, u8 inc){
  if(eg >= 2) return;
  MBFM_FrontPanelCtlChange(0x51+(eg<<4), -0x4000, inc);
}
///////////////////////////////////////////////////////////////////////////////
void MBFM_IncEGLevel(u8 eg, u8 inc){
  if(eg >= 2) return;
  MBFM_FrontPanelCtlChange(0x52+(eg<<4), -0x4000, inc);
}
///////////////////////////////////////////////////////////////////////////////
void MBFM_IncEGDec2(u8 eg, u8 inc){
  if(eg >= 2) return;
  MBFM_FrontPanelCtlChange(0x53+(eg<<4), -0x4000, inc);
}
///////////////////////////////////////////////////////////////////////////////
void MBFM_IncEGSus(u8 eg, u8 inc){
  if(eg >= 2) return;
  MBFM_FrontPanelCtlChange(0x54+(eg<<4), -0x4000, inc);
}
///////////////////////////////////////////////////////////////////////////////
void MBFM_IncEGRel(u8 eg, u8 inc){
  if(eg >= 2) return;
  MBFM_FrontPanelCtlChange(0x55+(eg<<4), -0x4000, inc);
}
///////////////////////////////////////////////////////////////////////////////
void MBFM_IncLFOFreq(u8 lfo, u8 inc){
  if(lfo >= 4) return;
  MBFM_FrontPanelCtlChange(0x56+((lfo>>1)<<4)+(lfo&1), -0x4000, inc);
}
///////////////////////////////////////////////////////////////////////////////
void MBFM_IncLFODelay(u8 lfo, u8 inc){
  if(lfo >= 4) return;
  MBFM_FrontPanelCtlChange(0x58+((lfo>>1)<<4)+(lfo&1), -0x4000, inc);
}
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
void MBFM_SetDatawheel(u8 value){
  lastdatawheelchange = MIOS32_TIMESTAMP_Get();
  char *nameptr; u8 i;
  switch(mainmode){
  case MBFM_MAINMODE_FM:
    switch(screenmode){
    case MBFM_SCREENMODE_FM_MOD:
      switch(screenmodeidx){
      case 0:
        //Source
        if(value < 0) value = 0;
        if(value > 11) value = 11;
        modllists[act_voice][act_modconn].src = value;
        break;
      case 1:
        //Dest
        if(value < 0) value = 0;
        i = (OPL3_IsChannel4Op(act_midich) ? 0x8F : 0x7F);
        if(value > i) value = i;
        modllists[act_voice][act_modconn].dest = value;
        break;
      case 2:
        //Depth
        if(value < -127) value = -127;
        if(value > 127) value = 127;
        modllists[act_voice][act_modconn].pre_depth = value;
        break;
      default:
        return;
      }
      MBFM_DrawScreen();
      break;
    case MBFM_SCREENMODE_FM_TEMPER:
      switch(screenmodeidx){
      case 0:
        //Basenote
        SetOPL3BaseNote(value, GetEqualTemperBaseFreq(value));
        break;
      case 1:
        //Basefreq
        SetOPL3BaseNote(GetOPL3BaseNote(), value * 10); //TODO value is u8!
        break;
      case 2:
        //Subnote
        if(value > 11) value = 11;
        act_temper = value;
        break;
      case 3:
        //Numerator
        SetTemperNumerator(act_temper, value);
        break;
      case 4:
        //Denominator
        SetTemperDenominator(act_temper, value);
        break;
      default:
        return;
      }
      MBFM_DrawScreen();
      break;
    case MBFM_SCREENMODE_FM_OPT:
      switch(screenmodeidx){
      case 0:
        //msecperdelaystep
        if(value <= 0) value = 1;
        if(value > 50) value = 50;
        msecperdelaystep = value;
        MBNG_LCD_CursorSet(0,1,1);
        MBNG_LCD_PrintFormattedString("%3d", msecperdelaystep);
        break;
      case 1:
        //tuningrange
        if(value <= 0) value = 1;
        if(value > 48) value = 48;
        tuningrange = value;
        MBNG_LCD_CursorSet(0,6,1);
        MBNG_LCD_PrintFormattedString("%3d", tuningrange);
        break;
      case 2:
        //pitchbendrange
        if(value <= 0) value = 0;
        if(value > 24) value = 24;
        pitchbendrange = value;
        MBNG_LCD_CursorSet(0,11,1);
        MBNG_LCD_PrintFormattedString("%3d", pitchbendrange);
        break;
      }
      break;
    }
    break;
  case MBFM_MAINMODE_MIDI:
    switch(screenmode){
    case MBFM_SCREENMODE_NONE:
      if(act_midich == 9){
        //Adjust MIDI note number for drum
        if(value > 127) value = 127;
        mididrummapping[seq_seldrum] = value;
        MBNG_EVENT_ReceiveMBFM(MBNG_EVENT_MBFM_TYPE_DISPVALUE, 0, value);
        MBFM_DrawScreen();
      }
      break;
    case MBFM_SCREENMODE_MIDI_LOAD:
    case MBFM_SCREENMODE_MIDI_SAVE:
      patchloadquery = 1;
      if(value > 127) value = 127;
      switch(screenmodeidx){
      case 0:
        midibankmsb[act_midich] = value;
        MBNG_LCD_CursorSet(0,21,1);
        MBNG_LCD_PrintFormattedString("%3d", midibankmsb[act_midich]);
        break;
      case 1:
        midibanklsb[act_midich] = value;
        MBNG_LCD_CursorSet(0,26,1);
        MBNG_LCD_PrintFormattedString("%3d", midibanklsb[act_midich]);
        break;
      case 2:
        midiprogram[act_midich] = value;
        MBNG_LCD_CursorSet(0,31,1);
        MBNG_LCD_PrintFormattedString("%3d", midiprogram[act_midich]);
        break;
      default:
        patchloadquery = 0;
        return;
      }
      break;
    case MBFM_SCREENMODE_MIDI_NAME:
      if(act_midich == 9){
        nameptr = drumnames[prgstate[9].drum];
      }else{
        nameptr = patchnames[act_midich];
      }
      if(value < ' ') value = ' ';
      if(value == '='){
        value++;
      }
      nameptr[screenmodeidx] = value;
      MBNG_LCD_CursorSet(0,5+screenmodeidx,0);
      MBNG_LCD_PrintFormattedString("%c", (char)value);
      break;
    case MBFM_SCREENMODE_MIDI_MIXER:
      if(value > 127) value = 127;
      if(screenmodeidx){
        midiexpr[act_midich].level = value;
      }else{
        midivol[act_midich].level = value;
      }
      for(i=0; i<18*OPL3_COUNT; i++){
        if(midichanmapping[i] == act_midich ||
            (voicedupl[i].voice >= 0 && midichanmapping[voicedupl[i].voice] == act_midich) ||
            (voicelink[i].voice >= 0 && midichanmapping[voicelink[i].voice] == act_midich)){
          voicemisc[i].refreshvol = 1;
        }
      }
      MBNG_LCD_CursorSet(0,0,0);
      MBNG_LCD_PrintFormattedString("%3d", screenmodeidx ? midiexpr[act_midich].level : midivol[act_midich].level);
      MBNG_LCD_CursorSet(0,9+(act_midich<<1),1);
      MBNG_LCD_PrintChar(screenmodeidx ? (midiexpr[act_midich].level >> 4) : (midivol[act_midich].level >> 4));
      break;
    case MBFM_SCREENMODE_MIDI_IN:
      if(midiportinopts[act_port].mode == 2){
        if(value > 15) value = 15;
        midiportinopts[act_port].chan = value;
        MBFM_DrawScreen();
      }
      break;
    }
    break;
  }
}
///////////////////////////////////////////////////////////////////////////////
void MBFM_IncDatawheel(u8 inc){
  char *nameptr;
  lastdatawheelchange = MIOS32_TIMESTAMP_Get();
  s16 realinc = (s16)inc - 0x40;
  s32 value; u8 i;
  switch(mainmode){
  case MBFM_MAINMODE_FM:
    switch(screenmode){
    case MBFM_SCREENMODE_FM_MOD:
      switch(screenmodeidx){
      case 0:
        //Source
        value = modllists[act_voice][act_modconn].src;
        value += realinc;
        if(value < 0) value = 0;
        if(value > 11) value = 11;
        modllists[act_voice][act_modconn].src = value;
        break;
      case 1:
        //Dest
        value = modllists[act_voice][act_modconn].dest;
        value += realinc;
        if(value < 0) value = 0;
        i = (OPL3_IsChannel4Op(act_midich) ? 0x8F : 0x7F);
        if(value > i) value = i;
        modllists[act_voice][act_modconn].dest = value;
        break;
      case 2:
        //Depth
        value = modllists[act_voice][act_modconn].pre_depth;
        value += realinc;
        if(value < -127) value = -127;
        if(value > 127) value = 127;
        modllists[act_voice][act_modconn].pre_depth = value;
        break;
      default:
        return;
      }
      MBFM_DrawScreen();
      break;
    case MBFM_SCREENMODE_FM_TEMPER:
      switch(screenmodeidx){
      case 0:
        //Basenote
        value = GetOPL3BaseNote();
        value += realinc;
        SetOPL3BaseNote(value, GetEqualTemperBaseFreq(value));
        break;
      case 1:
        //Basefreq
        value = GetOPL3BaseFreq();
        value += realinc;
        if(value < 0) value = 0;
        if(value > 0xFFFF) value = 0xFFFF;
        SetOPL3BaseNote(GetOPL3BaseNote(), value);
        break;
      case 2:
        //Subnote
        value = act_temper;
        value += realinc;
        if(value < 0) value = 0;
        if(value > 11) value = 11;
        act_temper = value;
        break;
      case 3:
        //Numerator
        value = GetTemperNumerator(act_temper);
        value += realinc;
        if(value < 0) value = 0;
        if(value > 0xFF) value = 0xFF;
        SetTemperNumerator(act_temper, value);
        break;
      case 4:
        //Denominator
        value = GetTemperDenominator(act_temper);
        value += realinc;
        if(value < 0) value = 0;
        if(value > 0xFF) value = 0xFF;
        SetTemperDenominator(act_temper, value);
        break;
      default:
        return;
      }
      MBFM_DrawScreen();
      break;
    case MBFM_SCREENMODE_FM_OPT:
      switch(screenmodeidx){
      case 0:
        //msecperdelaystep
        value = msecperdelaystep;
        value += realinc;
        if(value <= 0) value = 1;
        if(value > 50) value = 50;
        msecperdelaystep = value;
        MBNG_LCD_CursorSet(0,1,1);
        MBNG_LCD_PrintFormattedString("%3d", msecperdelaystep);
        break;
      case 1:
        //tuningrange
        value = tuningrange;
        value += realinc;
        if(value <= 0) value = 1;
        if(value > 48) value = 48;
        tuningrange = value;
        MBNG_LCD_CursorSet(0,6,1);
        MBNG_LCD_PrintFormattedString("%3d", tuningrange);
        break;
      case 2:
        //pitchbendrange
        value = pitchbendrange;
        value += realinc;
        if(value <= 0) value = 0;
        if(value > 24) value = 24;
        pitchbendrange = value;
        MBNG_LCD_CursorSet(0,11,1);
        MBNG_LCD_PrintFormattedString("%3d", pitchbendrange);
        break;
      }
      break;
    }
    break;
  
  case MBFM_MAINMODE_MIDI:
    switch(screenmode){
    case MBFM_SCREENMODE_NONE:
      if(act_midich == 9){
        //Adjust MIDI note number for drum
        value = (s16)mididrummapping[seq_seldrum] + realinc;
        if(value < 0) value = 0;
        if(value > 127) value = 127;
        mididrummapping[seq_seldrum] = value;
        MBNG_EVENT_ReceiveMBFM(MBNG_EVENT_MBFM_TYPE_DISPVALUE, 0, value);
        MBFM_DrawScreen();
      }
      break;
    case MBFM_SCREENMODE_MIDI_LOAD:
    case MBFM_SCREENMODE_MIDI_SAVE:
      patchloadquery = 1;
      switch(screenmodeidx){
      case 0:
        value = (s16)midibankmsb[act_midich] + realinc;
        if(value < 0) value = 0;
        if(value > 127) value = 127;
        midibankmsb[act_midich] = value;
        MBNG_LCD_CursorSet(0,21,1);
        MBNG_LCD_PrintFormattedString("%3d", midibankmsb[act_midich]);
        break;
      case 1:
        value = (s16)midibanklsb[act_midich] + realinc;
        if(value < 0) value = 0;
        if(value > 127) value = 127;
        midibanklsb[act_midich] = value;
        MBNG_LCD_CursorSet(0,26,1);
        MBNG_LCD_PrintFormattedString("%3d", midibanklsb[act_midich]);
        break;
      case 2:
        value = (s16)midiprogram[act_midich] + realinc;
        if(value < 0) value = 0;
        if(value > 127) value = 127;
        midiprogram[act_midich] = value;
        MBNG_LCD_CursorSet(0,31,1);
        MBNG_LCD_PrintFormattedString("%3d", midiprogram[act_midich]);
        break;
      default:
        patchloadquery = 0;
        return;
      }
      break;
    case MBFM_SCREENMODE_MIDI_NAME:
      if(act_midich == 9){
        nameptr = drumnames[prgstate[9].drum];
      }else{
        nameptr = patchnames[act_midich];
      }
      value = nameptr[screenmodeidx];
      value += realinc;
      if(value < ' ') value = ' ';
      if(value > 0xFF) value = 0xFF;
      if(value == '='){
        if(realinc > 0){
          value++;
        }else{
          value--;
        }
      }
      nameptr[screenmodeidx] = value;
      MBNG_LCD_CursorSet(0,5+screenmodeidx,0);
      MBNG_LCD_PrintFormattedString("%c", (char)value);
      break;
    case MBFM_SCREENMODE_MIDI_MIXER:
      if(screenmodeidx){
        value = midiexpr[act_midich].level;
        value += realinc;
        if(value < 0) value = 0;
        if(value > 127) value = 127;
        midiexpr[act_midich].level = value;
      }else{
        value = midivol[act_midich].level;
        value += realinc;
        if(value < 0) value = 0;
        if(value > 127) value = 127;
        midivol[act_midich].level = value;
      }
      for(i=0; i<18*OPL3_COUNT; i++){
        if(midichanmapping[i] == act_midich ||
            (voicedupl[i].voice >= 0 && midichanmapping[voicedupl[i].voice] == act_midich) ||
            (voicelink[i].voice >= 0 && midichanmapping[voicelink[i].voice] == act_midich)){
          voicemisc[i].refreshvol = 1;
        }
      }
      MBNG_LCD_CursorSet(0,0,0);
      MBNG_LCD_PrintFormattedString("%3d", screenmodeidx ? midiexpr[act_midich].level : midivol[act_midich].level);
      MBNG_LCD_CursorSet(0,9+(act_midich<<1),1);
      MBNG_LCD_PrintChar(screenmodeidx ? (midiexpr[act_midich].level >> 4) : (midivol[act_midich].level >> 4));
      break;
    case MBFM_SCREENMODE_MIDI_IN:
      if(midiportinopts[act_port].mode == 2){
        value = midiportinopts[act_port].chan;
        value += realinc;
        if(value < 0) value = 0;
        if(value > 15) value = 15;
        midiportinopts[act_port].chan = value;
        MBFM_DrawScreen();
      }
      break;
    }
    break;
    
  case MBFM_MAINMODE_SEQ:
    MBFM_SEQ_Datawheel(realinc);
    break;
  }
}

///////////////////////////////////////////////////////////////////////////////
void MBFM_ControlMod(u8 channel, u8 value){
  //DEBUG_MSG("MBFM_ControlMod, ch %d, value %d", channel, value);
  if(channel >= 16) return;
  u8 i;
  if(value > 127) value = 127;
  for(i=0; i<18*OPL3_COUNT; i++){
    if(midichanmapping[i] == channel){
      modl_mods[i] = value;
    }
  }
}
///////////////////////////////////////////////////////////////////////////////
void MBFM_ControlVari(u8 channel, u8 value){
  //DEBUG_MSG("MBFM_ControlVari, ch %d, value %d", channel, value);
  if(channel >= 16) return;
  u8 i;
  if(value > 127) value = 127;
  for(i=0; i<18*OPL3_COUNT; i++){
    if(midichanmapping[i] == channel){
      modl_varis[i] = value;
    }
  }
}
///////////////////////////////////////////////////////////////////////////////
void MBFM_ControlSus(u8 channel, u8 value){
  //DEBUG_MSG("MBFM_ControlSus, ch %d, value %d", channel, value);
  if(channel >= 16) return;
  value = (value >= 64);
  //chansustainpedals[channel] = value;
  //if(!value){
  //  MBFM_LetGoSustainPedal(channel);
  //}
}
///////////////////////////////////////////////////////////////////////////////
void MBFM_ControlCC(u8 channel, u8 cc, u8 value){
  //switch(cc){
  //case MBFM_CC_
  //}
}
///////////////////////////////////////////////////////////////////////////////



void MBFM_SetUpLCDChars(){
  switch(mainmode){
  case MBFM_MAINMODE_FM:
    MIOS32_LCD_SpecialCharsInit((u8 *)charset_mbfm_fm);
    break;
  case MBFM_MAINMODE_MIDI:
    MBNG_LCD_SpecialCharsInit(0, 1);
    break;
  }
}

///////////////////////////////////////////////////////////////////////////////
void MBFM_DrawScreen(){
  u8 i; u16 j; char* nameptr;
  MUTEX_LCD_TAKE;
  MBNG_LCD_Clear();
  switch(mainmode){
  case MBFM_MAINMODE_FM:
    switch(holdmode){
    case MBFM_HOLD_DUPL:
      MBNG_LCD_CursorSet(0,0,0);
      MBNG_LCD_PrintFormattedString("DUPL (polyphonic copy) to voice %d", voicedupl[act_voice].voice+1);
      MBNG_LCD_CursorSet(0,0,1);
      MBNG_LCD_PrintFormattedString("None (0)  %cFollow", voicedupl[act_voice].follow ? '*' : ' ');
      break;
    case MBFM_HOLD_LINK:
      MBNG_LCD_CursorSet(0,0,0);
      MBNG_LCD_PrintFormattedString("LINK (mono copy with mods) to voice %d", voicelink[act_voice].voice+1);
      MBNG_LCD_CursorSet(0,0,1);
      MBNG_LCD_PrintFormattedString("None (0)  %cFollow", voicelink[act_voice].follow ? '*' : ' ');
      break;
    case MBFM_HOLD_ALG:
      i = voiceparams[act_voice].alg;
      MBNG_LCD_CursorSet(0,0,0);
      MBNG_LCD_PrintFormattedString("FM algorithm currently %d", i);
      MBNG_LCD_CursorSet(0,0,1);
      if(OPL3_IsChannel4Op(act_voice)){
        MBNG_LCD_PrintFormattedString(" A~B~C~D   A~B_C~D   A_B~C~D   A_B~C_D  ");
      }else{
        MBNG_LCD_PrintFormattedString("   A~B       A,B");
      }
      MBNG_LCD_CursorSet(0,10*i, 1);
      MBNG_LCD_PrintFormattedString("*");
      MBNG_LCD_CursorSet(0,10*i + 8, 1);
      MBNG_LCD_PrintFormattedString("*");
      break;
    case MBFM_HOLD_DEST:
      i = voiceparams[act_voice+holdmodeidx].dest;
      for(j=0; j<4; j++){
        MBNG_LCD_CursorSet(0,22+(5*j), 0);
        if(i&1){
          MBNG_LCD_PrintFormattedString("*");
        }
        i >>= 1;
      }
      MBNG_LCD_CursorSet(0,0,1);
      MBNG_LCD_PrintFormattedString("Voice output chan:    L    R   Bus1 Bus2");
      break;
    case MBFM_HOLD_OPPARAM:
      i = (act_voice*2)+holdmodeidx;
      if(pre_opparams[i].frqLFO){
        MBNG_LCD_CursorSet(0,2,0);
        MBNG_LCD_PrintFormattedString("*");
      }
      if(pre_opparams[i].ampLFO){
        MBNG_LCD_CursorSet(0,7,0);
        MBNG_LCD_PrintFormattedString("*");
      }
      MBNG_LCD_CursorSet(0,12,0);
      MBNG_LCD_PrintFormattedString("%d", pre_opparams[i].ampKSCL);
      if(pre_opparams[i].rateKSCL){
        MBNG_LCD_CursorSet(0,17,0);
        MBNG_LCD_PrintFormattedString("*");
      }
      if(pre_opparams[i].dosus){
        MBNG_LCD_CursorSet(0,22,0);
        MBNG_LCD_PrintFormattedString("*");
      }
      MBNG_LCD_CursorSet(0,0,1);
      MBNG_LCD_PrintFormattedString(" Frq\7 Amp\7 KSL  KSR DoSus");
      break;
    case MBFM_HOLD_WAVE:
      MBNG_LCD_CursorSet(0,(5*pre_opparams[(act_voice*2)+holdmodeidx].wave)+2,0);
      MBNG_LCD_PrintFormattedString("*");
      MBNG_LCD_CursorSet(0,0,1);
      MBNG_LCD_PrintFormattedString(" ^\1   ^-   ^^   \2\2   \3-   \4-   \5-   \6\6");
      MBNG_LCD_CursorSet(0,1,1);
      MBNG_LCD_PrintChar(0);
      MBNG_LCD_CursorSet(0,6,1);
      MBNG_LCD_PrintChar(0);
      MBNG_LCD_CursorSet(0,11,1);
      MBNG_LCD_PrintChar(0);
      MBNG_LCD_PrintChar(0);
      break;
    case MBFM_HOLD_ASGN:
      MBNG_LCD_CursorSet(0,0,0);
      if(MBFM_GetModDepth(act_voice, modl_lastsrc, modl_lastdest) != 0){
        MBNG_LCD_PrintFormattedString("Assign %s to %s", 
                        MBFM_GetModSourceName(modl_lastsrc),
                        MBFM_GetParamName(modl_lastdest));
      }else{
        MBNG_LCD_PrintFormattedString("%s not assigned to %s", 
                        MBFM_GetModSourceName(modl_lastsrc),
                        MBFM_GetParamName(modl_lastdest));
      }
      MBNG_LCD_CursorSet(0,0,1);
      MBNG_LCD_PrintFormattedString("%d of %d mod conns used             Zero",
                        MBFM_GetNumModulationConnections(act_voice), MBFM_MODL_NUMCONN);
      break;
    case MBFM_HOLD_LFOWAVE:
      i = modlparams[act_voice+(holdmodeidx>>1)].LFOwave[holdmodeidx&1];
      MBNG_LCD_CursorSet(0,(5*(i & 127)) + 2,0);
      MBNG_LCD_PrintFormattedString("*");
      MBNG_LCD_CursorSet(0,0,1);
      MBNG_LCD_PrintFormattedString(" Sin  Tri ExpSw Saw  Sqr  Rand      Free");
      if(i & 128){
        MBNG_LCD_CursorSet(0,35,1);
        MBNG_LCD_PrintFormattedString("*");
      }
      break;
    default:
      switch(screenmode){
      case MBFM_SCREENMODE_NONE:
        MBNG_LCD_CursorSet(0,0,0);
        MBNG_LCD_PrintFormattedString("V%d %dop / %d mod conns", act_voice+1,
                          (OPL3_IsChannel4Op(act_voice)+1)*2,
                          MBFM_GetNumModulationConnections(act_voice));
        MBNG_LCD_CursorSet(0,0,1);
        MBNG_LCD_PrintFormattedString(" Mod                Tmpr  Opt OPL3 Zero ");
        break;
      case MBFM_SCREENMODE_FM_MOD:
        MBNG_LCD_CursorSet(0,0,0);
        mbfm_modlentry_t conn = modllists[act_voice][act_modconn];
        MBNG_LCD_PrintFormattedString("Conn %d: %s ~ %s", act_modconn+1,
              MBFM_GetModSourceName(conn.src),
              conn.src ? MBFM_GetParamName(conn.dest) : "None");
        MBNG_LCD_CursorSet(0,0,1);
        MBNG_LCD_PrintFormattedString("  ^    v    Source     Dest   Depth %d", conn.depth);
        switch(screenmodeidx){
        case 0: MBNG_LCD_CursorSet(0,11,1); break;
        case 1: MBNG_LCD_CursorSet(0,22,1); break;
        case 2: MBNG_LCD_CursorSet(0,29,1); break;
        }
        MBNG_LCD_PrintChar('~');
        break;
      case MBFM_SCREENMODE_FM_TEMPER:
        if(GetOPL3Temperament()){
          i = GetOPL3BaseNote();
          j = GetOPL3BaseFreq();
          MBNG_LCD_CursorSet(0,0,0);
          ///////////////////////////////+---++---++---++---++---++---++---++---+
          MBNG_LCD_PrintFormattedString("Tmpr   Base    Freq Note Numr Dnmr Freq ");
          MBNG_LCD_CursorSet(0,0,1);
          MBNG_LCD_PrintFormattedString("Cust: %d,%s%d %4d.%d", 
                i, NoteName(i), i/12, j/10, j%10);
          j = GetTemperFrequency(act_temper);
          MBNG_LCD_CursorSet(0,20,1);
          MBNG_LCD_PrintFormattedString("%d,%s %3d  %3d %4d.%d", 
                act_temper, NoteName(GetOPL3BaseNote()+act_temper),
                GetTemperNumerator(act_temper)+1, GetTemperDenominator(act_temper)+1, j/10, j%10);
          MBNG_LCD_CursorSet(0,(screenmodeidx == 0) ? 6 : (9 + (5*screenmodeidx)), 0);
          MBNG_LCD_PrintFormattedString("~");
        }else{
          MBNG_LCD_CursorSet(0,0,0);
          MBNG_LCD_PrintFormattedString("Temperament");
          MBNG_LCD_CursorSet(0,0,1);
          MBNG_LCD_PrintFormattedString("Equal");
        }
        break;
      case MBFM_SCREENMODE_FM_OPT:
        MBNG_LCD_CursorSet(0,0,0);
        MBNG_LCD_PrintFormattedString(" Dly Tune Bend ");
        MBNG_LCD_CursorSet(0,0,1);
        MBNG_LCD_PrintFormattedString(" %3d  %3d  %3d ", 
              msecperdelaystep, tuningrange, pitchbendrange);
        MBNG_LCD_CursorSet(0,5*screenmodeidx, 1);
        MBNG_LCD_PrintFormattedString("~");
        break;
      case MBFM_SCREENMODE_FM_OPL3:
        MBNG_LCD_CursorSet(0,0,0);
        MBNG_LCD_PrintFormattedString(" Frq\7 Amp\7     OPL3:  A                ");
        #if OPL3_COUNT >= 2
        MBNG_LCD_CursorSet(0,27,0);
        MBNG_LCD_PrintFormattedString("B");
        #endif
        MBNG_LCD_CursorSet(0,2,1);
        MBNG_LCD_PrintFormattedString("%s   %s           %s                   ",
                        opl3_chip[0].vibratodepth ? "Hi" : "Lo",
                        opl3_chip[0].tremelodepth ? "Hi" : "Lo",
                        opl3_chip[0].percussion ? "Perc" : "Voic");
        #if OPL3_COUNT >= 2
        MBNG_LCD_CursorSet(0,25,1);
        MBNG_LCD_PrintFormattedString("%s", opl3_chip[1].percussion ? "Perc" : "Voic");
        #endif
        break;
      }
    }
    break;
  case MBFM_MAINMODE_MIDI:
    if(holdmode == MBFM_HOLD_SELECT){
      MBNG_LCD_CursorSet(0,0,0);
      MBNG_LCD_PrintFormattedString("Ch%02d assign: %d 2op %d 4op: V%d", act_midich+1,
                        MBFM_GetNumVoicesInChannel(act_midich, 2),
                        MBFM_GetNumVoicesInChannel(act_midich, 4),
                        midifirstvoice[act_midich]+1);
      MBNG_LCD_CursorSet(0,0,1);
      MBNG_LCD_PrintFormattedString("Clear     %cAuto-Dupl  All-Off", (chanoptions[act_midich].autodupl ? '*' : ' '));
    }else{
      switch(screenmode){
      case MBFM_SCREENMODE_NONE:
        if(act_midich == 9){
          //Percussion
          MBNG_LCD_CursorSet(0,0,0);
          MBNG_LCD_PrintFormattedString("Ch 10 Perc %s: %s ~ Note %d",
                            (seq_seldrum % 6 == 0) ? "BD" : "SD/HH/TT/CY",
                            MBFM_Drum_GetName(seq_seldrum), 
                            mididrummapping[seq_seldrum]);
        }else{
          MBNG_LCD_CursorSet(0,0,0);
          MBNG_LCD_PrintFormattedString("Ch %02d %s: %d 2op %d 4op: V%d", act_midich+1, patchnames[act_midich],
                            MBFM_GetNumVoicesInChannel(act_midich, 2),
                            MBFM_GetNumVoicesInChannel(act_midich, 4),
                            midifirstvoice[act_midich]+1);
        }
        MBNG_LCD_CursorSet(0,0,1);
        MBNG_LCD_PrintFormattedString(" Load Save Auto Mix  In  Out");
        break;
      case MBFM_SCREENMODE_MIDI_LOAD:
        MBNG_LCD_CursorSet(0,0,0);
        if(act_midich == 9){
          MBNG_LCD_PrintFormattedString("%s %s", MBFM_CondensedDrum_GetName(prgstate[9].drum), 
                drumnames[prgstate[9].drum]);
        }else{
          MBNG_LCD_PrintFormattedString("Ch%02d %s", act_midich+1, patchnames[act_midich]);
        }
        MBNG_LCD_CursorSet(0,20,0);
        MBNG_LCD_PrintFormattedString(" MSB  LSB Prog Load ");
        MBNG_LCD_CursorSet(0,20,1);
        MBNG_LCD_PrintFormattedString(" %3d  %3d  %3d  Go! ", 
                    midibankmsb[act_midich], midibanklsb[act_midich], midiprogram[act_midich]);
        MBNG_LCD_CursorSet(0,20+(5*screenmodeidx),1);
        MBNG_LCD_PrintFormattedString("~");
        MBNG_LCD_CursorSet(0,0,1);
        MBNG_LCD_PrintFormattedString("Pat: %s", loadpatchname);
        break;
      case MBFM_SCREENMODE_MIDI_SAVE:
        MBNG_LCD_CursorSet(0,0,0);
        if(act_midich == 9){
          MBNG_LCD_PrintFormattedString("%s %s", MBFM_CondensedDrum_GetName(prgstate[9].drum), 
                drumnames[prgstate[9].drum]);
        }else{
          MBNG_LCD_PrintFormattedString("Ch%02d %s", act_midich+1, patchnames[act_midich]);
        }
        MBNG_LCD_CursorSet(0,21,0);
        MBNG_LCD_PrintFormattedString("MSB  LSB Prog Save ");
        MBNG_LCD_CursorSet(0,21,1);
        MBNG_LCD_PrintFormattedString("%3d  %3d  %3d  Go! ", 
                    midibankmsb[act_midich], midibanklsb[act_midich], midiprogram[act_midich]);
        MBNG_LCD_CursorSet(0,20+(5*screenmodeidx),1);
        MBNG_LCD_PrintFormattedString("~");
        MBNG_LCD_CursorSet(0,0,1);
        MBNG_LCD_PrintFormattedString("Over:%s", loadpatchname);
        MBNG_LCD_CursorSet(0,16,1);
        MBNG_LCD_PrintFormattedString("Name");
        break;
      case MBFM_SCREENMODE_MIDI_NAME:
        MBNG_LCD_CursorSet(0,0,0);
        MBNG_LCD_PrintFormattedString("Ch%02d %s", act_midich+1,
              (act_midich == 9) ? drumnames[prgstate[9].drum] : patchnames[act_midich]);
        MBNG_LCD_CursorSet(0,15,1);
        MBNG_LCD_PrintFormattedString(" Case Bksp  <    >   OK");
        MBNG_LCD_CursorSet(0,5+screenmodeidx,1);
        MBNG_LCD_PrintFormattedString("^");
        break;
      case MBFM_SCREENMODE_MIDI_AUTO:
        MBNG_LCD_CursorSet(0,0,0);
        MBNG_LCD_PrintFormattedString("CC:   Load BMSB BLSB Prog Pan Other");
        chanoptions_t co = chanoptions[act_midich];
        MBNG_LCD_CursorSet(0,0,1);
        MBNG_LCD_PrintFormattedString("       %c    %c    %c    %c    %c    %c", 
            co.autoload ? '*' : ' ', co.cc_bmsb ? '*' : ' ',
            co.cc_blsb  ? '*' : ' ', co.cc_prog ? '*' : ' ',
            co.cc_pan   ? '*' : ' ', co.cc_other? '*' : ' ');
        MBNG_LCD_CursorSet(0,0,1);
        if(screenmodeidx){
          MBNG_LCD_PrintFormattedString("-All-");
        }else{
          MBNG_LCD_PrintFormattedString("Chan");
        }
        break;
      case MBFM_SCREENMODE_MIDI_MIXER:
        nameptr = (act_midich == 9) ? "Drums" : patchnames[act_midich];
        if(act_midich < 8){
          MBNG_LCD_CursorSet(0,9+(act_midich<<1), 0);
          MBNG_LCD_PrintFormattedString("v-%s", nameptr);
        }else{
          MBNG_LCD_CursorSet(0,8+(act_midich<<1)-strlen(nameptr),0);
          MBNG_LCD_PrintFormattedString("%s-v", nameptr);
        }
        MBNG_LCD_CursorSet(0,0,0);
        if(screenmodeidx){
          MBNG_LCD_PrintFormattedString("%3d Enab ", midiexpr[act_midich].level);
          MBNG_LCD_CursorSet(0,0,1);
          MBNG_LCD_PrintFormattedString(" Expr  %c ", midiexpr[act_midich].enable ? '*' : ' ');
        }else{
          MBNG_LCD_PrintFormattedString("%3d Enab ", midivol[act_midich].level);
          MBNG_LCD_CursorSet(0,0,1);
          MBNG_LCD_PrintFormattedString(" Vol   %c ", midivol[act_midich].enable ? '*' : ' ');
        }
        for(i=0; i<15; i++){
          MBNG_LCD_PrintChar(screenmodeidx ? (midiexpr[i].level >> 4) : (midivol[i].level >> 4));
          MBNG_LCD_PrintChar(' ');
        }
        i = 15;
        MBNG_LCD_PrintChar(screenmodeidx ? (midiexpr[i].level >> 4) : (midivol[i].level >> 4));
        break;
      case MBFM_SCREENMODE_MIDI_IN:
        MBNG_LCD_CursorSet(0,0,0);
        MBNG_LCD_PrintFormattedString("MIDI In Port %s", MIDIPortName(act_port));
        MBNG_LCD_CursorSet(0,2,1);
        MBNG_LCD_PrintFormattedString("^    v");
        MBNG_LCD_CursorSet(0,20,0);
        MBNG_LCD_PrintFormattedString("Mode ");
        i = midiportinopts[act_port].mode;
        if(i == 2){
          MBNG_LCD_PrintFormattedString("Chan ");
          MBNG_LCD_CursorSet(0,26,1);
          MBNG_LCD_PrintFormattedString("%2d", midiportinopts[act_port].chan+1);
        }
        MBNG_LCD_CursorSet(0,20,1);
        switch(i){
        case 0:  MBNG_LCD_PrintFormattedString("Disab"); break;
        case 1:  MBNG_LCD_PrintFormattedString("Norm"); break;
        case 2:  MBNG_LCD_PrintFormattedString("Force"); break;
        default: MBNG_LCD_PrintFormattedString("Force to Selected");
        }
        break;
      case MBFM_SCREENMODE_MIDI_OUT:
        
        break;
      }
    }
    break;
  case MBFM_MAINMODE_SEQ:
    //TODO
    break;
  case MBFM_MAINMODE_WTED:
    //TODO
    break;
  }
  MUTEX_LCD_GIVE;
}


void MBFM_Control_Init(){
  u8 i;
  for(i=0; i<6*OPL3_COUNT; i++){
    drumactivity[i] = 0;
  }
  for(i=0; i<18*OPL3_COUNT; i++){
    voicemisc[i].activity = 0;
  }
  for(i=0; i<16; i++){
    midiactivity[i] = 0;
  }
  return;
}
void MBFM_Control_Tick(u32 time){
  u8 i, j;
  //
  //Every 8 frames, check for indicators to turn on
  //
  if((time & 7) == 0){
    //MIDI indicators, on only
    if(mainmode == MBFM_MAINMODE_MIDI && holdmode == MBFM_HOLD_NONE){
      for(i=0; i<16; i++){
        if(midiactivity[i] == 1){
          MBNG_EVENT_ReceiveMBFM(MBNG_EVENT_MBFM_TYPE_VOICELEDRED, i, 1);
          midiactivity[i] = 2;
        }
      }
    }
    if(holdmode != MBFM_HOLD_MUTE && holdmode != MBFM_HOLD_SOLO){
      //Drums, on/off
      for(i=0; i<6*OPL3_COUNT; i++){
        if(drumactivity[i] == 1){
          MBNG_EVENT_ReceiveMBFM(MBNG_EVENT_MBFM_TYPE_SEQTRIGGERLED, i, 1);
          drumactivity[i] = 2;
        }else if(drumactivity[i] == 0){
          MBNG_EVENT_ReceiveMBFM(MBNG_EVENT_MBFM_TYPE_SEQTRIGGERLED, i, 0);
          drumactivity[i] = -1;
        }
      }
    }
    //Voices, on/off
    if(mainmode == MBFM_MAINMODE_FM && holdmode == MBFM_HOLD_NONE){
      for(i=0; i<18; i++){
        j = i+(18*act_opl3);
        if(voicemisc[j].activity == 1){
          MBNG_EVENT_ReceiveMBFM(MBNG_EVENT_MBFM_TYPE_VOICELEDRED, i, 1);
          voicemisc[j].activity = 2;
        }else if(voicemisc[j].activity == 0){
          MBNG_EVENT_ReceiveMBFM(MBNG_EVENT_MBFM_TYPE_VOICELEDRED, i, 0);
          voicemisc[j].activity = -1;
        }
      }
    }
    //Redrawing screen for MIDI mixer
    if(screenmode == MBFM_SCREENMODE_MIDI_MIXER && ctlrefreshvolume){
      MBFM_DrawScreen();
      ctlrefreshvolume = 0;
    }
  }
  //
  //Every 128 frames, turn all MIDI indicators off
  //
  if((time & 127) == 14){
    if(mainmode == MBFM_MAINMODE_MIDI && holdmode == MBFM_HOLD_NONE){
      for(i=0; i<16; i++){
        if(midiactivity[i] > 0){
          MBNG_EVENT_ReceiveMBFM(MBNG_EVENT_MBFM_TYPE_VOICELEDRED, i, 0);
          midiactivity[i] = -1;
        }
      }
    }
  }
  //
  //Every 1024 frames
  //
  /*
  if((time & 1023) == 57){
    
  }
  */
}


char* NoteName(u8 midinote){
  switch(midinote % 12){
  case 0: return "C "; 
  case 1: return "C#"; 
  case 2: return "D "; 
  case 3: return "Eb"; 
  case 4: return "E "; 
  case 5: return "F "; 
  case 6: return "F#"; 
  case 7: return "G "; 
  case 8: return "G#"; 
  case 9: return "A "; 
  case 10: return "Bb"; 
  default: return "B "; 
  }
}

char* MIDIPortName(u8 midiport){
  switch(midiport){
  case 0: return "UART0";
  case 1: return "UART1";
  case 2: return "UART2";
  case 3: return "UART3";
  case 4: return "USB";
  default: return "Other";
  }
}
