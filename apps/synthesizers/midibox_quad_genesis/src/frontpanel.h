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
#define FP_E_COUNT 21

typedef union {
    u16 all;
    struct{
        u8 pin:3;
        u8 sr:5;
        u8 row:3;
        u8 dummy:5;
    };
} LED_T;

#define FP_LED_SYSTEM 0
#define FP_LED_VOICE 1
#define FP_LED_CHAN 2
#define FP_LED_PROG 3
#define FP_LED_VGM 4
#define FP_LED_MDLTR 5
#define FP_LED_SAMPLE 6
#define FP_LED_MUTE 7
#define FP_LED_SOLO 8
#define FP_LED_RELEASE 9
#define FP_LED_PNLOVR 10
#define FP_LED_RESTART 11
#define FP_LED_PLAY 12
#define FP_LED_RESET 13
#define FP_LED_LOAD 14
#define FP_LED_SAVE 15
#define FP_LED_NEW 16
#define FP_LED_DELETE 17
#define FP_LED_CROP 18
#define FP_LED_CAPTURE 19
#define FP_LED_DUPL 20
#define FP_LED_PASTE 21
#define FP_LED_GROUP 22
#define FP_LED_CTRL_R 23
#define FP_LED_CTRL_G 24
#define FP_LED_TIME_R 25
#define FP_LED_TIME_G 26
#define FP_LED_CMDS 27
#define FP_LED_STATE 28
#define FP_LED_KSR 29
#define FP_LED_SSGON 30
#define FP_LED_SSGINIT 31
#define FP_LED_SSGTGL 32
#define FP_LED_SSGHOLD 33
#define FP_LED_LFOAM 34
#define FP_LED_CH3_NORMAL 35
#define FP_LED_CH3_4FREQ 36
#define FP_LED_CH3_CSM 37
#define FP_LED_CH3FAST 38
#define FP_LED_UGLY 39
#define FP_LED_DACOVR 40
#define FP_LED_LFO 41
#define FP_LED_EG 42
#define FP_LED_NS_HI 43
#define FP_LED_NS_MED 44
#define FP_LED_NS_LOW 45
#define FP_LED_NS_SQ3 46
#define FP_LED_NS_WHT 47
#define FP_LED_NS_PLS 48
#define FP_LED_SELOP_1 49
#define FP_LED_SELOP_2 50
#define FP_LED_SELOP_3 51
#define FP_LED_SELOP_4 52
#define FP_LED_OPCARR_1 53
#define FP_LED_OPCARR_2 54
#define FP_LED_OPCARR_3 55
#define FP_LED_OPCARR_4 56
#define FP_LED_OPNODE_R_1 57
#define FP_LED_OPNODE_R_2 58
#define FP_LED_OPNODE_R_3 59
#define FP_LED_OPNODE_R_4 60
#define FP_LED_OPNODE_G_1 61
#define FP_LED_OPNODE_G_2 62
#define FP_LED_OPNODE_G_3 63
#define FP_LED_OPNODE_G_4 64
#define FP_LED_FMW_A1 65
#define FP_LED_FMW_A2 66
#define FP_LED_FMW_A3 67
#define FP_LED_FMW_A4 68
#define FP_LED_FMW_A5 69
#define FP_LED_FMW_A6 70
#define FP_LED_FMW_B1 71
#define FP_LED_FMW_B2 72
#define FP_LED_FMW_B3 73
#define FP_LED_FMW_C1 74
#define FP_LED_FMW_C2 75
#define FP_LED_FMW_C3 76
#define FP_LED_FMW_C4 77
#define FP_LED_FMW_D1 78
#define FP_LED_FMW_D2 79
#define FP_LED_FMW_E1 80
#define FP_LED_FMW_E2 81
#define FP_LED_FMW_F 82
#define FP_LED_FEEDBACK 83
#define FP_LED_OUTL 84
#define FP_LED_OUTR 85
#define FP_LED_KEYON_1 86
#define FP_LED_KEYON_2 87
#define FP_LED_KEYON_3 88
#define FP_LED_KEYON_4 89
#define FP_LED_DACEN 90
#define FP_LED_DAC_1 91
#define FP_LED_DAC_2 92
#define FP_LED_DAC_3 93
#define FP_LED_DAC_4 94
#define FP_LED_DAC_5 95
#define FP_LED_DAC_6 96
#define FP_LED_DAC_7 97
#define FP_LED_DAC_8 98
#define FP_LED_DAC_9 99
#define FP_LED_LOAD_CHIP 100
#define FP_LED_LOAD_RAM 101
#define FP_LED_VGMMTX_1 102
#define FP_LED_VGMMTX_2 103
#define FP_LED_VGMMTX_3 104
#define FP_LED_VGMMTX_4 105
#define FP_LED_VGMMTX_5 106
#define FP_LED_VGMMTX_6 107
#define FP_LED_VGMMTX_7 108
#define FP_LED_VGMMTX_8 109
#define FP_LED_VGMMTX_9 110
#define FP_LED_VGMMTX_10 111
#define FP_LED_VGMMTX_11 112
#define FP_LED_VGMMTX_12 113
#define FP_LED_VGMMTX_13 114
#define FP_LED_VGMMTX_14 115
#define FP_LED_DIG_MAIN_1 116
#define FP_LED_DIG_MAIN_2 117
#define FP_LED_DIG_MAIN_3 118
#define FP_LED_DIG_MAIN_4 119
#define FP_LED_DIG_FREQ_1 120
#define FP_LED_DIG_FREQ_2 121
#define FP_LED_DIG_FREQ_3 122
#define FP_LED_DIG_FREQ_4 123
#define FP_LED_DIG_OCT 124
#define FP_LED_RING_FB_1 125
#define FP_LED_RING_FB_2 126
#define FP_LED_RING_FB_3 127
#define FP_LED_RING_FB_4 128
#define FP_LED_RING_FB_5 129
#define FP_LED_RING_FB_6 130
#define FP_LED_RING_FB_7 131
#define FP_LED_RING_FB_8 132
#define FP_LED_RING_LFOA_1 133
#define FP_LED_RING_LFOA_2 134
#define FP_LED_RING_LFOA_3 135
#define FP_LED_RING_LFOA_4 136
#define FP_LED_COUNT 137


