// $Id$
/*
 * MBNet Hardware Abstraction Layer for LPC17xx derivatives
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

#include "mbnet_hal.h"


/////////////////////////////////////////////////////////////////////////////
// Peripheral/Pin definitions
/////////////////////////////////////////////////////////////////////////////

#if 0
// pointer to CAN peripheral
#define MBNET_CAN      LPC_CAN2
// index of CAN counted from 0 (0..1)
#define MBNET_CAN_IX   1

// CAN clocked at CCLK (typically 100 MHz)
#define CAN_PERIPHERAL_FRQ (MIOS32_SYS_CPU_FREQUENCY)

// RxD: P0.4 (RD2)
#define MBNET_RXD_INIT    { MIOS32_SYS_LPC_PINSEL(0, 4, 2); }
// TxD: P0.5 (TD2)
#define MBNET_TXD_INIT    { MIOS32_SYS_LPC_PINSEL(0, 5, 2); }

#else
// pointer to CAN peripheral
#define MBNET_CAN      LPC_CAN1
// index of CAN counted from 0 (0..1)
#define MBNET_CAN_IX   0

// CAN clocked at CCLK (typically 100 MHz)
#define CAN_PERIPHERAL_FRQ (MIOS32_SYS_CPU_FREQUENCY)

// RxD: P0.0 (RD1)
#define MBNET_RXD_INIT    { MIOS32_SYS_LPC_PINSEL(0, 0, 1); }
// TxD: P0.1 (TD1)
#define MBNET_TXD_INIT    { MIOS32_SYS_LPC_PINSEL(0, 1, 1); }

#endif


// size of the two request/acknowledge receive FIFOs
#define MBNET_RX_FIFO_SIZE MBNET_SLAVE_NODES_MAX


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

// receive fifos
static mbnet_packet_t mbnet_fifo_req[MBNET_RX_FIFO_SIZE];
static volatile u8 mbnet_fifo_req_tail;
static volatile u8 mbnet_fifo_req_head;
static volatile u8 mbnet_fifo_req_size;

static mbnet_packet_t mbnet_fifo_ack[MBNET_RX_FIFO_SIZE];
static volatile u8 mbnet_fifo_ack_tail;
static volatile u8 mbnet_fifo_ack_head;
static volatile u8 mbnet_fifo_ack_size;

// optional TX handler callback
s32 (*tx_handler_callback)(mbnet_id_t *mbnet_id, mbnet_msg_t *msg, u8 *dlc);


/////////////////////////////////////////////////////////////////////////////
// Initializes CAN interface
// IN: <mode>: currently only mode 0 supported
// OUT: returns < 0 if initialisation failed
/////////////////////////////////////////////////////////////////////////////
s32 MBNET_HAL_Init(u32 mode)
{
  // currently only mode 0 supported
  if( mode != 0 )
    return -1; // unsupported mode

  // clear FIFOs
  mbnet_fifo_req_tail = mbnet_fifo_req_head = mbnet_fifo_req_size = 0;
  mbnet_fifo_ack_tail = mbnet_fifo_ack_head = mbnet_fifo_ack_size = 0;

  // disable TX handler callback
  tx_handler_callback = NULL;

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
#elif CAN_PERIPHERAL_FRQ == 120000000
  // -> 100 Mhz / 6 -> 20 MHz --> 10 quanta for 2 MBaud
  //                 TESG2         TESG1         SJW         Prescaler
  //                 -> 3          -> 6         -> 1        1:6
  MBNET_CAN->BTR  = (0x2 << 20) | (0x5 << 16) | (0 << 14) | (5 << 0);
#else
# error "BTR baudrate not prepared for this peripheral frequency"
#endif

  // Return to normal operation
  MBNET_CAN->MOD = 0;

  // enable CAN interrupt
  MIOS32_IRQ_Install(CAN_IRQn, MIOS32_IRQ_PRIO_HIGH);

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Configures the CAN acceptance filter depending on my node_id
// OUT: returns < 0 if configuration failed
/////////////////////////////////////////////////////////////////////////////
s32 MBNET_HAL_FilterInit(u8 node_id)
{
  // my node_id must be 0..127
  if( node_id >= 128 )
    return -1;

  // CAN filter initialisation
  LPC_CANAF->AFMR = 0x02; // filter bypass to allow configuration

  // unfortunately the "FullCAN" function can't be used here, because it doesn't support extended IDs.
  // means: we can't organize two FIFOs for incoming request and acknowledge messages
  // therefore we just set a mask for this node, and handle FIFO buffering via software

  // range 1: requests (ACK=0)
  LPC_CANAF_RAM->mask[0] = (MBNET_CAN_IX << 29) | (0 << 28) | (node_id << 21) | 0x000000;
  LPC_CANAF_RAM->mask[1] = (MBNET_CAN_IX << 29) | (0 << 28) | (node_id << 21) | 0x1fffff;

  // range 2: acknowledges (ACK=1)
  LPC_CANAF_RAM->mask[2] = (MBNET_CAN_IX << 29) | (1 << 28) | (node_id << 21) | 0x000000;
  LPC_CANAF_RAM->mask[3] = (MBNET_CAN_IX << 29) | (1 << 28) | (node_id << 21) | 0x1fffff;

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
// Send a MBNet packet
// IN: the packet which should be sent
// returns 1 if message sent successfully
// returns -3 on transmission error
/////////////////////////////////////////////////////////////////////////////
s32 MBNET_HAL_Send(mbnet_id_t mbnet_id, mbnet_msg_t msg, u8 dlc)
{
  // select an empty transmit mailbox
  s8 mailbox = -1;
  do {
    // TODO: timeout required?
    u32 sr = MBNET_CAN->SR;

    if     ( sr & (1 << 2) ) // TBS1
      mailbox = 0;
    else if( sr & (1 << 10) ) // TBS2
      mailbox = 1;
#if 0
    // assigned for interrupt driven transfers
    else if( sr & (1 << 18) ) // TBS3
      mailbox = 2;
#endif

    // no free buffer: exit if CAN bus errors (CAN doesn't send messages anymore)
    if( mailbox < 0 && MBNET_HAL_BusErrorCheck() < 0 )
      return -3; // transmission error
  } while( mailbox == -1 ); // TODO: provide timeout mechanism

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

  return 1; // no error, message sent
}


/////////////////////////////////////////////////////////////////////////////
// Receives a MBNet acknowledge packet
// IN: pointer to packet which should get the received content
// OUT: returns 0 if no new message in FIFO
//      returns 1 if message received successfully
//      returns -3 on transmission error
/////////////////////////////////////////////////////////////////////////////
s32 MBNET_HAL_ReceiveAck(mbnet_packet_t *p)
{
  u8 got_msg = 0;

  MIOS32_IRQ_Disable();
  if( mbnet_fifo_ack_size ) {
    got_msg = 1;
    *p = mbnet_fifo_ack[mbnet_fifo_ack_tail];
    if( ++mbnet_fifo_ack_tail >= MBNET_RX_FIFO_SIZE )
      mbnet_fifo_ack_tail = 0;
    --mbnet_fifo_ack_size;
  }
  MIOS32_IRQ_Enable();

  return got_msg;
}


/////////////////////////////////////////////////////////////////////////////
// Receives a MBNet request packet
// IN: pointer to packet which should get the received content
// OUT: returns 0 if no new message in FIFO
//      returns 1 if message received successfully
//      returns -3 on transmission error
/////////////////////////////////////////////////////////////////////////////
s32 MBNET_HAL_ReceiveReq(mbnet_packet_t *p)
{
  u8 got_msg = 0;

  MIOS32_IRQ_Disable();
  if( mbnet_fifo_req_size ) {
    got_msg = 1;
    *p = mbnet_fifo_req[mbnet_fifo_req_tail];
    if( ++mbnet_fifo_req_tail >= MBNET_RX_FIFO_SIZE )
      mbnet_fifo_req_tail = 0;
    --mbnet_fifo_req_size;
  }
  MIOS32_IRQ_Enable();

  return got_msg;
}


/////////////////////////////////////////////////////////////////////////////
// Used during transmit or receive polling to determine if a bus error has occured
// (e.g. receiver passive or no nodes connected to bus)
// In this case, all pending transmissions will be aborted
// The mbnet_state.PANIC flag is set to report to the application, that the
// bus is not ready for transactions (flag accessible via MBNET_ErrorStateGet).
// This flag will be cleared by WaitAck once we got back a message from any slave
// OUT: returns -1 if bus permanent off (requires re-initialisation)
//      returns -2 if panic state reached
/////////////////////////////////////////////////////////////////////////////
s32 MBNET_HAL_BusErrorCheck(void)
{
  // exit if CAN not in error warning state
  if( !(MBNET_CAN->GSR & (1 << 6)) ) // ES
    return 0; // no error

  // abort all pending transmissions
  //MBNET_CAN->CMR = CAN_CMR_AT | CAN_CMR_RRB | CAN_CMR_CDO;
  MBNET_CAN->CMR = (1<<1) | (1<<2) | (1<<3);

  // TODO: try to recover

  return -2; // PANIC state reached
}


/////////////////////////////////////////////////////////////////////////////
// Interrupt handler for CAN receive messages
/////////////////////////////////////////////////////////////////////////////
void CAN_IRQHandler(void)
{
  u32 icr = MBNET_CAN->ICR; // read and clear interrupt flags

  if( icr & (1 << 0 ) ) { // RI (receive interrupt)
    // get ID and branch depending on FIFO
    mbnet_packet_t fifo;
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

    // release buffer
    MBNET_CAN->CMR = (1 << 2);
  }

  if( icr & (1 << 10) ) { // TIE3
    // trigger a new packet
    MBNET_HAL_TriggerTxHandler();
  }
}


/////////////////////////////////////////////////////////////////////////////
// Installs an optional Tx Handler which is called via interrupt whenever
// a new message can be sent
// Currently only supported by LPC17xx HAL!
// tx_handler_callback == NULL will disable the handler
/////////////////////////////////////////////////////////////////////////////
s32 MBNET_HAL_InstallTxHandler(s32 (*_tx_handler_callback)(mbnet_id_t *mbnet_id, mbnet_msg_t *msg, u8 *dlc))
{
  tx_handler_callback = _tx_handler_callback;

  if( tx_handler_callback == NULL ) {
    // disable transmit interrupt for buffer 3
    MBNET_CAN->IER &= ~(1 << 10); // TIE3
  } else {
    // enable transmit interrupt for buffer 3
    MBNET_CAN->IER |= (1 << 10); // TIE3
    // request to send first frame
    MBNET_HAL_TriggerTxHandler();
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Manual request of transmit handler
// Has to be called whenever the transmission has been stopped to restart
// interrupt driven transfers
/////////////////////////////////////////////////////////////////////////////
s32 MBNET_HAL_TriggerTxHandler(void)
{
  if( tx_handler_callback == NULL )
    return -1; // no callback installed

  s32 status;
  mbnet_id_t mbnet_id;
  mbnet_msg_t msg;
  u8 dlc;

  if( (status=tx_handler_callback(&mbnet_id, &msg, &dlc)) > 0 ) {
    //              FF (extended frame)  DLC (length)   
    MBNET_CAN->TFI3 = (1 << 31)         | (dlc << 16);
    // the extended ID
    MBNET_CAN->TID3 = mbnet_id.ALL;
    // the playload
    MBNET_CAN->TDA3 = msg.data_l;
    MBNET_CAN->TDB3 = msg.data_h;
    
    // transmission request
    MBNET_CAN->CMR = (1 << (5+2)) | (1 << 0);
  }

  return status;
}
