// $Id: mbnet_hal.c 1182 2011-04-21 21:18:08Z tk $
/*
 * MBNet Hardware Abstraction Layer for STM32F10x derivatives
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

// note: remapped CAN IO - for different pins, 
// the MBNET_Init() function has to be enhanced
#define MBNET_RX_PORT     GPIOD
#define MBNET_RX_PIN      GPIO_Pin_0
#define MBNET_TX_PORT     GPIOD
#define MBNET_TX_PIN      GPIO_Pin_1
#define MBNET_REMAP_FUNC  { GPIO_PinAFConfig(GPIOD, GPIO_PinSource0, GPIO_AF_CAN1); GPIO_PinAFConfig(GPIOD, GPIO_PinSource1, GPIO_AF_CAN1); }

/////////////////////////////////////////////////////////////////////////////
// Initializes CAN interface
// IN: <mode>: currently only mode 0 supported
// OUT: returns < 0 if initialisation failed
/////////////////////////////////////////////////////////////////////////////
s32 MBNET_HAL_Init(u32 mode)
{
  int poll_ctr;

  // currently only mode 0 supported
  if( mode != 0 )
    return -1; // unsupported mode

  // map CAN pins
  MBNET_REMAP_FUNC;

  // configure CAN pins
  GPIO_InitTypeDef GPIO_InitStructure;
  GPIO_StructInit(&GPIO_InitStructure);
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_25MHz;

  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_UP;
  GPIO_InitStructure.GPIO_Pin = MBNET_RX_PIN;
  GPIO_Init(MBNET_RX_PORT, &GPIO_InitStructure);
  GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;
  GPIO_InitStructure.GPIO_Pin = MBNET_TX_PIN;
  GPIO_Init(MBNET_TX_PORT, &GPIO_InitStructure);

  // enable CAN clock
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_CAN1, ENABLE);

#if 0
  // reset CAN
  RCC_APB1PeriphResetCmd(RCC_APB1Periph_CAN, ENABLE);
  RCC_APB1PeriphResetCmd(RCC_APB1Periph_CAN, DISABLE);
#endif

  // CAN initialisation

  // Request initialisation and wait until acknowledged
  CAN1->MCR = (1 << 0); // CAN_MCR_INRQ
  poll_ctr = 1000; // poll up to 1000 times
  // according to datasheet, CAN waits until all ongoing transfers are finished
  while( !(CAN1->MSR & (1 << 0)) ) { // CAN_MSR_INAK
    if( --poll_ctr == 0 )
      return -1; // initialisation failed
  }

  // enable receive FIFO locked mode (if FIFO is full, next incoming message will be discarded)
  CAN1->MCR |= (1 << 3); // CAN_MCR_RFLM

  // bit timings for 2 MBaud:
  // -> 84 Mhz / 2 / 3 -> 14 MHz --> 7 quanta for 2 MBaud
  //          normal mode               Resynch. Jump Width   Time Segment 1         Time Segment 2       Prescaler
  CAN1->BTR = (CAN_Mode_Normal << 30) | (CAN_SJW_1tq << 24) | (CAN_BS1_4tq << 16) | (CAN_BS2_2tq << 20) | (3 - 1);

  // leave initialisation mode and wait for acknowledge
  // Datasheet: we expect that the bus is unbusy after CAN has monitored a sequence of 11 consecutive recessive bits
  CAN1->MCR &= ~(1 << 0); // CAN_MCR_INRQ
  poll_ctr = 1000; // poll up to 1000 times
  while( !(CAN1->MSR & (1 << 0)) ) { // CAN_MSR_INAK
    if( --poll_ctr == 0 ) {
      return -1; // initialisation failed
    }
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Configures the CAN acceptance filter depending on my node_id
// OUT: returns < 0 if configuration failed
/////////////////////////////////////////////////////////////////////////////
s32 MBNET_HAL_FilterInit(u8 node_id)
{
  CAN_FilterInitTypeDef CAN_FilterInitStructure;

  // my node_id must be 0..127
  if( node_id >= 128 )
    return -1;

  // CAN filter initialisation

  // filter/FIFO 0 checks for incoming request messages directed to this node
  CAN_FilterInitStructure.CAN_FilterNumber=0;
  CAN_FilterInitStructure.CAN_FilterMode=CAN_FilterMode_IdMask;
  CAN_FilterInitStructure.CAN_FilterScale=CAN_FilterScale_32bit;
  CAN_FilterInitStructure.CAN_FilterIdHigh=(node_id << 8); // ACK=0
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
  CAN_FilterInitStructure.CAN_FilterIdHigh=(1 << 15) | (node_id << 8); // ACK=1
  CAN_FilterInitStructure.CAN_FilterIdLow=0x0004; // only accept EID=1, RTR=0
  CAN_FilterInitStructure.CAN_FilterMaskIdHigh=0xff00;
  CAN_FilterInitStructure.CAN_FilterMaskIdLow=0x0006; // check for EID and RTR
  CAN_FilterInitStructure.CAN_FilterFIFOAssignment=1;
  CAN_FilterInitStructure.CAN_FilterActivation=ENABLE;
  CAN_FilterInit(&CAN_FilterInitStructure);

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
    // exit immediately if CAN bus errors (CAN doesn't send messages anymore)
    if( MBNET_HAL_BusErrorCheck() < 0 )
      return -3; // transmission error

    // TODO: timeout required?
    if     ( CAN1->TSR & (1 << 26) ) // TME0
      mailbox = 0;
    else if( CAN1->TSR & (1 << 27) ) // TME1
      mailbox = 1;
    else if( CAN1->TSR & (1 << 28) ) // TME2
      mailbox = 2;
  } while( mailbox == -1 ); // TODO: provide timeout mechanism

  CAN1->sTxMailBox[mailbox].TDTR = (dlc << 0); // set dlc, disable timestamp
  CAN1->sTxMailBox[mailbox].TDLR = msg.data_l;
  CAN1->sTxMailBox[mailbox].TDHR = msg.data_h;
  CAN1->sTxMailBox[mailbox].TIR = (mbnet_id.ALL << 3) | (1 << 2) | (1 << 0); // id field, EID flag, TX Req flag

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
  if( CAN1->RF1R & 0x3 ) { // FMP1 contains number of messages
    // get EID, MSG and DLC
    p->id.ALL = CAN1->sFIFOMailBox[1].RIR >> 3;
    p->dlc = CAN1->sFIFOMailBox[1].RDTR & 0xf;
    p->msg.data_l = CAN1->sFIFOMailBox[1].RDLR;
    p->msg.data_h = CAN1->sFIFOMailBox[1].RDHR;

    // release FIFO
    CAN1->RF1R = (1 << 5); // set RFOM1 flag

    return 1; // message received
  }

  return 0; // no message received
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
  if( CAN1->RF0R & 0x3 ) { // FMP1 contains number of messages
    // get EID, MSG and DLC
    p->id.ALL = CAN1->sFIFOMailBox[0].RIR >> 3;
    p->dlc = CAN1->sFIFOMailBox[0].RDTR & 0xf;
    p->msg.data_l = CAN1->sFIFOMailBox[0].RDLR;
    p->msg.data_h = CAN1->sFIFOMailBox[0].RDHR;

    // release FIFO
    CAN1->RF0R = (1 << 5); // set RFOM0 flag

    return 1; // message received
  }

  return 0; // no message received
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
  if( !(CAN1->ESR & (1 << 0)) ) // EWGF
    return 0; // no error

  // abort all pending transmissions
  CAN1->TSR |= (1 << 23) | (1 << 15) | (1 << 7); // set ABRQ[210]

  // TODO: try to recover

  return -2; // PANIC state reached
}


/////////////////////////////////////////////////////////////////////////////
// Installs an optional Tx Handler which is called via interrupt whenever
// a new message can be sent
// Currently only supported by LPC17xx HAL!
// tx_handler_callback == NULL will disable the handler
/////////////////////////////////////////////////////////////////////////////
s32 MBNET_HAL_InstallTxHandler(s32 (*_tx_handler_callback)(mbnet_id_t *mbnet_id, mbnet_msg_t *msg, u8 *dlc))
{
  return -1; // not supported by this HAL
}

/////////////////////////////////////////////////////////////////////////////
// Manual request of transmit handler
// Has to be called whenever the transmission has been stopped to restart
// interrupt driven transfers
/////////////////////////////////////////////////////////////////////////////
s32 MBNET_HAL_TriggerTxHandler(void)
{
  return -1; // not supported by this HAL
}

