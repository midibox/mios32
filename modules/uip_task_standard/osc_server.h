// $Id$
/*
 * Header file for uIP OSC daemon/server
 *
 * ==========================================================================
 *
 *  Copyright (C) 2009 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _OSC_SERVER_H
#define _OSC_SERVER_H


/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////

// for 4 independent OSC ports
#define OSC_SERVER_NUM_CONNECTIONS 4

// can be overruled in mios32_config.h
#ifndef OSC_REMOTE_IP
//                      192       .  168        .    2       .  101
#define OSC_REMOTE_IP ( 10 << 24) | (  0 << 16) | (  0 << 8) | (  2 << 0)
#endif

// can be overruled in mios32_config.h
#ifndef OSC_LOCAL_PORT
#define OSC_LOCAL_PORT 8000
#endif

#ifndef OSC_REMOTE_PORT
#define OSC_REMOTE_PORT 8001
#endif

// should transfer mode be ignored on incoming OSC packets?
#ifndef OSC_IGNORE_TRANSFER_MODE
#define OSC_IGNORE_TRANSFER_MODE 1
#endif

/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////

// clashes with dhcpc.h
// not used by OSC server anyhow
//typedef unsigned int uip_udp_appstate_t;


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 OSC_SERVER_Init(u32 mode);
extern s32 OSC_SERVER_InitFromPresets(u8 con, u32 _osc_remote_ip, u16 _osc_remote_port, u16 _osc_local_port);

extern s32 OSC_SERVER_RemoteIP_Set(u8 con, u32 ip);
extern u32 OSC_SERVER_RemoteIP_Get(u8 con);
extern s32 OSC_SERVER_RemotePortSet(u8 con, u16 port);
extern u16 OSC_SERVER_RemotePortGet(u8 con);
extern s32 OSC_SERVER_LocalPortSet(u8 con, u16 port);
extern u16 OSC_SERVER_LocalPortGet(u8 con);

extern s32 OSC_SERVER_AppCall(void);
extern s32 OSC_SERVER_SendPacket(u8 con, u8 *packet, u32 len);


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////

#endif /* _OSC_SERVER_H */