typedef union {
    u32 all;
    struct{
        u8 special:5; //0 if standard format, > 0 if special handler
        u8 offset:3; //The number to add to get to the first segment
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
#define FP_LEDR_COUNT 18

typedef union {
    u8 all;
    struct{
        u8 pin:3;
        u8 sr:5;
    };
} GenesisLEDColumn_T;


extern void FrontPanel_Init();

// Called from hardware hooks
extern void FrontPanel_ButtonChange(u32 btn, u32 value);
extern void FrontPanel_EncoderChange(u32 encoder, u32 incrementer);


extern void FrontPanel_LEDSet(u32 led, u8 value); //Special case: if led is the index of a digit, this draws the decimal point
extern void FrontPanel_GenesisLEDSet(u8 genesis, u8 voice, u8 color, u8 value);
extern void FrontPanel_DrawAlgorithm(u8 algorithm);
extern void FrontPanel_DrawDACValue(u16 bits);
extern void FrontPanel_VGMMatrixPoint(u8 row, u8 col, u8 value);
extern void FrontPanel_VGMMatrixVUMeter(u8 col, u8 value);
extern void FrontPanel_LEDRingSet(u8 ring, u8 mode, u8 value); //Mode: 0 line, 1 fill, else clear
extern void FrontPanel_DrawDigit(u8 digit, char value);
extern void FrontPanel_DrawNumber(u8 firstdigit, s16 number);
extern void FrontPanel_DrawLoad(u8 type, u8 value);


#ifdef __cplusplus
}
#endif


#endif /* _FRONTPANEL_H */
