/*
 * MIDIbox Quad Genesis: VGM Command Editor
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
#include "cmdeditor.h"

#include <genesis.h>
#include "syeng.h"
#include "frontpanel.h"
#include "genesisstate.h"
#include "mode_vgm.h"
#include "mode_prog.h"


static const u8 init_reg_vals[8] = {0x01, 0x20, 0x5F, 0x00, 0x00, 0x0B, 0x00, 0x00};


void DrawUsageOnVoices(VgmUsageBits usage, u8 r){
    u32 u = usage.all;
    u8 i;
    for(i=0; i<6; ++i){
        FrontPanel_GenesisLEDSet(r, i+1, 0, (u & 0x00000001)); //FM
        u >>= 1;
    }
    for(i=0; i<6; ++i){
        FrontPanel_GenesisLEDSet(r+1, i+1, 0, (u & 0x00000001)); //LFO
        u >>= 1;
    }
    FrontPanel_GenesisLEDSet(r, 7, 0, (u & 0x00000001)); //DAC
    u >>= 1;
    if(r == 0){
        FrontPanel_LEDSet(FP_LED_CH3_4FREQ, (u & 0x00000001)); //FM3 special
        FrontPanel_LEDSet(FP_LED_CH3_CSM, (u & 0x00000001));
    }
    u >>= 1;
    FrontPanel_GenesisLEDSet(r, 0, 0, (u & 0x00000001)); //FM globals
    u >>= 10; //Skip LFO globals
    for(i=0; i<4; ++i){
        FrontPanel_GenesisLEDSet(r, i+8, 0, (u & 0x00000001)); //SQ, NS
        u >>= 1;
    }
    if(r == 0){
        FrontPanel_LEDSet(FP_LED_NS_SQ3, (u & 0x00000001)); //SQ3/NS
    }
}


void DrawCmdLine(VgmChipWriteCmd cmd, u8 row, u8 ctrlled){
    u16 outbits = ctrlled;
    if(cmd.cmd == 0x50){
        //PSG write
        outbits |= 1 << (10 + ((cmd.data >> 5) & 3));
    }else if((cmd.cmd & 0xFE) == 0x52){
        //OPN2 write
        u8 chan, op, addrhi = (cmd.cmd & 1)*3;
        if(cmd.addr <= 0x2F){
            if(cmd.addr < 0x20 || addrhi) goto end;
            //OPN2 global register
            if(cmd.addr == 0x28){
                //Key On register
                chan = cmd.data & 0x07;
                if((chan & 0x03) == 0x03) goto end; //Make sure not writing to 0x03 or 0x07
                if(chan >= 0x04) chan -= 1; //Move channels 4, 5, 6 down to 3, 4, 5
                outbits |= 1 << (3+chan);
            }else if(cmd.addr >= 0x24 && cmd.addr <= 0x27){
                outbits |= 1 << (5);
            }else if(cmd.addr == 0x2A || cmd.addr == 0x2B){
                outbits |= 1 << (9);
            }else{
                outbits |= 1 << (2);
            }
        }else if(cmd.addr <= 0x9F){
            //Operator command
            chan = (cmd.addr & 0x03);
            if(chan == 0x03) goto end; //No channel 4 in first half
            chan += addrhi; //Add 3 for channels 4, 5, 6
            op = ((cmd.addr & 0x08) >> 3) | ((cmd.addr & 0x04) >> 1); //Ops 1,2,3,4: 0x30, 0x38, 0x34, 0x3C
            outbits |= 1 << (3+chan);
            outbits |= 1 << (10+op);
        }else if(cmd.addr <= 0xAE && cmd.addr >= 0xA8){
            //Channel 3 extra frequency
            outbits |= 1 << (5);
        }else if(cmd.addr <= 0xB6){
            //Channel command
            chan = (cmd.addr & 0x03);
            if(chan == 0x03) goto end; //No channel 4 in first half
            chan += addrhi; //Add 3 for channels 4, 5, 6
            outbits |= 1 << (3+chan);
        }
    }else if(cmd.cmd >= 0x80 && cmd.cmd <= 0x8F){
        //OPN2 DAC write
        outbits |= 1 << (1);
        outbits |= 1 << (9);
    }else if((cmd.cmd >= 0x70 && cmd.cmd <= 0x7F) | (cmd.cmd >= 0x61 && cmd.cmd <= 0x63)){
        //Wait
        outbits |= 1 << (1);
    }else{
        //Unsupported command, should not be here
        outbits |= 1 << (0);
    }
end:
    FrontPanel_VGMMatrixRow(row, outbits);
}

void DrawCmdContent(VgmChipWriteCmd cmd, u8 clear){
    u8 notclear = !clear;
    if(cmd.cmd == 0x50){
        //PSG write
        u8 voice;
        voice = (cmd.data & 0x60) >> 5;
        FrontPanel_GenesisLEDSet(0, 8+voice, 0, !clear);
        if(cmd.data & 0x10){
            //Attenuation
            FrontPanel_LEDRingSet(FP_LEDR_PSGVOL, notclear<<1, 15 - (cmd.data & 0x0F));
        }else{
            if(voice == 3){
                //Noise parameters
                u8 x = cmd.data & 0x03;
                FrontPanel_LEDSet(FP_LED_NS_HI,  x == 0 && !clear);
                FrontPanel_LEDSet(FP_LED_NS_MED, x == 1 && !clear);
                FrontPanel_LEDSet(FP_LED_NS_LOW, x == 2 && !clear);
                FrontPanel_LEDSet(FP_LED_NS_SQ3, x == 3 && !clear);
                x = (cmd.data & 0x04) >> 2;
                FrontPanel_LEDSet(FP_LED_NS_PLS, x == 0 && !clear);
                FrontPanel_LEDSet(FP_LED_NS_WHT, x == 1 && !clear);
            }else{
                u16 f = (cmd.data & 0x0F) | ((u16)(cmd.data2 & 0x3F) << 4);
                FrontPanel_LEDRingSet(FP_LEDR_PSGFREQ, notclear, ((1023 - f) >> 4) & 0xF);
            }
        }
    }else if((cmd.cmd & 0xFE) == 0x52){
        //OPN2 write
        u8 chan, op, reg, addrhi = (cmd.cmd & 1)*3, i;
        if(cmd.addr <= 0x2F){
            if(cmd.addr >= 0x20 && !addrhi){
                //OPN2 global register
                switch(cmd.addr){
                    case 0x28:
                        //Key On register
                        chan = cmd.data & 0x07;
                        if((chan & 0x03) != 0x03){ //Make sure not writing to 0x03 or 0x07
                            if(chan >= 0x04) chan -= 1; //Move channels 4, 5, 6 down to 3, 4, 5
                            FrontPanel_GenesisLEDSet(0, 1+chan, 0, !clear);
                            reg = cmd.data >> 4;
                            for(op=0; op<4; op++){
                                FrontPanel_LEDSet(FP_LED_KEYON_1 + op, (reg & 1) && !clear);
                                reg >>= 1;
                            }
                        }
                        break;
                    case 0x21:
                        //Test Register 1
                        FrontPanel_LEDSet(FP_LED_UGLY, ((cmd.data >> 4) & 1) && !clear);
                        FrontPanel_LEDSet(FP_LED_EG, ((cmd.data >> 5) & 1) && !clear);
                        FrontPanel_GenesisLEDSet(0, 0, 0, !clear);
                        break;
                    case 0x22:
                        //LFO Register
                        FrontPanel_LEDRingSet(FP_LEDR_LFOFREQ, notclear<<1, cmd.data & 7);
                        FrontPanel_LEDSet(FP_LED_LFO, ((cmd.data >> 3) & 1) && !clear);
                        FrontPanel_GenesisLEDSet(0, 0, 0, !clear);
                        break;
                    case 0x24:
                        FrontPanel_LEDRingSet(FP_LEDR_CSMFREQ, notclear<<1, cmd.data >> 4);
                        break;
                    case 0x27:
                        i = cmd.data >> 6;
                        FrontPanel_LEDSet(FP_LED_CH3_NORMAL, (i == 0) && !clear);
                        FrontPanel_LEDSet(FP_LED_CH3_4FREQ, (i & 1) && !clear);
                        FrontPanel_LEDSet(FP_LED_CH3_CSM, (i == 2) && !clear);
                        FrontPanel_LEDSet(FP_LED_CH3FAST, (cmd.data & 1) && !clear);
                        break;
                    case 0x2A:
                        FrontPanel_DrawDACValue(clear ? 0 : cmd.data << 1);
                        break;
                    case 0x2B:
                        FrontPanel_LEDSet(FP_LED_DACEN, (cmd.data >> 7) && !clear);
                        FrontPanel_GenesisLEDSet(0, 0, 0, !clear);
                        break;
                    case 0x2C:
                        FrontPanel_DrawDACValue(((cmd.data >> 3) & 1) && !clear);
                        FrontPanel_LEDSet(FP_LED_DACOVR, ((cmd.data >> 5) & 1) && !clear);
                        FrontPanel_GenesisLEDSet(0, 0, 0, !clear);
                        break;
                    default:
                        return;
                }
            }
        }else if(cmd.addr <= 0x9F){
            //Operator command
            chan = (cmd.addr & 0x03);
            if(chan != 0x03){ //No channel 4 in first half
                chan += addrhi; //Add 3 for channels 4, 5, 6
                reg = (cmd.addr & 0xF0);
                op = ((cmd.addr & 0x08) >> 3) | ((cmd.addr & 0x04) >> 1); //Ops 1,2,3,4: 0x30, 0x38, 0x34, 0x3C
                FrontPanel_GenesisLEDSet(0, 1+chan, 0, !clear);
                FrontPanel_LEDSet(FP_LED_OPNODE_R_1 + op, !clear);
                switch(reg){
                    case 0x30:
                        FrontPanel_LEDRingSet(FP_LEDR_HARM, notclear<<1, cmd.data & 0x0F);
                        i = (cmd.data >> 4) & 7;
                        FrontPanel_LEDRingSet(FP_LEDR_DETUNE, notclear, DETUNE_DISPLAY(i));
                        break;
                    case 0x40:
                        FrontPanel_LEDRingSet(FP_LEDR_OP1LVL + op, notclear<<1, (127 - (cmd.data & 0x7F)) >> 3);
                        break;
                    case 0x50:
                        FrontPanel_LEDRingSet(FP_LEDR_ATTACK, notclear<<1, (cmd.data & 0x1F) >> 1);
                        FrontPanel_LEDSet(FP_LED_KSR, cmd.data >= 0x40 && !clear);
                        break;
                    case 0x60:
                        FrontPanel_LEDRingSet(FP_LEDR_DEC1R, notclear<<1, (cmd.data & 0x1F) >> 1);
                        FrontPanel_LEDSet(FP_LED_LFOAM, cmd.data >= 0x80 && !clear);
                        break;
                    case 0x70:
                        FrontPanel_LEDRingSet(FP_LEDR_DEC2R, notclear<<1, (cmd.data & 0x1F) >> 1);
                        break;
                    case 0x80:
                        FrontPanel_LEDRingSet(FP_LEDR_RELRATE, notclear<<1, cmd.data & 0x0F);
                        FrontPanel_LEDRingSet(FP_LEDR_DECLVL, notclear<<1, 15 - (cmd.data >> 4));
                        break;
                    case 0x90:
                        i = (cmd.data & 0x0F);
                        FrontPanel_LEDSet(FP_LED_SSGHOLD, (i&1) && !clear);
                        i >>= 1;
                        FrontPanel_LEDSet(FP_LED_SSGTGL, (i&1) && !clear);
                        i >>= 1;
                        FrontPanel_LEDSet(FP_LED_SSGINIT, (i&1) && !clear);
                        i >>= 1;
                        FrontPanel_LEDSet(FP_LED_SSGON, (i&1) && !clear);
                        break;
                    default:
                        return;
                }
            }
        }else if(cmd.addr <= 0xAE && cmd.addr >= 0xA8){
            //Channel 3 extra frequency
            if(cmd.addr == 0xAD) op = 0;
            else if(cmd.addr == 0xAE) op = 1;
            else if(cmd.addr == 0xAC) op = 2;
            else return;
            FrontPanel_GenesisLEDSet(0, 3, 0, !clear);
            FrontPanel_LEDSet(FP_LED_OPNODE_R_1 + op, !clear);
            if(clear){
                FrontPanel_DrawDigit(FP_LED_DIG_OCT, ' ');
                FrontPanel_ClearDisplay(FP_LED_DIG_FREQ_1);
            }else{
                FrontPanel_DrawDigit(FP_LED_DIG_OCT, '0' + ((cmd.data >> 3) & 7));
                FrontPanel_DrawNumber(FP_LED_DIG_FREQ_1, ((u16)(cmd.data & 7) << 8) | cmd.data2);
            }
        }else if(cmd.addr <= 0xB6){
            //Channel command
            chan = (cmd.addr & 0x03);
            if(chan != 0x03){ //No channel 4 in first half
                chan += addrhi; //Add 3 for channels 4, 5, 6
                FrontPanel_GenesisLEDSet(0, chan+1, 0, !clear);
                reg = cmd.addr & 0xFC;
                if(reg == 0xA0){
                    return; //Should not be
                }else if(reg == 0xA4){
                    //Frequency write
                    if(clear){
                        FrontPanel_DrawDigit(FP_LED_DIG_OCT, ' ');
                        FrontPanel_ClearDisplay(FP_LED_DIG_FREQ_1);
                    }else{
                        FrontPanel_DrawDigit(FP_LED_DIG_OCT, '0' + ((cmd.data >> 3) & 7));
                        FrontPanel_DrawNumber(FP_LED_DIG_FREQ_1, ((u16)(cmd.data & 7) << 8) | cmd.data2);
                    }
                }else if(reg == 0xB0){
                    FrontPanel_DrawAlgorithm((cmd.data & 7) + (clear << 4));
                    FrontPanel_LEDRingSet(FP_LEDR_FEEDBACK, notclear<<1, (cmd.data >> 3) & 7);
                    FrontPanel_LEDSet(FP_LED_FEEDBACK, (cmd.data & 0x38) > 0 && !clear);
                }else if(reg == 0xB4){
                    FrontPanel_LEDRingSet(FP_LEDR_LFOFDEP, notclear<<1, (cmd.data & 7));
                    FrontPanel_LEDRingSet(FP_LEDR_LFOADEP, notclear<<1, (cmd.data >> 4) & 3);
                    FrontPanel_LEDSet(FP_LED_OUTR, ((cmd.data >> 6) & 1) && !clear);
                    FrontPanel_LEDSet(FP_LED_OUTL, ((cmd.data >> 7) & 1) && !clear);
                }
            }
        }
    }else if(cmd.cmd >= 0x80 && cmd.cmd <= 0x8F){
        //OPN2 DAC write
        if(cmd.addr != 0x2A) return;
        FrontPanel_DrawDACValue(clear ? 0 : cmd.data << 1);
        FrontPanel_LEDSet(FP_LED_TIME_R, !clear);
        if(clear){
            FrontPanel_ClearDisplay(FP_LED_DIG_MAIN_1);
            FrontPanel_DrawDigit(FP_LED_DIG_OCT, ' ');
        }else{
            FrontPanel_DrawNumber(FP_LED_DIG_MAIN_1, cmd.data << 1);
            FrontPanel_DrawNumberHex(FP_LED_DIG_OCT, (cmd.cmd & 0x0F));
        }
    }else if(cmd.cmd >= 0x70 && cmd.cmd <= 0x7F){
        //Short wait
        FrontPanel_LEDSet(FP_LED_TIME_R, !clear);
        if(clear){
            FrontPanel_DrawDigit(FP_LED_DIG_OCT, ' ');
        }else{
            FrontPanel_DrawNumberHex(FP_LED_DIG_OCT, (cmd.cmd & 0x0F));
        }
    }else if(cmd.cmd == 0x61){
        //Long wait
        FrontPanel_LEDSet(FP_LED_TIME_R, !clear);
        if(clear){
            FrontPanel_ClearDisplay(FP_LED_DIG_FREQ_1);
        }else{
            FrontPanel_DrawNumberHex(FP_LED_DIG_FREQ_1, (u16)(cmd.data) | ((u16)cmd.data2 << 8));
        }
    }else if(cmd.cmd == 0x62 || cmd.cmd == 0x63){
        //60 Hz or 50 Hz wait
        FrontPanel_LEDSet(FP_LED_TIME_R, !clear);
        if(clear){
            FrontPanel_ClearDisplay(FP_LED_DIG_FREQ_1);
        }else{
            FrontPanel_DrawDigit(FP_LED_DIG_FREQ_1, cmd.cmd == 0x62 ? '6' : '5');
            FrontPanel_DrawDigit(FP_LED_DIG_FREQ_2, '0');
            FrontPanel_DrawDigit(FP_LED_DIG_FREQ_3, ' ');
            FrontPanel_DrawDigit(FP_LED_DIG_FREQ_4, 'H');
        }
    }else{
        //Unsupported command, should not be here
        FrontPanel_DrawAlgorithm(8 + (clear << 3));
    }
}

void GetCmdDescription(VgmChipWriteCmd cmd, char* desc){
    if(cmd.cmd == 0x50){
        //PSG write
        u8 voice;
        voice = (cmd.data & 0x60) >> 5;
        if(cmd.data & 0x10){
            //Attenuation
            if(voice == 3){
                sprintf(desc, "NS Vol:%d", 15 - (cmd.data & 0x0F));
            }else{
                sprintf(desc, "SQ%d Vol:%d", voice+1, 15 - (cmd.data & 0x0F));
            }
        }else{
            if(voice == 3){
                //Noise parameters
                static const char* const ns_msgs[4] = { "Hi", "Med", "Lo", "SQ3" };
                sprintf(desc, "NS Freq:%s, Type:%s", ns_msgs[cmd.data & 0x03], (cmd.data & 0x04) ? "White" : "Pulse");
            }else{
                u16 f = (cmd.data & 0x0F) | ((u16)(cmd.data2 & 0x3F) << 4);
                sprintf(desc, "SQ%d Freq:%d", voice+1, 1023-f);
            }
        }
    }else if((cmd.cmd & 0xFE) == 0x52){
        //OPN2 write
        u8 chan, op, reg, addrhi = (cmd.cmd & 1)*3, i;
        if(cmd.addr <= 0x2F){
            if(cmd.addr >= 0x20 && !addrhi){
                //OPN2 global register
                switch(cmd.addr){
                    case 0x28:
                        //Key On register
                        chan = cmd.data & 0x07;
                        if((chan & 0x03) != 0x03){ //Make sure not writing to 0x03 or 0x07
                            if(chan >= 0x04) chan -= 1; //Move channels 4, 5, 6 down to 3, 4, 5
                            sprintf(desc, "FM%d KeyOn:%c%c%c%c", chan+1, 
                                (cmd.data & 0x10) ? 'O' : '-',
                                (cmd.data & 0x20) ? 'O' : '-',
                                (cmd.data & 0x40) ? 'O' : '-',
                                (cmd.data & 0x80) ? 'O' : '-');
                        }
                        break;
                    case 0x21:
                        //Test Register 1
                        sprintf(desc, "Test21 Ugly:%d, EG:%d", (cmd.data >> 4) & 1, (cmd.data >> 5) & 1);
                        break;
                    case 0x22:
                        //LFO Register
                        sprintf(desc, "Lfo Enab:%d, Freq:%d", (cmd.data >> 3) & 1, cmd.data & 7);
                        break;
                    case 0x24:
                        sprintf(desc, "CSM Freq:%d", ((u16)cmd.data << 2) | (cmd.data2 & 0x03));
                        break;
                    case 0x25:
                        sprintf(desc, "Error: TimerA lsbs?");
                        break;
                    case 0x27:
                        i = cmd.data >> 6;
                        static const char* const ch3_msgs[4] = { "Normal", "4Freq", "CSM", "4Freq" };
                        sprintf(desc, "FM3 Mode:%s, TmrEn:%d", ch3_msgs[cmd.data >> 6], cmd.data & 1);
                        break;
                    case 0x2A:
                        sprintf(desc, "DAC Byte:%d", cmd.data);
                        break;
                    case 0x2B:
                        sprintf(desc, "DAC Enable:%d", cmd.data >> 7);
                        break;
                    case 0x2C:
                        sprintf(desc, "Test2C DAC9:%d, DACOvr:%d", (cmd.data >> 3) & 1, (cmd.data >> 5) & 1);
                        break;
                    default:
                        return;
                }
            }
        }else if(cmd.addr <= 0x9F){
            //Operator command
            chan = (cmd.addr & 0x03);
            if(chan != 0x03){ //No channel 4 in first half
                chan += addrhi; //Add 3 for channels 4, 5, 6
                reg = (cmd.addr & 0xF0);
                op = ((cmd.addr & 0x08) >> 3) | ((cmd.addr & 0x04) >> 1); //Ops 1,2,3,4: 0x30, 0x38, 0x34, 0x3C
                switch(reg){
                    case 0x30:
                        i = (cmd.data >> 4) & 7;
                        sprintf(desc, "FM%d Op%d Harm:%d Detune:%d", chan+1, op+1, cmd.data & 0x0F, i >= 4 ? (4-i) : i);
                        break;
                    case 0x40:
                        sprintf(desc, "FM%d Op%d Level:%d", chan+1, op+1, 127 - (cmd.data & 0x7F));
                        break;
                    case 0x50:
                        sprintf(desc, "FM%d Op%d Atk Rate:%d, KSR:%d", chan+1, op+1, cmd.data & 0x1F, cmd.data >> 6);
                        break;
                    case 0x60:
                        sprintf(desc, "FM%d Op%d Dec1 Rate:%d, LFO AM:%d", chan+1, op+1, cmd.data & 0x1F, cmd.data >> 7);
                        break;
                    case 0x70:
                        sprintf(desc, "FM%d Op%d Dec2 Rate:%d", chan+1, op+1, cmd.data & 0x1F);
                        break;
                    case 0x80:
                        sprintf(desc, "FM%d Op%d Rel Rate:%d, Dec1 Lvl:%d", chan+1, op+1, cmd.data & 0x0F, 15 - (cmd.data >> 4));
                        break;
                    case 0x90:
                        i = (cmd.data & 0x0F);
                        sprintf(desc, "FM%d Op%d SSG-EG On%d Init%d Tgl%d Hold%d", chan+1, op+1,
                            (cmd.data & 0x08) > 0,
                            (cmd.data & 0x04) > 0,
                            (cmd.data & 0x02) > 0,
                            (cmd.data & 0x01) > 0);
                        break;
                    default:
                        return;
                }
            }
        }else if(cmd.addr <= 0xAE && cmd.addr >= 0xA8){
            //Channel 3 extra frequency
            if(cmd.addr == 0xAD) op = 0;
            else if(cmd.addr == 0xAE) op = 1;
            else if(cmd.addr == 0xAC) op = 2;
            else return;
            sprintf(desc, "FM3 Op%d Block:%d, Freq:%d", op+1, (cmd.data >> 3) & 7, ((u16)(cmd.data & 7) << 8) | cmd.data2);
        }else if(cmd.addr <= 0xB6){
            //Channel command
            chan = (cmd.addr & 0x03);
            if(chan != 0x03){ //No channel 4 in first half
                chan += addrhi; //Add 3 for channels 4, 5, 6
                reg = cmd.addr & 0xFC;
                if(reg == 0xA0){
                    sprintf(desc, "Error: frequency order");
                }else if(reg == 0xA4){
                    //Frequency write
                    sprintf(desc, "FM%d Block:%d, Freq:%d", chan+1, (cmd.data >> 3) & 7, ((u16)(cmd.data & 7) << 8) | cmd.data2);
                }else if(reg == 0xB0){
                    sprintf(desc, "FM%d Algorithm:%d, Feedback:%d", chan+1, cmd.data & 7, (cmd.data >> 3) & 7);
                }else if(reg == 0xB4){
                    sprintf(desc, "FM%d LFO-Frq:%d, LFO-Amp:%d, Out:%c%c", chan+1, cmd.data & 7, (cmd.data >> 4) & 3, (cmd.data & 0x40) ? 'L' : '-', (cmd.data & 0x80) ? 'R' : '-');
                }
            }
        }
    }else if(cmd.cmd >= 0x80 && cmd.cmd <= 0x8F){
        //OPN2 DAC write
        if(cmd.addr != 0x2A) return;
        sprintf(desc, "DAC+Time Byte:%d, Wait:%d", cmd.data, cmd.cmd - 0x80);
    }else if(cmd.cmd >= 0x70 && cmd.cmd <= 0x7F){
        sprintf(desc, "Short Wait:%d", cmd.cmd - 0x6F);
    }else if(cmd.cmd == 0x61){
        sprintf(desc, "Long Wait:%d", cmd.data | ((u16)cmd.data2 << 8));
    }else if(cmd.cmd == 0x62){
        sprintf(desc, "60Hz Wait [%d]", VGM_DELAY62);
    }else if(cmd.cmd == 0x63){
        sprintf(desc, "50Hz Wait [%d]", VGM_DELAY63);
    }else{
        //Unsupported command, should not be here
        sprintf(desc, "Unsuppoted cmd:0x%02X, addr:0x%02X, data:0x%02X, data2:0x%02X", cmd.cmd, cmd.addr, cmd.data, cmd.data2);
    }
}

static inline u8 CmdEditor_Clip(u8 v, s32 incrementer, u8 bits){
    s32 nv = (s32)v + incrementer;
    if(nv < 0) nv = 0;
    if(nv >= (1 << bits)) nv = (1 << bits) - 1;
    return (u8)nv;
}
static inline u16 CmdEditor_Clip16(u16 v, s32 incrementer, u8 bits){
    s32 nv = (s32)v + incrementer;
    if(nv < 0) nv = 0;
    if(nv >= (1 << bits)) nv = (1 << bits) - 1;
    return (u16)nv;
}

VgmChipWriteCmd EditCmd(VgmChipWriteCmd cmd, u8 encoder, s32 incrementer, u8 button, u8 state, u8 reqvoice, u8 reqop){
    VgmChipWriteCmd ret = {.all = cmd.all};
    u8 couldbeusagechange = 0;
    u8 d; u16 f;
    if(cmd.cmd == 0x50){
        //PSG write
        u8 voice = (cmd.data & 0x60) >> 5;
        if(reqvoice < 0xFF && reqvoice != voice+8) return cmd;
        if(cmd.data & 0x10){
            //Attenuation
            if(encoder == FP_E_PSGVOL){
                d = CmdEditor_Clip(cmd.data & 0x0F, 0-incrementer, 4);
                ret.data = (cmd.data & 0xF0) | d;
                FrontPanel_DrawNumber(FP_LED_DIG_MAIN_1, 15-d);
            }
        }else{
            if(voice == 3){
                //Noise parameters
                if(button == FP_B_NSFREQ){
                    d = (cmd.data+1) & 0x03;
                    ret.data = (cmd.data & 0xFC) | d;
                    if(d == 0 || d == 3) couldbeusagechange = 1;
                    FrontPanel_DrawNumber(FP_LED_DIG_MAIN_1, d);
                }else if(button == FP_B_NSTYPE){
                    ret.data = cmd.data ^ 0x04;
                    FrontPanel_DrawNumber(FP_LED_DIG_MAIN_1, (cmd.data >> 2) & 1);
                }
            }else{
                if(encoder == FP_E_PSGFREQ){
                    f = (cmd.data & 0x0F) | ((u16)(cmd.data2 & 0x3F) << 4);
                    s32 temp = (s32)f + incrementer;
                    if(temp < 0) temp = 0;
                    if(temp >= 0x400) temp = 0x3FF;
                    f = temp;
                    ret.data = (cmd.data & 0xF0) | (f & 0x000F);
                    ret.data2 = (ret.data2 & 0xC0) | (f >> 4);
                    FrontPanel_DrawNumber(FP_LED_DIG_MAIN_1, f);
                }
            }
        }
    }else if((cmd.cmd & 0xFE) == 0x52){
        //OPN2 write
        u8 chan, op, reg, addrhi = (cmd.cmd & 1)*3;
        if(cmd.addr <= 0x2F){
            if(cmd.addr >= 0x20 && !addrhi){
                //OPN2 global register
                switch(cmd.addr){
                    case 0x28:
                        //Key On register
                        if(reqvoice < 0xFF){
                            chan = cmd.data & 0x07;
                            if((chan & 0x03) == 0x03) return cmd;
                            if(chan >= 0x04) chan -= 1; //Move channels 4, 5, 6 down to 3, 4, 5
                            if(reqvoice != chan+1) return cmd;
                        }
                        if(button == FP_B_KON){
                            ret.data = cmd.data ^ (state << 4);
                            for(op=0; op<4; ++op){
                                FrontPanel_DrawDigit(FP_LED_DIG_MAIN_1+op, ((ret.data >> (4+op)) & 1) ? 'O' : '-');
                            }
                        }
                        break;
                    case 0x21:
                        //Test Register 1
                        if(reqvoice < 0xFF && reqvoice != 0) return cmd;
                        if(button == FP_B_UGLY){
                            ret.data = cmd.data ^ 0x10;
                            couldbeusagechange = 1;
                            FrontPanel_DrawNumber(FP_LED_DIG_MAIN_1, (ret.data >> 4) & 1);
                        }else if(button == FP_B_EG){
                            ret.data = cmd.data ^ 0x20;
                            couldbeusagechange = 1;
                            FrontPanel_DrawNumber(FP_LED_DIG_MAIN_1, (cmd.data >> 5) & 1);
                        }
                        break;
                    case 0x22:
                        //LFO Register
                        if(reqvoice < 0xFF && reqvoice != 0) return cmd;
                        if(button == FP_B_LFO){
                            ret.data = cmd.data ^ 0x08;
                            couldbeusagechange = 1;
                            FrontPanel_DrawNumber(FP_LED_DIG_MAIN_1, (cmd.data >> 2) & 1);
                        }else if(encoder == FP_E_LFOFREQ){
                            d = CmdEditor_Clip(cmd.data & 7, incrementer, 3);
                            ret.data = (cmd.data & 0xF8) | d;
                            FrontPanel_DrawNumber(FP_LED_DIG_MAIN_1, d);
                        }
                        break;
                    case 0x24:
                        if(reqvoice < 0xFF && reqvoice != 3) return cmd;
                        if(encoder == FP_E_CSMFREQ){
                            f = CmdEditor_Clip16(((u16)cmd.data << 2) | (cmd.data2 & 3), incrementer, 10);
                            ret.data = f >> 2;
                            ret.data2 = f & 3;
                            FrontPanel_DrawNumber(FP_LED_DIG_MAIN_1, f);
                        }
                        break;
                    case 0x25:
                        if(reqvoice < 0xFF && reqvoice != 3) return cmd;
                        DBG("TimerA lsbs?");
                        FrontPanel_DrawDigit(FP_LED_DIG_MAIN_4, '?');
                        break;
                    case 0x27:
                        if(reqvoice < 0xFF && reqvoice != 3) return cmd;
                        if(button == FP_B_CH3MODE){
                            ret.data = cmd.data + 0x40;
                            FrontPanel_DrawNumber(FP_LED_DIG_MAIN_1, ret.data >> 6);
                        }else if(button == FP_B_CH3FAST){
                            ret.data = cmd.data ^ 0x01;
                            FrontPanel_DrawNumber(FP_LED_DIG_MAIN_1, ret.data & 1);
                        }
                        if((cmd.data == 0) ^ (ret.data == 0)) couldbeusagechange = 1;
                        break;
                    case 0x2A:
                        if(reqvoice < 0xFF && reqvoice != 7) return cmd;
                        if(encoder != FP_E_DATAWHEEL){
                            ret.data = CmdEditor_Clip(cmd.data, incrementer, 8);
                            FrontPanel_DrawNumber(FP_LED_DIG_MAIN_1, ret.data);
                        }
                        break;
                    case 0x2B:
                        if(reqvoice < 0xFF && reqvoice != 7) return cmd;
                        if(button == FP_B_DACEN){
                            ret.data = cmd.data ^ 0x80;
                            couldbeusagechange = 1;
                            FrontPanel_DrawNumber(FP_LED_DIG_MAIN_1, ret.data >> 7);
                        }
                        break;
                    case 0x2C:
                        if(reqvoice < 0xFF && reqvoice != 0) return cmd;
                        if(button == FP_B_DACOVR){
                            ret.data = cmd.data ^ 0x20;
                            couldbeusagechange = 1;
                            FrontPanel_DrawNumber(FP_LED_DIG_MAIN_1, (ret.data >> 5) & 1);
                        }
                        break;
                }
            }
        }else if(cmd.addr <= 0x9F){
            //Operator command
            reg = (cmd.addr & 0xF0);
            op = ((cmd.addr & 0x08) >> 3) | ((cmd.addr & 0x04) >> 1); //Ops 1,2,3,4: 0x30, 0x38, 0x34, 0x3C
            if(reqvoice < 0xFF){
                if(op != reqop) return cmd;
                chan = (cmd.addr & 0x03);
                if(chan == 0x03) return cmd;
                chan += addrhi; //Add 3 for channels 4, 5, 6
                if(reqvoice != chan+1) return cmd;
            }
            switch(reg){
                case 0x30:
                    if(encoder == FP_E_HARM){
                        d = CmdEditor_Clip(cmd.data & 0x0F, incrementer, 4);
                        ret.data = (cmd.data & 0xF0) | d;
                        FrontPanel_DrawNumber(FP_LED_DIG_MAIN_1, d);
                    }else if(encoder == FP_E_DETUNE){
                        d = (cmd.data >> 4) & 7;
                        if(d == 5 && incrementer > 0){
                            d = incrementer - 1;
                        }else if(d == 0 && incrementer < 0){
                            d = 4 - incrementer;
                        }else if(d == 7 && incrementer < 0){
                            return cmd;
                        }else if(d == 3 && incrementer > 0){
                            return cmd;
                        }else if(d >= 4){
                            d -= incrementer;
                        }else{
                            d += incrementer;
                        }
                        ret.data = (d << 4) | (cmd.data & 0x0F);
                        FrontPanel_DrawNumber(FP_LED_DIG_MAIN_1, (d >= 4) ? (4 - (s16)d) : d);
                    }
                    break;
                case 0x40:
                    if(encoder == (FP_E_OP1LVL + op)){
                        d = CmdEditor_Clip(cmd.data & 0x7F, 0-incrementer, 7);
                        ret.data = (cmd.data & 0x80) | d;
                        FrontPanel_DrawNumber(FP_LED_DIG_MAIN_1, 0x7F-d);
                    }
                    break;
                case 0x50:
                    if(encoder == FP_E_ATTACK){
                        d = CmdEditor_Clip(cmd.data & 0x1F, incrementer, 5);
                        ret.data = (cmd.data & 0xE0) | d;
                        FrontPanel_DrawNumber(FP_LED_DIG_MAIN_1, d);
                    }else if(button == FP_B_KSR){
                        ret.data = cmd.data + 0x40;
                        FrontPanel_DrawNumber(FP_LED_DIG_MAIN_1, ret.data >> 6);
                    }
                    break;
                case 0x60:
                    if(encoder == FP_E_DEC1R){
                        d = CmdEditor_Clip(cmd.data & 0x1F, incrementer, 5);
                        ret.data = (cmd.data & 0xE0) | d;
                        FrontPanel_DrawNumber(FP_LED_DIG_MAIN_1, d);
                    }else if(button == FP_B_LFOAM){
                        ret.data = cmd.data ^ 0x80;
                        FrontPanel_DrawNumber(FP_LED_DIG_MAIN_1, ret.data >> 7);
                    }
                    break;
                case 0x70:
                    if(encoder == FP_E_DEC2R){
                        d = CmdEditor_Clip(cmd.data & 0x1F, incrementer, 5);
                        ret.data = (cmd.data & 0xE0) | d;
                        FrontPanel_DrawNumber(FP_LED_DIG_MAIN_1, d);
                    }
                    break;
                case 0x80:
                    if(encoder == FP_E_RELRATE){
                        d = CmdEditor_Clip(cmd.data & 0x0F, incrementer, 4);
                        ret.data = (cmd.data & 0xF0) | d;
                        FrontPanel_DrawNumber(FP_LED_DIG_MAIN_1, d);
                    }else if(encoder == FP_E_DECLVL){
                        d = CmdEditor_Clip(cmd.data >> 4, 0-incrementer, 4);
                        ret.data = (cmd.data & 0x0F) | (d << 4);
                        FrontPanel_DrawNumber(FP_LED_DIG_MAIN_1, 15-d);
                    }
                    break;
                case 0x90:
                    if(button == FP_B_SSGON){
                        ret.data = cmd.data ^ 0x08;
                    }else if(button == FP_B_SSGINIT){
                        ret.data = cmd.data ^ 0x04;
                    }else if(button == FP_B_SSGTGL){
                        ret.data = cmd.data ^ 0x02;
                    }else if(button == FP_B_SSGHOLD){
                        ret.data = cmd.data ^ 0x01;
                    }
                    if(ret.data > cmd.data){ //Hack for "turned anything on"
                        FrontPanel_DrawNumber(FP_LED_DIG_MAIN_1, 1);
                    }else if(ret.data < cmd.data){ //Turned anything off
                        FrontPanel_DrawNumber(FP_LED_DIG_MAIN_1, 0);
                    }
                    break;
            }
        }else if(cmd.addr <= 0xAE && cmd.addr >= 0xA8){
            //Channel 3 extra frequency
            if(reqvoice < 0xFF && reqvoice != 3) return cmd;
            if(cmd.addr > 0xAE || cmd.addr < 0xAC) return cmd;
            if(reqop < 0xFF){
                if(!((cmd.addr == 0xAD && reqop == 0)
                   ||(cmd.addr == 0xAE && reqop == 1)
                   ||(cmd.addr == 0xAC && reqop == 2))) return cmd;
            }
            if(encoder == FP_E_OCTAVE){
                d = CmdEditor_Clip((cmd.data >> 3) & 0x07, incrementer, 3);
                ret.data = (cmd.data & 0xC7) | (d << 3);
                FrontPanel_DrawNumber(FP_LED_DIG_MAIN_1, d);
            }else if(encoder == FP_E_FREQ){
                f = CmdEditor_Clip16(((u16)(cmd.data & 7) << 8) | cmd.data2, incrementer, 12);
                ret.data = (cmd.data & 0xF8) | (f >> 8);
                ret.data2 = f & 0x00FF;
                FrontPanel_DrawNumber(FP_LED_DIG_MAIN_1, f);
            }
        }else if(cmd.addr <= 0xB6){
            //Channel command
            if(reqvoice < 0xFF){
                chan = (cmd.addr & 0x03);
                if(chan == 0x03) return cmd;
                chan += addrhi; //Add 3 for channels 4, 5, 6
                if(reqvoice != chan+1) return cmd;
            }
            reg = cmd.addr & 0xFC;
            if(reg == 0xA4){
                //Frequency write
                if(reqop < 0xFF && reqop != 3) return cmd; //If op required for frequency write,
                //we're editing fm3_special, and in that case only edit op 3 command
                if(encoder == FP_E_OCTAVE){
                    d = CmdEditor_Clip((cmd.data >> 3) & 0x07, incrementer, 3);
                    ret.data = (cmd.data & 0xC7) | (d << 3);
                    FrontPanel_DrawNumber(FP_LED_DIG_MAIN_1, d);
                }else if(encoder == FP_E_FREQ){
                    f = CmdEditor_Clip16(((u16)(cmd.data & 7) << 8) | cmd.data2, incrementer, 12);
                    ret.data = (cmd.data & 0xF8) | (f >> 8);
                    ret.data2 = f & 0x00FF;
                    FrontPanel_DrawNumber(FP_LED_DIG_MAIN_1, f);
                }
            }else if(reg == 0xB0){
                if(button == FP_B_ALG){
                    ret.data = (cmd.data & 0xF8) | (state & 7);
                    FrontPanel_DrawNumber(FP_LED_DIG_MAIN_1, state);
                }else if(encoder == FP_E_FEEDBACK){
                    d = CmdEditor_Clip((cmd.data >> 3)&7, incrementer, 3);
                    ret.data = (cmd.data & 0xC7) | (d << 3);
                    FrontPanel_DrawNumber(FP_LED_DIG_MAIN_1, d);
                }
            }else if(reg == 0xB4){
                u8 oldv;
                if(encoder == FP_E_LFOFDEP){
                    oldv = cmd.data & 7;
                    d = CmdEditor_Clip(oldv, incrementer, 3);
                    if((oldv == 0) ^ (d == 0)) couldbeusagechange = 1;
                    ret.data = (cmd.data & 0xF8) | d;
                    FrontPanel_DrawNumber(FP_LED_DIG_MAIN_1, d);
                }else if(encoder == FP_E_LFOADEP){
                    oldv = (cmd.data >> 4)&3;
                    d = CmdEditor_Clip(oldv, incrementer, 2);
                    if((oldv == 0) ^ (d == 0)) couldbeusagechange = 1;
                    ret.data = (cmd.data & 0xCF) | (d << 4);
                    FrontPanel_DrawNumber(FP_LED_DIG_MAIN_1, d);
                }else if(button == FP_B_OUT){
                    ret.data = cmd.data + 0x40;
                    FrontPanel_DrawNumber(FP_LED_DIG_MAIN_1, ret.data >> 6);
                }
            }
        }
    }else if(cmd.cmd >= 0x80 && cmd.cmd <= 0x8F){
        //OPN2 DAC write
        if(cmd.addr != 0x2A) return cmd;
        if(encoder == FP_E_DATAWHEEL){
            ret.data = CmdEditor_Clip(cmd.data, incrementer, 8);
            FrontPanel_DrawNumber(FP_LED_DIG_MAIN_1, ret.data);
        }else if(encoder == FP_E_OCTAVE){
            d = CmdEditor_Clip(cmd.cmd & 0x0F, incrementer, 4);
            ret.cmd = 0x80 | d;
            FrontPanel_DrawNumber(FP_LED_DIG_MAIN_1, d);
        }
    }else if(cmd.cmd >= 0x70 && cmd.cmd <= 0x7F){
        //Short wait
        if(encoder == FP_E_OCTAVE){
            d = CmdEditor_Clip(cmd.cmd & 0x0F, incrementer, 4);
            ret.cmd = 0x80 | d;
            FrontPanel_DrawNumber(FP_LED_DIG_MAIN_1, d);
        }
    }else if(cmd.cmd == 0x61){
        //Long wait
        s32 t = (s32)(cmd.data) | ((s32)cmd.data2 << 8);
        if(encoder == FP_E_FREQ || encoder == FP_E_OP4LVL){
            t += incrementer;
        }else if(encoder == FP_E_OP3LVL){
            t += incrementer << 4;
        }else if(encoder == FP_E_OP2LVL){
            t += incrementer << 8;
        }else if(encoder == FP_E_OP1LVL || encoder == FP_E_OCTAVE){
            t += incrementer << 12;
        }
        if(t < 0) t = 0;
        if(t > 0xFFFF) t = 0xFFFF;
        ret.data = (t & 0x000000FF);
        ret.data2 = (t & 0x0000FF00) >> 8;
        FrontPanel_DrawNumberHex(FP_LED_DIG_MAIN_1, (u16)t);
    }else if(cmd.cmd == 0x62 || cmd.cmd == 0x63){
        //60 Hz or 50 Hz wait--no editing possible
    }else{
        //Unsupported command, should not be here
    }
    if(ret.all != cmd.all){
        //A change was made, if we're working on an init VGM we have to soft-flush the program
        if(selvgm == selprogram->initsource){
            SyEng_SoftFlushProgram(selprogram);
        }
        if(couldbeusagechange){
            DBG("EditCmd signaling possible usage change");
            Mode_Vgm_SignalVgmUsageChange();
        }
    }
    return ret;
}

u8 EditState(VgmSource* vgs, VgmHead* head, u8 encoder, s32 incrementer, u8 button, u8 state, u8 voice, u8 op){
    VgmSourceRAM* vsr = (VgmSourceRAM*)vgs->data;
    VgmChipWriteCmd cmd, modcmd;
    u32 a = head->srcaddr;
    u8 fm3_special = head->channel[3].nodata ? 0 : (genesis[head->channel[3].map_chip].opn2.ch3_mode > 0);
    if(!fm3_special && (encoder == FP_E_OCTAVE || encoder == FP_E_FREQ)){
        op = 0xFF; //Allow editing octave/freq on any op if we're not fm3_special
    }
    while(1){
        cmd = vsr->cmds[a];
        modcmd = EditCmd(cmd, encoder, incrementer, button, state, voice, op);
        if(modcmd.all != cmd.all){
            vsr->cmds[a] = modcmd;
            if(modcmd.cmd == 0x50){
                modcmd.cmd = 0;
            }else if((modcmd.cmd & 0xFE) == 0x52){
                modcmd.cmd -= 0x50;
            }else if(modcmd.cmd >= 0x80 && modcmd.cmd <= 0x8F){
                modcmd.cmd = 2;
            }else{
                return 1; //Don't actually send the command to the chip
            }
            VGM_Head_doMapping(head, &modcmd); //Map the command to the chip according to the previewpi's head's mapping
            VGM_Tracker_Enqueue(modcmd, 0);
            return 1;
        }
        if(a == 0) return 0;
        --a;
    }
}

VgmSource* CreateNewVGM(u8 type, VgmUsageBits usage){
    if(type >= 3) return NULL;
    VgmSource* vs = VGM_SourceRAM_Create();
    u32 a = 0, u;
    u8 i, reg, op, regcnt, regval;
    VgmChipWriteCmd cmd;
    switch(type){
        case 0:
            //Init
            u = usage.all;
            //OPN2 globals
            if(usage.opn2_globals){
                VGM_SourceRAM_InsertCmd(vs, a++, (VgmChipWriteCmd){
                    .cmd=0x52, .addr=0x21, .data = 0x00, .data2=0});
                VGM_SourceRAM_InsertCmd(vs, a++, (VgmChipWriteCmd){
                    .cmd=0x52, .addr=0x2C, .data = 0x00, .data2=0});
            }
            //LFO enable
            if(usage.lfomode){
                VGM_SourceRAM_InsertCmd(vs, a++, (VgmChipWriteCmd){
                    .cmd=0x52, .addr=0x22, .data = 0x08 | usage.lfofixedspeed, .data2=0});
            }
            //Ch3 special
            if(usage.fm3_special){
                VGM_SourceRAM_InsertCmd(vs, a++, (VgmChipWriteCmd){
                    .cmd=0x52, .addr=0x27, .data = 0x40, .data2=0});
                VGM_SourceRAM_InsertCmd(vs, a++, (VgmChipWriteCmd){
                    .cmd=0x52, .addr=0x24, .data = 0x00, .data2=0});
            }
            //OPN2 channels
            for(i=0; i<6; ++i){
                if(u & 0x00000001){
                    //Operators
                    for(regcnt=0; regcnt<7; ++regcnt){
                        reg = (regcnt + 3) << 4;
                        for(op=0; op<4; ++op){
                            regval = init_reg_vals[regcnt];
                            if(regcnt == 1 && op == 3){
                                regval = 0x08;
                            }
                            VGM_SourceRAM_InsertCmd(vs, a++, (VgmChipWriteCmd){
                                .cmd = 0x52 | (i >= 3), 
                                .addr = reg | (i % 3) | ((op & 1) << 3) | ((op & 2) << 1), 
                                .data=regval, .data2=0});
                        }
                    }
                    //Channel registers
                    VGM_SourceRAM_InsertCmd(vs, a++, (VgmChipWriteCmd){
                        .cmd = 0x52 | (i >= 3), .addr = 0xB0 | (i % 3), .data=0, .data2=0});
                    VGM_SourceRAM_InsertCmd(vs, a++, (VgmChipWriteCmd){
                        .cmd = 0x52 | (i >= 3), .addr = 0xB4 | (i % 3), .data=0xC0, .data2=0});
                }
                u >>= 1;
            }
            //PSG channels
            if(usage.noise){
                if(usage.noisefreqsq3){
                    VGM_SourceRAM_InsertCmd(vs, a++, (VgmChipWriteCmd){
                        .cmd=0x50, .addr=0x00, .data = 0b11100111, .data2=0}); //Noise setup
                }else{
                    VGM_SourceRAM_InsertCmd(vs, a++, (VgmChipWriteCmd){
                        .cmd=0x50, .addr=0x00, .data = 0b11100100, .data2=0});
                }
            }
            break;
        case 1:
            //Key on
            u = usage.all;
            for(i=0; i<6; ++i){
                if(u & 0x00000001){
                    if(i == 2 && usage.fm3_special){
                        //Other operator frequencies
                        cmd = VGM_getOPN2Frequency(72, 0, genesis_clock_opn2);
                        cmd.cmd  = 0x52;
                        cmd.addr = 0xAD;
                        VGM_SourceRAM_InsertCmd(vs, a++, cmd);
                        cmd = VGM_getOPN2Frequency(67, 0, genesis_clock_opn2);
                        cmd.cmd  = 0x52;
                        cmd.addr = 0xAE;
                        VGM_SourceRAM_InsertCmd(vs, a++, cmd);
                        cmd = VGM_getOPN2Frequency(64, 0, genesis_clock_opn2);
                        cmd.cmd  = 0x52;
                        cmd.addr = 0xAC;
                        VGM_SourceRAM_InsertCmd(vs, a++, cmd);
                    }
                    cmd = VGM_getOPN2Frequency(60, 0, genesis_clock_opn2);
                    cmd.cmd  = 0x52 + (i >= 3);
                    cmd.addr = 0xA4 + (i % 3);
                    VGM_SourceRAM_InsertCmd(vs, a++, cmd);
                    VGM_SourceRAM_InsertCmd(vs, a++, (VgmChipWriteCmd){
                            .cmd=0x52, .addr=0x28, .data=0xF0|(i>=3 ? i+1 : i), .data2=0});
                }
                u >>= 1;
            }
            u = usage.all >> 24;
            for(i=0; i<3; ++i){
                if(u & 0x00000001){
                    cmd = VGM_getPSGFrequency(60, 0, genesis_clock_psg);
                    cmd.cmd  = 0x50;
                    cmd.addr = 0x00;
                    cmd.data |= 0b10000000 | (i << 5);
                    VGM_SourceRAM_InsertCmd(vs, a++, cmd);
                    if(i != 2 || !usage.noisefreqsq3){
                        //Don't key on sq3 if it's just being used to chonge noise frequency
                        VGM_SourceRAM_InsertCmd(vs, a++, (VgmChipWriteCmd){
                                .cmd=0x50, .addr=0x00, .data = 0b10010000 | (i << 5), .data2=0});
                    }
                }
                u >>= 1;
            }
            if(usage.noise){
                VGM_SourceRAM_InsertCmd(vs, a++, (VgmChipWriteCmd){
                        .cmd=0x50, .addr=0x00, .data = 0b11110000, .data2=0});
            }
            break;
        case 2:
            //Key off
            u = usage.all;
            for(i=0; i<6; ++i){
                if(u & 0x00000001){
                    VGM_SourceRAM_InsertCmd(vs, a++, (VgmChipWriteCmd){
                            .cmd=0x52, .addr=0x28, .data=(i>=3 ? i+1 : i), .data2=0});
                }
                u >>= 1;
            }
            u = usage.all >> 24;
            for(i=0; i<4; ++i){
                if(u & 0x00000001){
                    VGM_SourceRAM_InsertCmd(vs, a++, (VgmChipWriteCmd){
                            .cmd=0x50, .addr=0x00, .data = 0b10011111 | (i << 5), .data2=0});
                }
                u >>= 1;
            }
            break;
    }
    VGM_Source_UpdateUsage(vs);
    return vs;
}




