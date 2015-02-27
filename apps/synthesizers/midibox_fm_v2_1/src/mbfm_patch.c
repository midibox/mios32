// $Id: mbfm_patch.c $
/*
 * MIDIbox FM V2.0 patch file loading/saving
 *
 * ==========================================================================
 *
 *  Copyright (C) 2014 Sauraen (sauraen@gmail.com)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 *
 *  Copyright (C) 2012 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

/////////////////////////////////////////////////////////////////////////////
// Include files
/////////////////////////////////////////////////////////////////////////////

#include <mios32.h>
#include "mbfm_patch.h"

#include <string.h>
#include <uip_task.h>
#include "tasks.h"
#include "file.h"
#include <opl3.h>

#include "mbfm.h"
#include "mbfm_modulation.h"
#include "mbfm_sequencer.h"
#include "mbfm_controlhandler.h"
#include "mbng_file.h"
#include "mbng_file_c.h"


#define MBFM_PATCHFILE_LINEBUFFERSIZE 128

//va stands for voice allocation
static u8 va_chan;
static s8 va_voice;
static u8 va_count;
static u8 va_fourop;
static u8 va_drum;
static char AHB_SECTION mbfm_line_buffer[MBFM_PATCHFILE_LINEBUFFERSIZE];
char AHB_SECTION patchnames[16][MBFM_PATCHNAME_LEN];
char AHB_SECTION drumnames[2*OPL3_COUNT][MBFM_PATCHNAME_LEN];
u8 AHB_SECTION midibankmsb[16];
u8 AHB_SECTION midibanklsb[16];
u8 AHB_SECTION midiprogram[16];
prgstate_t AHB_SECTION prgstate[16];
char AHB_SECTION loadpatchname[MBFM_PATCHNAME_LEN];
u8 patchloadquery;
u32 lastdatawheelchange;

s32 va_init(u8 chan, u8 fourop, u8 count, u8 patchvoice){
  u8 i, pvc = 0;
  va_chan = chan;
  va_fourop = fourop;
  va_count = count;
  if(va_drum){
    va_voice = va_drum;
    return 0;
  }
  for(i=0; i<18*OPL3_COUNT; i++){
    if(midichanmapping[i] == chan && OPL3_IsChannel4Op(i) == fourop){
      if(patchvoice == pvc){
        va_voice = i;
        return 0;
      }
      pvc++;
    }
  }
  va_voice = -1;
  return -1;
}
s32 va_next(){
  u8 i, old_va_voice = va_voice, pvc = 1;
  if(va_voice < 0) return -1;
  if(va_count <= 0) return -1;
  if(va_drum){
    i = va_voice % 18;
    switch(i){
    case 15:
    case 17:
      va_voice = -1;
      return old_va_voice;
    case 16:
      va_voice++;
      return old_va_voice;
    }
  }else{
    for(i=va_voice+1; i<18*OPL3_COUNT; i++){
      if(midichanmapping[i] == va_chan && OPL3_IsChannel4Op(i) == va_fourop){
        if(pvc == va_count){
          va_voice = i;
          return old_va_voice;
        }
        pvc++;
      }
    }
  }//If it is a drum, it's guaranteed to fail next time
  va_voice = -1;
  return old_va_voice;
}



/////////////////////////////////////////////////////////////////////////////
// Functions
/////////////////////////////////////////////////////////////////////////////

s32 MBFM_PatchFile_GetName(u8 bankmsb, u8 banklsb, u8 program){
  s32 status;
  file_t file;
  u8 i, j;
  loadpatchname[0] = ' ';
  for(i=0; i<MBFM_PATCHNAME_LEN-1; i++){
    loadpatchname[i] = 0;
  }
  
  MUTEX_SDCARD_TAKE;
  
  sprintf(mbfm_line_buffer, "%s/BMSB%d/BLSB%d/Pat%d.fm", MBFM_FILES_PATH, bankmsb, banklsb, program);
  
  if( (status=FILE_ReadOpen(&file, mbfm_line_buffer)) < 0 ) {
    DEBUG_MSG("[MBFM_PatchFile_GetName] failed to open file %s for reading, status: %d\n", mbfm_line_buffer, status);
    FILE_ReadClose(&file);
    MUTEX_SDCARD_GIVE;
    return status;
  }

  char *parameter;
  char *valuestr;
  char *garbage;
  char c;

  // read patch values
  do {
    status=FILE_ReadLine((u8 *)(mbfm_line_buffer), MBFM_PATCHFILE_LINEBUFFERSIZE);
    if( status >= 1 ) {
      if(mbfm_line_buffer[0] == 0 || mbfm_line_buffer[0] == '#'){
        //ignore line, do nothing
      }else{
        parameter = strtok_r(mbfm_line_buffer, "=", &garbage);
        if(parameter == NULL){
          //DEBUG_MSG("[MBFM_PatchFile_GetName] Error parsing line %d", line);
        }else{
          valuestr = strtok_r(NULL, "=", &garbage);
          //Parse parameter that gets a text value
          if(strcasecmp(parameter, "name") == 0){
            if(valuestr != NULL){
              j = 1;
              for(i=0; i<MBFM_PATCHNAME_LEN-1; i++){
                c = (j ? valuestr[i] : 0);
                loadpatchname[i] = c;
                if(c == 0) j = 0;
              }
            }
            loadpatchname[MBFM_PATCHNAME_LEN-1] = 0;
            DEBUG_MSG("[MBFM_PatchFile_GetName] Patch name %s", loadpatchname);
            break;
          }
        }
      }
    }

  } while( status >= 1 );
  
  // close file
  status |= FILE_ReadClose(&file);
  
  MUTEX_SDCARD_GIVE;

  if( status < 0 ) {
    DEBUG_MSG("[MBFM_PatchFile_GetName] ERROR while reading file, status: %d\n", status);
    return status;
  }
  
  DEBUG_MSG("[MBFM_PatchFile_GetName] Read file fine, status %d\n", status);
  
  return 0; // no error
          
}

s32 MBFM_PatchFile_Load(u8 chan, s8 drum, u8 bankmsb, u8 banklsb, u8 program){
  if(chan >= 16) return -1;
  if(drum >= 2*OPL3_COUNT) return -1;
  s32 status;
  file_t file;
  
  MUTEX_SDCARD_TAKE;
  
  sprintf(mbfm_line_buffer, "%s/BMSB%d/BLSB%d/Pat%d.fm", MBFM_FILES_PATH, bankmsb, banklsb, program);
  
  if( (status=FILE_ReadOpen(&file, mbfm_line_buffer)) < 0 ) {
    DEBUG_MSG("[MBFM_PatchFile_Load] failed to open file %s for reading, status: %d\n", mbfm_line_buffer, status);
    FILE_ReadClose(&file);
    MUTEX_SDCARD_GIVE;
    return status;
  }

  char *parameter;
  char *valuestr;
  char *garbage;
  char *numbers;
  char *nameptr;
  s8 voice2op = -1, voice4op = -1, num2op = 0, num4op = 0, op = -1, eg = -1, lfo = -1, wt = -1, modconn = -1, voice = -1;
  long value;
  u32 line = 0;
  char c;
  u8 i, j;
#define VA_INIT_MACRO {if(voice4op >= 0){ va_init(chan, 1, num4op, voice4op); }else{ va_init(chan, 0, num2op, voice2op); } }
  
  //Clear voices
  if(drum >= 0){
    i = (drum >> 1) * 18; //OPL3
    j = drum & 1;  //BD or rest
    if(j){
      MBFM_InitPercValues(i+16);
      MBFM_InitPercValues(i+17);
    }else{
      MBFM_InitPercValues(i+15);
    }
    va_drum = 15 + (18 * (drum >> 1)) + (drum & 1);
    voice2op = va_drum;
    num2op = 1;
    voice4op = -1;
    num4op = 0;
    nameptr = drumnames[drum];
  }else{
    //Normal voices
    for(voice=0; voice<18*OPL3_COUNT; voice++){
      if(midichanmapping[voice] == chan){
        MBFM_InitVoiceValues(voice);
      }
    }
    va_drum = 0;
    nameptr = patchnames[chan];
  }
  
  // read patch values
  do {
    ++line;
    status=FILE_ReadLine((u8 *)(mbfm_line_buffer), MBFM_PATCHFILE_LINEBUFFERSIZE);
    if( status >= 1 ) {
      if(mbfm_line_buffer[0] == 0 || mbfm_line_buffer[0] == '#'){
        //ignore line, do nothing
      }else{
        parameter = strtok_r(mbfm_line_buffer, "=", &garbage);
        if(parameter == NULL){
          DEBUG_MSG("[MBFM_PatchFile_Load] Error parsing line %d", line);
        }else{
          valuestr = strtok_r(NULL, "=", &garbage);
          //Parse parameter that gets a text value
          if(strcasecmp(parameter, "name") == 0){
            if(valuestr == NULL){
              nameptr[0] = ' ';
              for(i=1; i<MBFM_PATCHNAME_LEN-1; i++){
                nameptr[i] = 0;
              }
            }else{
              j = 1;
              for(i=0; i<MBFM_PATCHNAME_LEN-1; i++){
                c = (j ? valuestr[i] : 0);
                nameptr[i] = c;
                if(c == 0) j = 0;
              }
            }
            patchnames[chan][MBFM_PATCHNAME_LEN-1] = 0;
            DEBUG_MSG("[MBFM_PatchFile_Load] Patch name %s", nameptr);
          }else{
            if( valuestr == NULL ){
              DEBUG_MSG("[MBFM_PatchFile_Load] No value specified at line %d, %s=", line, parameter);
            }else{
              value = strtol(valuestr, &numbers, 0);
              if( valuestr == numbers ){
                DEBUG_MSG("[MBFM_PatchFile_Load] Invalid integer value %s at line %d", valuestr, line);
              }else{
                //DEBUG_MSG("Parsing numeric assignment %s=%d", parameter, value);
                //Parse parameter that gets a numeric value
                if(strcasecmp(parameter, "drum") == 0){
                  //Check that we're actually writing to a drum
                  if(drum < 0){
                    if(value){
                      DEBUG_MSG("[MBFM_PatchFile_Load] Trying to load 'drum=%d' to channel %d (line %d)", value, chan, line);
                      status = MBFM_PATCHLOADERR_DRUMLOADTOVOICE;
                      break;
                    }else{
                      //It just said drum=0, go on
                      va_drum = 0;
                    }
                  }else{
                    //Drums 1 BD, 2 SD+HH+TT+CY
                    if(value - 1 != (drum & 1)){
                      DEBUG_MSG("[MBFM_PatchFile_Load] Trying to load 'drum=%d' to given drum=%d (line %d)", value, drum, line);
                      status = MBFM_PATCHLOADERR_WRONGDRUM;
                      break;
                    }
                  }
                }else if(strcasecmp(parameter, "use2op") == 0){
                  if(value){
                    if(drum >= 0){
                      DEBUG_MSG("[MBFM_PatchFile_Load] Trying to load voice 'use2op=%d' to drum %d (line %d)", value, drum, line);
                      status = MBFM_PATCHLOADERR_VOICELOADTODRUM;
                      break;
                    }
                    i = MBFM_GetNumVoicesInChannel(chan, 2);
                    if(value > i){
                      DEBUG_MSG("[MBFM_PatchFile_Load] Need %d 2op voices ch %d but only have %d (line %d)", value, chan, i, line);
                      status = MBFM_PATCHLOADERR_INSUFFICIENT2OP;
                      break;
                    }
                    if(i % value != 0){
                      DEBUG_MSG("[MBFM_PatchFile_Load] Need %d 2op voices but ch %d has non-multiple: %d voices (line %d)", value, chan, i, line);
                      status = MBFM_PATCHLOADERR_NONMULTIPLE2OP;
                      break;
                    }
                    num2op = value;
                  }else{
                    if(drum < 0){
                      num2op = 0;
                    }
                  }
                }else if(strcasecmp(parameter, "use4op") == 0){
                  if(value){
                    if(drum >= 0){
                      DEBUG_MSG("[MBFM_PatchFile_Load] Trying to load voice 'use4op=%d' to drum %d (line %d)", value, drum, line);
                      status = MBFM_PATCHLOADERR_VOICELOADTODRUM;
                      break;
                    }
                    i = MBFM_GetNumVoicesInChannel(chan, 4);
                    if(value > i){
                      DEBUG_MSG("[MBFM_PatchFile_Load] Need %d 2op voices ch %d but only have %d (line %d)", value, chan, i, line);
                      status = MBFM_PATCHLOADERR_INSUFFICIENT4OP;
                      break;
                    }
                    if(i % value != 0){
                      DEBUG_MSG("[MBFM_PatchFile_Load] Need %d 2op voices but ch %d has non-multiple: %d voices (line %d)", value, chan, i, line);
                      status = MBFM_PATCHLOADERR_NONMULTIPLE4OP;
                      break;
                    }
                    num4op = value;
                  }else{
                    if(drum < 0){
                      num4op = 0;
                    }
                  }
                //==============================Categories==============================
                }else if(strcasecmp(parameter, "voice2op") == 0){
                  if(value < 0){
                    DEBUG_MSG("[MBFM_PatchFile_Load] voice2op should be zero-indexed! (line %d)", line);
                  }else{
                    if(value >= num2op){
                      DEBUG_MSG("[MBFM_PatchFile_Load] voice2op should be zero-indexed, value %d too high! (line %d)", value, line);
                    }else{
                      voice2op = value;
                      voice4op = -1;
                    }
                  }
                }else if(strcasecmp(parameter, "voice4op") == 0){
                  if(value < 0){
                    DEBUG_MSG("[MBFM_PatchFile_Load] voice4op should be zero-indexed! (line %d)", line);
                  }else{
                    if(value >= num4op){
                      DEBUG_MSG("[MBFM_PatchFile_Load] voice4op should be zero-indexed, value %d too high! (line %d)", value, line);
                    }else{
                      voice4op = value;
                      voice2op = -1;
                    }
                  }
                }else if(strcasecmp(parameter, "op") == 0){
                  if(value < 0 || value > 3 || (value > 1 && voice4op < 0)){
                    DEBUG_MSG("[MBFM_PatchFile_Load] Invalid op number at line %d (should be zero-indexed)", line);
                  }else{
                    op = value;
                    eg = -1;
                    lfo = -1;
                    wt = -1;
                    modconn = -1;
                  }
                }else if(strcasecmp(parameter, "eg") == 0){
                  if(value < 0 || value > 1 || (value != 0 && voice4op < 0)){
                    DEBUG_MSG("[MBFM_PatchFile_Load] Invalid eg number at line %d (should be zero-indexed)", line);
                  }else{
                    eg = value;
                    op = -1;
                    lfo = -1;
                    wt = -1;
                    modconn = -1;
                  }
                }else if(strcasecmp(parameter, "lfo") == 0){
                  if(value < 0 || value > 3 || (value > 1 && voice4op < 0)){
                    DEBUG_MSG("[MBFM_PatchFile_Load] Invalid lfo number at line %d (should be zero-indexed)", line);
                  }else{
                    lfo = value;
                    op = -1;
                    eg = -1;
                    wt = -1;
                    modconn = -1;
                  }
                }else if(strcasecmp(parameter, "wt") == 0){
                  if(value < 0 || value > 1 || (value != 0 && voice4op < 0)){
                    DEBUG_MSG("[MBFM_PatchFile_Load] Invalid wt number at line %d (should be zero-indexed)", line);
                  }else{
                    wt = value;
                    op = -1;
                    eg = -1;
                    lfo = -1;
                    modconn = -1;
                  }
                }else if(strcasecmp(parameter, "modconn") == 0){
                  if(value < 0 || value >= 32 || (value >= 16 && voice4op < 0)){
                    DEBUG_MSG("[MBFM_PatchFile_Load] Invalid modconn number at line %d (should be zero-indexed)", line);
                  }else{
                    if(value != modconn + 1){
                      DEBUG_MSG("[MBFM_PatchFile_Load] Warning: non-consecutive modconn numbers at line %d", line);
                      DEBUG_MSG("(If you don't fill in up to here later in the file, this modconn will be ignored)");
                    }
                    modconn = value;
                    op = -1;
                    eg = -1;
                    lfo = -1;
                    wt = -1;
                  }
                //==============================Parameters==============================
                }else if(strcasecmp(parameter, "bits") == 0){//==============================
                  if(op >= 0){
                    if(value > 0xFF || value < 0){
                      DEBUG_MSG("[MBFM_PatchFile_Load] bits value (line %d) must be 0-255", line);
                    }else{
                      VA_INIT_MACRO;
                      while((voice = va_next()) >= 0){
                        //DEBUG_MSG("Bits voice %d v2op %d v4op %d", voice, voice2op, voice4op);
                        pre_opparams[(2*voice)+op].bits = value;
                      }
                    }
                  }else{
                    DEBUG_MSG("[MBFM_PatchFile_Load] bits assignment (line %d) must come after op selection!", line);
                  }
                }else if(strcasecmp(parameter, "wave") == 0){//==============================
                  if(op >= 0){
                    if(value > 7 || value < 0){
                      DEBUG_MSG("[MBFM_PatchFile_Load] op wave value (line %d) must be 0-7", line);
                    }else{
                      VA_INIT_MACRO;
                      while((voice = va_next()) >= 0){
                        pre_opparams[(2*voice)+op].wave = value;
                      }
                    }
                  }else if(lfo >= 0){
                    if(value > 0xFF || value < 0){
                      DEBUG_MSG("[MBFM_PatchFile_Load] lfo wave value (line %d) must be 0-255", line);
                    }else{
                      VA_INIT_MACRO;
                      while((voice = va_next()) >= 0){
                        modlparams[voice+(lfo/2)].LFOwave[lfo&1] = value;
                      }
                    }
                  }else{
                    DEBUG_MSG("[MBFM_PatchFile_Load] wave assignment (line %d) must come after op or lfo selection!", line);
                  }
                }else if(strcasecmp(parameter, "fmult") == 0){//==============================
                  if(op >= 0){
                    if(value > 15 || value < 0){
                      DEBUG_MSG("[MBFM_PatchFile_Load] fmult value (line %d) must be 0-15", line);
                    }else{
                      VA_INIT_MACRO;
                      while((voice = va_next()) >= 0){
                        pre_opparams[(2*voice)+op].fmult = value;
                      }
                    }
                  }else{
                    DEBUG_MSG("[MBFM_PatchFile_Load] fmult assignment (line %d) must come after op selection!", line);
                  }
                }else if(strcasecmp(parameter, "atk") == 0){//==============================
                  if(op >= 0){
                    if(value > 15 || value < 0){
                      DEBUG_MSG("[MBFM_PatchFile_Load] op atk value (line %d) must be 0-15", line);
                    }else{
                      VA_INIT_MACRO;
                      while((voice = va_next()) >= 0){
                        pre_opparams[(2*voice)+op].atk = value;
                      }
                    }
                  }else if(eg >= 0){
                    if(value > 0xFF || value < 0){
                      DEBUG_MSG("[MBFM_PatchFile_Load] eg atk value (line %d) must be 0-255", line);
                    }else{
                      VA_INIT_MACRO;
                      while((voice = va_next()) >= 0){
                        pre_modlparams[voice+eg].EGatk = value;
                      }
                    }
                  }else{
                    DEBUG_MSG("[MBFM_PatchFile_Load] atk assignment (line %d) must come after op or eg selection!", line);
                  }
                }else if(strcasecmp(parameter, "dec") == 0){//==============================
                  if(op >= 0){
                    if(value > 15 || value < 0){
                      DEBUG_MSG("[MBFM_PatchFile_Load] dec value (line %d) must be 0-15", line);
                    }else{
                      VA_INIT_MACRO;
                      while((voice = va_next()) >= 0){
                        pre_opparams[(2*voice)+op].dec = value;
                      }
                    }
                  }else{
                    DEBUG_MSG("[MBFM_PatchFile_Load] dec assignment (line %d) must come after op selection!", line);
                  }
                }else if(strcasecmp(parameter, "dec1") == 0){//==============================
                  if(eg >= 0){
                    if(value > 0xFF || value < 0){
                      DEBUG_MSG("[MBFM_PatchFile_Load] eg dec1 value (line %d) must be 0-255", line);
                    }else{
                      VA_INIT_MACRO;
                      while((voice = va_next()) >= 0){
                        pre_modlparams[voice+eg].EGdec1 = value;
                      }
                    }
                  }else{
                    DEBUG_MSG("[MBFM_PatchFile_Load] dec1 assignment (line %d) must come after eg selection!", line);
                  }
                }else if(strcasecmp(parameter, "lvl") == 0){//==============================
                  if(eg >= 0){
                    if(value > 0xFF || value < 0){
                      DEBUG_MSG("[MBFM_PatchFile_Load] eg lvl value (line %d) must be 0-255", line);
                    }else{
                      VA_INIT_MACRO;
                      while((voice = va_next()) >= 0){
                        pre_modlparams[voice+eg].EGlvl = value;
                      }
                    }
                  }else{
                    DEBUG_MSG("[MBFM_PatchFile_Load] lvl assignment (line %d) must come after eg selection!", line);
                  }
                }else if(strcasecmp(parameter, "dec2") == 0){//==============================
                  if(eg >= 0){
                    if(value > 0xFF || value < 0){
                      DEBUG_MSG("[MBFM_PatchFile_Load] eg dec2 value (line %d) must be 0-255", line);
                    }else{
                      VA_INIT_MACRO;
                      while((voice = va_next()) >= 0){
                        pre_modlparams[voice+eg].EGdec2 = value;
                      }
                    }
                  }else{
                    DEBUG_MSG("[MBFM_PatchFile_Load] dec2 assignment (line %d) must come after eg selection!", line);
                  }
                }else if(strcasecmp(parameter, "sus") == 0){//==============================
                  if(op >= 0){
                    if(value > 15 || value < 0){
                      DEBUG_MSG("[MBFM_PatchFile_Load] sus value (line %d) must be 0-15", line);
                    }else{
                      VA_INIT_MACRO;
                      while((voice = va_next()) >= 0){
                        pre_opparams[(2*voice)+op].sus = value;
                      }
                    }
                  }else if(eg >= 0){
                    if(value > 0xFF || value < 0){
                      DEBUG_MSG("[MBFM_PatchFile_Load] eg sus value (line %d) must be 0-255", line);
                    }else{
                      VA_INIT_MACRO;
                      while((voice = va_next()) >= 0){
                        pre_modlparams[voice+eg].EGsus = value;
                      }
                    }
                  }else{
                    DEBUG_MSG("[MBFM_PatchFile_Load] sus assignment (line %d) must come after op or eg selection!", line);
                  }
                }else if(strcasecmp(parameter, "rel") == 0){//==============================
                  if(op >= 0){
                    if(value > 15 || value < 0){
                      DEBUG_MSG("[MBFM_PatchFile_Load] rel value (line %d) must be 0-15", line);
                    }else{
                      VA_INIT_MACRO;
                      while((voice = va_next()) >= 0){
                        pre_opparams[(2*voice)+op].rel = value;
                      }
                    }
                  }else if(eg >= 0){
                    if(value > 0xFF || value < 0){
                      DEBUG_MSG("[MBFM_PatchFile_Load] eg rel value (line %d) must be 0-255", line);
                    }else{
                      VA_INIT_MACRO;
                      while((voice = va_next()) >= 0){
                        pre_modlparams[voice+eg].EGrel = value;
                      }
                    }
                  }else{
                    DEBUG_MSG("[MBFM_PatchFile_Load] rel assignment (line %d) must come after op or eg selection!", line);
                  }
                }else if(strcasecmp(parameter, "vol") == 0){//==============================
                  if(op >= 0){
                    if(value > 63 || value < 0){
                      DEBUG_MSG("[MBFM_PatchFile_Load] vol value (line %d) must be 0-63", line);
                    }else{
                      VA_INIT_MACRO;
                      while((voice = va_next()) >= 0){
                        pre_opparams[(2*voice)+op].vol = value;
                      }
                    }
                  }else{
                    DEBUG_MSG("[MBFM_PatchFile_Load] vol assignment (line %d) must come after op selection!", line);
                  }
                }else if(strcasecmp(parameter, "tp") == 0){//==============================
                  if(value > 127 || value < -127){
                    DEBUG_MSG("[MBFM_PatchFile_Load] tp value (line %d) must be -127-127", line);
                  }else{
                    VA_INIT_MACRO;
                    while((voice = va_next()) >= 0){
                      pre_voiceparams[voice].tp = value;
                    }
                  }
                }else if(strcasecmp(parameter, "tune") == 0){//==============================
                  if(value > 127 || value < -127){
                    DEBUG_MSG("[MBFM_PatchFile_Load] tune value (line %d) must be -127-127", line);
                  }else{
                    VA_INIT_MACRO;
                    while((voice = va_next()) >= 0){
                      pre_voiceparams[voice].tune = value;
                    }
                  }
                }else if(strcasecmp(parameter, "retrig") == 0){//==============================
                  if(value > 1 || value < 0){
                    DEBUG_MSG("[MBFM_PatchFile_Load] retrig value (line %d) must be 0 or 1", line);
                  }else{
                    VA_INIT_MACRO;
                    while((voice = va_next()) >= 0){
                      voiceparams[voice].retrig = value;
                    }
                  }
                }else if(strcasecmp(parameter, "porta") == 0){//==============================
                  if(value > 255 || value < 0){
                    DEBUG_MSG("[MBFM_PatchFile_Load] porta value (line %d) must be 0-255", line);
                  }else{
                    VA_INIT_MACRO;
                    while((voice = va_next()) >= 0){
                      pre_voiceparams[voice].porta = value;
                    }
                  }
                }else if(strcasecmp(parameter, "dlyscale") == 0){//==============================
                  if(value > 1 || value < 0){
                    DEBUG_MSG("[MBFM_PatchFile_Load] dlyscale value (line %d) must be 0 or 1", line);
                  }else{
                    VA_INIT_MACRO;
                    while((voice = va_next()) >= 0){
                      voiceparams[voice].dlyscale = value;
                    }
                  }
                }else if(strcasecmp(parameter, "dlytime") == 0){//==============================
                  if(value > 255 || value < 0){
                    DEBUG_MSG("[MBFM_PatchFile_Load] voice dlytime value (line %d) must be 0-255", line);
                  }else{
                    VA_INIT_MACRO;
                    while((voice = va_next()) >= 0){
                      pre_voiceparams[voice].dlytime = value;
                    }
                  }
                }else if(strcasecmp(parameter, "feedback") == 0){//==============================
                  if(value > 7 || value < 0){
                    DEBUG_MSG("[MBFM_PatchFile_Load] feedback value (line %d) must be 0-7", line);
                  }else{
                    VA_INIT_MACRO;
                    while((voice = va_next()) >= 0){
                      pre_voiceparams[voice].feedback = value;
                    }
                  }
                }else if(strcasecmp(parameter, "dest") == 0){//==============================
                  if(modconn >= 0){
                    if(value >= 0x90 || value < 0){
                      DEBUG_MSG("[MBFM_PatchFile_Load] modconn dest value (line %d) must be 0-52", line);
                    }else{
                      VA_INIT_MACRO;
                      while((voice = va_next()) >= 0){
                        modllists[voice][modconn].dest = value;
                      }
                    }
                  }else{
                    if(value > 15 || value < 0){
                      DEBUG_MSG("[MBFM_PatchFile_Load] output dest value (line %d) must be 0-15", line);
                    }else{
                      VA_INIT_MACRO;
                      while((voice = va_next()) >= 0){
                        voiceparams[voice].dest = value;
                      }
                    }
                  }
                }else if(strcasecmp(parameter, "alg") == 0){//==============================
                  if(value < 0 || value > 3 || (value > 1 && voice2op < 0)){
                    DEBUG_MSG("[MBFM_PatchFile_Load] invalid alg value (line %d)", line);
                  }else{
                    VA_INIT_MACRO;
                    while((voice = va_next()) >= 0){
                      voiceparams[voice].alg = value;
                    }
                  }
                }else if(strcasecmp(parameter, "freq") == 0){//==============================
                  if(lfo >= 0){
                    if(value > 0xFF || value < 0){
                      DEBUG_MSG("[MBFM_PatchFile_Load] lfo freq value (line %d) must be 0-255", line);
                    }else{
                      VA_INIT_MACRO;
                      while((voice = va_next()) >= 0){
                        pre_modlparams[voice+(lfo/2)].LFOfrq[lfo&1] = value;
                      }
                    }
                  }else{
                    DEBUG_MSG("[MBFM_PatchFile_Load] frq assignment (line %d) must come after lfo selection!", line);
                  }
                }else if(strcasecmp(parameter, "delay") == 0){//==============================
                  if(lfo >= 0){
                    if(value > 0xFF || value < 0){
                      DEBUG_MSG("[MBFM_PatchFile_Load] lfo delay value (line %d) must be 0-255", line);
                    }else{
                      VA_INIT_MACRO;
                      while((voice = va_next()) >= 0){
                        pre_modlparams[voice+(lfo/2)].LFOdly[lfo&1] = value;
                      }
                    }
                  }else{
                    DEBUG_MSG("[MBFM_PatchFile_Load] delay assignment (line %d) must come after lfo selection!", line);
                  }
                }else if(strcasecmp(parameter, "time") == 0){//==============================
                  //TODO
                }else if(strcasecmp(parameter, "d") == 0){//==============================
                  //TODO
                }else if(strcasecmp(parameter, "src") == 0){//==============================
                  if(modconn >= 0){
                    if(value > 11 || value < 0){
                      DEBUG_MSG("[MBFM_PatchFile_Load] src value (line %d) must be 0-11", line);
                    }else{
                      VA_INIT_MACRO;
                      while((voice = va_next()) >= 0){
                        modllists[voice][modconn].src = value;
                      }
                    }
                  }else{
                    DEBUG_MSG("[MBFM_PatchFile_Load] src assignment (line %d) must come after modconn selection!", line);
                  }
                }else if(strcasecmp(parameter, "depth") == 0){//==============================
                  if(modconn >= 0){
                    if(value > 127 || value < -127){
                      DEBUG_MSG("[MBFM_PatchFile_Load] depth value (line %d) must be -127-127", line);
                    }else{
                      VA_INIT_MACRO;
                      while((voice = va_next()) >= 0){
                        modllists[voice][modconn].pre_depth = value;
                      }
                    }
                  }else{
                    DEBUG_MSG("[MBFM_PatchFile_Load] depth assignment (line %d) must come after modconn selection!", line);
                  }
                }else{
                  DEBUG_MSG("[MBFM_PatchFile_Load] Unknown parameter %s at line %d", parameter, line);
                }
              }
            }
          }
        }
      }
    }

  } while( status >= 1 );

  //Refresh everything in the correct voices
  if(drum >= 0){
    i = (drum >> 1) * 18; //OPL3
    j = drum & 1;  //BD or rest
    if(j){
      MBFM_ModValuesToOPL3(i+16);
      MBFM_ModValuesToOPL3(i+17);
      voiceactualnotes[i+16].update = 1;
      voiceactualnotes[i+17].update = 1;
    }else{
      MBFM_ModValuesToOPL3(i+15);
      voiceactualnotes[i+15].update = 1;
    }
  }else{
    for(voice=0; voice<18*OPL3_COUNT; voice++){
      if(midichanmapping[voice] == chan){
        MBFM_ModValuesToOPL3(voice);
        voiceactualnotes[voice].update = 1;
      }
    }
  }
  
  // close file
  status |= FILE_ReadClose(&file);
  
  MUTEX_SDCARD_GIVE;

  if( status < 0 ) {
    DEBUG_MSG("[MBFM_PatchFile_Load] ERROR while reading file, status: %d\n", status);
    return status;
  }
  
  DEBUG_MSG("[MBFM_PatchFile_Load] Read file fine, status %d\n", status);
  
  return 0; // no error
}

///////////////////////////////////////////////////////////////////////////////////////////////////

s32 MBFM_PatchFile_Exists(u8 bankmsb, u8 banklsb, u8 program){
  s32 status;
  
  MUTEX_SDCARD_TAKE;
  sprintf(mbfm_line_buffer, "%s/BMSB%d/BLSB%d/Pat%d.fm", MBFM_FILES_PATH, bankmsb, banklsb, program);
  
  status = FILE_FileExists(mbfm_line_buffer);
  MUTEX_SDCARD_GIVE;
  
  return status;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

s32 MBFM_PatchFile_Save(u8 chan, s8 drum, u8 bankmsb, u8 banklsb, u8 program){
  if(chan >= 16) return -1;
  s32 status;
  
  MUTEX_SDCARD_TAKE;
  
  //Check MBFM_FILES_PATH/
  if(FILE_DirExists(MBFM_FILES_PATH) <= 0){
    DEBUG_MSG("[MBFM_PatchFile_Save] Directory %s doesn't exist yet, creating\n", MBFM_FILES_PATH);
    status = FILE_MakeDir(MBFM_FILES_PATH);
    if(status < 0){
      DEBUG_MSG("[MBFM_PatchFile_Save] failed to mkdir %s, status: %d\n", MBFM_FILES_PATH, status);
      MUTEX_SDCARD_GIVE;
      return status;
    }
  }
  
  //Check MBFM_FILES_PATH/BMSBXXX/
  sprintf(mbfm_line_buffer, "%s/BMSB%d", MBFM_FILES_PATH, bankmsb);
  if(FILE_DirExists(mbfm_line_buffer) <= 0){
    DEBUG_MSG("[MBFM_PatchFile_Save] Directory %s doesn't exist yet, creating\n", mbfm_line_buffer);
    status = FILE_MakeDir(mbfm_line_buffer);
    if(status < 0){
      DEBUG_MSG("[MBFM_PatchFile_Save] failed to mkdir %s, status: %d\n", mbfm_line_buffer, status);
      MUTEX_SDCARD_GIVE;
      return status;
    }
  }
  
  //Check MBFM_FILES_PATH/BMSBXXX/BLSBXXX/
  sprintf(mbfm_line_buffer, "%s/BMSB%d/BLSB%d", MBFM_FILES_PATH, bankmsb, banklsb);
  if(FILE_DirExists(mbfm_line_buffer) <= 0){
    DEBUG_MSG("[MBFM_PatchFile_Save] Directory %s doesn't exist yet, creating\n", mbfm_line_buffer);
    status = FILE_MakeDir(mbfm_line_buffer);
    if(status < 0){
      DEBUG_MSG("[MBFM_PatchFile_Save] failed to mkdir %s, status: %d\n", mbfm_line_buffer, status);
      MUTEX_SDCARD_GIVE;
      return status;
    }
  }
  
  //Actual file
  sprintf(mbfm_line_buffer, "%s/BMSB%d/BLSB%d/Pat%d.fm", MBFM_FILES_PATH, bankmsb, banklsb, program);
  
  if( (status=FILE_WriteOpen(mbfm_line_buffer, 1)) < 0 ) {
    DEBUG_MSG("[MBFM_PatchFile_Save] failed to open file %s for writing, status: %d\n", mbfm_line_buffer, status);
    FILE_WriteClose();
    MUTEX_SDCARD_GIVE;
    return status;
  }

  s8 voice2op = -1, voice4op = -1, num2op = 0, num4op = 0, op = -1, eg = -1, lfo = -1, wt = -1, modconn = -1, voice = -1;
  u8 fourop, fouropstop, index, flag;
  
  #define FLUSH_BUFFER { status |= FILE_WriteBuffer((u8 *)mbfm_line_buffer, strlen(mbfm_line_buffer)); }

  sprintf(mbfm_line_buffer, "#MIDIbox FM V2.0 Patch File\n");
  FLUSH_BUFFER;
  sprintf(mbfm_line_buffer, "name=%s\n", patchnames[chan]); //TODO should be "drumsetnames[drum]" or something
  FLUSH_BUFFER;
  if(drum < 0){
    index = 0;
  }else{
    index = 1 + (drum&1);
  }
  sprintf(mbfm_line_buffer, "drum=%d\n", index);
  FLUSH_BUFFER;
  //Voice counts
  if(drum < 0){
    for(voice=0; voice<18*OPL3_COUNT; voice++){
      if(midichanmapping[voice] == chan){
        if(voicedupl[voice].voice < 0){
          fourop = OPL3_IsChannel4Op(voice);
          if(fourop == 0){
            num2op++;
          }else if(fourop == 1){
            num4op++;
          }
        }
      }
    }
  }
  sprintf(mbfm_line_buffer, "use2op=%d\n", num2op);
  FLUSH_BUFFER;
  sprintf(mbfm_line_buffer, "use4op=%d\n", num4op);
  FLUSH_BUFFER;
  sprintf(mbfm_line_buffer, "# Begin voice data\n");
  FLUSH_BUFFER;
  //Begin voice data
  flag = 0;
  if(drum >= 0){
    voice = (18 * (drum >> 1)) + 15 + (drum&1);
    flag = 1;
  }else{
    voice = 0;
  }
  for(; voice<18*OPL3_COUNT; voice++){
    if(flag || (midichanmapping[voice] == chan && voicedupl[voice].voice < 0)){
      fourop = OPL3_IsChannel4Op(voice);
      if(fourop == 1){
        fouropstop = 4;
        voice4op++;
        sprintf(mbfm_line_buffer, "voice4op=%d\n", voice4op);
      }else{
        fouropstop = 2;
        voice2op++;
        sprintf(mbfm_line_buffer, "voice2op=%d\n", voice2op);
      }
      FLUSH_BUFFER;
      //Voice stuff
      sprintf(mbfm_line_buffer, "tp=%d\n", pre_voiceparams[voice].tp);
      FLUSH_BUFFER;
      sprintf(mbfm_line_buffer, "tune=%d\n", pre_voiceparams[voice].tune);
      FLUSH_BUFFER;
      sprintf(mbfm_line_buffer, "retrig=%d\n", voiceparams[voice].retrig);
      FLUSH_BUFFER;
      sprintf(mbfm_line_buffer, "porta=%d\n", pre_voiceparams[voice].porta);
      FLUSH_BUFFER;
      sprintf(mbfm_line_buffer, "dlyscale=%d\n", voiceparams[voice].dlyscale);
      FLUSH_BUFFER;
      sprintf(mbfm_line_buffer, "dlytime=%d\n", pre_voiceparams[voice].dlytime);
      FLUSH_BUFFER;
      sprintf(mbfm_line_buffer, "feedback=%d\n", pre_voiceparams[voice].feedback);
      FLUSH_BUFFER;
      sprintf(mbfm_line_buffer, "dest=%d\n", voiceparams[voice].dest);
      FLUSH_BUFFER;
      sprintf(mbfm_line_buffer, "alg=%d\n", voiceparams[voice].alg);
      FLUSH_BUFFER;
      //Op stuff
      for(op=0; op<fouropstop; op++){
        index = (2*voice)+op;
        sprintf(mbfm_line_buffer, "op=%d\n", op);
        FLUSH_BUFFER;
        sprintf(mbfm_line_buffer, "bits=%d\n", pre_opparams[index].bits);
        FLUSH_BUFFER;
        sprintf(mbfm_line_buffer, "wave=%d\n", pre_opparams[index].wave);
        FLUSH_BUFFER;
        sprintf(mbfm_line_buffer, "fmult=%d\n", pre_opparams[index].fmult);
        FLUSH_BUFFER;
        sprintf(mbfm_line_buffer, "atk=%d\n", pre_opparams[index].atk);
        FLUSH_BUFFER;
        sprintf(mbfm_line_buffer, "dec=%d\n", pre_opparams[index].dec);
        FLUSH_BUFFER;
        sprintf(mbfm_line_buffer, "sus=%d\n", pre_opparams[index].sus);
        FLUSH_BUFFER;
        sprintf(mbfm_line_buffer, "rel=%d\n", pre_opparams[index].rel);
        FLUSH_BUFFER;
        sprintf(mbfm_line_buffer, "vol=%d\n", pre_opparams[index].vol);
        FLUSH_BUFFER;
      }
      //EG stuff
      for(eg=0; eg<(fourop+1); eg++){
        index = voice+eg;
        sprintf(mbfm_line_buffer, "eg=%d\n", eg);
        FLUSH_BUFFER;
        sprintf(mbfm_line_buffer, "atk=%d\n", pre_modlparams[index].EGatk);
        FLUSH_BUFFER;
        sprintf(mbfm_line_buffer, "dec1=%d\n", pre_modlparams[index].EGdec1);
        FLUSH_BUFFER;
        sprintf(mbfm_line_buffer, "lvl=%d\n", pre_modlparams[index].EGlvl);
        FLUSH_BUFFER;
        sprintf(mbfm_line_buffer, "dec2=%d\n", pre_modlparams[index].EGdec2);
        FLUSH_BUFFER;
        sprintf(mbfm_line_buffer, "sus=%d\n", pre_modlparams[index].EGsus);
        FLUSH_BUFFER;
        sprintf(mbfm_line_buffer, "rel=%d\n", pre_modlparams[index].EGrel);
        FLUSH_BUFFER;
      }
      //LFO stuff
      for(lfo=0; lfo<fouropstop; lfo++){
        index = (2*voice)+(lfo/2);
        sprintf(mbfm_line_buffer, "lfo=%d\n", lfo);
        FLUSH_BUFFER;
        sprintf(mbfm_line_buffer, "freq=%d\n", pre_modlparams[index].LFOfrq[lfo&1]);
        FLUSH_BUFFER;
        sprintf(mbfm_line_buffer, "delay=%d\n", pre_modlparams[index].LFOdly[lfo&1]);
        FLUSH_BUFFER;
        sprintf(mbfm_line_buffer, "wave=%d\n", pre_modlparams[index].LFOwave[lfo&1]);
        FLUSH_BUFFER;
      }
      //WT stuff
      //TODO
      //Modconn stuff
      for(modconn=0; modconn<MBFM_MODL_NUMCONN; modconn++){
        if(modllists[voice][modconn].src == 0) break;
        sprintf(mbfm_line_buffer, "modconn=%d\n", modconn);
        FLUSH_BUFFER;
        sprintf(mbfm_line_buffer, "src=%d\n", modllists[voice][modconn].src);
        FLUSH_BUFFER;
        sprintf(mbfm_line_buffer, "dest=%d\n", modllists[voice][modconn].dest);
        FLUSH_BUFFER;
        sprintf(mbfm_line_buffer, "depth=%d\n", modllists[voice][modconn].pre_depth);
        FLUSH_BUFFER;
      }
      if(flag){
        if(drum&1){
          //Other drums
          if(voice%18 == 17) break;
        }else{
          break;
        }
      }
    }
  }

  // close file
  status |= FILE_WriteClose();
  
  MUTEX_SDCARD_GIVE;

  DEBUG_MSG("[MBFM_PatchFile_Save] finished writing with status %d\n", status);

  return status;
}



void MBFM_Patch_Init(){
  u8 i;
  for(i=0; i<16; i++){
    patchnames[i][0] = ' ';
    patchnames[i][1] = 0;
  }
  for(i=0; i<2*OPL3_COUNT; i++){
    drumnames[i][0] = ' ';
    drumnames[i][1] = 0;
  }
  //Percussion
  midibankmsb[9] = 127; //Just a startup default
}

void MBFM_Patch_Tick(u32 time){
  u8 i;
  s32 res;
  s8 drum;
  char *nameptr;
  //
  //Querying patch name
  //
  if(patchloadquery && (time - lastdatawheelchange > 200)){
    if(MBFM_PatchFile_Exists(midibankmsb[act_midich], midibanklsb[act_midich], midiprogram[act_midich])){
      MBFM_PatchFile_GetName(midibankmsb[act_midich], midibanklsb[act_midich], midiprogram[act_midich]);
    }else{
      sprintf(loadpatchname, "[no file]");
    }
    MBFM_DrawScreen();
    patchloadquery = 0;
  }
  //
  //Loading/saving done on this thread
  //
  for(i=0; i<16; i++){
    if(i == 9){
      drum = prgstate[9].drum;
      nameptr = drumnames[drum];
    }else{
      drum = -1;
      nameptr = patchnames[i];
    }
    if(prgstate[i].command == 1){
      //Actually load
      DEBUG_MSG("Loading patch MSB%d LSB%d Prog%d chn %d", midibankmsb[i], midibanklsb[i], midiprogram[i], i+1);
      res = MBFM_PatchFile_Load(i, drum, midibankmsb[i], midibanklsb[i], midiprogram[i]);
      if(res<0){
        sprintf(nameptr, "[ERROR!]\0");
      }
      prgstate[i].success = (res >= 0);
      if(res >= 0) screenmode = MBFM_SCREENMODE_NONE;
      MBFM_DrawScreen();
    }else if(prgstate[i].command == 2){
      //Actually save
      DEBUG_MSG("Saving patch MSB%d LSB%d Prog%d chn %d", midibankmsb[i], midibanklsb[i], midiprogram[i], i+1);
      res = MBFM_PatchFile_Save(i, drum, midibankmsb[i], midibanklsb[i], midiprogram[i]);
      if(res<0){
        sprintf(nameptr, "[ERROR!]\0");
      }
      prgstate[i].success = (res >= 0);
      if(res >= 0) screenmode = MBFM_SCREENMODE_NONE;
      MBFM_DrawScreen();
    }
    prgstate[i].command = 0;
  }
}
