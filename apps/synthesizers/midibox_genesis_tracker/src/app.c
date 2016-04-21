/*
 * MIDIbox Genesis Tracker: Main application
 *
 * ==========================================================================
 *
 *  Copyright (C) 2016 Sauraen (sauraen@gmail.com)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

/////////////////////////////////////////////////////////////////////////////
// Include files
/////////////////////////////////////////////////////////////////////////////

#include <mios32.h>
#include "app.h"

#include <genesis.h>
#include <vgm.h>

// Local (static) variables
static VgmSource* qsource;
static VgmHead* qhead;

/////////////////////////////////////////////////////////////////////////////
// This hook is called after startup to initialize the application
/////////////////////////////////////////////////////////////////////////////

void APP_Init(void){
    // initialize all LEDs
    MIOS32_BOARD_LED_Init(0xffffffff);

    //Initialize MBHP_Genesis module
    Genesis_Init();

    //Initialize VGM Player component
    VGM_Init();
    
    //Create a queue for the VGM events our MIDI events spawn
    qsource = VGM_SourceQueue_Create();
    qhead = VGM_Head_Create(qsource);
    qhead->playing = 1;
    
    //Send test patch to initialize voice 1
    VGM_HeadQueue_Enqueue(qhead, (VgmChipWriteCmd){.cmd=2, .addr=0x30, .data=0x71 }, 0);
    VGM_HeadQueue_Enqueue(qhead, (VgmChipWriteCmd){.cmd=2, .addr=0x34, .data=0x0D }, 0);
    VGM_HeadQueue_Enqueue(qhead, (VgmChipWriteCmd){.cmd=2, .addr=0x38, .data=0x33 }, 0);
    VGM_HeadQueue_Enqueue(qhead, (VgmChipWriteCmd){.cmd=2, .addr=0x3C, .data=0x01 }, 0);
    VGM_HeadQueue_Enqueue(qhead, (VgmChipWriteCmd){.cmd=2, .addr=0x40, .data=0x23 }, 0);
    VGM_HeadQueue_Enqueue(qhead, (VgmChipWriteCmd){.cmd=2, .addr=0x44, .data=0x2D }, 0);
    VGM_HeadQueue_Enqueue(qhead, (VgmChipWriteCmd){.cmd=2, .addr=0x48, .data=0x26 }, 0);
    VGM_HeadQueue_Enqueue(qhead, (VgmChipWriteCmd){.cmd=2, .addr=0x4C, .data=0x00 }, 0);
    VGM_HeadQueue_Enqueue(qhead, (VgmChipWriteCmd){.cmd=2, .addr=0x50, .data=0x5F }, 0);
    VGM_HeadQueue_Enqueue(qhead, (VgmChipWriteCmd){.cmd=2, .addr=0x54, .data=0x99 }, 0);
    VGM_HeadQueue_Enqueue(qhead, (VgmChipWriteCmd){.cmd=2, .addr=0x58, .data=0x5F }, 0);
    VGM_HeadQueue_Enqueue(qhead, (VgmChipWriteCmd){.cmd=2, .addr=0x5C, .data=0x94 }, 0);
    VGM_HeadQueue_Enqueue(qhead, (VgmChipWriteCmd){.cmd=2, .addr=0x60, .data=0x05 }, 0);
    VGM_HeadQueue_Enqueue(qhead, (VgmChipWriteCmd){.cmd=2, .addr=0x64, .data=0x05 }, 0);
    VGM_HeadQueue_Enqueue(qhead, (VgmChipWriteCmd){.cmd=2, .addr=0x68, .data=0x05 }, 0);
    VGM_HeadQueue_Enqueue(qhead, (VgmChipWriteCmd){.cmd=2, .addr=0x6C, .data=0x07 }, 0);
    VGM_HeadQueue_Enqueue(qhead, (VgmChipWriteCmd){.cmd=2, .addr=0x70, .data=0x02 }, 0);
    VGM_HeadQueue_Enqueue(qhead, (VgmChipWriteCmd){.cmd=2, .addr=0x74, .data=0x02 }, 0);
    VGM_HeadQueue_Enqueue(qhead, (VgmChipWriteCmd){.cmd=2, .addr=0x78, .data=0x02 }, 0);
    VGM_HeadQueue_Enqueue(qhead, (VgmChipWriteCmd){.cmd=2, .addr=0x7C, .data=0x02 }, 0);
    VGM_HeadQueue_Enqueue(qhead, (VgmChipWriteCmd){.cmd=2, .addr=0x80, .data=0x11 }, 0);
    VGM_HeadQueue_Enqueue(qhead, (VgmChipWriteCmd){.cmd=2, .addr=0x84, .data=0x11 }, 0);
    VGM_HeadQueue_Enqueue(qhead, (VgmChipWriteCmd){.cmd=2, .addr=0x88, .data=0x11 }, 0);
    VGM_HeadQueue_Enqueue(qhead, (VgmChipWriteCmd){.cmd=2, .addr=0x8C, .data=0xA6 }, 0);
    VGM_HeadQueue_Enqueue(qhead, (VgmChipWriteCmd){.cmd=2, .addr=0x90, .data=0x00 }, 0);
    VGM_HeadQueue_Enqueue(qhead, (VgmChipWriteCmd){.cmd=2, .addr=0x94, .data=0x00 }, 0);
    VGM_HeadQueue_Enqueue(qhead, (VgmChipWriteCmd){.cmd=2, .addr=0x98, .data=0x00 }, 0);
    VGM_HeadQueue_Enqueue(qhead, (VgmChipWriteCmd){.cmd=2, .addr=0x9C, .data=0x00 }, 0);
    VGM_HeadQueue_Enqueue(qhead, (VgmChipWriteCmd){.cmd=2, .addr=0xB0, .data=0x32 }, 0);
    VGM_HeadQueue_Enqueue(qhead, (VgmChipWriteCmd){.cmd=2, .addr=0xB4, .data=0xC0 }, 0);
}


/////////////////////////////////////////////////////////////////////////////
// This task is running endless in background
/////////////////////////////////////////////////////////////////////////////
void APP_Background(void){
    MIOS32_BOARD_LED_Set(0b1000, 0b1000);
}

#ifdef GENMDM_ALIGN_MSB
//Take the most significant bits of value
#define GENMDM_ALIGN(value,bits) ((value) >> (7-(bits)))
#else
//Take the least significant bits of value
#define GENMDM_ALIGN(value,bits) ((value) & ((1<<(bits))-1))
#endif

/////////////////////////////////////////////////////////////////////////////
// This hook is called when a MIDI package has been received
/////////////////////////////////////////////////////////////////////////////
void APP_MIDI_NotifyPackage(mios32_midi_port_t port, mios32_midi_package_t midi_package){
    u8 board = (port & 0x03);
    u8 chan = midi_package.chn;
    u8 chanhi = chan / 3;
    u8 chanlo = chan % 3;
    u8 psgcmd = (board >> 4);
    u8 opn2globcmd = psgcmd | 2;
    u8 opn2cmd = opn2globcmd | chanhi;
    if(midi_package.event == 0xB){
        //CC
        u8 cc = midi_package.cc_number;
        u8 value = midi_package.value;
        if(chan >= 6 && chan <= 9){
            //PSG
            if(cc == 85){
                //Pitch Transposition
                //TODO
            }else if(cc == 83){
                //NTSC/PAL Tuning
                //TODO
            }
        }else if(chan <= 5){
            //OPN2
            u8 op;
            //Operator CCs
            if(cc >= 16 && cc <= 27){
                op = cc & 0x03;
                cc &= 0xFC;
            }else if(cc >= 39 && cc <= 62){
                op = (cc-39) & 0x03;
                cc = ((cc-39) & 0xFC) + 39;
            }else if(cc >= 70 && cc <= 73){
                op = cc-70;
                cc = 70;
            }else if(cc >= 90 && cc <= 93){
                op = cc-90;
                cc = 90;
            }else if(cc >= 100 && cc <= 113){
                op = cc-100; //actually for DAC custom wave, not op
                cc = 100;
            }else{
                op = 0;
            }
            u8 opaddr = ((op & 1) << 3) | ((op & 2) << 1) | chanlo;
            switch(cc){
            case 74:
                //LFO Enable
                genesis[board].opn2.lfo_enabled = GENMDM_ALIGN(value,1);
                VGM_HeadQueue_Enqueue(qhead, (VgmChipWriteCmd){ .cmd = opn2globcmd, .addr = 0x22, 
                        .data = genesis[board].opn2.lforeg }, 0);
                break;
            case 1:
                //LFO Speed
                genesis[board].opn2.lfo_freq = GENMDM_ALIGN(value,3);
                VGM_HeadQueue_Enqueue(qhead, (VgmChipWriteCmd){ .cmd = opn2globcmd, .addr = 0x22, 
                        .data = genesis[board].opn2.lforeg }, 0);
                break;
            case 85:
                //Pitch Transposition
                //TODO
                break;
            case 84:
                //Octave Division
                //TODO and also wut?
                break;
            case 83:
                //PAL/NTSC Tuning
                //TODO
                break;
            case 80:
                //Voice 3 Special Mode
                genesis[board].opn2.ch3_mode = GENMDM_ALIGN(value,1);
                VGM_HeadQueue_Enqueue(qhead, (VgmChipWriteCmd){ .cmd = opn2globcmd, .addr = 0x27, 
                        .data = genesis[board].opn2.timerctrlreg }, 0);
                break;
            case 98: //originally 92, conflicts with SSG-EG
                //Timer Controls
                genesis[board].opn2.timerctrlreg = (genesis[board].opn2.timerctrlreg & 0xC0) | GENMDM_ALIGN(value,6);
                VGM_HeadQueue_Enqueue(qhead, (VgmChipWriteCmd){ .cmd = opn2globcmd, .addr = 0x27, 
                        .data = genesis[board].opn2.timerctrlreg }, 0);
                break;
            case 99: //originally 93, conflicts with SSG-EG
                //Ch3 CSM mode (rather, bit 7 of the register)
                genesis[board].opn2.ch3_mode = (genesis[board].opn2.ch3_mode & 1) | (GENMDM_ALIGN(value,1) << 1);
                VGM_HeadQueue_Enqueue(qhead, (VgmChipWriteCmd){ .cmd = opn2globcmd, .addr = 0x27, 
                        .data = genesis[board].opn2.timerctrlreg }, 0);
                break;
            case 94:
                //Test Register 0x21 Lowest Four Bits
                genesis[board].opn2.testreg21 = (genesis[board].opn2.testreg21 & 0xF0) | (GENMDM_ALIGN(value,4));
                VGM_HeadQueue_Enqueue(qhead, (VgmChipWriteCmd){ .cmd = opn2globcmd, .addr = 0x21, 
                        .data = genesis[board].opn2.testreg21 }, 0);
                break;
            case 95:
                //Test Register 0x21 Highest Four Bits
                genesis[board].opn2.testreg21 = (genesis[board].opn2.testreg21 & 0x0F) | (GENMDM_ALIGN(value,4) << 4);
                VGM_HeadQueue_Enqueue(qhead, (VgmChipWriteCmd){ .cmd = opn2globcmd, .addr = 0x21, 
                        .data = genesis[board].opn2.testreg21 }, 0);
                break;
            case 96:
                //Test Register 0x2C Lowest Four Bits
                genesis[board].opn2.testreg2C = (genesis[board].opn2.testreg2C & 0xF0) | (GENMDM_ALIGN(value,4));
                VGM_HeadQueue_Enqueue(qhead, (VgmChipWriteCmd){ .cmd = opn2globcmd, .addr = 0x2C, 
                        .data = genesis[board].opn2.testreg2C }, 0);
                break;
            case 97:
                //Test Register 0x2C Highest Four Bits
                genesis[board].opn2.testreg2C = (genesis[board].opn2.testreg2C & 0x0F) | (GENMDM_ALIGN(value,4) << 4);
                VGM_HeadQueue_Enqueue(qhead, (VgmChipWriteCmd){ .cmd = opn2globcmd, .addr = 0x2C, 
                        .data = genesis[board].opn2.testreg2C }, 0);
                break;
            case 6:
                //Preset Instrument Setting Store in RAM
                //TODO
                break;
            case 9:
                //Preset Instrument Setting Recall from RAM
                //TODO
                break;
            case 81:
                //Pitch Bend Amount
                //TODO
                break;
            case 14:
                //FM Algorithm
                genesis[board].opn2.chan[chan].algorithm = GENMDM_ALIGN(value,3);
                VGM_HeadQueue_Enqueue(qhead, (VgmChipWriteCmd){ .cmd = opn2cmd, .addr = 0xB0 | chanlo, 
                        .data = genesis[board].opn2.chan[chan].algfbreg }, 0);
                break;
            case 15:
                //FM Feedback
                genesis[board].opn2.chan[chan].feedback = GENMDM_ALIGN(value,3);
                VGM_HeadQueue_Enqueue(qhead, (VgmChipWriteCmd){ .cmd = opn2cmd, .addr = 0xB0 | chanlo, 
                        .data = genesis[board].opn2.chan[chan].algfbreg }, 0);
                break;
            case 77:
                //Stereo Configuration
                value = GENMDM_ALIGN(value,2);
                genesis[board].opn2.chan[chan].out_l = value & 1;
                genesis[board].opn2.chan[chan].out_r = value >> 1;
                VGM_HeadQueue_Enqueue(qhead, (VgmChipWriteCmd){ .cmd = opn2cmd, .addr = 0xB4 | chanlo, 
                        .data = genesis[board].opn2.chan[chan].lfooutreg }, 0);
                break;
            case 76:
                //Amplitude Modulation Level
                genesis[board].opn2.chan[chan].lfoampd = GENMDM_ALIGN(value,2);
                VGM_HeadQueue_Enqueue(qhead, (VgmChipWriteCmd){ .cmd = opn2cmd, .addr = 0xB4 | chanlo, 
                        .data = genesis[board].opn2.chan[chan].lfooutreg }, 0);
                break;
            case 75:
                //Frequency Modulation Level
                genesis[board].opn2.chan[chan].lfofreqd = GENMDM_ALIGN(value,3);
                VGM_HeadQueue_Enqueue(qhead, (VgmChipWriteCmd){ .cmd = opn2cmd, .addr = 0xB4 | chanlo, 
                        .data = genesis[board].opn2.chan[chan].lfooutreg }, 0);
                break;
            case 16:
                //Total Level
                genesis[board].opn2.chan[chan].op[op].totallevel = value;
                VGM_HeadQueue_Enqueue(qhead, (VgmChipWriteCmd){ .cmd = opn2cmd, .addr = 0x40 | opaddr, 
                        .data = genesis[board].opn2.chan[chan].op[op].tlreg }, 0);
                break;
            case 20:
                //Multiple
                genesis[board].opn2.chan[chan].op[op].fmult = GENMDM_ALIGN(value,4);
                VGM_HeadQueue_Enqueue(qhead, (VgmChipWriteCmd){ .cmd = opn2cmd, .addr = 0x30 | opaddr, 
                        .data = genesis[board].opn2.chan[chan].op[op].freqreg }, 0);
                break;
            case 24:
                //Detune
                genesis[board].opn2.chan[chan].op[op].detune = GENMDM_ALIGN(value,3);
                VGM_HeadQueue_Enqueue(qhead, (VgmChipWriteCmd){ .cmd = opn2cmd, .addr = 0x30 | opaddr, 
                        .data = genesis[board].opn2.chan[chan].op[op].freqreg }, 0);
                break;
            case 39:
                //Rate Scaling
                genesis[board].opn2.chan[chan].op[op].ratescale = GENMDM_ALIGN(value,2);
                VGM_HeadQueue_Enqueue(qhead, (VgmChipWriteCmd){ .cmd = opn2cmd, .addr = 0x50 | opaddr, 
                        .data = genesis[board].opn2.chan[chan].op[op].atkrsreg }, 0);
                break;
            case 43:
                //Attack Rate
                genesis[board].opn2.chan[chan].op[op].attackrate = GENMDM_ALIGN(value,5);
                VGM_HeadQueue_Enqueue(qhead, (VgmChipWriteCmd){ .cmd = opn2cmd, .addr = 0x50 | opaddr, 
                        .data = genesis[board].opn2.chan[chan].op[op].atkrsreg }, 0);
                break;
            case 47:
                //First Decay Rate
                genesis[board].opn2.chan[chan].op[op].decay1rate = GENMDM_ALIGN(value,5);
                VGM_HeadQueue_Enqueue(qhead, (VgmChipWriteCmd){ .cmd = opn2cmd, .addr = 0x60 | opaddr, 
                        .data = genesis[board].opn2.chan[chan].op[op].dec1amreg }, 0);
                break;
            case 51:
                //Secondary Decay Rate
                genesis[board].opn2.chan[chan].op[op].decay2rate = GENMDM_ALIGN(value,5);
                VGM_HeadQueue_Enqueue(qhead, (VgmChipWriteCmd){ .cmd = opn2cmd, .addr = 0x70 | opaddr, 
                        .data = genesis[board].opn2.chan[chan].op[op].dec2reg }, 0);
                break;
            case 55:
                //Secondary Amplitude Level
                genesis[board].opn2.chan[chan].op[op].decaylevel = GENMDM_ALIGN(value,4);
                VGM_HeadQueue_Enqueue(qhead, (VgmChipWriteCmd){ .cmd = opn2cmd, .addr = 0x80 | opaddr, 
                        .data = genesis[board].opn2.chan[chan].op[op].rellvlreg }, 0);
                break;
            case 59:
                //Release Rate
                genesis[board].opn2.chan[chan].op[op].releaserate = GENMDM_ALIGN(value,4);
                VGM_HeadQueue_Enqueue(qhead, (VgmChipWriteCmd){ .cmd = opn2cmd, .addr = 0x80 | opaddr, 
                        .data = genesis[board].opn2.chan[chan].op[op].rellvlreg }, 0);
                break;
            case 70:
                //Amplitude Modulation Enable
                genesis[board].opn2.chan[chan].op[op].amplfo = GENMDM_ALIGN(value,1);
                VGM_HeadQueue_Enqueue(qhead, (VgmChipWriteCmd){ .cmd = opn2cmd, .addr = 0x60 | opaddr, 
                        .data = genesis[board].opn2.chan[chan].op[op].dec1amreg }, 0);
                break;
            case 90:
                //SSG-EG
                genesis[board].opn2.chan[chan].op[op].ssgreg = GENMDM_ALIGN(value,4);
                VGM_HeadQueue_Enqueue(qhead, (VgmChipWriteCmd){ .cmd = opn2cmd, .addr = 0x90 | opaddr, 
                        .data = genesis[board].opn2.chan[chan].op[op].ssgreg }, 0);
                break;
            case 78:
                //DAC Enable
                genesis[board].opn2.dac_enable = GENMDM_ALIGN(value,1);
                VGM_HeadQueue_Enqueue(qhead, (VgmChipWriteCmd){ .cmd = opn2globcmd, .addr = 0x2B, 
                        .data = genesis[board].opn2.dacenablereg }, 0);
                break;
            case 79:
                //DAC Direct Data
                genesis[board].opn2.dac_high = value << 1;
                VGM_HeadQueue_Enqueue(qhead, (VgmChipWriteCmd){ .cmd = opn2globcmd, .addr = 0x2A, 
                        .data = genesis[board].opn2.dac_high }, 0);
                break;
            case 86:
                //DAC Sample Pitch Speed
                //TODO
                break;
            case 88:
                //DAC Sample Oversample
                //TODO
                break;
            case 89:
                //DAC Noise / Custom Wave Mode
                //TODO
                break;
            case 100:
                //Custom Wave Byte
                //TODO
                break;
            };
        }
    }else if(midi_package.event == 0x8 || (midi_package.event == 0x9 && midi_package.velocity == 0)){
        //Note Off
        if(chan < 5){
            //OPN2 channel
            //Key off
            VGM_HeadQueue_Enqueue(qhead, (VgmChipWriteCmd){ .cmd = opn2globcmd, .addr = 0x28, .data = ((chan < 3) ? chan : chan+1) }, 0);
        }else if(chan >= 6 && chan <= 9){
            //PSG square or noise channel
            VGM_HeadQueue_Enqueue(qhead, (VgmChipWriteCmd){ .cmd = (board << 4), .data = ((chan-6)<<5)|0x9F }, 0);
        }else if(chan <= 12){
            //OPN2 ch3 additional frequency
            //TODO key off individual operators?
        }
    }else if(midi_package.event == 0x9){
        //Note On
        if(chan < 5){
            //OPN2 channel
            //TODO under what conditions is velocity mapped to TL?
            //TODO key on individual operators if in Ch3 special/CSM mode?
            u8 subchan = (chan < 3) ? chan : chan+1;
            VgmChipWriteCmd cmd = VGM_getOPN2Frequency(midi_package.note, 0, 8000000); //TODO cents and clock
            cmd.cmd = (board << 4) | 2 | (subchan>>2);
            cmd.addr = 0xA4 | (subchan & 0x03);
            VGM_HeadQueue_Enqueue(qhead, cmd, 0);
            //Key on
            cmd.cmd &= 0xFE; //Switch to low bank
            cmd.addr = 0x28;
            cmd.data = 0xF0 + subchan;
            VGM_HeadQueue_Enqueue(qhead, cmd, 0);
        }else if(chan >= 6 && chan <= 8){
            //PSG square channel
            VgmChipWriteCmd cmd = VGM_getPSGFrequency(midi_package.note, 0, 3579545); //TODO cents and clock
            cmd.data |= ((chan-6)<<5) | 0x80;
            cmd.cmd = (board << 4);
            VGM_HeadQueue_Enqueue(qhead, cmd, 0);
            VGM_HeadQueue_Enqueue(qhead, (VgmChipWriteCmd){ .cmd = (board << 4), .data = ((chan-6)<<5)|0x90|(15-GENMDM_ALIGN(midi_package.velocity, 4)) }, 0);
        }else if(chan == 9){
            //PSG noise channel
            u8 mod, noiseopts;
            mod = (midi_package.note % 12);
            switch(mod){
            //turn your head 90^ clockwise (:
            case 0:
                case 1:  noiseopts = 0b000; break;
            case 2:
                case 3:  noiseopts = 0b001; break;
            case 4:      noiseopts = 0b010; break;
            case 5:      noiseopts = 0b100; break;
                case 6:  noiseopts = 0b101; break;
            case 7:
                case 8:  noiseopts = 0b110; break;
            case 9:
                case 10: noiseopts = 0b011; break;
            case 11:     noiseopts = 0b111; break;
            };
            VGM_HeadQueue_Enqueue(qhead, (VgmChipWriteCmd){ .cmd = (board << 4), .data = 0xE0|noiseopts }, 0);
            VGM_HeadQueue_Enqueue(qhead, (VgmChipWriteCmd){ .cmd = (board << 4), .data = 0xF0|(15-GENMDM_ALIGN(midi_package.velocity, 4)) }, 0);
        }else if(chan <= 12){
            //OPN2 ch3 additional frequency
            //TODO key on individual operators?
            u8 subchan = chan - 10;
            VgmChipWriteCmd cmd = VGM_getOPN2Frequency(midi_package.note, 0, 8000000); //TODO cents and clock
            cmd.cmd = (board << 4) | 2;
            cmd.addr = 0xAC | subchan;
            VGM_HeadQueue_Enqueue(qhead, cmd, 0);
        }
    }
}


// Not used
void APP_Tick(void){}
void APP_MIDI_Tick(void){}
void APP_SRIO_ServicePrepare(void){}
void APP_SRIO_ServiceFinish(void){}
void APP_DIN_NotifyToggle(u32 pin, u32 pin_value){}
void APP_ENC_NotifyChange(u32 encoder, s32 incrementer){}
void APP_AIN_NotifyChange(u32 pin, u32 pin_value){}

