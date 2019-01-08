/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
// $Id$
/*
 * MIDIbox SID Drum Instrument
 *
 * ==========================================================================
 *
 *  Copyright (C) 2010 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#include <string.h>
#include "MbSidDrum.h"


/////////////////////////////////////////////////////////////////////////////
// Local types
/////////////////////////////////////////////////////////////////////////////

typedef struct mbsid_dmodel_t {
  char name[5];
  u8 gatelength;
  u8 pulsewidth;
  u8 wt_speed;
  u8 wt_loop;
  u8 *wavetable;
} mbsid_dmodel_t;


/////////////////////////////////////////////////////////////////////////////
// Wavetables
/////////////////////////////////////////////////////////////////////////////

const u8 dmodel_BD1[] = {
  0x3c | 0x80, 0x01, // Note/Wave Step #1
  0x3a | 0x00, 0x01, // Note/Wave Step #1
  0x38 | 0x00, 0x01, // Note/Wave Step #2
  0x34 | 0x00, 0x01, // Note/Wave Step #3
  0x30 | 0x00, 0x01, // Note/Wave Step #4
  0x2c | 0x00, 0x01, // Note/Wave Step #5
  0x28 | 0x00, 0x01, // Note/Wave Step #6
  0x20 | 0x00, 0x01, // Note/Wave Step #7
  0, 0 // End of WT
};

const u8 dmodel_BD2[] = {
  0x6b | 0x80, 0x08, // Note/Wave Step #0
  0x3c | 0x00, 0x04, // Note/Wave Step #1
  0x37 | 0x00, 0x04, // Note/Wave Step #2
  0x32 | 0x00, 0x04, // Note/Wave Step #3
  0x2a | 0x00, 0x04, // Note/Wave Step #4
  0, 0 // End of WT
};

const u8 dmodel_BD3[] = {
  0x48 | 0x80, 0x01, // Note/Wave Step #0
  0x4e | 0x00, 0x08, // Note/Wave Step #1
  0x45 | 0x80, 0x01, // Note/Wave Step #2
  0x35 | 0x80, 0x04, // Note/Wave Step #3
  0x4e | 0x00, 0x08, // Note/Wave Step #4
  0x40 | 0x00, 0x08, // Note/Wave Step #5
  0x47 | 0x00, 0x08, // Note/Wave Step #6
  0x3b | 0x00, 0x08, // Note/Wave Step #7
  0, 0 // End of WT
};

const u8 dmodel_SD1[] = {
  0x6b | 0x00, 0x08, // Note/Wave Step #0
  0x3c | 0x80, 0x04, // Note/Wave Step #1
  0x6b | 0x00, 0x08, // Note/Wave Step #2
  0x3b | 0x80, 0x04, // Note/Wave Step #3
  0x6b | 0x00, 0x08, // Note/Wave Step #4
  0, 0 // End of WT
};

const u8 dmodel_SD2[] = {
  0x50 | 0x00, 0x08, // Note/Wave Step #0
  0x3c | 0x80, 0x04, // Note/Wave Step #1
  0x3a | 0x80, 0x04, // Note/Wave Step #2
  0x50 | 0x00, 0x08, // Note/Wave Step #3
  0x51 | 0x00, 0x08, // Note/Wave Step #4
  0x52 | 0x00, 0x08, // Note/Wave Step #5
  0x53 | 0x00, 0x08, // Note/Wave Step #6
  0x54 | 0x00, 0x08, // Note/Wave Step #7
  0x55 | 0x00, 0x08, // Note/Wave Step #8
  0x56 | 0x00, 0x08, // Note/Wave Step #9
  0x57 | 0x00, 0x08, // Note/Wave Step #10
  0, 0 // End of WT
};

const u8 dmodel_SD3[] = {
  0x4b | 0x00, 0x08, // Note/Wave Step #0
  0x3c | 0x80, 0x01, // Note/Wave Step #1
  0, 0 // End of WT
};

const u8 dmodel_HH1[] = {
  0x6c | 0x80, 0x08, // Note/Wave Step #0
  0, 0 // End of WT
};

const u8 dmodel_HH2[] = {
  0x6c | 0x80, 0x08, // Note/Wave Step #0
  0x33 | 0x00, 0x04, // Note/Wave Step #1
  0x6c | 0x80, 0x08, // Note/Wave Step #2
  0, 0 // End of WT
};

const u8 dmodel_TO1[] = {
  0x3c | 0x80, 0x04, // Note/Wave Step #0
  0x3b | 0x00, 0x04, // Note/Wave Step #1
  0x3a | 0x80, 0x04, // Note/Wave Step #2
  0x38 | 0x00, 0x04, // Note/Wave Step #3
  0x37 | 0x80, 0x04, // Note/Wave Step #4
  0x34 | 0x00, 0x04, // Note/Wave Step #5
  0x33 | 0x80, 0x04, // Note/Wave Step #6
  0x31 | 0x00, 0x04, // Note/Wave Step #7
  0x2f | 0x80, 0x04, // Note/Wave Step #8
  0x2d | 0x00, 0x04, // Note/Wave Step #9
  0x2a | 0x80, 0x04, // Note/Wave Step #10
  0x26 | 0x00, 0x04, // Note/Wave Step #11
  0x22 | 0x80, 0x04, // Note/Wave Step #12
  0x1c | 0x00, 0x04, // Note/Wave Step #13
  0x13 | 0x80, 0x04, // Note/Wave Step #14
  0, 0 // End of WT
};

const u8 dmodel_TO2[] = {
  0x3c | 0x80, 0x04, // Note/Wave Step #0
  0x3b | 0x00, 0x01, // Note/Wave Step #1
  0x3a | 0x80, 0x04, // Note/Wave Step #2
  0x38 | 0x00, 0x01, // Note/Wave Step #3
  0x37 | 0x80, 0x04, // Note/Wave Step #4
  0x34 | 0x00, 0x01, // Note/Wave Step #5
  0x33 | 0x80, 0x04, // Note/Wave Step #6
  0x31 | 0x00, 0x01, // Note/Wave Step #7
  0x2f | 0x80, 0x04, // Note/Wave Step #8
  0x2d | 0x00, 0x01, // Note/Wave Step #9
  0x2a | 0x80, 0x04, // Note/Wave Step #10
  0x26 | 0x00, 0x01, // Note/Wave Step #11
  0x22 | 0x80, 0x04, // Note/Wave Step #12
  0x1c | 0x00, 0x01, // Note/Wave Step #13
  0x13 | 0x80, 0x04, // Note/Wave Step #14
  0, 0 // End of WT
};

const u8 dmodel_CLP[] = {
  0x3c | 0x00, 0x08, // Note/Wave Step #0
  0x70 | 0x80, 0x08, // Note/Wave Step #1
  0x30 | 0x00, 0x08, // Note/Wave Step #2
  0x3c | 0x80, 0x08, // Note/Wave Step #3
  0, 0 // End of WT
};

const u8 dmodel_FX1[] = {
  0x3c | 0x80, 0x01, // Note/Wave Step #0
  0, 0 // End of WT
};

const u8 dmodel_FX2[] = {
  0x3c | 0x80, 0x01, // Note/Wave Step #0
  0x30 | 0x00, 0x01, // Note/Wave Step #1
  0x3c | 0x80, 0x01, // Note/Wave Step #2
  0x48 | 0x00, 0x01, // Note/Wave Step #3
  0, 0 // End of WT
};

const u8 dmodel_FX3[] = {
  0x3c | 0x80, 0x41, // Note/Wave Step #0
  0, 0 // End of WT
};

const u8 dmodel_FX4[] = {
  0x3c | 0x80, 0x41, // Note/Wave Step #0
  0x3b | 0x00, 0x41, // Note/Wave Step #1
  0x3a | 0x80, 0x41, // Note/Wave Step #2
  0x38 | 0x00, 0x41, // Note/Wave Step #3
  0x37 | 0x80, 0x41, // Note/Wave Step #4
  0x34 | 0x00, 0x41, // Note/Wave Step #5
  0x33 | 0x80, 0x41, // Note/Wave Step #6
  0x31 | 0x00, 0x41, // Note/Wave Step #7
  0x2f | 0x80, 0x41, // Note/Wave Step #8
  0x2d | 0x00, 0x41, // Note/Wave Step #9
  0x2a | 0x80, 0x41, // Note/Wave Step #10
  0x26 | 0x00, 0x41, // Note/Wave Step #11
  0x22 | 0x80, 0x41, // Note/Wave Step #12
  0x1c | 0x00, 0x41, // Note/Wave Step #13
  0x13 | 0x80, 0x41, // Note/Wave Step #14
  0, 0 // End of WT
};

const u8 dmodel_FX5[] = {
  0x54 | 0x80, 0x04, // Note/Wave Step #0
  0x3b | 0x00, 0x01, // Note/Wave Step #1
  0x60 | 0x80, 0x04, // Note/Wave Step #2
  0x38 | 0x00, 0x01, // Note/Wave Step #3
  0x54 | 0x80, 0x04, // Note/Wave Step #4
  0x34 | 0x00, 0x01, // Note/Wave Step #5
  0x60 | 0x80, 0x04, // Note/Wave Step #6
  0x31 | 0x00, 0x01, // Note/Wave Step #7
  0x54 | 0x80, 0x04, // Note/Wave Step #8
  0x2d | 0x00, 0x01, // Note/Wave Step #9
  0x60 | 0x80, 0x04, // Note/Wave Step #10
  0x26 | 0x00, 0x01, // Note/Wave Step #11
  0x54 | 0x80, 0x04, // Note/Wave Step #12
  0x1c | 0x00, 0x01, // Note/Wave Step #13
  0x60 | 0x80, 0x04, // Note/Wave Step #14
  0, 0 // End of WT
};

const u8 dmodel_FX6[] = {
  0x18 | 0x80, 0x01, // Note/Wave Step #0
  0x24 | 0x00, 0x01, // Note/Wave Step #1
  0x30 | 0x80, 0x01, // Note/Wave Step #2
  0x3c | 0x00, 0x01, // Note/Wave Step #3
  0x48 | 0x80, 0x01, // Note/Wave Step #4
  0x54 | 0x00, 0x01, // Note/Wave Step #5
  0, 0 // End of WT
};

const u8 dmodel_FX7[] = {
  0x18 | 0x80, 0x02, // Note/Wave Step #0
  0x3c | 0x00, 0x02, // Note/Wave Step #1
  0x24 | 0x00, 0x02, // Note/Wave Step #2
  0x3c | 0x00, 0x02, // Note/Wave Step #3
  0x30 | 0x80, 0x02, // Note/Wave Step #4
  0, 0 // End of WT
};

const u8 dmodel_FX8[] = {
  0x70 | 0x80, 0x08, // Note/Wave Step #0
  0x68 | 0x00, 0x08, // Note/Wave Step #1
  0x60 | 0x00, 0x04, // Note/Wave Step #2
  0x58 | 0x80, 0x04, // Note/Wave Step #3
  0x50 | 0x00, 0x08, // Note/Wave Step #4
  0x48 | 0x00, 0x08, // Note/Wave Step #5
  0x40 | 0x80, 0x04, // Note/Wave Step #6
  0x38 | 0x00, 0x08, // Note/Wave Step #7
  0x30 | 0x00, 0x04, // Note/Wave Step #8
  0, 0 // End of WT
};

const u8 dmodel_FX9[] = {
  0x18 | 0x80, 0x04, // Note/Wave Step #0
  0x54 | 0x00, 0x01, // Note/Wave Step #1
  0x24 | 0x00, 0x04, // Note/Wave Step #2
  0x48 | 0x80, 0x01, // Note/Wave Step #3
  0x30 | 0x00, 0x04, // Note/Wave Step #4
  0x3c | 0x00, 0x01, // Note/Wave Step #5
  0x3c | 0x80, 0x04, // Note/Wave Step #6
  0x30 | 0x00, 0x01, // Note/Wave Step #7
  0x48 | 0x00, 0x04, // Note/Wave Step #8
  0x18 | 0x80, 0x01, // Note/Wave Step #9
  0x54 | 0x00, 0x04, // Note/Wave Step #10
  0x24 | 0x00, 0x01, // Note/Wave Step #11
  0x48 | 0x80, 0x04, // Note/Wave Step #12
  0x30 | 0x00, 0x01, // Note/Wave Step #13
  0x3c | 0x00, 0x04, // Note/Wave Step #14
  0x3c | 0x80, 0x01, // Note/Wave Step #15
  0x30 | 0x00, 0x04, // Note/Wave Step #16
  0x48 | 0x00, 0x01, // Note/Wave Step #17
  0, 0 // End of WT
};


/////////////////////////////////////////////////////////////////////////////
// Drum Models
/////////////////////////////////////////////////////////////////////////////

#define DMODEL_NUM 20
const mbsid_dmodel_t dmodel[DMODEL_NUM] = {
  // name   GLng   PW  Speed  Loop  Wavetable
  { "BD1 ", 0x20, 0x80, 0x0a, 0xff, (u8 *)&dmodel_BD1 },
  { "BD2 ", 0x20, 0x80, 0x0a, 0xff, (u8 *)&dmodel_BD2 },
  { "BD3 ", 0x20, 0x80, 0x0a, 0xff, (u8 *)&dmodel_BD3 },
  { "SD1 ", 0x20, 0x80, 0x0a, 0xff, (u8 *)&dmodel_SD1 },
  { "SD2 ", 0x20, 0x80, 0x0a, 0xff, (u8 *)&dmodel_SD2 },
  { "SD3 ", 0x20, 0x80, 0x0a, 0x00, (u8 *)&dmodel_SD3 },
  { "HH1 ", 0x20, 0x80, 0x0a, 0xff, (u8 *)&dmodel_HH1 },
  { "HH2 ", 0x20, 0x80, 0x0a, 0xff, (u8 *)&dmodel_HH2 },
  { "TO1 ", 0x20, 0x80, 0x0a, 0xff, (u8 *)&dmodel_TO1 },
  { "TO2 ", 0x20, 0x80, 0x0a, 0x0c, (u8 *)&dmodel_TO2 },
  { "CLP ", 0x20, 0x80, 0x0a, 0x01, (u8 *)&dmodel_CLP },
  { "FX1 ", 0x20, 0x80, 0x0a, 0xff, (u8 *)&dmodel_FX1 },
  { "FX2 ", 0x20, 0x80, 0x0a, 0x00, (u8 *)&dmodel_FX2 },
  { "FX3 ", 0x20, 0x80, 0x0a, 0xff, (u8 *)&dmodel_FX3 },
  { "FX4 ", 0x20, 0x80, 0x0a, 0xff, (u8 *)&dmodel_FX4 },
  { "FX5 ", 0x20, 0x80, 0x0a, 0x0a, (u8 *)&dmodel_FX5 },
  { "FX6 ", 0x20, 0x80, 0x0a, 0x00, (u8 *)&dmodel_FX6 },
  { "FX7 ", 0x20, 0x80, 0x0a, 0x00, (u8 *)&dmodel_FX7 },
  { "FX8 ", 0x20, 0x80, 0x0a, 0x00, (u8 *)&dmodel_FX8 },
  { "FX9 ", 0x20, 0x80, 0x0a, 0x00, (u8 *)&dmodel_FX9 },
};


/////////////////////////////////////////////////////////////////////////////
// Constructor
/////////////////////////////////////////////////////////////////////////////
MbSidDrum::MbSidDrum()
{
}


/////////////////////////////////////////////////////////////////////////////
// Destructor
/////////////////////////////////////////////////////////////////////////////
MbSidDrum::~MbSidDrum()
{
}


/////////////////////////////////////////////////////////////////////////////
// Voice init function
/////////////////////////////////////////////////////////////////////////////
void MbSidDrum::init(void)
{
    drumVoiceAssignment = 0;
    drumModel = 0;
    drumAttackDecay.ALL = 0;
    drumSustainRelease.ALL = 0;
    drumTuneModifier = 0;
    drumGatelengthModifier = 0;
    drumSpeedModifier = 0;
    drumParameter = 0;
    drumVelocityAssignment = 0;
}


/////////////////////////////////////////////////////////////////////////////
// Returns note played by drum model at given step
/////////////////////////////////////////////////////////////////////////////
u8 MbSidDrum::getNote(u8 pos)
{
    u8 *wavetable = dmodel[drumModel].wavetable;

    int note = wavetable[2*pos + 0];

    // if bit #7 of note entry is set: add parameter and saturate
    if( note & (1 << 7) ) {
        note = (note & 0x7f) + (drumParameter / 2);
        if( note > 127 ) note = 127; else if( note < 0 ) note = 0;
    }

    return note;
}


/////////////////////////////////////////////////////////////////////////////
// Returns waveform played by drum model at given step
/////////////////////////////////////////////////////////////////////////////
u8 MbSidDrum::getWaveform(u8 pos)
{
    u8 *wavetable = dmodel[drumModel].wavetable;
    return wavetable[2*pos + 1];
}

/////////////////////////////////////////////////////////////////////////////
// Returns gatelength of drum
/////////////////////////////////////////////////////////////////////////////
u8 MbSidDrum::getGatelength(void)
{
    // vary gatelength depending on parameter
    int gatelength = dmodel[drumModel].gatelength;
    if( gatelength ) {
        gatelength += drumGatelengthModifier / 4;
        if( gatelength > 127 ) gatelength = 127; else if( gatelength < 2 ) gatelength = 2;
        // gatelength should never be 0, otherwise gate clear delay feature would be deactivated
    }

    return (u8)gatelength;
}

/////////////////////////////////////////////////////////////////////////////
// Returns speed of drum
/////////////////////////////////////////////////////////////////////////////
u8 MbSidDrum::getSpeed(void)
{
    // vary speed depending on parameter
    int speed = dmodel[drumModel].wt_speed;
    speed += drumSpeedModifier / 4;
    if( speed > 127 ) speed = 127; else if( speed < 0 ) speed = 0;

    return (u8)speed;
}

/////////////////////////////////////////////////////////////////////////////
// Returns pulsewidth of drum (12bit)
/////////////////////////////////////////////////////////////////////////////
u16 MbSidDrum::getPulsewidth(void)
{
    return dmodel[drumModel].pulsewidth << 4;
}

/////////////////////////////////////////////////////////////////////////////
// Returns loop point of drum model (0xff for oneshot)
/////////////////////////////////////////////////////////////////////////////
u8 MbSidDrum::getLoop(void)
{
    return dmodel[drumModel].wt_loop;
}
