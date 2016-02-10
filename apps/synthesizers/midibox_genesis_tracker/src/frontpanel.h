/*
 * MIDIbox Quad Genesis: Front Panel Wrapper Functions
 *
 * ==========================================================================
 *
 *  Copyright (C) 2016 Sauraen (sauraen@gmail.com)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _FRONTPANEL_H
#define _FRONTPANEL_H

#ifdef __cplusplus
extern "C" {
#endif


typedef union {
    u16 all;
    struct{
        u8 func;
        u8 val;
    };
} Button_T;

#define FP_B_NONE 0
#define FP_B_GVOICE 1
#define FP_B_SOFTKEY 2
#define FP_B_SELOP 3
#define FP_B_OPMUTE 4
#define FP_B_SYSTEM 10
#define FP_B_VOICE 11
#define FP_B_CHAN 12
#define FP_B_PROG 13
#define FP_B_VGM 14
#define FP_B_MDLTR 15
#define FP_B_SAMPLE 16
#define FP_B_MENU 17
#define FP_B_ENTER 18
#define FP_B_DATAWHEEL 19
#define FP_B_LOAD 20
#define FP_B_SAVE 21
#define FP_B_NEW 22
#define FP_B_DELETE 23
#define FP_B_CROP 24
#define FP_B_CAPTURE 25
#define FP_B_DUPL 26
#define FP_B_PASTE 27
#define FP_B_GROUP 28
#define FP_B_CTRL 29
#define FP_B_TIME 30
#define FP_B_CMDS 31
#define FP_B_STATE 32
#define FP_B_MUTE 33
#define FP_B_SOLO 34
#define FP_B_RELEASE 35
#define FP_B_PNLOVR 36
#define FP_B_RESTART 37
#define FP_B_PLAY 38
#define FP_B_RESET 39
#define FP_B_MARKST 40
#define FP_B_MARKEND 41
#define FP_B_MOVEUP 42
#define FP_B_MOVEDN 43
#define FP_B_CMDUP 44
#define FP_B_CMDDN 45
#define FP_B_STATEUP 46
#define FP_B_STATEDN 47
#define FP_B_OUT 50
#define FP_B_ALG 51
#define FP_B_KON 52
#define FP_B_DACEN 53
#define FP_B_KSR 60
#define FP_B_SSGON 61
#define FP_B_SSGINIT 62
#define FP_B_SSGTGL 63
#define FP_B_SSGHOLD 64
#define FP_B_LFOAM 65
#define FP_B_CH3MODE 70
#define FP_B_CH3FAST 71
#define FP_B_UGLY 72
#define FP_B_DACOVR 73
#define FP_B_LFO 74
#define FP_B_EG 75
#define FP_B_NSFREQ 80
#define FP_B_NSTYPE 81


#define FP_E_DATAWHEEL 0
#define FP_E_OP1LVL 1
#define FP_E_OP2LVL 2
#define FP_E_OP3LVL 3
#define FP_E_OP4LVL 4
#define FP_E_HARM 5
#define FP_E_DETUNE 6
#define FP_E_ATTACK 7
#define FP_E_DEC1R 8
#define FP_E_DECLVL 9
#define FP_E_DEC2R 10
#define FP_E_RELRATE 11
#define FP_E_CSMFREQ 12
#define FP_E_OCTAVE 13
#define FP_E_FREQ 14
#define FP_E_PSGFREQ 15
#define FP_E_PSGVOL 16
#define FP_E_LFOFDEP 17
#define FP_E_LFOADEP 18
#define FP_E_LFOFREQ 19
#define FP_E_FEEDBACK 20

typedef union {
    u16 all;
    struct{
        u8 pin:3;
        u8 sr:5;
        u8 row:3;
        u8 dummy:5;
    };
} LED_T;

typedef union {
    u32 all;
    struct{
        u8 special; //0 if standard format, > 0 if special handler
        u8 lopin:3;
        u8 losr:5;
        u8 hipin:3;
        u8 hisr:5;
    };
} LEDRing_T;

#define FP_LEDR_OP1LVL 0
#define FP_LEDR_OP2LVL 1
#define FP_LEDR_OP3LVL 2
#define FP_LEDR_OP4LVL 3
#define FP_LEDR_HARM 4
#define FP_LEDR_DETUNE 5
#define FP_LEDR_ATTACK 6
#define FP_LEDR_DEC1R 7
#define FP_LEDR_DECLVL 8
#define FP_LEDR_DEC2R 9
#define FP_LEDR_RELRATE 10
#define FP_LEDR_CSMFREQ 11
#define FP_LEDR_PSGFREQ 12
#define FP_LEDR_PSGVOL 13
#define FP_LEDR_LFOFDEP 14
#define FP_LEDR_LFOADEP 15
#define FP_LEDR_LFOFREQ 16
#define FP_LEDR_FEEDBACK 17


extern void FrontPanel_Init();

// Called from hardware hooks
extern void FrontPanel_ButtonChange(u32 btn, u32 value);
extern void FrontPanel_EncoderChange(u32 encoder, u32 incrementer);


extern void FrontPanel_LEDSet(u32 led, u8 value);
extern void FrontPanel_GenesisLEDSet(u8 genesis, u8 channel, u8 color, u8 value);
extern void FrontPanel_DrawAlgorithm(u8 algorithm);
extern void FrontPanel_DrawDACValue(u16 bits);
extern void FrontPanel_VGMMatrixPoint(u8 row, u8 col, u8 value);
extern void FrontPanel_LEDRingSet(u8 ring, u8 mode, u8 value);
extern void FrontPanel_DrawDigit(u8 digit, char value);


#ifdef __cplusplus
}
#endif


#endif /* _FRONTPANEL_H */
