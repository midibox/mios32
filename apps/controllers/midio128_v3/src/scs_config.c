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

#include <midi_port.h>
#include <midi_router.h>

#include <uip.h>
#include "uip_task.h"
#include "osc_client.h"
#include "osc_server.h"

#include <scs.h>
#include "scs_config.h"

#include <seq_bpm.h>
#include "seq.h"
#include "mid_file.h"

#include "midio_dout.h"

#include <file.h>
#include "midio_file.h"
#include "midio_file_p.h"
#include "midio_patch.h"



/////////////////////////////////////////////////////////////////////////////
// Local defines
/////////////////////////////////////////////////////////////////////////////

static u8 extraPage;

static u8 selectedDin;
static u8 selectedDout;
static u8 selectedMatrix;
static u8 selectedMatrixPin;
static u8 selectedAin;
static u8 selectedAinser;
static u8 selectedRouterNode;
static u8 selectedIpPar;
static u8 selectedOscPort;
static u8 monPageOffset;


/////////////////////////////////////////////////////////////////////////////
// Local parameter variables
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// String Conversion Functions
/////////////////////////////////////////////////////////////////////////////
static void stringEmpty(u32 ix, u16 value, char *label)   { label[0] = 0; }
static void stringDec(u32 ix, u16 value, char *label)    { sprintf(label, "%3d  ", value); }
static void stringDecP1(u32 ix, u16 value, char *label)  { sprintf(label, "%3d  ", value+1); }
static void stringDecPM(u32 ix, u16 value, char *label)  { sprintf(label, "%3d  ", (int)value - 64); }
static void stringDec03(u32 ix, u16 value, char *label)  { sprintf(label, "%03d  ", value); }
static void stringDec0Dis(u32 ix, u16 value, char *label){ sprintf(label, value ? "%3d  " : "---  ", value); }
static void stringDec4(u32 ix, u16 value, char *label)   { sprintf(label, "%4d ", value); }
static void stringDec5(u32 ix, u16 value, char *label)   { sprintf(label, "%5d", value); }
static void stringHex2(u32 ix, u16 value, char *label)    { sprintf(label, " %02X  ", value); }
static void stringHex2O80(u32 ix, u16 value, char *label) { sprintf(label, " %02X  ", value | 0x80); }
static void stringOnOff(u32 ix, u16 value, char *label)  { sprintf(label, " [%c] ", value ? 'x' : ' '); }

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

static void stringDIN_SR(u32 ix, u16 value, char *label)  { sprintf(label, "%2d.%d", (value/8)+1, value%8); }
static void stringDOUT_SR(u32 ix, u16 value, char *label) { sprintf(label, "%2d.%d", (value/8)+1, 7-(value%8)); }
static void stringDIN_Mode(u32 ix, u16 value, char *label)
{
  const char dinLabel[3][5] = { "Norm", "OnOf", "Togl" };
  if( value < 3 )
    strcpy(label, dinLabel[value]);
  else
    sprintf(label, "%3d ", value);
}

static void stringMatrixMode(u32 ix, u16 value, char *label)
{
  const char modeLabel[2][5] = { "Norm", "Map " };
  if( value < 2 )
    strcpy(label, modeLabel[value]);
  else
    sprintf(label, "%3d ", value);
}

static void stringMatrixPin(u32 ix, u16 value, char *label)
{
  if( midio_patch_matrix[selectedMatrix].mode != MIDIO_PATCH_MATRIX_MODE_MAPPED ) {
    sprintf(label, "n/a ", value);
  } else {
    sprintf(label, "%d.%d ", (value / 8)+1, (value % 8)+1);
  }
}

