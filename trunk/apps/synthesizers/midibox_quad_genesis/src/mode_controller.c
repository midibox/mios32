/*
 * MIDIbox Quad Genesis: Lighting Controller mode
 *
 * ==========================================================================
 *
 *  Copyright (C) 2018 Sauraen (sauraen@gmail.com)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#include <mios32.h>
#include <string.h>
#include <vgm.h>
#include "mode_controller.h"
#include "frontpanel.h"
#include "interface.h"

// Config ======================================================================

#define ENCODER_MODE_NONE 0
#define ENCODER_MODE_NORMAL 1
#define ENCODER_MODE_ENUM 2

#define LEDDD_MAIN -1
#define LEDDD_FREQ -2
#define LEDDD_OCT -3

typedef struct {
    u8 mode;
    u8 nvalues;
    s8 initvalue;
    u8 bank;
    u8 ovr;
    s8 ledring;
    u8 ringdivider;
} encoder_opts_t;

const static encoder_opts_t encoder_opts[FP_E_COUNT] = {
    {0, 128,  0, 0,  0, LEDDD_MAIN,      1}, //0 Datawheel
    {1,  32,  0, 0,  0, FP_LEDR_OP1LVL,  2}, //1 Sides Level
    {1,  32,  0, 0,  0, FP_LEDR_OP2LVL,  2}, //2 Audc Level
    {1,  32,  0, 0,  0, FP_LEDR_OP3LVL,  2}, //3 Twrs Level
    {1,  32,  0, 0,  0, FP_LEDR_OP4LVL,  2}, //4 Spots Level
    {1,  32,  0, 0,  0, FP_LEDR_HARM,    2}, //5 Front Level
    {2,   7,  3, 4,  0, FP_LEDR_DETUNE,  1}, //6 Gobo 1 Rotation
    {2,   7,  0, 4,  7, FP_LEDR_ATTACK,  1}, //7 Gobo 1
    {2,  10,  0, 4, 14, FP_LEDR_DEC1R,   1}, //8 Gobo 2
    {2,   8,  0, 4, 24, FP_LEDR_DECLVL,  1}, //9 Prism
    {2,   7,  0, 5,  0, FP_LEDR_DEC2R,   1}, //10 Animation
    {2,   9,  0, 5,  7, FP_LEDR_RELRATE, 1}, //11 Zoom
    {1,  64, 25, 0,  0, FP_LEDR_CSMFREQ, 4}, //12 Focus
    {0,  16,  0, 0,  0, LEDDD_OCT,       1}, //13 
    {0, 128,  0, 0,  0, LEDDD_FREQ,      1}, //14 
    {0,  32,  0, 0,  0, FP_LEDR_PSGFREQ, 2}, //15 
    {1,  32,  8, 0,  0, FP_LEDR_PSGVOL,  2}, //16 Global Speed
    {0,   8,  0, 0,  0, FP_LEDR_LFOFDEP, 1}, //17 
    {0,   4,  0, 0,  0, FP_LEDR_LFOADEP, 1}, //18 
    {0,   8,  0, 0,  0, FP_LEDR_LFOFREQ, 1}, //19 
    {0,   8,  0, 0,  0, FP_LEDR_FEEDBACK, 1} //20 
};

//Buttons

#define BUTTON_MODE_NONE 0
#define BUTTON_MODE_FLASH 1
#define BUTTON_MODE_STICKY 2
#define BUTTON_MODE_STATE 3
#define BUTTON_MODE_MUTEX 4

typedef struct {
    u8 mode;
    u8 idx;
    u8 bank;
    u8 ovr;
    s8 led1;
    s8 led2;
} button_def_t;

const static button_def_t button_defs[82] = {
    {BUTTON_MODE_NONE,   0, 0,  0, -1, -1}, //FP_B_NONE 0
    {BUTTON_MODE_NONE,   0, 0,  0, -1, -1}, //FP_B_GVOICE 1
    {BUTTON_MODE_NONE,   0, 0,  0, -1, -1}, //FP_B_SOFTKEY 2
    {BUTTON_MODE_FLASH,  0, 0, 24, FP_LED_SELOP_1, -1}, //FP_B_SELOP 3
    {BUTTON_MODE_FLASH,  0, 0, 25, FP_LED_SELOP_2, -1}, //SELOP 1 //FP_B_OPMUTE 4
    {BUTTON_MODE_FLASH,  0, 0, 26, FP_LED_SELOP_3, -1}, //SELOP 2
    {BUTTON_MODE_FLASH,  0, 0, 27, FP_LED_SELOP_4, -1}, //SELOP 3
    {BUTTON_MODE_NONE,   0, 0,  0, -1, -1}, // 7
    {BUTTON_MODE_NONE,   0, 0,  0, -1, -1}, // 8
    {BUTTON_MODE_NONE,   0, 0,  0, -1, -1}, // 9
    {BUTTON_MODE_MUTEX,  0, 0,  0, FP_LED_SYSTEM, -1}, //FP_B_SYSTEM 10
    {BUTTON_MODE_MUTEX,  1, 0,  0, FP_LED_VOICE, -1}, //FP_B_VOICE 11
    {BUTTON_MODE_MUTEX,  2, 0,  0, FP_LED_CHAN, -1}, //FP_B_CHAN 12
    {BUTTON_MODE_MUTEX,  3, 0,  0, FP_LED_PROG, -1}, //FP_B_PROG 13
    {BUTTON_MODE_MUTEX,  4, 0,  0, FP_LED_VGM, -1}, //FP_B_VGM 14
    {BUTTON_MODE_MUTEX,  5, 0,  0, FP_LED_MDLTR, -1}, //FP_B_MDLTR 15
    {BUTTON_MODE_MUTEX,  6, 0,  0, FP_LED_SAMPLE, -1}, //FP_B_SAMPLE 16
    {BUTTON_MODE_NONE,   0, 0,  0, -1, -1}, //FP_B_MENU 17
    {BUTTON_MODE_NONE,   0, 0,  0, -1, -1}, //FP_B_ENTER 18
    {BUTTON_MODE_NONE,   0, 0,  0, -1, -1}, //FP_B_DATAWHEEL 19
    {BUTTON_MODE_STATE,  0, 0,  0, FP_LED_LOAD, -1}, //FP_B_LOAD 20
    {BUTTON_MODE_STATE,  1, 0,  0, FP_LED_SAVE, -1}, //FP_B_SAVE 21
    {BUTTON_MODE_STATE,  2, 0,  0, FP_LED_NEW, -1}, //FP_B_NEW 22
    {BUTTON_MODE_STATE,  3, 0,  0, FP_LED_DELETE, -1}, //FP_B_DELETE 23
    {BUTTON_MODE_STATE,  4, 0,  0, FP_LED_CROP, -1}, //FP_B_CROP 24
    {BUTTON_MODE_STATE,  5, 0,  0, FP_LED_CAPTURE, -1}, //FP_B_CAPTURE 25
    {BUTTON_MODE_STATE,  6, 0,  0, FP_LED_DUPL, -1}, //FP_B_DUPL 26
    {BUTTON_MODE_STATE,  7, 0,  0, FP_LED_PASTE, -1}, //FP_B_PASTE 27
    {BUTTON_MODE_STATE,  8, 0,  0, FP_LED_GROUP, -1}, //FP_B_GROUP 28
    {BUTTON_MODE_STATE,  9, 0,  0, FP_LED_CTRL_R, -1}, //FP_B_CTRL 29
    {BUTTON_MODE_STATE, 10, 0,  0, FP_LED_TIME_R, -1}, //FP_B_TIME 30
    {BUTTON_MODE_FLASH,  0, 1, 28, FP_LED_CMDS, -1}, //FP_B_CMDS 31
    {BUTTON_MODE_FLASH,  0, 1, 29, FP_LED_STATE, -1}, //FP_B_STATE 32
    {BUTTON_MODE_MUTEX,  7, 0,  0, FP_LED_MUTE, -1}, //FP_B_MUTE 33
    {BUTTON_MODE_MUTEX,  8, 0,  0, FP_LED_SOLO, -1}, //FP_B_SOLO 34
    {BUTTON_MODE_MUTEX,  9, 0,  0, FP_LED_RELEASE, -1}, //FP_B_RELEASE 35
    {BUTTON_MODE_MUTEX, 10, 0,  0, FP_LED_PNLOVR, -1}, //FP_B_PNLOVR 36
    {BUTTON_MODE_MUTEX, 11, 0,  0, FP_LED_RESTART, -1}, //FP_B_RESTART 37
    {BUTTON_MODE_MUTEX, 12, 0,  0, FP_LED_PLAY, -1}, //FP_B_PLAY 38
    {BUTTON_MODE_MUTEX, 13, 0,  0, FP_LED_RESET, -1}, //FP_B_RESET 39
    {BUTTON_MODE_NONE,   0, 0,  0, -1, -1}, //FP_B_MARKST 40
    {BUTTON_MODE_NONE,   0, 0,  0, -1, -1}, //FP_B_MARKEND 41
    {BUTTON_MODE_NONE,   0, 0,  0, -1, -1}, //FP_B_MOVEUP 42
    {BUTTON_MODE_NONE,   0, 0,  0, -1, -1}, //FP_B_MOVEDN 43
    {BUTTON_MODE_FLASH,  0, 1, 24, -1, -1}, //FP_B_CMDUP 44
    {BUTTON_MODE_FLASH,  0, 1, 25, -1, -1}, //FP_B_CMDDN 45
    {BUTTON_MODE_FLASH,  0, 1, 26, -1, -1}, //FP_B_STATEUP 46
    {BUTTON_MODE_FLASH,  0, 1, 27, -1, -1}, //FP_B_STATEDN 47
    {BUTTON_MODE_NONE,   0, 0,  0, -1, -1}, // 48
    {BUTTON_MODE_NONE,   0, 0,  0, -1, -1}, // 49
    {BUTTON_MODE_STICKY, 0, 0, 28, FP_LED_OUTL, FP_LED_OUTR}, //FP_B_OUT 50
    {BUTTON_MODE_STICKY, 1, 0, 29, FP_LED_FMW_C1, FP_LED_FMW_D1}, //FP_B_ALG 51
    {BUTTON_MODE_STICKY, 2, 0, 30, FP_LED_KEYON_2, FP_LED_KEYON_3}, //FP_B_KON 52
    {BUTTON_MODE_STICKY, 3, 0, 31, FP_LED_DACEN, -1}, //FP_B_DACEN 53
    {BUTTON_MODE_NONE,   0, 0,  0, -1, -1}, // 54
    {BUTTON_MODE_NONE,   0, 0,  0, -1, -1}, // 55
    {BUTTON_MODE_NONE,   0, 0,  0, -1, -1}, // 56
    {BUTTON_MODE_NONE,   0, 0,  0, -1, -1}, // 57
    {BUTTON_MODE_NONE,   0, 0,  0, -1, -1}, // 58
    {BUTTON_MODE_NONE,   0, 0,  0, -1, -1}, // 59
    {BUTTON_MODE_FLASH,  0, 2, 24, FP_LED_KSR, -1}, //FP_B_KSR 60
    {BUTTON_MODE_FLASH,  0, 2, 25, FP_LED_SSGON, -1}, //FP_B_SSGON 61
    {BUTTON_MODE_FLASH,  0, 2, 26, FP_LED_SSGINIT, -1}, //FP_B_SSGINIT 62
    {BUTTON_MODE_FLASH,  0, 2, 27, FP_LED_SSGTGL, -1}, //FP_B_SSGTGL 63
    {BUTTON_MODE_FLASH,  0, 2, 28, FP_LED_SSGHOLD, -1}, //FP_B_SSGHOLD 64
    {BUTTON_MODE_FLASH,  0, 2, 29, FP_LED_LFOAM, -1}, //FP_B_LFOAM 65
    {BUTTON_MODE_NONE,   0, 0,  0, -1, -1}, // 66
    {BUTTON_MODE_NONE,   0, 0,  0, -1, -1}, // 67
    {BUTTON_MODE_NONE,   0, 0,  0, -1, -1}, // 68
    {BUTTON_MODE_NONE,   0, 0,  0, -1, -1}, // 69
    {BUTTON_MODE_FLASH,  0, 3, 24, FP_LED_CH3_CSM, -1}, //FP_B_CH3MODE 70
    {BUTTON_MODE_FLASH,  0, 3, 25, FP_LED_CH3FAST, -1}, //FP_B_CH3FAST 71
    {BUTTON_MODE_FLASH,  0, 3, 26, FP_LED_UGLY, -1}, //FP_B_UGLY 72
    {BUTTON_MODE_FLASH,  0, 3, 27, FP_LED_DACOVR, -1}, //FP_B_DACOVR 73
    {BUTTON_MODE_FLASH,  0, 3, 28, FP_LED_LFO, -1}, //FP_B_LFO 74
    {BUTTON_MODE_FLASH,  0, 3, 29, FP_LED_EG, -1}, //FP_B_EG 75
    {BUTTON_MODE_NONE,   0, 0,  0, -1, -1}, // 76
    {BUTTON_MODE_NONE,   0, 0,  0, -1, -1}, // 77
    {BUTTON_MODE_NONE,   0, 0,  0, -1, -1}, // 78
    {BUTTON_MODE_NONE,   0, 0,  0, -1, -1}, // 79
    {BUTTON_MODE_STICKY, 4, 2, 30, FP_LED_NS_LOW, FP_LED_NS_SQ3}, //FP_B_NSFREQ 80
    {BUTTON_MODE_STICKY, 5, 2, 31, FP_LED_NS_WHT, FP_LED_NS_PLS}, //FP_B_NSTYPE 81
};

const static u8 sticky2button[6] = {
    FP_B_OUT, FP_B_ALG, FP_B_KON, FP_B_DACEN, FP_B_NSFREQ, FP_B_NSTYPE
};

const static u8 mutex2button[14] = {
    FP_B_SYSTEM, FP_B_VOICE, FP_B_CHAN, FP_B_PROG, FP_B_VGM, FP_B_MDLTR, FP_B_SAMPLE,
    FP_B_MUTE, FP_B_SOLO, FP_B_RELEASE, FP_B_PNLOVR, FP_B_RESTART, FP_B_PLAY, FP_B_RESET
};

// MIDI Interface ==============================================================

//Encoders: 0-20
#define MIDI_BANKSEL_START 22 //+6 = 
#define MIDI_OVERRIDE_START 28 //+32 =
#define MIDI_ALLBUTTONSOFF 60 //+1 =
#define MIDI_SUBSON 61 //+14 =
#define MIDI_SUBSOFF 75 //+14

//Colors are banks 0-3 override buttons 0-11 and 12-23

// Variables ===================================================================

static u8 submode;
static u8 encoder_modifier;
static u8 color_modifier;
static u8 flash_counter;
static u8 blind_button;
static u8 copymode;
static u8 copysrc;

static u8 flashstates[82];

// States ======================================================================

typedef struct {
    s8 encoder_vals[FP_E_COUNT];
    s8 colorstates[8];
    u8 stickystates[6];
    s8 mutexstate;
    u8 protected:1;
} state_t;

static state_t states[12];
static u8 stagestate;
static u8 stageorigstate;
static u8 editstate;

inline u8 GetEncoderSendValue(u8 e, s8 v){
    return 127 * (s32)v / (encoder_opts[e].nvalues - 1);
}

inline void NoteOnOff(u8 note){
    if(note > 127) return;
    MIOS32_MIDI_SendNoteOn(USB0, 0, note, 127);
    MIOS32_MIDI_SendNoteOn(USB0, 0, note, 0);
}

inline void SendEncoder(u8 enc, u8 val){
    if(enc > FP_E_COUNT) return;
    if(val >= 128) return;
    MIOS32_MIDI_SendNoteOn(USB0, 0, enc, val);
}

inline void SendColor(u8 colorbank, u8 color){
    if(colorbank >= 8) return;
    if(color >= 12) return;
    NoteOnOff(MIDI_BANKSEL_START+(colorbank>>1));
    NoteOnOff(MIDI_OVERRIDE_START+((colorbank&1)*12)+color);
}

inline void SendMutex(u8 m){
    u8 i;
    for(i=0; i<14; ++i){
        NoteOnOff(((m == i) ? MIDI_SUBSON : MIDI_SUBSOFF) + i);
    }
}

void SendFullState(u8 s){
    u8 i;
    s8 v;
    //All Buttons Off
    NoteOnOff(MIDI_ALLBUTTONSOFF);
    //Send encoder states
    for(i=0; i<FP_E_COUNT; ++i){
        v = states[s].encoder_vals[i];
        if(encoder_opts[i].mode == ENCODER_MODE_NORMAL){
            SendEncoder(i, GetEncoderSendValue(i, v));
        }else if(encoder_opts[i].mode == ENCODER_MODE_ENUM){
            NoteOnOff(MIDI_BANKSEL_START + encoder_opts[i].bank);
            NoteOnOff(MIDI_OVERRIDE_START + encoder_opts[i].ovr + v);
        }
    }
    //Send color states
    for(i=0; i<8; ++i){
        SendColor(i, states[s].colorstates[i]);
    }
    //Send button states
    for(i=0; i<82; ++i){
        u8 mode = button_defs[i].mode;
        if(mode == BUTTON_MODE_FLASH){
            if(!flashstates[i]) continue;
        }else if(mode == BUTTON_MODE_STICKY){
            if(!states[s].stickystates[button_defs[i].idx]) continue;
        }else{
            continue;
        }
        NoteOnOff(MIDI_BANKSEL_START + button_defs[i].bank);
        NoteOnOff(MIDI_OVERRIDE_START + button_defs[i].ovr);
    }
    //Send mutex state
    SendMutex(states[s].mutexstate);
}

void SendDeltaState(u8 newstate, u8 curstate){
    u8 i;
    s8 v;
    //Send encoder states
    for(i=0; i<FP_E_COUNT; ++i){
        v = states[newstate].encoder_vals[i];
        if(encoder_opts[i].mode == ENCODER_MODE_NORMAL){
            SendEncoder(i, GetEncoderSendValue(i, v));
        }else if(encoder_opts[i].mode == ENCODER_MODE_ENUM){
            if(v == states[curstate].encoder_vals[i]) continue;
            NoteOnOff(MIDI_BANKSEL_START + encoder_opts[i].bank);
            NoteOnOff(MIDI_OVERRIDE_START + encoder_opts[i].ovr + v);
        }
    }
    //Send changed color states
    for(i=0; i<8; ++i){
        if(states[newstate].colorstates[i] != states[curstate].colorstates[i]){
            SendColor(i, states[newstate].colorstates[i]);
        }
    }
    //Send changed sticky states
    for(i=0; i<6; ++i){
        if(states[newstate].stickystates[i] != states[curstate].stickystates[i]){
            u8 b = sticky2button[i];
            NoteOnOff(MIDI_BANKSEL_START + button_defs[b].bank);
            NoteOnOff(MIDI_OVERRIDE_START + button_defs[b].ovr);
        }
    }
    //Send changed mutex state
    if(states[newstate].mutexstate != states[curstate].mutexstate){
        SendMutex(states[newstate].mutexstate);
    }
}

void ColorChange(u8 colorbank, u8 color){
    if(colorbank >= 8) return;
    if(color >= 12) return;
    s8* c = &states[editstate].colorstates[colorbank];
    if(*c == color) return;
    *c = color;
    if(editstate == stagestate){
        SendColor(colorbank, color);
    }
}

void SaveLoadState(u8 save){
    u8 s = (editstate == stagestate) ? stageorigstate : editstate;
    char *filename = malloc(32);
    sprintf(filename, "/LIGHTING");
    MUTEX_SDCARD_TAKE;
    if(!FILE_DirExists(filename)){
        if(FILE_MakeDir(filename) < 0){
            DBG("Error making directory %s\n", filename);
            goto Error;
        }
    }
    sprintf(filename, "/LIGHTING/%d.ST", s);
    if(save){
        if(FILE_FileExists(filename)){
            if(FILE_Remove(filename) < 0){
                DBG("Error deleting %s\n", filename);
                goto Error;
            }
        }
        if(FILE_WriteOpen(filename, 1) < 0){
            DBG("Error opening %s for writing\n", filename);
            goto Error;
        }
        FILE_WriteBuffer((u8*)&states[s], sizeof(state_t));
        FILE_WriteClose();
    }else{
        if(!FILE_FileExists(filename)){
            DBG("File %s does not exist\n");
            MIOS32_LCD_CursorSet(25, 1);
            MIOS32_LCD_PrintString("NONE");
            goto Done;
        }
        file_t file;
        if(FILE_ReadOpen(&file, filename) < 0){
            DBG("Error opening %s for reading\n", filename);
            goto Error;
        }
        if(FILE_ReadGetCurrentSize() != sizeof(state_t)){
            DBG("File %s is wrong size\n", filename);
            goto Error;
        }
        FILE_ReadBuffer((u8*)&states[s], sizeof(state_t));
        FILE_ReadClose(&file);
        SendFullState(s);
    }
    MIOS32_LCD_CursorSet(save ? 21 : 26, 1);
    MIOS32_LCD_PrintString("OK");
    goto Done;
Error:
    MIOS32_LCD_CursorSet(save ? 21 : 26, 1);
    MIOS32_LCD_PrintString("ERR");
Done:
    MUTEX_SDCARD_GIVE;
    free(filename);
}

// System ======================================================================

void DrawMenu(){
    switch(submode){
        case 0:
            MIOS32_LCD_Clear();
            MIOS32_LCD_CursorSet(0,0);
            if(copymode == 1){
                MIOS32_LCD_PrintString("Select state to copy from");
            }else if(copymode == 2){
                MIOS32_LCD_PrintFormattedString("Copy state %d to... (select dest)", copysrc);
            }else if(copymode == 3){
                MIOS32_LCD_PrintString("Copied");
            }else{
                MIOS32_LCD_PrintFormattedString("State %d", stageorigstate);
                if(states[stageorigstate].protected){
                    MIOS32_LCD_PrintString(" (Prot)");
                }
                if(editstate != stagestate){
                    MIOS32_LCD_PrintFormattedString(" / Blind edit %d", editstate);
                    if(states[editstate].protected){
                        MIOS32_LCD_PrintString(" (WARN: Prot)");
                    }
                }
            }
            MIOS32_LCD_CursorSet(0,1);
            MIOS32_LCD_PrintString(" Exit St8 ---- ----  ---- ---- Copy Blnd");
            break;
        case 1:
            MIOS32_LCD_Clear();
            MIOS32_LCD_CursorSet(0,1);
            MIOS32_LCD_PrintString("Press Enter to quit");
            break;
        case 2:
            MIOS32_LCD_Clear();
            MIOS32_LCD_CursorSet(0,0);
            MIOS32_LCD_PrintString("State Prot Resnd    Save Load");
            u8 s = (editstate == stagestate) ? stageorigstate : editstate;
            MIOS32_LCD_CursorSet(2,1);
            MIOS32_LCD_PrintFormattedString("%d", s);
            MIOS32_LCD_CursorSet(7,1);
            MIOS32_LCD_PrintChar(states[s].protected ? '*' : '-');
            if(editstate != stagestate){
                MIOS32_LCD_CursorSet(12,1);
                MIOS32_LCD_PrintFormattedString("%d", stageorigstate);
            }
            break;
    }
}
void Mode_Controller_Init(){
    u8 s, i;
    for(s=0; s<12; ++s){
        for(i=0; i<FP_E_COUNT; ++i){
            states[s].encoder_vals[i] = encoder_opts[i].initvalue;
        }
        for(i=0; i<8; ++i){
            states[s].colorstates[i] = -1;
        }
        for(i=0; i<6; ++i){
            states[s].stickystates[i] = 0;
        }
        states[s].mutexstate = -1;
        states[s].protected = 0;
    }
    for(i=0; i<82; ++i){
        flashstates[i] = 0;
    }
    stagestate = 0;
    stageorigstate = 0;
    editstate = 0;
}
void Mode_Controller_GotFocus(){
    submode = 0;
    encoder_modifier = 0;
    color_modifier = 0;
    copymode = 0;
    DrawMenu();
}

void Mode_Controller_Tick(){
    ++flash_counter;
}
void Mode_Controller_Background(){
    u8 i, j;
    s8 v;
    //Draw state LED
    for(i=0; i<11; ++i){
        FrontPanel_LEDSet(button_defs[FP_B_LOAD+i].led1,
            editstate == stagestate ? 
                i == stageorigstate :
                (i == editstate || (i == stageorigstate && (flash_counter & 0x40))));
    }
    //Draw encoder states
    state_t* st = &states[editstate];
    for(i=0; i<FP_E_COUNT; ++i){
        if(encoder_opts[i].mode == ENCODER_MODE_NONE) continue;
        v = st->encoder_vals[i];
        v /= encoder_opts[i].ringdivider;
        s8 ring = encoder_opts[i].ledring;
        if(ring >= 0){
            if(encoder_opts[i].mode == ENCODER_MODE_ENUM){
                if(ring == FP_LEDR_DECLVL){
                    if(v > 0) v += 4;
                }
                FrontPanel_LEDRingSet(ring, 1, v);
            }else{ //ENCODER_MODE_NORMAL
                FrontPanel_LEDRingSet(ring, 2, v);
            }
        }else if(ring == LEDDD_FREQ){
            FrontPanel_DrawNumber(FP_LED_DIG_FREQ_1, v);
        }else if(ring == LEDDD_OCT){
            FrontPanel_DrawNumberHex(FP_LED_DIG_OCT, v);
        }
    }
    //Draw color states
    for(i=0; i<8; ++i){
        for(j=0; j<12; ++j){
            FrontPanel_GenesisLEDSet(i>>1, j, i&1, st->colorstates[i] == j);
        }
    }
    //Draw sticky states
    for(i=0; i<6; ++i){
        j = sticky2button[i];
        v = button_defs[j].led1;
        if(v >= 0) FrontPanel_LEDSet(v, st->stickystates[i]);
        v = button_defs[j].led2;
        if(v >= 0) FrontPanel_LEDSet(v, st->stickystates[i]);
    }
    //Draw mutex state
    for(i=0; i<14; ++i){
        j = mutex2button[i];
        v = button_defs[j].led1;
        if(v >= 0) FrontPanel_LEDSet(v, st->mutexstate == i);
        v = button_defs[j].led2;
        if(v >= 0) FrontPanel_LEDSet(v, st->mutexstate == i);
    }
}

void Mode_Controller_BtnSoftkey(u8 softkey, u8 state){
    if(submode == 1) Mode_Controller_GotFocus();
    switch(submode){
    case 0:
        switch(softkey){
        case 0:
            if(!state) return;
            submode = 1;
            DrawMenu();
            return;
        case 1:
            if(!state) return;
            submode = 2;
            DrawMenu();
            return;
        case 6:
            copymode = state ? 1 : 0;
            DrawMenu();
            return;
        case 7:
            blind_button = state;
            return;
        }
        break;
    case 2:
        switch(softkey){
        case 1:
            if(!state) return;
            if(editstate == stagestate){
                u8 prot = states[stageorigstate].protected;
                if(prot){
                    //Unprotect
                    memcpy(&states[stageorigstate], &states[11], sizeof(state_t));
                    stagestate = editstate = stageorigstate;
                }else{
                    //Protect
                    memcpy(&states[11], &states[stageorigstate], sizeof(state_t));
                    stagestate = editstate = 11;
                }
                states[stageorigstate].protected = !prot;
            }else{
                states[editstate].protected = !states[editstate].protected;
            }
            DrawMenu();
            return;
        case 2:
            if(!state) return;
            SendFullState(stagestate);
            return;
        case 4:
            if(!state) return;
            SaveLoadState(1);
            return;
        case 5:
            if(!state) return;
            SaveLoadState(0);
            return;
        }
        break;
    }
}

// Encoders ====================================================================

void Mode_Controller_EncDatawheel(s32 incrementer){
    Mode_Controller_EncEdit(0, incrementer);
}
void Mode_Controller_EncEdit(u8 encoder, s32 incrementer){
    if(encoder >= FP_E_COUNT) return;
    //Basic value edit
    s32 v = states[editstate].encoder_vals[encoder];
    v += incrementer;
    //Limits
    u8 upperlimit = encoder_opts[encoder].nvalues-1;
    if(v < 0) v = 0;
    if(v > upperlimit) v = upperlimit;
    if(encoder_modifier == 1) v = 0;
    if(encoder_modifier == 2) v = upperlimit;
    //Store value
    u8 changed = states[editstate].encoder_vals[encoder] != (s8)v;
    states[editstate].encoder_vals[encoder] = (s8)v;
    //Send value
    u8 mode = encoder_opts[encoder].mode;
    if(mode == ENCODER_MODE_NORMAL){
        v = GetEncoderSendValue(encoder, v);
        FrontPanel_DrawNumber(FP_LED_DIG_MAIN_1, v<<1); //Draw DMX value not MIDI
        if(editstate == stagestate && changed){
            SendEncoder(encoder, v);
        }
    }else if(mode == ENCODER_MODE_ENUM){
        FrontPanel_DrawNumber(FP_LED_DIG_MAIN_1, v);
        if(editstate == stagestate && changed){
            NoteOnOff(MIDI_BANKSEL_START + encoder_opts[encoder].bank);
            NoteOnOff(MIDI_OVERRIDE_START + encoder_opts[encoder].ovr + v);
        }
    }
}
void Mode_Controller_BtnOpMute(u8 op, u8 state){
    if(submode == 1) Mode_Controller_GotFocus();
    if(!state) return;
    states[editstate].encoder_vals[1+op] = 0;
    FrontPanel_DrawNumber(FP_LED_DIG_MAIN_1, 0);
    if(editstate == stagestate){
        SendEncoder(1+op, 0);
    }
}

// Buttons =====================================================================

void Mode_Controller_BtnSelOp(u8 op, u8 state){
    Mode_Controller_BtnSystem(FP_B_SELOP+op, state);
}
void Mode_Controller_BtnEdit(u8 button, u8 state){
    Mode_Controller_BtnSystem(button, state);
}

void Mode_Controller_BtnGVoice(u8 gvoice, u8 state){
    if(submode == 1) Mode_Controller_GotFocus();
    u8 genesis = gvoice >> 4;
    u8 voice = gvoice & 0xF;
    if(voice >= 0xC) return;
    if(genesis >= 4) return;
    if(!state) return;
    if(color_modifier != 2) ColorChange(genesis<<1, voice);
    if(color_modifier != 1) ColorChange((genesis<<1)+1, voice);
}
void Mode_Controller_BtnSystem(u8 button, u8 state){
    switch(button){
        case FP_B_MENU:
            submode = 0;
            DrawMenu();
            return;
        case FP_B_ENTER:
            if(submode == 1){
                Interface_ChangeToMode(MODE_SYSTEM);
            }
            return;
        case FP_B_DATAWHEEL:
            return;
        case FP_B_MARKST:
            color_modifier = state ? 1 : 0;
            return;
        case FP_B_MOVEUP:
            color_modifier = state ? 2 : 0;
            return;
        case FP_B_MARKEND:
            encoder_modifier = state ? 2 : 0;
            return;
        case FP_B_MOVEDN:
            encoder_modifier = state ? 1 : 0;
            return;
    }
    if(submode == 1) Mode_Controller_GotFocus();
    if(button >= 82) return;
    u8 mode = button_defs[button].mode;
    s8 idx = button_defs[button].idx;
    u8 bank = button_defs[button].bank;
    u8 ovr = button_defs[button].ovr;
    switch(mode){
    case BUTTON_MODE_STATE:
        if(!state) return;
        if(blind_button){
            editstate = idx;
        }else if(copymode == 1){
            copysrc = idx;
            copymode = 2;
        }else if(copymode == 2){
            memcpy(&states[idx], &states[copysrc], sizeof(state_t));
            copymode = 3;
        }else if(copymode != 0){
            return;
        }else{
            SendDeltaState(idx, stagestate);
            if(states[idx].protected){
                memcpy(&states[11], &states[idx], sizeof(state_t));
                stageorigstate = idx;
                stagestate = editstate = 11;
            }else{
                stagestate = editstate = stageorigstate = idx;
            }
        }
        FrontPanel_ClearDisplay(FP_LED_DIG_MAIN_1);
        DrawMenu();
        return;
    case BUTTON_MODE_FLASH:
        if(!flashstates[button] && state){
            //Turn on flash
            NoteOnOff(MIDI_BANKSEL_START+bank);
            NoteOnOff(MIDI_OVERRIDE_START+ovr);
        }else if(flashstates[button] && !state){
            //Turn off flash (same thing)
            NoteOnOff(MIDI_BANKSEL_START+bank);
            NoteOnOff(MIDI_OVERRIDE_START+ovr);
        }
        flashstates[button] = state;
        s8 led = button_defs[button].led1;
        if(led >= 0) FrontPanel_LEDSet(led, state);
        led = button_defs[button].led2;
        if(led >= 0) FrontPanel_LEDSet(led, state);
        return;
    case BUTTON_MODE_STICKY:
        if(!state) return;
        state = states[editstate].stickystates[idx];
        state = !state;
        states[editstate].stickystates[idx] = state;
        if(editstate == stagestate){
            NoteOnOff(MIDI_BANKSEL_START+bank);
            NoteOnOff(MIDI_OVERRIDE_START+ovr);
        }
        return;
    case BUTTON_MODE_MUTEX:
        if(!state) return;
        if(idx == states[editstate].mutexstate) idx = -1;
        states[editstate].mutexstate = idx;
        if(editstate == stagestate){
            SendMutex(idx);
        }
        return;
    }
}

