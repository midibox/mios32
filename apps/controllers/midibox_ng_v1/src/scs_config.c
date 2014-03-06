// $Id$
//! \defgroup SCS_CONFIG
//! Local SCS Configuration
//! \{
/* ==========================================================================
 *
 *  Copyright (C) 2012 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

/////////////////////////////////////////////////////////////////////////////
//! Include files
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
#include <scs_lcd.h>
#include "scs_config.h"

#include <seq_bpm.h>

#include "mbng_dout.h"

#include <file.h>
#include "mbng_file.h"
#include "mbng_file_c.h"
#include "mbng_file_s.h"
#include "mbng_file_r.h"
#include "mbng_patch.h"
#include "mbng_lcd.h"
#include "mbng_enc.h"
#include "mbng_din.h"
#include "mbng_seq.h"



/////////////////////////////////////////////////////////////////////////////
//! Local variables
/////////////////////////////////////////////////////////////////////////////

static u8 extraPage;

static u8 selectedRouterNode;
static u8 selectedIpPar;
static u8 selectedOscPort;
static u8 monPageOffset;


/////////////////////////////////////////////////////////////////////////////
//! String Conversion Functions
/////////////////////////////////////////////////////////////////////////////
static void stringEmpty(u32 ix, u16 value, char *label)   { label[0] = 0; }
static void stringDec(u32 ix, u16 value, char *label)    { sprintf(label, "%3d  ", value); }
static void stringDecP1(u32 ix, u16 value, char *label)  { sprintf(label, "%3d  ", value+1); }
//static void stringDecPM(u32 ix, u16 value, char *label)  { sprintf(label, "%3d  ", (int)value - 64); }
//static void stringDec03(u32 ix, u16 value, char *label)  { sprintf(label, "%03d  ", value); }
//static void stringDec0Dis(u32 ix, u16 value, char *label){ sprintf(label, value ? "%3d  " : "---  ", value); }
//static void stringDec4(u32 ix, u16 value, char *label)   { sprintf(label, "%4d ", value); }
static void stringDec5(u32 ix, u16 value, char *label)   { sprintf(label, "%5d", value); }
//static void stringHex2(u32 ix, u16 value, char *label)    { sprintf(label, " %02X  ", value); }
//static void stringHex2O80(u32 ix, u16 value, char *label) { sprintf(label, " %02X  ", value | 0x80); }
static void stringOnOff(u32 ix, u16 value, char *label)  { sprintf(label, " [%c] ", value ? 'x' : ' '); }

//static void stringNote(u32 ix, u16 value, char *label)
//{
//  const char noteTab[12][3] = { "C-", "C#", "D-", "D#", "E-", "F-", "F#", "G-", "G#", "A-", "A#", "B-" };
//
//  // print "---" if note number is 0
//  if( value == 0 )
//    sprintf(label, "---  ");
//  else {
//    u8 octave = value / 12;
//    u8 note = value % 12;
//
//    // print semitone and octave (-2): up to 4 chars
//    sprintf(label, "%s%d  ",
//	    noteTab[note],
//	    (int)octave-2);
//  }
//}

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

static void stringPlay(u32 ix, u16 value, char *label)
{
  sprintf(label, SEQ_BPM_IsRunning() ? " ** " : "    ");
}

static void stringStop(u32 ix, u16 value, char *label)
{
  sprintf(label, SEQ_BPM_IsRunning() ? "    " : " ** ");
}

static void stringPause(u32 ix, u16 value, char *label)
{
  sprintf(label, MBNG_SEQ_PauseEnabled() ? " ** " : "    ");
}


/////////////////////////////////////////////////////////////////////////////
//! Parameter Selection Functions
/////////////////////////////////////////////////////////////////////////////
static u16  selectNOP(u32 ix, u16 value)    { return value; }

static u16  selectRunScript(u32 ix, u16 value)
{
  if( !MBNG_FILE_R_Valid() ) {
    char buffer[21];
    sprintf(buffer, "%s.NGR!", mbng_file_r_script_name);
    SCS_Msg(SCS_MSG_L, 1000, "Missing", buffer);
  } else {
    char buffer[21];
    sprintf(buffer, "%s.NGR %3d %3d", mbng_file_r_script_name, MBNG_FILE_R_VarSectionGet(), MBNG_FILE_R_VarValueGet());
    SCS_Msg(SCS_MSG_L, 1000, "Executed", buffer);
    MBNG_FILE_R_ReadRequest(NULL, MBNG_FILE_R_VarSectionGet(), MBNG_FILE_R_VarValueGet(), 0);
  }
  return value;
}

static u16  selectSnapshotLOAD(u32 ix, u16 value)
{
  s32 status;
  if( (status=MBNG_FILE_S_Read(mbng_file_s_patch_name, MBNG_FILE_S_SnapshotGet())) < 0 ) {
    SCS_Msg(SCS_MSG_ERROR_L, 1000, "Failed to load", "Snapshot");
  } else {
    char buffer[100];
    sprintf(buffer, "Snapshot %d", MBNG_FILE_S_SnapshotGet());
    SCS_Msg(SCS_MSG_L, 1000, buffer, (status == 0) ? "not filed yet" : "restored!");
  }
  return value;
}

static u16  selectSnapshotSAVE(u32 ix, u16 value)
{
  if( MBNG_FILE_S_Write(mbng_file_s_patch_name, MBNG_FILE_S_SnapshotGet()) < 0 ) {
    SCS_Msg(SCS_MSG_ERROR_L, 1000, "Failed to save", "Snapshot");
  } else {
    char buffer[100];
    sprintf(buffer, "Snapshot %d", MBNG_FILE_S_SnapshotGet());
    SCS_Msg(SCS_MSG_L, 1000, buffer, "stored!");
  }
  return value;
}

static u16  selectSnapshotDUMP(u32 ix, u16 value)
{
  MBNG_EVENT_Dump();
  SCS_Msg(SCS_MSG_L, 1000, "All Values", "dumped!");
  return value;
}


static void selectSAVE_Callback(char *newString)
{
  s32 status;

  if( (status=MBNG_PATCH_Store(newString)) < 0 ) {
    char buffer[100];
    sprintf(buffer, "Config %s", newString);
    SCS_Msg(SCS_MSG_ERROR_L, 1000, "Failed to store", buffer);
  } else {
    char buffer[100];
    sprintf(buffer, "Config %s", newString);
    SCS_Msg(SCS_MSG_L, 1000, buffer, "stored!");
  }
}
static u16  selectSAVE(u32 ix, u16 value)
{
  return SCS_InstallEditStringCallback(selectSAVE_Callback, "SAVE", mbng_file_c_config_name, MBNG_FILE_C_FILENAME_LEN);
}

static void selectLOAD_Callback(char *newString)
{
  s32 status;

  if( newString[0] != 0 ) {
    if( (status=MBNG_PATCH_Load(newString)) < 0 ) {
      char buffer[100];
      sprintf(buffer, "Config %s", newString);
      SCS_Msg(SCS_MSG_ERROR_L, 1000, "Failed to load", buffer);
    } else {
      char buffer[100];
      sprintf(buffer, "Config %s", newString);
      SCS_Msg(SCS_MSG_L, 1000, buffer, "loaded!");
    }
  }
}
static u8 getListLOAD_Callback(u8 offset, char *line)
{
  MUTEX_SDCARD_TAKE;
  s32 status = FILE_GetFiles("/", "NGC", line, 2, offset);
  MUTEX_SDCARD_GIVE;
  if( status < 1 ) {
    sprintf(line, "<no .NGC files>");
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

static u16  selectPlay(u32 ix, u16 value)
{
  MBNG_SEQ_PlayButton();
  return 0;
}

static u16  selectStop(u32 ix, u16 value)
{
  MBNG_SEQ_StopButton();
  return 0;
}

static u16  selectPause(u32 ix, u16 value)
{
  MBNG_SEQ_PauseButton();
  return 0;
}


/////////////////////////////////////////////////////////////////////////////
//! Parameter Access Functions
/////////////////////////////////////////////////////////////////////////////
static u16  dummyGet(u32 ix)              { return 0; }
static void dummySet(u32 ix, u16 value)   { }

static u16  sysExVarDevGet(u32 ix)             { return mbng_patch_cfg.sysex_dev; }
static void sysExVarDevSet(u32 ix, u16 value)  { mbng_patch_cfg.sysex_dev = value; }
static u16  sysExVarPatGet(u32 ix)             { return mbng_patch_cfg.sysex_pat; }
static void sysExVarPatSet(u32 ix, u16 value)  { mbng_patch_cfg.sysex_pat = value; }
static u16  sysExVarBnkGet(u32 ix)             { return mbng_patch_cfg.sysex_bnk; }
static void sysExVarBnkSet(u32 ix, u16 value)  { mbng_patch_cfg.sysex_bnk = value; }
static u16  sysExVarInsGet(u32 ix)             { return mbng_patch_cfg.sysex_ins; }
static void sysExVarInsSet(u32 ix, u16 value)  { mbng_patch_cfg.sysex_ins = value; }
static u16  sysExVarChnGet(u32 ix)             { return mbng_patch_cfg.sysex_chn; }
static void sysExVarChnSet(u32 ix, u16 value)  { mbng_patch_cfg.sysex_chn = value; }

static u16  runSectionGet(u32 ix)              { return MBNG_FILE_R_VarSectionGet(); }
static void runSectionSet(u32 ix, u16 value)   { MBNG_FILE_R_VarSectionSet(value); }

static u16  runValueGet(u32 ix)                { return MBNG_FILE_R_VarValueGet(); }
static void runValueSet(u32 ix, u16 value)     { MBNG_FILE_R_VarValueSet(value); }

static u16  snapshotGet(u32 ix)                { return MBNG_FILE_S_SnapshotGet(); }
static void snapshotSet(u32 ix, u16 value)     { MBNG_FILE_S_SnapshotSet(value); }

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

static u16  mclkBpmGet(u32 ix)                { return (u16)SEQ_BPM_Get(); }
static void mclkBpmSet(u32 ix, u16 value)     { SEQ_BPM_Set((float)value); }


static void MSD_EnableReq(u32 enable)
{
  TASK_MSD_EnableSet(enable);
  SCS_Msg(SCS_MSG_L, 1000, "Mass Storage", enable ? "enabled!" : "disabled!");
}

/////////////////////////////////////////////////////////////////////////////
//! Menu Structure
/////////////////////////////////////////////////////////////////////////////

const scs_menu_item_t pageVAR[] = {
  SCS_ITEM("Dev ", 0, 127, sysExVarDevGet, sysExVarDevSet, selectNOP, stringDec, NULL),
  SCS_ITEM("Pat ", 0, 127, sysExVarPatGet, sysExVarPatSet, selectNOP, stringDec, NULL),
  SCS_ITEM("Bnk ", 0, 127, sysExVarBnkGet, sysExVarBnkSet, selectNOP, stringDec, NULL),
  SCS_ITEM("Ins ", 0, 127, sysExVarInsGet, sysExVarInsSet, selectNOP, stringDec, NULL),
  SCS_ITEM("Chn ", 0, 127, sysExVarChnGet, sysExVarChnSet, selectNOP, stringDec, NULL),
};

const scs_menu_item_t pageRun[] = {
  SCS_ITEM("Run ",  0, 0,    dummyGet,      dummySet,      selectRunScript, stringEmpty, NULL),
  SCS_ITEM("Sec.",  0, 255,  runSectionGet, runSectionSet, selectNOP,    stringDec, NULL),
  SCS_ITEM("Val.",  0, 255,  runValueGet,   runValueSet,   selectNOP,    stringDec, NULL),
};

const scs_menu_item_t pageSnap[] = {
  SCS_ITEM("Snap",  0, MBNG_FILE_S_NUM_SNAPSHOTS-1, snapshotGet, snapshotSet, selectNOP,    stringDec, NULL),
  SCS_ITEM("Load ", 0, 0,                           dummyGet,    dummySet,    selectSnapshotLOAD, stringEmpty, NULL),
  SCS_ITEM("Save ", 0, 0,                           dummyGet,    dummySet,    selectSnapshotSAVE, stringEmpty, NULL),
  SCS_ITEM("Dump ", 0, 0,                           dummyGet,    dummySet,    selectSnapshotDUMP, stringEmpty, NULL),
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

const scs_menu_item_t pageMClk[] = {
  SCS_ITEM("BPM ",  1, 400,                         mclkBpmGet,  mclkBpmSet,  selectNOP,     stringDec, NULL),
  SCS_ITEM("Play",  0, 0,                           dummyGet,    dummySet,    selectPlay,    stringPlay, NULL),
  SCS_ITEM("Stop",  0, 0,                           dummyGet,    dummySet,    selectStop,    stringStop, NULL),
  SCS_ITEM("Pause", 0, 0,                           dummyGet,    dummySet,    selectPause,   stringPause, NULL),
};

const scs_menu_item_t pageLearn[] = {
  // dummy - will be overlayed in displayHook
  SCS_ITEM("     ", 0, 0,           dummyGet,        dummySet,        selectNOP,      stringEmpty, NULL),
};


const scs_menu_page_t rootMode0[] = {
  SCS_PAGE("Var. ", pageVAR),
  SCS_PAGE(".NGR ", pageRun),
  SCS_PAGE("Snap ", pageSnap),
  SCS_PAGE("Rout ", pageROUT),
  SCS_PAGE("OSC  ", pageOSC),
  SCS_PAGE("Netw ", pageNetw),
  SCS_PAGE("Mon. ", pageMON),
  SCS_PAGE("MClk ", pageMClk),
  SCS_PAGE("Learn", pageLearn),
  SCS_PAGE(" Disk", pageDsk),
};


/////////////////////////////////////////////////////////////////////////////
//! This function can overrule the display output
//! If it returns 0, the original SCS output will be print
//! If it returns 1, the output copied into line1 and/or line2 will be print
//! If a line is not changed (line[0] = 0 or line[1] = 0), the original output
//! will be displayed - this allows to overrule only a single line
/////////////////////////////////////////////////////////////////////////////
static s32 displayHook(char *line1, char *line2)
{
  static u8 special_chars_from_mbng = 0;

  // switch between charsets
  if( SCS_MenuStateGet() == SCS_MENU_STATE_MAINPAGE && !special_chars_from_mbng ) {
    special_chars_from_mbng = 1;
    MBNG_LCD_SpecialCharsReInit();
  } else if( SCS_MenuStateGet() != SCS_MENU_STATE_MAINPAGE && special_chars_from_mbng ) {
    special_chars_from_mbng = 0;
    SCS_LCD_SpecialCharsReInit();
  }

  if( SCS_MenuStateGet() != SCS_MENU_STATE_MAINPAGE && extraPage ) {
    char msdStr[5];
    TASK_MSD_FlagStrGet(msdStr);

    u8 clkMode = SEQ_BPM_ModeGet();
    sprintf(line1, "Clk  BPM  DOUT MSD  ");
    sprintf(line2, "%s %s Off  %s",
	    SEQ_BPM_IsRunning() ? "STOP" : "PLAY",
	    (clkMode == SEQ_BPM_MODE_Slave) ? "Slve" : ((clkMode == SEQ_BPM_MODE_Master) ? "Mstr" : "Auto"),
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
    // overlayed by LCD handler in MBNG_LCD!
    // only exception: not in MIDI learn mode
    if( MBNG_EVENT_MidiLearnModeGet() ) {
      MBNG_EVENT_MidiLearnStatusMsg(line1, line2);
      SCS_DisplayUpdateRequest(); // fast updates
    }

    return 1;
  }

  if( SCS_MenuStateGet() == SCS_MENU_STATE_SELECT_PAGE ) {
    if( line1[0] == 0 ) { // no MSD overlay?
      if( MBNG_FILE_StatusMsgGet() )
	sprintf(line1, MBNG_FILE_StatusMsgGet());
      else
	sprintf(line1, "Config: %s", mbng_file_c_config_name);
    }
    return 1;
  }

  if( SCS_MenuPageGet() == pageDsk ) {
    // Disk page: we want to show the patch at upper line, and menu items at lower line
    if( line1[0] == 0 ) { // no MSD overlay?
      if( MBNG_FILE_StatusMsgGet() )
	sprintf(line1, MBNG_FILE_StatusMsgGet());
      else
	sprintf(line1, "Config: %s", mbng_file_c_config_name);
    }
    sprintf(line2, "Load Save");
    return 1;
  }

  if( SCS_MenuPageGet() == pageMON ) {
    u8 fastRefresh = line1[0] == 0;

    int i;
    for(i=0; i<SCS_NumMenuItemsGet(); ++i) {
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
	int lastColumn = SCS_NumMenuItemsGet()*SCS_MENU_ITEM_WIDTH - 1;
	if( monPageOffset == 0 )
	  line1[lastColumn] = MIOS32_LCD_TypeIsGLCD() ? '>' : 3; // right arrow
	else if( monPageOffset >= (numItems-SCS_NumMenuItemsGet()) )
	  line1[lastColumn] = MIOS32_LCD_TypeIsGLCD() ? '<' : 1; // left arrow
	else
	  line1[lastColumn] = MIOS32_LCD_TypeIsGLCD() ? '-' : 2; // left/right arrow
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

  if( SCS_MenuPageGet() == pageLearn ) {
    if( MBNG_EVENT_MidiLearnModeGet() ) {
      MBNG_EVENT_MidiLearnStatusMsg(line1, line2);
    } else {
      if( line1[0] == 0 ) { // no MSD overlay?
	sprintf(line1, "  Common  NRPN      ");
      }
      sprintf(line2, "  Learn   Learn     ");
    }

    // fast updates
    SCS_DisplayUpdateRequest();

    return 1;
  }

  return (line1[0] != 0) ? 1 : 0; // return 1 if MSD overlay
}


/////////////////////////////////////////////////////////////////////////////
//! This function is called when the rotary encoder is moved
//! If it returns 0, the encoder increment will be handled by the SCS
//! If it returns 1, the SCS will ignore the encoder
/////////////////////////////////////////////////////////////////////////////
static s32 encHook(s32 incrementer)
{
  if( SCS_MenuStateGet() == SCS_MENU_STATE_MAINPAGE ) {
    // emulated encoder function?
    if( mbng_patch_scs.enc_emu_id )
      MBNG_ENC_NotifyChange(mbng_patch_scs.enc_emu_id-1, incrementer);

    return 1;
  }

  // auto-switch encoder mode to "normal", because MBNG_ENC_AutoSpeed could have changed it
  {
    mios32_enc_config_t enc_config;
    enc_config = MIOS32_ENC_ConfigGet(0);
    if( enc_config.cfg.speed != NORMAL ) {
      enc_config.cfg.speed = NORMAL;
      MIOS32_ENC_ConfigSet(0, enc_config);
    }
  }


  if( SCS_MenuStateGet() != SCS_MENU_STATE_MAINPAGE && extraPage ) {
    return 1; // ignore encoder movements in extra page
  }

  // encoder overlayed in monitor page to scroll through port list
  if( SCS_MenuPageGet() == pageMON ) {
    int numItems = MIDI_PORT_OutNumGet() - 1;
    int newOffset = monPageOffset + incrementer;
    if( newOffset < 0 )
      newOffset = 0;
    else if( (newOffset+SCS_NumMenuItemsGet()) >= numItems ) {
      newOffset = numItems - SCS_NumMenuItemsGet();
      if( newOffset < 0 )
	newOffset = 0;
    }
    monPageOffset = newOffset;
  }

  return 0;
}


/////////////////////////////////////////////////////////////////////////////
//! This function is called when a button has been pressed or depressed
//! If it returns 0, the button movement will be handled by the SCS
//! If it returns 1, the SCS will ignore the button event
/////////////////////////////////////////////////////////////////////////////
static s32 buttonHook(u8 scsButton, u8 depressed)
{
  if( SCS_MenuStateGet() == SCS_MENU_STATE_MAINPAGE ) {
    // emulated button function?

#if MBNG_PATCH_SCS_BUTTONS != 9
# error "Please adapt for new number of buttons"
#endif

    if( scsButton >= (SCS_PIN_SOFT1+SCS_NumMenuItemsGet()) ) {
      // shift button
      int button_ix = MBNG_PATCH_SCS_BUTTONS-1;

      if( mbng_patch_scs.button_emu_id[button_ix] )
	MBNG_DIN_NotifyToggle(mbng_patch_scs.button_emu_id[button_ix]-1, depressed ? 1 : 0);
    } else if( scsButton >= SCS_PIN_SOFT1 && scsButton <= SCS_PIN_SOFT9 ) {
      int button_ix = scsButton - SCS_PIN_SOFT1;

      if( mbng_patch_scs.button_emu_id[button_ix] )
	MBNG_DIN_NotifyToggle(mbng_patch_scs.button_emu_id[button_ix]-1, depressed ? 1 : 0);
    } else if( scsButton == SCS_PIN_EXIT ) {
      return 0; // switches to menu page
    }

    return 1;
  }

  if( extraPage ) {
    if( scsButton >= (SCS_PIN_SOFT1+SCS_NumMenuItemsGet()) && depressed ) // selects/deselects extra page
      extraPage = 0;
    else {
      switch( scsButton ) {
      case SCS_PIN_SOFT1:
	if( depressed )
	  return 1;
	MBNG_SEQ_PlayStopButton();
	break;

      case SCS_PIN_SOFT2:
	if( depressed )
	  return 1;
	if( SEQ_BPM_ModeGet() >= 2 )
	  SEQ_BPM_ModeSet(0);
	else
	  SEQ_BPM_ModeSet(SEQ_BPM_ModeGet() + 1);
	break;

      case SCS_PIN_SOFT3:
	if( depressed )
	  return 1;
	MBNG_DOUT_Init(0);
	SCS_Msg(SCS_MSG_L, 1000, "All DOUT pins", "deactivated");
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
    if( scsButton >= (SCS_PIN_SOFT1+SCS_NumMenuItemsGet()) && !depressed ) { // selects/deselects extra page
      extraPage = 1;
      return 1;
    }
  }

  if( SCS_MenuPageGet() == pageLearn ) {
    if( scsButton == SCS_PIN_SOFT1 || scsButton == SCS_PIN_SOFT2 ) {
      if( !depressed )
	MBNG_EVENT_MidiLearnModeSet(1);
      return 1;
    } else if( scsButton == SCS_PIN_SOFT3 || scsButton == SCS_PIN_SOFT4 ) {
      if( !depressed )
	MBNG_EVENT_MidiLearnModeSet(2);
      return 1;
    } else if( scsButton == SCS_PIN_EXIT ) {
      if( !depressed )
	MBNG_EVENT_MidiLearnModeSet(0);
    }
  }

  return 0; // no error
}



/////////////////////////////////////////////////////////////////////////////
//! Initialisation of SCS Config
//! mode selects the used SCS config (currently only one available selected with 0)
//! \return < 0 if initialisation failed
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


//! \}
