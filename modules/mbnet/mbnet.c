// $Id$
/*
 * MBNet Functions
 *
 * STM32: Note that CAN and USB cannot be used at the same time!
 * Add following defines to your local mios32_config.h file to disable USB:
 * #define MIOS32_DONT_USE_USB
 *
 *
 * TODO: CAN Bus error handling!
 * TODO: remove slaves from info list which timed out during transaction!
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

#include <FreeRTOS.h>
#include <portmacro.h>

#include "mbnet.h"


/////////////////////////////////////////////////////////////////////////////
// for optional debugging messages via MIDI
/////////////////////////////////////////////////////////////////////////////
#define DEBUG_VERBOSE_LEVEL 0
#define DEBUG_MSG MIOS32_MIDI_SendDebugMessage


/////////////////////////////////////////////////////////////////////////////
// Pin definitions
/////////////////////////////////////////////////////////////////////////////

// note: remapped CAN IO - for different pins, 
// the MBNET_Init() function has to be enhanced
#define MBNET_RX_PORT     GPIOB
#define MBNET_RX_PIN      GPIO_Pin_8
#define MBNET_TX_PORT     GPIOB
#define MBNET_TX_PIN      GPIO_Pin_9



/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

// my node ID
static u8 my_node_id = 0xff; // (node not configured)

// master only: search node
static u8 search_slave_id = 0xff; // no slave yet

// info index of available slave nodes
// if >128, used as retry counter! (to save memory)
static u8 slave_nodes_ix[MBNET_SLAVE_NODES_END-MBNET_SLAVE_NODES_BEGIN+1];

// informations about scanned nodes (contains "pong" reply)
static mbnet_msg_t slave_nodes_info[MBNET_SLAVE_NODES_MAX];

// for retry mechanism
static u8          last_req_slave_id = 0xff;
static mbnet_id_t  last_req_mbnet_id;
static mbnet_msg_t last_req_msg;
static u8          last_req_dlc;


#if DEBUG_VERBOSE_LEVEL >= 1
// only for debugging: skip messages after more than 8 TOS=1/2
static u32 tos12_ctr = 0;
#endif


/////////////////////////////////////////////////////////////////////////////
// Initializes CAN interface
// IN: <mode>: currently only mode 0 supported
// OUT: returns < 0 if initialisation failed
/////////////////////////////////////////////////////////////////////////////
s32 MBNET_Init(u32 mode)
{
  u32 poll_ctr;
  int i;

  // currently only mode 0 supported
  if( mode != 0 )
    return -1; // unsupported mode

  // set default node ID to 0xff (node not configured)
  // The application has to use MBNET_NodeIDSet() to initialise it, and to configure the CAN filters
  my_node_id = 0xff;

  // invalidate slave id to be searched
  search_slave_id = 0xff;

  // clear slave node index
  for(i=0; i<=MBNET_SLAVE_NODES_END-MBNET_SLAVE_NODES_BEGIN; ++i)
    slave_nodes_ix[i] = 0xff-32; // invalid slave entry, retry 32 times

  // clear slave info
  for(i=0; i<MBNET_SLAVE_NODES_MAX; ++i) {
    slave_nodes_info[i].data_l = 0;
    slave_nodes_info[i].data_h = 0;
  }

  // remap CAN pins
  GPIO_PinRemapConfig(GPIO_Remap1_CAN, ENABLE);

  // configure CAN pins
  GPIO_InitTypeDef GPIO_InitStructure;
  GPIO_StructInit(&GPIO_InitStructure);
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;

  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
  GPIO_InitStructure.GPIO_Pin = MBNET_RX_PIN;
  GPIO_Init(MBNET_RX_PORT, &GPIO_InitStructure);
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_OD;
  GPIO_InitStructure.GPIO_Pin = MBNET_TX_PIN;
  GPIO_Init(MBNET_TX_PORT, &GPIO_InitStructure);

  // enable CAN clock
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_CAN, ENABLE);

#if 0
  // reset CAN
  RCC_APB1PeriphResetCmd(RCC_APB1Periph_CAN, ENABLE);
  RCC_APB1PeriphResetCmd(RCC_APB1Periph_CAN, DISABLE);
#endif

  // CAN initialisation

  // Request initialisation and wait until acknowledged
  CAN->MCR = (1 << 0); // CAN_MCR_INRQ
  poll_ctr = 1000; // poll up to 1000 times
  // according to datasheet, CAN waits until all ongoing transfers are finished
  while( !(CAN->MSR & (1 << 0)) ) { // CAN_MSR_INAK
    if( --poll_ctr == 0 )
      return -1; // initialisation failed
  }

  // enable receive FIFO locked mode (if FIFO is full, next incoming message will be discarded)
  CAN->MCR |= (1 << 3); // CAN_MCR_RFLM

  // bit timings for 2 MBaud:
  // -> 72 Mhz / 2 / 3 -> 12 MHz --> 6 quanta for 2 MBaud
  //          normal mode               Resynch. Jump Width   Time Segment 1         Time Segment 2       Prescaler
  CAN->BTR = (CAN_Mode_Normal << 30) | (CAN_SJW_1tq << 24) | (CAN_BS1_3tq << 16) | (CAN_BS2_2tq << 20) | (3 - 1);

  // leave initialisation mode and wait for acknowledge
  // Datasheet: we expect that the bus is unbusy after CAN has monitored a sequence of 11 consecutive recessive bits
  CAN->MCR &= ~(1 << 0); // CAN_MCR_INRQ
  poll_ctr = 1000; // poll up to 1000 times
  while( !(CAN->MSR & (1 << 0)) ) { // CAN_MSR_INAK
    if( --poll_ctr == 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
      DEBUG_MSG("[MBNET] ----- initialisation FAILED! ------------------------------------------\n");
#endif
      return -1; // initialisation failed
    }
  }

#if DEBUG_VERBOSE_LEVEL >= 1
  DEBUG_MSG("[MBNET] ----- initialized -----------------------------------------------------\n");
#endif

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Changes the Node ID and configures the CAN filters accordingly
// IN: node_id: configures the MBNet Node ID
//              if [3:0] == 0 (0x00, 0x10, 0x20, ... 0x70), the node can act
//              as a MBNet master and access MBNet slaves
// OUT: returns < 0 if configuration failed
/////////////////////////////////////////////////////////////////////////////
s32 MBNET_NodeIDSet(u8 node_id)
{
  u32 filter_mask;
  CAN_FilterInitTypeDef CAN_FilterInitStructure;

  // my node_id must be 0..127
  if( node_id >= 128 )
    return -1;

  // take over node ID
  my_node_id = node_id;

  // CAN filter initialisation

  // filter/FIFO 0 checks for incoming request messages directed to this node
  CAN_FilterInitStructure.CAN_FilterNumber=0;
  CAN_FilterInitStructure.CAN_FilterMode=CAN_FilterMode_IdMask;
  CAN_FilterInitStructure.CAN_FilterScale=CAN_FilterScale_32bit;
  CAN_FilterInitStructure.CAN_FilterIdHigh=(my_node_id << 8); // ACK=0
  CAN_FilterInitStructure.CAN_FilterIdLow=0x0004; // only accept EID=1, RTR=0
  CAN_FilterInitStructure.CAN_FilterMaskIdHigh=0xff00;
  CAN_FilterInitStructure.CAN_FilterMaskIdLow=0x0006; // check for EID and RTR
  CAN_FilterInitStructure.CAN_FilterFIFOAssignment=0;
  CAN_FilterInitStructure.CAN_FilterActivation=ENABLE;
  CAN_FilterInit(&CAN_FilterInitStructure);

  // filter/FIFO 1 matches on incoming acknowledge messages directed to this node
  CAN_FilterInitStructure.CAN_FilterNumber=1;
  CAN_FilterInitStructure.CAN_FilterMode=CAN_FilterMode_IdMask;
  CAN_FilterInitStructure.CAN_FilterScale=CAN_FilterScale_32bit;
  CAN_FilterInitStructure.CAN_FilterIdHigh=(1 << 15) | (my_node_id << 8); // ACK=1
  CAN_FilterInitStructure.CAN_FilterIdLow=0x0004; // only accept EID=1, RTR=0
  CAN_FilterInitStructure.CAN_FilterMaskIdHigh=0xff00;
  CAN_FilterInitStructure.CAN_FilterMaskIdLow=0x0006; // check for EID and RTR
  CAN_FilterInitStructure.CAN_FilterFIFOAssignment=1;
  CAN_FilterInitStructure.CAN_FilterActivation=ENABLE;
  CAN_FilterInit(&CAN_FilterInitStructure);

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Returns the Node ID
// IN: -
// OUT: if -1: node not configured yet
//      if >= 0: node ID
/////////////////////////////////////////////////////////////////////////////
s32 MBNET_NodeIDGet(void)
{
  return my_node_id == 0xff ? -1 : my_node_id;
}


/////////////////////////////////////////////////////////////////////////////
// Returns informations about a found node
// IN: slave node ID (MBNET_SLAVE_NODES_BEGIN..MBNET_SLAVE_NODES_END)
// OUT: "pong" reply in *info
//      returns 0 if slave has been scanned
//      returns -1 if this node hasn't been configured yet
//      returns -2 if this node isn't configured as master
//      returns -3 if slave index outside allowed range
//      returns -4 if slave info not available (no slave found at this index)
/////////////////////////////////////////////////////////////////////////////
s32 MBNET_SlaveNodeInfoGet(u8 slave_id, mbnet_msg_t *info)
{
  int i;
  u8 ix;
  
  if( my_node_id >= 128 )
    return -1; // node not configured

  if( my_node_id & 0x0f )
    return -2; // no master

#if MBNET_SLAVE_NODES_BEGIN > 0
  if( slave_id < MBNET_SLAVE_NODES_BEGIN || slave_id > MBNET_SLAVE_NODES_END )
#else
  if( slave_id > MBNET_SLAVE_NODES_END )
#endif
    return -3; // outside allowed range

  if( (ix=slave_nodes_ix[slave_id-MBNET_SLAVE_NODES_BEGIN]) >= 128 )
    return -4; // slave info not available

  info = &slave_nodes_info[ix];

  return 0;
}



/////////////////////////////////////////////////////////////////////////////
// internal function to send a MBNet message
// Used by MBNET_SendReq and MBNET_SendAck
// returns 0 if message sent successfully
// returns -3 on transmission error
/////////////////////////////////////////////////////////////////////////////
static s32 MBNET_SendMsg(mbnet_id_t mbnet_id, mbnet_msg_t msg, u8 dlc)
{
  // select an empty transmit mailbox
  s8 mailbox = -1;
  do {
    // TODO: timeout required?
    // TODO: check for CAN bus errors
    if     ( CAN->TSR & (1 << 26) ) // TME0
      mailbox = 0;
    else if( CAN->TSR & (1 << 27) ) // TME1
      mailbox = 1;
    else if( CAN->TSR & (1 << 28) ) // TME2
      mailbox = 2;
  } while( mailbox == -1 ); // TODO: provide timeout mechanism

  CAN->sTxMailBox[mailbox].TDTR = (dlc << 0); // set dlc, disable timestamp
  CAN->sTxMailBox[mailbox].TDLR = msg.data_l;
  CAN->sTxMailBox[mailbox].TDHR = msg.data_h;
  CAN->sTxMailBox[mailbox].TIR = (mbnet_id.ALL << 3) | (1 << 2) | (1 << 0); // id field, EID flag, TX Req flag

#if DEBUG_VERBOSE_LEVEL >= 2
  if( tos12_ctr <= 8 ) {
    DEBUG_MSG("[MBNET] sent %s via mailbox %d: ID:%02x TOS=%d DLC=%d MSG=%02x %02x %02x %02x %02x %02x %02x %02x\n",
	      mbnet_id.ack ? "ACK" : "REQ",
	      mailbox,
	      mbnet_id.node,
	      mbnet_id.tos,
	      dlc,
	      msg.bytes[0], msg.bytes[1], msg.bytes[2], msg.bytes[3],
	      msg.bytes[4], msg.bytes[5], msg.bytes[6], msg.bytes[7]);
  }
#endif

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Sends request to slave node
// IN: <slave_id>: slave node ID (0x00..0x7f)
//     <tos_req>: request TOS
//     <control>: 16bit control field of ID
//     <msg>: MBNet message (see mbnet_msg_t structure)
//     <dlc>: data field length (0..8)
// OUT: returns 0 if message sent successfully
//      returns -1 if this node hasn't been configured yet
//      returns -2 if this node isn't configured as master
//      returns -3 on transmission error
/////////////////////////////////////////////////////////////////////////////
s32 MBNET_SendReq(u8 slave_id, mbnet_tos_req_t tos_req, u16 control, mbnet_msg_t msg, u8 dlc)
{
  s32 error;

  if( my_node_id >= 128 )
    return -1; // node not configured

  if( my_node_id & 0x0f )
    return -2; // this node isn's configured as master

  mbnet_id_t mbnet_id;
  mbnet_id.control = control;
  mbnet_id.tos     = tos_req;
  mbnet_id.ms      = my_node_id >> 4;
  mbnet_id.ack     = 0;
  mbnet_id.node    = slave_id;

  // remember settings for MBNET_SendReqAgain()
  last_req_slave_id = slave_id;
  last_req_mbnet_id = mbnet_id;
  last_req_msg = msg;
  last_req_dlc = dlc;

  return MBNET_SendMsg(mbnet_id, msg, dlc);
}


/////////////////////////////////////////////////////////////////////////////
// If a slave acknowledged with retry, the last message has to be sent again
// It can be initiated with this function
// Only required if MBNET_WaitAck_NonBlocked() is used!
// Automatically handled by MBNET_WaitAck()
// IN: <slave_id>: slave node ID (0x00..0x7f)
// OUT: returns 0 if message sent successfully
//      returns -1 if this node hasn't been configured yet
//      returns -2 if this node isn't configured as master
//      returns -3 on transmission error
//      returns -7 if slave message lost (sequence error)
/////////////////////////////////////////////////////////////////////////////
s32 MBNET_SendReqAgain(u8 slave_id)
{
  s32 error;

  if( my_node_id >= 128 )
    return -1; // node not configured

  if( my_node_id & 0x0f )
    return -2; // this node isn's configured as master

  if( slave_id != last_req_slave_id )
    return -7; // sequence error

  return MBNET_SendMsg(last_req_mbnet_id, last_req_msg, last_req_dlc);
}


/////////////////////////////////////////////////////////////////////////////
// Sends acknowledge reply to master node
// IN: <master_id>:  master node ID (0x00, 0x10, 0x20, ... 0x70)
//     <tos_ack>: acknowledge TOS
//     <msg>: MBNet message (see mbnet_msg_t structure)
//     <dlc>: data field length (0..8)
// OUT: returns 0 if message sent successfully
//      returns -1 if this node hasn't been configured yet
//      returns -2 if destination node is not a master node
//      returns -3 on transmission error
/////////////////////////////////////////////////////////////////////////////
s32 MBNET_SendAck(u8 master_id, mbnet_tos_ack_t tos_ack, mbnet_msg_t msg, u8 dlc)
{
  if( my_node_id >= 128 )
    return -1; // node not configured

  if( master_id & 0x0f )
    return -2; // destination node is not a master node

  mbnet_id_t mbnet_id;
  mbnet_id.control = my_node_id;
  mbnet_id.tos     = tos_ack;
  mbnet_id.ms      = 0;
  mbnet_id.ack     = 1;
  mbnet_id.node    = master_id;

  return MBNET_SendMsg(mbnet_id, msg, dlc);
}



/////////////////////////////////////////////////////////////////////////////
// Checks for acknowledge from a specific slave
// IN: <slave_id>: slave node ID (0x00..0x7f)
//     <*msg>: received MBNet message (see mbnet_msg_t structure)
//     <*dlc>: received data field length (0..8)
// OUT: returns 0 if message received successfully
//      returns -1 if this node hasn't been configured yet
//      returns -2 if this node isn't configured as master
//      returns -3 on transmission error
//      returns -4 if slave acknowledged with retry (send message again)
//      returns -5 if no response from slave yet (check again or timeout!)
/////////////////////////////////////////////////////////////////////////////
s32 MBNET_WaitAck_NonBlocked(u8 slave_id, mbnet_msg_t *ack_msg, u8 *dlc)
{
  s32 error;

  if( my_node_id >= 128 )
    return -1; // node not configured

  if( my_node_id & 0x0f )
    return -2; // this node isn's configured as master

  // check for incoming FIFO1 messages (MBNet acknowledges)
  u8 again;   // allows to poll the FIFO again
  do {
    again = 0;
    if( CAN->RF1R & 0x3 ) { // FMP1 contains number of messages
      // get EID, MSG and DLC
      mbnet_id_t mbnet_id;
      mbnet_id.ALL = CAN->sFIFOMailBox[1].RIR >> 3;
      *dlc = CAN->sFIFOMailBox[1].RDTR & 0xf;
      ack_msg->data_l = CAN->sFIFOMailBox[1].RDLR;
      ack_msg->data_h = CAN->sFIFOMailBox[1].RDHR;

      // release FIFO, and check again next loop
      CAN->RF1R = (1 << 5); // set RFOM1 flag
      again = 1;

      // response from expected slave? if not - ignore it!
      if( (mbnet_id.control & 0xff) == slave_id ) {
#if DEBUG_VERBOSE_LEVEL >= 2
	DEBUG_MSG("[MBNET] got ACK from slave ID %02x TOS=%d DLC=%d MSG=%02x %02x %02x %02x %02x %02x %02x %02x\n",
		  slave_id,
		  mbnet_id.tos,
		  *dlc,
		  ack_msg->bytes[0], ack_msg->bytes[1], ack_msg->bytes[2], ack_msg->bytes[3],
		  ack_msg->bytes[4], ack_msg->bytes[5], ack_msg->bytes[6], ack_msg->bytes[7]);
#endif

	// retry requested?
	if( mbnet_id.tos == MBNET_ACK_RETRY ) {
#if DEBUG_VERBOSE_LEVEL >= 2
	  DEBUG_MSG("[MBNET] Slave ID %02x requested to retry the transfer!\n", slave_id);
#endif
	  return -4; // slave requested to retry
	}

	return 0; // wait ack successful!
      } else {
#if DEBUG_VERBOSE_LEVEL >= 1
	DEBUG_MSG("[MBNET] ERROR: ACK from unexpected slave ID %02x TOS=%d DLC=%d MSG=%02x %02x %02x %02x %02x %02x %02x %02x\n",
		  mbnet_id.node,
		  mbnet_id.tos,
		  *dlc,
		  ack_msg->bytes[0], ack_msg->bytes[1], ack_msg->bytes[2], ack_msg->bytes[3],
		  ack_msg->bytes[4], ack_msg->bytes[5], ack_msg->bytes[6], ack_msg->bytes[7]);
#endif
	again = 1;
      }
    }
  } while( again );

  return -5; // no response from slave yet (check again or timeout!)
}


/////////////////////////////////////////////////////////////////////////////
// Checks for acknowledge from a specific slave
// (blocked function)
// IN: <slave_id>: slave node ID (0x00..0x7f)
//     <*msg>: received MBNet message (see mbnet_msg_t structure)
//     <*dlc>: received data field length (0..8)
// OUT: returns 0 if message received successfully
//      returns -1 if this node hasn't been configured yet
//      returns -2 if this node isn't configured as master
//      returns -3 on transmission error
//      returns -6 on timeout (no response within 10 mS)
/////////////////////////////////////////////////////////////////////////////
s32 MBNET_WaitAck(u8 slave_id, mbnet_msg_t *ack_msg, u8 *dlc)
{
  // TODO: timeout after 10 mS, use system timer!
  u32 timeout_ctr = 0;
  s32 error;

  do {
    error = MBNET_WaitAck_NonBlocked(slave_id, ack_msg, dlc);
    if( error == -4 ) {
      MBNET_SendReqAgain(slave_id);
      timeout_ctr = 0;
    }
  } while( (error == -4 || error == -5) && ++timeout_ctr < 1000 );

  if( timeout_ctr == 1000 ) {
#if DEBUG_VERBOSE_LEVEL >= 2
    DEBUG_MSG("[MBNET] ACK polling for slave ID %02x timed out!\n", slave_id);
#endif
    return -6; // timeout
  }

  return error;
}


/////////////////////////////////////////////////////////////////////////////
// Handles CAN messages, should be called periodically to check the BUS
// state and to check for incoming messages
//
// Application specific ETOS, READ, WRITE and PING requests are forwarded to
// the callback function with following parameters:
//   void <callback>(u8 master_id, u8 tos, u16 control, mbnet_msg_t req_msg, u8 dlc)
// In any case, the function has to send an acknowledge message back
// to the master node!
//
// IN: pointer to callback function
// OUT: returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 MBNET_Handler(void *_callback)
{
  if( my_node_id >= 128 )
    return -1; // node not configured

  s32 (*callback)(u8 master_id, mbnet_tos_req_t tos, u16 control, mbnet_msg_t req_msg, u8 dlc) = _callback;

  // scan for slave nodes if this node is a master
  if( (my_node_id & 0x0f) == 0 ) {
    
    // determine next slave node
    u8 again = 0;
    do {
#if MBNET_SLAVE_NODES_BEGIN > 0
      if( search_slave_id < MBNET_SLAVE_NODES_BEGIN || search_slave_id > MBNET_SLAVE_NODES_END )
#else
      if( search_slave_id > MBNET_SLAVE_NODES_END )
#endif
	search_slave_id = MBNET_SLAVE_NODES_BEGIN-1; // restart

      // next slave
      ++search_slave_id;

      if( search_slave_id == my_node_id )
	++search_slave_id; // skip my own ID
    } while( again );

    // search slave if not found yet, and retry counter hasn't reached end yet
    u8 ix_ix = search_slave_id-MBNET_SLAVE_NODES_BEGIN;
    if( slave_nodes_ix[ix_ix] >= 128 && slave_nodes_ix[ix_ix] != 0xff ) {
      // increment retry counter
      ++slave_nodes_ix[ix_ix];

      // send a ping, wait for a pong
      mbnet_msg_t req_msg;
      req_msg.data_l = 0;
      req_msg.data_h = 0;
      if( MBNET_SendReq(search_slave_id, MBNET_REQ_PING, 0x0000, req_msg, 0) == 0 ) {
	mbnet_msg_t ack_msg;
	u8 dlc;
	if( MBNET_WaitAck(search_slave_id, &ack_msg, &dlc) == 0 ) {

	  // got it! :-)
	  slave_nodes_ix[ix_ix] = search_slave_id;

	  // try to put it into info array
	  int i;
	  for(i=0; i<MBNET_SLAVE_NODES_MAX; ++i) {
	    if( slave_nodes_info[i].protocol_version == 0 ) {
	      slave_nodes_info[i].data_l = ack_msg.data_l;
	      slave_nodes_info[i].data_h = ack_msg.data_h;
	      slave_nodes_ix[ix_ix] = i;
	      break;
	    }
	  }

	  // still a free slot?
	  if( i >= MBNET_SLAVE_NODES_MAX ) {
	    slave_nodes_ix[ix_ix] = 0xff; // disable retry counter
	  }

#if DEBUG_VERBOSE_LEVEL >= 1
	  DEBUG_MSG("[MBNET] new slave found: ID %02x (Ix=%d) P:%d T:%c%c%c%c V:%d.%d\n", 
		    search_slave_id,
		    slave_nodes_ix[ix_ix],
		    ack_msg.protocol_version,
		    ack_msg.node_type[0], ack_msg.node_type[1], ack_msg.node_type[2], ack_msg.node_type[3],
		    ack_msg.node_version, ack_msg.node_subversion);

	  if( slave_nodes_ix[ix_ix] == 0xff )
	    DEBUG_MSG("[MBNET] unfortunately no free info slot anymore!\n");
#endif
	}
      }
    }
    

  }

  u8 again = 0;   // allows to poll the FIFO again
  s8 locked = -1; // contains MS node ID if slave has been locked, otherwise -1 if not locked
#if DEBUG_VERBOSE_LEVEL >= 1
  tos12_ctr = 0; // only for debugging
#endif
  do {
    // check for incoming FIFO0 messages (MBNet requests)
    if( CAN->RF0R & 0x3 ) { // FMP0 contains number of messages
      // get EID, MSG and DLC
      mbnet_id_t mbnet_id;
      mbnet_id.ALL = CAN->sFIFOMailBox[0].RIR >> 3;
      u8 dlc = CAN->sFIFOMailBox[0].RDTR & 0xf;
      mbnet_msg_t req_msg;
      req_msg.data_l = CAN->sFIFOMailBox[0].RDLR;
      req_msg.data_h = CAN->sFIFOMailBox[0].RDHR;

      // release FIFO, and check again next loop
      CAN->RF0R = (1 << 5); // set RFOM0 flag
      again = 1;

      // master node ID
      u8 master_id = mbnet_id.ms << 4;

#if DEBUG_VERBOSE_LEVEL >= 1
      if( mbnet_id.tos == 1 || mbnet_id.tos == 2 ) {
	++tos12_ctr;
	if( tos12_ctr > 8 ) {
	  DEBUG_MSG("[MBNET] ... skip display of remaining read/write operations\n");
	}
      } else {
	tos12_ctr = 0;
      }
#endif
#if DEBUG_VERBOSE_LEVEL >= 2
      if( tos12_ctr <= 8 ) {
	DEBUG_MSG("[MBNET] request ID=%02x TOS=%d CTRL=%04x DLC=%d MSG=%02x %02x %02x %02x %02x %02x %02x %02x\n",
		  master_id,
		  mbnet_id.tos,
		  mbnet_id.control,
		  dlc,
		  req_msg.bytes[0], req_msg.bytes[1], req_msg.bytes[2], req_msg.bytes[3],
		  req_msg.bytes[4], req_msg.bytes[5], req_msg.bytes[6], req_msg.bytes[7]);
      }
#endif
      // default acknowledge message is empty
      mbnet_msg_t ack_msg;
      ack_msg.data_l = ack_msg.data_h = 0;

      // if slave has been locked by another master: send retry acknowledge
      if( locked != -1 && mbnet_id.ms != locked ) {
	MBNET_SendAck(master_id, MBNET_ACK_RETRY, ack_msg, 0); // master_id, tos, msg, dlc
#if DEBUG_VERBOSE_LEVEL >= 2
	DEBUG_MSG("[MBNET] ignore - node is locked by MS=%d\n", locked);
#endif
      } else {
	// branch depending on TOS
	switch( mbnet_id.tos ) {
          case MBNET_REQ_SPECIAL:
	    // branch depending on ETOS (extended TOS)
	    switch( mbnet_id.control & 0xff ) {
	      case 0x00: // lock receiver
		locked = mbnet_id.ms;
		MBNET_SendAck(master_id, MBNET_ACK_OK, ack_msg, 0); // master_id, tos, msg, dlc
		break;
      	      case 0x01: // unlock receiver
		locked = -1;
		MBNET_SendAck(master_id, MBNET_ACK_OK, ack_msg, 0); // master_id, tos, msg, dlc
		break;
      	      case 0x02: // reserved
		MBNET_SendAck(master_id, MBNET_ACK_ERROR, ack_msg, 0); // master_id, tos, msg, dlc
		break;
      	      case 0x03: // reserved
		MBNET_SendAck(master_id, MBNET_ACK_ERROR, ack_msg, 0); // master_id, tos, msg, dlc
		break;
      	      case 0x04: // call APP_Init
		APP_Init();
		MBNET_SendAck(master_id, MBNET_ACK_OK, ack_msg, 0); // master_id, tos, msg, dlc
		break;
      	      case 0x05: // call APP_MPROC_DebugTrigger not implemented
		MBNET_SendAck(master_id, MBNET_ACK_ERROR, ack_msg, 0); // master_id, tos, msg, dlc
		break;
      	      case 0x06: // call APP_NotifyReceivedEvent
		if( dlc != 3 ) {
		  MBNET_SendAck(master_id, MBNET_ACK_ERROR, ack_msg, 0); // master_id, tos, msg, dlc
		} else {
		  mios32_midi_package_t package;
		  APP_NotifyReceivedEvent(0, package); // TODO: we could forward the port number as well
		  MBNET_SendAck(master_id, MBNET_ACK_OK, ack_msg, 0); // master_id, tos, msg, dlc
		}
		break;
	      case 0x07: // call APP_NotifyReceivedSysEx
		if( dlc != 1 ) {
		  MBNET_SendAck(master_id, MBNET_ACK_ERROR, ack_msg, 0); // master_id, tos, msg, dlc
		} else {
		  APP_NotifyReceivedSysEx(0, req_msg.bytes[0]); // TODO: we could forward the port number as well
		  MBNET_SendAck(master_id, MBNET_ACK_OK, ack_msg, 0); // master_id, tos, msg, dlc
		}
		break;
      	      case 0x08: // call APP_DIN_NotifyToggle
		if( dlc != 2 ) {
		  MBNET_SendAck(master_id, MBNET_ACK_ERROR, ack_msg, 0); // master_id, tos, msg, dlc
		} else {
		  APP_DIN_NotifyToggle(req_msg.bytes[0], req_msg.bytes[1]);
		  MBNET_SendAck(master_id, MBNET_ACK_OK, ack_msg, 0); // master_id, tos, msg, dlc
		}
		break;
      	      case 0x09: // call APP_ENC_NotifyChange
		if( dlc != 2 ) {
		  MBNET_SendAck(master_id, MBNET_ACK_ERROR, ack_msg, 0); // master_id, tos, msg, dlc
		} else {
		  APP_ENC_NotifyChange(req_msg.bytes[0], req_msg.bytes[1]);
		  MBNET_SendAck(master_id, MBNET_ACK_OK, ack_msg, 0); // master_id, tos, msg, dlc
		}
		break;
      	      case 0x0a: // call APP_AIN_NotifyChange
		if( dlc != 3 ) {
		  MBNET_SendAck(master_id, MBNET_ACK_ERROR, ack_msg, 0); // master_id, tos, msg, dlc
		} else {
		  APP_ENC_NotifyChange(req_msg.bytes[0], req_msg.bytes[1] | (req_msg.bytes[2] << 8));
		  MBNET_SendAck(master_id, MBNET_ACK_OK, ack_msg, 0); // master_id, tos, msg, dlc
		}
		break;
      	      case 0x0b: // reserved
		MBNET_SendAck(master_id, MBNET_ACK_ERROR, ack_msg, 0); // master_id, tos, msg, dlc
		break;
      	      case 0x0c: // reserved
		MBNET_SendAck(master_id, MBNET_ACK_ERROR, ack_msg, 0); // master_id, tos, msg, dlc
		break;
      	      case 0x0d: // reserved
		MBNET_SendAck(master_id, MBNET_ACK_ERROR, ack_msg, 0); // master_id, tos, msg, dlc
		break;
      	      case 0x0e: // reserved
		MBNET_SendAck(master_id, MBNET_ACK_ERROR, ack_msg, 0); // master_id, tos, msg, dlc
		break;
      	      case 0x0f: // cloning not supported
		MBNET_SendAck(master_id, MBNET_ACK_ERROR, ack_msg, 0); // master_id, tos, msg, dlc
		break;
    	      default:
		// send ETOS to application via callback hook
		callback(mbnet_id.ms << 4, mbnet_id.tos, mbnet_id.control, req_msg, dlc);
		break;
	    }
	    break;
  
	  case MBNET_REQ_RAM_READ:
          case MBNET_REQ_RAM_WRITE:
          case MBNET_REQ_PING:
	    // handled by application
	    callback(mbnet_id.ms << 4, mbnet_id.tos, mbnet_id.control, req_msg, dlc);
	    break;

	  default: // mbnet_id.tos>=4 will never be received, just for the case...
	    MBNET_SendAck(master_id, MBNET_ACK_ERROR, ack_msg, 0); // master_id, tos, msg, dlc
	    break;
        }  
      }
    }
  } while( again || locked != -1);

  return 0; // no error
}
