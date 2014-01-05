// $Id$
/*
 * Local SCS Configuration
 *
 * ==========================================================================
 *
 *  Copyright (C) 2011 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

/////////////////////////////////////////////////////////////////////////////
// Include files
/////////////////////////////////////////////////////////////////////////////

#include <mios32.h>
#include <string.h>
#include "tasks.h"

#include <scs.h>
#include "scs_config.h"

#include <file.h>

#include <app.h>
#include <MbCvEnvironment.h>
#include <MbCvTables.h>

#include <uip.h>
#include "uip_task.h"
#include "osc_client.h"
#include "osc_server.h"

#include <midi_port.h>
#include <midi_router.h>

#include "mbcv_map.h"
#include "mbcv_file.h"
#include "mbcv_file_p.h"
#include "mbcv_patch.h"
#include "mbcv_lre.h"


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

static u8 extraPage;

static u8 selectedCv;
static u8 selectedLfo;
static u8 selectedEnv1; // currently only 1, just to be prepared...
static u8 selectedEnv2; // currently only 1, just to be prepared...
static u8 selectedRouterNode;
static u8 selectedIpPar;
static u8 selectedOscPort;
static u8 monPageOffset;

// we need a reference to this environment very often
static MbCvEnvironment* env;


/////////////////////////////////////////////////////////////////////////////
// String Conversion Functions
/////////////////////////////////////////////////////////////////////////////
static void stringEmpty(u32 ix, u16 value, char *label)   { label[0] = 0; }
static void stringDec(u32 ix, u16 value, char *label)    { sprintf(label, "%3d  ", value); }
static void stringDecP1(u32 ix, u16 value, char *label)  { sprintf(label, "%3d  ", value+1); }
//static void stringDecPM(u32 ix, u16 value, char *label)  { sprintf(label, "%3d  ", (int)value - 64); }
static void stringDecPM128(u32 ix, u16 value, char *label)  { sprintf(label, "%3d  ", (int)value - 128); }
static void stringDecPercent(u32 ix, u16 value, char *label)  { sprintf(label, "%3d%% ", value); }
//static void stringDec03(u32 ix, u16 value, char *label)  { sprintf(label, "%03d  ", value); }
static void stringDec0Dis(u32 ix, u16 value, char *label){ sprintf(label, value ? "%3d  " : "---  ", value); }
static void stringDec4(u32 ix, u16 value, char *label)   { sprintf(label, "%4d ", value); }
static void stringDec5(u32 ix, u16 value, char *label)   { sprintf(label, "%5d", value); }
//static void stringHex2(u32 ix, u16 value, char *label)    { sprintf(label, " %02X  ", value); }
//static void stringHex2O80(u32 ix, u16 value, char *label) { sprintf(label, " %02X  ", value | 0x80); }
static void stringOnOff(u32 ix, u16 value, char *label)  { sprintf(label, " [%c] ", value ? 'x' : ' '); }
static void stringCurve(u32 ix, u16 value, char *label)  { sprintf(label, value ? "Exp. " : "Lin. "); }

static void stringCvUpdateRate(u32 ix, u16 value, char *label) {
  u32 updateRate = (value+1) * 500; // Hz
  if( value < 10000 ) {
    sprintf(label, "%d.%dk", updateRate / 1000, (updateRate % 1000) / 100);
  } else {
    sprintf(label, "%3dk", updateRate / 1000);
  }
}


static void stringNote(u32 ix, u16 value, char *label)
{
  const char noteTab[12][3] = { "C-", "C#", "D-", "D#", "E-", "F-", "F#", "G-", "G#", "A-", "A#", "B-" };

  // print "---" if note number is 0
  if( value == 0 )
    sprintf(label, "---  ");
  else {
    u8 octave = value / 12;
    u8 note = value % 12;

    // print semitone and octave (-2): up to 4 chars
    sprintf(label, "%s%d  ",
	    noteTab[note],
	    (int)octave-2);
  }
}

static void stringScope(u32 ix, u16 value, char *label)
{
  if( value == 0 ) {
    sprintf(label, "CV- ");
  } else {
    sprintf(label, "CV%d ", value);
  }
}

static void stringAoutIf(u32 ix, u16 value, char *label)
{
  char *name = (char *)MBCV_MAP_IfNameGet(MBCV_MAP_IfGet()); // 8 chars max
  if( ix == 0 ) {
    label[0] = ' ';
    memcpy(label+1, (char *)name, 4);
  } else {
    memcpy(label, (char *)(name + 4), 5);
  }
  label[5] = 0;
}

static void stringCvMode(u32 ix, u16 value, char *label)
{
  static const char mode[MBCV_MIDI_EVENT_MODE_NUM+1][6] = {
    "Note ",
    "Vel. ",
    "Aft. ",
    " CC  ",
    "NRPN ",
    "PBnd ",
    "CMin ",
    "CMid ",
    "CMax ",
    "???? ",
  };

  sprintf(label, mode[(value < MBCV_MIDI_EVENT_MODE_NUM) ? value : MBCV_MIDI_EVENT_MODE_NUM]);
}

static void stringCvPlayMode(u32 ix, u16 value, char *label)
{
  static const char playMode[4][6] = {
    "Mono ",
    "Leg. ",
    "Poly ",
    "???? ",
  };

  sprintf(label, playMode[value&3]);
}

static void stringCvTranspose(u32 ix, u16 value, char *label) { if( value >= 8 ) sprintf(label, " +%d  ", value-8); else sprintf(label, " -%d  ", 8-value); }

static void stringCvPortamentoMode(u32 ix, u16 value, char *label)
{
  static const char portamentoMode[4][6] = {
    "Norm ",
    "CGld ",
    "Glis ",
    "???? ",
  };

  sprintf(label, portamentoMode[value&3]);
}

static void stringCvCurve(u32 ix, u16 value, char *label) { memcpy(label, env->mbCv[selectedCv].mbCvVoice.getAoutCurveName(), 5); label[5] = 0; }
static void stringCvCaliMode(u32 ix, u16 value, char *label) { memcpy(label, MBCV_MAP_CaliNameGet(), 5); label[5] = 0; }

static void stringArpDir(u32 ix, u16 value, char *label)
{
  const char dirLabel[7][5] = { " Up ", "Down", "Up&D", "D&Up", "U&D2", "D&U2", "Rand" };
  if( value < 7 )
    strcpy(label, dirLabel[value]);
  else
    sprintf(label, "%3d ", value);
}

static void stringLfoRate(u32 ix, u16 value, char *label)
{
#if 0  
  sprintf(label, "%3d  ", value); 
#else
  // rate in Herz
  float hz = 500.0 / (65536.0 / (float)mbCvLfoTable[value]);
  if( env->mbCv[selectedCv].mbCvLfo[selectedLfo].lfoModeFast )
    hz *= env->updateSpeedFactor;

  if( hz < 1.0 ) {
    sprintf(label, ".%03d ", (int)(hz*1000));
  } else if( hz < 10.0 ) {
    sprintf(label, "%d.%02d ", (int)hz, (int)(hz*100) % 100);
  } else if( hz < 100.0 ) {
    sprintf(label, "%2d.%d ", (int)hz, (int)((hz*10)) % 10);
  } else {
    sprintf(label, "%4d ", (int)hz);
  }
#endif
}

static void stringLfoWave(u32 ix, u16 value, char *label)
{
  const char waveLabel[9][5] = {
    "Sine",
    "Tri.",
    "Saw.",
    "Puls",
    "Rnd.",
    "PSin",
    "PTri",
    "PSaw",
    "PPul",
  };

  if( value < 10 )
    strcpy(label, waveLabel[value]);
  else
    sprintf(label, "%3d ", value);
}

static void stringInPort(u32 ix, u16 value, char *label)
{
  sprintf(label, MIDI_PORT_InNameGet(value));
}

static void stringOutPort(u32 ix, u16 value, char *label)
{
  sprintf(label, MIDI_PORT_OutNameGet(value));
}

static void stringRouterChn(u32 ix, u16 value, char *label)
{
  if( value == 0 )
    sprintf(label, "---  ");
  else if( value == 17 )
    sprintf(label, "All  ");
  else
    sprintf(label, "%3d  ", value);
}

static void stringOscPort(u32 ix, u16 value, char *label)
{
  sprintf(label, "OSC%d", value+1);
}

static void stringOscMode(u32 ix, u16 value, char *label)
{
  sprintf(label, " %s", OSC_CLIENT_TransferModeShortNameGet(value));
}

static void stringOscModeFull(u32 ix, u16 value, char *line1, char *line2)
{
  // print CC parameter on full screen (2x20)
  sprintf(line1, "OSC%d Transfer Mode:", selectedOscPort+1);
  sprintf(line2, "%s", OSC_CLIENT_TransferModeFullNameGet(value));
}

static void stringIpPar(u32 ix, u16 value, char *label)
{
  const char ipParLabel[3][6] = { "Host", "Mask", "Gate" };
  strcpy(label, ipParLabel[(selectedIpPar < 3) ? selectedIpPar : 0]);
}

static void stringRemoteIp(u32 ix, u16 value, char *label)
{
  char buffer[16];
  u32 ip = OSC_SERVER_RemoteIP_Get(selectedOscPort);

  sprintf(buffer, "%3d.%3d.%3d.%3d",
	  (ip >> 24) & 0xff,
	  (ip >> 16) & 0xff,
	  (ip >>  8) & 0xff,
	  (ip >>  0) & 0xff);  

  memcpy(label, (char *)&buffer[ix*SCS_MENU_ITEM_WIDTH], SCS_MENU_ITEM_WIDTH);
  label[SCS_MENU_ITEM_WIDTH] = 0;
}

static void stringIp(u32 ix, u16 value, char *label)
{
  char buffer[16];

  // 3 items combined to a 15 char IP string
  if( UIP_TASK_DHCP_EnableGet() && !UIP_TASK_ServicesRunning() ) {
    sprintf(buffer, "???.???.???.???");
  } else {
    u32 ip = 0;
    switch( selectedIpPar ) {
    case 0: ip = UIP_TASK_IP_EffectiveAddressGet(); break;
    case 1: ip = UIP_TASK_EffectiveNetmaskGet(); break;
    case 2: ip = UIP_TASK_EffectiveGatewayGet(); break;
    }

    sprintf(buffer, "%3d.%3d.%3d.%3d",
	    (ip >> 24) & 0xff,
	    (ip >> 16) & 0xff,
	    (ip >>  8) & 0xff,
	    (ip >>  0) & 0xff);  
  }
  memcpy(label, (char *)&buffer[ix*SCS_MENU_ITEM_WIDTH], SCS_MENU_ITEM_WIDTH);
  label[SCS_MENU_ITEM_WIDTH] = 0;
}


/////////////////////////////////////////////////////////////////////////////
// Parameter Selection Functions
/////////////////////////////////////////////////////////////////////////////
static u16  selectNOP(u32 ix, u16 value)    { return value; }

static void selectSAVE_Callback(char *newString)
{
  s32 status;

  if( (status=MBCV_PATCH_StoreGlobal(newString)) < 0 ) {
    char buffer[100];
    sprintf(buffer, "Patch %s", newString);
    SCS_Msg(SCS_MSG_ERROR_L, 1000, "Failed to store", buffer);
  } else {
    char buffer[100];
    sprintf(buffer, "Patch %s", newString);
    SCS_Msg(SCS_MSG_L, 1000, buffer, "stored!");
  }
}
static u16  selectSAVE(u32 ix, u16 value)
{
  return SCS_InstallEditStringCallback(selectSAVE_Callback, "SAVE", mbcv_file_p_patch_name, MBCV_FILE_P_FILENAME_LEN);
}

static void selectLOAD_Callback(char *newString)
{
  s32 status;

  if( newString[0] != 0 ) {
    if( (status=MBCV_PATCH_LoadGlobal(newString)) < 0 ) {
      char buffer[100];
      sprintf(buffer, "Patch %s", newString);
      SCS_Msg(SCS_MSG_ERROR_L, 1000, "Failed to load", buffer);
    } else {
      char buffer[100];
      sprintf(buffer, "Patch %s", newString);
      SCS_Msg(SCS_MSG_L, 1000, buffer, "loaded!");
    }
  }
}
static u8 getListLOAD_Callback(u8 offset, char *line)
{
  MUTEX_SDCARD_TAKE;
  s32 status = FILE_GetFiles("/", "MIO", line, 2, offset);
  MUTEX_SDCARD_GIVE;
  if( status < 1 ) {
    sprintf(line, "<no .MIO files>");
    status = 0;
  }
  return status;
}
static u16  selectLOAD(u32 ix, u16 value)
{
  return SCS_InstallEditBrowserCallback(selectLOAD_Callback, getListLOAD_Callback, "LOAD", 9, 2);
}

static void selectRemoteIp_Callback(u32 newIp)
{
  OSC_SERVER_RemoteIP_Set(selectedOscPort, newIp);
  OSC_SERVER_Init(0);
}

static u16 selectRemoteIp(u32 ix, u16 value)
{
  u32 initialIp = OSC_SERVER_RemoteIP_Get(selectedOscPort);
  SCS_InstallEditIpCallback(selectRemoteIp_Callback, "IP:", initialIp);
  return value;
}

static void selectIpEnter_Callback(u32 newIp)
{
  switch( selectedIpPar ) {
  case 0: UIP_TASK_IP_AddressSet(newIp); break;
  case 1: UIP_TASK_NetmaskSet(newIp); break;
  case 2: UIP_TASK_GatewaySet(newIp); break;
  }
}

static u16 selectIpEnter(u32 ix, u16 value)
{
  const char headerString[3][6] = { "Host:", "Netm:", "Gate:" };

  if( selectedIpPar < 3 ) {
    u32 initialIp = 0;

    switch( selectedIpPar ) {
    case 0: initialIp = UIP_TASK_IP_AddressGet(); break;
    case 1: initialIp = UIP_TASK_NetmaskGet(); break;
    case 2: initialIp = UIP_TASK_GatewayGet(); break;
    }
    SCS_InstallEditIpCallback(selectIpEnter_Callback, (char *)headerString[selectedIpPar], initialIp);
  }
  return value;
}


/////////////////////////////////////////////////////////////////////////////
// Parameter Access Functions
/////////////////////////////////////////////////////////////////////////////
static u16  dummyGet(u32 ix)              { return 0; }
static void dummySet(u32 ix, u16 value)   { }

static u16  cvGet(u32 ix)              { return selectedCv; }
static void cvSet(u32 ix, u16 value)
{
  selectedCv = value;
  MBCV_MAP_CaliModeSet(selectedCv, MBCV_MAP_CaliModeGet()); // change calibration mode to new pin
}

static u16  cvChnGet(u32 ix)            { return env->mbCv[selectedCv].mbCvMidiVoice.midivoiceChannel; }
static void cvChnSet(u32 ix, u16 value) { env->mbCv[selectedCv].mbCvMidiVoice.midivoiceChannel = value; }

static u16  cvEventGet(u32 ix)            { return (u32)env->mbCv[selectedCv].mbCvVoice.voiceEventMode; }
static void cvEventSet(u32 ix, u16 value) { env->mbCv[selectedCv].mbCvVoice.voiceEventMode = (mbcv_midi_event_mode_t)value; }

static u16  cvPlayModeGet(u32 ix)
{
  return
    (env->mbCv[selectedCv].mbCvVoice.voiceLegato ? 1 : 0) |
    (env->mbCv[selectedCv].mbCvVoice.voicePoly ? 2 : 0);
}
static void cvPlayModeSet(u32 ix, u16 value)
{
  env->mbCv[selectedCv].mbCvVoice.voiceLegato = (value & 1) ? 1 : 0;
  env->mbCv[selectedCv].mbCvVoice.voicePoly = (value & 2) ? 1 : 0;
}

static u16  cvInvGateGet(u32 ix)            { return env->mbCv[selectedCv].mbCvVoice.voiceGateInverted; }
static void cvInvGateSet(u32 ix, u16 value) { env->mbCv[selectedCv].mbCvVoice.voiceGateInverted = value; }

static u16  cvSplitLowerGet(u32 ix)            { return env->mbCv[selectedCv].mbCvMidiVoice.midivoiceSplitLower; }
static void cvSplitLowerSet(u32 ix, u16 value) { env->mbCv[selectedCv].mbCvMidiVoice.midivoiceSplitLower = value; }

static u16  cvSplitUpperGet(u32 ix)            { return env->mbCv[selectedCv].mbCvMidiVoice.midivoiceSplitUpper; }
static void cvSplitUpperSet(u32 ix, u16 value) { env->mbCv[selectedCv].mbCvMidiVoice.midivoiceSplitUpper = value; }

static u16  cvPitchRangeGet(u32 ix)            { return env->mbCv[selectedCv].mbCvVoice.voicePitchrange; }
static void cvPitchRangeSet(u32 ix, u16 value) { env->mbCv[selectedCv].mbCvVoice.voicePitchrange = value; }

static u16  cvTranspOctGet(u32 ix)            { return env->mbCv[selectedCv].mbCvVoice.voiceTransposeOctave + 8; }
static void cvTranspOctSet(u32 ix, u16 value) { env->mbCv[selectedCv].mbCvVoice.voiceTransposeOctave = (int)value - 8; }

static u16  cvTranspSemiGet(u32 ix)            { return env->mbCv[selectedCv].mbCvVoice.voiceTransposeSemitone + 8; }
static void cvTranspSemiSet(u32 ix, u16 value) { env->mbCv[selectedCv].mbCvVoice.voiceTransposeSemitone = (int)value - 8; }

static u16  cvFinetuneGet(u32 ix)            { return env->mbCv[selectedCv].mbCvVoice.voiceFinetune + 128; }
static void cvFinetuneSet(u32 ix, u16 value) { env->mbCv[selectedCv].mbCvVoice.voiceFinetune = (int)value - 128; }

static u16  cvPortamentoGet(u32 ix)            { return env->mbCv[selectedCv].mbCvVoice.voicePortamentoRate; }
static void cvPortamentoSet(u32 ix, u16 value) { env->mbCv[selectedCv].mbCvVoice.voicePortamentoRate = value; }

static u16  cvPortamentoModeGet(u32 ix)            { return env->mbCv[selectedCv].mbCvVoice.getPortamentoMode(); }
static void cvPortamentoModeSet(u32 ix, u16 value) { env->mbCv[selectedCv].mbCvVoice.setPortamentoMode(value); }

static u16  cvSusKeyGet(u32 ix)            { return env->mbCv[selectedCv].mbCvVoice.voiceSusKey; }
static void cvSusKeySet(u32 ix, u16 value) { env->mbCv[selectedCv].mbCvVoice.voiceSusKey = value; }

static u16  cvCCGet(u32 ix)            { return env->mbCv[selectedCv].mbCvMidiVoice.midivoiceCCNumber; }
static void cvCCSet(u32 ix, u16 value) { env->mbCv[selectedCv].mbCvMidiVoice.midivoiceCCNumber = value; }

static u16  cvPortGet(u32 ix)
{
  return env->mbCv[selectedCv].mbCvMidiVoice.getPortEnabled(ix);
}
static void cvPortSet(u32 ix, u16 value)
{
  env->mbCv[selectedCv].mbCvMidiVoice.setPortEnabled(ix, value ? 1 : 0);
}


/////////////////////////////////////////////////////////////////////////////
// ARP
/////////////////////////////////////////////////////////////////////////////

static u16  arpOnGet(u32 ix)            { return env->mbCv[selectedCv].mbCvArp.arpEnabled; }
static void arpOnSet(u32 ix, u16 value) { env->mbCv[selectedCv].mbCvArp.arpEnabled = value; }

static u16  arpDirGet(u32 ix)            { return env->mbCv[selectedCv].mbCvArp.arpDirGet(); }
static void arpDirSet(u32 ix, u16 value) { env->mbCv[selectedCv].mbCvArp.arpDirSet(value); }

static u16  arpSpeedGet(u32 ix)            { return env->mbCv[selectedCv].mbCvArp.arpSpeed; }
static void arpSpeedSet(u32 ix, u16 value) { env->mbCv[selectedCv].mbCvArp.arpSpeed = value; }

static u16  arpGatelenGet(u32 ix)            { return env->mbCv[selectedCv].mbCvArp.arpGatelength; }
static void arpGatelenSet(u32 ix, u16 value) { env->mbCv[selectedCv].mbCvArp.arpGatelength = value; }

static u16  arpRangeGet(u32 ix)            { return env->mbCv[selectedCv].mbCvArp.arpOctaveRange; }
static void arpRangeSet(u32 ix, u16 value) { env->mbCv[selectedCv].mbCvArp.arpOctaveRange = value; }

static u16  arpSortGet(u32 ix)            { return env->mbCv[selectedCv].mbCvArp.arpSortedNotes; }
static void arpSortSet(u32 ix, u16 value) { env->mbCv[selectedCv].mbCvArp.arpSortedNotes = value; }

static u16  arpHoldGet(u32 ix)            { return env->mbCv[selectedCv].mbCvArp.arpHoldMode; }
static void arpHoldSet(u32 ix, u16 value) { env->mbCv[selectedCv].mbCvArp.arpHoldMode = value; }

static u16  arpSyncGet(u32 ix)            { return env->mbCv[selectedCv].mbCvArp.arpSyncMode; }
static void arpSyncSet(u32 ix, u16 value) { env->mbCv[selectedCv].mbCvArp.arpSyncMode = value; }

static u16  arpCACGet(u32 ix)            { return env->mbCv[selectedCv].mbCvArp.arpConstantCycle; }
static void arpCACSet(u32 ix, u16 value) { env->mbCv[selectedCv].mbCvArp.arpConstantCycle = value; }

static u16  arpOneshotGet(u32 ix)            { return env->mbCv[selectedCv].mbCvArp.arpOneshotMode; }
static void arpOneshotSet(u32 ix, u16 value) { env->mbCv[selectedCv].mbCvArp.arpOneshotMode = value; }

static u16  arpEasyChordGet(u32 ix)            { return env->mbCv[selectedCv].mbCvArp.arpEasyChordMode; }
static void arpEasyChordSet(u32 ix, u16 value) { env->mbCv[selectedCv].mbCvArp.arpEasyChordMode = value; }


/////////////////////////////////////////////////////////////////////////////
// LFO
/////////////////////////////////////////////////////////////////////////////

static u16  lfoGet(u32 ix)            { return selectedLfo; }
static void lfoSet(u32 ix, u16 value) { selectedLfo = value; }

static u16  lfoWaveGet(u32 ix)
{
  return env->mbCv[selectedCv].mbCvLfo[selectedLfo].lfoWaveform;
}
static void lfoWaveSet(u32 ix, u16 value)
{
  env->mbCv[selectedCv].mbCvLfo[selectedLfo].lfoWaveform = value;
}

static u16  lfoAmplitudeGet(u32 ix)            { return env->mbCv[selectedCv].mbCvLfo[selectedLfo].lfoAmplitude + 128; }
static void lfoAmplitudeSet(u32 ix, u16 value) { env->mbCv[selectedCv].mbCvLfo[selectedLfo].lfoAmplitude = value - 128; }

static u16  lfoRateGet(u32 ix)            { return env->mbCv[selectedCv].mbCvLfo[selectedLfo].lfoRate; }
static void lfoRateSet(u32 ix, u16 value) { env->mbCv[selectedCv].mbCvLfo[selectedLfo].lfoRate = value; }

static u16  lfoClkSyncGet(u32 ix)            { return env->mbCv[selectedCv].mbCvLfo[selectedLfo].lfoModeClkSync; }
static void lfoClkSyncSet(u32 ix, u16 value) { env->mbCv[selectedCv].mbCvLfo[selectedLfo].lfoModeClkSync = value; }

static u16  lfoKeySyncGet(u32 ix)            { return env->mbCv[selectedCv].mbCvLfo[selectedLfo].lfoModeKeySync; }
static void lfoKeySyncSet(u32 ix, u16 value) { env->mbCv[selectedCv].mbCvLfo[selectedLfo].lfoModeKeySync = value; }

static u16  lfoOneshotGet(u32 ix)            { return env->mbCv[selectedCv].mbCvLfo[selectedLfo].lfoModeOneshot; }
static void lfoOneshotSet(u32 ix, u16 value) { env->mbCv[selectedCv].mbCvLfo[selectedLfo].lfoModeOneshot = value; }

static u16  lfoModeFastGet(u32 ix)            { return env->mbCv[selectedCv].mbCvLfo[selectedLfo].lfoModeFast; }
static void lfoModeFastSet(u32 ix, u16 value) { env->mbCv[selectedCv].mbCvLfo[selectedLfo].lfoModeFast = value; }

static u16  lfoDelayGet(u32 ix)            { return env->mbCv[selectedCv].mbCvLfo[selectedLfo].lfoDelay; }
static void lfoDelaySet(u32 ix, u16 value) { env->mbCv[selectedCv].mbCvLfo[selectedLfo].lfoDelay = value; }

static u16  lfoPhaseGet(u32 ix)            { return env->mbCv[selectedCv].mbCvLfo[selectedLfo].lfoPhase; }
static void lfoPhaseSet(u32 ix, u16 value) { env->mbCv[selectedCv].mbCvLfo[selectedLfo].lfoPhase = value; }

static u16  lfoDepthCvGet(u32 ix)            { return env->mbCv[selectedCv].mbCvLfo[selectedLfo].lfoDepthPitch + 128; }
static void lfoDepthCvSet(u32 ix, u16 value) { env->mbCv[selectedCv].mbCvLfo[selectedLfo].lfoDepthPitch = value - 128; }

static u16  lfoDepthLfoAmpGet(u32 ix)            { return env->mbCv[selectedCv].mbCvLfo[selectedLfo].lfoDepthLfoAmplitude + 128; }
static void lfoDepthLfoAmpSet(u32 ix, u16 value) { env->mbCv[selectedCv].mbCvLfo[selectedLfo].lfoDepthLfoAmplitude = value - 128; }

static u16  lfoDepthLfoRateGet(u32 ix)            { return env->mbCv[selectedCv].mbCvLfo[selectedLfo].lfoDepthLfoRate + 128; }
static void lfoDepthLfoRateSet(u32 ix, u16 value) { env->mbCv[selectedCv].mbCvLfo[selectedLfo].lfoDepthLfoRate = value - 128; }

static u16  lfoDepthEnv1RateGet(u32 ix)            { return env->mbCv[selectedCv].mbCvLfo[selectedLfo].lfoDepthEnv1Rate + 128; }
static void lfoDepthEnv1RateSet(u32 ix, u16 value) { env->mbCv[selectedCv].mbCvLfo[selectedLfo].lfoDepthEnv1Rate = value - 128; }

static u16  lfoDepthEnv2RateGet(u32 ix)            { return env->mbCv[selectedCv].mbCvLfo[selectedLfo].lfoDepthEnv2Rate + 128; }
static void lfoDepthEnv2RateSet(u32 ix, u16 value) { env->mbCv[selectedCv].mbCvLfo[selectedLfo].lfoDepthEnv2Rate = value - 128; }


/////////////////////////////////////////////////////////////////////////////
// ENV1
/////////////////////////////////////////////////////////////////////////////

static u16  env1AmplitudeGet(u32 ix)            { return env->mbCv[selectedCv].mbCvEnv1[selectedEnv1].envAmplitude + 128; }
static void env1AmplitudeSet(u32 ix, u16 value) { env->mbCv[selectedCv].mbCvEnv1[selectedEnv1].envAmplitude = value - 128; }

static u16  env1DelayGet(u32 ix)            { return env->mbCv[selectedCv].mbCvEnv1[selectedEnv1].envDelay; }
static void env1DelaySet(u32 ix, u16 value) { env->mbCv[selectedCv].mbCvEnv1[selectedEnv1].envDelay = value; }

static u16  env1AttackGet(u32 ix)            { return env->mbCv[selectedCv].mbCvEnv1[selectedEnv1].envAttack; }
static void env1AttackSet(u32 ix, u16 value) { env->mbCv[selectedCv].mbCvEnv1[selectedEnv1].envAttack = value; }

static u16  env1DecayGet(u32 ix)            { return env->mbCv[selectedCv].mbCvEnv1[selectedEnv1].envDecay; }
static void env1DecaySet(u32 ix, u16 value) { env->mbCv[selectedCv].mbCvEnv1[selectedEnv1].envDecay = value; }

static u16  env1SustainGet(u32 ix)            { return env->mbCv[selectedCv].mbCvEnv1[selectedEnv1].envSustain; }
static void env1SustainSet(u32 ix, u16 value) { env->mbCv[selectedCv].mbCvEnv1[selectedEnv1].envSustain = value; }

static u16  env1ReleaseGet(u32 ix)            { return env->mbCv[selectedCv].mbCvEnv1[selectedEnv1].envRelease; }
static void env1ReleaseSet(u32 ix, u16 value) { env->mbCv[selectedCv].mbCvEnv1[selectedEnv1].envRelease = value; }

static u16  env1CurveGet(u32 ix)            { return env->mbCv[selectedCv].mbCvEnv1[selectedEnv1].envCurve; }
static void env1CurveSet(u32 ix, u16 value) { env->mbCv[selectedCv].mbCvEnv1[selectedEnv1].envCurve = value; }

static u16  env1ClkSyncGet(u32 ix)            { return env->mbCv[selectedCv].mbCvEnv1[selectedEnv1].envModeClkSync; }
static void env1ClkSyncSet(u32 ix, u16 value) { env->mbCv[selectedCv].mbCvEnv1[selectedEnv1].envModeClkSync = value; }

static u16  env1KeySyncGet(u32 ix)            { return env->mbCv[selectedCv].mbCvEnv1[selectedEnv1].envModeKeySync; }
static void env1KeySyncSet(u32 ix, u16 value) { env->mbCv[selectedCv].mbCvEnv1[selectedEnv1].envModeKeySync = value; }

static u16  env1OneshotGet(u32 ix)            { return env->mbCv[selectedCv].mbCvEnv1[selectedEnv1].envModeOneshot; }
static void env1OneshotSet(u32 ix, u16 value) { env->mbCv[selectedCv].mbCvEnv1[selectedEnv1].envModeOneshot = value; }

static u16  env1ModeFastGet(u32 ix)            { return env->mbCv[selectedCv].mbCvEnv1[selectedEnv1].envModeFast; }
static void env1ModeFastSet(u32 ix, u16 value) { env->mbCv[selectedCv].mbCvEnv1[selectedEnv1].envModeFast = value; }

static u16  env1DepthCvGet(u32 ix)            { return env->mbCv[selectedCv].mbCvEnv1[selectedEnv1].envDepthPitch + 128; }
static void env1DepthCvSet(u32 ix, u16 value) { env->mbCv[selectedCv].mbCvEnv1[selectedEnv1].envDepthPitch = value - 128; }

static u16  env1DepthLfo1AmpGet(u32 ix)            { return env->mbCv[selectedCv].mbCvEnv1[selectedEnv1].envDepthLfo1Amplitude + 128; }
static void env1DepthLfo1AmpSet(u32 ix, u16 value) { env->mbCv[selectedCv].mbCvEnv1[selectedEnv1].envDepthLfo1Amplitude = value - 128; }

static u16  env1DepthLfo1RateGet(u32 ix)            { return env->mbCv[selectedCv].mbCvEnv1[selectedEnv1].envDepthLfo1Rate + 128; }
static void env1DepthLfo1RateSet(u32 ix, u16 value) { env->mbCv[selectedCv].mbCvEnv1[selectedEnv1].envDepthLfo1Rate = value - 128; }

static u16  env1DepthLfo2AmpGet(u32 ix)            { return env->mbCv[selectedCv].mbCvEnv1[selectedEnv1].envDepthLfo2Amplitude + 128; }
static void env1DepthLfo2AmpSet(u32 ix, u16 value) { env->mbCv[selectedCv].mbCvEnv1[selectedEnv1].envDepthLfo2Amplitude = value - 128; }

static u16  env1DepthLfo2RateGet(u32 ix)            { return env->mbCv[selectedCv].mbCvEnv1[selectedEnv1].envDepthLfo2Rate + 128; }
static void env1DepthLfo2RateSet(u32 ix, u16 value) { env->mbCv[selectedCv].mbCvEnv1[selectedEnv1].envDepthLfo2Rate = value - 128; }


/////////////////////////////////////////////////////////////////////////////
// ENV2
/////////////////////////////////////////////////////////////////////////////

static u16  env2AmplitudeGet(u32 ix)            { return env->mbCv[selectedCv].mbCvEnv2[selectedEnv2].envAmplitude + 128; }
static void env2AmplitudeSet(u32 ix, u16 value) { env->mbCv[selectedCv].mbCvEnv2[selectedEnv2].envAmplitude = value - 128; }

static u16  env2OffsetGet(u32 ix)            { return env->mbCv[selectedCv].mbCvEnv2[selectedEnv2].envOffset + 128; }
static void env2OffsetSet(u32 ix, u16 value) { env->mbCv[selectedCv].mbCvEnv2[selectedEnv2].envOffset = value - 128; }

static u16  env2RateGet(u32 ix)            { return env->mbCv[selectedCv].mbCvEnv2[selectedEnv2].envRate; }
static void env2RateSet(u32 ix, u16 value) { env->mbCv[selectedCv].mbCvEnv2[selectedEnv2].envRate = value; }

static u16  env2CurveGet(u32 ix)            { return env->mbCv[selectedCv].mbCvEnv2[selectedEnv2].envCurve; }
static void env2CurveSet(u32 ix, u16 value) { env->mbCv[selectedCv].mbCvEnv2[selectedEnv2].envCurve = value; }

static u16  env2LastStepGet(u32 ix)            { return env->mbCv[selectedCv].mbCvEnv2[selectedEnv2].envLastStep; }
static void env2LastStepSet(u32 ix, u16 value) { env->mbCv[selectedCv].mbCvEnv2[selectedEnv2].envLastStep = value; }

static u16  env2LoopAttackGet(u32 ix)            { return env->mbCv[selectedCv].mbCvEnv2[selectedEnv2].envLoopAttack; }
static void env2LoopAttackSet(u32 ix, u16 value) { env->mbCv[selectedCv].mbCvEnv2[selectedEnv2].envLoopAttack = value; }

static u16  env2SustainStepGet(u32 ix)            { return env->mbCv[selectedCv].mbCvEnv2[selectedEnv2].envSustainStep; }
static void env2SustainStepSet(u32 ix, u16 value) { env->mbCv[selectedCv].mbCvEnv2[selectedEnv2].envSustainStep = value; }

static u16  env2LoopReleaseGet(u32 ix)            { return env->mbCv[selectedCv].mbCvEnv2[selectedEnv2].envLoopRelease; }
static void env2LoopReleaseSet(u32 ix, u16 value) { env->mbCv[selectedCv].mbCvEnv2[selectedEnv2].envLoopRelease = value; }

static u16  env2ClkSyncGet(u32 ix)            { return env->mbCv[selectedCv].mbCvEnv2[selectedEnv2].envModeClkSync; }
static void env2ClkSyncSet(u32 ix, u16 value) { env->mbCv[selectedCv].mbCvEnv2[selectedEnv2].envModeClkSync = value; }

static u16  env2KeySyncGet(u32 ix)            { return env->mbCv[selectedCv].mbCvEnv2[selectedEnv2].envModeKeySync; }
static void env2KeySyncSet(u32 ix, u16 value) { env->mbCv[selectedCv].mbCvEnv2[selectedEnv2].envModeKeySync = value; }

static u16  env2OneshotGet(u32 ix)            { return env->mbCv[selectedCv].mbCvEnv2[selectedEnv2].envModeOneshot; }
static void env2OneshotSet(u32 ix, u16 value) { env->mbCv[selectedCv].mbCvEnv2[selectedEnv2].envModeOneshot = value; }

static u16  env2ModeFastGet(u32 ix)            { return env->mbCv[selectedCv].mbCvEnv2[selectedEnv2].envModeFast; }
static void env2ModeFastSet(u32 ix, u16 value) { env->mbCv[selectedCv].mbCvEnv2[selectedEnv2].envModeFast = value; }

static u16  env2DepthCvGet(u32 ix)            { return env->mbCv[selectedCv].mbCvEnv2[selectedEnv2].envDepthPitch + 128; }
static void env2DepthCvSet(u32 ix, u16 value) { env->mbCv[selectedCv].mbCvEnv2[selectedEnv2].envDepthPitch = value - 128; }

static u16  env2DepthLfo1AmpGet(u32 ix)            { return env->mbCv[selectedCv].mbCvEnv2[selectedEnv2].envDepthLfo1Amplitude + 128; }
static void env2DepthLfo1AmpSet(u32 ix, u16 value) { env->mbCv[selectedCv].mbCvEnv2[selectedEnv2].envDepthLfo1Amplitude = value - 128; }

static u16  env2DepthLfo1RateGet(u32 ix)            { return env->mbCv[selectedCv].mbCvEnv2[selectedEnv2].envDepthLfo1Rate + 128; }
static void env2DepthLfo1RateSet(u32 ix, u16 value) { env->mbCv[selectedCv].mbCvEnv2[selectedEnv2].envDepthLfo1Rate = value - 128; }

static u16  env2DepthLfo2AmpGet(u32 ix)            { return env->mbCv[selectedCv].mbCvEnv2[selectedEnv2].envDepthLfo2Amplitude + 128; }
static void env2DepthLfo2AmpSet(u32 ix, u16 value) { env->mbCv[selectedCv].mbCvEnv2[selectedEnv2].envDepthLfo2Amplitude = value - 128; }

static u16  env2DepthLfo2RateGet(u32 ix)            { return env->mbCv[selectedCv].mbCvEnv2[selectedEnv2].envDepthLfo2Rate + 128; }
static void env2DepthLfo2RateSet(u32 ix, u16 value) { env->mbCv[selectedCv].mbCvEnv2[selectedEnv2].envDepthLfo2Rate = value - 128; }

static u16  env2StepLevelGet(u32 ix)            { return env->mbCv[selectedCv].mbCvEnv2[selectedEnv2].envLevel[ix]; }
static void env2StepLevelSet(u32 ix, u16 value) { env->mbCv[selectedCv].mbCvEnv2[selectedEnv2].envLevel[ix] = value; }


/////////////////////////////////////////////////////////////////////////////
// AOUT
/////////////////////////////////////////////////////////////////////////////

static u16  cvCurveGet(u32 ix)            { return (u32)env->mbCv[selectedCv].mbCvVoice.getAoutCurve(); }
static void cvCurveSet(u32 ix, u16 value) { env->mbCv[selectedCv].mbCvVoice.setAoutCurve(value); }

static u16  cvSlewRateGet(u32 ix)            { return env->mbCv[selectedCv].mbCvVoice.getAoutSlewRate(); }
static void cvSlewRateSet(u32 ix, u16 value) { env->mbCv[selectedCv].mbCvVoice.setAoutSlewRate(value); }

static u16  cvCaliModeGet(u32 ix)            { return MBCV_MAP_CaliModeGet(); }
static void cvCaliModeSet(u32 ix, u16 value) { MBCV_MAP_CaliModeSet(selectedCv, (aout_cali_mode_t)value); }

static u16  aoutIfGet(u32 ix)              { return MBCV_MAP_IfGet(); }
static void aoutIfSet(u32 ix, u16 value)   { MBCV_MAP_IfSet((aout_if_t)value); }

static u16  cvUpdateRateGet(u32 ix)            { return APP_CvUpdateRateFactorGet() - 1; }
static void cvUpdateRateSet(u32 ix, u16 value) { APP_CvUpdateRateFactorSet(value+1); }

static u16  cvScopeGet(u32 ix)            { return env->mbCv[selectedCv].scopeSelect; }
static void cvScopeSet(u32 ix, u16 value) { env->mbCv[selectedCv].scopeSelect = value; env->updateScopeParameters(); }

static u16  cvScopeOversamplingGet(u32 ix)            { return env->mbCv[selectedCv].scopeOversamplingFactor; }
static void cvScopeOversamplingSet(u32 ix, u16 value) { env->mbCv[selectedCv].scopeOversamplingFactor = value; env->updateScopeParameters(); }

static u16  cvScopeTriggerLevelGet(u32 ix)            { return env->mbCv[selectedCv].scopeTriggerLevelPercent; }
static void cvScopeTriggerLevelSet(u32 ix, u16 value) { env->mbCv[selectedCv].scopeTriggerLevelPercent = value; env->updateScopeParameters(); }


/////////////////////////////////////////////////////////////////////////////
// Router
/////////////////////////////////////////////////////////////////////////////

static u16  routerNodeGet(u32 ix)             { return selectedRouterNode; }
static void routerNodeSet(u32 ix, u16 value)  { selectedRouterNode = value; }

static u16  routerSrcPortGet(u32 ix)             { return MIDI_PORT_InIxGet((mios32_midi_port_t)midi_router_node[selectedRouterNode].src_port); }
static void routerSrcPortSet(u32 ix, u16 value)  { midi_router_node[selectedRouterNode].src_port = MIDI_PORT_InPortGet(value); }

static u16  routerSrcChnGet(u32 ix)              { return midi_router_node[selectedRouterNode].src_chn; }
static void routerSrcChnSet(u32 ix, u16 value)   { midi_router_node[selectedRouterNode].src_chn = value; }

static u16  routerDstPortGet(u32 ix)             { return MIDI_PORT_OutIxGet((mios32_midi_port_t)midi_router_node[selectedRouterNode].dst_port); }
static void routerDstPortSet(u32 ix, u16 value)  { midi_router_node[selectedRouterNode].dst_port = MIDI_PORT_OutPortGet(value); }

static u16  routerDstChnGet(u32 ix)              { return midi_router_node[selectedRouterNode].dst_chn; }
static void routerDstChnSet(u32 ix, u16 value)   { midi_router_node[selectedRouterNode].dst_chn = value; }


/////////////////////////////////////////////////////////////////////////////
// OSC/Network
/////////////////////////////////////////////////////////////////////////////

static u16  oscPortGet(u32 ix)            { return selectedOscPort; }
static void oscPortSet(u32 ix, u16 value) { selectedOscPort = value; }
static u16  oscRemotePortGet(u32 ix)            { return OSC_SERVER_RemotePortGet(selectedOscPort); }
static void oscRemotePortSet(u32 ix, u16 value) { OSC_SERVER_RemotePortSet(selectedOscPort, value); OSC_SERVER_Init(0); }
static u16  oscLocalPortGet(u32 ix)             { return OSC_SERVER_LocalPortGet(selectedOscPort); }
static void oscLocalPortSet(u32 ix, u16 value)  { OSC_SERVER_LocalPortSet(selectedOscPort, value);  OSC_SERVER_Init(0); }
static u16  oscModeGet(u32 ix)                  { return OSC_CLIENT_TransferModeGet(selectedOscPort); }
static void oscModeSet(u32 ix, u16 value)       { OSC_CLIENT_TransferModeSet(selectedOscPort, value); }

static u16  dhcpGet(u32 ix)                { return UIP_TASK_DHCP_EnableGet(); }
static void dhcpSet(u32 ix, u16 value)     { UIP_TASK_DHCP_EnableSet(value); }
static u16  selIpParGet(u32 ix)            { return selectedIpPar; }
static void selIpParSet(u32 ix, u16 value)
{
  selectedIpPar = value;
}



static void MSD_EnableReq(u32 enable)
{
  TASK_MSD_EnableSet(enable);
  SCS_Msg(SCS_MSG_L, 1000, "Mass Storage", (char *)(enable ? "enabled!" : "disabled!"));
}

/////////////////////////////////////////////////////////////////////////////
// Menu Structure
/////////////////////////////////////////////////////////////////////////////

const scs_menu_item_t pageCV[] = {
  SCS_ITEM(" CV  ", 0, CV_SE_NUM-1, cvGet, cvSet, selectNOP, stringDecP1, NULL),
  SCS_ITEM("Chn  ", 0, 16,    cvChnGet,      cvChnSet,      selectNOP, stringDec0Dis, NULL),
  SCS_ITEM("Mode ", 0, MBCV_MIDI_EVENT_MODE_NUM-1,    cvEventGet,     cvEventSet,     selectNOP, stringCvMode, NULL),
  SCS_ITEM("Play ", 0, 2,     cvPlayModeGet, cvPlayModeSet, selectNOP, stringCvPlayMode, NULL),
  SCS_ITEM("InvG ", 0, 1,     cvInvGateGet,  cvInvGateSet,  selectNOP, stringOnOff, NULL),
  SCS_ITEM("SplL ", 0, 127,   cvSplitLowerGet,cvSplitLowerSet,selectNOP, stringNote, NULL),
  SCS_ITEM("SplU ", 0, 127,   cvSplitUpperGet,cvSplitUpperSet,selectNOP, stringNote, NULL),
  SCS_ITEM("PRng ", 0, 24,    cvPitchRangeGet,cvPitchRangeSet,selectNOP, stringDec, NULL),
  SCS_ITEM("Oct. ", 0, 15,    cvTranspOctGet, cvTranspOctSet,selectNOP, stringCvTranspose, NULL),
  SCS_ITEM("Semi ", 0, 15,    cvTranspSemiGet,cvTranspSemiSet,selectNOP,stringCvTranspose, NULL),
  SCS_ITEM("Fine ", 0, 255,   cvFinetuneGet,  cvFinetuneSet, selectNOP, stringDecPM128, NULL),
  SCS_ITEM("Port ", 0, 255,   cvPortamentoGet,cvPortamentoSet,selectNOP, stringDec, NULL),
  SCS_ITEM("PMod ", 0, 2,     cvPortamentoModeGet,cvPortamentoModeSet,selectNOP, stringCvPortamentoMode, NULL),
  SCS_ITEM("SusK ", 0, 1,     cvSusKeyGet,    cvSusKeySet,    selectNOP, stringOnOff, NULL),
  SCS_ITEM(" CC  ", 0, 127,   cvCCGet,        cvCCSet,       selectNOP,stringDec, NULL),
  SCS_ITEM("USB1 ", 0, 1,           cvPortGet,      cvPortSet,      selectNOP, stringOnOff, NULL),
#if MIOS32_USB_MIDI_NUM_PORTS >= 2
  SCS_ITEM("USB2 ", 1, 1,           cvPortGet,      cvPortSet,      selectNOP, stringOnOff, NULL),
#endif
#if MIOS32_USB_MIDI_NUM_PORTS >= 3
  SCS_ITEM("USB3 ", 2, 1,           cvPortGet,      cvPortSet,      selectNOP, stringOnOff, NULL),
#endif
#if MIOS32_USB_MIDI_NUM_PORTS >= 4
  SCS_ITEM("USB4 ", 3, 1,           cvPortGet,      cvPortSet,      selectNOP, stringOnOff, NULL),
#endif
  SCS_ITEM("IN1  ", 4, 1,           cvPortGet,      cvPortSet,      selectNOP, stringOnOff, NULL),
  SCS_ITEM("IN2  ", 5, 1,           cvPortGet,      cvPortSet,      selectNOP, stringOnOff, NULL),
#if MIOS32_UART_NUM >= 3
  SCS_ITEM("IN3  ", 6, 1,           cvPortGet,      cvPortSet,      selectNOP, stringOnOff, NULL),
#endif
#if MIOS32_UART_NUM >= 4
  SCS_ITEM("IN4  ", 7, 1,           cvPortGet,      cvPortSet,      selectNOP, stringOnOff, NULL),
#endif
  SCS_ITEM("OSC1 ",12, 1,           cvPortGet,      cvPortSet,      selectNOP, stringOnOff, NULL),
  SCS_ITEM("OSC2 ",13, 1,           cvPortGet,      cvPortSet,      selectNOP, stringOnOff, NULL),
  SCS_ITEM("OSC3 ",14, 1,           cvPortGet,      cvPortSet,      selectNOP, stringOnOff, NULL),
  SCS_ITEM("OSC4 ",15, 1,           cvPortGet,      cvPortSet,      selectNOP, stringOnOff, NULL),
};

const scs_menu_item_t pageARP[] = {
  SCS_ITEM(" CV  ", 0, CV_SE_NUM-1, cvGet, cvSet, selectNOP, stringDecP1, NULL),
  SCS_ITEM(" On  ", 0,  1,          arpOnGet,       arpOnSet,       selectNOP, stringOnOff,  NULL),
  SCS_ITEM("Mode ", 0,  6,          arpDirGet,      arpDirSet,      selectNOP, stringArpDir, NULL),
  SCS_ITEM("Sort ", 0,  1,          arpSortGet,     arpSortSet,     selectNOP, stringOnOff,  NULL),
  SCS_ITEM("Hold ", 0,  1,          arpHoldGet,     arpHoldSet,     selectNOP, stringOnOff,  NULL),
  SCS_ITEM("Spd. ", 0, 63,          arpSpeedGet,    arpSpeedSet,    selectNOP, stringDecP1,  NULL),
  SCS_ITEM("GLn. ", 0, 31,          arpGatelenGet,  arpGatelenSet,  selectNOP, stringDecP1,  NULL),
  SCS_ITEM("Rnge ", 0,  7,          arpRangeGet,    arpRangeSet,    selectNOP, stringDecP1,  NULL),
  SCS_ITEM("Sync ", 0,  1,          arpSyncGet,     arpSyncSet,     selectNOP, stringOnOff,  NULL),
  SCS_ITEM("CAC  ", 0,  1,          arpCACGet,      arpCACSet,      selectNOP, stringOnOff,  NULL),
  SCS_ITEM("OSht ", 0,  1,          arpOneshotGet,  arpOneshotSet,  selectNOP, stringOnOff,  NULL),
  SCS_ITEM("Easy ", 0,  1,          arpEasyChordGet,arpEasyChordSet,selectNOP, stringOnOff,  NULL),
};

const scs_menu_item_t pageLFO[] = {
  SCS_ITEM(" CV  ", 0, CV_SE_NUM-1, cvGet, cvSet, selectNOP, stringDecP1, NULL),
  SCS_ITEM("LFO  ", 0,   1,         lfoGet,         lfoSet,         selectNOP, stringDecP1, NULL),
  SCS_ITEM("Ampl ", 0, 255,         lfoAmplitudeGet,lfoAmplitudeSet,selectNOP, stringDecPM128, NULL),
  SCS_ITEM("Rate ", 0, 255,         lfoRateGet,     lfoRateSet,     selectNOP, stringLfoRate, NULL),
  SCS_ITEM("Wave ", 0,   9,         lfoWaveGet,     lfoWaveSet,     selectNOP, stringLfoWave, NULL),
  SCS_ITEM("ClkS ", 0,   1,         lfoClkSyncGet,  lfoClkSyncSet,  selectNOP, stringOnOff, NULL),
  SCS_ITEM("KeyS ", 0,   1,         lfoKeySyncGet,  lfoKeySyncSet,  selectNOP, stringOnOff, NULL),
  SCS_ITEM("Dely ", 0, 255,         lfoDelayGet,    lfoDelaySet,    selectNOP, stringDec, NULL),
  SCS_ITEM("Phse ", 0, 255,         lfoPhaseGet,    lfoPhaseSet,    selectNOP, stringDec, NULL),
  SCS_ITEM("Fast ", 0,   1,         lfoModeFastGet, lfoModeFastSet, selectNOP, stringOnOff, NULL),
  SCS_ITEM("OSht ", 0,   1,         lfoOneshotGet,  lfoOneshotSet,  selectNOP, stringOnOff, NULL),
  SCS_ITEM("D.CV ", 0, 255,         lfoDepthCvGet,  lfoDepthCvSet,  selectNOP, stringDecPM128, NULL),
  SCS_ITEM("DLAmp", 0, 255,         lfoDepthLfoAmpGet,lfoDepthLfoAmpSet,selectNOP, stringDecPM128, NULL),
  SCS_ITEM("DLRte", 0, 255,         lfoDepthLfoRateGet,lfoDepthLfoRateSet,selectNOP, stringDecPM128, NULL),
  SCS_ITEM("DE1Rt", 0, 255,         lfoDepthEnv1RateGet,lfoDepthEnv1RateSet,selectNOP, stringDecPM128, NULL),
  SCS_ITEM("DE2Rt", 0, 255,         lfoDepthEnv2RateGet,lfoDepthEnv2RateSet,selectNOP, stringDecPM128, NULL),
};

const scs_menu_item_t pageENV1[] = {
  SCS_ITEM(" CV  ", 0, CV_SE_NUM-1, cvGet, cvSet, selectNOP, stringDecP1, NULL),
  SCS_ITEM("Ampl ", 0, 255,         env1AmplitudeGet,env1AmplitudeSet,selectNOP, stringDecPM128, NULL),
  SCS_ITEM("Dely ", 0, 255,         env1DelayGet,    env1DelaySet,    selectNOP, stringDec, NULL),
  SCS_ITEM("Atk. ", 0, 255,         env1AttackGet,   env1AttackSet,   selectNOP, stringDec, NULL),
  SCS_ITEM("Dec. ", 0, 255,         env1DecayGet,    env1DecaySet,    selectNOP, stringDec, NULL),
  SCS_ITEM("Sus. ", 0, 255,         env1SustainGet,  env1SustainSet,  selectNOP, stringDec, NULL),
  SCS_ITEM("Rel. ", 0, 255,         env1ReleaseGet,  env1ReleaseSet,  selectNOP, stringDec, NULL),
  SCS_ITEM("Curv ", 0,   1,         env1CurveGet,    env1CurveSet,    selectNOP, stringCurve, NULL),
  SCS_ITEM("ClkS ", 0,   1,         env1ClkSyncGet,  env1ClkSyncSet,  selectNOP, stringOnOff, NULL),
  SCS_ITEM("KeyS ", 0,   1,         env1KeySyncGet,  env1KeySyncSet,  selectNOP, stringOnOff, NULL),
  SCS_ITEM("Fast ", 0,   1,         env1ModeFastGet, env1ModeFastSet, selectNOP, stringOnOff, NULL),
  SCS_ITEM("OSht ", 0,   1,         env1OneshotGet,  env1OneshotSet,  selectNOP, stringOnOff, NULL),
  SCS_ITEM("D.CV ", 0, 255,         env1DepthCvGet,  env1DepthCvSet,  selectNOP, stringDecPM128, NULL),
  SCS_ITEM("DL1A ", 0, 255,         env1DepthLfo1AmpGet,env1DepthLfo1AmpSet,selectNOP, stringDecPM128, NULL),
  SCS_ITEM("DL1R ", 0, 255,         env1DepthLfo1RateGet,env1DepthLfo1RateSet,selectNOP, stringDecPM128, NULL),
  SCS_ITEM("DL2A ", 0, 255,         env1DepthLfo2AmpGet,env1DepthLfo2AmpSet,selectNOP, stringDecPM128, NULL),
  SCS_ITEM("DL2R ", 0, 255,         env1DepthLfo2RateGet,env1DepthLfo2RateSet,selectNOP, stringDecPM128, NULL),
};

const scs_menu_item_t pageENV2[] = {
  SCS_ITEM(" CV  ", 0, CV_SE_NUM-1, cvGet, cvSet, selectNOP, stringDecP1, NULL),
  SCS_ITEM("Ampl ", 0, 255,         env2AmplitudeGet,   env2AmplitudeSet,selectNOP, stringDecPM128, NULL),
  SCS_ITEM("Offs ", 0, 255,         env2OffsetGet,      env2OffsetSet,   selectNOP, stringDecPM128, NULL),
  SCS_ITEM("Rate ", 0, 255,         env2RateGet,        env2RateSet,     selectNOP, stringDec, NULL),
  SCS_ITEM("Curv ", 0,   1,         env2CurveGet,       env2CurveSet,    selectNOP, stringCurve, NULL),
  SCS_ITEM("Stps ", 0,  15,         env2LastStepGet,    env2LastStepSet, selectNOP, stringDecP1, NULL),
  SCS_ITEM("LpAt ", 0,  16,         env2LoopAttackGet,  env2LoopAttackSet,    selectNOP, stringDec, NULL),
  SCS_ITEM("SusS ", 0,  16,         env2SustainStepGet, env2SustainStepSet,    selectNOP, stringDec, NULL),
  SCS_ITEM("LpRl ", 0,  16,         env2LoopReleaseGet, env2LoopReleaseSet,    selectNOP, stringDec, NULL),
  SCS_ITEM("ClkS ", 0,   1,         env2ClkSyncGet,     env2ClkSyncSet,  selectNOP, stringOnOff, NULL),
  SCS_ITEM("KeyS ", 0,   1,         env2KeySyncGet,     env2KeySyncSet,  selectNOP, stringOnOff, NULL),
  SCS_ITEM("Fast ", 0,   1,         env2ModeFastGet,    env2ModeFastSet, selectNOP, stringOnOff, NULL),
  SCS_ITEM("OSht ", 0,   1,         env2OneshotGet,     env2OneshotSet,  selectNOP, stringOnOff, NULL),
  SCS_ITEM("D.CV ", 0, 255,         env2DepthCvGet,     env2DepthCvSet,  selectNOP, stringDecPM128, NULL),
  SCS_ITEM("DL1A ", 0, 255,         env2DepthLfo1AmpGet,env2DepthLfo1AmpSet,selectNOP, stringDecPM128, NULL),
  SCS_ITEM("DL1R ", 0, 255,         env2DepthLfo1RateGet,env2DepthLfo1RateSet,selectNOP, stringDecPM128, NULL),
  SCS_ITEM("DL2A ", 0, 255,         env2DepthLfo2AmpGet,env2DepthLfo2AmpSet,selectNOP, stringDecPM128, NULL),
  SCS_ITEM("DL2R ", 0, 255,         env2DepthLfo2RateGet,env2DepthLfo2RateSet,selectNOP, stringDecPM128, NULL),
  SCS_ITEM("Lvl1 ", 0, 255,         env2StepLevelGet,   env2StepLevelSet,selectNOP, stringDecPM128, NULL),
  SCS_ITEM("Lvl2 ", 1, 255,         env2StepLevelGet,   env2StepLevelSet,selectNOP, stringDecPM128, NULL),
  SCS_ITEM("Lvl3 ", 2, 255,         env2StepLevelGet,   env2StepLevelSet,selectNOP, stringDecPM128, NULL),
  SCS_ITEM("Lvl4 ", 3, 255,         env2StepLevelGet,   env2StepLevelSet,selectNOP, stringDecPM128, NULL),
  SCS_ITEM("Lvl5 ", 4, 255,         env2StepLevelGet,   env2StepLevelSet,selectNOP, stringDecPM128, NULL),
  SCS_ITEM("Lvl6 ", 5, 255,         env2StepLevelGet,   env2StepLevelSet,selectNOP, stringDecPM128, NULL),
  SCS_ITEM("Lvl7 ", 6, 255,         env2StepLevelGet,   env2StepLevelSet,selectNOP, stringDecPM128, NULL),
  SCS_ITEM("Lvl8 ", 7, 255,         env2StepLevelGet,   env2StepLevelSet,selectNOP, stringDecPM128, NULL),
  SCS_ITEM("Lvl9 ", 8, 255,         env2StepLevelGet,   env2StepLevelSet,selectNOP, stringDecPM128, NULL),
  SCS_ITEM("Lvl10", 9, 255,         env2StepLevelGet,   env2StepLevelSet,selectNOP, stringDecPM128, NULL),
  SCS_ITEM("Lvl11", 10, 255,        env2StepLevelGet,   env2StepLevelSet,selectNOP, stringDecPM128, NULL),
  SCS_ITEM("Lvl12", 11, 255,        env2StepLevelGet,   env2StepLevelSet,selectNOP, stringDecPM128, NULL),
  SCS_ITEM("Lvl13", 12, 255,        env2StepLevelGet,   env2StepLevelSet,selectNOP, stringDecPM128, NULL),
  SCS_ITEM("Lvl14", 13, 255,        env2StepLevelGet,   env2StepLevelSet,selectNOP, stringDecPM128, NULL),
  SCS_ITEM("Lvl15", 14, 255,        env2StepLevelGet,   env2StepLevelSet,selectNOP, stringDecPM128, NULL),
  SCS_ITEM("Lvl16", 15, 255,        env2StepLevelGet,   env2StepLevelSet,selectNOP, stringDecPM128, NULL),
};

const scs_menu_item_t pageAOUT[] = {
  SCS_ITEM(" CV  ", 0, CV_SE_NUM-1, cvGet, cvSet, selectNOP, stringDecP1, NULL),
  SCS_ITEM("Curve", 0, MBCV_MAP_NUM_CURVES-1, cvCurveGet, cvCurveSet, selectNOP, stringCvCurve, NULL),
  SCS_ITEM(" Slew", 0, 255,   cvSlewRateGet, cvSlewRateSet, selectNOP, stringDec4, NULL),
  SCS_ITEM(" Cali", 0, MBCV_MAP_NUM_CALI_MODES-1, cvCaliModeGet, cvCaliModeSet, selectNOP, stringCvCaliMode, NULL),
  SCS_ITEM(" Modu", 0, AOUT_NUM_IF-1, aoutIfGet, aoutIfSet, selectNOP, stringAoutIf, NULL),
  SCS_ITEM("le   ", 1, AOUT_NUM_IF-1, aoutIfGet, aoutIfSet, selectNOP, stringAoutIf, NULL),
  SCS_ITEM("UpdR",  0, APP_CV_UPDATE_RATE_FACTOR_MAX-1, cvUpdateRateGet, cvUpdateRateSet, selectNOP, stringCvUpdateRate, NULL),
};

const scs_menu_item_t pageScpe[] = {
  SCS_ITEM(" CV ", 0, CV_SE_NUM-1,    cvGet,                  cvSet,                  selectNOP, stringDecP1, NULL),
  SCS_ITEM("Scpe", 0, CV_SCOPE_NUM,   cvScopeGet,             cvScopeSet,             selectNOP, stringScope, NULL),
  SCS_ITEM("OSmp", 0, 255,            cvScopeOversamplingGet, cvScopeOversamplingSet, selectNOP, stringDec, NULL),
  SCS_ITEM("TLvl", 0, 100,            cvScopeTriggerLevelGet, cvScopeTriggerLevelSet, selectNOP, stringDecPercent, NULL),
};

const scs_menu_item_t pageROUT[] = {
  SCS_ITEM("Node", 0, MIDI_ROUTER_NUM_NODES-1,  routerNodeGet, routerNodeSet,selectNOP, stringDecP1, NULL),
  SCS_ITEM("SrcP", 0, MIDI_PORT_NUM_IN_PORTS-1, routerSrcPortGet, routerSrcPortSet,selectNOP, stringInPort, NULL),
  SCS_ITEM("Chn.", 0, 17,                       routerSrcChnGet, routerSrcChnSet,selectNOP, stringRouterChn, NULL),
  SCS_ITEM("DstP", 0, MIDI_PORT_NUM_OUT_PORTS-1, routerDstPortGet, routerDstPortSet,selectNOP, stringOutPort, NULL),
  SCS_ITEM("Chn.", 0, 17,                       routerDstChnGet, routerDstChnSet,selectNOP, stringRouterChn, NULL),
};

const scs_menu_item_t pageDsk[] = {
  SCS_ITEM("Load ", 0, 0,           dummyGet,        dummySet,        selectLOAD, stringEmpty, NULL),
  SCS_ITEM("Save ", 0, 0,           dummyGet,        dummySet,        selectSAVE, stringEmpty, NULL),
};

const scs_menu_item_t pageOSC[] = {
  SCS_ITEM("Port ", 0, 3,           oscPortGet,      oscPortSet,      selectNOP,  stringOscPort, NULL),
  SCS_ITEM("Remot", 0, 0,           dummyGet,        dummySet,        selectRemoteIp, stringRemoteIp, NULL),
  SCS_ITEM("e IP:", 1, 0,           dummyGet,        dummySet,        selectRemoteIp, stringRemoteIp, NULL),
  SCS_ITEM("     ", 2, 0,           dummyGet,        dummySet,        selectRemoteIp, stringRemoteIp, NULL),
  SCS_ITEM("RPort", 0, 65535,       oscRemotePortGet,oscRemotePortSet,selectNOP,      stringDec5,     NULL),
  SCS_ITEM("LPort", 0, 65535,       oscLocalPortGet, oscLocalPortSet, selectNOP,      stringDec5,     NULL),
  SCS_ITEM(" Mode", 0, OSC_CLIENT_NUM_TRANSFER_MODES-1, oscModeGet, oscModeSet, selectNOP, stringOscMode, stringOscModeFull),
};

const scs_menu_item_t pageNetw[] = {
  SCS_ITEM("DHCP ", 0, 1,           dhcpGet,         dhcpSet,         selectNOP,  stringOnOff, NULL),
  SCS_ITEM(" IP  ", 0, 2,           selIpParGet,     selIpParSet,     selectNOP,  stringIpPar, NULL),
  SCS_ITEM("     ", 0, 0,           dummyGet,        dummySet,        selectIpEnter,stringIp, NULL),
  SCS_ITEM("     ", 1, 0,           dummyGet,        dummySet,        selectIpEnter,stringIp, NULL),
  SCS_ITEM("     ", 2, 0,           dummyGet,        dummySet,        selectIpEnter,stringIp, NULL),
};

const scs_menu_item_t pageMON[] = {
  // dummy - will be overlayed in displayHook
  SCS_ITEM("     ", 0, 0,           dummyGet,        dummySet,        selectNOP,      stringEmpty, NULL),
};

const scs_menu_page_t rootMode0[] = {
  SCS_PAGE(" CV  ", pageCV),
  SCS_PAGE("ARP  ", pageARP),
  SCS_PAGE("LFO  ", pageLFO),
  SCS_PAGE("ENV1 ", pageENV1),
  SCS_PAGE("ENV2 ", pageENV2),
  SCS_PAGE("AOUT ", pageAOUT),
  SCS_PAGE("Scpe ", pageScpe),
  SCS_PAGE("Rout ", pageROUT),
  SCS_PAGE("OSC  ", pageOSC),
  SCS_PAGE("Netw ", pageNetw),
  SCS_PAGE("Mon. ", pageMON),
  SCS_PAGE("Disk ", pageDsk),
};


/////////////////////////////////////////////////////////////////////////////
// This function can overrule the display output
// If it returns 0, the original SCS output will be print
// If it returns 1, the output copied into line1 and/or line2 will be print
// If a line is not changed (line[0] = 0 or line[1] = 0), the original output
// will be displayed - this allows to overrule only a single line
/////////////////////////////////////////////////////////////////////////////
static s32 displayHook(char *line1, char *line2)
{
  if( extraPage ) {
    char msdStr[5];
    TASK_MSD_FlagStrGet(msdStr);

    sprintf(line1, "      Enc Bank MSD  ");
    sprintf(line2, "Panic %s  %2d  %s ",
	    MBCV_LRE_ConfigModeGet() ? "On " : "Cfg",
	    MBCV_LRE_BankGet()+1,
	    TASK_MSD_EnableGet() ? msdStr : "----");
    return 1;
  }

  // overlay on overload
  if( APP_CvUpdateOverloadStatusGet() ) {
    if( (MIOS32_TIMESTAMP_Get() % 100) > 50 )
      sprintf(line1, "[-! CV  OVERLOAD !-]");
  }

  // overlay in MSD mode (user should disable soon since this sucks performance)
  if( TASK_MSD_EnableGet() ) {
    char msdStr[5];
    TASK_MSD_FlagStrGet(msdStr);

    sprintf(line1, "[ MSD ACTIVE: %s ]", msdStr);
  }

  if( SCS_MenuStateGet() == SCS_MENU_STATE_MAINPAGE ) {
    u8 fastRefresh = line1[0] == 0;
//    u32 tick = SEQ_BPM_TickGet();
//    u32 ticks_per_step = SEQ_BPM_PPQN_Get() / 4;
//    u32 ticks_per_measure = ticks_per_step * 16;
//    u32 measure = (tick / ticks_per_measure) + 1;
//    u32 step = ((tick % ticks_per_measure) / ticks_per_step) + 1;

    if( line1[0] == 0 ) { // no MSD overlay?
      if( MBCV_FILE_StatusMsgGet() )
	sprintf(line1, MBCV_FILE_StatusMsgGet());
      else
	sprintf(line1, "Patch: %-8s  [%d]", mbcv_file_p_patch_name, selectedCv+1);
    }
    sprintf(line2, "%s   <    >   MENU", env->mbCvClock.isRunning ? "STOP" : "PLAY");

    // request LCD update - this will lead to fast refresh rate in main screen
    if( fastRefresh )
      SCS_DisplayUpdateRequest();

    return 1;
  }

  if( SCS_MenuStateGet() == SCS_MENU_STATE_SELECT_PAGE ) {
    if( line1[0] == 0 ) { // no MSD overlay?
      if( MBCV_FILE_StatusMsgGet() )
	sprintf(line1, MBCV_FILE_StatusMsgGet());
      else
	sprintf(line1, "Patch: %s", mbcv_file_p_patch_name);
    }
    return 1;
  }

  if( SCS_MenuPageGet() == pageDsk ) {
    // Disk page: we want to show the patch at upper line, and menu items at lower line
    if( line1[0] == 0 ) { // no MSD overlay?
      if( MBCV_FILE_StatusMsgGet() )
	sprintf(line1, MBCV_FILE_StatusMsgGet());
      else
	sprintf(line1, "Patch: %s", mbcv_file_p_patch_name);
    }
    sprintf(line2, "Load Save");
    return 1;
  }

  if( SCS_MenuPageGet() == pageMON ) {
    u8 fastRefresh = line1[0] == 0;

    int i;
    for(i=0; i<SCS_NUM_MENU_ITEMS; ++i) {
      u8 portIx = 1 + i + monPageOffset;

      if( fastRefresh ) { // no MSD overlay?
	mios32_midi_port_t port = MIDI_PORT_InPortGet(portIx);
	mios32_midi_package_t package = MIDI_PORT_InPackageGet(port);
	if( port == 0xff ) {
	  strcat(line1, "     ");
	} else if( package.type ) {
	  char buffer[6];
	  MIDI_PORT_EventNameGet(package, buffer, 5);
	  strcat(line1, buffer);
	} else {
	  strcat(line1, MIDI_PORT_InNameGet(portIx));
	  strcat(line1, " ");
	}

	// insert arrow at upper right corner
	int numItems = MIDI_PORT_OutNumGet() - 1;
	if( monPageOffset == 0 )
	  line1[19] = 3; // right arrow
	else if( monPageOffset >= (numItems-SCS_NUM_MENU_ITEMS) )
	  line1[19] = 1; // left arrow
	else
	  line1[19] = 2; // left/right arrow

      }

      mios32_midi_port_t port = MIDI_PORT_OutPortGet(portIx);
      mios32_midi_package_t package = MIDI_PORT_OutPackageGet(port);
      if( port == 0xff ) {
	strcat(line2, "     ");
      } else if( package.type ) {
	char buffer[6];
	MIDI_PORT_EventNameGet(package, buffer, 5);
	strcat(line2, buffer);
      } else {
	strcat(line2, MIDI_PORT_OutNameGet(portIx));
	strcat(line2, " ");
      }
    }

    // request LCD update - this will lead to fast refresh rate in monitor screen
    if( fastRefresh )
      SCS_DisplayUpdateRequest();

    return 1;
  }

  return (line1[0] != 0) ? 1 : 0; // return 1 if MSD overlay
}


/////////////////////////////////////////////////////////////////////////////
// This function is called when the rotary encoder is moved
// If it returns 0, the encoder increment will be handled by the SCS
// If it returns 1, the SCS will ignore the encoder
/////////////////////////////////////////////////////////////////////////////
static s32 encHook(s32 incrementer)
{
  if( extraPage )
    return 1; // ignore encoder movements in extra page

  // encoder overlayed in monitor page to scroll through port list
  if( SCS_MenuPageGet() == pageMON ) {
    int numItems = MIDI_PORT_OutNumGet() - 1;
    int newOffset = monPageOffset + incrementer;
    if( newOffset < 0 )
      newOffset = 0;
    else if( (newOffset+SCS_NUM_MENU_ITEMS) >= numItems ) {
      newOffset = numItems - SCS_NUM_MENU_ITEMS;
      if( newOffset < 0 )
	newOffset = 0;
    }
    monPageOffset = newOffset;
  }

  return 0;
}


/////////////////////////////////////////////////////////////////////////////
// This function is called when a button has been pressed or depressed
// If it returns 0, the button movement will be handled by the SCS
// If it returns 1, the SCS will ignore the button event
/////////////////////////////////////////////////////////////////////////////
static s32 buttonHook(u8 scsButton, u8 depressed)
{
  if( extraPage ) {
    if( scsButton == SCS_PIN_SOFT5 && depressed ) { // selects/deselects extra page
      extraPage = 0;
      SCS_MsgStop();
    } else {
      switch( scsButton ) {
      case SCS_PIN_SOFT1:
	if( depressed )
	  return 1;
	MBCV_MAP_ResetAllChannels();
	SCS_Msg(SCS_MSG_L, 1000, "All Notes", "off!");
	break;

      case SCS_PIN_SOFT2:
	if( depressed )
	  return 1;
	MBCV_LRE_ConfigModeSet(MBCV_LRE_ConfigModeGet() ? 0 : 1);
	break;

      case SCS_PIN_SOFT3:
	if( depressed )
	  return 1;
	MBCV_LRE_BankSet((MBCV_LRE_BankGet() + 1) % MBCV_LRE_NUM_BANKS);
	break;

      case SCS_PIN_SOFT4: {
	u8 do_enable = TASK_MSD_EnableGet() ? 0 : 1;
	if( depressed )
	  SCS_UnInstallDelayedActionCallback(MSD_EnableReq);
	else {
	  if( !do_enable ) {
	    // wait a bit longer... normaly it would be better to print a warning that "unmounting via OS" is better
	    SCS_InstallDelayedActionCallback(MSD_EnableReq, 5000, do_enable);
	    SCS_Msg(SCS_MSG_DELAYED_ACTION_L, 5001, "", "to disable MSD USB!");
	  } else {
	    SCS_InstallDelayedActionCallback(MSD_EnableReq, 2000, do_enable);
	    SCS_Msg(SCS_MSG_DELAYED_ACTION_L, 2001, "", "to enable MSD USB!");
	  }
	}
      } break;
      }
    }

    return 1;
  } else {
    if( scsButton == SCS_PIN_SOFT5 && !depressed ) { // selects/deselects extra page
      extraPage = 1;
      return 1;
    }

    if( SCS_MenuStateGet() == SCS_MENU_STATE_MAINPAGE ) {
      if( depressed )
	return 0;

      switch( scsButton ) {
      case SCS_PIN_SOFT1: // Play/Stop
	if( env->mbCvClock.isRunning )
	  env->mbCvClock.midiReceiveRealTimeEvent(DEFAULT, 0xfc); // stop
	else
	  env->mbCvClock.midiReceiveRealTimeEvent(DEFAULT, 0xfa); // start
	return 1;

      case SCS_PIN_SOFT2: { // previous song
	MUTEX_SDCARD_TAKE;
	//SEQ_PlayFileReq(-1, 1);
	MUTEX_SDCARD_GIVE;
	return 1;
      }

      case SCS_PIN_SOFT3: { // next song
	MUTEX_SDCARD_TAKE;
	//SEQ_PlayFileReq(1, 1);
	MUTEX_SDCARD_GIVE;
	return 1;
      }
      }
    }
  }

  return 0; // no error
}



/////////////////////////////////////////////////////////////////////////////
// Initialisation of SCS Config
// mode selects the used SCS config (currently only one available selected with 0)
// return < 0 if initialisation failed
/////////////////////////////////////////////////////////////////////////////
s32 SCS_CONFIG_Init(u32 mode)
{
  env = APP_GetEnv();

  if( mode > 0 )
    return -1;

  switch( mode ) {
  case 0: {
    // install table
    SCS_INSTALL_ROOT(rootMode0);
    SCS_InstallDisplayHook(displayHook);
    SCS_InstallEncHook(encHook);
    SCS_InstallButtonHook(buttonHook);
    monPageOffset = 0;
    break;
  }
  default: return -1; // mode not supported
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Access functions for MBCV_BUTTON
s32 SCS_CONFIG_CvSet(u8 cv)
{
  selectedCv = cv;
  return 0; // no error
}

s32 SCS_CONFIG_LfoSet(u8 lfo)
{
  selectedLfo = lfo;
  SCS_ChangePage((scs_menu_item_t *)pageLFO);
  return 0; // no error
}

s32 SCS_CONFIG_EnvSet(u8 env)
{
  switch( env ) {
  case 0:
    SCS_ChangePage((scs_menu_item_t *)pageENV1);
    selectedEnv1 = 0;
    break;
  case 1:
    SCS_ChangePage((scs_menu_item_t *)pageENV2);
    selectedEnv2 = 0;
    break;
  }

  return 0; // no error
}

s32 SCS_CONFIG_ScopeSet(u8 scope)
{
  //selectedScope = scope;
  selectedCv = scope; // TODO... currently CVs are mapped to scopes, and not vice versa
  SCS_ChangePage((scs_menu_item_t *)pageScpe);
  return 0; // no error
}
