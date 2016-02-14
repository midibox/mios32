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

#include <mios32.h>
#include "frontpanel.h"

#include "interface.h"
#include <blm_x.h>

#ifdef FRONTPANEL_REVERSE_ROWS
#define MATRIX_LED_SET(row, sr, pin, state) BLM_X_LEDSet((((7 - (row)) * 88) + (((sr) - 2) << 3) + (7 - (pin))), 0, (state))
#else
#error //Not really feeling it
#endif

s8 FP_SRADDR[7];
Button_T FP_BUTTONS[64*3];

LEDRing_T LED_RINGS[18];
GenesisLEDColumn_T GENESIS_COLUMNS[0xC];

void FrontPanel_Init(){
    //TODO read this all from a text-based config file
    FP_SRADDR[0] = 0;
    FP_SRADDR[1] = 1;
    FP_SRADDR[2] = -1;
    FP_SRADDR[3] = -1;
    FP_SRADDR[4] = -1;
    FP_SRADDR[5] = -1;
    FP_SRADDR[6] = 2;
    
    u8 i;
    for(i=0; i<0xC0; i++){
        FP_BUTTONS[i].func = FP_B_NONE;
        FP_BUTTONS[i].val = 0x00;
    }
    // ==== SR 2, addr 0 ====
    FP_BUTTONS[0x00].func = FP_B_MENU;
    FP_BUTTONS[0x01].func = FP_B_GVOICE;    FP_BUTTONS[0x01].val = 0x02;
    FP_BUTTONS[0x02].func = FP_B_NONE;
    FP_BUTTONS[0x03].func = FP_B_GVOICE;    FP_BUTTONS[0x03].val = 0x12;
    FP_BUTTONS[0x04].func = FP_B_NONE;
    FP_BUTTONS[0x05].func = FP_B_GVOICE;    FP_BUTTONS[0x05].val = 0x22;
    FP_BUTTONS[0x06].func = FP_B_ENTER;
    FP_BUTTONS[0x07].func = FP_B_GVOICE;    FP_BUTTONS[0x07].val = 0x32;
    
    FP_BUTTONS[0x08].func = FP_B_SOFTKEY;   FP_BUTTONS[0x08].val = 0x00;
    FP_BUTTONS[0x09].func = FP_B_GVOICE;    FP_BUTTONS[0x09].val = 0x03;
    FP_BUTTONS[0x0A].func = FP_B_SOFTKEY;   FP_BUTTONS[0x0A].val = 0x01;
    FP_BUTTONS[0x0B].func = FP_B_GVOICE;    FP_BUTTONS[0x0B].val = 0x13;
    FP_BUTTONS[0x0C].func = FP_B_SOFTKEY;   FP_BUTTONS[0x0C].val = 0x02;
    FP_BUTTONS[0x0D].func = FP_B_GVOICE;    FP_BUTTONS[0x0D].val = 0x23;
    FP_BUTTONS[0x0E].func = FP_B_SOFTKEY;   FP_BUTTONS[0x0E].val = 0x03;
    FP_BUTTONS[0x0F].func = FP_B_GVOICE;    FP_BUTTONS[0x0F].val = 0x33;
    
    FP_BUTTONS[0x10].func = FP_B_SOFTKEY;   FP_BUTTONS[0x10].val = 0x04;
    FP_BUTTONS[0x11].func = FP_B_GVOICE;    FP_BUTTONS[0x11].val = 0x04;
    FP_BUTTONS[0x12].func = FP_B_SOFTKEY;   FP_BUTTONS[0x12].val = 0x05;
    FP_BUTTONS[0x13].func = FP_B_GVOICE;    FP_BUTTONS[0x13].val = 0x14;
    FP_BUTTONS[0x14].func = FP_B_SOFTKEY;   FP_BUTTONS[0x14].val = 0x06;
    FP_BUTTONS[0x15].func = FP_B_GVOICE;    FP_BUTTONS[0x15].val = 0x24;
    FP_BUTTONS[0x16].func = FP_B_SOFTKEY;   FP_BUTTONS[0x16].val = 0x07;
    FP_BUTTONS[0x17].func = FP_B_GVOICE;    FP_BUTTONS[0x17].val = 0x34;
    
    FP_BUTTONS[0x18].func = FP_B_MARKST;
    FP_BUTTONS[0x19].func = FP_B_GVOICE;    FP_BUTTONS[0x19].val = 0x05;
    FP_BUTTONS[0x1A].func = FP_B_MOVEUP;
    FP_BUTTONS[0x1B].func = FP_B_GVOICE;    FP_BUTTONS[0x1B].val = 0x15;
    FP_BUTTONS[0x1C].func = FP_B_MARKEND;
    FP_BUTTONS[0x1D].func = FP_B_GVOICE;    FP_BUTTONS[0x1D].val = 0x25;
    FP_BUTTONS[0x1E].func = FP_B_MOVEDN;
    FP_BUTTONS[0x1F].func = FP_B_GVOICE;    FP_BUTTONS[0x1F].val = 0x35;
    
    FP_BUTTONS[0x20].func = FP_B_NONE;
    FP_BUTTONS[0x21].func = FP_B_GVOICE;    FP_BUTTONS[0x21].val = 0x01;
    FP_BUTTONS[0x22].func = FP_B_NONE;
    FP_BUTTONS[0x23].func = FP_B_GVOICE;    FP_BUTTONS[0x23].val = 0x11;
    FP_BUTTONS[0x24].func = FP_B_NONE;
    FP_BUTTONS[0x25].func = FP_B_GVOICE;    FP_BUTTONS[0x25].val = 0x21;
    FP_BUTTONS[0x26].func = FP_B_NONE;
    FP_BUTTONS[0x27].func = FP_B_GVOICE;    FP_BUTTONS[0x27].val = 0x31;
    
    FP_BUTTONS[0x28].func = FP_B_NONE;
    FP_BUTTONS[0x29].func = FP_B_GVOICE;    FP_BUTTONS[0x29].val = 0x07;
    FP_BUTTONS[0x2A].func = FP_B_NONE;
    FP_BUTTONS[0x2B].func = FP_B_GVOICE;    FP_BUTTONS[0x2B].val = 0x17;
    FP_BUTTONS[0x2C].func = FP_B_NONE;
    FP_BUTTONS[0x2D].func = FP_B_GVOICE;    FP_BUTTONS[0x2D].val = 0x27;
    FP_BUTTONS[0x2E].func = FP_B_NONE;
    FP_BUTTONS[0x2F].func = FP_B_GVOICE;    FP_BUTTONS[0x2F].val = 0x37;
    
    FP_BUTTONS[0x30].func = FP_B_SELOP;     FP_BUTTONS[0x30].val = 0x00;
    FP_BUTTONS[0x31].func = FP_B_OUT;
    FP_BUTTONS[0x32].func = FP_B_SELOP;     FP_BUTTONS[0x32].val = 0x01;
    FP_BUTTONS[0x33].func = FP_B_ALG;
    FP_BUTTONS[0x34].func = FP_B_SELOP;     FP_BUTTONS[0x34].val = 0x02;
    FP_BUTTONS[0x35].func = FP_B_DACEN;
    FP_BUTTONS[0x36].func = FP_B_SELOP;     FP_BUTTONS[0x36].val = 0x03;
    FP_BUTTONS[0x37].func = FP_B_KON;
    
    FP_BUTTONS[0x38].func = FP_B_NONE;
    FP_BUTTONS[0x39].func = FP_B_NONE;
    FP_BUTTONS[0x3A].func = FP_B_NONE;
    FP_BUTTONS[0x3B].func = FP_B_NONE;
    FP_BUTTONS[0x3C].func = FP_B_OPMUTE;    FP_BUTTONS[0x3C].val = 0x00;
    FP_BUTTONS[0x3D].func = FP_B_OPMUTE;    FP_BUTTONS[0x3D].val = 0x01;
    FP_BUTTONS[0x3E].func = FP_B_OPMUTE;    FP_BUTTONS[0x3E].val = 0x02;
    FP_BUTTONS[0x3F].func = FP_B_OPMUTE;    FP_BUTTONS[0x3F].val = 0x03;
    
    // ==== SR 3, addr 1 ====
    FP_BUTTONS[0x40].func = FP_B_NONE;
    FP_BUTTONS[0x41].func = FP_B_GVOICE;    FP_BUTTONS[0x41].val = 0x06;
    FP_BUTTONS[0x42].func = FP_B_NONE;
    FP_BUTTONS[0x43].func = FP_B_GVOICE;    FP_BUTTONS[0x43].val = 0x16;
    FP_BUTTONS[0x44].func = FP_B_NONE;
    FP_BUTTONS[0x45].func = FP_B_GVOICE;    FP_BUTTONS[0x45].val = 0x26;
    FP_BUTTONS[0x46].func = FP_B_NONE;
    FP_BUTTONS[0x47].func = FP_B_GVOICE;    FP_BUTTONS[0x47].val = 0x36;
    
    FP_BUTTONS[0x48].func = FP_B_NONE;
    FP_BUTTONS[0x49].func = FP_B_GVOICE;    FP_BUTTONS[0x49].val = 0x00;
    FP_BUTTONS[0x4A].func = FP_B_NONE;
    FP_BUTTONS[0x4B].func = FP_B_GVOICE;    FP_BUTTONS[0x4B].val = 0x10;
    FP_BUTTONS[0x4C].func = FP_B_NONE;
    FP_BUTTONS[0x4D].func = FP_B_GVOICE;    FP_BUTTONS[0x4D].val = 0x20;
    FP_BUTTONS[0x4E].func = FP_B_NONE;
    FP_BUTTONS[0x4F].func = FP_B_GVOICE;    FP_BUTTONS[0x4F].val = 0x30;
    
    FP_BUTTONS[0x50].func = FP_B_NONE;
    FP_BUTTONS[0x51].func = FP_B_GVOICE;    FP_BUTTONS[0x51].val = 0x08;
    FP_BUTTONS[0x52].func = FP_B_NONE;
    FP_BUTTONS[0x53].func = FP_B_GVOICE;    FP_BUTTONS[0x53].val = 0x18;
    FP_BUTTONS[0x54].func = FP_B_NONE;
    FP_BUTTONS[0x55].func = FP_B_GVOICE;    FP_BUTTONS[0x55].val = 0x28;
    FP_BUTTONS[0x56].func = FP_B_NONE;
    FP_BUTTONS[0x57].func = FP_B_GVOICE;    FP_BUTTONS[0x57].val = 0x38;
    
    FP_BUTTONS[0x58].func = FP_B_NONE;
    FP_BUTTONS[0x59].func = FP_B_GVOICE;    FP_BUTTONS[0x59].val = 0x09;
    FP_BUTTONS[0x5A].func = FP_B_NONE;
    FP_BUTTONS[0x5B].func = FP_B_GVOICE;    FP_BUTTONS[0x5B].val = 0x19;
    FP_BUTTONS[0x5C].func = FP_B_NONE;
    FP_BUTTONS[0x5D].func = FP_B_GVOICE;    FP_BUTTONS[0x5D].val = 0x29;
    FP_BUTTONS[0x5E].func = FP_B_NONE;
    FP_BUTTONS[0x5F].func = FP_B_GVOICE;    FP_BUTTONS[0x5F].val = 0x39;
    
    FP_BUTTONS[0x60].func = FP_B_NONE;
    FP_BUTTONS[0x61].func = FP_B_GVOICE;    FP_BUTTONS[0x61].val = 0x0A;
    FP_BUTTONS[0x62].func = FP_B_NONE;
    FP_BUTTONS[0x63].func = FP_B_GVOICE;    FP_BUTTONS[0x63].val = 0x1A;
    FP_BUTTONS[0x64].func = FP_B_NONE;
    FP_BUTTONS[0x65].func = FP_B_GVOICE;    FP_BUTTONS[0x65].val = 0x2A;
    FP_BUTTONS[0x66].func = FP_B_NONE;
    FP_BUTTONS[0x67].func = FP_B_GVOICE;    FP_BUTTONS[0x67].val = 0x3A;
    
    FP_BUTTONS[0x68].func = FP_B_NONE;
    FP_BUTTONS[0x69].func = FP_B_GVOICE;    FP_BUTTONS[0x69].val = 0x0B;
    FP_BUTTONS[0x6A].func = FP_B_NONE;
    FP_BUTTONS[0x6B].func = FP_B_GVOICE;    FP_BUTTONS[0x6B].val = 0x1B;
    FP_BUTTONS[0x6C].func = FP_B_NONE;
    FP_BUTTONS[0x6D].func = FP_B_GVOICE;    FP_BUTTONS[0x6D].val = 0x2B;
    FP_BUTTONS[0x6E].func = FP_B_NONE;
    FP_BUTTONS[0x6F].func = FP_B_GVOICE;    FP_BUTTONS[0x6F].val = 0x3B;
    
    FP_BUTTONS[0x70].func = FP_B_NSTYPE;
    FP_BUTTONS[0x71].func = FP_B_NSFREQ;
    FP_BUTTONS[0x72].func = FP_B_EG;
    FP_BUTTONS[0x73].func = FP_B_LFO;
    FP_BUTTONS[0x74].func = FP_B_DACOVR;
    FP_BUTTONS[0x75].func = FP_B_UGLY;
    FP_BUTTONS[0x76].func = FP_B_CH3FAST;
    FP_BUTTONS[0x77].func = FP_B_CH3MODE;
    
    FP_BUTTONS[0x78].func = FP_B_NONE;
    FP_BUTTONS[0x79].func = FP_B_NONE;
    FP_BUTTONS[0x7A].func = FP_B_KSR;
    FP_BUTTONS[0x7B].func = FP_B_SSGON;
    FP_BUTTONS[0x7C].func = FP_B_SSGINIT;
    FP_BUTTONS[0x7D].func = FP_B_SSGTGL;
    FP_BUTTONS[0x7E].func = FP_B_SSGHOLD;
    FP_BUTTONS[0x7F].func = FP_B_LFOAM;
    
    // ==== SR 8, addr 2 ====
    FP_BUTTONS[0x80].func = FP_B_NONE;
    FP_BUTTONS[0x81].func = FP_B_NONE;
    FP_BUTTONS[0x82].func = FP_B_NONE;
    FP_BUTTONS[0x83].func = FP_B_NONE;
    FP_BUTTONS[0x84].func = FP_B_NONE;
    FP_BUTTONS[0x85].func = FP_B_NONE;
    FP_BUTTONS[0x86].func = FP_B_NONE;
    FP_BUTTONS[0x87].func = FP_B_NONE;
    
    FP_BUTTONS[0x88].func = FP_B_NONE;
    FP_BUTTONS[0x89].func = FP_B_NONE;
    FP_BUTTONS[0x8A].func = FP_B_NONE;
    FP_BUTTONS[0x8B].func = FP_B_NONE;
    FP_BUTTONS[0x8C].func = FP_B_NONE;
    FP_BUTTONS[0x8D].func = FP_B_NONE;
    FP_BUTTONS[0x8E].func = FP_B_NONE;
    FP_BUTTONS[0x8F].func = FP_B_NONE;
    
    FP_BUTTONS[0x90].func = FP_B_NONE;
    FP_BUTTONS[0x91].func = FP_B_NONE;
    FP_BUTTONS[0x92].func = FP_B_NONE;
    FP_BUTTONS[0x93].func = FP_B_NONE;
    FP_BUTTONS[0x94].func = FP_B_NONE;
    FP_BUTTONS[0x95].func = FP_B_NONE;
    FP_BUTTONS[0x96].func = FP_B_NONE;
    FP_BUTTONS[0x97].func = FP_B_NONE;
    
    FP_BUTTONS[0x98].func = FP_B_NONE;
    FP_BUTTONS[0x99].func = FP_B_NONE;
    FP_BUTTONS[0x9A].func = FP_B_NONE;
    FP_BUTTONS[0x9B].func = FP_B_NONE;
    FP_BUTTONS[0x9C].func = FP_B_NONE;
    FP_BUTTONS[0x9D].func = FP_B_NONE;
    FP_BUTTONS[0x9E].func = FP_B_NONE;
    FP_BUTTONS[0x9F].func = FP_B_NONE;
    
    FP_BUTTONS[0xA0].func = FP_B_CTRL;
    FP_BUTTONS[0xA1].func = FP_B_CMDDN;
    FP_BUTTONS[0xA2].func = FP_B_CMDUP;
    FP_BUTTONS[0xA3].func = FP_B_STATEDN;
    FP_BUTTONS[0xA4].func = FP_B_TIME;
    FP_BUTTONS[0xA5].func = FP_B_CMDS;
    FP_BUTTONS[0xA6].func = FP_B_STATEUP;
    FP_BUTTONS[0xA7].func = FP_B_STATE;
    
    FP_BUTTONS[0xA8].func = FP_B_LOAD;
    FP_BUTTONS[0xA9].func = FP_B_CROP;
    FP_BUTTONS[0xAA].func = FP_B_SAVE;
    FP_BUTTONS[0xAB].func = FP_B_CAPTURE;
    FP_BUTTONS[0xAC].func = FP_B_NEW;
    FP_BUTTONS[0xAD].func = FP_B_DUPL;
    FP_BUTTONS[0xAE].func = FP_B_DELETE;
    FP_BUTTONS[0xAF].func = FP_B_PASTE;
    
    FP_BUTTONS[0xB0].func = FP_B_GROUP;
    FP_BUTTONS[0xB1].func = FP_B_MUTE;
    FP_BUTTONS[0xB2].func = FP_B_RESTART;
    FP_BUTTONS[0xB3].func = FP_B_SOLO;
    FP_BUTTONS[0xB4].func = FP_B_PLAY;
    FP_BUTTONS[0xB5].func = FP_B_RELEASE;
    FP_BUTTONS[0xB6].func = FP_B_RESET;
    FP_BUTTONS[0xB7].func = FP_B_PNLOVR;
    
    FP_BUTTONS[0xB8].func = FP_B_PROG;
    FP_BUTTONS[0xB9].func = FP_B_DATAWHEEL;
    FP_BUTTONS[0xBA].func = FP_B_VGM;
    FP_BUTTONS[0xBB].func = FP_B_SYSTEM;
    FP_BUTTONS[0xBC].func = FP_B_MDLTR;
    FP_BUTTONS[0xBD].func = FP_B_VOICE;
    FP_BUTTONS[0xBE].func = FP_B_SAMPLE;
    FP_BUTTONS[0xBF].func = FP_B_CHAN;
    
    // Configure encoders
    mios32_enc_config_t enc_config = MIOS32_ENC_ConfigGet(0);
    enc_config.cfg.type = DETENTED3;
    enc_config.cfg.speed = NORMAL;
    enc_config.cfg.speed_par = 0;
    for(i=0; i<21; i++){
        MIOS32_ENC_ConfigSet(i, enc_config);
    }
    
    enc_config.cfg.sr  = 8;
    enc_config.cfg.pos = 0;
    MIOS32_ENC_ConfigSet(FP_E_DATAWHEEL, enc_config);
    
    enc_config.cfg.sr  = 7;
    enc_config.cfg.pos = 4;
    MIOS32_ENC_ConfigSet(FP_E_OP1LVL, enc_config);
    
    enc_config.cfg.sr  = 7;
    enc_config.cfg.pos = 0;
    MIOS32_ENC_ConfigSet(FP_E_OP2LVL, enc_config);
    
    enc_config.cfg.sr  = 7;
    enc_config.cfg.pos = 2;
    MIOS32_ENC_ConfigSet(FP_E_OP3LVL, enc_config);
    
    enc_config.cfg.sr  = 1;
    enc_config.cfg.pos = 1;
    MIOS32_ENC_ConfigSet(FP_E_OP4LVL, enc_config);
    
    enc_config.cfg.sr  = 6;
    enc_config.cfg.pos = 4;
    MIOS32_ENC_ConfigSet(FP_E_HARM, enc_config);
    
    enc_config.cfg.sr  = 6;
    enc_config.cfg.pos = 2;
    MIOS32_ENC_ConfigSet(FP_E_DETUNE, enc_config);
    
    enc_config.cfg.sr  = 6;
    enc_config.cfg.pos = 0;
    MIOS32_ENC_ConfigSet(FP_E_ATTACK, enc_config);
    
    enc_config.cfg.sr  = 5;
    enc_config.cfg.pos = 4;
    MIOS32_ENC_ConfigSet(FP_E_DEC1R, enc_config);
    
    enc_config.cfg.sr  = 5;
    enc_config.cfg.pos = 2;
    MIOS32_ENC_ConfigSet(FP_E_DECLVL, enc_config);
    
    enc_config.cfg.sr  = 5;
    enc_config.cfg.pos = 0;
    MIOS32_ENC_ConfigSet(FP_E_DEC2R, enc_config);
    
    enc_config.cfg.sr  = 4;
    enc_config.cfg.pos = 3;
    MIOS32_ENC_ConfigSet(FP_E_RELRATE, enc_config);
    
    enc_config.cfg.sr  = 4;
    enc_config.cfg.pos = 1;
    MIOS32_ENC_ConfigSet(FP_E_CSMFREQ, enc_config);
    
    enc_config.cfg.sr  = 6;
    enc_config.cfg.pos = 6;
    MIOS32_ENC_ConfigSet(FP_E_OCTAVE, enc_config);
    
    enc_config.cfg.sr  = 5;
    enc_config.cfg.pos = 6;
    MIOS32_ENC_ConfigSet(FP_E_FREQ, enc_config);
    
    enc_config.cfg.sr  = 4;
    enc_config.cfg.pos = 5;
    MIOS32_ENC_ConfigSet(FP_E_PSGFREQ, enc_config);
    
    enc_config.cfg.sr  = 4;
    enc_config.cfg.pos = 7;
    MIOS32_ENC_ConfigSet(FP_E_PSGVOL, enc_config);
    
    enc_config.cfg.sr  = 1;
    enc_config.cfg.pos = 2;
    MIOS32_ENC_ConfigSet(FP_E_LFOFDEP, enc_config);
    
    enc_config.cfg.sr  = 1;
    enc_config.cfg.pos = 4;
    MIOS32_ENC_ConfigSet(FP_E_LFOADEP, enc_config);
    
    enc_config.cfg.sr  = 1;
    enc_config.cfg.pos = 7;
    MIOS32_ENC_ConfigSet(FP_E_LFOFREQ, enc_config);
    
    enc_config.cfg.sr  = 7;
    enc_config.cfg.pos = 6;
    MIOS32_ENC_ConfigSet(FP_E_FEEDBACK, enc_config);
    
    
    //Configure LED rings
    LED_RINGS[FP_LEDR_OP1LVL]   = (LEDRing_T){ .special = 0, .losr = 7, .lopin = 0, .hisr = 7, .hipin = 1 };
    LED_RINGS[FP_LEDR_OP2LVL]   = (LEDRing_T){ .special = 0, .losr = 7, .lopin = 2, .hisr = 7, .hipin = 3 };
    LED_RINGS[FP_LEDR_OP3LVL]   = (LEDRing_T){ .special = 0, .losr = 8, .lopin = 0, .hisr = 8, .hipin = 1 };
    LED_RINGS[FP_LEDR_OP4LVL]   = (LEDRing_T){ .special = 0, .losr = 8, .lopin = 2, .hisr = 8, .hipin = 3 };
    LED_RINGS[FP_LEDR_HARM]     = (LEDRing_T){ .special = 0, .losr = 6, .lopin = 7, .hisr = 6, .hipin = 6 };
    LED_RINGS[FP_LEDR_DETUNE]   = (LEDRing_T){ .special = 0, .losr = 6, .lopin = 5, .hisr = 6, .hipin = 4 };
    LED_RINGS[FP_LEDR_ATTACK]   = (LEDRing_T){ .special = 0, .losr = 6, .lopin = 3, .hisr = 6, .hipin = 2 };
    LED_RINGS[FP_LEDR_DEC1R]    = (LEDRing_T){ .special = 0, .losr = 6, .lopin = 1, .hisr = 6, .hipin = 0 };
    LED_RINGS[FP_LEDR_DECLVL]   = (LEDRing_T){ .special = 0, .losr = 5, .lopin = 5, .hisr = 5, .hipin = 4 };
    LED_RINGS[FP_LEDR_DEC2R]    = (LEDRing_T){ .special = 0, .losr = 5, .lopin = 7, .hisr = 5, .hipin = 6 };
    LED_RINGS[FP_LEDR_RELRATE]  = (LEDRing_T){ .special = 0, .losr = 5, .lopin = 1, .hisr = 5, .hipin = 0 };
    LED_RINGS[FP_LEDR_CSMFREQ]  = (LEDRing_T){ .special = 0, .losr = 5, .lopin = 3, .hisr = 5, .hipin = 2 };
    LED_RINGS[FP_LEDR_PSGFREQ]  = (LEDRing_T){ .special = 0, .losr = 4, .lopin = 7, .hisr = 4, .hipin = 6 };
    LED_RINGS[FP_LEDR_PSGVOL]   = (LEDRing_T){ .special = 0, .losr = 4, .lopin = 5, .hisr = 4, .hipin = 4 };
    LED_RINGS[FP_LEDR_LFOFDEP]  = (LEDRing_T){ .special = 0, .losr = 8, .lopin = 4, .hisr = 8, .hipin = 5 };
    LED_RINGS[FP_LEDR_LFOADEP]  = (LEDRing_T){ .special = 1 };
    LED_RINGS[FP_LEDR_LFOFREQ]  = (LEDRing_T){ .special = 0, .losr = 2, .lopin = 6, .hisr = 2, .hipin = 7 };
    LED_RINGS[FP_LEDR_FEEDBACK] = (LEDRing_T){ .special = 2 };
    
    //Configure Genesis LED columns
    GENESIS_COLUMNS[0x0] = (GenesisLEDColumn_T){ .sr = 3, .pin = 1 };
    GENESIS_COLUMNS[0x1] = (GenesisLEDColumn_T){ .sr = 2, .pin = 1 };
    GENESIS_COLUMNS[0x2] = (GenesisLEDColumn_T){ .sr = 2, .pin = 2 };
    GENESIS_COLUMNS[0x3] = (GenesisLEDColumn_T){ .sr = 2, .pin = 3 };
    GENESIS_COLUMNS[0x4] = (GenesisLEDColumn_T){ .sr = 2, .pin = 4 };
    GENESIS_COLUMNS[0x5] = (GenesisLEDColumn_T){ .sr = 2, .pin = 5 };
    GENESIS_COLUMNS[0x6] = (GenesisLEDColumn_T){ .sr = 3, .pin = 0 };
    GENESIS_COLUMNS[0x7] = (GenesisLEDColumn_T){ .sr = 2, .pin = 0 };
    GENESIS_COLUMNS[0x8] = (GenesisLEDColumn_T){ .sr = 3, .pin = 4 };
    GENESIS_COLUMNS[0x9] = (GenesisLEDColumn_T){ .sr = 3, .pin = 5 };
    GENESIS_COLUMNS[0xA] = (GenesisLEDColumn_T){ .sr = 3, .pin = 6 };
    GENESIS_COLUMNS[0xB] = (GenesisLEDColumn_T){ .sr = 3, .pin = 7 };
    
}

