/*
 * MIDIbox Quad Genesis: Demo programs
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
#include "syeng.h"

#include <genesis.h>
#include <vgm.h>
#include "syeng.h"

void DemoPrograms_Init(){
    ////////////////////////////////////////////////////////////////////////////
    //////////////////////////// OPN2 GRAND PIANO //////////////////////////////
    ////////////////////////////////////////////////////////////////////////////
    synprogram_t* prog = vgmh2_malloc(sizeof(synprogram_t));
    channels[1].program = prog;
    sprintf(prog->name, "Grand Piano");
    prog->rootnote = 72;
    prog->usage.all = 0;
    //Create init VGM file
    VgmSource* source = VGM_SourceRAM_Create();
    VgmSourceRAM* vsr = (VgmSourceRAM*)source->data;
    source->opn2clock = 8000000;
    vsr->numcmds = 30;
    VgmChipWriteCmd* data = vgmh2_malloc(30*sizeof(VgmChipWriteCmd));
    vsr->cmds = data;
    //data[0] = (VgmChipWriteCmd){.cmd = 0x02, .addr = 0x5C, .data = 0x1F, .data2 = 0}; //Set Ch1:Op4 attack rate to full
    //
    u8 i=0;
    data[i++] = (VgmChipWriteCmd){.cmd=0x52, .addr=0x30, .data=0x71 };
    data[i++] = (VgmChipWriteCmd){.cmd=0x52, .addr=0x38, .data=0x33 };
    data[i++] = (VgmChipWriteCmd){.cmd=0x52, .addr=0x34, .data=0x0D };
    data[i++] = (VgmChipWriteCmd){.cmd=0x52, .addr=0x3C, .data=0x01 };
    data[i++] = (VgmChipWriteCmd){.cmd=0x52, .addr=0x40, .data=0x23 };
    data[i++] = (VgmChipWriteCmd){.cmd=0x52, .addr=0x48, .data=0x26 };
    data[i++] = (VgmChipWriteCmd){.cmd=0x52, .addr=0x44, .data=0x2D };
    data[i++] = (VgmChipWriteCmd){.cmd=0x52, .addr=0x4C, .data=0x00 };
    data[i++] = (VgmChipWriteCmd){.cmd=0x52, .addr=0x50, .data=0x5F };
    data[i++] = (VgmChipWriteCmd){.cmd=0x52, .addr=0x58, .data=0x5F };
    data[i++] = (VgmChipWriteCmd){.cmd=0x52, .addr=0x54, .data=0x99 };
    data[i++] = (VgmChipWriteCmd){.cmd=0x52, .addr=0x5C, .data=0x94 };
    data[i++] = (VgmChipWriteCmd){.cmd=0x52, .addr=0x60, .data=0x05 };
    data[i++] = (VgmChipWriteCmd){.cmd=0x52, .addr=0x68, .data=0x05 };
    data[i++] = (VgmChipWriteCmd){.cmd=0x52, .addr=0x64, .data=0x05 };
    data[i++] = (VgmChipWriteCmd){.cmd=0x52, .addr=0x6C, .data=0x07 };
    data[i++] = (VgmChipWriteCmd){.cmd=0x52, .addr=0x70, .data=0x00 };
    data[i++] = (VgmChipWriteCmd){.cmd=0x52, .addr=0x78, .data=0x00 };
    data[i++] = (VgmChipWriteCmd){.cmd=0x52, .addr=0x74, .data=0x00 };
    data[i++] = (VgmChipWriteCmd){.cmd=0x52, .addr=0x7C, .data=0x00 };
    data[i++] = (VgmChipWriteCmd){.cmd=0x52, .addr=0x80, .data=0x11 };
    data[i++] = (VgmChipWriteCmd){.cmd=0x52, .addr=0x88, .data=0x11 };
    data[i++] = (VgmChipWriteCmd){.cmd=0x52, .addr=0x84, .data=0x11 };
    data[i++] = (VgmChipWriteCmd){.cmd=0x52, .addr=0x8C, .data=0xA6 };
    data[i++] = (VgmChipWriteCmd){.cmd=0x52, .addr=0x90, .data=0x00 };
    data[i++] = (VgmChipWriteCmd){.cmd=0x52, .addr=0x98, .data=0x00 };
    data[i++] = (VgmChipWriteCmd){.cmd=0x52, .addr=0x94, .data=0x00 };
    data[i++] = (VgmChipWriteCmd){.cmd=0x52, .addr=0x9C, .data=0x00 };
    data[i++] = (VgmChipWriteCmd){.cmd=0x52, .addr=0xB0, .data=0x32 };
    data[i++] = (VgmChipWriteCmd){.cmd=0x52, .addr=0xB4, .data=0xC0 };
    //
    VGM_Source_UpdateUsage(source);
    prog->initsource = source;
    //Create note-on VGM file
    source = VGM_SourceRAM_Create();
    vsr = (VgmSourceRAM*)source->data;
    source->opn2clock = 8000000;
    vsr->numcmds = 2;
    data = vgmh2_malloc(2*sizeof(VgmChipWriteCmd));
    vsr->cmds = data;
    data[0] = VGM_getOPN2Frequency(60, 0, 8000000); //Middle C
        data[0].cmd  = 0x52;
        data[0].addr = 0xA4;
    data[1] = (VgmChipWriteCmd){.cmd=0x52, .addr=0x28, .data=0xF0, .data2=0}; //Key on Ch1
    VGM_Source_UpdateUsage(source);
    prog->noteonsource = source;
    //Create note-off VGM file
    source = VGM_SourceRAM_Create();
    vsr = (VgmSourceRAM*)source->data;
    source->opn2clock = 8000000;
    vsr->numcmds = 1;
    data = vgmh2_malloc(1*sizeof(VgmChipWriteCmd));
    vsr->cmds = data;
    data[0] = (VgmChipWriteCmd){.cmd=0x52, .addr=0x28, .data=0x00, .data2=0}; //Key off Ch1
    VGM_Source_UpdateUsage(source);
    prog->noteoffsource = source;
    SyEng_RecalcProgramUsage(prog);
    ////////////////////////////////////////////////////////////////////////////
    /////////////////////////// OPN2 3-VOICE CHORDS ////////////////////////////
    ////////////////////////////////////////////////////////////////////////////
    prog = vgmh2_malloc(sizeof(synprogram_t));
    channels[5].program = prog;
    sprintf(prog->name, "3Voice Chord");
    prog->rootnote = 60;
    prog->usage.all = 0;
    //Create init VGM file
    source = VGM_SourceRAM_Create();
    vsr = (VgmSourceRAM*)source->data;
    source->opn2clock = 8000000;

    vsr->numcmds = 27;
    data = vgmh2_malloc(27*sizeof(VgmChipWriteCmd));
    vsr->cmds = data;
    //data[0] = (VgmChipWriteCmd){.cmd = 0x02, .addr = 0x5C, .data = 0x1F, .data2 = 0}; //Set Ch1:Op4 attack rate to full
    //
    i=0;
    //Voice 2
    data[i++] = (VgmChipWriteCmd){.cmd=0x52, .addr=0x3D, .data=0x01 };
    //data[i++] = (VgmChipWriteCmd){.cmd=0x52, .addr=0x45, .data=0x7F };
    //data[i++] = (VgmChipWriteCmd){.cmd=0x52, .addr=0x4D, .data=0x00 };
    data[i++] = (VgmChipWriteCmd){.cmd=0x52, .addr=0x5D, .data=0x1F };
    //data[i++] = (VgmChipWriteCmd){.cmd=0x52, .addr=0x6D, .data=0x02 };
    //data[i++] = (VgmChipWriteCmd){.cmd=0x52, .addr=0x7D, .data=0x00 };
    data[i++] = (VgmChipWriteCmd){.cmd=0x52, .addr=0x8D, .data=0xFF };
    //data[i++] = (VgmChipWriteCmd){.cmd=0x52, .addr=0xB1, .data=0x00 };
    //data[i++] = (VgmChipWriteCmd){.cmd=0x52, .addr=0xB5, .data=0xC0 };
    //Voice 4
    data[i++] = (VgmChipWriteCmd){.cmd=0x53, .addr=0x3C, .data=0x01 };
    //data[i++] = (VgmChipWriteCmd){.cmd=0x53, .addr=0x44, .data=0x7F };
    //data[i++] = (VgmChipWriteCmd){.cmd=0x53, .addr=0x4C, .data=0x00 };
    data[i++] = (VgmChipWriteCmd){.cmd=0x53, .addr=0x5C, .data=0x1F };
    //data[i++] = (VgmChipWriteCmd){.cmd=0x53, .addr=0x6C, .data=0x02 };
    //data[i++] = (VgmChipWriteCmd){.cmd=0x53, .addr=0x7C, .data=0x00 };
    data[i++] = (VgmChipWriteCmd){.cmd=0x53, .addr=0x8C, .data=0xFF };
    //data[i++] = (VgmChipWriteCmd){.cmd=0x53, .addr=0xB0, .data=0x00 };
    //data[i++] = (VgmChipWriteCmd){.cmd=0x53, .addr=0xB4, .data=0xC0 };
    //Voice 5
    data[i++] = (VgmChipWriteCmd){.cmd=0x53, .addr=0x3D, .data=0x01 };
    //data[i++] = (VgmChipWriteCmd){.cmd=0x53, .addr=0x45, .data=0x7F };
    //data[i++] = (VgmChipWriteCmd){.cmd=0x53, .addr=0x4D, .data=0x00 };
    data[i++] = (VgmChipWriteCmd){.cmd=0x53, .addr=0x5D, .data=0x1F };
    //data[i++] = (VgmChipWriteCmd){.cmd=0x53, .addr=0x6D, .data=0x02 };
    //data[i++] = (VgmChipWriteCmd){.cmd=0x53, .addr=0x7D, .data=0x00 };
    data[i++] = (VgmChipWriteCmd){.cmd=0x53, .addr=0x8D, .data=0xFF };
    //data[i++] = (VgmChipWriteCmd){.cmd=0x53, .addr=0xB1, .data=0x00 };
    //data[i++] = (VgmChipWriteCmd){.cmd=0x53, .addr=0xB5, .data=0xC0 };
    //
    for(; i<27; ++i) data[i] = (VgmChipWriteCmd){.all=0};
    VGM_Source_UpdateUsage(source);
    prog->initsource = source;
    //Create note-on VGM file
    source = VGM_SourceRAM_Create();
    vsr = (VgmSourceRAM*)source->data;
    source->opn2clock = 8000000;
    vsr->numcmds = 6;
    data = vgmh2_malloc(6*sizeof(VgmChipWriteCmd));
    vsr->cmds = data;
    data[0] = VGM_getOPN2Frequency(60, 0, 8000000); //Middle C
        data[0].cmd  = 0x52;
        data[0].addr = 0xA5;
    data[1] = (VgmChipWriteCmd){.cmd=0x52, .addr=0x28, .data=0xF1, .data2=0}; //Key on Ch2
    data[2] = VGM_getOPN2Frequency(64, 0, 8000000); //E
        data[2].cmd  = 0x53;
        data[2].addr = 0xA4;
    data[3] = (VgmChipWriteCmd){.cmd=0x52, .addr=0x28, .data=0xF4, .data2=0}; //Key on Ch4
    data[4] = VGM_getOPN2Frequency(67, 0, 8000000); //G
        data[4].cmd  = 0x53;
        data[4].addr = 0xA5;
    data[5] = (VgmChipWriteCmd){.cmd=0x52, .addr=0x28, .data=0xF5, .data2=0}; //Key on Ch5
    VGM_Source_UpdateUsage(source);
    prog->noteonsource = source;
    //Create note-off VGM file
    source = VGM_SourceRAM_Create();
    vsr = (VgmSourceRAM*)source->data;
    source->opn2clock = 8000000;
    vsr->numcmds = 3;
    data = vgmh2_malloc(3*sizeof(VgmChipWriteCmd));
    vsr->cmds = data;
    data[0] = (VgmChipWriteCmd){.cmd=0x52, .addr=0x28, .data=0x01, .data2=0}; //Key off Ch2
    data[1] = (VgmChipWriteCmd){.cmd=0x52, .addr=0x28, .data=0x04, .data2=0}; //Key off Ch4
    data[2] = (VgmChipWriteCmd){.cmd=0x52, .addr=0x28, .data=0x05, .data2=0}; //Key off Ch5
    VGM_Source_UpdateUsage(source);
    prog->noteoffsource = source;
    SyEng_RecalcProgramUsage(prog);
    ////////////////////////////////////////////////////////////////////////////
    ///////////////////////////// PSG MARIO COIN ///////////////////////////////
    ////////////////////////////////////////////////////////////////////////////
    prog = vgmh2_malloc(sizeof(synprogram_t));
    channels[2].program = prog;
    sprintf(prog->name, "SMB1 Coin");
    prog->rootnote = 60;
    prog->usage.all = 0;
    //Create init VGM file
    prog->initsource = NULL;
    //Create note-on VGM file
    source = VGM_SourceRAM_Create();
    vsr = (VgmSourceRAM*)source->data;
    vsr->numcmds = 18;
    data = vgmh2_malloc(18*sizeof(VgmChipWriteCmd));
    vsr->cmds = data;
    data[0] = VGM_getPSGFrequency(60, 0, 3579545); //Middle C
        data[0].cmd  = 0x50;
        data[0].addr = 0x00;
        data[0].data |= 0b10100000;
    data[1] = (VgmChipWriteCmd){.cmd=0x50, .addr=0x00, .data=0b10110000, .data2=0}; //Turn on SQ2
    data[2] = (VgmChipWriteCmd){.cmd=0x61, .addr=0x00, .data=0x00, .data2=0x0B}; //Wait
    data[3] = VGM_getPSGFrequency(65, 0, 3579545); //F
        data[3].cmd  = 0x50;
        data[3].addr = 0x00;
        data[3].data |= 0b10100000;
    data[4] = (VgmChipWriteCmd){.cmd=0x61, .addr=0x00, .data=0x00, .data2=0x0C}; //Wait
    data[5] = (VgmChipWriteCmd){.cmd=0x50, .addr=0x00, .data=0b10110010, .data2=0}; //Attenuate
    data[6] = (VgmChipWriteCmd){.cmd=0x61, .addr=0x00, .data=0x00, .data2=0x0C}; //Wait
    data[7] = (VgmChipWriteCmd){.cmd=0x50, .addr=0x00, .data=0b10110100, .data2=0}; //Attenuate
    data[8] = (VgmChipWriteCmd){.cmd=0x61, .addr=0x00, .data=0x00, .data2=0x0C}; //Wait
    data[9] = (VgmChipWriteCmd){.cmd=0x50, .addr=0x00, .data=0b10110110, .data2=0}; //Attenuate
    data[10] = (VgmChipWriteCmd){.cmd=0x61, .addr=0x00, .data=0x00, .data2=0x18}; //Wait
    data[11] = (VgmChipWriteCmd){.cmd=0x50, .addr=0x00, .data=0b10111000, .data2=0}; //Attenuate
    data[12] = (VgmChipWriteCmd){.cmd=0x61, .addr=0x00, .data=0x00, .data2=0x18}; //Wait
    data[13] = (VgmChipWriteCmd){.cmd=0x50, .addr=0x00, .data=0b10111010, .data2=0}; //Attenuate
    data[14] = (VgmChipWriteCmd){.cmd=0x61, .addr=0x00, .data=0x00, .data2=0x18}; //Wait
    data[15] = (VgmChipWriteCmd){.cmd=0x50, .addr=0x00, .data=0b10111100, .data2=0}; //Attenuate
    data[16] = (VgmChipWriteCmd){.cmd=0x61, .addr=0x00, .data=0x00, .data2=0x18}; //Wait
    data[17] = (VgmChipWriteCmd){.cmd=0x50, .addr=0x00, .data=0b10111111, .data2=0}; //Turn off
    VGM_Source_UpdateUsage(source);
    prog->noteonsource = source;
    //Create note-off VGM file
    prog->noteoffsource = NULL;
    SyEng_RecalcProgramUsage(prog);
    ////////////////////////////////////////////////////////////////////////////
    ///////////////////////////// PSG PULSE WAVE ///////////////////////////////
    ////////////////////////////////////////////////////////////////////////////
    prog = vgmh2_malloc(sizeof(synprogram_t));
    channels[4].program = prog;
    sprintf(prog->name, "Noise Test");
    prog->rootnote = 42;
    prog->usage.all = 0;
    //Create init VGM file
    source = VGM_SourceRAM_Create();
    vsr = (VgmSourceRAM*)source->data;
    vsr->numcmds = 3;
    data = vgmh2_malloc(3*sizeof(VgmChipWriteCmd));
    vsr->cmds = data;
    //
    i=0;
    //SQ2
    data[i++] = (VgmChipWriteCmd){.cmd=0x50, .addr=0x00, .data=0b11100111 }; //Pulse wave, SQ3
    data[i++] = (VgmChipWriteCmd){.cmd=0x50, .addr=0x00, .data=0b11111111 }; //Attenuate noise
    data[i++] = (VgmChipWriteCmd){.cmd=0x50, .addr=0x00, .data=0b11011111 }; //Attenuate SQ3
    //
    VGM_Source_UpdateUsage(source);
    prog->initsource = source;
    //Create note-on VGM file
    source = VGM_SourceRAM_Create();
    vsr = (VgmSourceRAM*)source->data;
    vsr->numcmds = 2;
    data = vgmh2_malloc(2*sizeof(VgmChipWriteCmd));
    vsr->cmds = data;
    //data[0] = VGM_getPSGFrequency(60, 0, 3579545); //Middle C
        data[0].cmd   = 0x50;
        data[0].addr  = 0x00;
        data[0].data  = 0b11000000;
        data[0].data2 = 0b00001000;
    data[1] = (VgmChipWriteCmd){.cmd=0x50, .addr=0x00, .data=0b11110000, .data2=0}; //Turn on noise
    VGM_Source_UpdateUsage(source);
    prog->noteonsource = source;
    //Create note-off VGM file
    source = VGM_SourceRAM_Create();
    vsr = (VgmSourceRAM*)source->data;
    vsr->numcmds = 1;
    data = vgmh2_malloc(1*sizeof(VgmChipWriteCmd));
    vsr->cmds = data;
    data[0] = (VgmChipWriteCmd){.cmd=0x50, .addr=0x00, .data=0b11111111, .data2=0}; //Turn off noise
    VGM_Source_UpdateUsage(source);
    prog->noteoffsource = source;
    SyEng_RecalcProgramUsage(prog);
    ////////////////////////////////////////////////////////////////////////////
    /////////////////////////////// VGM PLAYBACK ///////////////////////////////
    ////////////////////////////////////////////////////////////////////////////
    prog = vgmh2_malloc(sizeof(synprogram_t));
    channels[3].program = prog;
    sprintf(prog->name, "VGM Playback");
    prog->rootnote = 76;
    prog->usage.all = 0;
    //Create init VGM file
    prog->initsource = NULL;
    //Create note-on VGM file
    prog->noteonsource = NULL;
    source = VGM_SourceStream_Create();
    VgmFileMetadata md;
    s32 res = VGM_ScanFile("/GENESIS/SOR1/BEATNIK.VGM", &md);
    if(res >= 0){
        res = VGM_SourceStream_Start(source, &md);
        if(res >= 0){
            /*
            source->usage = (VgmUsageBits){.fm1=1, .fm2=1, .fm3=1, .fm4=1, .fm5=1, .fm6=1,
                                         .fm1_lfo=0, .fm2_lfo=0, .fm3_lfo=0, .fm4_lfo=0, .fm5_lfo=0, .fm6_lfo=0,
                                         .dac=1, .fm3_special=1, .opn2_globals=0, .lfofixed=0, .lfofixedspeed=0,
                                         .sq1=1, .sq2=1, .sq3=1, .noise=1, .noisefreqsq3=1};
            */
            prog->noteonsource = source;
        }
    }
    //Create note-off VGM file
    source = VGM_SourceRAM_Create();
    vsr = (VgmSourceRAM*)source->data;
    vsr->numcmds = 10;
    data = vgmh2_malloc(10*sizeof(VgmChipWriteCmd));
    vsr->cmds = data;
    data[0] = (VgmChipWriteCmd){.cmd=0x52, .addr=0x28, .data=0x00, .data2=0}; //Key off
    data[1] = (VgmChipWriteCmd){.cmd=0x52, .addr=0x28, .data=0x01, .data2=0}; //Key off
    data[2] = (VgmChipWriteCmd){.cmd=0x52, .addr=0x28, .data=0x02, .data2=0}; //Key off
    data[3] = (VgmChipWriteCmd){.cmd=0x52, .addr=0x28, .data=0x04, .data2=0}; //Key off
    data[4] = (VgmChipWriteCmd){.cmd=0x52, .addr=0x28, .data=0x05, .data2=0}; //Key off
    data[5] = (VgmChipWriteCmd){.cmd=0x52, .addr=0x28, .data=0x06, .data2=0}; //Key off
    data[6] = (VgmChipWriteCmd){.cmd=0x50, .addr=0x00, .data=0b10011111, .data2=0}; //Turn off
    data[7] = (VgmChipWriteCmd){.cmd=0x50, .addr=0x00, .data=0b10111111, .data2=0}; //Turn off
    data[8] = (VgmChipWriteCmd){.cmd=0x50, .addr=0x00, .data=0b11011111, .data2=0}; //Turn off
    data[9] = (VgmChipWriteCmd){.cmd=0x50, .addr=0x00, .data=0b11111111, .data2=0}; //Turn off
    VGM_Source_UpdateUsage(source);
    prog->noteoffsource = source;
    SyEng_RecalcProgramUsage(prog);
}

