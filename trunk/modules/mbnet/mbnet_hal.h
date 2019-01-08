// $Id: mbnet.h 1177 2011-04-18 23:20:38Z tk $
/*
 * Header file for MBNET hardware abstraction layer
 *
 * ==========================================================================
 *
 *  Copyright (C) 2008 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _MBNET_HAL_H
#define _MBNET_HAL_H

#include <mbnet.h>

typedef struct mbnet_packet_t {
  mbnet_id_t  id;
  u8          dlc;
  mbnet_msg_t msg;
} mbnet_packet_t;


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 MBNET_HAL_Init(u32 mode);
extern s32 MBNET_HAL_FilterInit(u8 node_id);

extern s32 MBNET_HAL_Send(mbnet_id_t mbnet_id, mbnet_msg_t msg, u8 dlc);
extern s32 MBNET_HAL_ReceiveAck(mbnet_packet_t *p);
extern s32 MBNET_HAL_ReceiveReq(mbnet_packet_t *p);

extern s32 MBNET_HAL_BusErrorCheck(void);

extern s32 MBNET_HAL_InstallTxHandler(s32 (*tx_handler_callback)(mbnet_id_t *mbnet_id, mbnet_msg_t *msg, u8 *dlc));
extern s32 MBNET_HAL_TriggerTxHandler(void);


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////


#endif /* _MBNET_HAL_H */
