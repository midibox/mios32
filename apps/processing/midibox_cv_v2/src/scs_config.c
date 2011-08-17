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

#include <uip.h>
#include "uip_task.h"
#include "osc_client.h"
#include "osc_server.h"

#include <scs.h>
#include "scs_config.h"

#include <seq_bpm.h>

#include "mbcv_port.h"
#include "mbcv_map.h"

#include <file.h>
#include "mbcv_file.h"
#include "mbcv_file_p.h"
#include "mbcv_patch.h"



/////////////////////////////////////////////////////////////////////////////
// Local defines
/////////////////////////////////////////////////////////////////////////////

static u8 extraPage;

static u8 selectedCv;
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

static void stringAoutIf(u32 ix, u16 value, char *label)
{
  char *name = MBCV_MAP_IfNameGet(); // 8 chars max
  if( ix == 0 ) {
    label[0] = ' ';
    memcpy(label+1, (char *)(MBCV_MAP_IfNameGet()), 4);
  } else {
    memcpy(label, (char *)(MBCV_MAP_IfNameGet() + 4), 5);
  }
  label[5] = 0;
}

static void stringCvCurve(u32 ix, u16 value, char *label) { memcpy(label, MBCV_MAP_CurveNameGet(selectedCv), 5); label[5] = 0; }
static void stringCvCaliMode(u32 ix, u16 value, char *label) { memcpy(label, MBCV_MAP_CaliNameGet(), 5); label[5] = 0; }

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

static void stringInPort(u32 ix, u16 value, char *label)
{
  sprintf(label, MBCV_PORT_InNameGet(value));
}

