// $Id$
/*
 * Ethernet Configuration page
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
#include <string.h>

#include "tasks.h"

#include "seq_lcd.h"
#include "seq_ui.h"

#include "seq_file.h"
#include "seq_file_gc.h"

#include "seq_midi_osc.h"


#if !defined(MIOS32_FAMILY_EMULATION)
#include "uip.h"
#include "uip_task.h"
#include "osc_server.h"
#else
#define OSC_SERVER_NUM_CONNECTIONS 4
#endif


/////////////////////////////////////////////////////////////////////////////
// Local definitions
/////////////////////////////////////////////////////////////////////////////

#define NUM_OF_ITEMS           12
#define ITEM_DHCP              0
#define ITEM_NETWORK_CFG       1
#define ITEM_NETWORK_1         2
#define ITEM_NETWORK_2         3
#define ITEM_NETWORK_3         4
#define ITEM_NETWORK_4         5
#define ITEM_OSC_PORT          6
#define ITEM_OSC_CFG           7
#define ITEM_OSC_1             8
#define ITEM_OSC_2             9
#define ITEM_OSC_3             10
#define ITEM_OSC_4             11

// Network configuration pages
#define NCFG_NUM_OF_ITEMS       3
#define NCFG_IP                 0
#define NCFG_NETMASK            1
#define NCFG_GATEWAY            2

#define OCFG_NUM_OF_ITEMS       4
#define OCFG_REMOTE_IP          0
#define OCFG_REMOTE_PORT        1
#define OCFG_LOCAL_PORT         2
#define OCFG_TRANSFER_MODE      3


/////////////////////////////////////////////////////////////////////////////
// Local Variables
/////////////////////////////////////////////////////////////////////////////

static u8 selected_osc_con = 0;

static u8  ncfg_item = 0;
static u8  ncfg_value_changed;
static u32 ncfg_value;
static u8  ocfg_item = 0;
static u8  ocfg_value_changed;
static u32 ocfg_value;

/////////////////////////////////////////////////////////////////////////////
// Local Prototypes
/////////////////////////////////////////////////////////////////////////////
static s32 SEQ_UI_ETH_UpdateNCfg(void);
static s32 SEQ_UI_ETH_UpdateOCfg(void);
static s32 SEQ_UI_ETH_StoreNCfg(void);
static s32 SEQ_UI_ETH_StoreOCfg(void);


/////////////////////////////////////////////////////////////////////////////
// Local LED handler function
/////////////////////////////////////////////////////////////////////////////
static s32 LED_Handler(u16 *gp_leds)
{
  if( ui_cursor_flash ) // if flashing flag active: no LED flag set
    return 0;

  switch( ui_selected_item ) {
  case ITEM_DHCP:        *gp_leds = 0x0001; break;
  case ITEM_NETWORK_CFG: *gp_leds = 0x0006; break;
  case ITEM_NETWORK_1:   *gp_leds = 0x0008; break;
  case ITEM_NETWORK_2:   *gp_leds = 0x0010; break;
  case ITEM_NETWORK_3:   *gp_leds = 0x0020; break;
  case ITEM_NETWORK_4:   *gp_leds = 0x0040; break;
  case ITEM_OSC_PORT:    *gp_leds = 0x0100; break;
  case ITEM_OSC_CFG:     *gp_leds = 0x0600; break;
  case ITEM_OSC_1:       *gp_leds = (ocfg_item == OCFG_REMOTE_IP) ? 0x0800 : 0x7800; break;
  case ITEM_OSC_2:       *gp_leds = (ocfg_item == OCFG_REMOTE_IP) ? 0x1000 : 0x7800; break;
  case ITEM_OSC_3:       *gp_leds = (ocfg_item == OCFG_REMOTE_IP) ? 0x2000 : 0x7800; break;
  case ITEM_OSC_4:       *gp_leds = (ocfg_item == OCFG_REMOTE_IP) ? 0x4000 : 0x7800; break;
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Local encoder callback function
// Should return:
//   1 if value has been changed
//   0 if value hasn't been changed
//  -1 if invalid or unsupported encoder
/////////////////////////////////////////////////////////////////////////////
static s32 Encoder_Handler(seq_ui_encoder_t encoder, s32 incrementer)
{
#if defined(MIOS32_FAMILY_EMULATION)
  u8 networkAvailable = 0;
  u8 dhcpOn = 0;
  u8 dhcpValid = 0;
#else
  u8 networkAvailable = UIP_TASK_NetworkDeviceAvailable();
  u8 dhcpOn = UIP_TASK_DHCP_EnableGet();
  u8 dhcpValid = dhcpOn && UIP_TASK_ServicesRunning();
#endif

  switch( encoder ) {
  case SEQ_UI_ENCODER_GP1:
    ui_selected_item = ITEM_DHCP;
    break;

  case SEQ_UI_ENCODER_GP2:
  case SEQ_UI_ENCODER_GP3:
    ui_selected_item = ITEM_NETWORK_CFG;
    break;

  case SEQ_UI_ENCODER_GP4:
    ui_selected_item = ITEM_NETWORK_1;
    break;

  case SEQ_UI_ENCODER_GP5:
    ui_selected_item = ITEM_NETWORK_2;
    break;

  case SEQ_UI_ENCODER_GP6:
    ui_selected_item = ITEM_NETWORK_3;
    break;

  case SEQ_UI_ENCODER_GP7:
    ui_selected_item = ITEM_NETWORK_4;
    break;

  case SEQ_UI_ENCODER_GP8:
    if( !networkAvailable ) {
      SEQ_UI_Msg(SEQ_UI_MSG_USER, 2000, "MBHP_ETH not", "connected!");
    } else {
      SEQ_UI_ETH_StoreNCfg();
      // for my own comfort: select config item when enter button is pressed
      ui_selected_item = ITEM_NETWORK_CFG;
    }
    return 1;

  case SEQ_UI_ENCODER_GP9:
    ui_selected_item = ITEM_OSC_PORT;
    break;

  case SEQ_UI_ENCODER_GP10:
  case SEQ_UI_ENCODER_GP11:
    ui_selected_item = ITEM_OSC_CFG;
    break;

  case SEQ_UI_ENCODER_GP12:
    ui_selected_item = ITEM_OSC_1;
    break;

  case SEQ_UI_ENCODER_GP13:
    ui_selected_item = ITEM_OSC_2;
    break;

  case SEQ_UI_ENCODER_GP14:
    ui_selected_item = ITEM_OSC_3;
    break;

  case SEQ_UI_ENCODER_GP15:
    ui_selected_item = ITEM_OSC_4;
    break;

  case SEQ_UI_ENCODER_GP16:
    SEQ_UI_ETH_StoreOCfg();
    // for my own comfort: select config item when enter button is pressed
    ui_selected_item = ITEM_OSC_CFG;
    return 1;
  }

  // for GP encoders and Datawheel
  switch( ui_selected_item ) {
  case ITEM_DHCP:
    if( !networkAvailable ) {
      SEQ_UI_Msg(SEQ_UI_MSG_USER, 2000, "MBHP_ETH not", "connected!");
    } else {
      u8 prevDhcpOn = dhcpOn;
      dhcpOn = incrementer >= 0;
      if( dhcpOn != prevDhcpOn ) {
#if !defined(MIOS32_FAMILY_EMULATION)
	UIP_TASK_DHCP_EnableSet(dhcpOn);
	SEQ_UI_ETH_UpdateNCfg();
#endif
	ncfg_value_changed = 1;
	return 1; // value changed
      }
    }
    return 0; // value not changed

  case ITEM_NETWORK_CFG:
    if( SEQ_UI_Var8_Inc(&ncfg_item, 0, NCFG_NUM_OF_ITEMS-1, incrementer) ) {
      SEQ_UI_ETH_UpdateNCfg();
      return 1; // value changed
    }
    return 0; // value not changed

  case ITEM_NETWORK_1:
  case ITEM_NETWORK_2:
  case ITEM_NETWORK_3:
  case ITEM_NETWORK_4: {
    if( !networkAvailable )
      SEQ_UI_Msg(SEQ_UI_MSG_USER, 2000, "MBHP_ETH not", "connected!");
    else {
      if( dhcpOn ) {
	if( dhcpValid )
	  SEQ_UI_Msg(SEQ_UI_MSG_USER, 2000, "DHCP enabled!", "Auto-Config!");
	else
	  SEQ_UI_Msg(SEQ_UI_MSG_USER, 2000, "DHCP enabled!", "No Config yet!");
      } else {
	int bitPos = 8*(3-(ui_selected_item-ITEM_NETWORK_1));
	u8 value = (ncfg_value >> bitPos) & 0xff;
	if( SEQ_UI_Var8_Inc(&value, 0, 255, incrementer) ) {
	  ncfg_value &= ~(0xff << bitPos);
	  ncfg_value |= (u32)value << bitPos;
	  ncfg_value_changed = 1;
	  return 1; // value changed
	}
      }
    }
    return 0; // value not changed
  } break;

  case ITEM_OSC_PORT:
    if( SEQ_UI_Var8_Inc(&selected_osc_con, 0, OSC_SERVER_NUM_CONNECTIONS-1, incrementer) ) {
      SEQ_UI_ETH_UpdateOCfg();
      return 1; // value changed
    }
    return 0; // value not changed

  case ITEM_OSC_CFG:
    if( SEQ_UI_Var8_Inc(&ocfg_item, 0, OCFG_NUM_OF_ITEMS-1, incrementer) ) {
      SEQ_UI_ETH_UpdateOCfg();
      return 1; // value changed
    }
    return 0; // value not changed

  case ITEM_OSC_1:
  case ITEM_OSC_2:
  case ITEM_OSC_3:
  case ITEM_OSC_4: {
    if( ocfg_item == OCFG_REMOTE_IP ) {
      int bitPos = 8*(3-(ui_selected_item-ITEM_OSC_1));
      u8 value = (ocfg_value >> bitPos) & 0xff;
      if( SEQ_UI_Var8_Inc(&value, 0, 255, incrementer) ) {
	ocfg_value &= ~(0xff << bitPos);
	ocfg_value |= (u32)value << bitPos;
	ocfg_value_changed = 1;
	return 1; // value changed
      }
      return 0; // value not changed
    } else if( ocfg_item == OCFG_TRANSFER_MODE ) {
      u8 value = ocfg_value;
      if( SEQ_UI_Var8_Inc(&value, 0, SEQ_MIDI_OSC_NUM_TRANSFER_MODES-1, incrementer) ) {
	ocfg_value = value;
	ocfg_value_changed = 1;
	return 1; // value changed
      }
      return 0; // value not changed
    } else {
      u16 value = ocfg_value;
      if( SEQ_UI_Var16_Inc(&value, 0, 65535, incrementer) ) {
	ocfg_value = value;
	ocfg_value_changed = 1;
	return 1; // value changed
      }
      return 0; // value not changed
    }
  } break;
  }

  return -1; // invalid or unsupported encoder
}


/////////////////////////////////////////////////////////////////////////////
// Local button callback function
// Should return:
//   1 if value has been changed
//   0 if value hasn't been changed
//  -1 if invalid or unsupported button
/////////////////////////////////////////////////////////////////////////////
static s32 Button_Handler(seq_ui_button_t button, s32 depressed)
{
  if( depressed ) return 0; // ignore when button depressed

  if( button <= SEQ_UI_BUTTON_GP16 ) {
    // -> forward to encoder handler
    return Encoder_Handler((int)button, 0);
  }

  switch( button ) {
    case SEQ_UI_BUTTON_Select:
    case SEQ_UI_BUTTON_Right:
      if( ++ui_selected_item >= NUM_OF_ITEMS )
	ui_selected_item = 0;
      return 1; // value always changed

    case SEQ_UI_BUTTON_Left:
      if( ui_selected_item == 0 )
	ui_selected_item = NUM_OF_ITEMS-1;
      else
	--ui_selected_item;
      return 1; // value always changed

    case SEQ_UI_BUTTON_Up:
      return Encoder_Handler(SEQ_UI_ENCODER_Datawheel, 1);

    case SEQ_UI_BUTTON_Down:
      return Encoder_Handler(SEQ_UI_ENCODER_Datawheel, -1);
  }

  return -1; // invalid or unsupported button
}


/////////////////////////////////////////////////////////////////////////////
// Local Display Handler function
// IN: <high_prio>: if set, a high-priority LCD update is requested
/////////////////////////////////////////////////////////////////////////////
static s32 LCD_Handler(u8 high_prio)
{
  int i;

  if( high_prio )
    return 0; // there are no high-priority updates

#if defined(MIOS32_FAMILY_EMULATION)
  u8 networkAvailable = 0;
  u8 dhcpOn = 0;
  u8 dhcpValid = 0;
#else
  u8 networkAvailable = UIP_TASK_NetworkDeviceAvailable();
  u8 dhcpOn = UIP_TASK_DHCP_EnableGet();
  u8 dhcpValid = dhcpOn && UIP_TASK_ServicesRunning();
  if( dhcpOn )
    SEQ_UI_ETH_UpdateNCfg(); // could change during runtime
#endif

  // layout:
  // 00000000001111111111222222222233333333330000000000111111111122222222223333333333
  // 01234567890123456789012345678901234567890123456789012345678901234567890123456789
  // <--------------------------------------><-------------------------------------->
  // DHCP      Network Configuration         Port        OSC Configuration           
  //  on   Local IP: 123. 123. 123. 123 EnterOSC1 Remote IP: 123. 123. 123. 123 Enter

  SEQ_LCD_CursorSet(0, 0);
  SEQ_LCD_PrintString("DHCP      Network Configuration         Port        OSC Configuration           ");

  ///////////////////////////////////////////////////////////////////////////
  SEQ_LCD_CursorSet(0, 1);
  if( ui_selected_item == ITEM_DHCP && ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(4);
  } else {
    SEQ_LCD_PrintString(dhcpOn ? " on " : " off");
  }
  SEQ_LCD_PrintSpaces(1);

  ///////////////////////////////////////////////////////////////////////////
  if( ui_selected_item == ITEM_NETWORK_CFG && ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(10);
  } else {
    const char ncfg_str[NCFG_NUM_OF_ITEMS][11] = {
      " Local IP:",
      "  Netmask:",
      "  Gateway:"
    };

    SEQ_LCD_PrintString((char *)ncfg_str[ncfg_item]);
  }

  ///////////////////////////////////////////////////////////////////////////
  for(i=0; i<4; ++i) {
    u8 printChar = 0;
    if( ui_selected_item == (ITEM_NETWORK_1+i) && ui_cursor_flash )
      printChar = ' ';
    else if( !networkAvailable )
      printChar = '-';
    else if( dhcpOn && !dhcpValid )
      printChar = '?';

    if( printChar ) {
      if( printChar == ' ' )
	SEQ_LCD_PrintSpaces(4);
      else {
	SEQ_LCD_PrintChar(' ');
	SEQ_LCD_PrintChar(printChar);
	SEQ_LCD_PrintChar(printChar);
	SEQ_LCD_PrintChar(printChar);
      }
    } else {
      SEQ_LCD_PrintFormattedString("%4d", (ncfg_value >> ((3-i)*8)) & 0xff);
    }
    SEQ_LCD_PrintChar((i == 3) ? ' ' : '.');
  }

  ///////////////////////////////////////////////////////////////////////////
  if( !networkAvailable ) {
    SEQ_LCD_PrintSpaces(5);
  } else {
    if( ncfg_value_changed && !ui_cursor_flash ) {
      SEQ_LCD_CursorSet(35, 0);
      SEQ_LCD_PrintString("Press");
      SEQ_LCD_CursorSet(35, 1);
    }

    if( ncfg_value_changed && ui_cursor_flash ) {
      SEQ_LCD_PrintSpaces(5);
    } else {
      SEQ_LCD_PrintString("Enter");
    }
  }

  ///////////////////////////////////////////////////////////////////////////
  if( ui_selected_item == ITEM_OSC_PORT && ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(5);
  } else {
    SEQ_LCD_PrintFormattedString("OSC%d ", selected_osc_con+1);
  }

  ///////////////////////////////////////////////////////////////////////////
  if( ui_selected_item == ITEM_OSC_CFG && ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(10);
  } else {
    const char ocfg_str[OCFG_NUM_OF_ITEMS][11] = {
      "Remote IP:",
      "Rem. Port:",
      "LocalPort:",
      "  Tx Mode:",
    };

    SEQ_LCD_PrintString((char *)ocfg_str[ocfg_item]);
  }

  ///////////////////////////////////////////////////////////////////////////
  switch( ocfg_item ) {
  case OCFG_REMOTE_IP:
    for(i=0; i<4; ++i) {
      if( ui_selected_item == (ITEM_OSC_1+i) && ui_cursor_flash ) {
	SEQ_LCD_PrintSpaces(5);
      } else {
	SEQ_LCD_PrintFormattedString("%4d", (ocfg_value >> ((3-i)*8)) & 0xff);
	SEQ_LCD_PrintChar((i == 3) ? ' ' : '.');
      }
    }
    break;

  case OCFG_REMOTE_PORT:
  case OCFG_LOCAL_PORT:
    if( ui_selected_item >= ITEM_OSC_1 && ui_selected_item <= ITEM_OSC_4 && ui_cursor_flash ) {
      SEQ_LCD_PrintSpaces(20);
    } else {
      SEQ_LCD_PrintFormattedString(" %5d", ocfg_value);
      SEQ_LCD_PrintSpaces(14);
    }
    break;

  case OCFG_TRANSFER_MODE:
    if( ui_selected_item >= ITEM_OSC_1 && ui_selected_item <= ITEM_OSC_4 && ui_cursor_flash ) {
      SEQ_LCD_PrintSpaces(20);
    } else {
      const char txmode_str[SEQ_MIDI_OSC_NUM_TRANSFER_MODES][21] = {
	" MIDI Messages      ",
	" Text Msg (Integer) ",
	" Text Msg (Float)   ",
	" Pianist Pro (iPad) ",
	" TouchOSC           ",
      };

      SEQ_LCD_PrintString((char *)txmode_str[ocfg_value]);
    }
    break;
  }

  ///////////////////////////////////////////////////////////////////////////
  if( ocfg_value_changed && !ui_cursor_flash ) {
    SEQ_LCD_CursorSet(75, 0);
    SEQ_LCD_PrintString("Press");
    SEQ_LCD_CursorSet(75, 1);
  }

  if( ocfg_value_changed && ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(5);
  } else {
    SEQ_LCD_PrintString("Enter");
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_UI_ETH_Init(u32 mode)
{
  // install callback routines
  SEQ_UI_InstallButtonCallback(Button_Handler);
  SEQ_UI_InstallEncoderCallback(Encoder_Handler);
  SEQ_UI_InstallLEDCallback(LED_Handler);
  SEQ_UI_InstallLCDCallback(LCD_Handler);

  // load charset (if this hasn't been done yet)
  SEQ_LCD_InitSpecialChars(SEQ_LCD_CHARSET_Menu);

  SEQ_UI_ETH_UpdateNCfg();
  SEQ_UI_ETH_UpdateOCfg();

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Update/Store functions
/////////////////////////////////////////////////////////////////////////////
static s32 SEQ_UI_ETH_UpdateNCfg(void)
{
  ncfg_value = 0;
  ncfg_value_changed = 0;

#if !defined(MIOS32_FAMILY_EMULATION)
  u8 dhcpOn = UIP_TASK_DHCP_EnableGet();

  switch( ncfg_item ) {
  case NCFG_IP:
    if( dhcpOn ) {
      uip_ipaddr_t ipaddr;
      uip_gethostaddr(&ipaddr);
      ncfg_value =
	(uip_ipaddr1(ipaddr) << 24) |
	(uip_ipaddr2(ipaddr) << 16) |
	(uip_ipaddr3(ipaddr) <<  8) |
	(uip_ipaddr4(ipaddr) <<  0);
    } else {
      ncfg_value = UIP_TASK_IP_AddressGet();
    }
    break;

  case NCFG_NETMASK:
    if( dhcpOn ) {
      uip_ipaddr_t netmask;
      uip_getnetmask(&netmask);
      ncfg_value =
	(uip_ipaddr1(netmask) << 24) |
	(uip_ipaddr2(netmask) << 16) |
	(uip_ipaddr3(netmask) <<  8) |
	(uip_ipaddr4(netmask) <<  0);
    } else {
      ncfg_value = UIP_TASK_NetmaskGet();
    }
    break;

  case NCFG_GATEWAY:
    if( dhcpOn ) {
      uip_ipaddr_t draddr;
      uip_getdraddr(&draddr);
      ncfg_value =
	(uip_ipaddr1(draddr) << 24) |
	(uip_ipaddr2(draddr) << 16) |
	(uip_ipaddr3(draddr) <<  8) |
	(uip_ipaddr4(draddr) <<  0);
    } else {
      ncfg_value = UIP_TASK_GatewayGet();
    }
    break;
  }
#endif

  return 0; // no error
}

static s32 SEQ_UI_ETH_UpdateOCfg(void)
{
  ocfg_value = 0;
  ocfg_value_changed = 0;

#if !defined(MIOS32_FAMILY_EMULATION)
  switch( ocfg_item ) {
  case OCFG_REMOTE_IP:     ocfg_value = OSC_SERVER_RemoteIP_Get(selected_osc_con); break;
  case OCFG_REMOTE_PORT:   ocfg_value = OSC_SERVER_RemotePortGet(selected_osc_con); break;
  case OCFG_LOCAL_PORT:    ocfg_value = OSC_SERVER_LocalPortGet(selected_osc_con); break;
  case OCFG_TRANSFER_MODE: ocfg_value = SEQ_MIDI_OSC_TransferModeGet(selected_osc_con); break;
  }
#endif

  return 0; // no error
}


static s32 SEQ_UI_ETH_StoreNCfg(void)
{
#if !defined(MIOS32_FAMILY_EMULATION)
  switch( ncfg_item ) {
  case NCFG_IP:      UIP_TASK_IP_AddressSet(ncfg_value); break;
  case NCFG_NETMASK: UIP_TASK_NetmaskSet(ncfg_value); break;
  case NCFG_GATEWAY: UIP_TASK_GatewaySet(ncfg_value); break;
  }
#endif

  // store configuration file
  MUTEX_SDCARD_TAKE;
  s32 status;
  if( (status=SEQ_FILE_GC_Write()) < 0 )
    SEQ_UI_SDCardErrMsg(2000, status);
  MUTEX_SDCARD_GIVE;

  ncfg_value_changed = 0;

  return 0; // no error
}

static s32 SEQ_UI_ETH_StoreOCfg(void)
{
#if !defined(MIOS32_FAMILY_EMULATION)
  switch( ocfg_item ) {
  case OCFG_REMOTE_IP:     OSC_SERVER_RemoteIP_Set(selected_osc_con, ocfg_value); break;
  case OCFG_REMOTE_PORT:   OSC_SERVER_RemotePortSet(selected_osc_con, ocfg_value); break;
  case OCFG_LOCAL_PORT:    OSC_SERVER_LocalPortSet(selected_osc_con, ocfg_value); break;
  case OCFG_TRANSFER_MODE: SEQ_MIDI_OSC_TransferModeSet(selected_osc_con, ocfg_value); break;
  }

  // OSC_SERVER_Init(0) has to be called after any setting has been changed
  OSC_SERVER_Init(0);
#endif

  // store configuration file
  MUTEX_SDCARD_TAKE;
  s32 status;
  if( (status=SEQ_FILE_GC_Write()) < 0 )
    SEQ_UI_SDCardErrMsg(2000, status);
  MUTEX_SDCARD_GIVE;

  ocfg_value_changed = 0;

  return 0; // no error
}
