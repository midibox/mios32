// $Id$
/*
 * MBSID Parameter Handlers
 *
 * ==========================================================================
 *
 *  Copyright (C) 2009 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

/////////////////////////////////////////////////////////////////////////////
// Include files
/////////////////////////////////////////////////////////////////////////////

#include <mios32.h>

#include "sid_se.h"
#include "sid_patch.h"
#include "sid_par.h"


/////////////////////////////////////////////////////////////////////////////
// for optional debugging messages via DEBUG_MSG (defined in mios32_config.h)
// should be at least 1 for sending error messages
/////////////////////////////////////////////////////////////////////////////
#define DEBUG_VERBOSE_LEVEL 1


/////////////////////////////////////////////////////////////////////////////
// String IDs and Table
/////////////////////////////////////////////////////////////////////////////

// parameter strings
// must be kept aligned with sid_par_table_s[] below!
typedef enum {
  P_S_NOP = 0,
  P_S_VOLUME,
  P_S_PHASE,
  P_S_DETUNE,
  P_S_CUTOFF,
  P_S_RESONANCE,
  P_S_CHANNELS,
  P_S_MODE,
  P_S_KNOB,
  P_S_EXT_AOUT,
  P_S_EXT_SWITCH,
  P_S_WAVEFORM,
  P_S_TRANSPOSE,
  P_S_FINETUNE,
  P_S_PORTAMENTO,
  P_S_PULSEWIDTH,
  P_S_DELAY,
  P_S_ATTACK,
  P_S_DECAY,
  P_S_SUSTAIN,
  P_S_RELEASE,
  P_S_PITCHBENDER,
  P_S_ARP_SPEED,
  P_S_ARP_GL,
  P_S_DEPTH,
  P_S_MODINV,
  P_S_RATE,
  P_S_ATTACK1,
  P_S_ALEVEL,
  P_S_ATTACK2,
  P_S_DECAY1,
  P_S_DLEVEL,
  P_S_DECAY2,
  P_S_RELEASE1,
  P_S_RLEVEL,
  P_S_RELEASE2,
  P_S_CURVE,
  P_S_ATT_CURVE,
  P_S_DEC_CURVE,
  P_S_REL_CURVE,
  P_S_SPEED,
  P_S_START,
  P_S_END,
  P_S_LOOP,
  P_S_POSITION,
  P_S_NOTE,
  P_S_DEPTH_P,
  P_S_DEPTH_PW,
  P_S_DEPTH_F,
  P_S_ENVMOD,
  P_S_ACCENT,
  P_S_DECAY_A,
  P_S_MODEL,
  P_S_GATELENGTH,
  P_S_PAR3
} sid_par_table_s_t;


#define SID_PAR_TABLE_S_WIDTH 8 // chars per entry
static const char sid_par_table_s[] = {
  "--------"
  "Volume  "
  "Phase   "
  "Detune  "
  "CutOff  "
  "Resonan."
  "Channels"
  "Mode    "
  "Knob    "
  "Ext.AOUT"
  "Ext.Sw. "
  "Waveform"
  "Transp. "
  "Finetune"
  "Portam. "
  "Pulsew. "
  "Delay   "
  "Attack  "
  "Decay   "
  "Sustain "
  "Release "
  "P.Bender"
  "Arp.Spd."
  "Arp.GL. "
  "Depth   "
  "ModInv  "
  "Rate    "
  "Attack1 "
  "A.Level "
  "Attack2 "
  "Decay1  "
  "D.Level "
  "Decay2  "
  "Release1"
  "R.Level "
  "Release2"
  "Curve   "
  "A.Curve "
  "D.Curve "
  "R.Curve "
  "Speed   "
  "Start   "
  "End     "
  "Loop    "
  "Position"
  "Note    "
  "Depth P."
  "Depth PW"
  "Depth F."
  "EnvMod  "
  "Accent  "
  "Decay A."
  "D.Model "
  "GateLn. "
  "Param.3 "
};


// parameter number string definition
typedef enum {
  P_N_NOP = 0,
  P_N_OSC,
  P_N_FIL,
  P_N_FIL_L,
  P_N_FIL_R,
  P_N_KNOB,
  P_N_EXT,
  P_N_OSC123,
  P_N_MOD,
  P_N_LFO,
  P_N_ENV,
  P_N_WT,
  P_N_OSC_INS,
  P_N_LFO1_INS,
  P_N_LFO2_INS,
  P_N_ENV_INS,
  P_N_OSC_BL,
  P_N_LFO_BL,
  P_N_ENV_BL,
  P_N_DRM_CUR,
  P_N_DRM_INS
} sid_par_table_n_t;


// parameter modification definitions
// must be kept aligned with sid_par_table_m_resolution[] below!
typedef enum {
  P_M_NOP = 0,
  P_M_7,
  P_M_8,
  P_M_PM8,
  P_M_4L,
  P_M_4U,
  P_M_PAR12,
  P_M_CUSTOM_SW,
  P_M_FIL4L,
  P_M_FIL4U,
  P_M_FIL12,
  P_M_FIL12_DIRECT,
  P_M_FIL8,
  P_M_OSC123_PM7,
  P_M_OSC123_PM8,
  P_M_OSC123_7,
  P_M_OSC123_8,
  P_M_OSC123_12,
  P_M_OSC123_4L,
  P_M_OSC123_5L,
  P_M_OSC123_6L,
  P_M_OSC123_4U,
  P_M_OSC123_PB,
  P_M_MOD_PM8,
  P_M_MOD_B76,
  P_M_LFO_4U,
  P_M_LFO_PM8,
  P_M_LFO_8,
  P_M_ENV_PM8,
  P_M_ENV_8,
  P_M_WT_6,
  P_M_WT_7,
  P_M_WT_POS,
  P_M_NOTE,
  P_M_OSC_INS_PM7,
  P_M_OSC_INS_PM8,
  P_M_OSC_INS_7,
  P_M_OSC_INS_8,
  P_M_OSC_INS_12,
  P_M_OSC_INS_4L,
  P_M_OSC_INS_5L,
  P_M_OSC_INS_6L,
  P_M_OSC_INS_4U,
  P_M_OSC_INS_PB,
  P_M_OSC_BL_PM7,
  P_M_OSC_BL_PM8,
  P_M_OSC_BL_P8,
  P_M_OSC_BL_7,
  P_M_OSC_BL_8,
  P_M_OSC_BL_12,
  P_M_OSC_BL_4L,
  P_M_OSC_BL_5L,
  P_M_OSC_BL_6L,
  P_M_OSC_BL_4U,
  P_M_OSC_BL_PB,
  P_M_OSC_BL_FIL12,
  P_M_OSC_BL_FIL8,
  P_M_DRM_8,
  P_M_DRM_PM8,
  P_M_DRM_4U,
  P_M_DRM_4L,
  P_M_NOTE_INS
} sid_par_table_m_t;

static const u8 sid_par_table_m_resolution[] = {
  0, // NOP
  7, // 7
  8, // 8
  8, // PM8
  4, // 4L
  4, // 4U
  12, // PAR12
  1, // CUSTOM_SW
  4, // FIL4L
  4, // FIL4U
  12, // FIL12
  12, // FIL12_DIRECT
  8, // FIL8
  7, // OSC123_PM7
  8, // OSC123_PM8
  7, // OSC123_7
  8, // OSC123_8
  12, // OSC123_12
  4, // OSC123_4L
  5, // OSC123_5L
  6, // OSC123_6L
  4, // OSC123_4U
  8, // OSC123_PB
  8, // MOD_PM8
  2, // MOD_B76
  4, // LFO_4U
  8, // LFO_PM8
  8, // LFO_8
  8, // ENV_PM8
  8, // ENV_8
  6, // WT_6
  7, // WT_7
  7, // WT_POS
  7, // NOTE
  7, // OSC_INS_PM7
  8, // OSC_INS_PM8
  7, // OSC_INS_7
  8, // OSC_INS_8
  12, // OSC_INS_12
  4, // OSC_INS_4L
  5, // OSC_INS_5L
  6, // OSC_INS_6L
  4, // OSC_INS_4U
  8, // OSC_INS_PB
  7, // OSC_BL_PM7
  8, // OSC_BL_PM8
  7, // OSC_BL_P8
  7, // OSC_BL_7
  8, // OSC_BL_8
  12, // OSC_BL_12
  4, // OSC_BL_4L
  5, // OSC_BL_5L
  6, // OSC_BL_6L
  4, // OSC_BL_4U
  8, // OSC_BL_PB
  12, // OSC_BL_FIL12
  8, // OSC_BL_FIL8
  8, // DRM_8
  8, // DRM_PM8
  4, // DRM_4U
  4, // DRM_4L
  7, // NOTE_INS
};


/////////////////////////////////////////////////////////////////////////////
// Type definitions
/////////////////////////////////////////////////////////////////////////////

typedef struct sid_par_table_item_t {
  u8 left_string;
  u8 right_string;
  u8 mod_function;
  u8 addr_l;
} sid_par_table_item_t;



/////////////////////////////////////////////////////////////////////////////
// Parameter Mappings
/////////////////////////////////////////////////////////////////////////////

// --------------------------------------------------------------------------
// -- Lead Engine
// --------------------------------------------------------------------------
static const sid_par_table_item_t sid_par_table[2][256] = {

// SID_SE_LEAD
{
  //left string	right string	mod function	register/number
  // --[ 0x00-0x03 ]-----------------------------------------------------------------------------
  { P_S_NOP,	P_N_NOP,	P_M_NOP,	0x00 },
  { P_S_VOLUME,	P_N_NOP,	P_M_7,		0x52 }, // SID_Ix_L_VOLUME
  { P_S_PHASE,	P_N_OSC,	P_M_8,		0x53 }, // SID_Ix_L_OSC_PHASE
  { P_S_DETUNE,	P_N_OSC,	P_M_8,		0x51 }, // SID_Ix_L_OSC_DETUNE
  // --[ 0x04-0x07 ]-----------------------------------------------------------------------------
  { P_S_CUTOFF,	P_N_FIL,	P_M_FIL12,	0x54+1 }, // SID_Ix_L_S1F_BASE+SID_Ix_L_Fx_CUTOFF_L
  { P_S_RESONANCE, P_N_FIL,	P_M_FIL8,	0x54+3 }, // SID_Ix_L_S1F_BASE+SID_Ix_L_Fx_RESONANCE
  { P_S_CHANNELS, P_N_FIL,	P_M_FIL4L,	0x54+0 }, // SID_Ix_L_S1F_BASE+SID_Ix_L_Fx_CHN_MODE
  { P_S_MODE,	P_N_FIL,	P_M_FIL4U,	0x54+0 }, // SID_Ix_L_S1F_BASE+SID_Ix_L_Fx_CHN_MODE
  // --[ 0x08-0x0f ]-----------------------------------------------------------------------------
  { P_S_KNOB,	P_N_KNOB,	P_M_8,		0x18+0*5+0 }, // SID_Ix_P_K1_BASE+SID_Ix_Px_VALUE
  { P_S_KNOB,	P_N_KNOB,	P_M_8,		0x18+1*5+0 }, // SID_Ix_P_K2_BASE+SID_Ix_Px_VALUE
  { P_S_KNOB,	P_N_KNOB,	P_M_8,		0x18+2*5+0 }, // SID_Ix_P_K3_BASE+SID_Ix_Px_VALUE
  { P_S_KNOB,	P_N_KNOB,	P_M_8,		0x18+3*5+0 }, // SID_Ix_P_K4_BASE+SID_Ix_Px_VALUE
  { P_S_KNOB,	P_N_KNOB,	P_M_8,		0x18+4*5+0 }, // SID_Ix_P_K5_BASE+SID_Ix_Px_VALUE
  { P_S_KNOB,	P_N_KNOB,	P_M_8,		0x18+5*5+0 }, // SID_Ix_P_KV_BASE+SID_Ix_Px_VALUE
  { P_S_KNOB,	P_N_KNOB,	P_M_8,		0x18+6*5+0 }, // SID_Ix_P_KP_BASE+SID_Ix_Px_VALUE
  { P_S_KNOB,	P_N_KNOB,	P_M_8,		0x18+7*5+0 }, // SID_Ix_P_KA_BASE+SID_Ix_Px_VALUE
  // --[ 0x10-0x17 ]-----------------------------------------------------------------------------
  { P_S_EXT_AOUT, P_N_EXT,	P_M_PAR12,	0x40 }, // SID_Ix_EXT_PAR1_L
  { P_S_EXT_AOUT, P_N_EXT,	P_M_PAR12,	0x42 }, // SID_Ix_EXT_PAR2_L
  { P_S_EXT_AOUT, P_N_EXT,	P_M_PAR12,	0x44 }, // SID_Ix_EXT_PAR3_L
  { P_S_EXT_AOUT, P_N_EXT,	P_M_PAR12,	0x46 }, // SID_Ix_EXT_PAR4_L
  { P_S_EXT_AOUT, P_N_EXT,	P_M_PAR12,	0x48 }, // SID_Ix_EXT_PAR5_L
  { P_S_EXT_AOUT, P_N_EXT,	P_M_PAR12,	0x4a }, // SID_Ix_EXT_PAR6_L
  { P_S_EXT_AOUT, P_N_EXT,	P_M_PAR12,	0x4c }, // SID_Ix_EXT_PAR7_L
  { P_S_EXT_AOUT, P_N_EXT,	P_M_PAR12,	0x4e }, // SID_Ix_EXT_PAR8_L
  // --[ 0x18-0x1f ]-----------------------------------------------------------------------------
  { P_S_EXT_SWITCH, P_N_EXT,	P_M_CUSTOM_SW,	0x14 }, // SID_Ix_CUSTOM_SW
  { P_S_EXT_SWITCH, P_N_EXT,	P_M_CUSTOM_SW,	0x14 }, // SID_Ix_CUSTOM_SW
  { P_S_EXT_SWITCH, P_N_EXT,	P_M_CUSTOM_SW,	0x14 }, // SID_Ix_CUSTOM_SW
  { P_S_EXT_SWITCH, P_N_EXT,	P_M_CUSTOM_SW,	0x14 }, // SID_Ix_CUSTOM_SW
  { P_S_EXT_SWITCH, P_N_EXT,	P_M_CUSTOM_SW,	0x14 }, // SID_Ix_CUSTOM_SW
  { P_S_EXT_SWITCH, P_N_EXT,	P_M_CUSTOM_SW,	0x14 }, // SID_Ix_CUSTOM_SW
  { P_S_EXT_SWITCH, P_N_EXT,	P_M_CUSTOM_SW,	0x14 }, // SID_Ix_CUSTOM_SW
  { P_S_EXT_SWITCH, P_N_EXT,	P_M_CUSTOM_SW,	0x14 }, // SID_Ix_CUSTOM_SW
  // --[ 0x20-0x23 ]-----------------------------------------------------------------------------
  { P_S_WAVEFORM,   P_N_OSC123,	P_M_OSC123_7,	0x60+1 }, // SID_Ix_L_S1V1_BASE+SID_Ix_Vx_WAVEFORM
  { P_S_WAVEFORM,   P_N_OSC123,	P_M_OSC123_7,	0x60+1 }, // SID_Ix_L_S1V1_BASE+SID_Ix_Vx_WAVEFORM
  { P_S_WAVEFORM,   P_N_OSC123,	P_M_OSC123_7,	0x60+1 }, // SID_Ix_L_S1V1_BASE+SID_Ix_Vx_WAVEFORM
  { P_S_WAVEFORM,   P_N_OSC123,	P_M_OSC123_7,	0x60+1 }, // SID_Ix_L_S1V1_BASE+SID_Ix_Vx_WAVEFORM
  // --[ 0x24-0x27 ]-----------------------------------------------------------------------------
  { P_S_TRANSPOSE,  P_N_OSC123,	P_M_OSC123_PM7,	0x60+8 }, // SID_Ix_L_S1V1_BASE+SID_Ix_Vx_TRANSPOSE
  { P_S_TRANSPOSE,  P_N_OSC123,	P_M_OSC123_PM7,	0x60+8 }, // SID_Ix_L_S1V1_BASE+SID_Ix_Vx_TRANSPOSE
  { P_S_TRANSPOSE,  P_N_OSC123,	P_M_OSC123_PM7,	0x60+8 }, // SID_Ix_L_S1V1_BASE+SID_Ix_Vx_TRANSPOSE
  { P_S_TRANSPOSE,  P_N_OSC123,	P_M_OSC123_PM7,	0x60+8 }, // SID_Ix_L_S1V1_BASE+SID_Ix_Vx_TRANSPOSE
  // --[ 0x28-0x2b ]-----------------------------------------------------------------------------
  { P_S_FINETUNE,   P_N_OSC123,	P_M_OSC123_PM8,	0x60+9 }, // SID_Ix_L_S1V1_BASE+SID_Ix_Vx_FINETUNE
  { P_S_FINETUNE,   P_N_OSC123,	P_M_OSC123_PM8,	0x60+9 }, // SID_Ix_L_S1V1_BASE+SID_Ix_Vx_FINETUNE
  { P_S_FINETUNE,   P_N_OSC123,	P_M_OSC123_PM8,	0x60+9 }, // SID_Ix_L_S1V1_BASE+SID_Ix_Vx_FINETUNE
  { P_S_FINETUNE,   P_N_OSC123,	P_M_OSC123_PM8,	0x60+9 }, // SID_Ix_L_S1V1_BASE+SID_Ix_Vx_FINETUNE
  // --[ 0x2c-0x2f ]-----------------------------------------------------------------------------
  { P_S_PORTAMENTO, P_N_OSC123,	P_M_OSC123_8,	0x60+11 }, // SID_Ix_L_S1V1_BASE+SID_Ix_Vx_PORTAMENTO
  { P_S_PORTAMENTO, P_N_OSC123,	P_M_OSC123_8,	0x60+11 }, // SID_Ix_L_S1V1_BASE+SID_Ix_Vx_PORTAMENTO
  { P_S_PORTAMENTO, P_N_OSC123,	P_M_OSC123_8,	0x60+11 }, // SID_Ix_L_S1V1_BASE+SID_Ix_Vx_PORTAMENTO
  { P_S_PORTAMENTO, P_N_OSC123,	P_M_OSC123_8,	0x60+11 }, // SID_Ix_L_S1V1_BASE+SID_Ix_Vx_PORTAMENTO
  // --[ 0x30-0x33 ]-----------------------------------------------------------------------------
  { P_S_PULSEWIDTH, P_N_OSC123,	P_M_OSC123_12,	0x60+4 }, // SID_Ix_L_S1V1_BASE+SID_Ix_Vx_PULSEWIDTH_L
  { P_S_PULSEWIDTH, P_N_OSC123,	P_M_OSC123_12,	0x60+4 }, // SID_Ix_L_S1V1_BASE+SID_Ix_Vx_PULSEWIDTH_L
  { P_S_PULSEWIDTH, P_N_OSC123,	P_M_OSC123_12,	0x60+4 }, // SID_Ix_L_S1V1_BASE+SID_Ix_Vx_PULSEWIDTH_L
  { P_S_PULSEWIDTH, P_N_OSC123,	P_M_OSC123_12,	0x60+4 }, // SID_Ix_L_S1V1_BASE+SID_Ix_Vx_PULSEWIDTH_L
  // --[ 0x34-0x37 ]-----------------------------------------------------------------------------
  { P_S_DELAY,	P_N_OSC123,	P_M_OSC123_8,	0x60+7 }, // SID_Ix_L_S1V1_BASE+SID_Ix_Vx_DELAY
  { P_S_DELAY,	P_N_OSC123,	P_M_OSC123_8,	0x60+7 }, // SID_Ix_L_S1V1_BASE+SID_Ix_Vx_DELAY
  { P_S_DELAY,	P_N_OSC123,	P_M_OSC123_8,	0x60+7 }, // SID_Ix_L_S1V1_BASE+SID_Ix_Vx_DELAY
  { P_S_DELAY,	P_N_OSC123,	P_M_OSC123_8,	0x60+7 }, // SID_Ix_L_S1V1_BASE+SID_Ix_Vx_DELAY
  // --[ 0x38-0x3b ]-----------------------------------------------------------------------------
  { P_S_ATTACK,	P_N_OSC123,	P_M_OSC123_4U,	0x60+2 }, // SID_Ix_L_S1V1_BASE+SID_Ix_Vx_AD
  { P_S_ATTACK,	P_N_OSC123,	P_M_OSC123_4U,	0x60+2 }, // SID_Ix_L_S1V1_BASE+SID_Ix_Vx_AD
  { P_S_ATTACK,	P_N_OSC123,	P_M_OSC123_4U,	0x60+2 }, // SID_Ix_L_S1V1_BASE+SID_Ix_Vx_AD
  { P_S_ATTACK,	P_N_OSC123,	P_M_OSC123_4U,	0x60+2 }, // SID_Ix_L_S1V1_BASE+SID_Ix_Vx_AD
  // --[ 0x3c-0x3f ]-----------------------------------------------------------------------------
  { P_S_DECAY,	P_N_OSC123,	P_M_OSC123_4L,	0x60+2 }, // SID_Ix_L_S1V1_BASE+SID_Ix_Vx_AD
  { P_S_DECAY,	P_N_OSC123,	P_M_OSC123_4L,	0x60+2 }, // SID_Ix_L_S1V1_BASE+SID_Ix_Vx_AD
  { P_S_DECAY,	P_N_OSC123,	P_M_OSC123_4L,	0x60+2 }, // SID_Ix_L_S1V1_BASE+SID_Ix_Vx_AD
  { P_S_DECAY,	P_N_OSC123,	P_M_OSC123_4L,	0x60+2 }, // SID_Ix_L_S1V1_BASE+SID_Ix_Vx_AD
  // --[ 0x40-0x43 ]-----------------------------------------------------------------------------
  { P_S_SUSTAIN, P_N_OSC123,	P_M_OSC123_4U,	0x60+3 }, // SID_Ix_L_S1V1_BASE+SID_Ix_Vx_SR
  { P_S_SUSTAIN, P_N_OSC123,	P_M_OSC123_4U,	0x60+3 }, // SID_Ix_L_S1V1_BASE+SID_Ix_Vx_SR
  { P_S_SUSTAIN, P_N_OSC123,	P_M_OSC123_4U,	0x60+3 }, // SID_Ix_L_S1V1_BASE+SID_Ix_Vx_SR
  { P_S_SUSTAIN, P_N_OSC123,	P_M_OSC123_4U,	0x60+3 }, // SID_Ix_L_S1V1_BASE+SID_Ix_Vx_SR
  // --[ 0x44-0x47 ]-----------------------------------------------------------------------------
  { P_S_RELEASE, P_N_OSC123,	P_M_OSC123_4L,	0x60+3 }, // SID_Ix_L_S1V1_BASE+SID_Ix_Vx_SR
  { P_S_RELEASE, P_N_OSC123,	P_M_OSC123_4L,	0x60+3 }, // SID_Ix_L_S1V1_BASE+SID_Ix_Vx_SR
  { P_S_RELEASE, P_N_OSC123,	P_M_OSC123_4L,	0x60+3 }, // SID_Ix_L_S1V1_BASE+SID_Ix_Vx_SR
  { P_S_RELEASE, P_N_OSC123,	P_M_OSC123_4L,	0x60+3 }, // SID_Ix_L_S1V1_BASE+SID_Ix_Vx_SR
  // --[ 0x48-0x4b ]-----------------------------------------------------------------------------
  { P_S_ARP_SPEED, P_N_OSC123,	P_M_OSC123_6L,	0x60+13 }, // SID_Ix_L_S1V1_BASE+SID_Ix_Vx_ARP_SPEED_DIV
  { P_S_ARP_SPEED, P_N_OSC123,	P_M_OSC123_6L,	0x60+13 }, // SID_Ix_L_S1V1_BASE+SID_Ix_Vx_ARP_SPEED_DIV
  { P_S_ARP_SPEED, P_N_OSC123,	P_M_OSC123_6L,	0x60+13 }, // SID_Ix_L_S1V1_BASE+SID_Ix_Vx_ARP_SPEED_DIV
  { P_S_ARP_SPEED, P_N_OSC123,	P_M_OSC123_6L,	0x60+13 }, // SID_Ix_L_S1V1_BASE+SID_Ix_Vx_ARP_SPEED_DIV
  // --[ 0x4c-0x4f ]-----------------------------------------------------------------------------
  { P_S_ARP_GL,	P_N_OSC123,	P_M_OSC123_5L,	0x60+14 }, // SID_Ix_L_S1V1_BASE+SID_Ix_Vx_ARP_GL_RNG
  { P_S_ARP_GL,	P_N_OSC123,	P_M_OSC123_5L,	0x60+14 }, // SID_Ix_L_S1V1_BASE+SID_Ix_Vx_ARP_GL_RNG
  { P_S_ARP_GL,	P_N_OSC123,	P_M_OSC123_5L,	0x60+14 }, // SID_Ix_L_S1V1_BASE+SID_Ix_Vx_ARP_GL_RNG
  { P_S_ARP_GL,	P_N_OSC123,	P_M_OSC123_5L,	0x60+14 }, // SID_Ix_L_S1V1_BASE+SID_Ix_Vx_ARP_GL_RNG
  // --[ 0x50-0x53 ]-----------------------------------------------------------------------------
  { P_S_PITCHBENDER,P_N_OSC123,	P_M_OSC123_PB,	0 }, // SIDL_V1_BASE
  { P_S_PITCHBENDER,P_N_OSC123,	P_M_OSC123_PB,	0 }, // SIDL_V1_BASE
  { P_S_PITCHBENDER,P_N_OSC123,	P_M_OSC123_PB,	1 }, // SIDL_V2_BASE
  { P_S_PITCHBENDER,P_N_OSC123,	P_M_OSC123_PB,	2 }, // SIDL_V3_BASE
  // --[ 0x54-0x5f ]-----------------------------------------------------------------------------
  { P_S_NOP,	P_N_NOP,	P_M_NOP,	0x00 },
  { P_S_NOP,	P_N_NOP,	P_M_NOP,	0x00 },
  { P_S_NOP,	P_N_NOP,	P_M_NOP,	0x00 },
  { P_S_NOP,	P_N_NOP,	P_M_NOP,	0x00 },
  { P_S_NOP,	P_N_NOP,	P_M_NOP,	0x00 },
  { P_S_NOP,	P_N_NOP,	P_M_NOP,	0x00 },
  { P_S_NOP,	P_N_NOP,	P_M_NOP,	0x00 },
  { P_S_NOP,	P_N_NOP,	P_M_NOP,	0x00 },
  { P_S_NOP,	P_N_NOP,	P_M_NOP,	0x00 },
  { P_S_NOP,	P_N_NOP,	P_M_NOP,	0x00 },
  { P_S_NOP,	P_N_NOP,	P_M_NOP,	0x00 },
  { P_S_NOP,	P_N_NOP,	P_M_NOP,	0x00 },
  // --[ 0x60-0x67 ]-----------------------------------------------------------------------------
  { P_S_DEPTH,	P_N_MOD,	P_M_MOD_PM8,	0x00+0*8+3 }, // SID_Ix_L_MOD1_BASE+SID_Ix_MODx_DEPTH
  { P_S_DEPTH,	P_N_MOD,	P_M_MOD_PM8,	0x00+1*8+3 }, // SID_Ix_L_MOD2_BASE+SID_Ix_MODx_DEPTH
  { P_S_DEPTH,	P_N_MOD,	P_M_MOD_PM8,	0x00+2*8+3 }, // SID_Ix_L_MOD3_BASE+SID_Ix_MODx_DEPTH
  { P_S_DEPTH,	P_N_MOD,	P_M_MOD_PM8,	0x00+3*8+3 }, // SID_Ix_L_MOD4_BASE+SID_Ix_MODx_DEPTH
  { P_S_DEPTH,	P_N_MOD,	P_M_MOD_PM8,	0x00+4*8+3 }, // SID_Ix_L_MOD5_BASE+SID_Ix_MODx_DEPTH
  { P_S_DEPTH,	P_N_MOD,	P_M_MOD_PM8,	0x00+5*8+3 }, // SID_Ix_L_MOD6_BASE+SID_Ix_MODx_DEPTH
  { P_S_DEPTH,	P_N_MOD,	P_M_MOD_PM8,	0x00+6*8+3 }, // SID_Ix_L_MOD7_BASE+SID_Ix_MODx_DEPTH
  { P_S_DEPTH,	P_N_MOD,	P_M_MOD_PM8,	0x00+7*8+3 }, // SID_Ix_L_MOD8_BASE+SID_Ix_MODx_DEPTH
  // --[ 0x68-0x6f ]-----------------------------------------------------------------------------
  { P_S_MODINV,	P_N_MOD,	P_M_MOD_B76,	0x00+0*8+2 }, // SID_Ix_L_MOD1_BASE+SID_Ix_MODx_OP
  { P_S_MODINV,	P_N_MOD,	P_M_MOD_B76,	0x00+1*8+2 }, // SID_Ix_L_MOD2_BASE+SID_Ix_MODx_OP
  { P_S_MODINV,	P_N_MOD,	P_M_MOD_B76,	0x00+2*8+2 }, // SID_Ix_L_MOD3_BASE+SID_Ix_MODx_OP
  { P_S_MODINV,	P_N_MOD,	P_M_MOD_B76,	0x00+3*8+2 }, // SID_Ix_L_MOD4_BASE+SID_Ix_MODx_OP
  { P_S_MODINV,	P_N_MOD,	P_M_MOD_B76,	0x00+4*8+2 }, // SID_Ix_L_MOD5_BASE+SID_Ix_MODx_OP
  { P_S_MODINV,	P_N_MOD,	P_M_MOD_B76,	0x00+5*8+2 }, // SID_Ix_L_MOD6_BASE+SID_Ix_MODx_OP
  { P_S_MODINV,	P_N_MOD,	P_M_MOD_B76,	0x00+6*8+2 }, // SID_Ix_L_MOD7_BASE+SID_Ix_MODx_OP
  { P_S_MODINV,	P_N_MOD,	P_M_MOD_B76,	0x00+7*8+2 }, // SID_Ix_L_MOD8_BASE+SID_Ix_MODx_OP
  // --[ 0x70-0x7f ]-----------------------------------------------------------------------------
  { P_S_NOP,	P_N_NOP,	P_M_NOP,	0x00 },
  { P_S_NOP,	P_N_NOP,	P_M_NOP,	0x00 },
  { P_S_NOP,	P_N_NOP,	P_M_NOP,	0x00 },
  { P_S_NOP,	P_N_NOP,	P_M_NOP,	0x00 },
  { P_S_NOP,	P_N_NOP,	P_M_NOP,	0x00 },
  { P_S_NOP,	P_N_NOP,	P_M_NOP,	0x00 },
  { P_S_NOP,	P_N_NOP,	P_M_NOP,	0x00 },
  { P_S_NOP,	P_N_NOP,	P_M_NOP,	0x00 },
  { P_S_NOP,	P_N_NOP,	P_M_NOP,	0x00 },
  { P_S_NOP,	P_N_NOP,	P_M_NOP,	0x00 },
  { P_S_NOP,	P_N_NOP,	P_M_NOP,	0x00 },
  { P_S_NOP,	P_N_NOP,	P_M_NOP,	0x00 },
  { P_S_NOP,	P_N_NOP,	P_M_NOP,	0x00 },
  { P_S_NOP,	P_N_NOP,	P_M_NOP,	0x00 },
  { P_S_NOP,	P_N_NOP,	P_M_NOP,	0x00 },
  { P_S_NOP,	P_N_NOP,	P_M_NOP,	0x00 },
  // --[ 0x80-0x87 ]-----------------------------------------------------------------------------
  { P_S_WAVEFORM, P_N_LFO,	P_M_LFO_4U,	0xc0+0*5+0 }, // SID_Ix_L_LFO1_BASE+SID_Ix_LFOx_MODE
  { P_S_WAVEFORM, P_N_LFO,	P_M_LFO_4U,	0xc0+1*5+0 }, // SID_Ix_L_LFO2_BASE+SID_Ix_LFOx_MODE
  { P_S_WAVEFORM, P_N_LFO,	P_M_LFO_4U,	0xc0+2*5+0 }, // SID_Ix_L_LFO3_BASE+SID_Ix_LFOx_MODE
  { P_S_WAVEFORM, P_N_LFO,	P_M_LFO_4U,	0xc0+3*5+0 }, // SID_Ix_L_LFO4_BASE+SID_Ix_LFOx_MODE
  { P_S_WAVEFORM, P_N_LFO,	P_M_LFO_4U,	0xc0+4*5+0 }, // SID_Ix_L_LFO5_BASE+SID_Ix_LFOx_MODE
  { P_S_WAVEFORM, P_N_LFO,	P_M_LFO_4U,	0xc0+5*5+0 }, // SID_Ix_L_LFO6_BASE+SID_Ix_LFOx_MODE
  { P_S_NOP,	P_N_NOP,	P_M_NOP,	0x00 },
  { P_S_NOP,	P_N_NOP,	P_M_NOP,	0x00 },
  // --[ 0x88-0x8f ]-----------------------------------------------------------------------------
  { P_S_DEPTH,	P_N_LFO,	P_M_LFO_PM8,	0xc0+0*5+1 }, // SID_Ix_L_LFO1_BASE+SID_Ix_LFOx_DEPTH
  { P_S_DEPTH,	P_N_LFO,	P_M_LFO_PM8,	0xc0+1*5+1 }, // SID_Ix_L_LFO2_BASE+SID_Ix_LFOx_DEPTH
  { P_S_DEPTH,	P_N_LFO,	P_M_LFO_PM8,	0xc0+2*5+1 }, // SID_Ix_L_LFO3_BASE+SID_Ix_LFOx_DEPTH
  { P_S_DEPTH,	P_N_LFO,	P_M_LFO_PM8,	0xc0+3*5+1 }, // SID_Ix_L_LFO4_BASE+SID_Ix_LFOx_DEPTH
  { P_S_DEPTH,	P_N_LFO,	P_M_LFO_PM8,	0xc0+4*5+1 }, // SID_Ix_L_LFO5_BASE+SID_Ix_LFOx_DEPTH
  { P_S_DEPTH,	P_N_LFO,	P_M_LFO_PM8,	0xc0+5*5+1 }, // SID_Ix_L_LFO6_BASE+SID_Ix_LFOx_DEPTH
  { P_S_NOP,	P_N_NOP,	P_M_NOP,	0x00 },
  { P_S_NOP,	P_N_NOP,	P_M_NOP,	0x00 },
  // --[ 0x90-097 ]-----------------------------------------------------------------------------
  { P_S_RATE,	P_N_LFO,	P_M_LFO_8,	0xc0+0*5+2 }, // SID_Ix_L_LFO1_BASE+SID_Ix_LFOx_RATE
  { P_S_RATE,	P_N_LFO,	P_M_LFO_8,	0xc0+1*5+2 }, // SID_Ix_L_LFO2_BASE+SID_Ix_LFOx_RATE
  { P_S_RATE,	P_N_LFO,	P_M_LFO_8,	0xc0+2*5+2 }, // SID_Ix_L_LFO3_BASE+SID_Ix_LFOx_RATE
  { P_S_RATE,	P_N_LFO,	P_M_LFO_8,	0xc0+3*5+2 }, // SID_Ix_L_LFO4_BASE+SID_Ix_LFOx_RATE
  { P_S_RATE,	P_N_LFO,	P_M_LFO_8,	0xc0+4*5+2 }, // SID_Ix_L_LFO5_BASE+SID_Ix_LFOx_RATE
  { P_S_RATE,	P_N_LFO,	P_M_LFO_8,	0xc0+5*5+2 }, // SID_Ix_L_LFO6_BASE+SID_Ix_LFOx_RATE
  { P_S_NOP,	P_N_NOP,	P_M_NOP,	0x00 },
  { P_S_NOP,	P_N_NOP,	P_M_NOP,	0x00 },
  // --[ 0x98-09f ]-----------------------------------------------------------------------------
  { P_S_DELAY,	P_N_LFO,	P_M_LFO_8,	0xc0+0*5+3 }, // SID_Ix_L_LFO1_BASE+SID_Ix_LFOx_DELAY
  { P_S_DELAY,	P_N_LFO,	P_M_LFO_8,	0xc0+1*5+3 }, // SID_Ix_L_LFO2_BASE+SID_Ix_LFOx_DELAY
  { P_S_DELAY,	P_N_LFO,	P_M_LFO_8,	0xc0+2*5+3 }, // SID_Ix_L_LFO3_BASE+SID_Ix_LFOx_DELAY
  { P_S_DELAY,	P_N_LFO,	P_M_LFO_8,	0xc0+3*5+3 }, // SID_Ix_L_LFO4_BASE+SID_Ix_LFOx_DELAY
  { P_S_DELAY,	P_N_LFO,	P_M_LFO_8,	0xc0+4*5+3 }, // SID_Ix_L_LFO5_BASE+SID_Ix_LFOx_DELAY
  { P_S_DELAY,	P_N_LFO,	P_M_LFO_8,	0xc0+5*5+3 }, // SID_Ix_L_LFO6_BASE+SID_Ix_LFOx_DELAY
  { P_S_NOP,	P_N_NOP,	P_M_NOP,	0x00 },
  { P_S_NOP,	P_N_NOP,	P_M_NOP,	0x00 },
  // --[ 0xa0-0a7 ]-----------------------------------------------------------------------------
  { P_S_PHASE,	P_N_LFO,	P_M_LFO_8,	0xc0+0*5+4 }, // SID_Ix_L_LFO1_BASE+SID_Ix_LFOx_PHASE
  { P_S_PHASE,	P_N_LFO,	P_M_LFO_8,	0xc0+1*5+4 }, // SID_Ix_L_LFO2_BASE+SID_Ix_LFOx_PHASE
  { P_S_PHASE,	P_N_LFO,	P_M_LFO_8,	0xc0+2*5+4 }, // SID_Ix_L_LFO3_BASE+SID_Ix_LFOx_PHASE
  { P_S_PHASE,	P_N_LFO,	P_M_LFO_8,	0xc0+3*5+4 }, // SID_Ix_L_LFO4_BASE+SID_Ix_LFOx_PHASE
  { P_S_PHASE,	P_N_LFO,	P_M_LFO_8,	0xc0+4*5+4 }, // SID_Ix_L_LFO5_BASE+SID_Ix_LFOx_PHASE
  { P_S_PHASE,	P_N_LFO,	P_M_LFO_8,	0xc0+5*5+4 }, // SID_Ix_L_LFO6_BASE+SID_Ix_LFOx_PHASE
  { P_S_NOP,	P_N_NOP,	P_M_NOP,	0x00 },
  { P_S_NOP,	P_N_NOP,	P_M_NOP,	0x00 },
  // --[ 0xa8-0af ]-----------------------------------------------------------------------------
  { P_S_NOP,	P_N_NOP,	P_M_NOP,	0x00 },
  { P_S_NOP,	P_N_NOP,	P_M_NOP,	0x00 },
  { P_S_NOP,	P_N_NOP,	P_M_NOP,	0x00 },
  { P_S_NOP,	P_N_NOP,	P_M_NOP,	0x00 },
  { P_S_NOP,	P_N_NOP,	P_M_NOP,	0x00 },
  { P_S_NOP,	P_N_NOP,	P_M_NOP,	0x00 },
  { P_S_NOP,	P_N_NOP,	P_M_NOP,	0x00 },
  { P_S_NOP,	P_N_NOP,	P_M_NOP,	0x00 },
  // --[ 0xb0-0bf ]-----------------------------------------------------------------------------
  { P_S_NOP,	P_N_NOP,	P_M_NOP,	0x00 },
  { P_S_NOP,	P_N_NOP,	P_M_NOP,	0x00 },
  { P_S_NOP,	P_N_NOP,	P_M_NOP,	0x00 },
  { P_S_NOP,	P_N_NOP,	P_M_NOP,	0x00 },
  { P_S_NOP,	P_N_NOP,	P_M_NOP,	0x00 },
  { P_S_NOP,	P_N_NOP,	P_M_NOP,	0x00 },
  { P_S_NOP,	P_N_NOP,	P_M_NOP,	0x00 },
  { P_S_NOP,	P_N_NOP,	P_M_NOP,	0x00 },
  { P_S_NOP,	P_N_NOP,	P_M_NOP,	0x00 },
  { P_S_NOP,	P_N_NOP,	P_M_NOP,	0x00 },
  { P_S_NOP,	P_N_NOP,	P_M_NOP,	0x00 },
  { P_S_NOP,	P_N_NOP,	P_M_NOP,	0x00 },
  { P_S_NOP,	P_N_NOP,	P_M_NOP,	0x00 },
  { P_S_NOP,	P_N_NOP,	P_M_NOP,	0x00 },
  { P_S_NOP,	P_N_NOP,	P_M_NOP,	0x00 },
  { P_S_NOP,	P_N_NOP,	P_M_NOP,	0x00 },
  // --[ 0xc0-0cf ]-----------------------------------------------------------------------------
  { P_S_MODE,	P_N_ENV,	P_M_ENV_8,	0xe0+0*16+0 }, // SID_Ix_L_ENV1_BASE+SID_Ix_L_ENVx_MODE
  { P_S_DEPTH,	P_N_ENV,	P_M_ENV_PM8,	0xe0+0*16+1 }, // SID_Ix_L_ENV1_BASE+SID_Ix_L_ENVx_DEPTH
  { P_S_DELAY,	P_N_ENV,	P_M_ENV_8,	0xe0+0*16+2 }, // SID_Ix_L_ENV1_BASE+SID_Ix_L_ENVx_DELAY
  { P_S_ATTACK1, P_N_ENV,	P_M_ENV_8,	0xe0+0*16+3 }, // SID_Ix_L_ENV1_BASE+SID_Ix_L_ENVx_ATTACK1
  { P_S_ALEVEL,	P_N_ENV,	P_M_ENV_8,	0xe0+0*16+4 }, // SID_Ix_L_ENV1_BASE+SID_Ix_L_ENVx_ATTLVL
  { P_S_ATTACK2, P_N_ENV,	P_M_ENV_8,	0xe0+0*16+5 }, // SID_Ix_L_ENV1_BASE+SID_Ix_L_ENVx_ATTACK2
  { P_S_DECAY1,	P_N_ENV,	P_M_ENV_8,	0xe0+0*16+6 }, // SID_Ix_L_ENV1_BASE+SID_Ix_L_ENVx_DECAY1
  { P_S_DLEVEL,	P_N_ENV,	P_M_ENV_8,	0xe0+0*16+7 }, // SID_Ix_L_ENV1_BASE+SID_Ix_L_ENVx_DECLVL
  { P_S_DECAY2,	P_N_ENV,	P_M_ENV_8,	0xe0+0*16+8 }, // SID_Ix_L_ENV1_BASE+SID_Ix_L_ENVx_DECAY2
  { P_S_SUSTAIN, P_N_ENV,	P_M_ENV_8,	0xe0+0*16+9 }, // SID_Ix_L_ENV1_BASE+SID_Ix_L_ENVx_SUSTAIN
  { P_S_RELEASE1, P_N_ENV,	P_M_ENV_8,	0xe0+0*16+10 }, // SID_Ix_L_ENV1_BASE+SID_Ix_L_ENVx_RELEASE1
  { P_S_RLEVEL,	P_N_ENV,	P_M_ENV_8,	0xe0+0*16+11 }, // SID_Ix_L_ENV1_BASE+SID_Ix_L_ENVx_RELLVL
  { P_S_RELEASE2, P_N_ENV,	P_M_ENV_8,	0xe0+0*16+12 }, // SID_Ix_L_ENV1_BASE+SID_Ix_L_ENVx_RELEASE2
  { P_S_ATT_CURVE, P_N_ENV,	P_M_ENV_8,	0xe0+0*16+13 }, // SID_Ix_L_ENV1_BASE+SID_Ix_L_ENVx_ATT_CURVE
  { P_S_DEC_CURVE, P_N_ENV,	P_M_ENV_8,	0xe0+0*16+14 }, // SID_Ix_L_ENV1_BASE+SID_Ix_L_ENVx_DEC_CURVE
  { P_S_REL_CURVE, P_N_ENV,	P_M_ENV_8,	0xe0+0*16+15 }, // SID_Ix_L_ENV1_BASE+SID_Ix_L_ENVx_REL_CURVE
  // --[ 0xd0-0df ]-----------------------------------------------------------------------------
  { P_S_MODE,	P_N_ENV,	P_M_ENV_8,	0xe0+1*16+0 }, // SID_Ix_L_ENV2_BASE+SID_Ix_L_ENVx_MODE
  { P_S_DEPTH,	P_N_ENV,	P_M_ENV_PM8,	0xe0+1*16+1 }, // SID_Ix_L_ENV2_BASE+SID_Ix_L_ENVx_DEPTH
  { P_S_DELAY,	P_N_ENV,	P_M_ENV_8,	0xe0+1*16+2 }, // SID_Ix_L_ENV2_BASE+SID_Ix_L_ENVx_DELAY
  { P_S_ATTACK1, P_N_ENV,	P_M_ENV_8,	0xe0+1*16+3 }, // SID_Ix_L_ENV2_BASE+SID_Ix_L_ENVx_ATTACK1
  { P_S_ALEVEL,	P_N_ENV,	P_M_ENV_8,	0xe0+1*16+4 }, // SID_Ix_L_ENV2_BASE+SID_Ix_L_ENVx_ATTLVL
  { P_S_ATTACK2, P_N_ENV,	P_M_ENV_8,	0xe0+1*16+5 }, // SID_Ix_L_ENV2_BASE+SID_Ix_L_ENVx_ATTACK2
  { P_S_DECAY1,	P_N_ENV,	P_M_ENV_8,	0xe0+1*16+6 }, // SID_Ix_L_ENV2_BASE+SID_Ix_L_ENVx_DECAY1
  { P_S_DLEVEL,	P_N_ENV,	P_M_ENV_8,	0xe0+1*16+7 }, // SID_Ix_L_ENV2_BASE+SID_Ix_L_ENVx_DECLVL
  { P_S_DECAY2,	P_N_ENV,	P_M_ENV_8,	0xe0+1*16+8 }, // SID_Ix_L_ENV2_BASE+SID_Ix_L_ENVx_DECAY2
  { P_S_SUSTAIN, P_N_ENV,	P_M_ENV_8,	0xe0+1*16+9 }, // SID_Ix_L_ENV2_BASE+SID_Ix_L_ENVx_SUSTAIN
  { P_S_RELEASE1, P_N_ENV,	P_M_ENV_8,	0xe0+1*16+10 }, // SID_Ix_L_ENV2_BASE+SID_Ix_L_ENVx_RELEASE1
  { P_S_RLEVEL,	P_N_ENV,	P_M_ENV_8,	0xe0+1*16+11 }, // SID_Ix_L_ENV2_BASE+SID_Ix_L_ENVx_RELLVL
  { P_S_RELEASE2, P_N_ENV,	P_M_ENV_8,	0xe0+1*16+12 }, // SID_Ix_L_ENV2_BASE+SID_Ix_L_ENVx_RELEASE2
  { P_S_ATT_CURVE, P_N_ENV,	P_M_ENV_8,	0xe0+1*16+13 }, // SID_Ix_L_ENV2_BASE+SID_Ix_L_ENVx_ATT_CURVE
  { P_S_DEC_CURVE, P_N_ENV,	P_M_ENV_8,	0xe0+1*16+14 }, // SID_Ix_L_ENV2_BASE+SID_Ix_L_ENVx_DEC_CURVE
  { P_S_REL_CURVE, P_N_ENV,	P_M_ENV_8,	0xe0+1*16+15 }, // SID_Ix_L_ENV2_BASE+SID_Ix_L_ENVx_REL_CURVE
  // --[ 0xe0-0e3 ]-----------------------------------------------------------------------------
  { P_S_SPEED,	P_N_WT,		P_M_WT_6,	0x6c+0*5+0 }, // SID_Ix_L_WT1_BASE+SID_Ix_WTx_SPEED
  { P_S_SPEED,	P_N_WT,		P_M_WT_6,	0x6c+1*5+0 }, // SID_Ix_L_WT2_BASE+SID_Ix_WTx_SPEED
  { P_S_SPEED,	P_N_WT,		P_M_WT_6,	0x6c+2*5+0 }, // SID_Ix_L_WT3_BASE+SID_Ix_WTx_SPEED
  { P_S_SPEED,	P_N_WT,		P_M_WT_6,	0x6c+3*5+0 }, // SID_Ix_L_WT4_BASE+SID_Ix_WTx_SPEED
  // --[ 0xe4-0e7 ]-----------------------------------------------------------------------------
  { P_S_START,	P_N_WT,		P_M_WT_7,	0x6c+0*5+2 }, // SID_Ix_L_WT1_BASE+SID_Ix_WTx_BEGIN
  { P_S_START,	P_N_WT,		P_M_WT_7,	0x6c+1*5+2 }, // SID_Ix_L_WT2_BASE+SID_Ix_WTx_BEGIN
  { P_S_START,	P_N_WT,		P_M_WT_7,	0x6c+2*5+2 }, // SID_Ix_L_WT3_BASE+SID_Ix_WTx_BEGIN
  { P_S_START,	P_N_WT,		P_M_WT_7,	0x6c+3*5+2 }, // SID_Ix_L_WT4_BASE+SID_Ix_WTx_BEGIN
  // --[ 0xe8-0eb ]-----------------------------------------------------------------------------
  { P_S_END,	P_N_WT,		P_M_WT_7,	0x6c+0*5+3 }, // SID_Ix_L_WT1_BASE+SID_Ix_WTx_END
  { P_S_END,	P_N_WT,		P_M_WT_7,	0x6c+1*5+3 }, // SID_Ix_L_WT2_BASE+SID_Ix_WTx_END
  { P_S_END,	P_N_WT,		P_M_WT_7,	0x6c+2*5+3 }, // SID_Ix_L_WT3_BASE+SID_Ix_WTx_END
  { P_S_END,	P_N_WT,		P_M_WT_7,	0x6c+3*5+3 }, // SID_Ix_L_WT4_BASE+SID_Ix_WTx_END
  // --[ 0xec-0ef ]-----------------------------------------------------------------------------
  { P_S_LOOP,	P_N_WT,		P_M_WT_7,	0x6c+0*5+4 }, // SID_Ix_L_WT1_BASE+SID_Ix_WTx_LOOP
  { P_S_LOOP,	P_N_WT,		P_M_WT_7,	0x6c+1*5+4 }, // SID_Ix_L_WT2_BASE+SID_Ix_WTx_LOOP
  { P_S_LOOP,	P_N_WT,		P_M_WT_7,	0x6c+2*5+4 }, // SID_Ix_L_WT3_BASE+SID_Ix_WTx_LOOP
  { P_S_LOOP,	P_N_WT,		P_M_WT_7,	0x6c+3*5+4 }, // SID_Ix_L_WT4_BASE+SID_Ix_WTx_LOOP
  // --[ 0xf0-0f3 ]-----------------------------------------------------------------------------
  { P_S_POSITION, P_N_WT,	P_M_WT_POS,	0 }, // SID_WT1_BASE+SID_WTx_POS
  { P_S_POSITION, P_N_WT,	P_M_WT_POS,	1 }, // SID_WT2_BASE+SID_WTx_POS
  { P_S_POSITION, P_N_WT,	P_M_WT_POS,	2 }, // SID_WT3_BASE+SID_WTx_POS
  { P_S_POSITION, P_N_WT,	P_M_WT_POS,	3 }, // SID_WT4_BASE+SID_WTx_POS
  // --[ 0xf4-0ff ]-----------------------------------------------------------------------------
  { P_S_NOP,	P_N_NOP,	P_M_NOP,	0x00 },
  { P_S_NOP,	P_N_NOP,	P_M_NOP,	0x00 },
  { P_S_NOP,	P_N_NOP,	P_M_NOP,	0x00 },
  { P_S_NOP,	P_N_NOP,	P_M_NOP,	0x00 },
  { P_S_NOP,	P_N_NOP,	P_M_NOP,	0x00 },
  { P_S_NOP,	P_N_NOP,	P_M_NOP,	0x00 },
  { P_S_NOP,	P_N_NOP,	P_M_NOP,	0x00 },
  { P_S_NOP,	P_N_NOP,	P_M_NOP,	0x00 },
  // --[ 0xfc-0ff ]-----------------------------------------------------------------------------
  { P_S_NOTE,	P_N_OSC123,	P_M_NOTE,	0 }, // SIDL_V1_BASE
  { P_S_NOTE,	P_N_OSC123,	P_M_NOTE,	0 }, // SIDL_V1_BASE
  { P_S_NOTE,	P_N_OSC123,	P_M_NOTE,	1 }, // SIDL_V2_BASE
  { P_S_NOTE,	P_N_OSC123,	P_M_NOTE,	2 }, // SIDL_V3_BASE
},

// SID_SE_BASSLINE
{
  //left string	right string	mod function	register/number
  // --[ 0x00-0x03 ]-----------------------------------------------------------------------------
  { P_S_NOP,	P_N_NOP,	P_M_NOP,	0x00 },
  { P_S_VOLUME,	P_N_NOP,	P_M_7,		0x52 }, // SID_Ix_L_VOLUME
  { P_S_NOP,	P_N_NOP,	P_M_NOP,	0x00 },
  { P_S_DETUNE,	P_N_OSC,	P_M_8,		0x51 }, // SID_Ix_L_OSC_DETUNE
  // --[ 0x04-0x07 ]-----------------------------------------------------------------------------
  { P_S_CUTOFF,	P_N_FIL,	P_M_FIL12,	0x54+1 }, // SID_Ix_L_S1F_BASE+SID_Ix_L_Fx_CUTOFF_L
  { P_S_RESONANCE, P_N_FIL,	P_M_FIL8,	0x54+3 }, // SID_Ix_L_S1F_BASE+SID_Ix_L_Fx_RESONANCE
  { P_S_CHANNELS, P_N_FIL,	P_M_FIL4L,	0x54+0 }, // SID_Ix_L_S1F_BASE+SID_Ix_L_Fx_CHN_MODE
  { P_S_MODE,	P_N_FIL,	P_M_FIL4U,	0x54+0 }, // SID_Ix_L_S1F_BASE+SID_Ix_L_Fx_CHN_MODE
  // --[ 0x08-0x0f ]-----------------------------------------------------------------------------
  { P_S_KNOB,	P_N_KNOB,	P_M_8,		0x18+0*5+0 }, // SID_Ix_P_K1_BASE+SID_Ix_Px_VALUE
  { P_S_KNOB,	P_N_KNOB,	P_M_8,		0x18+1*5+0 }, // SID_Ix_P_K2_BASE+SID_Ix_Px_VALUE
  { P_S_KNOB,	P_N_KNOB,	P_M_8,		0x18+2*5+0 }, // SID_Ix_P_K3_BASE+SID_Ix_Px_VALUE
  { P_S_KNOB,	P_N_KNOB,	P_M_8,		0x18+3*5+0 }, // SID_Ix_P_K4_BASE+SID_Ix_Px_VALUE
  { P_S_KNOB,	P_N_KNOB,	P_M_8,		0x18+4*5+0 }, // SID_Ix_P_K5_BASE+SID_Ix_Px_VALUE
  { P_S_KNOB,	P_N_KNOB,	P_M_8,		0x18+5*5+0 }, // SID_Ix_P_KV_BASE+SID_Ix_Px_VALUE
  { P_S_KNOB,	P_N_KNOB,	P_M_8,		0x18+6*5+0 }, // SID_Ix_P_KP_BASE+SID_Ix_Px_VALUE
  { P_S_KNOB,	P_N_KNOB,	P_M_8,		0x18+7*5+0 }, // SID_Ix_P_KA_BASE+SID_Ix_Px_VALUE
  // --[ 0x10-0x17 ]-----------------------------------------------------------------------------
  { P_S_EXT_AOUT, P_N_EXT,	P_M_PAR12,	0x40 }, // SID_Ix_EXT_PAR1_L
  { P_S_EXT_AOUT, P_N_EXT,	P_M_PAR12,	0x42 }, // SID_Ix_EXT_PAR2_L
  { P_S_EXT_AOUT, P_N_EXT,	P_M_PAR12,	0x44 }, // SID_Ix_EXT_PAR3_L
  { P_S_EXT_AOUT, P_N_EXT,	P_M_PAR12,	0x46 }, // SID_Ix_EXT_PAR4_L
  { P_S_EXT_AOUT, P_N_EXT,	P_M_PAR12,	0x48 }, // SID_Ix_EXT_PAR5_L
  { P_S_EXT_AOUT, P_N_EXT,	P_M_PAR12,	0x4a }, // SID_Ix_EXT_PAR6_L
  { P_S_EXT_AOUT, P_N_EXT,	P_M_PAR12,	0x4c }, // SID_Ix_EXT_PAR7_L
  { P_S_EXT_AOUT, P_N_EXT,	P_M_PAR12,	0x4e }, // SID_Ix_EXT_PAR8_L
  // --[ 0x18-0x1f ]-----------------------------------------------------------------------------
  { P_S_EXT_SWITCH, P_N_EXT,	P_M_CUSTOM_SW,	0x14 }, // SID_Ix_CUSTOM_SW
  { P_S_EXT_SWITCH, P_N_EXT,	P_M_CUSTOM_SW,	0x14 }, // SID_Ix_CUSTOM_SW
  { P_S_EXT_SWITCH, P_N_EXT,	P_M_CUSTOM_SW,	0x14 }, // SID_Ix_CUSTOM_SW
  { P_S_EXT_SWITCH, P_N_EXT,	P_M_CUSTOM_SW,	0x14 }, // SID_Ix_CUSTOM_SW
  { P_S_EXT_SWITCH, P_N_EXT,	P_M_CUSTOM_SW,	0x14 }, // SID_Ix_CUSTOM_SW
  { P_S_EXT_SWITCH, P_N_EXT,	P_M_CUSTOM_SW,	0x14 }, // SID_Ix_CUSTOM_SW
  { P_S_EXT_SWITCH, P_N_EXT,	P_M_CUSTOM_SW,	0x14 }, // SID_Ix_CUSTOM_SW
  { P_S_EXT_SWITCH, P_N_EXT,	P_M_CUSTOM_SW,	0x14 }, // SID_Ix_CUSTOM_SW
  // --[ 0x20-0x23 ]-----------------------------------------------------------------------------
  { P_S_WAVEFORM, P_N_OSC_BL,	P_M_OSC_BL_7,	0x60+1 }, // SID_Ix_B_S1V1_BASE+SID_Ix_Vx_WAVEFORM
  { P_S_WAVEFORM, P_N_OSC_BL,	P_M_OSC_BL_7,	0x60+1 }, // SID_Ix_B_S1V1_BASE+SID_Ix_Vx_WAVEFORM
  { P_S_WAVEFORM, P_N_OSC_BL,	P_M_OSC_BL_7,	0x60+1 }, // SID_Ix_B_S1V1_BASE+SID_Ix_Vx_WAVEFORM
  { P_S_WAVEFORM, P_N_OSC_BL,	P_M_OSC_BL_7,	0x60+1 }, // SID_Ix_B_S1V1_BASE+SID_Ix_Vx_WAVEFORM
  // --[ 0x24-0x27 ]-----------------------------------------------------------------------------
  { P_S_TRANSPOSE, P_N_OSC_BL,	P_M_OSC_BL_PM7,	0x60+8 }, // SID_Ix_B_S1V1_BASE+SID_Ix_Vx_TRANSPOSE
  { P_S_TRANSPOSE, P_N_OSC_BL,	P_M_OSC_BL_PM7,	0x60+8 }, // SID_Ix_B_S1V1_BASE+SID_Ix_Vx_TRANSPOSE
  { P_S_TRANSPOSE, P_N_OSC_BL,	P_M_OSC_BL_PM7,	0x60+8 }, // SID_Ix_B_S1V1_BASE+SID_Ix_Vx_TRANSPOSE
  { P_S_TRANSPOSE, P_N_OSC_BL,	P_M_OSC_BL_PM7,	0x60+8 }, // SID_Ix_B_S1V1_BASE+SID_Ix_Vx_TRANSPOSE
  // --[ 0x28-0x2b ]-----------------------------------------------------------------------------
  { P_S_FINETUNE, P_N_OSC_BL,	P_M_OSC_BL_PM8,	0x60+9 }, // SID_Ix_B_S1V1_BASE+SID_Ix_Vx_FINETUNE
  { P_S_FINETUNE, P_N_OSC_BL,	P_M_OSC_BL_PM8,	0x60+9 }, // SID_Ix_B_S1V1_BASE+SID_Ix_Vx_FINETUNE
  { P_S_FINETUNE, P_N_OSC_BL,	P_M_OSC_BL_PM8,	0x60+9 }, // SID_Ix_B_S1V1_BASE+SID_Ix_Vx_FINETUNE
  { P_S_FINETUNE, P_N_OSC_BL,	P_M_OSC_BL_PM8,	0x60+9 }, // SID_Ix_B_S1V1_BASE+SID_Ix_Vx_FINETUNE
  // --[ 0x2c-0x2f ]-----------------------------------------------------------------------------
  { P_S_PORTAMENTO, P_N_OSC_BL,	P_M_OSC_BL_8,	0x60+11 }, // SID_Ix_B_S1V1_BASE+SID_Ix_Vx_PORTAMENTO
  { P_S_PORTAMENTO, P_N_OSC_BL,	P_M_OSC_BL_8,	0x60+11 }, // SID_Ix_B_S1V1_BASE+SID_Ix_Vx_PORTAMENTO
  { P_S_PORTAMENTO, P_N_OSC_BL,	P_M_OSC_BL_8,	0x60+11 }, // SID_Ix_B_S1V1_BASE+SID_Ix_Vx_PORTAMENTO
  { P_S_PORTAMENTO, P_N_OSC_BL,	P_M_OSC_BL_8,	0x60+11 }, // SID_Ix_B_S1V1_BASE+SID_Ix_Vx_PORTAMENTO
  // --[ 0x30-0x33 ]-----------------------------------------------------------------------------
  { P_S_PULSEWIDTH, P_N_OSC_BL,	P_M_OSC_BL_12,	0x60+4 }, // SID_Ix_B_S1V1_BASE+SID_Ix_Vx_PULSEWIDTH_L
  { P_S_PULSEWIDTH, P_N_OSC_BL,	P_M_OSC_BL_12,	0x60+4 }, // SID_Ix_B_S1V1_BASE+SID_Ix_Vx_PULSEWIDTH_L
  { P_S_PULSEWIDTH, P_N_OSC_BL,	P_M_OSC_BL_12,	0x60+4 }, // SID_Ix_B_S1V1_BASE+SID_Ix_Vx_PULSEWIDTH_L
  { P_S_PULSEWIDTH, P_N_OSC_BL,	P_M_OSC_BL_12,	0x60+4 }, // SID_Ix_B_S1V1_BASE+SID_Ix_Vx_PULSEWIDTH_L
  // --[ 0x34-0x37 ]-----------------------------------------------------------------------------
  { P_S_DELAY,	P_N_OSC_BL,	P_M_OSC_BL_8,	0x60+7 }, // SID_Ix_B_S1V1_BASE+SID_Ix_Vx_DELAY
  { P_S_DELAY,	P_N_OSC_BL,	P_M_OSC_BL_8,	0x60+7 }, // SID_Ix_B_S1V1_BASE+SID_Ix_Vx_DELAY
  { P_S_DELAY,	P_N_OSC_BL,	P_M_OSC_BL_8,	0x60+7 }, // SID_Ix_B_S1V1_BASE+SID_Ix_Vx_DELAY
  { P_S_DELAY,	P_N_OSC_BL,	P_M_OSC_BL_8,	0x60+7 }, // SID_Ix_B_S1V1_BASE+SID_Ix_Vx_DELAY
  // --[ 0x38-0x3b ]-----------------------------------------------------------------------------
  { P_S_ATTACK,	P_N_OSC_BL,	P_M_OSC_BL_4U,	0x60+2 }, // SID_Ix_B_S1V1_BASE+SID_Ix_Vx_AD
  { P_S_ATTACK,	P_N_OSC_BL,	P_M_OSC_BL_4U,	0x60+2 }, // SID_Ix_B_S1V1_BASE+SID_Ix_Vx_AD
  { P_S_ATTACK,	P_N_OSC_BL,	P_M_OSC_BL_4U,	0x60+2 }, // SID_Ix_B_S1V1_BASE+SID_Ix_Vx_AD
  { P_S_ATTACK,	P_N_OSC_BL,	P_M_OSC_BL_4U,	0x60+2 }, // SID_Ix_B_S1V1_BASE+SID_Ix_Vx_AD
  // --[ 0x3c-0x3f ]-----------------------------------------------------------------------------
  { P_S_DECAY,	P_N_OSC_BL,	P_M_OSC_BL_4L,	0x60+2 }, // SID_Ix_B_S1V1_BASE+SID_Ix_Vx_AD
  { P_S_DECAY,	P_N_OSC_BL,	P_M_OSC_BL_4L,	0x60+2 }, // SID_Ix_B_S1V1_BASE+SID_Ix_Vx_AD
  { P_S_DECAY,	P_N_OSC_BL,	P_M_OSC_BL_4L,	0x60+2 }, // SID_Ix_B_S1V1_BASE+SID_Ix_Vx_AD
  { P_S_DECAY,	P_N_OSC_BL,	P_M_OSC_BL_4L,	0x60+2 }, // SID_Ix_B_S1V1_BASE+SID_Ix_Vx_AD
  // --[ 0x40-0x43 ]-----------------------------------------------------------------------------
  { P_S_SUSTAIN, P_N_OSC_BL,	P_M_OSC_BL_4U,	0x60+3 }, // SID_Ix_B_S1V1_BASE+SID_Ix_Vx_SR
  { P_S_SUSTAIN, P_N_OSC_BL,	P_M_OSC_BL_4U,	0x60+3 }, // SID_Ix_B_S1V1_BASE+SID_Ix_Vx_SR
  { P_S_SUSTAIN, P_N_OSC_BL,	P_M_OSC_BL_4U,	0x60+3 }, // SID_Ix_B_S1V1_BASE+SID_Ix_Vx_SR
  { P_S_SUSTAIN, P_N_OSC_BL,	P_M_OSC_BL_4U,	0x60+3 }, // SID_Ix_B_S1V1_BASE+SID_Ix_Vx_SR
  // --[ 0x44-0x47 ]-----------------------------------------------------------------------------
  { P_S_RELEASE, P_N_OSC_BL,	P_M_OSC_BL_4L,	0x60+3 }, // SID_Ix_B_S1V1_BASE+SID_Ix_Vx_SR
  { P_S_RELEASE, P_N_OSC_BL,	P_M_OSC_BL_4L,	0x60+3 }, // SID_Ix_B_S1V1_BASE+SID_Ix_Vx_SR
  { P_S_RELEASE, P_N_OSC_BL,	P_M_OSC_BL_4L,	0x60+3 }, // SID_Ix_B_S1V1_BASE+SID_Ix_Vx_SR
  { P_S_RELEASE, P_N_OSC_BL,	P_M_OSC_BL_4L,	0x60+3 }, // SID_Ix_B_S1V1_BASE+SID_Ix_Vx_SR
  // --[ 0x48-0x4b ]-----------------------------------------------------------------------------
  { P_S_ARP_SPEED, P_N_OSC_BL,	P_M_OSC_BL_6L,	0x60+13 }, // SID_Ix_B_S1V1_BASE+SID_Ix_Vx_ARP_SPEED_DIV
  { P_S_ARP_SPEED, P_N_OSC_BL,	P_M_OSC_BL_6L,	0x60+13 }, // SID_Ix_B_S1V1_BASE+SID_Ix_Vx_ARP_SPEED_DIV
  { P_S_ARP_SPEED, P_N_OSC_BL,	P_M_OSC_BL_6L,	0x60+13 }, // SID_Ix_B_S1V1_BASE+SID_Ix_Vx_ARP_SPEED_DIV
  { P_S_ARP_SPEED, P_N_OSC_BL,	P_M_OSC_BL_6L,	0x60+13 }, // SID_Ix_B_S1V1_BASE+SID_Ix_Vx_ARP_SPEED_DIV
  // --[ 0x4c-0x4f ]-----------------------------------------------------------------------------
  { P_S_ARP_GL,	P_N_OSC_BL,	P_M_OSC_BL_5L,	0x60+14 }, // SID_Ix_B_S1V1_BASE+SID_Ix_Vx_ARP_GL_RNG
  { P_S_ARP_GL,	P_N_OSC_BL,	P_M_OSC_BL_5L,	0x60+14 }, // SID_Ix_B_S1V1_BASE+SID_Ix_Vx_ARP_GL_RNG
  { P_S_ARP_GL,	P_N_OSC_BL,	P_M_OSC_BL_5L,	0x60+14 }, // SID_Ix_B_S1V1_BASE+SID_Ix_Vx_ARP_GL_RNG
  { P_S_ARP_GL,	P_N_OSC_BL,	P_M_OSC_BL_5L,	0x60+14 }, // SID_Ix_B_S1V1_BASE+SID_Ix_Vx_ARP_GL_RNG
  // --[ 0x50-0x53 ]-----------------------------------------------------------------------------
  { P_S_PITCHBENDER,P_N_OSC_BL,	P_M_OSC_BL_PB,	0 },
  { P_S_PITCHBENDER,P_N_OSC_BL,	P_M_OSC_BL_PB,	0 },
  { P_S_PITCHBENDER,P_N_OSC_BL,	P_M_OSC_BL_PB,	0 },
  { P_S_PITCHBENDER,P_N_OSC_BL,	P_M_OSC_BL_PB,	0 },
  // --[ 0x54-0x5f ]-----------------------------------------------------------------------------
  { P_S_NOP,	P_N_NOP,	P_M_NOP,	0 },
  { P_S_NOP,	P_N_NOP,	P_M_NOP,	0 },
  { P_S_NOP,	P_N_NOP,	P_M_NOP,	0 },
  { P_S_NOP,	P_N_NOP,	P_M_NOP,	0 },
  { P_S_NOP,	P_N_NOP,	P_M_NOP,	0 },
  { P_S_NOP,	P_N_NOP,	P_M_NOP,	0 },
  { P_S_NOP,	P_N_NOP,	P_M_NOP,	0 },
  { P_S_NOP,	P_N_NOP,	P_M_NOP,	0 },
  { P_S_NOP,	P_N_NOP,	P_M_NOP,	0 },
  { P_S_NOP,	P_N_NOP,	P_M_NOP,	0 },
  { P_S_NOP,	P_N_NOP,	P_M_NOP,	0 },
  { P_S_NOP,	P_N_NOP,	P_M_NOP,	0 },
  // --[ 0x60-0x63 ]-----------------------------------------------------------------------------
  { P_S_CUTOFF,	P_N_OSC_BL,	P_M_OSC_BL_FIL12,0x54+1 }, // SID_Ix_L_S1F_BASE+SID_Ix_L_Fx_CUTOFF_L
  { P_S_CUTOFF,	P_N_OSC_BL,	P_M_OSC_BL_FIL12,0x54+1 }, // SID_Ix_L_S1F_BASE+SID_Ix_L_Fx_CUTOFF_L
  { P_S_CUTOFF,	P_N_OSC_BL,	P_M_OSC_BL_FIL12,0x54+1 }, // SID_Ix_L_S1F_BASE+SID_Ix_L_Fx_CUTOFF_L
  { P_S_CUTOFF,	P_N_OSC_BL,	P_M_OSC_BL_FIL12,0x54+1 }, // SID_Ix_L_S1F_BASE+SID_Ix_L_Fx_CUTOFF_L
  // --[ 0x64-0x67 ]-----------------------------------------------------------------------------
  { P_S_RESONANCE, P_N_OSC_BL,	P_M_OSC_BL_FIL8,0x54+3 }, // SID_Ix_L_S1F_BASE+SID_Ix_L_Fx_RESONANCE
  { P_S_RESONANCE, P_N_OSC_BL,	P_M_OSC_BL_FIL8,0x54+3 }, // SID_Ix_L_S1F_BASE+SID_Ix_L_Fx_RESONANCE
  { P_S_RESONANCE, P_N_OSC_BL,	P_M_OSC_BL_FIL8,0x54+3 }, // SID_Ix_L_S1F_BASE+SID_Ix_L_Fx_RESONANCE
  { P_S_RESONANCE, P_N_OSC_BL,	P_M_OSC_BL_FIL8,0x54+3 }, // SID_Ix_L_S1F_BASE+SID_Ix_L_Fx_RESONANCE
  // --[ 0x68-0x6b ]-----------------------------------------------------------------------------
  { P_S_ENVMOD,	P_N_OSC_BL,	P_M_OSC_BL_P8,	0x60+0x25 }, // SID_Ix_B_S1V1_BASE+SID_Ix_B_Vx_ENV_DEPTH_F
  { P_S_ENVMOD,	P_N_OSC_BL,	P_M_OSC_BL_P8,	0x60+0x25 }, // SID_Ix_B_S1V1_BASE+SID_Ix_B_Vx_ENV_DEPTH_F
  { P_S_ENVMOD,	P_N_OSC_BL,	P_M_OSC_BL_P8,	0x60+0x25 }, // SID_Ix_B_S1V1_BASE+SID_Ix_B_Vx_ENV_DEPTH_F
  { P_S_ENVMOD,	P_N_OSC_BL,	P_M_OSC_BL_P8,	0x60+0x25 }, // SID_Ix_B_S1V1_BASE+SID_Ix_B_Vx_ENV_DEPTH_F
  // --[ 0x6c-0x6f ]-----------------------------------------------------------------------------
  { P_S_DECAY,	P_N_OSC_BL,	P_M_OSC_BL_8,	0x60+0x27 }, // SID_Ix_B_S1V1_BASE+SID_Ix_B_Vx_ENV_DECAY
  { P_S_DECAY,	P_N_OSC_BL,	P_M_OSC_BL_8,	0x60+0x27 }, // SID_Ix_B_S1V1_BASE+SID_Ix_B_Vx_ENV_DECAY
  { P_S_DECAY,	P_N_OSC_BL,	P_M_OSC_BL_8,	0x60+0x27 }, // SID_Ix_B_S1V1_BASE+SID_Ix_B_Vx_ENV_DECAY
  { P_S_DECAY,	P_N_OSC_BL,	P_M_OSC_BL_8,	0x60+0x27 }, // SID_Ix_B_S1V1_BASE+SID_Ix_B_Vx_ENV_DECAY
  // --[ 0x70-0x73 ]-----------------------------------------------------------------------------
  { P_S_ACCENT,	P_N_OSC_BL,	P_M_OSC_BL_7,	0x60+6 }, // SID_Ix_B_S1V1_BASE+SID_Ix_Vx_ACCENT
  { P_S_ACCENT,	P_N_OSC_BL,	P_M_OSC_BL_7,	0x60+6 }, // SID_Ix_B_S1V1_BASE+SID_Ix_Vx_ACCENT
  { P_S_ACCENT,	P_N_OSC_BL,	P_M_OSC_BL_7,	0x60+6 }, // SID_Ix_B_S1V1_BASE+SID_Ix_Vx_ACCENT
  { P_S_ACCENT,	P_N_OSC_BL,	P_M_OSC_BL_7,	0x60+6 }, // SID_Ix_B_S1V1_BASE+SID_Ix_Vx_ACCENT
  { P_S_NOP,	P_N_NOP,	P_M_NOP,	0 },
  { P_S_NOP,	P_N_NOP,	P_M_NOP,	0 },
  { P_S_NOP,	P_N_NOP,	P_M_NOP,	0 },
  { P_S_NOP,	P_N_NOP,	P_M_NOP,	0 },
  { P_S_NOP,	P_N_NOP,	P_M_NOP,	0 },
  { P_S_NOP,	P_N_NOP,	P_M_NOP,	0 },
  { P_S_NOP,	P_N_NOP,	P_M_NOP,	0 },
  { P_S_NOP,	P_N_NOP,	P_M_NOP,	0 },
  { P_S_NOP,	P_N_NOP,	P_M_NOP,	0 },
  { P_S_NOP,	P_N_NOP,	P_M_NOP,	0 },
  { P_S_NOP,	P_N_NOP,	P_M_NOP,	0 },
  { P_S_NOP,	P_N_NOP,	P_M_NOP,	0 },
  // --[ 0x80-0x87 ]-----------------------------------------------------------------------------
  { P_S_WAVEFORM, P_N_LFO_BL,	P_M_OSC_BL_4U,	0x60+0x14 }, // SID_Ix_B_S1V1_BASE+SID_Ix_B_Vx_LFO1_MODE
  { P_S_WAVEFORM, P_N_LFO_BL,	P_M_OSC_BL_4U,	0x60+0x14 }, // SID_Ix_B_S1V1_BASE+SID_Ix_B_Vx_LFO1_MODE
  { P_S_WAVEFORM, P_N_LFO_BL,	P_M_OSC_BL_4U,	0x60+0x14 }, // SID_Ix_B_S1V1_BASE+SID_Ix_B_Vx_LFO1_MODE
  { P_S_WAVEFORM, P_N_LFO_BL,	P_M_OSC_BL_4U,	0x60+0x14 }, // SID_Ix_B_S1V1_BASE+SID_Ix_B_Vx_LFO1_MODE
  { P_S_WAVEFORM, P_N_LFO_BL,	P_M_OSC_BL_4U,	0x60+0x1b }, // SID_Ix_B_S1V1_BASE+SID_Ix_B_Vx_LFO2_MODE
  { P_S_WAVEFORM, P_N_LFO_BL,	P_M_OSC_BL_4U,	0x60+0x1b }, // SID_Ix_B_S1V1_BASE+SID_Ix_B_Vx_LFO2_MODE
  { P_S_WAVEFORM, P_N_LFO_BL,	P_M_OSC_BL_4U,	0x60+0x1b }, // SID_Ix_B_S1V1_BASE+SID_Ix_B_Vx_LFO2_MODE
  { P_S_WAVEFORM, P_N_LFO_BL,	P_M_OSC_BL_4U,	0x60+0x1b }, // SID_Ix_B_S1V1_BASE+SID_Ix_B_Vx_LFO2_MODE
  // --[ 0x88-0x8f ]-----------------------------------------------------------------------------
  { P_S_DEPTH_P, P_N_LFO_BL,	P_M_OSC_BL_PM8,	0x60+0x15 }, // SID_Ix_B_S1V1_BASE+SID_Ix_B_Vx_LFO1_DEPTH_P
  { P_S_DEPTH_P, P_N_LFO_BL,	P_M_OSC_BL_PM8,	0x60+0x15 }, // SID_Ix_B_S1V1_BASE+SID_Ix_B_Vx_LFO1_DEPTH_P
  { P_S_DEPTH_P, P_N_LFO_BL,	P_M_OSC_BL_PM8,	0x60+0x15 }, // SID_Ix_B_S1V1_BASE+SID_Ix_B_Vx_LFO1_DEPTH_P
  { P_S_DEPTH_P, P_N_LFO_BL,	P_M_OSC_BL_PM8,	0x60+0x15 }, // SID_Ix_B_S1V1_BASE+SID_Ix_B_Vx_LFO1_DEPTH_P
  { P_S_DEPTH_P, P_N_LFO_BL,	P_M_OSC_BL_PM8,	0x60+0x1c }, // SID_Ix_B_S1V1_BASE+SID_Ix_B_Vx_LFO2_DEPTH_P
  { P_S_DEPTH_P, P_N_LFO_BL,	P_M_OSC_BL_PM8,	0x60+0x1c }, // SID_Ix_B_S1V1_BASE+SID_Ix_B_Vx_LFO2_DEPTH_P
  { P_S_DEPTH_P, P_N_LFO_BL,	P_M_OSC_BL_PM8,	0x60+0x1c }, // SID_Ix_B_S1V1_BASE+SID_Ix_B_Vx_LFO2_DEPTH_P
  { P_S_DEPTH_P, P_N_LFO_BL,	P_M_OSC_BL_PM8,	0x60+0x1c }, // SID_Ix_B_S1V1_BASE+SID_Ix_B_Vx_LFO2_DEPTH_P
  // --[ 0x90-0x97 ]-----------------------------------------------------------------------------
  { P_S_DEPTH_PW, P_N_LFO_BL,	P_M_OSC_BL_PM8,	0x60+0x19 }, // SID_Ix_B_S1V1_BASE+SID_Ix_B_Vx_LFO1_DEPTH_PW
  { P_S_DEPTH_PW, P_N_LFO_BL,	P_M_OSC_BL_PM8,	0x60+0x19 }, // SID_Ix_B_S1V1_BASE+SID_Ix_B_Vx_LFO1_DEPTH_PW
  { P_S_DEPTH_PW, P_N_LFO_BL,	P_M_OSC_BL_PM8,	0x60+0x19 }, // SID_Ix_B_S1V1_BASE+SID_Ix_B_Vx_LFO1_DEPTH_PW
  { P_S_DEPTH_PW, P_N_LFO_BL,	P_M_OSC_BL_PM8,	0x60+0x19 }, // SID_Ix_B_S1V1_BASE+SID_Ix_B_Vx_LFO1_DEPTH_PW
  { P_S_DEPTH_PW, P_N_LFO_BL,	P_M_OSC_BL_PM8,	0x60+0x20 }, // SID_Ix_B_S1V1_BASE+SID_Ix_B_Vx_LFO2_DEPTH_PW
  { P_S_DEPTH_PW, P_N_LFO_BL,	P_M_OSC_BL_PM8,	0x60+0x20 }, // SID_Ix_B_S1V1_BASE+SID_Ix_B_Vx_LFO2_DEPTH_PW
  { P_S_DEPTH_PW, P_N_LFO_BL,	P_M_OSC_BL_PM8,	0x60+0x20 }, // SID_Ix_B_S1V1_BASE+SID_Ix_B_Vx_LFO2_DEPTH_PW
  { P_S_DEPTH_PW, P_N_LFO_BL,	P_M_OSC_BL_PM8,	0x60+0x20 }, // SID_Ix_B_S1V1_BASE+SID_Ix_B_Vx_LFO2_DEPTH_PW
  // --[ 0x98-0x9f ]-----------------------------------------------------------------------------
  { P_S_DEPTH_F, P_N_LFO_BL,	P_M_OSC_BL_PM8,	0x60+0x1a }, // SID_Ix_B_S1V1_BASE+SID_Ix_B_Vx_LFO1_DEPTH_F
  { P_S_DEPTH_F, P_N_LFO_BL,	P_M_OSC_BL_PM8,	0x60+0x1a }, // SID_Ix_B_S1V1_BASE+SID_Ix_B_Vx_LFO1_DEPTH_F
  { P_S_DEPTH_F, P_N_LFO_BL,	P_M_OSC_BL_PM8,	0x60+0x1a }, // SID_Ix_B_S1V1_BASE+SID_Ix_B_Vx_LFO1_DEPTH_F
  { P_S_DEPTH_F, P_N_LFO_BL,	P_M_OSC_BL_PM8,	0x60+0x1a }, // SID_Ix_B_S1V1_BASE+SID_Ix_B_Vx_LFO1_DEPTH_F
  { P_S_DEPTH_F, P_N_LFO_BL,	P_M_OSC_BL_PM8,	0x60+0x21 }, // SID_Ix_B_S1V1_BASE+SID_Ix_B_Vx_LFO2_DEPTH_F
  { P_S_DEPTH_F, P_N_LFO_BL,	P_M_OSC_BL_PM8,	0x60+0x21 }, // SID_Ix_B_S1V1_BASE+SID_Ix_B_Vx_LFO2_DEPTH_F
  { P_S_DEPTH_F, P_N_LFO_BL,	P_M_OSC_BL_PM8,	0x60+0x21 }, // SID_Ix_B_S1V1_BASE+SID_Ix_B_Vx_LFO2_DEPTH_F
  { P_S_DEPTH_F, P_N_LFO_BL,	P_M_OSC_BL_PM8,	0x60+0x21 }, // SID_Ix_B_S1V1_BASE+SID_Ix_B_Vx_LFO2_DEPTH_F
  // --[ 0xa0-0xa7 ]-----------------------------------------------------------------------------
  { P_S_RATE,	P_N_LFO_BL,	P_M_OSC_BL_8,	0x60+0x16 }, // SID_Ix_B_S1V1_BASE+SID_Ix_B_Vx_LFO1_RATE
  { P_S_RATE,	P_N_LFO_BL,	P_M_OSC_BL_8,	0x60+0x16 }, // SID_Ix_B_S1V1_BASE+SID_Ix_B_Vx_LFO1_RATE
  { P_S_RATE,	P_N_LFO_BL,	P_M_OSC_BL_8,	0x60+0x16 }, // SID_Ix_B_S1V1_BASE+SID_Ix_B_Vx_LFO1_RATE
  { P_S_RATE,	P_N_LFO_BL,	P_M_OSC_BL_8,	0x60+0x16 }, // SID_Ix_B_S1V1_BASE+SID_Ix_B_Vx_LFO1_RATE
  { P_S_RATE,	P_N_LFO_BL,	P_M_OSC_BL_8,	0x60+0x1f }, // SID_Ix_B_S1V1_BASE+SID_Ix_B_Vx_LFO2_RATE
  { P_S_RATE,	P_N_LFO_BL,	P_M_OSC_BL_8,	0x60+0x1f }, // SID_Ix_B_S1V1_BASE+SID_Ix_B_Vx_LFO2_RATE
  { P_S_RATE,	P_N_LFO_BL,	P_M_OSC_BL_8,	0x60+0x1f }, // SID_Ix_B_S1V1_BASE+SID_Ix_B_Vx_LFO2_RATE
  { P_S_RATE,	P_N_LFO_BL,	P_M_OSC_BL_8,	0x60+0x1f }, // SID_Ix_B_S1V1_BASE+SID_Ix_B_Vx_LFO2_RATE
  // --[ 0xa8-0xaf ]-----------------------------------------------------------------------------
  { P_S_DELAY,	P_N_LFO_BL,	P_M_OSC_BL_8,	0x60+0x17 }, // SID_Ix_B_S1V1_BASE+SID_Ix_B_Vx_LFO1_DELAY
  { P_S_DELAY,	P_N_LFO_BL,	P_M_OSC_BL_8,	0x60+0x17 }, // SID_Ix_B_S1V1_BASE+SID_Ix_B_Vx_LFO1_DELAY
  { P_S_DELAY,	P_N_LFO_BL,	P_M_OSC_BL_8,	0x60+0x17 }, // SID_Ix_B_S1V1_BASE+SID_Ix_B_Vx_LFO1_DELAY
  { P_S_DELAY,	P_N_LFO_BL,	P_M_OSC_BL_8,	0x60+0x17 }, // SID_Ix_B_S1V1_BASE+SID_Ix_B_Vx_LFO1_DELAY
  { P_S_DELAY,	P_N_LFO_BL,	P_M_OSC_BL_8,	0x60+0x1e }, // SID_Ix_B_S1V1_BASE+SID_Ix_B_Vx_LFO2_DELAY
  { P_S_DELAY,	P_N_LFO_BL,	P_M_OSC_BL_8,	0x60+0x1e }, // SID_Ix_B_S1V1_BASE+SID_Ix_B_Vx_LFO2_DELAY
  { P_S_DELAY,	P_N_LFO_BL,	P_M_OSC_BL_8,	0x60+0x1e }, // SID_Ix_B_S1V1_BASE+SID_Ix_B_Vx_LFO2_DELAY
  { P_S_DELAY,	P_N_LFO_BL,	P_M_OSC_BL_8,	0x60+0x1e }, // SID_Ix_B_S1V1_BASE+SID_Ix_B_Vx_LFO2_DELAY
  // --[ 0xb0-0xb7 ]-----------------------------------------------------------------------------
  { P_S_PHASE,	P_N_LFO_BL,	P_M_OSC_BL_8,	0x60+0x18 }, // SID_Ix_B_S1V1_BASE+SID_Ix_B_Vx_LFO1_PHASE
  { P_S_PHASE,	P_N_LFO_BL,	P_M_OSC_BL_8,	0x60+0x18 }, // SID_Ix_B_S1V1_BASE+SID_Ix_B_Vx_LFO1_PHASE
  { P_S_PHASE,	P_N_LFO_BL,	P_M_OSC_BL_8,	0x60+0x18 }, // SID_Ix_B_S1V1_BASE+SID_Ix_B_Vx_LFO1_PHASE
  { P_S_PHASE,	P_N_LFO_BL,	P_M_OSC_BL_8,	0x60+0x18 }, // SID_Ix_B_S1V1_BASE+SID_Ix_B_Vx_LFO1_PHASE
  { P_S_PHASE,	P_N_LFO_BL,	P_M_OSC_BL_8,	0x60+0x1d }, // SID_Ix_B_S1V1_BASE+SID_Ix_B_Vx_LFO2_PHASE
  { P_S_PHASE,	P_N_LFO_BL,	P_M_OSC_BL_8,	0x60+0x1d }, // SID_Ix_B_S1V1_BASE+SID_Ix_B_Vx_LFO2_PHASE
  { P_S_PHASE,	P_N_LFO_BL,	P_M_OSC_BL_8,	0x60+0x1d }, // SID_Ix_B_S1V1_BASE+SID_Ix_B_Vx_LFO2_PHASE
  { P_S_PHASE,	P_N_LFO_BL,	P_M_OSC_BL_8,	0x60+0x1d }, // SID_Ix_B_S1V1_BASE+SID_Ix_B_Vx_LFO2_PHASE
  { P_S_NOP,	P_N_NOP,	P_M_NOP,	0 },
  { P_S_NOP,	P_N_NOP,	P_M_NOP,	0 },
  { P_S_NOP,	P_N_NOP,	P_M_NOP,	0 },
  { P_S_NOP,	P_N_NOP,	P_M_NOP,	0 },
  { P_S_NOP,	P_N_NOP,	P_M_NOP,	0 },
  { P_S_NOP,	P_N_NOP,	P_M_NOP,	0 },
  { P_S_NOP,	P_N_NOP,	P_M_NOP,	0 },
  { P_S_NOP,	P_N_NOP,	P_M_NOP,	0 },
  // --[ 0xc0-0xc3 ]-----------------------------------------------------------------------------
  { P_S_DEPTH_P, P_N_ENV_BL,	P_M_OSC_BL_PM8,	0x60+0x23 }, // SID_Ix_B_S1V1_BASE+SID_Ix_B_Vx_ENV_DEPTH_P
  { P_S_DEPTH_P, P_N_ENV_BL,	P_M_OSC_BL_PM8,	0x60+0x23 }, // SID_Ix_B_S1V1_BASE+SID_Ix_B_Vx_ENV_DEPTH_P
  { P_S_DEPTH_P, P_N_ENV_BL,	P_M_OSC_BL_PM8,	0x60+0x23 }, // SID_Ix_B_S1V1_BASE+SID_Ix_B_Vx_ENV_DEPTH_P
  { P_S_DEPTH_P, P_N_ENV_BL,	P_M_OSC_BL_PM8,	0x60+0x23 }, // SID_Ix_B_S1V1_BASE+SID_Ix_B_Vx_ENV_DEPTH_P
  // --[ 0xc4-0xc7 ]-----------------------------------------------------------------------------
  { P_S_DEPTH_PW, P_N_ENV_BL,	P_M_OSC_BL_PM8,	0x60+0x24 }, // SID_Ix_B_S1V1_BASE+SID_Ix_B_Vx_ENV_DEPTH_PW
  { P_S_DEPTH_PW, P_N_ENV_BL,	P_M_OSC_BL_PM8,	0x60+0x24 }, // SID_Ix_B_S1V1_BASE+SID_Ix_B_Vx_ENV_DEPTH_PW
  { P_S_DEPTH_PW, P_N_ENV_BL,	P_M_OSC_BL_PM8,	0x60+0x24 }, // SID_Ix_B_S1V1_BASE+SID_Ix_B_Vx_ENV_DEPTH_PW
  { P_S_DEPTH_PW, P_N_ENV_BL,	P_M_OSC_BL_PM8,	0x60+0x24 }, // SID_Ix_B_S1V1_BASE+SID_Ix_B_Vx_ENV_DEPTH_PW
  // --[ 0xc8-0xcb ]-----------------------------------------------------------------------------
  { P_S_DEPTH_F, P_N_ENV_BL,	P_M_OSC_BL_PM8,	0x60+0x25 }, // SID_Ix_B_S1V1_BASE+SID_Ix_B_Vx_ENV_DEPTH_F
  { P_S_DEPTH_F, P_N_ENV_BL,	P_M_OSC_BL_PM8,	0x60+0x25 }, // SID_Ix_B_S1V1_BASE+SID_Ix_B_Vx_ENV_DEPTH_F
  { P_S_DEPTH_F, P_N_ENV_BL,	P_M_OSC_BL_PM8,	0x60+0x25 }, // SID_Ix_B_S1V1_BASE+SID_Ix_B_Vx_ENV_DEPTH_F
  { P_S_DEPTH_F, P_N_ENV_BL,	P_M_OSC_BL_PM8,	0x60+0x25 }, // SID_Ix_B_S1V1_BASE+SID_Ix_B_Vx_ENV_DEPTH_F
  // --[ 0xcc-0xcf ]-----------------------------------------------------------------------------
  { P_S_ATTACK,	P_N_ENV_BL,	P_M_OSC_BL_8,	0x60+0x26 }, // SID_Ix_B_S1V1_BASE+SID_Ix_B_Vx_ENV_ATTACK
  { P_S_ATTACK,	P_N_ENV_BL,	P_M_OSC_BL_8,	0x60+0x26 }, // SID_Ix_B_S1V1_BASE+SID_Ix_B_Vx_ENV_ATTACK
  { P_S_ATTACK,	P_N_ENV_BL,	P_M_OSC_BL_8,	0x60+0x26 }, // SID_Ix_B_S1V1_BASE+SID_Ix_B_Vx_ENV_ATTACK
  { P_S_ATTACK,	P_N_ENV_BL,	P_M_OSC_BL_8,	0x60+0x26 }, // SID_Ix_B_S1V1_BASE+SID_Ix_B_Vx_ENV_ATTACK
  // --[ 0xd0-0xd3 ]-----------------------------------------------------------------------------
  { P_S_DECAY,	P_N_ENV_BL,	P_M_OSC_BL_8,	0x60+0x27 }, // SID_Ix_B_S1V1_BASE+SID_Ix_B_Vx_ENV_DECAY
  { P_S_DECAY,	P_N_ENV_BL,	P_M_OSC_BL_8,	0x60+0x27 }, // SID_Ix_B_S1V1_BASE+SID_Ix_B_Vx_ENV_DECAY
  { P_S_DECAY,	P_N_ENV_BL,	P_M_OSC_BL_8,	0x60+0x27 }, // SID_Ix_B_S1V1_BASE+SID_Ix_B_Vx_ENV_DECAY
  { P_S_DECAY,	P_N_ENV_BL,	P_M_OSC_BL_8,	0x60+0x27 }, // SID_Ix_B_S1V1_BASE+SID_Ix_B_Vx_ENV_DECAY
  // --[ 0xd4-0xd7 ]-----------------------------------------------------------------------------
  { P_S_SUSTAIN, P_N_ENV_BL,	P_M_OSC_BL_8,	0x60+0x28 }, // SID_Ix_B_S1V1_BASE+SID_Ix_B_Vx_ENV_SUSTAIN
  { P_S_SUSTAIN, P_N_ENV_BL,	P_M_OSC_BL_8,	0x60+0x28 }, // SID_Ix_B_S1V1_BASE+SID_Ix_B_Vx_ENV_SUSTAIN
  { P_S_SUSTAIN, P_N_ENV_BL,	P_M_OSC_BL_8,	0x60+0x28 }, // SID_Ix_B_S1V1_BASE+SID_Ix_B_Vx_ENV_SUSTAIN
  { P_S_SUSTAIN, P_N_ENV_BL,	P_M_OSC_BL_8,	0x60+0x28 }, // SID_Ix_B_S1V1_BASE+SID_Ix_B_Vx_ENV_SUSTAIN
  // --[ 0xd8-0xdb ]-----------------------------------------------------------------------------
  { P_S_RELEASE, P_N_ENV_BL,	P_M_OSC_BL_8,	0x60+0x29 }, // SID_Ix_B_S1V1_BASE+SID_Ix_B_Vx_ENV_RELEASE
  { P_S_RELEASE, P_N_ENV_BL,	P_M_OSC_BL_8,	0x60+0x29 }, // SID_Ix_B_S1V1_BASE+SID_Ix_B_Vx_ENV_RELEASE
  { P_S_RELEASE, P_N_ENV_BL,	P_M_OSC_BL_8,	0x60+0x29 }, // SID_Ix_B_S1V1_BASE+SID_Ix_B_Vx_ENV_RELEASE
  { P_S_RELEASE, P_N_ENV_BL,	P_M_OSC_BL_8,	0x60+0x29 }, // SID_Ix_B_S1V1_BASE+SID_Ix_B_Vx_ENV_RELEASE
  // --[ 0xdc-0xdf ]-----------------------------------------------------------------------------
  { P_S_CURVE,	P_N_ENV_BL,	P_M_OSC_BL_8,	0x60+0x2a }, // SID_Ix_B_S1V1_BASE+SID_Ix_B_Vx_ENV_CURVE
  { P_S_CURVE,	P_N_ENV_BL,	P_M_OSC_BL_8,	0x60+0x2a }, // SID_Ix_B_S1V1_BASE+SID_Ix_B_Vx_ENV_CURVE
  { P_S_CURVE,	P_N_ENV_BL,	P_M_OSC_BL_8,	0x60+0x2a }, // SID_Ix_B_S1V1_BASE+SID_Ix_B_Vx_ENV_CURVE
  { P_S_CURVE,	P_N_ENV_BL,	P_M_OSC_BL_8,	0x60+0x2a }, // SID_Ix_B_S1V1_BASE+SID_Ix_B_Vx_ENV_CURVE
  // --[ 0xe0-0xe3 ]-----------------------------------------------------------------------------
  { P_S_DECAY_A, P_N_ENV_BL,	P_M_OSC_BL_8,	0x60+0x30 }, // SID_Ix_B_S1V1_BASE+SID_Ix_B_Vx_ENV_DECAY_A
  { P_S_DECAY_A, P_N_ENV_BL,	P_M_OSC_BL_8,	0x60+0x30 }, // SID_Ix_B_S1V1_BASE+SID_Ix_B_Vx_ENV_DECAY_A
  { P_S_DECAY_A, P_N_ENV_BL,	P_M_OSC_BL_8,	0x60+0x30 }, // SID_Ix_B_S1V1_BASE+SID_Ix_B_Vx_ENV_DECAY_A
  { P_S_DECAY_A, P_N_ENV_BL,	P_M_OSC_BL_8,	0x60+0x30 }, // SID_Ix_B_S1V1_BASE+SID_Ix_B_Vx_ENV_DECAY_A
  { P_S_NOP,	P_N_NOP,	P_M_NOP,	0 },
  { P_S_NOP,	P_N_NOP,	P_M_NOP,	0 },
  { P_S_NOP,	P_N_NOP,	P_M_NOP,	0 },
  { P_S_NOP,	P_N_NOP,	P_M_NOP,	0 },
  { P_S_NOP,	P_N_NOP,	P_M_NOP,	0 },
  { P_S_NOP,	P_N_NOP,	P_M_NOP,	0 },
  { P_S_NOP,	P_N_NOP,	P_M_NOP,	0 },
  { P_S_NOP,	P_N_NOP,	P_M_NOP,	0 },
  { P_S_NOP,	P_N_NOP,	P_M_NOP,	0 },
  { P_S_NOP,	P_N_NOP,	P_M_NOP,	0 },
  { P_S_NOP,	P_N_NOP,	P_M_NOP,	0 },
  { P_S_NOP,	P_N_NOP,	P_M_NOP,	0 },
  // --[ 0xf0-0xff ]-----------------------------------------------------------------------------
  { P_S_NOP,	P_N_NOP,	P_M_NOP,	0 },
  { P_S_NOP,	P_N_NOP,	P_M_NOP,	0 },
  { P_S_NOP,	P_N_NOP,	P_M_NOP,	0 },
  { P_S_NOP,	P_N_NOP,	P_M_NOP,	0 },
  { P_S_NOP,	P_N_NOP,	P_M_NOP,	0 },
  { P_S_NOP,	P_N_NOP,	P_M_NOP,	0 },
  { P_S_NOP,	P_N_NOP,	P_M_NOP,	0 },
  { P_S_NOP,	P_N_NOP,	P_M_NOP,	0 },
  { P_S_NOP,	P_N_NOP,	P_M_NOP,	0 },
  { P_S_NOP,	P_N_NOP,	P_M_NOP,	0 },
  { P_S_NOP,	P_N_NOP,	P_M_NOP,	0 },
  { P_S_NOP,	P_N_NOP,	P_M_NOP,	0 },
  { P_S_NOP,	P_N_NOP,	P_M_NOP,	0 },
  { P_S_NOP,	P_N_NOP,	P_M_NOP,	0 },
  { P_S_NOP,	P_N_NOP,	P_M_NOP,	0 },
  { P_S_NOP,	P_N_NOP,	P_M_NOP,	0 },
},
};



/////////////////////////////////////////////////////////////////////////////
// Scales a 16bit value to target range (scale_up=0 -> scale down)
// or a source value to 16bit (scale_up=1)
/////////////////////////////////////////////////////////////////////////////
static u16 SID_PAR_Scale(sid_par_table_item_t *item, u16 value, u8 scale_up)
{
  u8 resolution = sid_par_table_m_resolution[item->mod_function];

  if( resolution == 0 || resolution >= 16 )
    return value;

  return scale_up ? (value << (16-resolution)) : (value >> (16-resolution));
}


/////////////////////////////////////////////////////////////////////////////
// Sets a parameter value
// sid selects the SID
// par selects the parameter (0..255)
// value contains unscaled value (range depends on parameter)
// sidlr selects left/right SID channel (0..1)
// ins selects current bassline/multi/drum instrument (1..3/0..5/0..15)
/////////////////////////////////////////////////////////////////////////////
s32 SID_PAR_Set(u8 sid, u8 par, u16 value, u8 sidlr, u8 ins)
{
  int i;
  sid_se_engine_t engine = sid_patch[sid].engine;
  if( engine >= SID_SE_NUM_ENGINES )
    engine = SID_SE_LEAD;
  sid_par_table_item_t *item = (sid_par_table_item_t *)&sid_par_table[engine][par];
  sid_patch_t *patch = (sid_patch_t *)&sid_patch[sid];

  // help variables used by voice parameters
  u8 voice_sel = 0x3f; // select all 6 voices by default
  switch( engine ) {
    case SID_SE_LEAD:
      if( (par & 3) > 0)
	voice_sel = 0x09 << ((par&3)-1); // 2 voices
      if( !(sidlr & 1) )
	voice_sel &= ~0x7;
      if( !(sidlr & 2) )
	voice_sel &= ~0x38;
      break;

    case SID_SE_BASSLINE:
      switch( par & 3 ) {
        case 1: voice_sel = sidlr; break; // selected voices
        case 2: voice_sel = 0x01; break; // first voice only
        case 3: voice_sel = 0x02; break; // second voice only
        default: voice_sel = 3; break; // both voices
      }
      break;

    default:
      return -1; // engine not supported yet
  }

  switch( item->mod_function ) {
    case P_M_NOP: {
      return 0; // nothing to do
    } break;
  
    case P_M_7: {
      patch->ALL[item->addr_l] = value & 0x7f;
    } break;
  
    case P_M_8:
    case P_M_PM8:
    case P_M_LFO_PM8:
    case P_M_LFO_8:
    case P_M_ENV_PM8:
    case P_M_ENV_8: {
  
      patch->ALL[item->addr_l] = value;
    } break;
  
    case P_M_4L: {
      patch->ALL[item->addr_l] = (patch->ALL[item->addr_l] & 0xf0) | (value & 0x0f);
    } break;
  
    case P_M_4U:
    case P_M_LFO_4U: {
      patch->ALL[item->addr_l] = (patch->ALL[item->addr_l] & 0x0f) | ((value << 4) & 0xf0);
    } break;
  
    case P_M_PAR12: {
      u16 value16 = value << 4;
      patch->ALL[item->addr_l + 0] = (value16 >> 0);
      patch->ALL[item->addr_l + 1] = (value16 >> 8);
    } break;
  
    case P_M_CUSTOM_SW: {
      patch->ALL[item->addr_l] = ((value & 1) << (par & 7));
    } break;
  
    case P_M_FIL4L: {
      for(i=0; i<2; ++i)
	if( sidlr & (1 << i) )
	  patch->ALL[item->addr_l + i*6] = (patch->ALL[item->addr_l + i*6] & 0xf0) | (value & 0x0f);
    } break;
  
    case P_M_FIL4U: {
      for(i=0; i<2; ++i)
	if( sidlr & (1 << i) )
	  patch->ALL[item->addr_l + i*6] = (patch->ALL[item->addr_l + i*6] & 0x0f) | ((value << 4) & 0xf0);
    } break;
  
    case P_M_FIL12: {
      for(i=0; i<2; ++i)
	if( sidlr & (1 << i) ) {
	  patch->ALL[item->addr_l + i*6 + 0] = value;
	  patch->ALL[item->addr_l + i*6 + 1] = (patch->ALL[item->addr_l + i*6 + 1] & 0xf0) | ((value >> 8) & 0x0f);
	}
    } break;
  
    case P_M_FIL12_DIRECT: {
      patch->ALL[item->addr_l + 0] = value;
      patch->ALL[item->addr_l + 1] = (patch->ALL[item->addr_l + 1] & 0xf0) | ((value >> 8) & 0x0f);
    } break;
  
    case P_M_FIL8: {
      for(i=0; i<2; ++i)
	if( sidlr & (1 << i) )
	  patch->ALL[item->addr_l + i*6 + 0] = value;
    } break;
  
    case P_M_OSC123_PM7:
    case P_M_OSC123_PM8:
    case P_M_OSC123_7:
    case P_M_OSC123_8: {
      for(i=0; i<6; ++i)
	if( voice_sel & (1 << i) )
	  patch->ALL[item->addr_l + i*0x10] = value;
    } break;
  
    case P_M_OSC123_12: {
      for(i=0; i<6; ++i)
	if( voice_sel & (1 << i) ) {
	  patch->ALL[item->addr_l + i*0x10 + 0] = value;
	  patch->ALL[item->addr_l + i*0x10 + 1] = (patch->ALL[item->addr_l + i*0x10 + 1] & 0xf0) | ((value >> 8) & 0x0f);
	}
    } break;
  
    case P_M_OSC123_4L: {
      for(i=0; i<6; ++i)
	if( voice_sel & (1 << i) )
	  patch->ALL[item->addr_l + i*0x10] = (patch->ALL[item->addr_l + i*0x10] & 0xf0) | (value & 0x0f);
    } break;
  
    case P_M_OSC123_5L: {
      for(i=0; i<6; ++i)
	if( voice_sel & (1 << i) )
	  patch->ALL[item->addr_l + i*0x10] = (patch->ALL[item->addr_l + i*0x10] & 0xe0) | (value & 0x1f);
    } break;
  
    case P_M_OSC123_6L: {
      for(i=0; i<6; ++i)
	if( voice_sel & (1 << i) )
	  patch->ALL[item->addr_l + i*0x10] = (patch->ALL[item->addr_l + i*0x10] & 0xc0) | (value & 0x3f);
    } break;
  
    case P_M_OSC123_4U: {
      for(i=0; i<6; ++i)
	if( voice_sel & (1 << i) )
	  patch->ALL[item->addr_l + i*0x10] = (patch->ALL[item->addr_l + i*0x10] & 0x0f) | ((value << 4) & 0xf0);
    } break;
  
    case P_M_OSC123_PB: {
      for(i=0; i<6; ++i)
	if( voice_sel & (1 << i) )
	  sid_se_voice[sid][i].mv->pitchbender = value;
    } break;
  
    case P_M_MOD_PM8: {
      patch->ALL[0x100 | item->addr_l] = value;
    } break;
  
    case P_M_MOD_B76: {
      patch->ALL[0x100 | item->addr_l] = (patch->ALL[0x100 | item->addr_l] & 0x3f) | ((value << 6) & 0xc0);
    } break;
  
    case P_M_WT_6: {
      patch->ALL[0x100 | item->addr_l] = (0x100 | patch->ALL[item->addr_l] & 0xc0) | (value & 0x3f);
    } break;
  
    case P_M_WT_7: {
      patch->ALL[0x100 | item->addr_l] = (0x100 | patch->ALL[item->addr_l] & 0x80) | (value & 0x7f);
    } break;
  
    case P_M_WT_POS: {
      sid_se_wt[sid][par&3].pos = value & 0x7f;
    } break;
  
    case P_M_OSC_INS_PM7: {
      // TODO
    } break;
  
    case P_M_OSC_INS_PM8: {
      // TODO
    } break;
  
    case P_M_OSC_INS_7: {
      // TODO
    } break;
  
    case P_M_OSC_INS_8: {
      // TODO
    } break;
  
    case P_M_OSC_INS_12: {
      // TODO
    } break;
  
    case P_M_OSC_INS_4L: {
      // TODO
    } break;
  
    case P_M_OSC_INS_5L: {
      // TODO
    } break;
  
    case P_M_OSC_INS_6L: {
      // TODO
    } break;

    case P_M_OSC_INS_4U: {
      // TODO  
    } break;

    case P_M_OSC_INS_PB: {
      // TODO  
    } break;

    case P_M_OSC_BL_PM7:
    case P_M_OSC_BL_PM8:
    case P_M_OSC_BL_7:
    case P_M_OSC_BL_8: {
      for(i=0; i<2; ++i)
	if( voice_sel & (1 << i) )
	  patch->ALL[item->addr_l + i*0x50] = value;
    } break;

    case P_M_OSC_BL_P8: {
      for(i=0; i<2; ++i)
	if( voice_sel & (1 << i) )
	  patch->ALL[item->addr_l + i*0x50] = value + 0x80;
    } break;

    case P_M_OSC_BL_12: {
      for(i=0; i<2; ++i)
	if( voice_sel & (1 << i) ) {
	  patch->ALL[item->addr_l + i*0x50 + 0] = value;
	  patch->ALL[item->addr_l + i*0x50 + 1] = (patch->ALL[item->addr_l + i*0x50 + 1] & 0xf0) | ((value >> 8) & 0x0f);
	}
    } break;

    case P_M_OSC_BL_4L: {
      for(i=0; i<2; ++i)
	if( voice_sel & (1 << i) )
	  patch->ALL[item->addr_l + i*0x50] = (patch->ALL[item->addr_l + i*0x50] & 0xf0) | (value & 0x0f);
    } break;

    case P_M_OSC_BL_5L: {
      for(i=0; i<2; ++i)
	if( voice_sel & (1 << i) )
	  patch->ALL[item->addr_l + i*0x50] = (patch->ALL[item->addr_l + i*0x50] & 0xe0) | (value & 0x1f);
    } break;

    case P_M_OSC_BL_6L: {
      for(i=0; i<2; ++i)
	if( voice_sel & (1 << i) )
	  patch->ALL[item->addr_l + i*0x50] = (patch->ALL[item->addr_l + i*0x50] & 0xc0) | (value & 0x3f);
    } break;

    case P_M_OSC_BL_4U: {
      for(i=0; i<2; ++i)
	if( voice_sel & (1 << i) )
	  patch->ALL[item->addr_l + i*0x50] = (patch->ALL[item->addr_l + i*0x50] & 0x0f) | ((value << 4) & 0xf0);
    } break;

    case P_M_OSC_BL_PB: {
      for(i=0; i<2; ++i)
	if( voice_sel & (1 << i) ) {
	  sid_se_voice[sid][i*3+0].mv->pitchbender = value;
	  sid_se_voice[sid][i*3+1].mv->pitchbender = value;
	  sid_se_voice[sid][i*3+2].mv->pitchbender = value;
	}
    } break;

    case P_M_OSC_BL_FIL12: {
      for(i=0; i<2; ++i)
	if( voice_sel & (1 << i) ) {
	  patch->ALL[item->addr_l + i*6 + 0] = value;
	  patch->ALL[item->addr_l + i*6 + 1] = (patch->ALL[item->addr_l + i*6 + 1] & 0xf0) | ((value >> 8) & 0x0f);
	}
    } break;

    case P_M_OSC_BL_FIL8: {
      for(i=0; i<2; ++i)
	if( voice_sel & (1 << i) )
	  patch->ALL[item->addr_l + i*6 + 0] = value;
    } break;

    case P_M_DRM_8: {
      // TODO  
    } break;

    case P_M_DRM_PM8: {
      // TODO  
    } break;

    case P_M_DRM_4U: {
      // TODO  
    } break;
  
    case P_M_DRM_4L: {
      // TODO
    } break;
  
    case P_M_NOTE_INS: {
      // TODO
    } break;
  
    case P_M_NOTE: {
      // do nothing if hold note (0x01) is played
      if( value != 0x01 ) {
	for(i=0; i<6; ++i)
	  if( voice_sel & (1 << i) ) {
	    sid_se_voice_t *v = (sid_se_voice_t *)&sid_se_voice[sid][i];

	    // clear gate bit if note value is 0
	    if( value == 0 ) {
	      if( v->state.GATE_ACTIVE ) {
		v->state.GATE_SET_REQ = 0;
		v->state.GATE_CLR_REQ = 1;

		// propagate to trigger matrix
		v->trg_dst[0] |= v->trg_mask_note_off[0] & 0xc0; // gates handled separately
		v->trg_dst[1] |= v->trg_mask_note_off[1];
		//v->trg_dst[2] |= v->trg_mask_note_off[2]; // no wavetable events - wouldn't make sense here!
	      }
	    } else {
	      // TODO: hold mode (via trigger matrix?)

	      // set gate bit if voice active and gate not already active
	      if( v->state.VOICE_ACTIVE && !v->state.GATE_ACTIVE ) {
		v->state.GATE_SET_REQ = 1;

		// propagate to trigger matrix
		v->trg_dst[0] |= v->trg_mask_note_on[0] & 0xc0; // gates handled separately
		v->trg_dst[1] |= v->trg_mask_note_on[1];
		//v->trg_dst[2] |= v->trg_mask_note_on[2]; // no wavetable events - wouldn't make sense here!
	      }

	      // set new note
	      // if >= 0x7c, play arpeggiator note
	      if( value >= 0x7c ) {
		u8 arp_note = v->mv->wt_stack[value & 3] & 0x7f;
		if( !arp_note )
		  arp_note = v->mv->wt_stack[0] & 0x7f;

		if( arp_note ) {
		  v->note = arp_note;
		  v->state.PORTA_ACTIVE = 1; // will be cleared automatically if no portamento enabled
		} else {
		  // no note played: request note off if gate active
		  if( v->state.GATE_ACTIVE ) {
		    v->state.GATE_SET_REQ = 0;
		    v->state.GATE_CLR_REQ = 1;

		    // propagate to trigger matrix
		    v->trg_dst[0] |= v->trg_mask_note_off[0] & 0xc0; // gates handled separately
		    v->trg_dst[1] |= v->trg_mask_note_off[1];
		    //v->trg_dst[2] |= v->trg_mask_note_off[2]; // no wavetable events - wouldn't make sense here!
		  }
		}
	      } else {
		v->note = value;
		v->state.PORTA_ACTIVE = 1; // will be cleared automatically if no portamento enabled
	      }
	    }
	  }
      }
    } break;
  
    default:
      return -1; // unsupported mod function
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Sets a parameter value from 16bit value
// sid selects the SID
// par selects the parameter (0..255)
// value contains 16bit scaled value (0..65535)
// sidlr selects left/right SID channel
// ins selects current bassline/multi/drum instrument (1..3/0..5/0..15)
/////////////////////////////////////////////////////////////////////////////
s32 SID_PAR_Set16(u8 sid, u8 par, u16 value, u8 sidlr, u8 ins)
{
  sid_se_engine_t engine = sid_patch[sid].engine;
  if( engine >= SID_SE_NUM_ENGINES )
    engine = SID_SE_LEAD;
  sid_par_table_item_t *item = (sid_par_table_item_t *)&sid_par_table[engine][par];

  return SID_PAR_Set(sid, par, SID_PAR_Scale(item, value, 0), sidlr, ins);
}


/////////////////////////////////////////////////////////////////////////////
// Sets a parameter value from NRPN value
// sid selects the SID
// addr_[lsb|msb] selects parameter
// data_[lsb|msb] contains value
// value contains 8bit scaled value (0..255)
// sidlr selects left/right SID channel
// ins selects multi/drum instrument
/////////////////////////////////////////////////////////////////////////////
s32 SID_PAR_SetNRPN(u8 sid, u8 addr_lsb, u8 addr_msb, u8 data_lsb, u8 data_msb, u8 sidlr, u8 ins)
{
  // TODO

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// Sets a parameter value from WT handler
// sid selects the SID
// par selects the parameter (0..255)
// wt_value contains WT table value (0..127: relative change, 128..255: absolute change)
// sidlr selects left/right SID channel
// ins selects current bassline/multi/drum instrument (1..3/0..5/0..15)
/////////////////////////////////////////////////////////////////////////////
s32 SID_PAR_SetWT(u8 sid, u8 par, u8 wt_value, u8 sidlr, u8 ins)
{
  sid_se_engine_t engine = sid_patch[sid].engine;
  if( engine >= SID_SE_NUM_ENGINES )
    engine = SID_SE_LEAD;
  sid_par_table_item_t *item = (sid_par_table_item_t *)&sid_par_table[engine][par];

  // branch depending on relative (bit 7 cleared) or absolute value (bit 7 set)
  int par_value;
  if( (wt_value & (1 << 7)) == 0 ) { // relative
    // convert signed 7bit to signed 16bit
    int diff = ((int)wt_value << 9) - 0x8000;
    // do nothing if value is 0 (for compatibility with old presets, e.g. "A077: Analog Dream 1")
    if( diff == 0 )
      return 0;

    // get current value
    par_value = SID_PAR_Get(sid, par, sidlr, ins, 0); // from variable patch
    // scale value to 16bit
    par_value = SID_PAR_Scale(item, par_value, 1); // scale up
    // add signed wt value to parameter value and saturate
    par_value += diff;
    if( par_value < 0 ) par_value = 0; else if( par_value > 0xffff ) par_value = 0xffff;
  } else { // absolute
    // 7bit -> 16bit value
    par_value = (wt_value & 0x7f) << 9;
  }

  // perform write operation
  return SID_PAR_Set16(sid, par, par_value, sidlr, ins);
}


/////////////////////////////////////////////////////////////////////////////
// Returns a parameter value
// sid selects the SID
// par selects the parameter (0..255)
// sidlr selects left/right SID channel (if 0, 1 or 3, SIDL register will be returned, with 2 the SIDR register)
// ins selects current bassline/multi/drum instrument (1..3/0..5/0..15)
// shadow if 0, value taken from variable patch, if 1, value taken from constant patch
/////////////////////////////////////////////////////////////////////////////
u16 SID_PAR_Get(u8 sid, u8 par, u8 sidlr, u8 ins, u8 shadow)
{
  int i;
  sid_se_engine_t engine = sid_patch[sid].engine;
  if( engine >= SID_SE_NUM_ENGINES )
    engine = SID_SE_LEAD;
  sid_par_table_item_t *item = (sid_par_table_item_t *)&sid_par_table[engine][par];

  sid_patch_t *patch = shadow ? (sid_patch_t *)&sid_patch_shadow[sid] : (sid_patch_t *)&sid_patch[sid];

  // help variables used by voice parameters
  u8 voice = 0; // select first voice by default
  switch( engine ) {
    case SID_SE_LEAD:
      if( (par & 3) > 0)
	voice = (par&3) - 1;
      if( !(sidlr & 1) ) // if first voice not selected
	voice += 3;
      break;

    case SID_SE_BASSLINE:
      switch( par & 3 ) {
        case 1: voice = (sidlr & 1) ? 0 : 1; break; // selected voice, first voice has higher priority
        case 2: voice = 0; break; // first voice
        case 3: voice = 1; break; // second voice
        default: voice = 0; // first voice
      }
      break;

    default:
      return -1; // engine not supported yet
  }

  switch( item->mod_function ) {
    case P_M_NOP: {
      return 0; // nothing to do
    } break;
  
    case P_M_7: {
      return patch->ALL[item->addr_l] & 0x7f;
    } break;
  
    case P_M_8:
    case P_M_PM8:
    case P_M_LFO_PM8:
    case P_M_LFO_8:
    case P_M_ENV_PM8:
    case P_M_ENV_8: {
      return patch->ALL[item->addr_l];
    } break;
  
    case P_M_4L: {
      return patch->ALL[item->addr_l] & 0x0f;
    } break;
  
    case P_M_4U:
    case P_M_LFO_4U: {
      return (patch->ALL[item->addr_l] >> 4) & 0x0f;
    } break;
  
    case P_M_PAR12: {
      u16 value16 = patch->ALL[item->addr_l + 0] | ((u16)patch->ALL[item->addr_l + 1] << 8);
      return value16 >> 4;
    } break;
  
    case P_M_CUSTOM_SW: {
      return (patch->ALL[item->addr_l] & (1 << (par & 7))) ? 1 : 0;
    } break;
  
    case P_M_FIL4L: {
      for(i=0; i<2; ++i)
	if( sidlr & (1 << i) )
	  return patch->ALL[item->addr_l + i*6] & 0x0f;
    } break;
  
    case P_M_FIL4U: {
      for(i=0; i<2; ++i)
	if( sidlr & (1 << i) )
	  return (patch->ALL[item->addr_l + i*6] >> 4) & 0x0f;
    } break;
  
    case P_M_FIL12: {
      for(i=0; i<2; ++i)
	if( sidlr & (1 << i) ) {
	  u16 value = patch->ALL[item->addr_l + i*6 + 0] | ((u16)patch->ALL[item->addr_l + i*6 + 1] << 8);
	  return value & 0xfff;
	}
    } break;
  
    case P_M_FIL12_DIRECT: {
      u16 value = patch->ALL[item->addr_l + 0] | ((u16)patch->ALL[item->addr_l + 1] << 8);
      return value & 0xfff;
    } break;
  
    case P_M_FIL8: {
      for(i=0; i<2; ++i)
	if( sidlr & (1 << i) )
	  return patch->ALL[item->addr_l + i*6 + 0];
    } break;
  
    case P_M_OSC123_PM7:
    case P_M_OSC123_PM8:
    case P_M_OSC123_7:
    case P_M_OSC123_8: {
      return patch->ALL[item->addr_l + voice*0x10];
    } break;
  
    case P_M_OSC123_12: {
      u16 value = patch->ALL[item->addr_l + voice*0x10 + 0] | ((u16)patch->ALL[item->addr_l + voice*0x10 + 1] << 8);
      return value & 0xfff;
    } break;
  
    case P_M_OSC123_4L: {
      return patch->ALL[item->addr_l + voice*0x10] & 0x0f;
    } break;
  
    case P_M_OSC123_5L: {
      return patch->ALL[item->addr_l + voice*0x10] & 0x1f;
    } break;
  
    case P_M_OSC123_6L: {
      return patch->ALL[item->addr_l + voice*0x10] & 0x3f;
    } break;
  
    case P_M_OSC123_4U: {
      return (patch->ALL[item->addr_l + voice*0x10] >> 4) & 0x0f;
    } break;
  
    case P_M_OSC123_PB: {
      return sid_se_voice[sid][voice].mv->pitchbender;
    } break;
  
    case P_M_MOD_PM8: {
      return patch->ALL[0x100 | item->addr_l];
    } break;
  
    case P_M_MOD_B76: {
      return patch->ALL[0x100 | item->addr_l] >> 6;
    } break;
  
    case P_M_WT_6: {
      return patch->ALL[0x100 | item->addr_l] & 0x3f;
    } break;
  
    case P_M_WT_7: {
      return patch->ALL[0x100 | item->addr_l] & 0x7f;
    } break;
  
    case P_M_WT_POS: {
      return sid_se_wt[sid][par&3].pos;
    } break;
  
    case P_M_OSC_INS_PM7: {
      // TODO
    } break;
  
    case P_M_OSC_INS_PM8: {
      // TODO
    } break;
  
    case P_M_OSC_INS_7: {
      // TODO
    } break;
  
    case P_M_OSC_INS_8: {
      // TODO
    } break;
  
    case P_M_OSC_INS_12: {
      // TODO
    } break;
  
    case P_M_OSC_INS_4L: {
      // TODO
    } break;
  
    case P_M_OSC_INS_5L: {
      // TODO
    } break;
  
    case P_M_OSC_INS_6L: {
      // TODO
    } break;

    case P_M_OSC_INS_4U: {
      // TODO  
    } break;

    case P_M_OSC_INS_PB: {
      // TODO  
    } break;

    case P_M_OSC_BL_PM7:
    case P_M_OSC_BL_PM8:
    case P_M_OSC_BL_7:
    case P_M_OSC_BL_8: {
      return patch->ALL[item->addr_l + voice*0x50];
    } break;

    case P_M_OSC_BL_P8: {
      return patch->ALL[item->addr_l + voice*0x50] - 0x80;
    } break;

    case P_M_OSC_BL_12: {
      u16 value = patch->ALL[item->addr_l + voice*0x50 + 0] | ((u16)patch->ALL[item->addr_l + voice*0x50 + 1] << 8);
      return value & 0xfff;
    } break;

    case P_M_OSC_BL_4L: {
      return patch->ALL[item->addr_l + voice*0x50] & 0x0f;
    } break;

    case P_M_OSC_BL_5L: {
      return patch->ALL[item->addr_l + voice*0x50] & 0x1f;
    } break;

    case P_M_OSC_BL_6L: {
      return patch->ALL[item->addr_l + voice*0x50] & 0x3f;
    } break;

    case P_M_OSC_BL_4U: {
      return (patch->ALL[item->addr_l + voice*0x50] >> 4) & 0x0f;
    } break;

    case P_M_OSC_BL_PB: {
      return sid_se_voice[sid][voice*3].mv->pitchbender;
    } break;

    case P_M_OSC_BL_FIL12: {
      u16 value = patch->ALL[item->addr_l + voice*6 + 0] | ((u16)patch->ALL[item->addr_l + voice*6 + 1] << 8);
      return value & 0xfff;
    } break;

    case P_M_OSC_BL_FIL8: {
      return patch->ALL[item->addr_l + voice*6 + 0];
    } break;

    case P_M_DRM_8: {
      // TODO  
    } break;

    case P_M_DRM_PM8: {
      // TODO  
    } break;

    case P_M_DRM_4U: {
      // TODO  
    } break;
  
    case P_M_DRM_4L: {
      // TODO
    } break;
  
    case P_M_NOTE_INS: {
      // TODO
    } break;
  
    case P_M_NOTE: {
      return sid_se_voice[sid][voice].note;
    } break;
  
    default:
      return 0; // unsupported mod function - return 0
  }

  return 0; // no error
}
