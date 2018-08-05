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
#include "mode_controller.h"
#include "frontpanel.h"
#include "interface.h"

// Config ======================================================================

typedef struct {
    u8 mode;
    s8 ledring;
    u8 ringsegs;
    u8 ticksperseg;
} encoder_opts_t;

#define LEDDD_MAIN -1
#define LEDDD_FREQ -2
#define LEDDD_OCT -3

#define ENCODER_MODE_NONE 0 //Encoder ignored
#define ENCODER_MODE_DIM 1 //State tracked, LED ring or display
#define ENCODER_MODE_SPEED 2 //State tracked, sub on/off sent, LED ring or display
#define ENCODER_MODE_ENUM 3 //This and up for enums
#define     ENCODER_MODE_PRISM 3
#define     ENCODER_MODE_GOBO1 4
#define     ENCODER_MODE_GOBO1ROT 5
#define     ENCODER_MODE_GOBO2 6
#define     ENCODER_MODE_ANIMSPD 7

const static encoder_opts_t encoder_opts[FP_E_COUNT] = {
    {0, LEDDD_MAIN, 128, 1}, //0 Datawheel
    {1, FP_LEDR_OP1LVL, 16, 2}, //1 Sides Level
    {1, FP_LEDR_OP2LVL, 16, 2}, //2 Audc Level
    {1, FP_LEDR_OP3LVL, 16, 2}, //3 Twrs Level
    {1, FP_LEDR_OP4LVL, 16, 2}, //4 Spots Level
    {1, FP_LEDR_HARM, 16, 2}, //5 Front Level
    {3, FP_LEDR_DETUNE, 7, 1}, //6 Prism, special handler for LED ring
    {1, FP_LEDR_ATTACK, 16, 2}, //7
    {2, FP_LEDR_DEC1R, 16, 2}, //8 Sub 16
    {2, FP_LEDR_DECLVL, 16, 2}, //9 Sub 17
    {2, FP_LEDR_DEC2R, 16, 2}, //10 Sub 18
    {2, FP_LEDR_RELRATE, 16, 2}, //11 Sub 19
    {2, FP_LEDR_CSMFREQ, 16, 2}, //12 Sub 20
    {1, LEDDD_OCT, 16, 1}, //13 Octave
    {1, LEDDD_FREQ, 128, 1}, //14 Frequency
    {1, FP_LEDR_PSGFREQ, 16, 2}, //15 Sub 14 - L spot pan
    {1, FP_LEDR_PSGVOL, 16, 2}, //16 Sub 15 - R spot pan
    {4, FP_LEDR_LFOFDEP, 8, 1}, //17 Gobo 1
    {5, FP_LEDR_LFOADEP, 4, 1}, //18 Gobo 1 Rotation, special handler for LED ring and changes
    {6, FP_LEDR_LFOFREQ, 8, 1}, //19 Gobo 2
    {7, FP_LEDR_FEEDBACK, 8, 1} //20 Gobo Animation Speed, special handler for mode
	//21 Extra for Gobo Animation Position
};

typedef struct {
    u8 numvalues;
    u8 values[16];
} encoder_enum_t;

