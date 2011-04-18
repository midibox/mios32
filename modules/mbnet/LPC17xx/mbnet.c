// $Id$
/*
 * MBNet Functions
 *
 * TODO: try recovery on CAN Bus errors
 * TODO: remove slaves from info list which timed out during transaction!
 * TODO: try to share more code between LPC17 and STM32 driver in higher application layer
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

#include <FreeRTOS.h>
#include <portmacro.h>

#include "mbnet.h"


/////////////////////////////////////////////////////////////////////////////
// for optional debugging messages via MIDI
/////////////////////////////////////////////////////////////////////////////
#define DEBUG_VERBOSE_LEVEL 1
#define DEBUG_MSG MIOS32_MIDI_SendDebugMessage


/////////////////////////////////////////////////////////////////////////////
// Peripheral/Pin definitions
/////////////////////////////////////////////////////////////////////////////

// pointer to CAN peripheral
#define MBNET_CAN      LPC_CAN2
// index of CAN counted from 0 (0..1)
#define MBNET_CAN_IX   1

// CAN clocked at CCLK (typically 100 MHz)
#define CAN_PERIPHERAL_FRQ (MIOS32_SYS_CPU_FREQUENCY)

// RxD: P2.7 (RD2)
#define MBNET_RXD_INIT    { MIOS32_SYS_LPC_PINSEL(2, 7, 1); }
// TxD: P2.8 (TD2)
#define MBNET_TXD_INIT    { MIOS32_SYS_LPC_PINSEL(2, 8, 1); }


// size of the two request/acknowledge receive FIFOs
#define MBNET_RX_FIFO_SIZE 2

#define MBNET_TIMEOUT_CTR_MAX 5000

/////////////////////////////////////////////////////////////////////////////
// Local types
/////////////////////////////////////////////////////////////////////////////

typedef struct mbnet_fifo_t {
  mbnet_id_t  id;
  mbnet_msg_t msg;
  u8          dlc;
} mbnet_fifo_t;


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

// error state flags
mbnet_state_t mbnet_state;

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


#if DEBUG_VERBOSE_LEVEL >= 2
// only for debugging: skip messages after more than 8 TOS=1/2
static u32 tos12_ctr = 0;
#endif


// receive fifos
static mbnet_fifo_t mbnet_fifo_req[MBNET_RX_FIFO_SIZE];
static volatile u8 mbnet_fifo_req_tail;
static volatile u8 mbnet_fifo_req_head;
static volatile u8 mbnet_fifo_req_size;

static mbnet_fifo_t mbnet_fifo_ack[MBNET_RX_FIFO_SIZE];
static volatile u8 mbnet_fifo_ack_tail;
static volatile u8 mbnet_fifo_ack_head;
static volatile u8 mbnet_fifo_ack_size;


/////////////////////////////////////////////////////////////////////////////
// Local prototypes
/////////////////////////////////////////////////////////////////////////////
static s32 MBNET_BusErrorCheck(void);


/////////////////////////////////////////////////////////////////////////////
// Initializes CAN interface
// IN: <mode>: currently only mode 0 supported
// OUT: returns < 0 if initialisation failed
/////////////////////////////////////////////////////////////////////////////
s32 MBNET_Init(u32 mode)
{
  // currently only mode 0 supported
  if( mode != 0 )
    return -1; // unsupported mode

  // clear state flags
  mbnet_state.ALL = 0;

  // set default node ID to 0xff (node not configured)
  // The application has to use MBNET_NodeIDSet() to initialise it, and to configure the CAN filters
  my_node_id = 0xff;

  // invalidate slave informations
  MBNET_Reconnect();

  // clear FIFOs
  mbnet_fifo_req_tail = mbnet_fifo_req_head = mbnet_fifo_req_size = 0;
  mbnet_fifo_ack_tail = mbnet_fifo_ack_head = mbnet_fifo_ack_size = 0;

  // init CAN pins
  MBNET_RXD_INIT;
  MBNET_TXD_INIT;

  // CAN initialisation
  MBNET_CAN->MOD = 1; // Enter Reset Mode
  MBNET_CAN->IER = 0; // Disable All CAN Interrupts
  MBNET_CAN->GSR = 0; // clear error status

  // Request command to release Rx, Tx buffer and clear data overrun
  //MBNET_CAN->CMR = CAN_CMR_AT | CAN_CMR_RRB | CAN_CMR_CDO;
  MBNET_CAN->CMR = (1<<1) | (1<<2) | (1<<3);
  // Read to clear interrupt pending in interrupt capture register
  if( MBNET_CAN->ICR );

  // Reset CANAF value
  LPC_CANAF->AFMR = 0x01;

  LPC_CANAF->SFF_sa = 0x00;
  LPC_CANAF->SFF_GRP_sa = 0x00;
  LPC_CANAF->EFF_sa = 0x00;
  LPC_CANAF->EFF_GRP_sa = 0x00;
  LPC_CANAF->ENDofTable = 0x00;

  LPC_CANAF->AFMR = 0x02;

  // configure CAN interrupt
  MBNET_CAN->IER = (1 << 0); // only receive interrupts

  // Set bit timing
#if CAN_PERIPHERAL_FRQ == 50000000
  // -> 50 Mhz / 1 -> 25 MHz --> 25 quanta for 2 MBaud
  //                 TESG2         TESG1         SJW         Prescaler
  //                 -> 8          -> 16         -> 1        1:1
  MBNET_CAN->BTR  = (0x7 << 20) | (0xf << 16) | (0 << 14) | (0 << 0);
#elif CAN_PERIPHERAL_FRQ == 100000000
  // -> 100 Mhz / 5 -> 20 MHz --> 10 quanta for 2 MBaud
  //                 TESG2         TESG1         SJW         Prescaler
  //                 -> 3          -> 6         -> 1        1:5
  MBNET_CAN->BTR  = (0x2 << 20) | (0x5 << 16) | (0 << 14) | (4 << 0);
#else
# error "BTR baudrate not prepared for this peripheral frequency"
#endif

  // Return to normal operation
  MBNET_CAN->MOD = 0;

  // enable CAN interrupt
  // TODO: use NVIC_SetPriority; will require some encoding...
  u32 tmppriority = (0x700 - ((SCB->AIRCR) & (uint32_t)0x700)) >> 8;
  u32 tmppre = (4 - tmppriority);
  tmppriority = MIOS32_IRQ_PRIO_HIGH << tmppre;
  tmppriority = tmppriority << 4;
  NVIC->IP[CAN_IRQn] = tmppriority;

  NVIC_EnableIRQ(CAN_IRQn);

#if DEBUG_VERBOSE_LEVEL >= 2
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
  // my node_id must be 0..127
  if( node_id >= 128 )
    return -1;

  // take over node ID
  my_node_id = node_id;

  // CAN filter initialisation
  LPC_CANAF->AFMR = 0x02; // filter bypass to allow configuration

  // unfortunately the "FullCAN" function can't be used here, because it doesn't support extended IDs.
  // means: we can't organize two FIFOs for incoming request and acknowledge messages
  // therefore we just set a mask for this node, and handle FIFO buffering via software

  // range 1: requests (ACK=0)
  LPC_CANAF_RAM->mask[0] = (MBNET_CAN_IX << 29) | (0 << 28) | (my_node_id << 21) | 0x000000;
  LPC_CANAF_RAM->mask[1] = (MBNET_CAN_IX << 29) | (0 << 28) | (my_node_id << 21) | 0x1fffff;

  // range 2: acknowledges (ACK=1)
  LPC_CANAF_RAM->mask[2] = (MBNET_CAN_IX << 29) | (1 << 28) | (my_node_id << 21) | 0x000000;
  LPC_CANAF_RAM->mask[3] = (MBNET_CAN_IX << 29) | (1 << 28) | (my_node_id << 21) | 0x1fffff;

  // setup table pointers
  LPC_CANAF->SFF_sa     = 0;
  LPC_CANAF->SFF_GRP_sa = 0;
  LPC_CANAF->EFF_sa     = 0;
  LPC_CANAF->EFF_GRP_sa = 0x000;       // word address: start of extended ranges
  LPC_CANAF->ENDofTable = 0x000 + 4*4; // word address: end of extended ranges + 4

  LPC_CANAF->AFMR = 0x00; // filter on

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
// invalidates all slave informations and scans the MBNET (again)
// OUT: < 0 on errora
/////////////////////////////////////////////////////////////////////////////
s32 MBNET_Reconnect(void)
{
  int i;

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
}



/////////////////////////////////////////////////////////////////////////////
// Returns the MBNet error state:
// OUT: 0 if no errors
//      -1 if MBNet reached panic state (failing transactions, automatic retries to recover)
//      -2 if MBNet reached "permanent off" state (recovery not possible)
/////////////////////////////////////////////////////////////////////////////
s32 MBNET_ErrorStateGet(void)
{
  if( mbnet_state.PERMANENT_OFF )
    return -2;
  if( mbnet_state.PANIC )
    return -1;

  return 0; // no error
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
    // exit immediately if CAN bus errors (CAN doesn't send messages anymore)
    if( MBNET_BusErrorCheck() < 0 )
      return -3; // transmission error

    // TODO: timeout required?
    u32 sr = MBNET_CAN->SR;

    if     ( sr & (1 << 2) ) // TBS1
      mailbox = 0;
    else if( sr & (1 << 10) ) // TBS2
      mailbox = 1;
    else if( sr & (1 << 18) ) // TBS3
      mailbox = 2;
  } while( mailbox == -1 ); // TODO: provide timeout mechanism

  //DEBUG_MSG("%08x", mbnet_id.ALL);
  LPC_CAN_TypeDef *tx_base = (LPC_CAN_TypeDef *)((unsigned)(MBNET_CAN) + 0x10*mailbox);

  //              FF (extended frame)  DLC (length)   
  tx_base->TFI1 = (1 << 31)         | (dlc << 16);
  // the extended ID
  tx_base->TID1 = mbnet_id.ALL;
  // the playload
  tx_base->TDA1 = msg.data_l;
  tx_base->TDB1 = msg.data_h;

  // transmission request
  MBNET_CAN->CMR = (1 << (mailbox+5)) | (1 << 0);

#if 0
  u32 trace[10];
  trace[0] = MBNET_CAN->SR;
  trace[1] = MBNET_CAN->SR;
  trace[2] = MBNET_CAN->SR;
  trace[3] = MBNET_CAN->SR;
  trace[4] = MBNET_CAN->SR;
  trace[5] = MBNET_CAN->SR;
  trace[6] = MBNET_CAN->SR;
  trace[7] = MBNET_CAN->SR;
  trace[8] = MBNET_CAN->SR;
  trace[9] = MBNET_CAN->SR;

  int x = 0;
  while( !(MBNET_CAN->SR & (1 << 2)) ) {
    ++x;
  }
  DEBUG_MSG("x=%d\n", x);

  for(x=0; x<10; ++x)
    DEBUG_MSG("%d %08x\n", x, trace[x]);
#endif


#if DEBUG_VERBOSE_LEVEL >= 3
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

  // exit immediately if CAN bus errors (CAN doesn't send messages anymore)
  if( MBNET_BusErrorCheck() < 0 )
    return -3; // transmission error

  // check for incoming FIFO1 messages (MBNet acknowledges)
  u8 got_msg;
  do {
    got_msg = 0;
    MIOS32_IRQ_Disable();
    mbnet_fifo_t fifo;
    if( mbnet_fifo_ack_size ) {
      got_msg = 1;
      fifo = mbnet_fifo_ack[mbnet_fifo_ack_tail];
      if( ++mbnet_fifo_ack_tail >= MBNET_RX_FIFO_SIZE )
	mbnet_fifo_ack_tail = 0;
      --mbnet_fifo_ack_size;
    }
    MIOS32_IRQ_Enable();

    if( got_msg ) {
      // get EID, MSG and DLC
      mbnet_id_t mbnet_id;
      mbnet_id.ALL = MBNET_CAN->RID & 0x1fffffff;
      *dlc = fifo.dlc;
      ack_msg->data_l = fifo.msg.data_l;
      ack_msg->data_h = fifo.msg.data_h;

      // response from expected slave? if not - ignore it!
      if( (fifo.id.control & 0xff) == slave_id ) {
#if DEBUG_VERBOSE_LEVEL >= 3
	DEBUG_MSG("[MBNET] got ACK from slave ID %02x TOS=%d DLC=%d MSG=%02x %02x %02x %02x %02x %02x %02x %02x\n",
		  fifo.id.control & 0xff,
		  fifo.id.tos,
		  *dlc,
		  ack_msg->bytes[0], ack_msg->bytes[1], ack_msg->bytes[2], ack_msg->bytes[3],
		  ack_msg->bytes[4], ack_msg->bytes[5], ack_msg->bytes[6], ack_msg->bytes[7]);
#endif

	// retry requested?
	if( fifo.id.tos == MBNET_ACK_RETRY ) {
#if DEBUG_VERBOSE_LEVEL >= 3
	  DEBUG_MSG("[MBNET] Slave ID %02x requested to retry the transfer!\n", slave_id);
#endif
	  return -4; // slave requested to retry
	}

	return 0; // wait ack successful!
      } else {
#if DEBUG_VERBOSE_LEVEL >= 1
	DEBUG_MSG("[MBNET] ERROR: ACK from unexpected slave ID %02x (TOS=%d DLC=%d MSG=%02x %02x %02x...)\n",
		  fifo.id.control & 0xff,
		  fifo.id.tos,
		  *dlc,
		  ack_msg->bytes[0], ack_msg->bytes[1], ack_msg->bytes[2]);
#endif
      }
    }
  } while( got_msg );

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
  } while( (error == -4 || error == -5) && ++timeout_ctr < MBNET_TIMEOUT_CTR_MAX );

  if( timeout_ctr == MBNET_TIMEOUT_CTR_MAX ) {
#if DEBUG_VERBOSE_LEVEL >= 3
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
      if( search_slave_id < MBNET_SLAVE_NODES_BEGIN || search_slave_id >= MBNET_SLAVE_NODES_END )
#else
      if( search_slave_id >= MBNET_SLAVE_NODES_END )
#endif
	search_slave_id = MBNET_SLAVE_NODES_BEGIN-1; // restart

      // next slave
      ++search_slave_id;

      if( search_slave_id == my_node_id )
	again = 1; // skip my own ID
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

  u8 got_msg;
  s8 locked = -1; // contains MS node ID if slave has been locked, otherwise -1 if not locked
#if DEBUG_VERBOSE_LEVEL >= 2
  tos12_ctr = 0; // only for debugging
#endif
  do {
    got_msg = 0;

    // check for incoming FIFO messages (MBNet requests)
    MIOS32_IRQ_Disable();
    mbnet_fifo_t fifo;
    if( mbnet_fifo_req_size ) {
      got_msg = 1;
      fifo = mbnet_fifo_req[mbnet_fifo_req_tail];
      if( ++mbnet_fifo_req_tail >= MBNET_RX_FIFO_SIZE )
	mbnet_fifo_req_tail = 0;
      --mbnet_fifo_req_size;
    }
    MIOS32_IRQ_Enable();

    if( got_msg ) {
      // master node ID
      u8 master_id = fifo.id.ms << 4;

#if DEBUG_VERBOSE_LEVEL >= 2
      if( fifo.id.tos == 1 || fifo.id.tos == 2 ) {
	++tos12_ctr;
	if( tos12_ctr > 8 ) {
	  DEBUG_MSG("[MBNET] ... skip display of remaining read/write operations\n");
	}
      } else {
	tos12_ctr = 0;
      }
#endif
#if DEBUG_VERBOSE_LEVEL >= 3
      if( tos12_ctr <= 8 ) {
	DEBUG_MSG("[MBNET] request ID=%02x TOS=%d CTRL=%04x DLC=%d MSG=%02x %02x %02x %02x %02x %02x %02x %02x\n",
		  master_id,
		  fifo.id.tos,
		  fifo.id.control,
		  fifo.dlc,
		  fifo.msg.bytes[0], fifo.msg.bytes[1], fifo.msg.bytes[2], fifo.msg.bytes[3],
		  fifo.msg.bytes[4], fifo.msg.bytes[5], fifo.msg.bytes[6], fifo.msg.bytes[7]);
      }
#endif
      // default acknowledge message is empty
      mbnet_msg_t ack_msg;
      ack_msg.data_l = ack_msg.data_h = 0;

      // if slave has been locked by another master: send retry acknowledge
      if( locked != -1 && fifo.id.ms != locked ) {
	MBNET_SendAck(master_id, MBNET_ACK_RETRY, ack_msg, 0); // master_id, tos, msg, dlc
#if DEBUG_VERBOSE_LEVEL >= 3
	DEBUG_MSG("[MBNET] ignore - node is locked by MS=%d\n", locked);
#endif
      } else {
	// branch depending on TOS
	switch( fifo.id.tos ) {
          case MBNET_REQ_SPECIAL:
	    // branch depending on ETOS (extended TOS)
	    switch( fifo.id.control & 0xff ) {
	      case 0x00: // lock receiver
		locked = fifo.id.ms;
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
      	      case 0x06: // call APP_MIDI_NotifyPackage
		if( fifo.dlc != 3 ) {
		  MBNET_SendAck(master_id, MBNET_ACK_ERROR, ack_msg, 0); // master_id, tos, msg, dlc
		} else {
		  mios32_midi_package_t package;
		  APP_MIDI_NotifyPackage(0, package); // TODO: we could forward the port number as well
		  MBNET_SendAck(master_id, MBNET_ACK_OK, ack_msg, 0); // master_id, tos, msg, dlc
		}
		break;
	      case 0x07: // call APP_NotifyReceivedSysEx
		// not for MIOS32
		MBNET_SendAck(master_id, MBNET_ACK_ERROR, ack_msg, 0); // master_id, tos, msg, dlc
		break;
      	      case 0x08: // call APP_DIN_NotifyToggle
		if( fifo.dlc != 2 ) {
		  MBNET_SendAck(master_id, MBNET_ACK_ERROR, ack_msg, 0); // master_id, tos, msg, dlc
		} else {
		  APP_DIN_NotifyToggle(fifo.msg.bytes[0], fifo.msg.bytes[1]);
		  MBNET_SendAck(master_id, MBNET_ACK_OK, ack_msg, 0); // master_id, tos, msg, dlc
		}
		break;
      	      case 0x09: // call APP_ENC_NotifyChange
		if( fifo.dlc != 2 ) {
		  MBNET_SendAck(master_id, MBNET_ACK_ERROR, ack_msg, 0); // master_id, tos, msg, dlc
		} else {
		  APP_ENC_NotifyChange(fifo.msg.bytes[0], fifo.msg.bytes[1]);
		  MBNET_SendAck(master_id, MBNET_ACK_OK, ack_msg, 0); // master_id, tos, msg, dlc
		}
		break;
      	      case 0x0a: // call APP_AIN_NotifyChange
		if( fifo.dlc != 3 ) {
		  MBNET_SendAck(master_id, MBNET_ACK_ERROR, ack_msg, 0); // master_id, tos, msg, dlc
		} else {
		  APP_ENC_NotifyChange(fifo.msg.bytes[0], fifo.msg.bytes[1] | (fifo.msg.bytes[2] << 8));
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
		callback(fifo.id.ms << 4, fifo.id.tos, fifo.id.control, fifo.msg, fifo.dlc);
		break;
	    }
	    break;
  
	  case MBNET_REQ_RAM_READ:
          case MBNET_REQ_RAM_WRITE:
          case MBNET_REQ_PING:
	    // handled by application
	    callback(fifo.id.ms << 4, fifo.id.tos, fifo.id.control, fifo.msg, fifo.dlc);
	    break;

	  default: // fifo.id.tos>=4 will never be received, just for the case...
	    MBNET_SendAck(master_id, MBNET_ACK_ERROR, ack_msg, 0); // master_id, tos, msg, dlc
	    break;
        }  
      }
    }
  } while( got_msg || locked != -1);

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Local function:
// Used during transmit or receive polling to determine if a bus error has occured
// (e.g. receiver passive or no nodes connected to bus)
// In this case, all pending transmissions will be aborted
// The mbnet_state.PANIC flag is set to report to the application, that the
// bus is not ready for transactions (flag accessible via MBNET_ErrorStateGet).
// This flag will be cleared by WaitAck once we got back a message from any slave
// OUT: returns -1 if bus permanent off (requires re-initialisation)
//      returns -2 if panic state reached
/////////////////////////////////////////////////////////////////////////////
static s32 MBNET_BusErrorCheck(void)
{
  // check if permanent off state is reached
  if( mbnet_state.PERMANENT_OFF )
    return -1; // requires re-initialisation

  // exit if CAN not in error warning state
  if( !(MBNET_CAN->GSR & (1 << 6)) ) // ES
    return 0; // no error

  // abort all pending transmissions
  //MBNET_CAN->CMR = CAN_CMR_AT | CAN_CMR_RRB | CAN_CMR_CDO;
  MBNET_CAN->CMR = (1<<1) | (1<<2) | (1<<3);

  // notify that an abort has happened
  mbnet_state.PANIC = 1;

  // TODO: try to recover

#if DEBUG_VERBOSE_LEVEL >= 1
  DEBUG_MSG("[MBNET] ERRORs detected (GSR=0x%08x) - permanent off state reached!\n", MBNET_CAN->GSR);
#endif

  // recovery not possible: prevent MBNet accesses
  mbnet_state.PERMANENT_OFF = 1;

  return -1; // bus permanent off
}


/////////////////////////////////////////////////////////////////////////////
// Interrupt handler for CAN receive messages
/////////////////////////////////////////////////////////////////////////////
void CAN_IRQHandler(void)
{
  u32 icr = MBNET_CAN->ICR; // read and clear interrupt flags

  if( icr & (1 << 0 ) ) { // RI (receive interrupt)
    // get ID and branch depending on FIFO
    mbnet_fifo_t fifo;
    // get message and store in fifo buffer
    fifo.id.ALL = MBNET_CAN->RID & 0x1fffffff;
    fifo.dlc = (MBNET_CAN->RFS >> 16) & 0xf;
    fifo.msg.data_l = MBNET_CAN->RDA;
    fifo.msg.data_h = MBNET_CAN->RDB;

    MIOS32_IRQ_Disable();
    if( fifo.id.ack ) {
      if( mbnet_fifo_ack_size >= MBNET_RX_FIFO_SIZE ) {
	// overflow message?
      } else {
	++mbnet_fifo_ack_size;
	mbnet_fifo_ack[mbnet_fifo_ack_head] = fifo;
	if( ++mbnet_fifo_ack_head >= MBNET_RX_FIFO_SIZE )
	  mbnet_fifo_ack_head = 0;
      }
    } else {
      if( mbnet_fifo_req_size >= MBNET_RX_FIFO_SIZE ) {
	// overflow message?
      } else {
	++mbnet_fifo_req_size;
	mbnet_fifo_req[mbnet_fifo_req_head] = fifo;
	if( ++mbnet_fifo_req_head >= MBNET_RX_FIFO_SIZE )
	  mbnet_fifo_req_head = 0;
      }
    }
    MIOS32_IRQ_Enable();

#if 0
    // DANGEROUS !!! CAN HANG-UP THE SYSTEM DUE TO LONG MESSAGE
    DEBUG_MSG("[MBNET:IRQ] %s NODE=%02x MS=%d TOS=%d CTRL=%04x DLC=%d MSG=%02x %02x %02x %02x %02x %02x %02x %02x\n",
	      fifo.id.ack ? "ACK" : "REQ",
	      fifo.id.node,
	      fifo.id.ms,
	      fifo.id.tos,
	      fifo.id.control,
	      fifo.dlc,
	      fifo.msg.bytes[0], fifo.msg.bytes[1], fifo.msg.bytes[2], fifo.msg.bytes[3],
	      fifo.msg.bytes[4], fifo.msg.bytes[5], fifo.msg.bytes[6], fifo.msg.bytes[7]);
#endif

    // release buffer
    MBNET_CAN->CMR = (1 << 2);
  }

}