void FrontPanel_ButtonChange(u32 btn, u32 value){
    value = (value == 0); //Invert
    u8 row = btn / BLM_X_BTN_NUM_COLS;
    u8 sr = (btn % BLM_X_BTN_NUM_COLS) >> 3;
    u8 pin = (btn & 7);
    #ifdef FRONTPANEL_REVERSE_ROWS
    row = 7 - row;
    #endif
    s8 sraddr = FP_SRADDR[sr];
    //DBG("FrontPanel_ButtonChange(%d, %d): row %d, sr %d, pin %d, sraddr %d", btn, value, row, sr, pin, sraddr);
    if(sraddr < 0){
        //DBG("----Shift register %d excluded", sr);
        return;
    }
    Button_T button = FP_BUTTONS[64*sraddr + 8*pin + row];
    if(button.func == FP_B_NONE){
        //DBG("----No function assigned");
        return;
    }else if(button.func >= FP_B_SYSTEM && button.func < FP_B_OUT){
        Interface_BtnSystem(button.func, value);
    }else if(button.func >= FP_B_OUT){
        Interface_BtnEdit(button.func, value);
    }else if(button.func == FP_B_GVOICE){
        Interface_BtnGVoice(button.val, value);
    }else if(button.func == FP_B_SOFTKEY){
        Interface_BtnSoftkey(button.val, value);
    }else if(button.func == FP_B_SELOP){
        Interface_BtnSelOp(button.val, value);
    }else if(button.func == FP_B_OPMUTE){
        Interface_BtnOpMute(button.val, value);
    }else{
        DBG("Unhandled button type %d!", button.func);
        return;
    }
}