const static encoder_enum_t encoder_enum[5] = {
    {8, {0, 15, 25, 35, 42, 50, 60, 70}}, //Prism
    {7, {0, 19, 21, 23, 25, 27, 29}}, //Gobo 1
    {11, {70, 80, 90, 100, 110, 0, 18, 28, 38, 48, 58}}, //Gobo 1 Rotation
    {10, {0, 95, 90, 86, 81, 76, 72, 67, 63, 59}}, //Gobo 2
    {15, {0, 70, 90, 110, 0, 18, 38, 58, 70, 90, 110, 0, 18, 38, 58}}  //Gobo Animation
};

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
    {BUTTON_MODE_MUTEX,  0, 5,  0, FP_LED_SYSTEM, -1}, //FP_B_SYSTEM 10
    {BUTTON_MODE_MUTEX,  1, 5,  1, FP_LED_VOICE, -1}, //FP_B_VOICE 11
    {BUTTON_MODE_MUTEX,  2, 5,  2, FP_LED_CHAN, -1}, //FP_B_CHAN 12
    {BUTTON_MODE_MUTEX,  3, 5,  3, FP_LED_PROG, -1}, //FP_B_PROG 13
    {BUTTON_MODE_MUTEX,  4, 5,  4, FP_LED_VGM, -1}, //FP_B_VGM 14
    {BUTTON_MODE_MUTEX,  5, 5,  5, FP_LED_MDLTR, -1}, //FP_B_MDLTR 15
    {BUTTON_MODE_MUTEX,  6, 5,  6, FP_LED_SAMPLE, -1}, //FP_B_SAMPLE 16
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
    {BUTTON_MODE_MUTEX,  7, 5,  7, FP_LED_MUTE, -1}, //FP_B_MUTE 33
    {BUTTON_MODE_MUTEX,  8, 5,  8, FP_LED_SOLO, -1}, //FP_B_SOLO 34
    {BUTTON_MODE_MUTEX,  9, 5,  9, FP_LED_RELEASE, -1}, //FP_B_RELEASE 35
    {BUTTON_MODE_MUTEX, 10, 5, 10, FP_LED_PNLOVR, -1}, //FP_B_PNLOVR 36
    {BUTTON_MODE_MUTEX, 11, 5, 11, FP_LED_RESTART, -1}, //FP_B_RESTART 37
    {BUTTON_MODE_MUTEX, 12, 5, 12, FP_LED_PLAY, -1}, //FP_B_PLAY 38
    {BUTTON_MODE_MUTEX, 13, 5, 13, FP_LED_RESET, -1}, //FP_B_RESET 39
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
    {BUTTON_MODE_STICKY, 0, 4,  0, FP_LED_OUTL, FP_LED_OUTR}, //FP_B_OUT 50
    {BUTTON_MODE_STICKY, 1, 4,  1, FP_LED_FMW_C1, FP_LED_FMW_D1}, //FP_B_ALG 51
    {BUTTON_MODE_STICKY, 2, 4,  2, FP_LED_KEYON_2, FP_LED_KEYON_3}, //FP_B_KON 52
    {BUTTON_MODE_STICKY, 3, 4,  3, FP_LED_DACEN, -1}, //FP_B_DACEN 53
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
    {BUTTON_MODE_STICKY, 4, 4,  4, FP_LED_NS_LOW, FP_LED_NS_SQ3}, //FP_B_NSFREQ 80
    {BUTTON_MODE_STICKY, 5, 4,  5, FP_LED_NS_WHT, FP_LED_NS_PLS}, //FP_B_NSTYPE 81
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
#define MIDI_SUBSON 61 //+5 =
#define MIDI_SUBSOFF 66 //+5 =
#define MIDI_SPOTSSUB1ON 71
#define MIDI_SPOTSSUB2ON 72
#define MIDI_SPOTSSUB1OFF 73
#define MIDI_SPOTSSUB2OFF 74

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

