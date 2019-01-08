// $Id: mbfm.c $
/*
 * MBHP_MBFM MIDIbox FM V2.0 synth engine
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
#include "mbfm.h"

#include <opl3.h>
#include "mbfm_modulation.h"
#include "mbfm_sequencer.h"
#include "mbfm_controlhandler.h"
#include "mbfm_patch.h"
#include "mbfm_temperament.h"

//Global variables
s8 AHB_SECTION midichanmapping[18*OPL3_COUNT];
s8 AHB_SECTION midifirstvoice[16];
u8 AHB_SECTION mididrummapping[6*OPL3_COUNT];

dupllink_t AHB_SECTION voicedupl[18*OPL3_COUNT];
dupllink_t AHB_SECTION voicelink[18*OPL3_COUNT];

notevel_t AHB_SECTION voicemidistate[18*OPL3_COUNT];
u8 AHB_SECTION voicectr[16];
notevel_t AHB_SECTION channotestacks[16][MBFM_NOTESTACK_LEN];
notevel_t AHB_SECTION voiceactualnotes[18*OPL3_COUNT];
midivol_t AHB_SECTION midivol[16];
midivol_t AHB_SECTION midiexpr[16];

midiportinopts_t AHB_SECTION midiportinopts[6];
midiportoutopts_t AHB_SECTION midiportoutopts[6];

u8 AHB_SECTION chansustainpedals[16];
chanoptions_t AHB_SECTION chanoptions[16];

s8 AHB_SECTION drumactivity[6*OPL3_COUNT];
voicemisc_t AHB_SECTION voicemisc[18*OPL3_COUNT];
s8 AHB_SECTION midiactivity[16];


/////////////////////////////////////////////////////////////////////////////
// Functions
/////////////////////////////////////////////////////////////////////////////

void MBFM_Init(){
  u8 i;
  //OPL3 driver
  OPL3_Init();
  //Global OPL3 variables
  for(i=0; i<OPL3_COUNT; i++){
    OPL3_SetOpl3Mode(i, 1);
    OPL3_SetNoteSel(i, 1);
    OPL3_SetCSW(i, 0);
    OPL3_SetVibratoDepth(i, 1);
    OPL3_SetTremeloDepth(i, 1);
    OPL3_SetPercussionMode(i, 0); //TODO change to 1 once I get the percussion working
  }
  //Components
  MBFM_SEQ_Init();
  MBFM_Modulation_Init();
  MBFM_Control_Init();
  MBFM_Patch_Init();
  Temperament_Init();
  //mbfm.c component
  voicelink[0].voice = -1;
  voicelink[0].follow = 1;
  voicedupl[0].voice = -1;
  voicedupl[0].follow = 1;
  for(i=1; i<18*OPL3_COUNT; i++){
    voicelink[i].voice = -1;
    voicelink[i].follow = 1;
    voicedupl[i].voice = 0; //On startup, all dupl to voice 0
    voicedupl[i].follow = 1;
  }
  for(i=0; i<16; i++){
    chanoptions[i].autodupl = 1;
    chanoptions[i].autoload = 1;
    chanoptions[i].cc_bmsb = 0;
    chanoptions[i].cc_blsb = 0;
    chanoptions[i].cc_prog = 1;
    chanoptions[i].cc_pan  = 1;
    chanoptions[i].cc_other= 1;
    midifirstvoice[i] = -1;
    midivol[i].level = 127;
    midivol[i].enable = 1;
    midiexpr[i].level = 127;
    midiexpr[i].enable = 1;
  }
  for(i=0; i<6; i++){
    midiportinopts[i].mode = 1;
    midiportinopts[i].chan = 0;
    midiportoutopts[i].thruport = 0;
    midiportoutopts[i].fpcc = 1;
    midiportoutopts[i].seq = 1;
  }
  midifirstvoice[0] = 0;
  chanoptions[9].autoload = 0;
  //Finally
  OPL3_RefreshAll();
}
void MBFM_InitAfterFrontPanel(){
  mainmode = MBFM_MAINMODE_MIDI;
  MBFM_SelectMode(MBFM_MAINMODE_FM, 1);
}
void MBFM_1msTick(u32 time){
  
}
void MBFM_BackgroundTick(u32 time){
  MBFM_Control_Tick(time);
  MBFM_Patch_Tick(time);
  MBFM_SEQ_Tick(time);
  MBFM_Modulation_Tick(time); //includes OPL3_OnFrame();
  MBFM_SEQ_PostTick(time);
}

//Note == 0 finds voices off
//Return < 0 means none found
s8 FindVoicePlayingNote(u8 chan, u8 note, u8 toreplace){
  if(chan >= 16) return -1;
  u8 i, vcount = voicectr[chan];
  if(!note && toreplace){
    //Do a first scan to see if there's a voice whose last playing note was toreplace
    for(i=0; i<18*OPL3_COUNT; i++){
      if(midichanmapping[vcount] == chan){
        //On this channel
        if(voicelink[vcount].voice < 0){
          //Not linked
          if(!voicemidistate[vcount].velocity && voicemidistate[vcount].note == toreplace){
            //Playing this note
            if(!voicemisc[vcount].sustain){
              //Not turned-off-but-being-held-by-pedal
              return vcount;
            }
          }
        }
      }
      vcount++;
      if(vcount >= 18*OPL3_COUNT) vcount = 0;
    }
  }
  vcount = voicectr[chan];
  //Normal scan
  for(i=0; i<18*OPL3_COUNT; i++){
    if(midichanmapping[vcount] == chan){
      //On this channel
      if(voicelink[vcount].voice < 0){
        //Not linked
        if((!note && !voicemidistate[vcount].velocity) || 
           (note && voicemidistate[vcount].velocity && voicemidistate[vcount].note == note)){
          //Playing this note
          if(!voicemisc[vcount].sustain){
            //Not turned-off-but-being-held-by-pedal
            return vcount;
          }
        }
      }
    }
    vcount++;
    if(vcount >= 18*OPL3_COUNT) vcount = 0;
  }
  //voicectr[chan] = vcount; //No need, should already be the same
  return -1;
}

void MBFM_SetVoiceAndLinkedNote(u8 voice, u8 note, u8 velocity){
  if(voice >= 18*OPL3_COUNT) return;
  if(note == 0){
    voicemidistate[voice].velocity = 0;
  }else{
    if(note == voicemidistate[voice].note){
      voicemidistate[voice].retrig = 1;
    }
    voicemidistate[voice].note = note;
    voicemidistate[voice].velocity = velocity;
  }
  voicemisc[voice].sustain = 0;
  if(voicemisc[voice].activity > 0){
    if(!note) voicemisc[voice].activity = 0;
  }else{
    if(note) voicemisc[voice].activity = 1;
  }
  u8 i;
  //Set all linked voices
  for(i=0; i<18*OPL3_COUNT; i++){
    if(voicelink[i].voice == voice){
      if(note == 0){
        voicemidistate[i].velocity = 0;
      }else{
        if(note == voicemidistate[i].note){
          voicemidistate[i].retrig = 1;
        }
        voicemidistate[i].note = note;
        voicemidistate[i].velocity = velocity;
      }
      voicemisc[i].sustain = 0;
      if(voicemisc[voice].activity > 0){
        if(!note) voicemisc[voice].activity = 0;
      }else{
        if(note) voicemisc[voice].activity = 1;
      }
    }
  }
}

void MBFM_ReceiveMIDIMessage(mios32_midi_port_t port, mios32_midi_package_t midi_package){
  u8 type = midi_package.type, velocity = midi_package.velocity, orignote = midi_package.note, writenote;
  u8 i, j, chn = midi_package.chn, ccvalue = midi_package.value;
  s8 targetvoice;
  if(port >= UART0 && port <= UART3){
    i = port - UART0;
  }else if(port >= USB0 && port <= USB7){
    i = 4;
  }else{
    i = 5;
  }
  j = midiportinopts[i].mode;
  if(!j) return;
  if(j == 2){
    chn = midiportinopts[i].chan;
  }else if(j == 3){
    chn = act_midich;
  }
  if(chn >= 16) return;
  if(midiactivity[chn] <= 0) midiactivity[chn] = 1;
  if(chn == 9 && (type == 0x8 || type == 0x9)){
    //Drums only
    for(i=0; i<6*OPL3_COUNT; i++){
      if(mididrummapping[i] == orignote){
        //DEBUG_MSG("Triggered drum %d vel %d", i, velocity);
        MBFM_Drum_ReceiveMIDITrigger(i, (type == 0x8) ? 0 : velocity);
        return;
      }
    }
    return;
  }
  if(type == 0x8 || (type == 0x9 && velocity == 0)){
    //////////////////////////////////////////Note Off/////////////////////////////////////////
    targetvoice = FindVoicePlayingNote(chn, orignote, 0);
    if(targetvoice >= 18*OPL3_COUNT) return;
    if(targetvoice < 0){
      //Look in notestack
      for(i=0; i<MBFM_NOTESTACK_LEN; i++){
        if(channotestacks[chn][i].note == orignote){
          //It doesn't matter whether sustain pedal is held or not, remove it
          //Shift down the stack
          channotestacks[chn][i].note = 0;
          channotestacks[chn][i].velocity = 0;
          //DEBUG_MSG("Removed note from notestack pos %d", i);
          for(; i<MBFM_NOTESTACK_LEN - 1; i++){
            channotestacks[chn][i].note = channotestacks[chn][i+1].note;
            channotestacks[chn][i].velocity = channotestacks[chn][i+1].velocity;
            if(channotestacks[chn][i].note == 0) return; //Top
          }
          channotestacks[chn][i].note = 0;
          channotestacks[chn][i].velocity = 0;
          return;
        }
      }
      //DEBUG_MSG("Rec'd note off %d, but could not find it playing or notestacked", orignote);
      return;
    }else{
      //Note off for note being played in voice
      if(chansustainpedals[chn]){
        //Being held down by pedal
        voicemisc[targetvoice].sustain = 1;
        return;
      }
      //See if there's something from the stack that should be put in
      if(channotestacks[chn][0].note){
        //Use note from stack instead of note-off
        writenote = channotestacks[chn][0].note;
        velocity = channotestacks[chn][0].velocity;
        //Shift down the stack
        for(i=0; i<MBFM_NOTESTACK_LEN - 1; i++){
          channotestacks[chn][i].note = channotestacks[chn][i+1].note;
          channotestacks[chn][i].velocity = channotestacks[chn][i+1].velocity;
          if(channotestacks[chn][i].note == 0) break; //Top
        }
        channotestacks[chn][i].note = 0;
        channotestacks[chn][i].velocity = 0;
      }else{
        //Nothing on stack
        writenote = 0;
        velocity = 0;
      }
      //DEBUG_MSG("Put note %d in voice %d", writenote, targetvoice);
      MBFM_SetVoiceAndLinkedNote(targetvoice, writenote, velocity);
      return;
    }
  }else if(type == 0x9){
    //////////////////////////////////////////Note On/////////////////////////////////////////
    //See if there's a voice playing this note already
    for(i=0; i<18*OPL3_COUNT; i++){
      if(midichanmapping[i] == chn){
        if(voicelink[i].voice < 0){
          //Not linked
          if(voicemidistate[i].velocity && voicemidistate[i].note == orignote){
            //Playing this note
            #ifdef MBFM_ALLOW_MULTIPLE_SAME_NOTES
            if(voicemisc[i].sustain){ //Only if the note is still playing due to sus pedal
            #endif
              //Retrigger note
              //DEBUG_MSG("Ch %d, note on %d, found voice %d playing note already, retriggering", chn, orignote, i);
              MBFM_SetVoiceAndLinkedNote(i, orignote, velocity);
              return;
            #ifdef MBFM_ALLOW_MULTIPLE_SAME_NOTES
            }
            #endif
          }
        }
      }
    }
    //See if there's a notestack entry playing this note already
    for(i=0; i<MBFM_NOTESTACK_LEN; ){
      if(!channotestacks[chn][i].note) break; //Off top
      if(channotestacks[chn][i].note == orignote){
        //Playing this note
        //Shift down the stack into it
        //DEBUG_MSG("Ch %d, note on %d, found notestack %d playing note already, removing", chn, orignote, i);
        for(j=i; j<MBFM_NOTESTACK_LEN-1; j++){
          channotestacks[chn][j].note = channotestacks[chn][j+1].note;
          channotestacks[chn][j].velocity = channotestacks[chn][j+1].velocity;
          if(channotestacks[chn][j].note == 0) break; //Top
        }
        channotestacks[chn][j].note = 0;
        channotestacks[chn][j].velocity = 0;
        break;
      }else{
        i++;
      }
    }
    //Find voice that's off
    targetvoice = FindVoicePlayingNote(chn, 0, orignote);
    if(targetvoice < 0){
      //DEBUG_MSG("Ch %d, no empty voices found to put note %d in", chn, orignote);
      //Shove a note playing into the notestack
      //Find the next playing voice
      targetvoice = voicectr[chn];
      for(i=0; i<18*OPL3_COUNT; i++){
        if(midichanmapping[targetvoice] == chn){
          voicectr[chn] = targetvoice;
          break;
        }
        targetvoice++;
        if(targetvoice >= 18*OPL3_COUNT) targetvoice = 0;
      }
      if(midichanmapping[targetvoice] != chn){
        //DEBUG_MSG("No voices assigned to channel %d!", chn);
        return;
      }
      //Only put the note in the notestack if it's not being held with pedal--otherwise, drop it
      //TODO better method would be to do two scans, one for sustained voices and one for all voices,
      //so that sustained voices would be dropped before manually-held voices
      if(!voicemisc[targetvoice].sustain){
        //Find the top of the notestack
        for(i=0; i<MBFM_NOTESTACK_LEN; i++){
          if(channotestacks[chn][i].note == 0){
            break;
          }
        }
        //Is notestack full?
        if(i >= MBFM_NOTESTACK_LEN){
          i = MBFM_NOTESTACK_LEN - 1; //TODO drop first not last note?
          //DEBUG_MSG("Notestack ch %d full, dropping notestacked note %d", chn, channotestacks[chn][i].note);
        }
        //Move playing note to here
        channotestacks[chn][i].all = voicemidistate[targetvoice].all;
      }
      //Put new note here
      MBFM_SetVoiceAndLinkedNote(targetvoice, orignote, velocity);
      //Find next place to put voicectr
      for(i=0; i<18*OPL3_COUNT; i++){
        targetvoice++;
        if(targetvoice >= 18*OPL3_COUNT) targetvoice = 0;
        if(midichanmapping[targetvoice] == chn){
          voicectr[chn] = targetvoice;
          break;
        }
      }
      return;
    }else{
      //Voice available
      //DEBUG_MSG("Put note %d in voice %d", orignote, targetvoice);
      MBFM_SetVoiceAndLinkedNote(targetvoice, orignote, velocity);
      //Find the next place to put voicectr
      for(i=0; i<18*OPL3_COUNT; i++){
        targetvoice++;
        if(targetvoice >= 18*OPL3_COUNT) targetvoice = 0;
        if(midichanmapping[targetvoice] == chn){
          voicectr[chn] = targetvoice;
          break;
        }
      }
      return;
    }
  }else if(type == 0xB){
    //Control change
    switch(midi_package.cc_number){
    case 0:
      //Bank MSB
      if(chanoptions[chn].cc_bmsb){
        if(!prgstate[chn].command){
          midibankmsb[chn] = ccvalue;
        }else{
          //DEBUG_MSG("Skipping received Bank MSB %d chn %d because waiting to %s",
          //      ccvalue, chn, (prgstate[chn].command == 1) ? "load" : "save");
        }
      }
      break;
    case 32:
      //Bank LSB
      if(chanoptions[chn].cc_blsb){
        if(!prgstate[chn].command){
          midibanklsb[chn] = ccvalue;
        }else{
          //DEBUG_MSG("Skipping received Bank LSB %d chn %d because waiting to %s",
          //      ccvalue, chn, (prgstate[chn].command == 1) ? "load" : "save");
        }
      }
      break;
    case 1:
      //Mod Wheel
      //DEBUG_MSG("Got Modwheel ch %d value %d", chn, ccvalue);
      //Find all voices
      for(i=0; i<18*OPL3_COUNT; i++){
        if(midichanmapping[i] == chn){
          modl_mods[i] = ccvalue;
        }
      }
      break;
    case 64:
      //Sustain pedal
      //DEBUG_MSG("Got sustain pedal ch %d value %d", chn, ccvalue);
      j = (ccvalue >= 64);
      chansustainpedals[chn] = j;
      if(!j){
        MBFM_LetGoSustainPedal(chn);
      }
      break;
    case 7:
      //Volume
      midivol[chn].level = ccvalue;
      for(i=0; i<18*OPL3_COUNT; i++){
        if(midichanmapping[i] == chn ||
            (voicedupl[i].voice >= 0 && midichanmapping[voicedupl[i].voice] == chn) ||
            (voicelink[i].voice >= 0 && midichanmapping[voicelink[i].voice] == chn)){
          voicemisc[i].refreshvol = 1;
        }
      }
      ctlrefreshvolume = 1;
      break;
    case 11:
      //Expression
      midiexpr[chn].level = ccvalue;
      for(i=0; i<18*OPL3_COUNT; i++){
        if(midichanmapping[i] == chn ||
            (voicedupl[i].voice >= 0 && midichanmapping[voicedupl[i].voice] == chn) ||
            (voicelink[i].voice >= 0 && midichanmapping[voicelink[i].voice] == chn)){
          voicemisc[i].refreshvol = 1;
        }
      }
      ctlrefreshvolume = 1;
      break;
    case 10:
      //Pan
      if(chanoptions[chn].cc_pan){
        for(i=0; i<18*OPL3_COUNT; i++){
          if(midichanmapping[i] == chn){
            //DEBUG_MSG("Setting pan chn %d value %d", chn, ccvalue);
            j = voiceparams[i].dest;
            j &= 12;
            if(ccvalue < 32){
              j++;
            }else if(ccvalue >= 96){
              j += 2;
            }else{
              j += 3;
            }
            voiceparams[i].dest = j;
            voiceactualnotes[i].update = 1;
          }
        }
      }
      break;
    case 120:
      //All Sound Off
    case 123:
      //All Notes Off
      //DEBUG_MSG("Got AllNotesOff ch %d value %d", chn, ccvalue);
      MBFM_AllNotesOff(chn);
      break;
    default:
      MBFM_ControlCC(chn, midi_package.cc_number, ccvalue);
    }
  }else if(type == 0xC){
    //Program change
    //DEBUG_MSG("Program change chn %d value %d, BMSB%d BLSB%d", chn, midi_package.program_change, midibankmsb[chn], midibanklsb[chn]);
    u8 newprog = midi_package.program_change;
    if(chanoptions[chn].cc_prog){
      if(midiprogram[chn] != newprog){
        if(!prgstate[chn].command){
          midiprogram[chn] = newprog;
          //If desired, load patch immediately
          if(chanoptions[chn].autoload){
            prgstate[chn].command = 1;
            //DEBUG_MSG("MIDI: Command to load ch %d BMSB%d BLSB%d Prog %d",
            //      chn, midibankmsb[chn], midibanklsb[chn], newprog);
          }
        }else{
          //DEBUG_MSG("Skipping received Prog Change %d chn %d because waiting to %s", 
          //      newprog, chn, (prgstate[chn].command == 1) ? "load" : "save");
        }
      }else{
        //DEBUG_MSG("Skipping received Prog Change %d chn %d because already that program", newprog, chn);
      }
    }
  }else if(type == 0xE){
    //Pitch bend
    //DEBUG_MSG("Got pitch bend ch %d value %d", chn, ccvalue1);
    //Find all voices
    for(i=0; i<18*OPL3_COUNT; i++){
      if(midichanmapping[i] == chn ||
            (voicelink[i].voice >= 0 && midichanmapping[voicelink[i].voice] == chn)){
        pitchbends[i] = (s8)((s16)ccvalue - 64);
        voiceactualnotes[i].update = 1;
      }
    }
  }else{
    //All other MIDI message types handled through MBNG
    return;
  }
}

void MBFM_LetGoSustainPedal(u8 channel){
  u8 voice, j, writenote, velocity;
  //Turn off all the voices that had been let up after the pedal was pressed
  for(voice=0; voice<18*OPL3_COUNT; voice++){
    if(midichanmapping[voice] == channel){
      if(voicemisc[voice].sustain){
        voicemisc[voice].sustain = 0;
        writenote = 0;
        velocity = 0;
        //See if there's anything from the stack that should go in here instead of 0
        for(j=0; j<MBFM_NOTESTACK_LEN; j++){
          if(!channotestacks[channel][j].note) break; //Off the end
          //A note that is not due to be released, use it
          writenote = channotestacks[channel][j].note;
          velocity = channotestacks[channel][j].velocity;
          //Shift down the stack
          for(; j<MBFM_NOTESTACK_LEN - 1; j++){
            channotestacks[channel][j].note = channotestacks[channel][j+1].note;
            channotestacks[channel][j].velocity = channotestacks[channel][j+1].velocity;
            if(channotestacks[channel][j].note == 0) break; //Top
          }
          channotestacks[channel][j].note = 0;
          break;
        }
        //Write note
        MBFM_SetVoiceAndLinkedNote(voice, writenote, velocity);
      }//endif .sustain
    }
  }
}

u8 MBFM_GetNumVoicesInChannel(u8 channel, u8 ops){
  u8 count=0, i, fourop;
  for(i=0; i<18*OPL3_COUNT; i++){
    if(midichanmapping[i] == channel){
      if(ops == 0){
        count++;
      }else{
        fourop = OPL3_IsChannel4Op(i);
        if((fourop == 1 && ops == 4) || (fourop == 0 && ops == 2)){
          count++;
        }
      }
    }
  }
  return count;
}

void MBFM_AllNotesOff(u8 channel){
  u8 i;
  for(i=0; i<18*OPL3_COUNT; i++){
    if(midichanmapping[i] == channel){
      MBFM_SetVoiceAndLinkedNote(i, 0, 0);
      voicemisc[i].activity = 0;
      pitchbends[i] = 0;
    }
  }
  for(i=0; i<MBFM_NOTESTACK_LEN; i++){
    channotestacks[channel][i].all = 0;
  }
  midiactivity[i] = 0;
  midivol[i].level = 127;
  midiexpr[i].level = 127;
  if(channel == 9){
    for(i=0; i<6*OPL3_COUNT; i++){
      MBFM_Drum_ReceiveMIDITrigger(i, 0);
    }
  }
}

s8 MBFM_CopyVoice(u8 srcvoice, u8 destvoice){
  if(srcvoice >= 18*OPL3_COUNT || destvoice >= 18*OPL3_COUNT) return -1;
  //DEBUG_MSG("Copying voice %d into %d", srcvoice, destvoice);
  u8 i, op, numops, a, b;
  //Set up 4-op things
  numops = OPL3_IsChannel4Op(srcvoice);
  if(numops != OPL3_IsChannel4Op(destvoice)) return -1;
  if(numops == 2) return -1;
  numops = numops ? 4 : 2;
  //Clear things
  voicemidistate[destvoice].note = 0;
  voicemidistate[destvoice].velocity = 0;
  voicemisc[destvoice].sustain = 0;
  for(i=0; i<MBFM_DLY_STEPS; i++){
    delaytape[destvoice][i].all = 0;
  }
  delayrechead[destvoice] = 0;
  portastartnote[destvoice] = 0;
  portastarttime[destvoice] = 0;
  egstarttime[destvoice] = 0;
  egmode[destvoice] = 1;
  egrelstartval[destvoice] = 0;
  lfostarttime[destvoice] = 0;
  lforandval[destvoice][0] = 0;
  lforandval[destvoice][1] = 0;
  lforandstate[destvoice] = 0;
  //Copy things
  for(op=0; op<numops; op++){
    a = (srcvoice*2)+op;
    b = (destvoice*2)+op;
    pre_opparams[b].data1 = pre_opparams[a].data1;
    pre_opparams[b].data2 = pre_opparams[a].data2;
  }
  a = srcvoice;
  b = destvoice;
  for(op=0; op<numops; op+=2){
    pre_voiceparams[b].data1 = pre_voiceparams[a].data1;
    pre_voiceparams[b].data2 = pre_voiceparams[a].data2;
    pre_voiceparams[b].data3 = pre_voiceparams[a].data3;
    voiceparams[b].data1 = voiceparams[a].data1;
    voiceparams[b].data2 = voiceparams[a].data2;
    voiceparams[b].data3 = voiceparams[a].data3;
    for(i=0; i<13; i++){
      pre_modlparams[b].data[i] = pre_modlparams[a].data[i];
      modlparams[b].data[i] = modlparams[a].data[i];
    }
    for(i=0; i<MBFM_MODL_NUMCONN; i++){
      modllists[b][i].src = modllists[a][i].src;
      modllists[b][i].dest = modllists[a][i].dest;
      modllists[b][i].pre_depth = modllists[a][i].pre_depth;
    }
    for(i=0; i<MBFM_WT_LEN; i++){
      wavetable[b][i] = wavetable[a][i];
    }
    a++;
    b++;
  }
  modl_mods[destvoice] = modl_mods[srcvoice];
  modl_varis[destvoice] = modl_varis[srcvoice];
  pitchbends[destvoice] = pitchbends[srcvoice];
  return 0;
}


void MBFM_AssignVoiceToChannel(u8 voice, u8 chan){
  u8 j, f;
  if(midichanmapping[voice] != chan){
    //=========================New connection to this channel==================
    //DEBUG_MSG("New voice %d connecting to chan %d", voice, chan);
    midichanmapping[voice] = chan;
    //-------------------------First voice, dupl/copy/load
    if(midifirstvoice[chan] < 0){
      //This is the first voice
      midifirstvoice[chan] = voice;
      if(chanoptions[chan].autodupl){
        voicedupl[voice].voice = -1;
        voicelink[voice].voice = -1;
        if(chanoptions[chan].autoload){ //Both autodupl and autoload active
          //Load patch
          if(!prgstate[chan].command){
            prgstate[chan].command = 1;
          }
        }
      }
    }else{
      f = midifirstvoice[chan];
      //Copy voice if autodupl
      if(chanoptions[chan].autodupl){
        voicelink[voice].voice = -1;
        if(OPL3_IsChannel4Op(voice) == OPL3_IsChannel4Op(f)){
          voicedupl[voice].voice = f;
          voicedupl[voice].follow = 1;
          //Make copy
          MBFM_CopyVoice(f, voice);
        }else{
          voicedupl[voice].voice = -1;
        }
      }
      //Set this as firstvoice if applicable
      if(midifirstvoice[chan] > voice){
        //There was a first voice, but this is lower
        midifirstvoice[chan] = voice;
        if(chanoptions[chan].autodupl){
          //Set all voices to be dupl'd to this new one instead of the old one
          for(j=0; j<18*OPL3_COUNT; j++){
            if(voicedupl[j].voice == f){
              voicedupl[j].voice = voice;
            }
          }
          voicedupl[f].voice = voice;
          voicedupl[voice].voice = -1;
        }
      }
    }
    //-------------------------Determine note to fill in
    u8 writenote, velocity;
    //If there's something on the notestack, put it in
    if(channotestacks[chan][0].note){
      //Use note from stack instead of note-off
      writenote = channotestacks[chan][0].note;
      velocity = channotestacks[chan][0].velocity;
      //Shift down the stack
      for(j=0; j<MBFM_NOTESTACK_LEN - 1; j++){
        channotestacks[chan][j].note = channotestacks[chan][j+1].note;
        channotestacks[chan][j].velocity = channotestacks[chan][j+1].velocity;
        if(channotestacks[chan][j].note == 0) break; //Top
      }
      channotestacks[chan][j].note = 0;
      channotestacks[chan][j].velocity = 0;
    }else{
      //Nothing on stack
      writenote = 0;
      velocity = 0;
    }
    MBFM_SetVoiceAndLinkedNote(voice, writenote, velocity);
  }else{
    //=========================Removing channel connection=====================
    //DEBUG_MSG("Removing voice %d from chan %d", voice, chan);
    midichanmapping[voice] = -1;
    //-------------------------First voice
    if(midifirstvoice[chan] == voice){
      midifirstvoice[chan] = -1;
      //Find new first voice
      for(j=0; j<18*OPL3_COUNT; j++){
        if(midichanmapping[j] == chan){
          midifirstvoice[chan] = j;
          break;
        }
      }
      f = midifirstvoice[chan];
      //If there is one, and autodupl,
      if(f >= 0 && chanoptions[chan].autodupl){
        //Set all voices to be dupl'd to this new one instead of the old one
        for(j=0; j<18*OPL3_COUNT; j++){
          if(voicedupl[j].voice == voice){
            voicedupl[j].voice = f;
          }
        }
        voicedupl[f].voice = -1;
      }
    }
    //-------------------------Last voice
    if(MBFM_GetNumVoicesInChannel(chan, 2) + MBFM_GetNumVoicesInChannel(chan, 4) == 0){
      MBFM_AllNotesOff(chan);
    }
    voicemidistate[voice].note = 0;
    voicemidistate[voice].velocity = 0;
    voicemisc[voice].sustain = 0;
  }
}
