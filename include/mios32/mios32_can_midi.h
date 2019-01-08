// $Id: mios32_can_midi.h 528 2009-05-17 16:45:51Z tk $
/*
 * Header file for CAN MIDI functions
 *
 * ==========================================================================
 *
 *  Copyright (C) 2008 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _MIOS32_CAN_MIDI_H
#define _MIOS32_CAN_MIDI_H

/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////

// allowed numbers: 1..16
#ifndef MIOS32_CAN_MIDI_NUM_PORTS
#define MIOS32_CAN_MIDI_NUM_PORTS 16
#else
#if MIOS32_CAN_MIDI_NUM_PORTS >16
#define MIOS32_CAN_MIDI_NUM_PORTS 16
#endif
#endif

/* - Enhanced mode allowed (send/receive extended message)
 if necessary (if needed and requested by an other node).
 Note: Message type and cable filter is used in this mode.
 - If not precised, MCAN is in Basic Mode(not able to send/receive extended message)
 */
//#define MIOS32_CAN_MIDI_ENHANCED

/* Change MCAN verbose level here, default is 0
 note: can be changed in terminal
 no verbose if not defined
 */
//#define MIOS32_CAN_MIDI_VERBOSE_LEVEL 1

/* forces the MCAN Input to work with streamed SYSEX
 if not defined:
 normal flow is: "packet->package"
 -> package using APP_MIDI_NotifySysex (MIOS32_MIDI_SysExCallback_Init)
 -> package using APP_MIDI_NotifyPackage
 (+)less RAM use (-)_SysExStreamCallback_ not usable, more packaging process
 if defined flow becomes: "packet->stream" first then stream->package
 -> stream using APP_MCAN_NotifySysexStream (MIOS32_CAN_MIDI_SysExStreamCallback_Init)
 -> to normal flow
 (+)stream ready, less process (-)16KB RAM is used for Sysex buffering
 */
//#define MIOS32_CAN_MIDI_SYSEX_STREAM_FIRST

// by default, equal to the mios32 Device_Id (BSL)
//#define MIOS32_CAN_MIDI_NODE_ID     0

// MCAN Common Layer applicaton Code
#define MIOS32_MCAN_APP_CODE      0 // MCAN

/* Default application code
 */
#ifndef MIOS32_CAN_MIDI_APP_CODE
#define MIOS32_CAN_MIDI_APP_CODE      MIOS32_MCAN_APP_CODE
#endif

#define MIOS32_CAN_MIDI_BROADCAST_ID  0xff
#define MIOS32_CAN_MIDI_PORT_ALL      0x02

// default node destination is all
#ifndef MIOS32_CAN_MIDI_DEF_DEST_ID
#define MIOS32_CAN_MIDI_DEF_DEST_ID  MIOS32_CAN_MIDI_BROADCAST_ID
#endif
// default is vlan 0
#ifndef MIOS32_CAN_MIDI_DEF_VMAN
#define MIOS32_CAN_MIDI_DEF_VMAN      0
#endif
// default destination port is
#ifndef MIOS32_CAN_MIDI_DEF_DEST_PORT
#define MIOS32_CAN_MIDI_DEF_DEST_PORT MIOS32_CAN_MIDI_PORT_ALL
#endif


/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////

typedef union {
    can_packet_t  packet;
  struct {
    can_ext_id_t id;
    can_ctrl_t ctrl;
    can_data_t data;
  };
} mcan_packet_t;

typedef union {
  struct {
    can_ext_id_t id;
    u16 data_l;
  };
  struct {
    u32 ext_id;
    u16 ports;
  };
  struct {
    // extended
    u32 none:1;
    u32 is_request:1;
    u32 is_extended:1;
    u32 src_node:7;
    u32 dst_node:8;
    u32 app_type:3;
    // standard
    u32 cable:4;
    u32 type:4;
    u32 vman_prio:3;
    // ports
    u8  src_port;
    u8  dst_port;
  };
} mcan_header_t;


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 MIOS32_CAN_MIDI_Init(u32 mode);

extern s32 MIOS32_CAN_MIDI_VerboseSet(u8 level);
extern s32 MIOS32_CAN_MIDI_VerboseGet(void);
extern s32 MIOS32_CAN_MIDI_NodeIDSet(u8 node_id);
extern s32 MIOS32_CAN_MIDI_NodeIDGet(void);
extern s32 MIOS32_CAN_MIDI_ModeSet(u8 enabled);
extern s32 MIOS32_CAN_MIDI_ModeGet(void);

extern s32 MIOS32_CAN_MIDI_FilterInit(u8 bypass);

extern s32 MIOS32_CAN_MIDI_CheckAvailable(u8 cable);

extern s32 MIOS32_CAN_MIDI_Periodic_mS(void);

extern s32 MIOS32_CAN_MIDI_PacketTransmit_NonBlocking(mcan_packet_t p);
extern s32 MIOS32_CAN_MIDI_PacketTransmit(mcan_packet_t p);

extern s32 MIOS32_CAN_MIDI_PacketSend_NonBlocking(mcan_header_t header, mios32_midi_package_t package);
extern s32 MIOS32_CAN_MIDI_PacketSend(mcan_header_t header, mios32_midi_package_t package);

extern s32 MIOS32_CAN_MIDI_PackageSend_NonBlocking(mios32_midi_package_t package);
extern s32 MIOS32_CAN_MIDI_PackageSend(mios32_midi_package_t package);
extern s32 MIOS32_CAN_MIDI_SysexRepackSend(mcan_header_t header, mios32_midi_package_t package);

extern s32 MIOS32_CAN_MIDI_SysexSend_NonBlocking(mcan_header_t header, u8 *stream, u16 size);
extern s32 MIOS32_CAN_MIDI_SysexSend(mcan_header_t header, u8 *stream, u16 size);

extern s32 MIOS32_CAN_MIDI_SysExStreamCallback_Init(s32 (*callback_sysex_stream)(mcan_header_t header, u8* stream, u16 size));

extern s32 MIOS32_CAN_MIDI_PackageCallback_Init(s32 (*direct_package_callback)(mcan_header_t header, mios32_midi_package_t package));

extern s32 MIOS32_CAN_MIDI_PackageReceive(mios32_midi_package_t *package);

extern u32 MIOS32_CAN_MIDI_DefaultHeaderInit(mcan_header_t* header);
/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////
extern u8 mios32_mcan_id;


#endif /* _MIOS32_CAN_MIDI_H */