static void stringAINSER_Pin(u32 ix, u16 value, char *label)
{
  sprintf(label, "%c%d.%d", 'A' + (value / 64), ((value % 64)/8)+1, value%8);
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

static void stringMidiMode(u32 ix, u16 value, char *label)
{
  const char midiModeLabel[SEQ_MIDI_PLAY_MODE_NUM][6] = { "All ", "Sngl" };
  strcpy(label, midiModeLabel[value < SEQ_MIDI_PLAY_MODE_NUM ? value : 0]);  
}

static void stringMidiFileName(u32 ix, u16 value, char *label)
{
  memcpy(label, (char *)(MID_FILE_UI_NameGet() + 5*ix), 5);
}


/////////////////////////////////////////////////////////////////////////////
// Parameter Selection Functions
/////////////////////////////////////////////////////////////////////////////
static u16  selectNOP(u32 ix, u16 value)    { return value; }

static void selectSAVE_Callback(char *newString)
{
  s32 status;

  if( (status=MIDIO_PATCH_Store(newString)) < 0 ) {
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
  return SCS_InstallEditStringCallback(selectSAVE_Callback, "SAVE", midio_file_p_patch_name, MIDIO_FILE_P_FILENAME_LEN);
}

static void selectLOAD_Callback(char *newString)
{
  s32 status;

  if( newString[0] != 0 ) {
    if( (status=MIDIO_PATCH_Load(newString)) < 0 ) {
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

static void selectMidiFile_Callback(char *newString)
{
  s32 status;

  if( newString[0] != 0 ) {
    char midifile[20];
    sprintf(midifile, "%s.MID", newString);

    if( (status=SEQ_PlayFile(midifile)) < 0 ) {
      SCS_Msg(SCS_MSG_ERROR_L, 1000, "Failed to load", midifile);
    } else {
      SCS_Msg(SCS_MSG_L, 1000, "Playing:", midifile);
    }
  }
}
static u8 getListMidiFile_Callback(u8 offset, char *line)
{
  MUTEX_SDCARD_TAKE;
  s32 status = FILE_GetFiles("/", "MID", line, 2, offset);
  MUTEX_SDCARD_GIVE;
  if( status < 1 ) {
    sprintf(line, "<no .MID files>");
    status = 0;
  }
  return status;
}
static u16  selectMidiFile(u32 ix, u16 value)
{
  return SCS_InstallEditBrowserCallback(selectMidiFile_Callback, getListMidiFile_Callback, "LOAD", 9, 2);
}


/////////////////////////////////////////////////////////////////////////////
// Parameter Access Functions
/////////////////////////////////////////////////////////////////////////////
static u16  dummyGet(u32 ix)              { return 0; }
static void dummySet(u32 ix, u16 value)   { }

static u16  dinGet(u32 ix)                { return selectedDin; }
static void dinSet(u32 ix, u16 value)     { selectedDin = value; }

static u16  dinModeGet(u32 ix)            { return midio_patch_din[selectedDin].mode; }
static void dinModeSet(u32 ix, u16 value) { midio_patch_din[selectedDin].mode = value; }

static u16  dinEvntGet(u32 ix)
{
  midio_patch_din_entry_t *din_cfg = (midio_patch_din_entry_t *)&midio_patch_din[selectedDin];
  switch( ix ) {
  case 0: return din_cfg->evnt0_on;
  case 1: return din_cfg->evnt1_on;
  case 2: return din_cfg->evnt2_on;
  case 3: return din_cfg->evnt0_off;
  case 4: return din_cfg->evnt1_off;
  case 5: return din_cfg->evnt2_off;
  }
  return 0; // error...
}
static void dinEvntSet(u32 ix, u16 value)
{
  midio_patch_din_entry_t *din_cfg = (midio_patch_din_entry_t *)&midio_patch_din[selectedDin];
  switch( ix ) {
  case 0: din_cfg->evnt0_on = value; break;
  case 1: din_cfg->evnt1_on = value; break;
  case 2: din_cfg->evnt2_on = value; break;
  case 3: din_cfg->evnt0_off = value; break;
  case 4: din_cfg->evnt1_off = value; break;
  case 5: din_cfg->evnt2_off = value; break;
  }
}

static u16  dinPortGet(u32 ix)
{
  midio_patch_din_entry_t *din_cfg = (midio_patch_din_entry_t *)&midio_patch_din[selectedDin];
  return (din_cfg->enabled_ports >> ix) & 0x1;
}
static void dinPortSet(u32 ix, u16 value)
{
  midio_patch_din_entry_t *din_cfg = (midio_patch_din_entry_t *)&midio_patch_din[selectedDin];
  din_cfg->enabled_ports &= ~(1 << ix);
  din_cfg->enabled_ports |= ((value&1) << ix);
}

static u16  doutGet(u32 ix)               { return selectedDout; }
static void doutSet(u32 ix, u16 value)    { selectedDout = value; }

static u16  doutEvntGet(u32 ix)
{
  midio_patch_dout_entry_t *dout_cfg = (midio_patch_dout_entry_t *)&midio_patch_dout[selectedDout];
  switch( ix ) {
  case 0: return dout_cfg->evnt0;
  case 1: return dout_cfg->evnt1;
  }
  return 0; // error...
}
static void doutEvntSet(u32 ix, u16 value)
{
  midio_patch_dout_entry_t *dout_cfg = (midio_patch_dout_entry_t *)&midio_patch_dout[selectedDout];
  switch( ix ) {
  case 0: dout_cfg->evnt0 = value; break;
  case 1: dout_cfg->evnt1 = value; break;
  }
}

static u16  doutPortGet(u32 ix)
{
  midio_patch_dout_entry_t *dout_cfg = (midio_patch_dout_entry_t *)&midio_patch_dout[selectedDout];
  return (dout_cfg->enabled_ports >> ix) & 1;
}
static void doutPortSet(u32 ix, u16 value)
{
  midio_patch_dout_entry_t *dout_cfg = (midio_patch_dout_entry_t *)&midio_patch_dout[selectedDout];
  dout_cfg->enabled_ports &= ~(1 << ix);
  dout_cfg->enabled_ports |= ((value&1) << ix);
}

static u16  matrixGet(u32 ix)                { return selectedMatrix; }
static void matrixSet(u32 ix, u16 value)     { selectedMatrix = value; }

static u16  matrixModeGet(u32 ix)            { return midio_patch_matrix[selectedMatrix].mode; }
static void matrixModeSet(u32 ix, u16 value) { midio_patch_matrix[selectedMatrix].mode = value; }

static u16  matrixPinGet(u32 ix)             { return selectedMatrixPin; }
static void matrixPinSet(u32 ix, u16 value)  { selectedMatrixPin = value; }

static u16  matrixChnGet(u32 ix)
{
  if( midio_patch_matrix[selectedMatrix].mode == MIDIO_PATCH_MATRIX_MODE_MAPPED )
    return (midio_patch_matrix[selectedMatrix].map_chn[selectedMatrixPin]-1) & 0xf;
  else
    return (midio_patch_matrix[selectedMatrix].chn-1) & 0xf;
}
static void matrixChnSet(u32 ix, u16 value)
{
  if( midio_patch_matrix[selectedMatrix].mode == MIDIO_PATCH_MATRIX_MODE_MAPPED )
    midio_patch_matrix[selectedMatrix].map_chn[selectedMatrixPin] = value+1;
  else
    midio_patch_matrix[selectedMatrix].chn = value+1;
}

static u16  matrixArgGet(u32 ix)
{
  if( midio_patch_matrix[selectedMatrix].mode == MIDIO_PATCH_MATRIX_MODE_MAPPED )
    return midio_patch_matrix[selectedMatrix].map_evnt1[selectedMatrixPin];
  else
    return midio_patch_matrix[selectedMatrix].arg;
}
static void matrixArgSet(u32 ix, u16 value)
{
  if( midio_patch_matrix[selectedMatrix].mode == MIDIO_PATCH_MATRIX_MODE_MAPPED )
    midio_patch_matrix[selectedMatrix].map_evnt1[selectedMatrixPin] = value;
  else
    midio_patch_matrix[selectedMatrix].arg = value;
}

static u16  matrixDinGet(u32 ix)             { return midio_patch_matrix[selectedMatrix].sr_din; }
static void matrixDinSet(u32 ix, u16 value)  { midio_patch_matrix[selectedMatrix].sr_din = value; }

static u16  matrixDoutGet(u32 ix)            { return midio_patch_matrix[selectedMatrix].sr_dout; }
static void matrixDoutSet(u32 ix, u16 value) { midio_patch_matrix[selectedMatrix].sr_dout = value; }

static u16  matrixPortGet(u32 ix)
{
  midio_patch_matrix_entry_t *m = (midio_patch_matrix_entry_t *)&midio_patch_matrix[selectedMatrix];
  return (m->enabled_ports >> ix) & 0x1;
}
static void matrixPortSet(u32 ix, u16 value)
{
  midio_patch_matrix_entry_t *m = (midio_patch_matrix_entry_t *)&midio_patch_matrix[selectedMatrix];
  m->enabled_ports &= ~(1 << ix);
  m->enabled_ports |= ((value&1) << ix);
}

static u16  ainGet(u32 ix)               { return selectedAin; }
static void ainSet(u32 ix, u16 value)    { selectedAin = value; }

static u16  ainEvntGet(u32 ix)
{
  midio_patch_ain_entry_t *ain_cfg = (midio_patch_ain_entry_t *)&midio_patch_ain[selectedAin];
  switch( ix ) {
  case 0: return ain_cfg->evnt0;
  case 1: return ain_cfg->evnt1;
  }
  return 0; // error...
}
static void ainEvntSet(u32 ix, u16 value)
{
  midio_patch_ain_entry_t *ain_cfg = (midio_patch_ain_entry_t *)&midio_patch_ain[selectedAin];
  switch( ix ) {
  case 0: ain_cfg->evnt0 = value; break;
  case 1: ain_cfg->evnt1 = value; break;
  }
}

static u16  ainPortGet(u32 ix)
{
  midio_patch_ain_entry_t *ain_cfg = (midio_patch_ain_entry_t *)&midio_patch_ain[selectedAin];
  return (ain_cfg->enabled_ports >> ix) & 1;
}
static void ainPortSet(u32 ix, u16 value)
{
  midio_patch_ain_entry_t *ain_cfg = (midio_patch_ain_entry_t *)&midio_patch_ain[selectedAin];
  ain_cfg->enabled_ports &= ~(1 << ix);
  ain_cfg->enabled_ports |= ((value&1) << ix);
}


static u16  ainserGet(u32 ix)               { return selectedAinser; }
static void ainserSet(u32 ix, u16 value)    { selectedAinser = value; }

static u16  ainserEvntGet(u32 ix)
{
  midio_patch_ain_entry_t *ain_cfg = (midio_patch_ain_entry_t *)&midio_patch_ainser[selectedAinser];
  switch( ix ) {
  case 0: return ain_cfg->evnt0;
  case 1: return ain_cfg->evnt1;
  }
  return 0; // error...
}
static void ainserEvntSet(u32 ix, u16 value)
{
  midio_patch_ain_entry_t *ain_cfg = (midio_patch_ain_entry_t *)&midio_patch_ainser[selectedAinser];
  switch( ix ) {
  case 0: ain_cfg->evnt0 = value; break;
  case 1: ain_cfg->evnt1 = value; break;
  }
}

static u16  ainserPortGet(u32 ix)
{
  midio_patch_ain_entry_t *ain_cfg = (midio_patch_ain_entry_t *)&midio_patch_ainser[selectedAinser];
  return (ain_cfg->enabled_ports >> ix) & 1;
}
static void ainserPortSet(u32 ix, u16 value)
{
  midio_patch_ain_entry_t *ain_cfg = (midio_patch_ain_entry_t *)&midio_patch_ainser[selectedAinser];
  ain_cfg->enabled_ports &= ~(1 << ix);
  ain_cfg->enabled_ports |= ((value&1) << ix);
}


static u16  routerNodeGet(u32 ix)             { return selectedRouterNode; }
static void routerNodeSet(u32 ix, u16 value)  { selectedRouterNode = value; }

static u16  routerSrcPortGet(u32 ix)             { return MIDI_PORT_InIxGet(midi_router_node[selectedRouterNode].src_port); }
static void routerSrcPortSet(u32 ix, u16 value)  { midi_router_node[selectedRouterNode].src_port = MIDI_PORT_InPortGet(value); }

static u16  routerSrcChnGet(u32 ix)              { return midi_router_node[selectedRouterNode].src_chn; }
static void routerSrcChnSet(u32 ix, u16 value)   { midi_router_node[selectedRouterNode].src_chn = value; }

static u16  routerDstPortGet(u32 ix)             { return MIDI_PORT_OutIxGet(midi_router_node[selectedRouterNode].dst_port); }
static void routerDstPortSet(u32 ix, u16 value)  { midi_router_node[selectedRouterNode].dst_port = MIDI_PORT_OutPortGet(value); }

static u16  routerDstChnGet(u32 ix)              { return midi_router_node[selectedRouterNode].dst_chn; }
static void routerDstChnSet(u32 ix, u16 value)   { midi_router_node[selectedRouterNode].dst_chn = value; }

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

static u16  midiPlayModeGet(u32 ix)            { return SEQ_MidiPlayModeGet(); }
static void midiPlayModeSet(u32 ix, u16 value) { SEQ_MidiPlayModeSet(value); }



static void MSD_EnableReq(u32 enable)
{
  TASK_MSD_EnableSet(enable);
  SCS_Msg(SCS_MSG_L, 1000, "Mass Storage", enable ? "enabled!" : "disabled!");
}

/////////////////////////////////////////////////////////////////////////////
// Menu Structure
/////////////////////////////////////////////////////////////////////////////

const scs_menu_item_t pageDIN[] = {
  SCS_ITEM("Pin# ", 0, MIDIO_PATCH_NUM_DIN-1, dinGet,         dinSet,         selectNOP, stringDec,  NULL),
  SCS_ITEM(" DIN ", 0, MIDIO_PATCH_NUM_DIN-1, dinGet,         dinSet,         selectNOP, stringDIN_SR,  NULL),
  SCS_ITEM("Mode ", 0, 2,           dinModeGet,      dinModeSet,      selectNOP, stringDIN_Mode,NULL),
  SCS_ITEM("E0On ", 0, 0x7f,        dinEvntGet,      dinEvntSet,      selectNOP, stringHex2O80, NULL),
  SCS_ITEM("E1On ", 1, 0x7f,        dinEvntGet,      dinEvntSet,      selectNOP, stringHex2, NULL),
  SCS_ITEM("E2On ", 2, 0x7f,        dinEvntGet,      dinEvntSet,      selectNOP, stringHex2, NULL),
  SCS_ITEM("E0Of ", 3, 0x7f,        dinEvntGet,      dinEvntSet,      selectNOP, stringHex2O80, NULL),
  SCS_ITEM("E1Of ", 4, 0x7f,        dinEvntGet,      dinEvntSet,      selectNOP, stringHex2, NULL),
  SCS_ITEM("E2Of ", 5, 0x7f,        dinEvntGet,      dinEvntSet,      selectNOP, stringHex2, NULL),
  SCS_ITEM("USB1 ", 0, 1,           dinPortGet,      dinPortSet,      selectNOP, stringOnOff, NULL),
#if MIOS32_USB_MIDI_NUM_PORTS >= 2
  SCS_ITEM("USB2 ", 1, 1,           dinPortGet,      dinPortSet,      selectNOP, stringOnOff, NULL),
#endif
#if MIOS32_USB_MIDI_NUM_PORTS >= 3
  SCS_ITEM("USB3 ", 2, 1,           dinPortGet,      dinPortSet,      selectNOP, stringOnOff, NULL),
#endif
#if MIOS32_USB_MIDI_NUM_PORTS >= 4
  SCS_ITEM("USB4 ", 3, 1,           dinPortGet,      dinPortSet,      selectNOP, stringOnOff, NULL),
#endif
  SCS_ITEM("OUT1 ", 4, 1,           dinPortGet,      dinPortSet,      selectNOP, stringOnOff, NULL),
  SCS_ITEM("OUT2 ", 5, 1,           dinPortGet,      dinPortSet,      selectNOP, stringOnOff, NULL),
#if MIOS32_UART_NUM >= 3
  SCS_ITEM("OUT3 ", 6, 1,           dinPortGet,      dinPortSet,      selectNOP, stringOnOff, NULL),
#endif
#if MIOS32_UART_NUM >= 4
  SCS_ITEM("OUT4 ", 7, 1,           dinPortGet,      dinPortSet,      selectNOP, stringOnOff, NULL),
#endif
  SCS_ITEM("OSC1 ",12, 1,           dinPortGet,      dinPortSet,      selectNOP, stringOnOff, NULL),
  SCS_ITEM("OSC2 ",13, 1,           dinPortGet,      dinPortSet,      selectNOP, stringOnOff, NULL),
  SCS_ITEM("OSC3 ",14, 1,           dinPortGet,      dinPortSet,      selectNOP, stringOnOff, NULL),
  SCS_ITEM("OSC4 ",15, 1,           dinPortGet,      dinPortSet,      selectNOP, stringOnOff, NULL),
};

const scs_menu_item_t pageDOUT[] = {
  SCS_ITEM("Pin# ", 0, MIDIO_PATCH_NUM_DOUT-1, doutGet,         doutSet,         selectNOP, stringDec,  NULL),
  SCS_ITEM("DOUT ", 0, MIDIO_PATCH_NUM_DOUT-1, doutGet,         doutSet,         selectNOP, stringDOUT_SR,  NULL),
  SCS_ITEM("Evn0 ", 0, 0x7f,        doutEvntGet,     doutEvntSet,     selectNOP, stringHex2O80, NULL),
  SCS_ITEM("Evn1 ", 1, 0x7f,        doutEvntGet,     doutEvntSet,     selectNOP, stringHex2, NULL),
  SCS_ITEM("USB1 ", 0, 1,           doutPortGet,     doutPortSet,     selectNOP, stringOnOff, NULL),
#if MIOS32_USB_MIDI_NUM_PORTS >= 2
  SCS_ITEM("USB2 ", 1, 1,           doutPortGet,     doutPortSet,     selectNOP, stringOnOff, NULL),
#endif
#if MIOS32_USB_MIDI_NUM_PORTS >= 3
  SCS_ITEM("USB3 ", 2, 1,           doutPortGet,     doutPortSet,     selectNOP, stringOnOff, NULL),
#endif
#if MIOS32_USB_MIDI_NUM_PORTS >= 4
  SCS_ITEM("USB4 ", 3, 1,           doutPortGet,     doutPortSet,     selectNOP, stringOnOff, NULL),
#endif
  SCS_ITEM(" IN1 ", 4, 1,           doutPortGet,     doutPortSet,     selectNOP, stringOnOff, NULL),
  SCS_ITEM(" IN2 ", 5, 1,           doutPortGet,     doutPortSet,     selectNOP, stringOnOff, NULL),
  SCS_ITEM(" IN3 ", 6, 1,           doutPortGet,     doutPortSet,     selectNOP, stringOnOff, NULL),
  SCS_ITEM("OSC1 ",12, 1,           doutPortGet,     doutPortSet,     selectNOP, stringOnOff, NULL),
  SCS_ITEM("OSC2 ",13, 1,           doutPortGet,     doutPortSet,     selectNOP, stringOnOff, NULL),
  SCS_ITEM("OSC3 ",14, 1,           doutPortGet,     doutPortSet,     selectNOP, stringOnOff, NULL),
  SCS_ITEM("OSC4 ",15, 1,           doutPortGet,     doutPortSet,     selectNOP, stringOnOff, NULL),
};

const scs_menu_item_t pageM8x8[] = {
  SCS_ITEM("Mat. ", 0, MIDIO_PATCH_NUM_MATRIX-1, matrixGet, matrixSet,selectNOP, stringDecP1, NULL),
  SCS_ITEM("Mode ", 0,  1,          matrixModeGet,   matrixModeSet,    selectNOP, stringMatrixMode, NULL),
  SCS_ITEM("Pin  ", 0, 63,          matrixPinGet,    matrixPinSet,    selectNOP, stringMatrixPin, NULL),
  SCS_ITEM("Chn. ", 0, 15,          matrixChnGet,    matrixChnSet,    selectNOP, stringDecP1, NULL),
  SCS_ITEM("Note ", 0, 127,         matrixArgGet,    matrixArgSet,    selectNOP, stringNote,  NULL),
  SCS_ITEM("DIN  ", 0, 16,          matrixDinGet,    matrixDinSet,    selectNOP, stringDec0Dis, NULL),
  SCS_ITEM("DOUT ", 0, 16,          matrixDoutGet,   matrixDoutSet,   selectNOP, stringDec0Dis, NULL),
  SCS_ITEM("USB1 ", 0, 1,           matrixPortGet,   matrixPortSet,   selectNOP, stringOnOff, NULL),
#if MIOS32_USB_MIDI_NUM_PORTS >= 2
  SCS_ITEM("USB2 ", 1, 1,           matrixPortGet,   matrixPortSet,   selectNOP, stringOnOff, NULL),
#endif
#if MIOS32_USB_MIDI_NUM_PORTS >= 3
  SCS_ITEM("USB3 ", 2, 1,           matrixPortGet,   matrixPortSet,   selectNOP, stringOnOff, NULL),
#endif
#if MIOS32_USB_MIDI_NUM_PORTS >= 4
  SCS_ITEM("USB4 ", 3, 1,           matrixPortGet,   matrixPortSet,   selectNOP, stringOnOff, NULL),
#endif
  SCS_ITEM("OUT1 ", 4, 1,           matrixPortGet,   matrixPortSet,   selectNOP, stringOnOff, NULL),
  SCS_ITEM("OUT2 ", 5, 1,           matrixPortGet,   matrixPortSet,   selectNOP, stringOnOff, NULL),
#if MIOS32_UART_NUM >= 3
  SCS_ITEM("OUT3 ", 6, 1,           matrixPortGet,   matrixPortSet,   selectNOP, stringOnOff, NULL),
#endif
#if MIOS32_UART_NUM >= 4
  SCS_ITEM("OUT4 ", 6, 1,           matrixPortGet,   matrixPortSet,   selectNOP, stringOnOff, NULL),
#endif
  SCS_ITEM("OSC1 ",12, 1,           matrixPortGet,   matrixPortSet,   selectNOP, stringOnOff, NULL),
  SCS_ITEM("OSC2 ",13, 1,           matrixPortGet,   matrixPortSet,   selectNOP, stringOnOff, NULL),
  SCS_ITEM("OSC3 ",14, 1,           matrixPortGet,   matrixPortSet,   selectNOP, stringOnOff, NULL),
  SCS_ITEM("OSC4 ",15, 1,           matrixPortGet,   matrixPortSet,   selectNOP, stringOnOff, NULL),
};

const scs_menu_item_t pageAIN[] = {
  SCS_ITEM("Pin# ", 0, MIDIO_PATCH_NUM_AIN-1, ainGet,   ainSet,     selectNOP, stringDec,  NULL),
  SCS_ITEM("Evn0 ", 0, 0x7f,        ainEvntGet,     ainEvntSet,     selectNOP, stringHex2O80, NULL),
  SCS_ITEM("Evn1 ", 1, 0x7f,        ainEvntGet,     ainEvntSet,     selectNOP, stringHex2, NULL),
  SCS_ITEM("USB1 ", 0, 1,           ainPortGet,     ainPortSet,     selectNOP, stringOnOff, NULL),
#if MIOS32_USB_MIDI_NUM_PORTS >= 2
  SCS_ITEM("USB2 ", 1, 1,           ainPortGet,     ainPortSet,     selectNOP, stringOnOff, NULL),
#endif
#if MIOS32_USB_MIDI_NUM_PORTS >= 3
  SCS_ITEM("USB3 ", 2, 1,           ainPortGet,     ainPortSet,     selectNOP, stringOnOff, NULL),
#endif
#if MIOS32_USB_MIDI_NUM_PORTS >= 4
  SCS_ITEM("USB4 ", 3, 1,           ainPortGet,     ainPortSet,     selectNOP, stringOnOff, NULL),
#endif
  SCS_ITEM("OUT1 ", 4, 1,           ainPortGet,     ainPortSet,     selectNOP, stringOnOff, NULL),
  SCS_ITEM("OUT2 ", 5, 1,           ainPortGet,     ainPortSet,     selectNOP, stringOnOff, NULL),
  SCS_ITEM("OUT3 ", 6, 1,           ainPortGet,     ainPortSet,     selectNOP, stringOnOff, NULL),
  SCS_ITEM("OSC1 ",12, 1,           ainPortGet,     ainPortSet,     selectNOP, stringOnOff, NULL),
  SCS_ITEM("OSC2 ",13, 1,           ainPortGet,     ainPortSet,     selectNOP, stringOnOff, NULL),
  SCS_ITEM("OSC3 ",14, 1,           ainPortGet,     ainPortSet,     selectNOP, stringOnOff, NULL),
  SCS_ITEM("OSC4 ",15, 1,           ainPortGet,     ainPortSet,     selectNOP, stringOnOff, NULL),
};

const scs_menu_item_t pageAINSER[] = {
  SCS_ITEM("Pin# ", 0, MIDIO_PATCH_NUM_AINSER-1, ainserGet, ainserSet,    selectNOP, stringDec,  NULL),
  SCS_ITEM("AINS",  0, MIDIO_PATCH_NUM_AINSER-1, ainserGet, ainserSet,    selectNOP, stringAINSER_Pin,  NULL),
  SCS_ITEM("Evn0 ", 0, 0x7f,        ainserEvntGet,     ainserEvntSet,     selectNOP, stringHex2O80, NULL),
  SCS_ITEM("Evn1 ", 1, 0x7f,        ainserEvntGet,     ainserEvntSet,     selectNOP, stringHex2, NULL),
  SCS_ITEM("USB1 ", 0, 1,           ainserPortGet,     ainserPortSet,     selectNOP, stringOnOff, NULL),
#if MIOS32_USB_MIDI_NUM_PORTS >= 2
  SCS_ITEM("USB2 ", 1, 1,           ainserPortGet,     ainserPortSet,     selectNOP, stringOnOff, NULL),
#endif
#if MIOS32_USB_MIDI_NUM_PORTS >= 3
  SCS_ITEM("USB3 ", 2, 1,           ainserPortGet,     ainserPortSet,     selectNOP, stringOnOff, NULL),
#endif
#if MIOS32_USB_MIDI_NUM_PORTS >= 4
  SCS_ITEM("USB4 ", 3, 1,           ainserPortGet,     ainserPortSet,     selectNOP, stringOnOff, NULL),
#endif
  SCS_ITEM("OUT1 ", 4, 1,           ainserPortGet,     ainserPortSet,     selectNOP, stringOnOff, NULL),
  SCS_ITEM("OUT2 ", 5, 1,           ainserPortGet,     ainserPortSet,     selectNOP, stringOnOff, NULL),
  SCS_ITEM("OUT3 ", 6, 1,           ainserPortGet,     ainserPortSet,     selectNOP, stringOnOff, NULL),
  SCS_ITEM("OSC1 ",12, 1,           ainserPortGet,     ainserPortSet,     selectNOP, stringOnOff, NULL),
  SCS_ITEM("OSC2 ",13, 1,           ainserPortGet,     ainserPortSet,     selectNOP, stringOnOff, NULL),
  SCS_ITEM("OSC3 ",14, 1,           ainserPortGet,     ainserPortSet,     selectNOP, stringOnOff, NULL),
  SCS_ITEM("OSC4 ",15, 1,           ainserPortGet,     ainserPortSet,     selectNOP, stringOnOff, NULL),
};

const scs_menu_item_t pageROUT[] = {
  SCS_ITEM("Node", 0, MIDI_ROUTER_NUM_NODES-1,  routerNodeGet, routerNodeSet,selectNOP, stringDecP1, NULL),
  SCS_ITEM("SrcP", 0, MIDI_PORT_NUM_IN_PORTS-1, routerSrcPortGet, routerSrcPortSet,selectNOP, stringInPort, NULL),
  SCS_ITEM("Chn.", 0, 17,                       routerSrcChnGet, routerSrcChnSet,selectNOP, stringRouterChn, NULL),
  SCS_ITEM("SrcD", 0, MIDI_PORT_NUM_OUT_PORTS-1, routerDstPortGet, routerDstPortSet,selectNOP, stringOutPort, NULL),
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

const scs_menu_item_t pageMIDI[] = {
  SCS_ITEM("Mode ", 0, 1,           midiPlayModeGet, midiPlayModeSet, selectNOP,  stringMidiMode, NULL),
  SCS_ITEM("Filen", 0, 0,           dummyGet,        dummySet,        selectMidiFile, stringMidiFileName, NULL),
  SCS_ITEM("ame  ", 1, 0,           dummyGet,        dummySet,        selectMidiFile, stringMidiFileName, NULL),
  SCS_ITEM("     ", 2, 0,           dummyGet,        dummySet,        selectMidiFile, stringMidiFileName, NULL),
};

const scs_menu_item_t pageMON[] = {
  // dummy - will be overlayed in displayHook
  SCS_ITEM("     ", 0, 0,           dummyGet,        dummySet,        selectNOP,      stringEmpty, NULL),
};

const scs_menu_page_t rootMode0[] = {
  SCS_PAGE("DIN  ", pageDIN),
  SCS_PAGE("DOUT ", pageDOUT),
  SCS_PAGE("M8x8 ", pageM8x8),
  SCS_PAGE("AIN  ", pageAIN),
  SCS_PAGE("AINS ", pageAINSER),
  SCS_PAGE("Rout ", pageROUT),
  SCS_PAGE("OSC  ", pageOSC),
  SCS_PAGE("Netw ", pageNetw),
  SCS_PAGE("MIDI ", pageMIDI),
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

    sprintf(line1, "MIDI DOUT MSD  ");
    sprintf(line2, "%s Off  %s",
	    SEQ_BPM_IsRunning() ? "STOP" : (SEQ_RecordingEnabled() ? "REC!" : "PLAY"),
	    TASK_MSD_EnableGet() ? msdStr : "----");
    return 1;
  }

  // overlay in MSD mode (user should disable soon since this sucks performance)
  if( TASK_MSD_EnableGet() ) {
    char msdStr[5];
    TASK_MSD_FlagStrGet(msdStr);

    sprintf(line1, "[ MSD ACTIVE: %s ]", msdStr);
  }

  if( SCS_MenuStateGet() == SCS_MENU_STATE_MAINPAGE ) {
    u8 fastRefresh = line1[0] == 0;
    u8 exitButtonPressed = SCS_PinGet(SCS_PIN_EXIT) ? 0 : 1;
    u32 tick = SEQ_BPM_TickGet();
    u32 ticks_per_step = SEQ_BPM_PPQN_Get() / 4;
    u32 ticks_per_measure = ticks_per_step * 16;
    u32 measure = (tick / ticks_per_measure) + 1;
    u32 step = ((tick % ticks_per_measure) / ticks_per_step) + 1;

    // print SD Card status message or patch
    if( line1[0] == 0 ) { // no MSD overlay?
      if( SEQ_RecordingEnabled() ) {
	if( (tick % (8*ticks_per_step)) >= (4*ticks_per_step) ) {
	  sprintf(line1, "*RECORDING*  %4u.%2d", measure, step);
	} else {
	  sprintf(line1, "%-12s %4u.%2d", SEQ_RecordingFilename(), measure, step);
	}
      } else {
	sprintf(line1, "%-12s %4u.%2d", MIDIO_FILE_StatusMsgGet() ? MIDIO_FILE_StatusMsgGet() : MID_FILE_UI_NameGet(), measure, step);
      }
    }

    if( SEQ_RecordingEnabled() ) {
      mios32_midi_port_t port = SEQ_LastRecordedPort();
      mios32_midi_package_t p = SEQ_LastRecordedEvent();
      char event_str[12];
      event_str[0] = 0;

      int time_passed = tick - SEQ_LastRecordedTick();
      if( time_passed > 0 && time_passed < (ticks_per_measure/4) ) {
	sprintf(event_str, "%s:%02X%02X%02X", MIDI_PORT_OutNameGet(MIDI_PORT_OutIxGet(port)), p.evnt0, p.evnt1, p.evnt2);
      }

      sprintf(line2, "%s  %-11s   ", SEQ_BPM_IsRunning() ? "STOP" : "REC ", event_str[0] ? event_str : "----:------" );
    } else if( exitButtonPressed ) {
      sprintf(line2, "%s FRew FFwd      ", SEQ_BPM_IsRunning() ? "STOP" : "REC ");
    } else {
      sprintf(line2, "%s   <    >   MENU", SEQ_BPM_IsRunning() ? "STOP" : "PLAY");
    }

    // request LCD update - this will lead to fast refresh rate in main screen
    if( fastRefresh )
      SCS_DisplayUpdateRequest();

    return 1;
  }

  if( SCS_MenuStateGet() == SCS_MENU_STATE_SELECT_PAGE ) {
    if( line1[0] == 0 ) { // no MSD overlay?
      if( MIDIO_FILE_StatusMsgGet() )
	sprintf(line1, MIDIO_FILE_StatusMsgGet());
      else
	sprintf(line1, "Patch: %s", midio_file_p_patch_name);
    }
    return 1;
  }

  if( SCS_MenuPageGet() == pageDsk ) {
    // Disk page: we want to show the patch at upper line, and menu items at lower line
    if( line1[0] == 0 ) { // no MSD overlay?
      if( MIDIO_FILE_StatusMsgGet() )
	sprintf(line1, MIDIO_FILE_StatusMsgGet());
      else
	sprintf(line1, "Patch: %s", midio_file_p_patch_name);
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
    if( scsButton == SCS_PIN_SOFT5 && depressed ) // selects/deselects extra page
      extraPage = 0;
    else {
      switch( scsButton ) {
      case SCS_PIN_SOFT1:
	if( depressed )
	  return 1;
	SEQ_PlayStopButton();
	break;

      case SCS_PIN_SOFT2:
	if( depressed )
	  return 1;
	MIDIO_DOUT_Init(0);
	SCS_Msg(SCS_MSG_L, 1000, "All DOUT pins", "deactivated");
	break;

      case SCS_PIN_SOFT3: {
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

      u8 exitButtonPressed = SCS_PinGet(SCS_PIN_EXIT) ? 0 : 1;

      if( exitButtonPressed || SEQ_RecordingEnabled() ) {
	switch( scsButton ) {
	case SCS_PIN_SOFT1: // Rec/Stop
	  SEQ_RecStopButton();
	  return 1;

	case SCS_PIN_SOFT2: { // Fast Rewind
	  SEQ_FRewButton();
	  return 1;
	}

	case SCS_PIN_SOFT3: { // Fast Forward
	  SEQ_FFwdButton();
	  return 1;
	}

	case SCS_PIN_SOFT4: { // Menu disabled
	  return 1;
	}
	}
      } else {
	switch( scsButton ) {
	case SCS_PIN_SOFT1: // Play/Stop
	  SEQ_PlayStopButton();
	  return 1;

	case SCS_PIN_SOFT2: { // previous song
	  MUTEX_SDCARD_TAKE;
	  SEQ_PlayFileReq(-1, 1);
	  MUTEX_SDCARD_GIVE;
	  return 1;
	}

	case SCS_PIN_SOFT3: { // next song
	  MUTEX_SDCARD_TAKE;
	  SEQ_PlayFileReq(1, 1);
	  MUTEX_SDCARD_GIVE;
	  return 1;
	}
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
