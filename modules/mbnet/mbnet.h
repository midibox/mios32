// $Id$
/*
 * Header file for MBNET functions
 *
 * ==========================================================================
 *
 *  Copyright (C) 2008 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _MBNET_H
#define _MBNET_H

/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////

// relevant if configured as master: how many nodes should be scanned maximum
#ifndef MBNET_SLAVE_NODES_MAX
#define MBNET_SLAVE_NODES_MAX 8
#endif

// first node to be scanned
#ifndef MBNET_SLAVE_NODES_BEGIN
#define MBNET_SLAVE_NODES_BEGIN 0x00
#endif

// last node to be scanned
#ifndef MBNET_SLAVE_NODES_END
#define MBNET_SLAVE_NODES_END   0x1f
#endif



/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////

typedef enum {
  MBNET_REQ_SPECIAL,
  MBNET_REQ_RAM_READ,
  MBNET_REQ_RAM_WRITE,
  MBNET_REQ_PING
} mbnet_tos_req_t;

typedef enum {
  MBNET_ACK_OK,
  MBNET_ACK_READ,
  MBNET_ACK_RETRY,
  MBNET_ACK_ERROR
} mbnet_tos_ack_t;

typedef union {
  u32 ALL;
  struct {
    u16 control;
    u32 tos:2;
    u32 ms:3;
    u32 node:7;
    u32 ack:1;
  };
} mbnet_id_t;

typedef union {
  struct {
    u32 data[2];
  };
  struct {
    u32 data_l;
    u32 data_h;
  };
  struct {
    u8 bytes[8];
  };
  struct {
    u8  protocol_version;
    u8  node_type[4];
    u8  node_version;
    u16 node_subversion;
  };
} mbnet_msg_t;


typedef union {
  u8 ALL;
  struct {
    u8 PANIC:1;
    u8 PERMANENT_OFF:1;
  };
} mbnet_state_t;


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 MBNET_Init(u32 mode);

extern s32 MBNET_NodeIDSet(u8 node_id);
extern s32 MBNET_NodeIDGet(void);

extern s32 MBNET_SlaveNodeInfoGet(u8 slave_id, mbnet_msg_t *info);

extern s32 MBNET_Reconnect(void);

extern s32 MBNET_ErrorStateGet(void);

extern s32 MBNET_SendReq(u8 slave_id, mbnet_tos_req_t tos_req, u16 control, mbnet_msg_t msg, u8 dlc);
extern s32 MBNET_SendReqAgain(u8 slave_id);
extern s32 MBNET_SendAck(u8 master_id, mbnet_tos_ack_t tos_ack, mbnet_msg_t msg, u8 dlc);

extern s32 MBNET_WaitAck_NonBlocking(u8 slave_id, mbnet_msg_t *ack_msg, u8 *dlc);
extern s32 MBNET_WaitAck(u8 slave_id, mbnet_msg_t *ack_msg, u8 *dlc);

extern s32 MBNET_Handler(void *_callback);


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////


#endif /* _MBNET_H */