static void stringOutPort(u32 ix, u16 value, char *label)
{
  sprintf(label, MBCV_PORT_OutNameGet(value));
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

  if( (status=MBCV_PATCH_Store(newString)) < 0 ) {
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
    if( (status=MBCV_PATCH_Load(newString)) < 0 ) {
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

static u16  cvCurveGet(u32 ix)            { return MBCV_MAP_CurveGet(selectedCv); }
static void cvCurveSet(u32 ix, u16 value) { MBCV_MAP_CurveSet(selectedCv, value); }

static u16  cvSlewRateGet(u32 ix)            { return MBCV_MAP_SlewRateGet(selectedCv); }
static void cvSlewRateSet(u32 ix, u16 value) { MBCV_MAP_SlewRateSet(selectedCv, value); }

static u16  cvCaliModeGet(u32 ix)            { return MBCV_MAP_CaliModeGet(); }
static void cvCaliModeSet(u32 ix, u16 value) { MBCV_MAP_CaliModeSet(selectedCv, value); }

static u16  aoutIfGet(u32 ix)              { return MBCV_MAP_IfGet(); }
static void aoutIfSet(u32 ix, u16 value)   { MBCV_MAP_IfSet(value); }

static u16  routerNodeGet(u32 ix)             { return selectedRouterNode; }
static void routerNodeSet(u32 ix, u16 value)  { selectedRouterNode = value; }

static u16  routerSrcPortGet(u32 ix)             { return MBCV_PORT_InIxGet(mbcv_patch_router[selectedRouterNode].src_port); }
static void routerSrcPortSet(u32 ix, u16 value)  { mbcv_patch_router[selectedRouterNode].src_port = MBCV_PORT_InPortGet(value); }

static u16  routerSrcChnGet(u32 ix)              { return mbcv_patch_router[selectedRouterNode].src_chn; }
static void routerSrcChnSet(u32 ix, u16 value)   { mbcv_patch_router[selectedRouterNode].src_chn = value; }

static u16  routerDstPortGet(u32 ix)             { return MBCV_PORT_OutIxGet(mbcv_patch_router[selectedRouterNode].dst_port); }
static void routerDstPortSet(u32 ix, u16 value)  { mbcv_patch_router[selectedRouterNode].dst_port = MBCV_PORT_OutPortGet(value); }

static u16  routerDstChnGet(u32 ix)              { return mbcv_patch_router[selectedRouterNode].dst_chn; }
static void routerDstChnSet(u32 ix, u16 value)   { mbcv_patch_router[selectedRouterNode].dst_chn = value; }

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
  SCS_Msg(SCS_MSG_L, 1000, "Mass Storage", enable ? "enabled!" : "disabled!");
}

/////////////////////////////////////////////////////////////////////////////
// Menu Structure
/////////////////////////////////////////////////////////////////////////////

const scs_menu_item_t pageAOUT[] = {
  SCS_ITEM(" CV  ", 0, MBCV_PATCH_NUM_CV-1, cvGet, cvSet, selectNOP, stringDecP1, NULL),
  SCS_ITEM("Curve", 0, MBCV_MAP_NUM_CURVES-1, cvCurveGet, cvCurveSet, selectNOP, stringCvCurve, NULL),
  SCS_ITEM(" Slew", 0, 255,   cvSlewRateGet, cvSlewRateSet, selectNOP, stringDec4, NULL),
  SCS_ITEM(" Cali", 0, MBCV_MAP_NUM_CALI_MODES-1, cvCaliModeGet, cvCaliModeSet, selectNOP, stringCvCaliMode, NULL),
  SCS_ITEM(" Modu", 0, AOUT_NUM_IF-1, aoutIfGet, aoutIfSet, selectNOP, stringAoutIf, NULL),
  SCS_ITEM("le   ", 1, AOUT_NUM_IF-1, aoutIfGet, aoutIfSet, selectNOP, stringAoutIf, NULL),
};

const scs_menu_item_t pageROUT[] = {
  SCS_ITEM("Node", 0, MBCV_PATCH_NUM_ROUTER-1, routerNodeGet, routerNodeSet,selectNOP, stringDecP1, NULL),
  SCS_ITEM("SrcP", 0, MBCV_PORT_NUM_IN_PORTS-1, routerSrcPortGet, routerSrcPortSet,selectNOP, stringInPort, NULL),
  SCS_ITEM("Chn.", 0, 17,                       routerSrcChnGet, routerSrcChnSet,selectNOP, stringRouterChn, NULL),
  SCS_ITEM("SrcD", 0, MBCV_PORT_NUM_OUT_PORTS-1, routerDstPortGet, routerDstPortSet,selectNOP, stringOutPort, NULL),
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
  SCS_PAGE("AOUT ", pageAOUT),
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

    sprintf(line1, "CLK  DOUT MSD  ");
    sprintf(line2, "%s Off  %s",
	    SEQ_BPM_IsRunning() ? "STOP" : "PLAY",
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
    u32 tick = SEQ_BPM_TickGet();
    u32 ticks_per_step = SEQ_BPM_PPQN_Get() / 4;
    u32 ticks_per_measure = ticks_per_step * 16;
    u32 measure = (tick / ticks_per_measure) + 1;
    u32 step = ((tick % ticks_per_measure) / ticks_per_step) + 1;

    if( line1[0] == 0 ) { // no MSD overlay?
      if( MBCV_FILE_StatusMsgGet() )
	sprintf(line1, MBCV_FILE_StatusMsgGet());
      else
	sprintf(line1, "Patch: %s", mbcv_file_p_patch_name);
    }
    sprintf(line2, "%s   <    >   MENU", SEQ_BPM_IsRunning() ? "STOP" : "PLAY");

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
	mios32_midi_port_t port = MBCV_PORT_InPortGet(portIx);
	mios32_midi_package_t package = MBCV_PORT_InPackageGet(port);
	if( port == 0xff ) {
	  strcat(line1, "     ");
	} else if( package.type ) {
	  char buffer[6];
	  MBCV_PORT_EventNameGet(package, buffer, 5);
	  strcat(line1, buffer);
	} else {
	  strcat(line1, MBCV_PORT_InNameGet(portIx));
	  strcat(line1, " ");
	}

	// insert arrow at upper right corner
	int numItems = MBCV_PORT_OutNumGet() - 1;
	if( monPageOffset == 0 )
	  line1[19] = 1; // right arrow
	else if( monPageOffset >= (numItems-SCS_NUM_MENU_ITEMS) )
	  line1[19] = 0; // left arrow
	else
	  line1[19] = 2; // left/right arrow

      }

      mios32_midi_port_t port = MBCV_PORT_OutPortGet(portIx);
      mios32_midi_package_t package = MBCV_PORT_OutPackageGet(port);
      if( port == 0xff ) {
	strcat(line2, "     ");
      } else if( package.type ) {
	char buffer[6];
	MBCV_PORT_EventNameGet(package, buffer, 5);
	strcat(line2, buffer);
      } else {
	strcat(line2, MBCV_PORT_OutNameGet(portIx));
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
    int numItems = MBCV_PORT_OutNumGet() - 1;
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
	//SEQ_PlayStopButton();
	break;

      case SCS_PIN_SOFT2:
	if( depressed )
	  return 1;
	// TODO
	SCS_Msg(SCS_MSG_L, 1000, "All Notes", "off!");
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

      switch( scsButton ) {
      case SCS_PIN_SOFT1: // Play/Stop
	//SEQ_PlayStopButton();
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