void FrontPanel_EncoderChange(u32 encoder, u32 incrementer){
    if(encoder == 0){
        Interface_EncDatawheel(incrementer);
    }else{
        Interface_EncEdit(encoder, incrementer);
    }
}

void FrontPanel_LEDRingSet(u8 ring, u8 mode, u8 value){
    if(ring >= 18) return;
    LEDRing_T ledring = LED_RINGS[ring];
    if(ledring.special){
        //TODO special case handlers
    }else{
        s8 r; 
        u8 d = 0;
        for(r=0; r<8; r++){
            MATRIX_LED_SET(r, ledring.losr, ledring.lopin, (mode ? (d <= value) : (d == value)));
            ++d;
        }
        for(r=7; r>=0; r--){
            MATRIX_LED_SET(r, ledring.hisr, ledring.hipin, (mode ? (d <= value) : (d == value)));
            ++d;
        }
    }
}

void FrontPanel_GenesisLEDSet(u8 genesis, u8 voice, u8 color, u8 value){
    if(genesis >= GENESIS_COUNT) return;
    if(voice >= 0xC) return;
    if(color >= 2) return;
    GenesisLEDColumn_T gled = GENESIS_COLUMNS[voice];
    MATRIX_LED_SET((genesis << 1) | color, gled.sr, gled.pin, value);
}