inline void GetEncoderSendValue(u8 e, s8 v, u8 *ev1, s8 *ev2, s8 *button){
    *ev2 = -1;
    *button = -1;
    u8 emode = encoder_opts[e].mode;
    if(emode == ENCODER_MODE_GOBO1ROT){
        u8 limit = encoder_enum[emode-3].numvalues >> 1;
        if(v < -limit) v = -limit;
        if(v > limit) v = limit;
        *ev1 = encoder_enum[emode-3].values[v+limit];
    }else if(emode >= ENCODER_MODE_ENUM){
        u8 limit = encoder_enum[emode-3].numvalues - 1;
        if(v < 0) v = 0;
        if(v > limit) v = limit;
        *ev1 = encoder_enum[emode-3].values[v];
        if(emode == ENCODER_MODE_ANIMSPD){
            if(v == 0) *ev2 = 0;
            else if(v < 8) *ev2 = 17;
            else *ev2 = 22;
        }
    }else{
        s32 upperlimit = (encoder_opts[e].ringsegs * encoder_opts[e].ticksperseg) - 1;
        *ev1 = 127 * v / upperlimit;
        if(emode == ENCODER_MODE_SPEED){
            *button = (e - FP_E_DEC1R) + (v == 0 ? MIDI_SUBSOFF : MIDI_SUBSON);
        }
    }
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

void SendFullState(u8 s){
    u8 i, j, ev1;
    s8 ev2, btn;
    //All Buttons Off
    NoteOnOff(MIDI_ALLBUTTONSOFF);
    //Send encoder states
    for(i=0; i<FP_E_COUNT; ++i){
        GetEncoderSendValue(i, states[s].encoder_vals[i], &ev1, &ev2, &btn);
        SendEncoder(i, ev1);
        if(ev2 >= 0) SendEncoder(i+1, ev2);
        if(btn >= 0) NoteOnOff(btn);
    }
    //Send color states
    for(i=0; i<8; ++i){
        SendColor(i, states[s].colorstates[i]);
    }
    //Send button states
    for(i=0; i<82; ++i){
        j = button_defs[i].mode;
        if(j == BUTTON_MODE_FLASH){
            if(!flashstates[i]) continue;
        }else if(j == BUTTON_MODE_STICKY){
            if(!states[s].stickystates[button_defs[i].idx]) continue;
        }else if(j == BUTTON_MODE_MUTEX){
            if(states[s].mutexstate != button_defs[i].idx) continue;
        }else{
            continue;
        }
        NoteOnOff(MIDI_BANKSEL_START + button_defs[i].bank);
        NoteOnOff(MIDI_OVERRIDE_START + button_defs[i].ovr);
    }
    //Send Spots Enable state
    if(states[s].stickystates[button_defs[FP_B_NSFREQ].idx]){
        NoteOnOff(MIDI_SPOTSSUB1ON);
        NoteOnOff(MIDI_SPOTSSUB2ON);
    }else{
        NoteOnOff(MIDI_SPOTSSUB1OFF);
        NoteOnOff(MIDI_SPOTSSUB2OFF);
    }
}

void SendDeltaState(u8 newstate, u8 curstate){
    u8 i, ev1;
    s8 ev2, btn;
    //Send encoder states
    for(i=0; i<FP_E_COUNT; ++i){
        GetEncoderSendValue(i, states[newstate].encoder_vals[i], &ev1, &ev2, &btn);
        SendEncoder(i, ev1);
        if(ev2 >= 0) SendEncoder(i+1, ev2);
        if(btn >= 0) NoteOnOff(btn);
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
            ev1 = sticky2button[i];
            NoteOnOff(MIDI_BANKSEL_START + button_defs[ev1].bank);
            NoteOnOff(MIDI_OVERRIDE_START + button_defs[ev1].ovr);
        }
    }
    //Send Spots Enable state
    if(states[newstate].stickystates[button_defs[FP_B_NSFREQ].idx]){
        NoteOnOff(MIDI_SPOTSSUB1ON);
        NoteOnOff(MIDI_SPOTSSUB2ON);
    }else{
        NoteOnOff(MIDI_SPOTSSUB1OFF);
        NoteOnOff(MIDI_SPOTSSUB2OFF);
    }
    //Send changed mutex state
    if(states[newstate].mutexstate != states[curstate].mutexstate){
        if(states[newstate].mutexstate < 0){
            ev1 = mutex2button[states[curstate].mutexstate];
        }else{
            ev1 = mutex2button[states[newstate].mutexstate];
        }
        NoteOnOff(MIDI_BANKSEL_START + button_defs[ev1].bank);
        NoteOnOff(MIDI_OVERRIDE_START + button_defs[ev1].ovr);
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
            MIOS32_LCD_PrintString("Prot     Resnd");
            MIOS32_LCD_CursorSet(2,1);
            MIOS32_LCD_PrintChar(states[stageorigstate].protected ? '*' : '-');
            break;
    }
}
void Mode_Controller_Init(){
    u8 s, i;
    for(s=0; s<12; ++s){
        for(i=0; i<FP_E_COUNT; ++i){
            states[s].encoder_vals[i] = 0;
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
    u8 i, j, emode;
    s8 v, ring;
    //Draw state LED
    for(i=0; i<11; ++i){
        FrontPanel_LEDSet(button_defs[FP_B_LOAD+i].led1, 
            editstate == stagestate ? 
                i == stageorigstate :
                i == editstate || (i == stageorigstate && (flash_counter & 0x40)));
    }
    //Draw encoder states
    state_t* st = &states[editstate];
    for(i=0; i<FP_E_COUNT; ++i){
        ring = encoder_opts[i].ledring;
        emode = encoder_opts[i].mode;
        v = st->encoder_vals[i];
        if(emode == ENCODER_MODE_PRISM){
            FrontPanel_LEDRingSet(ring, v != 0, v-1);
        }else if(emode == ENCODER_MODE_GOBO1ROT){
            if(v == 0){
                FrontPanel_LEDRingSet(ring, 1, 2);
            }else if(v < 0){
                FrontPanel_LEDRingSet(ring, 1, 1);
            }else{
                FrontPanel_LEDRingSet(ring, 1, 3);
            }
        }else if(emode == ENCODER_MODE_ANIMSPD){
            FrontPanel_LEDRingSet(ring, 1, v>>1);
        }else if(emode >= ENCODER_MODE_ENUM){
            FrontPanel_LEDRingSet(ring, 1, v);
        }else{
            v /= encoder_opts[i].ticksperseg;
            if(ring >= 0){
                FrontPanel_LEDRingSet(ring, 2, v);
            }else if(ring == LEDDD_FREQ){
                FrontPanel_DrawNumber(FP_LED_DIG_FREQ_1, v);
            }else if(ring == LEDDD_OCT){
                FrontPanel_DrawNumberHex(FP_LED_DIG_OCT, v);
            }
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
        case 0:
            if(!state) return;
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
            DrawMenu();
            return;
        case 2:
            if(!state) return;
            SendFullState(stagestate);
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
    u8 emode = encoder_opts[encoder].mode;
    //Basic value edit
    s32 v = states[editstate].encoder_vals[encoder];
    v += incrementer;
    //Limits
    s32 lowerlimit = 0, upperlimit;
    if(emode == ENCODER_MODE_GOBO1ROT){
        upperlimit = encoder_enum[emode-3].numvalues >> 1;
        lowerlimit = -upperlimit;
    }else if(emode >= ENCODER_MODE_ENUM){
        upperlimit = encoder_enum[emode-3].numvalues - 1;
    }else{
        upperlimit = (encoder_opts[encoder].ringsegs * encoder_opts[encoder].ticksperseg) - 1;
    }
    if(v < lowerlimit) v = lowerlimit;
    if(v > upperlimit) v = upperlimit;
    if(encoder_modifier == 1) v = 0;
    if(encoder_modifier == 2) v = upperlimit;
    //Store value
    states[editstate].encoder_vals[encoder] = (s8)v;
    //Send value
    u8 ev1; s8 ev2, btn;
    GetEncoderSendValue(encoder, (s8)v, &ev1, &ev2, &btn);
    FrontPanel_DrawNumber(FP_LED_DIG_MAIN_1, ev1);
    if(ev2 >= 0) FrontPanel_DrawDigit(FP_LED_DIG_MAIN_1, '0' + (ev2 / 10));
    if(btn >= 0) FrontPanel_LEDSet(FP_LED_DIG_MAIN_4, btn < MIDI_SUBSOFF);
    if(editstate == stagestate){
        SendEncoder(encoder, ev1);
        if(ev2 >= 0) SendEncoder(encoder+1, ev2);
        if(btn >= 0) NoteOnOff(btn);
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
    u8 idx = button_defs[button].idx;
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
            if(button == FP_B_NSFREQ){
                if(state){
                    NoteOnOff(MIDI_SPOTSSUB1ON);
                    NoteOnOff(MIDI_SPOTSSUB2ON);
                }else{
                    NoteOnOff(MIDI_SPOTSSUB1OFF);
                    NoteOnOff(MIDI_SPOTSSUB2OFF);
                }
            }
        }
        return;
    case BUTTON_MODE_MUTEX:
        if(!state) return;
        states[editstate].mutexstate = (idx == states[editstate].mutexstate) ? -1 : idx;
        if(editstate == stagestate){
            NoteOnOff(MIDI_BANKSEL_START+bank);
            NoteOnOff(MIDI_OVERRIDE_START+ovr);
        }
        return;
    }
}

