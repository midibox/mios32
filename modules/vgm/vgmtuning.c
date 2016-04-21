/*
 * VGM Data and Playback Driver: Tuning
 *
 * ==========================================================================
 *
 *  Copyright (C) 2016 Sauraen (sauraen@gmail.com)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#include "vgmtuning.h"

static const u32 midinotefreq[128] = {
    33488, 35479, 37589, 39824, 42192, 44701, 47359, 50175, 53159, 56320, 59668, 63217, 66976, 70958, 75178, 79648, 84384, 89402, 94718, 100350, 106318, 112640, 119337, 126434, 133952, 141917, 150356, 159297, 168769, 178804, 189437, 200701, 212636, 225280, 238675, 252868, 267904, 283835, 300712, 318594, 337538, 357609, 378874, 401403, 425272, 450560, 477351, 505736, 535809, 567670, 601425, 637188, 675077, 715219, 757748, 802806, 850544, 901120, 954703, 1011473, 1071618, 1135340, 1202850, 1274376, 1350154, 1430438, 1515497, 1605613, 1701088, 1802240, 1909406, 2022946, 2143236, 2270680, 2405701, 2548752, 2700308, 2860877, 3030994, 3211226, 3402176, 3604480, 3818813, 4045892, 4286473, 4541360, 4811403, 5097504, 5400617, 5721755, 6061988, 6422453, 6804352, 7208960, 7637627, 8091784, 8572946, 9082720, 9622807, 10195009, 10801235, 11443510, 12123977, 12844906, 13608704, 14417920, 15275254, 16183568, 17145893, 18165440, 19245614, 20390018, 21602471, 22887021, 24247954, 25689812, 27217408, 28835840, 30550508, 32367136, 34291786, 36330881, 38491228, 40780036, 43204943, 45774042, 48495908, 51379625
};

void VGM_fixOPN2Frequency(VgmChipWriteCmd* writecmd, u32 opn2mult){
    u8 block; u32 freq;
    block = (writecmd->data >> 3) & 0x07;
    freq = ((u32)(writecmd->data & 0x07) << 8) | writecmd->data2; //Up to 11 bits set
    freq <<= block; //Up to 18 bits set
    freq *= opn2mult; //If unity (0x1000), up to 30 bits set (29 downto 0)
    //Check for overflow
    if(freq & 0xC0000000){
        freq = 0x3FFFFFFF;
    }
    //To floating point: find most-significant 1
    for(block=8; block>1; --block){
        if(freq & 0x20000000) break;
        freq <<= 1;
    }
    --block;
    freq >>= 19; //Previously up to 30 bits set, now up to 11 bits set
    writecmd->data = (freq >> 8) | (block << 3);
    writecmd->data2 = (freq & 0xFF);
}

void VGM_fixPSGFrequency(VgmChipWriteCmd* writecmd, u32 psgmult, u8 psgfreq0to1){
    u32 freq = (writecmd->data & 0x0F) | ((writecmd->data2 & 0x3F) << 4);
    //TODO psgmult
    if(freq == 0 && psgfreq0to1){
        freq = 1;
    }
    writecmd->data = (writecmd->data & 0xF0) | (freq & 0x0F);
    writecmd->data2 = (freq >> 4) & 0x3F;
}

u32 VGM_interpFrequency(u8 midinote, s8 cents){
    //Convert negative tuning to positive tuning of lower note
    if(cents < 0){
        if(midinote == 0){
            return 0;
        }
        --midinote;
        cents += 100;
    }
    //Linearly interpolate frequency
    u32 flow = midinotefreq[midinote];
    u32 fhi = midinotefreq[midinote+1];
    return flow + ((fhi-flow) * cents / 100);
}

VgmChipWriteCmd VGM_getOPN2Frequency(u8 midinote, s8 cents, u32 opn2clock){
    VgmChipWriteCmd ret;
    //Check bounds
    if(midinote >= 117){
        ret.data = 0x3F;
        ret.data2 = 0xFF;
        return ret;
    }
    //Interpolate frequency
    u32 freq = VGM_interpFrequency(midinote, cents);
    if(freq == 0){
        ret.data = ret.data2 = 0;
        return ret;
    }
    //Convert to OPN2 frequency
    freq = (freq * 144) / (opn2clock >> 9);
    if(freq >= (1 << 18)){
        ret.data = 0x3F;
        ret.data2 = 0xFF;
        return ret;
    }
    //To floating point: find most-significant 1
    u8 block;
    for(block=8; block>1; --block){
        if(freq & (1<<17)) break;
        freq <<= 1;
    }
    --block;
    freq >>= 7;
    ret.data = (freq >> 8) | (block << 3);
    ret.data2 = (freq & 0xFF);
    return ret;
}
VgmChipWriteCmd VGM_getPSGFrequency(u8 midinote, s8 cents, u32 psgclock){
    VgmChipWriteCmd ret;
    u32 freq = VGM_interpFrequency(midinote, cents);
    if(freq < 400000){ //Going to be under range, or error
        ret.data = 0x0F;
        ret.data2 = 0x3F;
        return ret;
    }
    //Convert to PSG frequency
    freq = psgclock / (freq >> 7);
    if(freq >= 1024) freq = 1023;
    ret.data = (freq & 0x0000000F);
    ret.data2 = (freq >> 4) & 0x0000003F;
    return ret;
}
