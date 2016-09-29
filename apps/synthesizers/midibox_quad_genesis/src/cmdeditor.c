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


void DrawCmdLine(VgmChipWriteCmd cmd, u8 row){
    u16 outbits = 0;
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
            }else if(cmd.addr == 0x24 || cmd.addr == 0x25 || cmd.addr == 0x27){
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
    if(cmd.cmd == 0x50){
        //PSG write
        u8 voice;
        voice = (cmd.data & 0x60) >> 5;
        FrontPanel_GenesisLEDSet(0, 8+voice, 0, !clear);
        if(cmd.data & 0x10){
            //Attenuation
            FrontPanel_LEDRingSet(FP_LEDR_PSGVOL, clear+1, 15 - (cmd.data & 0x0F));
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
                FrontPanel_LEDRingSet(FP_LEDR_PSGFREQ, clear << 1, ((1023 - f) >> 4) & 0xF);
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
                        FrontPanel_LEDRingSet(FP_LEDR_LFOFREQ, 1+clear, cmd.data & 7);
                        FrontPanel_LEDSet(FP_LED_LFO, ((cmd.data >> 3) & 1) && !clear);
                        FrontPanel_GenesisLEDSet(0, 0, 0, !clear);
                        break;
                    case 0x24:
                    case 0x25:
                        //TODO data2 for TimerA frequency write?
                        //FrontPanel_LEDRingSet(FP_LEDR_CSMFREQ, 1, genesis[g].opn2.timera_high >> 4);
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
                        FrontPanel_LEDRingSet(FP_LEDR_HARM, clear+1, cmd.data & 0x0F);
                        i = (cmd.data >> 4) & 7;
                        FrontPanel_LEDRingSet(FP_LEDR_DETUNE, clear << 1, DETUNE_DISPLAY(i));
                        break;
                    case 0x40:
                        FrontPanel_LEDRingSet(FP_LEDR_OP1LVL + op, clear+1, (127 - (cmd.data & 0x7F)) >> 3);
                        break;
                    case 0x50:
                        FrontPanel_LEDRingSet(FP_LEDR_ATTACK, clear+1, (cmd.data & 0x1F) >> 1);
                        FrontPanel_LEDSet(FP_LED_KSR, cmd.data >= 0x40 && !clear);
                        break;
                    case 0x60:
                        FrontPanel_LEDRingSet(FP_LEDR_DEC1R, clear+1, (cmd.data & 0x1F) >> 1);
                        FrontPanel_LEDSet(FP_LED_LFOAM, cmd.data >= 0x80 && !clear);
                        break;
                    case 0x70:
                        FrontPanel_LEDRingSet(FP_LEDR_DEC2R, clear+1, (cmd.data & 0x1F) >> 1);
                        break;
                    case 0x80:
                        FrontPanel_LEDRingSet(FP_LEDR_RELRATE, clear+1, cmd.data & 0x0F);
                        FrontPanel_LEDRingSet(FP_LEDR_DECLVL, clear+1, 15 - (cmd.data >> 4));
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
            if(cmd.addr == 0xAD) op = 2;
            else if(cmd.addr == 0xAE) op = 3;
            else if(cmd.addr == 0xAC) op = 4;
            else return;
            FrontPanel_GenesisLEDSet(0, 3, 0, !clear);
            FrontPanel_LEDSet(FP_LED_OPNODE_R_1 + op, !clear);
            if(clear){
                FrontPanel_DrawDigit(FP_LED_DIG_OCT, ' ');
                FrontPanel_DrawDigit(FP_LED_DIG_FREQ_1, ' ');
                FrontPanel_DrawDigit(FP_LED_DIG_FREQ_2, ' ');
                FrontPanel_DrawDigit(FP_LED_DIG_FREQ_3, ' ');
                FrontPanel_DrawDigit(FP_LED_DIG_FREQ_4, ' ');
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
                        FrontPanel_DrawDigit(FP_LED_DIG_FREQ_1, ' ');
                        FrontPanel_DrawDigit(FP_LED_DIG_FREQ_2, ' ');
                        FrontPanel_DrawDigit(FP_LED_DIG_FREQ_3, ' ');
                        FrontPanel_DrawDigit(FP_LED_DIG_FREQ_4, ' ');
                    }else{
                        FrontPanel_DrawDigit(FP_LED_DIG_OCT, '0' + ((cmd.data >> 3) & 7));
                        FrontPanel_DrawNumber(FP_LED_DIG_FREQ_1, ((u16)(cmd.data & 7) << 8) | cmd.data2);
                    }
                }else if(reg == 0xB0){
                    FrontPanel_DrawAlgorithm((cmd.data & 7) + (clear << 4));
                    FrontPanel_LEDRingSet(FP_LEDR_FEEDBACK, clear+1, (cmd.data >> 3) & 7);
                    FrontPanel_LEDSet(FP_LED_FEEDBACK, (cmd.data & 0x38) > 0 && !clear);
                }else if(reg == 0xB4){
                    FrontPanel_LEDRingSet(FP_LEDR_LFOFDEP, clear+1, (cmd.data & 7));
                    FrontPanel_LEDRingSet(FP_LEDR_LFOADEP, clear+1, (cmd.data >> 4) & 3);
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
    }else if((cmd.cmd >= 0x70 && cmd.cmd <= 0x7F) | (cmd.cmd >= 0x61 && cmd.cmd <= 0x63)){
        //Wait
        FrontPanel_LEDSet(FP_LED_TIME_R, !clear);
    }else{
        //Unsupported command, should not be here
        FrontPanel_DrawAlgorithm(8 + (clear << 4));
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
                    case 0x25:
                        //TODO data2 for TimerA frequency write?
                        //FrontPanel_LEDRingSet(FP_LEDR_CSMFREQ, 1, genesis[g].opn2.timera_high >> 4);
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
            if(cmd.addr == 0xAD) op = 2;
            else if(cmd.addr == 0xAE) op = 3;
            else if(cmd.addr == 0xAC) op = 4;
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

VgmChipWriteCmd EditCmd(VgmChipWriteCmd cmd, u8 encoder, s32 incrementer, u8 button, u8 state, u8 reqvoice, u8 reqop){
    VgmChipWriteCmd ret = {.all = cmd.all};
    if(cmd.cmd == 0x50){
        //PSG write
        u8 voice = (cmd.data & 0x60) >> 5;
        if(reqvoice < 0xFF && reqvoice != voice+8) return cmd;
        if(cmd.data & 0x10){
            //Attenuation
            if(encoder == FP_E_PSGVOL){
                ret.data = (cmd.data & 0xF0) | CmdEditor_Clip(cmd.data & 0x0F, 0-incrementer, 4);
            }
        }else{
            if(voice == 3){
                //Noise parameters
                if(button == FP_B_NSFREQ){
                    ret.data = (cmd.data & 0xFC) | ((cmd.data+1) & 0x03);
                }else if(button == FP_B_NSTYPE){
                    ret.data = cmd.data ^ 0x04;
                }
            }else{
                if(encoder == FP_E_PSGFREQ){
                    u16 f = (cmd.data & 0x0F) | ((u16)(cmd.data2 & 0x3F) << 4);
                    s32 temp = (s32)f + incrementer;
                    if(temp < 0) temp = 0;
                    if(temp >= 0x400) temp = 0x3FF;
                    f = temp;
                    ret.data = (cmd.data & 0xF0) | (f & 0x000F);
                    ret.data2 = (ret.data2 & 0xC0) | (f >> 4);
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
                            ret.data = (cmd.data & 0x0F) | (state << 4);
                        }
                        break;
                    case 0x21:
                        //Test Register 1
                        if(reqvoice < 0xFF && reqvoice != 0) return cmd;
                        if(button == FP_B_UGLY){
                            ret.data = cmd.data ^ 0x10;
                        }else if(button == FP_B_EG){
                            ret.data = cmd.data ^ 0x20;
                        }
                        break;
                    case 0x22:
                        //LFO Register
                        if(reqvoice < 0xFF && reqvoice != 0) return cmd;
                        if(button == FP_B_LFO){
                            ret.data = cmd.data ^ 0x08;
                        }else if(encoder == FP_E_LFOFREQ){
                            ret.data = (cmd.data & 0xF8) | CmdEditor_Clip(cmd.data & 7, incrementer, 3);
                        }
                        break;
                    case 0x24:
                    case 0x25:
                        if(reqvoice < 0xFF && reqvoice != 3) return cmd;
                        //TODO data2 for TimerA frequency write?
                        //FrontPanel_LEDRingSet(FP_LEDR_CSMFREQ, 1, genesis[g].opn2.timera_high >> 4);
                        break;
                    case 0x27:
                        if(reqvoice < 0xFF && reqvoice != 3) return cmd;
                        if(button == FP_B_CH3MODE){
                            ret.data = cmd.data + 0x40;
                        }else if(button == FP_B_CH3FAST){
                            ret.data = cmd.data ^ 0x01;
                        }
                        break;
                    case 0x2A:
                        if(reqvoice < 0xFF && reqvoice != 7) return cmd;
                        if(encoder != FP_E_DATAWHEEL){
                            ret.data = CmdEditor_Clip(cmd.data, incrementer, 8);
                        }
                        break;
                    case 0x2B:
                        if(reqvoice < 0xFF && reqvoice != 7) return cmd;
                        if(button == FP_B_DACEN){
                            ret.data = cmd.data ^ 0x80;
                        }
                        break;
                    case 0x2C:
                        if(reqvoice < 0xFF && reqvoice != 0) return cmd;
                        if(button == FP_B_DACOVR){
                            ret.data = cmd.data ^ 0x20;
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
                        ret.data = (cmd.data & 0xF0) | CmdEditor_Clip(cmd.data & 0x0F, incrementer, 4);
                    }else if(encoder == FP_E_DETUNE){
                        u8 i = (cmd.data >> 4) & 7;
                        if(i == 5 && incrementer > 0){
                            i = incrementer - 1;
                        }else if(i == 0 && incrementer < 0){
                            i = 4 - incrementer;
                        }else if(i == 7 && incrementer < 0){
                            return ret;
                        }else if(i == 3 && incrementer > 0){
                            return ret;
                        }else if(i >= 4){
                            i -= incrementer;
                        }else{
                            i += incrementer;
                        }
                        ret.data = (i << 4) | (cmd.data & 0x0F);
                    }
                    break;
                case 0x40:
                    if(encoder == (FP_E_OP1LVL + op)){
                        ret.data = (cmd.data & 0x80) | CmdEditor_Clip(cmd.data & 0x7F, 0-incrementer, 7);
                    }
                    break;
                case 0x50:
                    if(encoder == FP_E_ATTACK){
                        ret.data = (cmd.data & 0xE0) | CmdEditor_Clip(cmd.data & 0x1F, incrementer, 5);
                    }else if(button == FP_B_KSR){
                        ret.data = cmd.data + 0x40;
                    }
                    break;
                case 0x60:
                    if(encoder == FP_E_DEC1R){
                        ret.data = (cmd.data & 0xE0) | CmdEditor_Clip(cmd.data & 0x1F, incrementer, 5);
                    }else if(button == FP_B_LFOAM){
                        ret.data = cmd.data ^ 0x80;
                    }
                    break;
                case 0x70:
                    if(encoder == FP_E_DEC2R){
                        ret.data = (cmd.data & 0xE0) | CmdEditor_Clip(cmd.data & 0x1F, incrementer, 5);
                    }
                    break;
                case 0x80:
                    if(encoder == FP_E_RELRATE){
                        ret.data = (cmd.data & 0xF0) | CmdEditor_Clip(cmd.data & 0x0F, incrementer, 4);
                    }else if(encoder == FP_E_DECLVL){
                        ret.data = (cmd.data & 0x0F) | (CmdEditor_Clip(cmd.data >> 4, 0-incrementer, 4) << 4);
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
                    break;
            }
        }else if(cmd.addr <= 0xAE && cmd.addr >= 0xA8){
            //Channel 3 extra frequency
            if(reqvoice < 0xFF && reqvoice != 3) return cmd;
            if(cmd.addr > 0xAE || cmd.addr < 0xAC) return cmd;
            if(encoder == FP_E_OCTAVE){
                ret.data = (cmd.data & 0xC7) | (CmdEditor_Clip((cmd.data >> 3) & 0x07, incrementer, 3) << 3);
            }else if(encoder == FP_E_FREQ){
                u16 f = ((u16)(cmd.data & 7) << 8) | cmd.data2;
                s32 temp = (s32)f + incrementer;
                if(temp < 0) temp = 0;
                if(temp >= 0x800) temp = 0x7FF;
                f = temp;
                ret.data = (cmd.data & 0xF8) | (f >> 8);
                ret.data2 = f & 0x00FF;
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
                if(encoder == FP_E_OCTAVE){
                    ret.data = (cmd.data & 0xC7) | (CmdEditor_Clip((cmd.data >> 3) & 0x07, incrementer, 3) << 3);
                }else if(encoder == FP_E_FREQ){
                    u16 f = ((u16)(cmd.data & 7) << 8) | cmd.data2;
                    s32 temp = (s32)f + incrementer;
                    if(temp < 0) temp = 0;
                    if(temp >= 0x800) temp = 0x7FF;
                    f = temp;
                    ret.data = (cmd.data & 0xF8) | (f >> 8);
                    ret.data2 = f & 0x00FF;
                }
            }else if(reg == 0xB0){
                if(button == FP_B_ALG){
                    ret.data = (cmd.data & 0xF8) | (state & 7);
                }else if(encoder == FP_E_FEEDBACK){
                    ret.data = (cmd.data & 0xC7) | (CmdEditor_Clip((cmd.data >> 3)&7, incrementer, 3) << 3);
                }
            }else if(reg == 0xB4){
                u8 oldv, newv;
                if(encoder == FP_E_LFOFDEP){
                    oldv = cmd.data & 7;
                    newv = CmdEditor_Clip(oldv, incrementer, 3);
                    if((oldv == 0) ^ (newv == 0)){
                        Mode_Vgm_SignalVgmUsageChange();
                    }
                    ret.data = (cmd.data & 0xF8) | newv;
                }else if(encoder == FP_E_LFOADEP){
                    oldv = (cmd.data >> 4)&3;
                    newv = CmdEditor_Clip(oldv, incrementer, 2);
                    if((oldv == 0) ^ (newv == 0)){
                        Mode_Vgm_SignalVgmUsageChange();
                    }
                    ret.data = (cmd.data & 0xCF) | (newv << 4);
                }else if(button == FP_B_OUT){
                    ret.data = cmd.data + 0x40;
                }
            }
        }
    }else if(cmd.cmd >= 0x80 && cmd.cmd <= 0x8F){
        //OPN2 DAC write
        if(cmd.addr != 0x2A) return cmd;
        if(encoder != FP_E_DATAWHEEL){
            ret.data = CmdEditor_Clip(cmd.data, incrementer, 8);
        }
    }else if(cmd.cmd >= 0x70 && cmd.cmd <= 0x7F){
        //TODO sprintf(desc, "Short Wait:%d", cmd.cmd - 0x6F);
    }else if(cmd.cmd == 0x61){
        //TODO sprintf(desc, "Long Wait:%d", cmd.data | ((u16)cmd.data2 << 8));
    }else if(cmd.cmd == 0x62){
        //TODO sprintf(desc, "60Hz Wait [%d]", VGM_DELAY62);
    }else if(cmd.cmd == 0x63){
        //TODO sprintf(desc, "50Hz Wait [%d]", VGM_DELAY63);
    }else{
        //Unsupported command, should not be here
    }
    if(ret.all != cmd.all){
        //A change was made, if we're working on an init VGM we have to soft-flush the program
        if(selvgm == selprogram->initsource){
            SyEng_SoftFlushProgram(selprogram);
        }
    }
    return ret;
}

u8 EditState(VgmSource* vgs, VgmHead* vh, u8 encoder, s32 incrementer, u8 button, u8 state, u8 voice, u8 op){
    VgmSourceRAM* vsr = (VgmSourceRAM*)vgs->data;
    VgmHeadRAM* vhr = (VgmHeadRAM*)vh->data;
    VgmChipWriteCmd cmd, modcmd;
    u32 a = vhr->srcaddr;
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
            VGM_Head_doMapping(vh, &modcmd); //Map the command to the chip according to the previewpi's head's mapping
            VGM_Tracker_Enqueue(modcmd, 0);
            return 1;
        }
        if(a == 0) return 0;
        --a;
    }
}

void UpdateProgramUsage(synprogram_t* prog, VgmSource* vgs){
    VGM_Source_UpdateUsage(vgs);
    VgmUsageBits usage = prog->initsource->usage;
    usage.all |= prog->noteonsource->usage.all;
    usage.all |= prog->noteoffsource->usage.all;
    if(usage.all != prog->usage.all){
        prog->usage.all = usage.all;
        SyEng_HardFlushProgram(prog);
        Mode_Vgm_SelectVgm(selvgm); //Force mode_vgm to get a new previewpi
    }
}










