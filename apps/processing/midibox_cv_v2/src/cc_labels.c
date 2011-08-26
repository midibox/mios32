// $Id: cc_labels.c 1244 2011-07-09 19:03:08Z tk $
/*
 * CC Label array
 *
 * ==========================================================================
 *
 *  Copyright (C) 2008 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

/////////////////////////////////////////////////////////////////////////////
// Include files
/////////////////////////////////////////////////////////////////////////////

#include <mios32.h>

#include "cc_labels.h"


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

static const char cc_labels[128][9] = {
  // 0x00..0x0f
  "Bank MSB",
  "ModWheel",
  "Breath  ",
  "Ctrl.  3",
  "FootCtrl",
  "PortTime",
  "Data MSB",
  "Volume  ",
  "Balance ",
  "Ctrl.  9",
  "Panorama",
  "Expr.   ",
  "Fx#1 MSB",
  "Fx#2 MSB",
  "Ctrl. 14",
  "Ctrl. 15",
  
  // 0x10..0x1f
  "Ctrl. 16",
  "Ctrl. 17",
  "Ctrl. 18",
  "Ctrl. 19",
  "Ctrl. 20",
  "Ctrl. 21",
  "Ctrl. 22",
  "Ctrl. 23",
  "Ctrl. 24",
  "Ctrl. 25",
  "Ctrl. 26",
  "Ctrl. 27",
  "Ctrl. 28",
  "Ctrl. 29",
  "Ctrl. 30",
  "Ctrl. 31",
  
  // 0x20..0x2f
  "Bank LSB",
  "Mod  LSB",
  "BreatLSB",
  "Ctrl. 35",
  "Foot.LSB",
  "PortaLSB",
  "Data LSB",
  "Vol. LSB",
  "Bal. LSB",
  "Ctrl. 41",
  "Pan. LSB",
  "Expr.LSB",
  "Fx#1 LSB",
  "Fx#2 LSB",
  "Ctrl. 46",
  "Ctrl. 47",
  
  // 0x30..0x3f
  "Ctrl. 48",
  "Ctrl. 49",
  "Ctrl. 50",
  "Ctrl. 51",
  "Ctrl. 52",
  "Ctrl. 53",
  "Ctrl. 54",
  "Ctrl. 55",
  "Ctrl. 56",
  "Ctrl. 57",
  "Ctrl. 58",
  "Ctrl. 59",
  "Ctrl. 60",
  "Ctrl. 61",
  "Ctrl. 62",
  "Ctrl. 63",
  
  // 0x40..0x4f
  "Sustain ",
  "Port. ON",
  "Sustenu.",
  "SoftPed.",
  "LegatoSw",
  "Hold2   ",
  "Ctrl. 70",
  "Harmonic",
  "Release ",
  "Attack  ",
  "CutOff  ",
  "Ctrl. 75",
  "Ctrl. 76",
  "Ctrl. 77",
  "Ctrl. 78",
  "Ctrl. 79",
  
  // 0x50..0x5f
  "Ctrl. 80",
  "Ctrl. 81",
  "Ctrl. 82",
  "Ctrl. 83",
  "Ctrl. 84",
  "Ctrl. 85",
  "Ctrl. 86",
  "Ctrl. 87",
  "Ctrl. 88",
  "Ctrl. 89",
  "Ctrl. 90",
  "Reverb  ",
  "Tremolo ",
  "Chorus  ",
  "Celeste ",
  "Phaser  ",
  
  // 0x60..0x6f
  "Data Inc",
  "Data Dec",
  "NRPN LSB",
  "NRPN MSB",
  "RPN LSB ",
  "RPN MSB ",
  "Ctrl.102",
  "Ctrl.103",
  "Ctrl.104",
  "Ctrl.105",
  "Ctrl.106",
  "Ctrl.107",
  "Ctrl.108",
  "Ctrl.109",
  "Ctrl.110",
  "Ctrl.111",
  
  // 0x70..0x7f
  "Ctrl.112",
  "Ctrl.113",
  "Ctrl.114",
  "Ctrl.115",
  "Ctrl.116",
  "Ctrl.117",
  "Ctrl.118",
  "Ctrl.119",
  "SoundOff",
  "ResetAll",
  "Local   ",
  "NotesOff",
  "Omni Off",
  "Omni On ",
  "Mono On ",
  "Poly On ",
};


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 CC_LABELS_Init(u32 mode)
{
  // here we could also generate the labels in RAM...

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Returns CC label
/////////////////////////////////////////////////////////////////////////////
const char *CC_LABELS_Get(u8 cc)
{
  if( cc >= 128 )
    cc = 0; // just to avoid buffer overruns if invalid CC number selected

  return cc_labels[cc];
}
