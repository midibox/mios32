// $Id: mios32_can.c 2312 2016-02-27 23:04:51Z tk $
//! \defgroup MIOS32_CAN
//!
//! U(S)ART functions for MIOS32
//!
//! Applications shouldn't call these functions directly, instead please use \ref MIOS32_COM or \ref MIOS32_MIDI layer functions
//!
//! \{
/* ==========================================================================
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

// this module can be optionally enabled in a local mios32_config.h file (included from mios32.h)
#if defined(MIOS32_USE_CAN)


/////////////////////////////////////////////////////////////////////////////
// Pin definitions and USART mappings
/////////////////////////////////////////////////////////////////////////////

// how many CANs are supported?
#define NUM_SUPPORTED_CANS MIOS32_CAN_NUM
#if MIOS32_CAN_NUM >1
// Note:If CAN2 is used, J19:SO(PB5) and J4B:SC(PB6))  OR  J8/9:SC(PB13) and J8/9:RC1(PB12) are no more available! :-/
// Defines the start filter bank for the CAN2 interface (Slave) in the range 0 to 27.
#define MIOS32_CAN2_STARTBANK 14
#else
// All filters banks for CAN1
#define MIOS32_CAN2_STARTBANK 27 // no bank for CAN2
#endif
#define MIOS32_CAN1             CAN1
#define MIOS32_CAN1_RX_PORT     GPIOD
#define MIOS32_CAN1_RX_PIN      GPIO_Pin_0
#define MIOS32_CAN1_TX_PORT     GPIOD
#define MIOS32_CAN1_TX_PIN      GPIO_Pin_1
#define MIOS32_CAN1_REMAP_FUNC  { GPIO_PinAFConfig(GPIOD, GPIO_PinSource0, GPIO_AF_CAN1); GPIO_PinAFConfig(GPIOD, GPIO_PinSource1, GPIO_AF_CAN1); }
#define MIOS32_CAN1_RX0_IRQn    CAN1_RX0_IRQn
#define MIOS32_CAN1_RX1_IRQn    CAN1_RX1_IRQn
#define MIOS32_CAN1_TX_IRQn     CAN1_TX_IRQn
#define MIOS32_CAN1_ER_IRQn     CAN1_SCE_IRQn
#define MIOS32_CAN1_RX0_IRQHANDLER_FUNC void CAN1_RX0_IRQHandler(void)
#define MIOS32_CAN1_RX1_IRQHANDLER_FUNC void CAN1_RX1_IRQHandler(void)
#define MIOS32_CAN1_TX_IRQHANDLER_FUNC void CAN1_TX_IRQHandler(void)
#define MIOS32_CAN1_ER_IRQHANDLER_FUNC void CAN1_SCE_IRQHandler(void)


#define MIOS32_CAN2             CAN2
#if MIOS32_CAN2_ALTFUNC == 0    //  0: CAN2.RX->PB5, CAN2.TX->PB6
#define MIOS32_CAN2_RX_PORT     GPIOB
#define MIOS32_CAN2_RX_PIN      GPIO_Pin_5
#define MIOS32_CAN2_TX_PORT     GPIOB
#define MIOS32_CAN2_TX_PIN      GPIO_Pin_6
#define MIOS32_CAN2_REMAP_FUNC  { GPIO_PinAFConfig(GPIOB, GPIO_PinSource5, GPIO_AF_CAN2); GPIO_PinAFConfig(GPIOB, GPIO_PinSource5, GPIO_AF_CAN2); }
#else                           //  1: CAN2.RX->PB12, CAN2.TX->PB13
#define MIOS32_CAN2_RX_PORT     GPIOB
#define MIOS32_CAN2_RX_PIN      GPIO_Pin_12
#define MIOS32_CAN2_TX_PORT     GPIOB
#define MIOS32_CAN2_TX_PIN      GPIO_Pin_13
#define MIOS32_CAN2_REMAP_FUNC  { GPIO_PinAFConfig(GPIOB, GPIO_PinSource12, GPIO_AF_CAN2); GPIO_PinAFConfig(GPIOB, GPIO_PinSource13, GPIO_AF_CAN2); }
#endif
#define MIOS32_CAN2_RX0_IRQn    CAN2_RX0_IRQn
#define MIOS32_CAN2_RX1_IRQn    CAN2_RX1_IRQn
#define MIOS32_CAN2_TX_IRQn     CAN2_TX_IRQn
#define MIOS32_CAN2_ER_IRQn     CAN2_SCE_IRQn
#define MIOS32_CAN2_RX0_IRQHANDLER_FUNC void CAN2_RX0_IRQHandler(void)
#define MIOS32_CAN2_RX1_IRQHANDLER_FUNC void CAN2_RX1_IRQHandler(void)
#define MIOS32_CAN2_TX_IRQHANDLER_FUNC void CAN2_TX_IRQHandler(void)
#define MIOS32_CAN2_ER_IRQHANDLER_FUNC void CAN2_SCE_IRQHandler(void)

//#define MIOS32_CAN_MIDI_FILT_BK_NODE_SYSEX    0     //Filter Number for Node SysEx
//#define MIOS32_CAN_MIDI_FILT_BK_SYSEX         1     //Filter Number for Device SysEx
//
//#define MIOS32_CAN_MIDI_FILT_BK_NODE_SYSCOM   2     //Filter Number for Node SysCom
//#define MIOS32_CAN_MIDI_FILT_BK_BYPASS        2     //Filter Number for Node SysCom
//
//#define MIOS32_CAN_BANK_FILT_BK_RT       3     //Filter Number for Real time messges
//#define MIOS32_CAN_MIDI_FILT_BK_SYSCOM   3     //Filter Number for Real time messges
//
//#define MIOS32_CAN_MIDI_FILT_BK_NODE_SYSEX    0     //Filter Number for Node SysEx
//#define MIOS32_CAN_MIDI_FILT_BK_SYSEX         1     //Filter Number for Device SysEx

/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

#if NUM_SUPPORTED_CANS >= 1
static u8  can_assigned_to_midi;
static can_packet_t rx_buffer[NUM_SUPPORTED_CANS][MIOS32_CAN_RX_BUFFER_SIZE];
static volatile u16 rx_buffer_tail[NUM_SUPPORTED_CANS];
static volatile u16 rx_buffer_head[NUM_SUPPORTED_CANS];
static volatile u16 rx_buffer_size[NUM_SUPPORTED_CANS];

static can_packet_t tx_buffer[NUM_SUPPORTED_CANS][MIOS32_CAN_TX_BUFFER_SIZE];
static volatile u16 tx_buffer_tail[NUM_SUPPORTED_CANS];
static volatile u16 tx_buffer_head[NUM_SUPPORTED_CANS];
static volatile u16 tx_buffer_size[NUM_SUPPORTED_CANS];

static can_stat_report_t can_stat_report[NUM_SUPPORTED_CANS];
#endif

#if defined MIOS32_CAN_VERBOSE
static u8 can_verbose = MIOS32_CAN_VERBOSE;
#else
static u8 can_verbose = 0;
#endif

u32 can_temp;



/////////////////////////////////////////////////////////////////////////////
//! Initializes CAN MIDI layer
//! \param[in] mode currently only mode 0 supported
//! \return < 0 if initialisation failed
//! \note Applications shouldn't call this function directly, instead please use \ref MIOS32_MIDI layer functions
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_CAN_VerboseSet(u8 level)
{
#if MIOS32_CAN_NUM == 0
  return -1; // no CAN enabled
#else
  
  can_verbose = level;
  
  return 0; // no error
#endif
}


/////////////////////////////////////////////////////////////////////////////
//! Initializes CAN MIDI layer
//! \param[in] mode currently only mode 0 supported
//! \return < 0 if initialisation failed
//! \note Applications shouldn't call this function directly, instead please use \ref MIOS32_MIDI layer functions
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_CAN_VerboseGet(void)
{
#if MIOS32_CAN_NUM == 0
  return -1; // no CAN enabled
#else
  return can_verbose; // no error
#endif
}


/////////////////////////////////////////////////////////////////////////////
//! Initializes CAN interfaces
//! \param[in] mode currently only mode 0 supported
//! \return < 0 if initialisation failed
//! \note Applications shouldn't call this function directly, instead please use \ref MIOS32_COM or \ref MIOS32_MIDI layer functions
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_CAN_Init(u32 mode)
{
  // currently only mode 0 supported
  if( mode != 0 )
    return -1; // unsupported mode

#if NUM_SUPPORTED_CANS == 0
  return -1; // no CANs
#else
  
  // map and init CAN pins
#if  MIOS32_CAN1_ASSIGNMENT != 0 // not disabled
  MIOS32_CAN1_REMAP_FUNC;
#endif
#if NUM_SUPPORTED_CANS >= 2 && MIOS32_CAN2_ASSIGNMENT != 0
  MIOS32_CAN2_REMAP_FUNC;
#endif
  
  // enable CANx clocks
#if  MIOS32_CAN1_ASSIGNMENT != 0 // not disabled
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_CAN1, ENABLE);
#endif
#if NUM_SUPPORTED_CANS >= 2 && MIOS32_CAN2_ASSIGNMENT != 0
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_CAN2, ENABLE);
#endif
  
  // initialize CANs and clear buffers
  u8 can;
  for(can=0; can<NUM_SUPPORTED_CANS; ++can) {
    // initialize Tx/Rx Buffers
    rx_buffer_tail[can] = rx_buffer_head[can] = rx_buffer_size[can] = 0;
    tx_buffer_tail[can] = tx_buffer_head[can] = tx_buffer_size[can] = 0;
    // initialize info report structure
    MIOS32_CAN_ReportReset(can);
    // initialize Default port
    MIOS32_CAN_InitPortDefault(can);
    // initialize peripheral
    if(MIOS32_CAN_InitPeriph(can) < 0)return -1;
  }
  
  // configure and enable CAN interrupts
#if MIOS32_CAN1_ASSIGNMENT != 0 // not disabled
  // enable CAN interrupt
  MIOS32_CAN1->IER = 0x0000000;
  /* Enable MIOS32_CAN1 RX0 interrupt IRQ channel */
  MIOS32_IRQ_Install(MIOS32_CAN1_RX0_IRQn, MIOS32_IRQ_CAN_PRIORITY);
  CAN_ITConfig(MIOS32_CAN1, CAN_IT_FMP0, ENABLE);
  /* Enable MIOS32_CAN1 RX1 interrupt IRQ channel */
  MIOS32_IRQ_Install(MIOS32_CAN1_RX1_IRQn, MIOS32_IRQ_CAN_PRIORITY);
  CAN_ITConfig(MIOS32_CAN1, CAN_IT_FMP1, ENABLE);
  /* Enable MIOS32_CAN1 TX interrupt IRQ channel */
  MIOS32_IRQ_Install(MIOS32_CAN1_TX_IRQn, MIOS32_IRQ_CAN_PRIORITY);
  CAN_ITConfig(MIOS32_CAN1, CAN_IT_TME, DISABLE);
#if 1
  /* Enable MIOS32_CAN1 SCE(errors) interrupts IRQ channels */
  MIOS32_IRQ_Install(MIOS32_CAN1_ER_IRQn, MIOS32_IRQ_CAN_PRIORITY);
  CAN_ITConfig(MIOS32_CAN1, CAN_IT_EWG, ENABLE);
  CAN_ITConfig(MIOS32_CAN1, CAN_IT_EPV, ENABLE);
  CAN_ITConfig(MIOS32_CAN1, CAN_IT_BOF, ENABLE);
  CAN_ITConfig(MIOS32_CAN1, CAN_IT_LEC, DISABLE);
  CAN_ITConfig(MIOS32_CAN1, CAN_IT_ERR, ENABLE);
#endif
#endif
  
#if NUM_SUPPORTED_CANS >= 2 && MIOS32_CAN1_ASSIGNMENT != 0
  // enable CAN interrupt
  MIOS32_CAN2->IER = 0x0000000;
  /* Enable MIOS32_CAN2 RX0 interrupt IRQ channel */
  MIOS32_IRQ_Install(MIOS32_CAN2_RX0_IRQn, MIOS32_IRQ_CAN_PRIORITY);
  CAN_ITConfig(MIOS32_CAN2, CAN_IT_FMP0, ENABLE);
  /* Enable MIOS32_CAN2 RX1 interrupt IRQ channel */
  MIOS32_IRQ_Install(MIOS32_CAN2_RX1_IRQn, MIOS32_IRQ_CAN_PRIORITY);
  CAN_ITConfig(MIOS32_CAN2, CAN_IT_FMP1, ENABLE);
  /* Enable MIOS32_CAN2 TX interrupt IRQ channel */
  MIOS32_IRQ_Install(MIOS32_CAN2_TX_IRQn, MIOS32_IRQ_CAN_PRIORITY);
  CAN_ITConfig(MIOS32_CAN2, CAN_IT_TME, ENABLE);
#if 0
  /* Enable MIOS32_CAN2 SCE(errors) interrupts IRQ channels */
  MIOS32_IRQ_Install(MIOS32_CAN2_ER_IRQn, MIOS32_IRQ_PRIO_MED);
  CAN_ITConfig(MIOS32_CAN2, CAN_IT_EWG, ENABLE);
  CAN_ITConfig(MIOS32_CAN2, CAN_IT_EPV, ENABLE);
  CAN_ITConfig(MIOS32_CAN2, CAN_IT_BOF, ENABLE);
  CAN_ITConfig(MIOS32_CAN2, CAN_IT_LEC, ENABLE);
  CAN_ITConfig(MIOS32_CAN2, CAN_IT_ERR, ENABLE);
#endif
#endif
  
  return 0; // no error
#endif
}


/////////////////////////////////////////////////////////////////////////////
//! \return 0 if CAN is not assigned to a MIDI function
//! \return 1 if CAN is assigned to a MIDI function
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_CAN_IsAssignedToMIDI(u8 can)
{
#if NUM_SUPPORTED_CANS == 0
  return 0; // no CAN available
#else
  return (can_assigned_to_midi & (1 << can)) ? 1 : 0;
#endif
}


/////////////////////////////////////////////////////////////////////////////
//! Initializes a given CAN interface based on given baudrate and TX output mode
//! \param[in] CAN number (0..1)
//! \param[in] is_midi MIDI or common CAN interface?
//! \return < 0 if initialisation failed
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_CAN_InitPort(u8 can, u8 is_midi)
{
#if NUM_SUPPORTED_CANS == 0
  return -1; // no CAN available
#else
  GPIO_InitTypeDef GPIO_InitStructure;
  GPIO_StructInit(&GPIO_InitStructure);
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
  
  if( can >= NUM_SUPPORTED_CANS )
    return -1; // unsupported CAN
  
  // MIDI assignment
  if( is_midi ) {
    can_assigned_to_midi |= (1 << can);
  } else {
    can_assigned_to_midi &= ~(1 << can);
  }
  
  switch( can ) {
#if NUM_SUPPORTED_CANS >= 1 && MIOS32_CAN1_ASSIGNMENT != 0
    case 0: {
      // configure CAN pins
      GPIO_InitTypeDef GPIO_InitStructure;
      GPIO_StructInit(&GPIO_InitStructure);
      GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
      
      GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
      GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
      GPIO_InitStructure.GPIO_Pin = MIOS32_CAN1_RX_PIN;
      GPIO_Init(MIOS32_CAN1_RX_PORT, &GPIO_InitStructure);
      
      GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF;
      GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
      GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_NOPULL;
      GPIO_InitStructure.GPIO_Pin = MIOS32_CAN1_TX_PIN;
      GPIO_Init(MIOS32_CAN1_TX_PORT, &GPIO_InitStructure);
    } break;
#endif
      
#if NUM_SUPPORTED_CANS >= 2 && MIOS32_CAN2_ASSIGNMENT != 0
    case 1: {
      // configure CAN pins
      GPIO_InitTypeDef GPIO_InitStructure;
      GPIO_StructInit(&GPIO_InitStructure);
      GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
      
      GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
      GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
      GPIO_InitStructure.GPIO_Pin = MIOS32_CAN2_RX_PIN;
      GPIO_Init(MIOS32_CAN2_RX_PORT, &GPIO_InitStructure);
      
      GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF;
      GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
      GPIO_InitStructure.GPIO_Pin = MIOS32_CAN2_TX_PIN;
      GPIO_Init(MIOS32_CAN2_TX_PORT, &GPIO_InitStructure);
    } break;
#endif
      
    default:
      return -1; // unsupported CAN
  }
  
  return 0; // no error
#endif
}


/////////////////////////////////////////////////////////////////////////////
//! Initializes a given CAN interface based on default settings
//! \param[in] CAN number (0..1)
//! \return < 0 if initialisation failed
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_CAN_InitPortDefault(u8 can)
{
#if NUM_SUPPORTED_CANS == 0
  return -1; // no CAN available
#else
  switch( can ) {
#if NUM_SUPPORTED_CANS >= 1 && MIOS32_CAN1_ASSIGNMENT != 0
    case 0: {
      MIOS32_CAN_InitPort(0, MIOS32_CAN1_ASSIGNMENT == 1);
    } break;
#endif
      
#if NUM_SUPPORTED_CANS >= 2 && MIOS32_CAN2_ASSIGNMENT != 0
    case 1: {
      MIOS32_CAN_InitPort(1, MIOS32_CAN2_ASSIGNMENT == 1);
    } break;
#endif
      
    default:
      return -1; // unsupported CAN
  }
  
  return 0; // no error
#endif
}


/////////////////////////////////////////////////////////////////////////////
//! sets the baudrate of a CAN port
//! \param[in] CAN number (0..1)
//! \return -1: can not available
//! \return -2: function not prepared for this CAN
//! \return -3: CAN Initialisation failed
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_CAN_InitPeriph(u8 can)
{
#if NUM_SUPPORTED_CANS == 0
  return -1; // no CAN available
#else
  if( can >= NUM_SUPPORTED_CANS )
    return -1;
  
  // CAN initialisation
  CAN_InitTypeDef CAN_InitStruct;
  CAN_StructInit(&CAN_InitStruct);
  CAN_InitStruct.CAN_TTCM = DISABLE;
  CAN_InitStruct.CAN_NART = DISABLE;
  CAN_InitStruct.CAN_RFLM = ENABLE;
  CAN_InitStruct.CAN_TXFP = ENABLE;
  CAN_InitStruct.CAN_ABOM = ENABLE;
  CAN_InitStruct.CAN_Mode = CAN_Mode_Normal;
  // -> 84 Mhz / 2 / 3 -> 14 MHz --> 7 quanta for 2 MBaud
  CAN_InitStruct.CAN_SJW = CAN_SJW_1tq;
  CAN_InitStruct.CAN_BS1 = CAN_BS1_4tq;
  CAN_InitStruct.CAN_BS2 = CAN_BS2_2tq;
  CAN_InitStruct.CAN_Prescaler = 4;
  
  switch( can ) {
    case 0: if(CAN_Init(MIOS32_CAN1, &CAN_InitStruct) == CAN_InitStatus_Failed)return -3; break;
#if NUM_SUPPORTED_CANS >= 2
    case 1: if(CAN_Init(MIOS32_CAN2, &CAN_InitStruct) == CAN_InitStatus_Failed)return -3; break;
#endif
    default:
      return -2; // not prepared
  }
  
  return 0;
#endif
}

/////////////////////////////////////////////////////////////////////////////
//! Initializes a 32 bits filter
//! \param[in] can filter bank number
//! \param[in] extended id for filter
//! \param[in] extended id for mask
//! \return < 0 if initialisation failed
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_CAN_Init32bitFilter(u8 bank, u8 fifo, can_ext_filter_t filter, u8 enabled)
{
#if MIOS32_CAN_NUM == 0
  return -1; // no CAN enabled
#else
  CAN_FilterInitTypeDef CAN_FilterInitStructure;
  // bank / mode / scale
  CAN_FilterInitStructure.CAN_FilterNumber=bank;
  CAN_FilterInitStructure.CAN_FilterMode=CAN_FilterMode_IdMask;
  CAN_FilterInitStructure.CAN_FilterScale=CAN_FilterScale_32bit;
  // filter / mask
  CAN_FilterInitStructure.CAN_FilterIdHigh=filter.filt.data_h;
  CAN_FilterInitStructure.CAN_FilterIdLow=filter.filt.data_l;
  CAN_FilterInitStructure.CAN_FilterMaskIdHigh=filter.mask.data_h;
  CAN_FilterInitStructure.CAN_FilterMaskIdLow=filter.mask.data_l;
  // fifo / enable
  CAN_FilterInitStructure.CAN_FilterFIFOAssignment=0;
  CAN_FilterInitStructure.CAN_FilterActivation=enabled;
  CAN_FilterInit(&CAN_FilterInitStructure);
  
  return 0; // no error
#endif
}

/////////////////////////////////////////////////////////////////////////////
//! Initializes a 16 bits filter
//! \param[in] can filter bank number
//! \param[in] standard id for filter
//! \param[in] standard id for mask
//! \return < 0 if initialisation failed
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_CAN_Init16bitFilter(u8 bank, u8 fifo, can_std_filter_t filter1, can_std_filter_t filter2, u8 enabled)
{
#if MIOS32_CAN_NUM == 0
  return -1; // no CAN enabled
#else
  CAN_FilterInitTypeDef CAN_FilterInitStructure;
  // bank / mode / scale
  CAN_FilterInitStructure.CAN_FilterNumber=bank;
  CAN_FilterInitStructure.CAN_FilterMode=CAN_FilterMode_IdMask;
  CAN_FilterInitStructure.CAN_FilterScale=CAN_FilterScale_16bit;
  // first filter/ firsf mask
  CAN_FilterInitStructure.CAN_FilterIdLow = filter1.filt.ALL;
  CAN_FilterInitStructure.CAN_FilterMaskIdLow = filter1.mask.ALL;
  // second filter / second mask
  CAN_FilterInitStructure.CAN_FilterIdHigh =  filter2.filt.ALL;
  CAN_FilterInitStructure.CAN_FilterMaskIdHigh = filter2.mask.ALL;
  // fifo / enable
  CAN_FilterInitStructure.CAN_FilterFIFOAssignment=fifo;
  CAN_FilterInitStructure.CAN_FilterActivation=enabled;
  CAN_FilterInit(&CAN_FilterInitStructure);
  
  return 0; // no error
#endif
}

/////////////////////////////////////////////////////////////////////////////
//! sets the baudrate of a CAN port
//! \param[in] CAN number (0..1)
//! \return -1: can not available
//! \return -2: function not prepared for this CAN
//! \return -3: CAN Initialisation failed
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_CAN_InitPacket(can_packet_t *packet)
{
#if NUM_SUPPORTED_CANS == 0
  return -1; // no CAN available
#else
  // reset
  packet->id.ALL = 0;
  packet->ctrl.ALL = 0;
  packet->data.data_l = 0;
  packet->data.data_h = 0;
  return 0;
#endif
}

/////////////////////////////////////////////////////////////////////////////
//! returns number of free bytes in receive buffer
//! \param[in] CAN number (0..1)
//! \return can number of free bytes
//! \return 1: can available
//! \return 0: can not available
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_CAN_RxBufferFree(u8 can)
{
#if NUM_SUPPORTED_CANS == 0
  return 0; // no CAN available
#else
  if( can >= NUM_SUPPORTED_CANS )
    return 0;
  else
    return MIOS32_CAN_RX_BUFFER_SIZE - rx_buffer_size[can];
#endif
}


/////////////////////////////////////////////////////////////////////////////
//! returns number of used bytes in receive buffer
//! \param[in] CAN number (0..1)
//! \return > 0: number of used bytes
//! \return 0 if can not available
//! \note Applications shouldn't call these functions directly, instead please use \ref MIOS32_COM or \ref MIOS32_MIDI layer functions
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_CAN_RxBufferUsed(u8 can)
{
#if NUM_SUPPORTED_CANS == 0
  return 0; // no CAN available
#else
  if( can >= NUM_SUPPORTED_CANS )
    return 0;
  else
    return rx_buffer_size[can];
#endif
}


/////////////////////////////////////////////////////////////////////////////
//! gets a byte from the receive buffer
//! \param[in] CAN number (0..1)
//! \return -1 if CAN not available
//! \return -2 if no new byte available
//! \return >= 0: number of received bytes
//! \note Applications shouldn't call these functions directly, instead please use \ref MIOS32_COM or \ref MIOS32_MIDI layer functions
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_CAN_RxBufferGet(u8 can, can_packet_t *p)
{
#if NUM_SUPPORTED_CANS == 0
  return -1; // no CAN available
#else
  if( can >= NUM_SUPPORTED_CANS )
    return -1; // CAN not available
  
  if( !rx_buffer_size[can] )
    return -2; // nothing new in buffer
  
  // get byte - this operation should be atomic!
  MIOS32_IRQ_Disable();
  *p = rx_buffer[can][rx_buffer_tail[can]];
  if( ++rx_buffer_tail[can] >= MIOS32_CAN_RX_BUFFER_SIZE )
    rx_buffer_tail[can] = 0;
  --rx_buffer_size[can];
  
  //can_stat_report[can].tx_done_ctr++;
  MIOS32_IRQ_Enable();
  
  return 0; // return received byte
#endif
}


/////////////////////////////////////////////////////////////////////////////
//! returns the next byte of the receive buffer without taking it
//! \param[in] CAN number (0..1)
//! \return -1 if CAN not available
//! \return -2 if no new byte available
//! \return >= 0: number of received bytes
//! \note Applications shouldn't call these functions directly, instead please use \ref MIOS32_COM or \ref MIOS32_MIDI layer functions
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_CAN_RxBufferPeek(u8 can, can_packet_t *p)
{
#if NUM_SUPPORTED_CANS == 0
  return -1; // no CAN available
#else
  if( can >= NUM_SUPPORTED_CANS )
    return -1; // CAN not available
  
  if( !rx_buffer_size[can] )
    return -2; // nothing new in buffer
  
  // get byte - this operation should be atomic!
  MIOS32_IRQ_Disable();
  *p = rx_buffer[can][rx_buffer_tail[can]];
  MIOS32_IRQ_Enable();
  
  return 0; // return received byte
#endif
}

/////////////////////////////////////////////////////////////////////////////
//! remove the next byte of the receive buffer without taking it
//! \param[in] CAN number (0..1)
//! \return -1 if CAN not available
//! \return -2 if no new byte available
//! \return >= 0: number of received bytes
//! \note Applications shouldn't call these functions directly, instead please use \ref MIOS32_COM or \ref MIOS32_MIDI layer functions
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_CAN_RxBufferRemove(u8 can)
{
#if NUM_SUPPORTED_CANS == 0
  return -1; // no CAN available
#else
  if( can >= NUM_SUPPORTED_CANS )
    return -1; // CAN not available
  
  if( !rx_buffer_size[can] )
    return -2; // nothing new in buffer
  
  // dec buffer - this operation should be atomic!
  MIOS32_IRQ_Disable();
  if( ++rx_buffer_tail[can] >= MIOS32_CAN_RX_BUFFER_SIZE )
    rx_buffer_tail[can] = 0;
  --rx_buffer_size[can];
  MIOS32_IRQ_Enable();
  
  return 0; // return received byte
#endif
}

/////////////////////////////////////////////////////////////////////////////
//! puts a byte onto the receive buffer
//! \param[in] CAN number (0..1)
//! \param[in] b byte which should be put into Rx buffer
//! \return 0 if no error
//! \return -1 if CAN not available
//! \return -2 if buffer full (retry)
//! \note Applications shouldn't call these functions directly, instead please use \ref MIOS32_COM or \ref MIOS32_MIDI layer functions
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_CAN_RxBufferPut(u8 can, can_packet_t p)
{
#if NUM_SUPPORTED_CANS == 0
  return -1; // no CAN available
#else
  if( can >= NUM_SUPPORTED_CANS )
    return -1; // CAN not available
  
  if( rx_buffer_size[can] >= MIOS32_CAN_RX_BUFFER_SIZE )
    return -2; // buffer full (retry)
  
  // copy received can packet into receive buffer
  // this operation should be atomic!
  MIOS32_IRQ_Disable();
  rx_buffer[can][rx_buffer_head[can]] = p;
  if( ++rx_buffer_head[can] >= MIOS32_CAN_RX_BUFFER_SIZE )
    rx_buffer_head[can] = 0;
  ++rx_buffer_size[can];
  
  
  MIOS32_IRQ_Enable();
  
  return 0; // no error
#endif
}

/////////////////////////////////////////////////////////////////////////////
//! returns number of free bytes in transmit buffer
//! \param[in] CAN number (0..1)
//! \return number of free bytes
//! \return 0 if can not available
//! \note Applications shouldn't call these functions directly, instead please use \ref MIOS32_COM or \ref MIOS32_MIDI layer functions
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_CAN_TxBufferFree(u8 can)
{
#if NUM_SUPPORTED_CANS == 0
  return -1; // no CAN available
#else
  if( can >= NUM_SUPPORTED_CANS )
    return -1;
  else
    return MIOS32_CAN_TX_BUFFER_SIZE - tx_buffer_size[can];
#endif
}


/////////////////////////////////////////////////////////////////////////////
//! returns number of used bytes in transmit buffer
//! \param[in] CAN number (0..1)
//! \return number of used bytes
//! \return 0 if can not available
//! \note Applications shouldn't call these functions directly, instead please use \ref MIOS32_COM or \ref MIOS32_MIDI layer functions
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_CAN_TxBufferUsed(u8 can)
{
#if NUM_SUPPORTED_CANS == 0
  return -1; // no CAN available
#else
  if( can >= NUM_SUPPORTED_CANS )
    return -1;
  else
    return tx_buffer_size[can];
#endif
}


/////////////////////////////////////////////////////////////////////////////
//! gets a byte from the transmit buffer
//! \param[in] CAN number (0..1)
//! \return -1 if CAN not available
//! \return -2 if no new byte available
//! \return >= 0: transmitted byte
//! \note Applications shouldn't call these functions directly, instead please use \ref MIOS32_COM or \ref MIOS32_MIDI layer functions
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_CAN_TxBufferGet(u8 can, can_packet_t *p)
{
#if NUM_SUPPORTED_CANS == 0
  return -1; // no CAN available
#else
  if( can >= NUM_SUPPORTED_CANS )
    return -1; // CAN not available
  
  if( !tx_buffer_size[can] )
    return -2; // nothing new in buffer
  
  // get byte - this operation should be atomic!
  MIOS32_IRQ_Disable();
  *p = tx_buffer[can][tx_buffer_tail[can]];
  if( ++tx_buffer_tail[can] >= MIOS32_CAN_TX_BUFFER_SIZE )
    tx_buffer_tail[can] = 0;
  --tx_buffer_size[can];
  MIOS32_IRQ_Enable();
  
  return 0; // return transmitted byte
#endif
}


/////////////////////////////////////////////////////////////////////////////
//! puts more than one byte onto the transmit buffer (used for atomic sends)
//! \param[in] CAN number (0..1)
//! \param[in] *buffer pointer to buffer to be sent
//! \param[in] len number of bytes to be sent
//! \return 0 if no error
//! \return -1 if CAN not available
//! \return -2 if buffer full or cannot get all requested bytes (retry)
//! \return -3 if CAN not supported by MIOS32_CAN_TxBufferPut Routine
//! \note Applications shouldn't call these functions directly, instead please use \ref MIOS32_COM or \ref MIOS32_MIDI layer functions
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_CAN_TxBufferPutMore_NonBlocking(u8 can, can_packet_t* p,u16 len)
{
#if NUM_SUPPORTED_CANS == 0
  return -1; // no CAN available
#else
  if( can >= NUM_SUPPORTED_CANS )
    return -1; // CAN not available
  
  if( (tx_buffer_size[can] + len) >= MIOS32_CAN_TX_BUFFER_SIZE )
    return -2; // buffer full or cannot get all requested bytes (retry)

  // exit immediately if CAN bus errors (CAN doesn't send messages anymore)
  if( MIOS32_CAN_BusErrorCheck(can) < 0 )
    return -3; // bus error
  
  CAN_TypeDef* CANx = NULL;
  if(!can)CANx = MIOS32_CAN1;else CANx = MIOS32_CAN2;
  u16 i;
  for(i=0; i<len; ++i) {
    // if buffer is empty / interrupt is not enable
    // we first try to fill some empty mailboxes
    if(MIOS32_CAN_TxBufferUsed(can) == 0){
      // is any mailbox empty ?
      s8 mailbox = -1;
      if( CANx->TSR & CAN_TSR_TME0)mailbox = 0;
      else if( CANx->TSR & CAN_TSR_TME1)mailbox = 1;
      else if( CANx->TSR & CAN_TSR_TME2)mailbox = 2;
      
      if(mailbox>=0){
        // yes, we send the packet
        p[i].id.txrq = 1; //TX Req flag
        CANx->sTxMailBox[mailbox].TDTR = p[i].ctrl.ALL;
        CANx->sTxMailBox[mailbox].TDLR = p[i].data.data_l;
        CANx->sTxMailBox[mailbox].TDHR = p[i].data.data_h;
        CANx->sTxMailBox[mailbox].TIR = p[i].id.ALL;
        // for CAN_Report
        can_stat_report[can].tx_packets_ctr +=1;
        // packet directly put in mailbox
        return mailbox; // >= 0 no error
      }
    }
    // there's something in buffer, new packet must be put after
    // or mailbpx < 0, we put packet in the buffer first
    // copy bytes to be transmitted into transmit buffer
    // this operation should be atomic!
    MIOS32_IRQ_Disable();
    tx_buffer[can][tx_buffer_head[can]] = p[i];
    // for CAN_Report
    can_stat_report[can].tx_packets_ctr +=1;
    if( ++tx_buffer_head[can] >= MIOS32_CAN_TX_BUFFER_SIZE )
      tx_buffer_head[can] = 0;
    tx_buffer_size[can]++;

    // and we start TME interrupt
    CANx->IER |= CAN_IT_TME;
    
    MIOS32_IRQ_Enable();
  }


  return 0; // no error
#endif
}

/////////////////////////////////////////////////////////////////////////////
//! puts more than one byte onto the transmit buffer (used for atomic sends)<BR>
//! (blocking function)
//! \param[in] CAN number (0..1)
//! \param[in] *buffer pointer to buffer to be sent
//! \param[in] len number of bytes to be sent
//! \return 0 if no error
//! \return -1 if CAN not available
//! \return -3 if CAN not supported by MIOS32_CAN_TxBufferPut Routine
//! \note Applications shouldn't call these functions directly, instead please use \ref MIOS32_COM or \ref MIOS32_MIDI layer functions
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_CAN_TxBufferPutMore(u8 can, can_packet_t *packets, u16 len)
{
  s32 error;
  
  while( (error=MIOS32_CAN_TxBufferPutMore_NonBlocking(can, packets, len)) == -2 );
  
  return error;
}


/////////////////////////////////////////////////////////////////////////////
//! puts a byte onto the transmit buffer
//! \param[in] CAN number (0..1)
//! \param[in] b byte which should be put into Tx buffer
//! \return 0 if no error
//! \return -1 if CAN not available
//! \return -2 if buffer full (retry)
//! \return -3 if CAN not supported by MIOS32_CAN_TxBufferPut Routine
//! \note Applications shouldn't call these functions directly, instead please use \ref MIOS32_COM or \ref MIOS32_MIDI layer functions
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_CAN_TxBufferPut_NonBlocking(u8 can, can_packet_t p)
{
  // for more comfortable usage...
  // -> just forward to MIOS32_CAN_TxBufferPutMore
  return MIOS32_CAN_TxBufferPutMore(can, &p, 1);
}


/////////////////////////////////////////////////////////////////////////////
//! puts a byte onto the transmit buffer<BR>
//! (blocking function)
//! \param[in] CAN number (0..1)
//! \param[in] b byte which should be put into Tx buffer
//! \return 0 if no error
//! \return -1 if CAN not available
//! \return -3 if CAN not supported by MIOS32_CAN_TxBufferPut Routine
//! \note Applications shouldn't call these functions directly, instead please use \ref MIOS32_COM or \ref MIOS32_MIDI layer functions
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_CAN_TxBufferPut(u8 can, can_packet_t p)
{
  s32 error;
  
  while( (error=MIOS32_CAN_TxBufferPutMore(can, &p, 1)) == -2 );
  
  return error;
}


/////////////////////////////////////////////////////////////////////////////
// Used during transmit or receive polling to determine if a bus error has occured
// (e.g. receiver passive or no nodes connected to bus)
// In this case, all pending transmissions will be aborted
// The midian_state.PANIC flag is set to report to the application, that the
// bus is not ready for transactions (flag accessible via MIDIAN_ErrorStateGet).
// This flag will be cleared by WaitAck once we got back a message from any slave
// OUT: returns -1 if bus permanent off (requires re-initialisation)
//      returns -2 if panic state reached
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_CAN_BusErrorCheck(u8 can)
{
#if NUM_SUPPORTED_CANS == 0
  return -1; // no CAN available
#else

  can_stat_err_t err;
  err = can_stat_report[can].bus_last_err;
  //if((CANx->ESR &7)!=0){
  if((err.tec) > (can_stat_report[can].bus_curr_err.tec)){
	  if(can_verbose)MIOS32_MIDI_SendDebugMessage("[MIOS32_CAN_BusErrorCheck] %s %s %s. rec:%d, tec:%d>", err.ewgf? "warning" : "", err.epvf? "passive" : "", err.boff? "off" : "", err.rec, err.tec);

	  if(err.ewgf){
		  can_stat_report[can].bus_state = WARNING;
	  }

	  if(err.epvf){
		  can_stat_report[can].bus_state = PASSIVE;
	  }
	  if(err.boff){
		  can_stat_report[can].bus_state = BUS_OFF;
	  }
	  can_stat_report[can].bus_curr_err = err;
  }else {
	  CAN_TypeDef* CANx = NULL;
	  if(!can)CANx = MIOS32_CAN1;else CANx = MIOS32_CAN2;
	  err.ALL = CANx->ESR;
	  if((err.tec) < (can_stat_report[can].bus_curr_err.tec)){
		  if(can_verbose)MIOS32_MIDI_SendDebugMessage("[MIOS32_CAN_BusErrorCheck] %s %s %s. rec:%d, tec:%d<", err.ewgf? "warning" : "", err.epvf? "passive" : "", err.boff? "off" : "", err.rec, err.tec);
		  can_stat_report[can].bus_curr_err = err;
		  can_stat_report[can].bus_last_err = err;
	  }
	  if(!(err.ALL & 7))can_stat_report[can].bus_state = BUS_OK;

  }
//	  CAN_TypeDef* CANx = NULL;
//	  if(!can)CANx = MIOS32_CAN1;else CANx = MIOS32_CAN2;
//	  err = CANx->ESR;
//
//	  if((err.ALL & 7) < (can_stat_report[can].bus_curr_err.ALL & 7)){
//
//
//	  if(!err.boff){
//		  //MIOS32_CAN_InitPeriph(can);
//		  can_stat_report[can].bus_state = PASSIVE;
//		  if(can_verbose)MIOS32_MIDI_SendDebugMessage("[MIOS32_CAN_BusErrorCheck] CAN is On! :)");
//	  }
//	  if(!err.epvf){
//		  can_stat_report[can].bus_state = WARNING;
//		  CAN_ITConfig(MIOS32_CAN1, CAN_IT_BOF, ENABLE);
//		  CAN_ITConfig(MIOS32_CAN1, CAN_IT_ERR, ENABLE);
//		  if(can_verbose)MIOS32_MIDI_SendDebugMessage("[MIOS32_CAN_BusErrorCheck] Leaves Passive.");
//	  }
//	  if(!err.ewgf){
//		  can_stat_report[can].bus_state = WARNING;
//		  CAN_ITConfig(MIOS32_CAN1, CAN_IT_EPV, ENABLE);
//		  CAN_ITConfig(MIOS32_CAN1, CAN_IT_BOF, ENABLE);
//		  CAN_ITConfig(MIOS32_CAN1, CAN_IT_ERR, ENABLE);
//		  if(can_verbose)MIOS32_MIDI_SendDebugMessage("[MIOS32_CAN_BusErrorCheck] Leaves Warning.");
//	  }
//
//	  can_stat_report[can].bus_curr_err = err;
//	  CAN_ITConfig(MIOS32_CAN1, CAN_IT_EWG, ENABLE);
//	  CAN_ITConfig(MIOS32_CAN1, CAN_IT_EPV, ENABLE);
//	  CAN_ITConfig(MIOS32_CAN1, CAN_IT_BOF, ENABLE);
//	  CAN_ITConfig(MIOS32_CAN1, CAN_IT_ERR, ENABLE);
//	  can_stat_report[can].bus_state = BUS_OK;
//	  if(can_verbose)MIOS32_MIDI_SendDebugMessage("[MIOS32_CAN_BusErrorCheck] Bus OK.");
//  }
  //if(err.lec != 0)can_stat_report[can].bus_last_err = err;
  return (s32)can_stat_report[can].bus_state ;
#endif
}

/////////////////////////////////////////////////////////////////////////////
//! transmit more than one byte
//! \param[in] CAN number (0..1)
//! \param[in] *buffer pointer to buffer to be sent
//! \param[in] len number of bytes to be sent
//! \return 0 if no error
//! \return -1 if CAN not available
//! \return -2 if buffer full or cannot get all requested bytes (retry)
//! \return -3 if CAN not supported by MIOS32_CAN_TxBufferPut Routine
//! \note Applications shouldn't call these functions directly, instead please use \ref MIOS32_COM or \ref MIOS32_MIDI layer functions
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_CAN_Transmit(u8 can, can_packet_t p, s16 block_time)
{
#if NUM_SUPPORTED_CANS == 0
  return -1; // no CAN available
#else
  if( can >= NUM_SUPPORTED_CANS )
    return -1; // CAN not available

  s8 mailbox = -2;
  CAN_TypeDef* CANx = NULL;
  
  //MIOS32_IRQ_Disable();
  
  if(!can)CANx = MIOS32_CAN1; else CANx = MIOS32_CAN2;
  
  // exit immediately if CAN bus errors (CAN doesn't send messages anymore)
  //if( MIOS32_CAN_BusErrorCheck(can) < 0 )
    //return -3; // bus error
  

  do {
    if(block_time>=1)block_time--;
    
    // select an empty transmit mailbox
    if( CANx->TSR & CAN_TSR_TME0)mailbox = 0;
    else if( CANx->TSR & CAN_TSR_TME1)mailbox = 1;
    else if( CANx->TSR & CAN_TSR_TME2)mailbox = 2;
    
  } while (mailbox == -2 && block_time);
  
  // exit on busy mailboxes and time out
  if(mailbox == -2)return -2;
  
  p.id.txrq = 1;
  CANx->sTxMailBox[mailbox].TDTR = p.ctrl.ALL;
  CANx->sTxMailBox[mailbox].TDLR = p.data.data_l;
  CANx->sTxMailBox[mailbox].TDHR = p.data.data_h;
  CANx->sTxMailBox[mailbox].TIR = p.id.ALL;
  // for CAN_Report
  can_stat_report[can].tx_packets_ctr++;
  
  //DEBUG_MSG("[MIOS32_CAN_Transmit] mailbox:%d",mailbox);
  
  return 0; // no error
#endif
}

/////////////////////////////////////////////////////////////////////////////
// Interrupt handler for first CAN
// always in this order TX, RX0, RX1, don't change it!
/////////////////////////////////////////////////////////////////////////////
#if NUM_SUPPORTED_CANS >= 1
MIOS32_CAN1_TX_IRQHANDLER_FUNC
{
  //if( MIOS32_CAN1->IER & CAN_IT_TME ) { // check if TME enabled
    while(CAN_GetITStatus(MIOS32_CAN1, CAN_IT_TME)){
      if(MIOS32_CAN_TxBufferUsed(0) >= 1){
        u8 mailbox = -1;
        if( MIOS32_CAN1->TSR & CAN_TSR_TME0)mailbox = 0;
        else if( MIOS32_CAN1->TSR & CAN_TSR_TME1)mailbox = 1;
        else if( MIOS32_CAN1->TSR & CAN_TSR_TME2)mailbox = 2;
        can_packet_t p;
        MIOS32_CAN_TxBufferGet(0, &p);
        p.id.txrq = 1; //TX Req flag, this reset RQCPx
        MIOS32_CAN1->sTxMailBox[mailbox].TDTR = p.ctrl.ALL;
        MIOS32_CAN1->sTxMailBox[mailbox].TDLR = p.data.data_l;
        MIOS32_CAN1->sTxMailBox[mailbox].TDHR = p.data.data_h;
        MIOS32_CAN1->sTxMailBox[mailbox].TIR = p.id.ALL;
      }else{
        // nothing to send anymore we reset all RQCPx flags
        MIOS32_CAN1->TSR |= CAN_TSR_RQCP0|CAN_TSR_RQCP1|CAN_TSR_RQCP2;
        // disable TME interrupt
        MIOS32_CAN1->IER &=~CAN_IT_TME;
      }
    }
    // I don't knoow why
    //MIOS32_CAN1->TSR |= (CAN_TSR_RQCP0|CAN_TSR_RQCP1|CAN_TSR_RQCP2);
}
MIOS32_CAN1_RX0_IRQHANDLER_FUNC
{
  
  while(MIOS32_CAN1->RF0R & CAN_RF0R_FMP0){     // FMP0 contains number of messages
    // get EID, MSG and DLC
    can_packet_t p;
    p.id.ALL = MIOS32_CAN1->sFIFOMailBox[0].RIR;
    p.ctrl.ALL = MIOS32_CAN1->sFIFOMailBox[0].RDTR;
    p.data.data_l = MIOS32_CAN1->sFIFOMailBox[0].RDLR;
    p.data.data_h = MIOS32_CAN1->sFIFOMailBox[0].RDHR;
    can_stat_report[0].rx_packets_ctr++;

    s32 status = 0;
//    if((status = MIOS32_CAN_IsAssignedToMIDI(0))){
//      switch(p.id.event){
//        case 0x78 ... 0x7f:  // Realtime
//          // no data, we forward (event +0x80)
//          status = MIOS32_MIDI_SendByteToRxCallback(MCAN0, 0x80 + p.id.event);
//          break;
//        case 0x80 ... 0xe7:  // Channel Voice messages
//          // Source and destination are transmited first. We just foward MIDI event bytes
//          for( i=2; i< p.ctrl.dlc; i++)status = MIOS32_MIDI_SendByteToRxCallback(MCAN0, p.data.bytes[i]);
//          break;
//        case 0xf0:  // SysEx messages
//          // Here we forward all data bytes
//          for( i=0; i< p.ctrl.dlc; i++)status = MIOS32_MIDI_SendByteToRxCallback(MCAN0, p.data.bytes[i]);
//          break;
//        case 0xf1 ... 0xf7:  // System Common messages
//          // depends on DLC
//          if(!p.ctrl.dlc)status = MIOS32_MIDI_SendByteToRxCallback(MCAN0, p.id.event);
//          else for( i=0; i< p.ctrl.dlc; i++)status = MIOS32_MIDI_SendByteToRxCallback(MCAN0, p.data.bytes[i]);
//          break;
//        case 0xfe:  // Node active Sensing ;) 
//          // Here we forward all data bytes
//          for( i=0; i< p.ctrl.dlc; i++)status = MIOS32_MIDI_SendByteToRxCallback(MCAN0, p.data.bytes[i]);
//          break;
//        case 0xf8 ... 0xfd:  // Node System Exclusive and Common messages
//          // toDo: foward to Node System Receive
//          break;
//        case 0xff:  // Node System Exclusive and Common messages
//          // toDo: foward to Node System Receive
//          break;
//        default:
//          break;
//      }
//    }
//    if( status == 0 ){
      if((status = MIOS32_CAN_RxBufferPut(0, p)) < 0 ){
        can_stat_report[0].rx_last_buff_err = status;
        can_stat_report[0].rx_buff_err_ctr = rx_buffer_size[0];
      }
//    }
    // release FIFO
    MIOS32_CAN1->RF0R |= CAN_RF0R_RFOM0; // set RFOM1 flag
  }
}

MIOS32_CAN1_RX1_IRQHANDLER_FUNC
{
  
  while(MIOS32_CAN1->RF1R & CAN_RF1R_FMP1){     // FMP1 contains number of messages
    // get EID, MSG and DLC
    can_packet_t p;
    p.id.ALL = MIOS32_CAN1->sFIFOMailBox[1].RIR;
    p.ctrl.ALL = MIOS32_CAN1->sFIFOMailBox[1].RDTR;
    p.data.data_l = MIOS32_CAN1->sFIFOMailBox[1].RDLR;
    p.data.data_h = MIOS32_CAN1->sFIFOMailBox[1].RDHR;
    can_stat_report[0].rx_packets_ctr++;

    s32 status = 0;
//    if((status = MIOS32_CAN_IsAssignedToMIDI(0))){
//      switch(p.id.event){
//        case 0x78 ... 0x7f:  // Realtime
//          // no data, we forward (event +0x80)
//          status = MIOS32_MIDI_SendByteToRxCallback(MCAN0, 0x80 + p.id.event);
//          break;
//        case 0x80 ... 0xe7:  // Channel Voice messages
//          // Source and destination are transmited first. We just foward MIDI event bytes
//          for( i=2; i< p.ctrl.dlc; i++)status = MIOS32_MIDI_SendByteToRxCallback(MCAN0, p.data.bytes[i]);
//          break;
//        case 0xf0:  // SysEx messages
//          // Here we foward all data bytes
//          for( i=0; i< p.ctrl.dlc; i++)status = MIOS32_MIDI_SendByteToRxCallback(MCAN0, p.data.bytes[i]);
//          break;
//        case 0xf1 ... 0xf7:  // System Common messages
//          if(!p.ctrl.dlc)status = MIOS32_MIDI_SendByteToRxCallback(MCAN0, p.id.event);
//          else for( i=0; i< p.ctrl.dlc; i++)status = MIOS32_MIDI_SendByteToRxCallback(MCAN0, p.data.bytes[i]);
//          break;
//        case 0xf8 ... 0xff:  // Node System Exclusive and Common messages
//          // toDo: foward to Node System Receive
//          break;
//        default:
//          break;
//      }
//    }
//    if( status == 0 ){
      if((status = MIOS32_CAN_RxBufferPut(0, p)) < 0 ){
        can_stat_report[0].rx_last_buff_err = status;
        can_stat_report[0].rx_buff_err_ctr = rx_buffer_size[0];
        can_stat_report[0].rx_packets_err++;
      }
//    }
    // release FIFO
    MIOS32_CAN1->RF1R |= CAN_RF1R_RFOM1; // set RFOM1 flag
  }
}

#if 1 // Here i tried to add a software fifo and using TME interrupt
MIOS32_CAN1_ER_IRQHANDLER_FUNC
{
  
  if( CAN_GetITStatus(MIOS32_CAN1, CAN_IT_ERR) ) { // General Err interrupt is enabled

    if( (can_stat_report[0].bus_last_err.ALL & 7) != (MIOS32_CAN1->ESR & 7) ){
	  //can_stat_err_t err = MIOS32_CAN1->ESR;
      can_stat_report[0].bus_last_err.ALL = MIOS32_CAN1->ESR;
      //if(can_verbose)MIOS32_MIDI_SendDebugMessage("0x%0x", can_stat_report[0].bus_last_err.ALL & 7);
      
      if(can_stat_report[0].bus_last_err.ewgf){
        //can_stat_report[0].bus_state = WARNING;
        CAN_ITConfig(MIOS32_CAN1, CAN_IT_EWG, DISABLE);
        //MIOS32_CAN_BusErrorCheck(0);
      }
      if(can_stat_report[0].bus_last_err.epvf){
        // definively stop the interrupt
        //can_stat_report[0].bus_state = PASSIVE;
        //CAN_ITConfig(MIOS32_CAN1, CAN_IT_ERR, DISABLE);
        CAN_ITConfig(MIOS32_CAN1, CAN_IT_EPV, DISABLE);
        //MIOS32_CAN_BusErrorCheck(0);
      }
      if(can_stat_report[0].bus_last_err.boff){
        // definively stop the interrupt
        //can_stat_report[0].bus_state = BUS_OFF;
        CAN_ITConfig(MIOS32_CAN1, CAN_IT_ERR, DISABLE);
        CAN_ITConfig(MIOS32_CAN1, CAN_IT_BOF, DISABLE);
        //MIOS32_CAN_BusErrorCheck(0);
      }
    }
    CAN_ClearITPendingBit(MIOS32_CAN1, CAN_IT_ERR);
    
  }
}
#endif

#endif


/////////////////////////////////////////////////////////////////////////////
// Interrupt handler for second CAN
/////////////////////////////////////////////////////////////////////////////
#if NUM_SUPPORTED_CANS >= 2
#if 0
MIOS32_CAN2_TX_IRQHANDLER_FUNC
{
  //if( MIOS32_CAN1->IER & CAN_IT_TME ) { // check if TME enabled
  while(CAN_GetITStatus(MIOS32_CAN2, CAN_IT_TME)){
    if(MIOS32_CAN_TxBufferUsed(1) >= 1){
      u8 mailbox = -1;
      if( MIOS32_CAN2->TSR & CAN_TSR_TME0)mailbox = 0;
      else if( MIOS32_CAN2->TSR & CAN_TSR_TME1)mailbox = 1;
      else if( MIOS32_CAN2->TSR & CAN_TSR_TME2)mailbox = 2;
      can_packet_t p;
      MIOS32_CAN_TxBufferGet(1, &p);
      p.id.txrq = 1; //TX Req flag, this reset RQCPx
      MIOS32_CAN2->sTxMailBox[mailbox].TDTR = p.ctrl.ALL;
      MIOS32_CAN2->sTxMailBox[mailbox].TDLR = p.data.data_l;
      MIOS32_CAN2->sTxMailBox[mailbox].TDHR = p.data.data_h;
      MIOS32_CAN2->sTxMailBox[mailbox].TIR = p.id.ALL;
    }else{
      // nothing to send anymore we reset all RQCPx flags
      MIOS32_CAN2->TSR |= CAN_TSR_RQCP0|CAN_TSR_RQCP1|CAN_TSR_RQCP2;
      // disable TME interrupt
      MIOS32_CAN2->IER &=~CAN_IT_TME;
    }
  }
  // I don't knoow why
  MIOS32_CAN2->TSR |= (CAN_TSR_RQCP0|CAN_TSR_RQCP1|CAN_TSR_RQCP2);
}
#endif
MIOS32_CAN2_RX0_IRQHANDLER_FUNC
{
  while(MIOS32_CAN2->RF0R & CAN_RF0R_FMP0){     // FMP0 contains number of messages
    // get EID, MSG and DLC
    can_packet_t p;
    p.id.ALL = MIOS32_CAN2->sFIFOMailBox[0].RIR;
    p.ctrl.ALL = MIOS32_CAN2->sFIFOMailBox[0].RDTR;
    p.data.data_l = MIOS32_CAN2->sFIFOMailBox[0].RDLR;
    p.data.data_h = MIOS32_CAN2->sFIFOMailBox[0].RDHR;
    
    u8 i;
    s32 status = 0;

    if(MIOS32_CAN_RxBufferPut(1, p) < 0 ){
      // here we could add some error handling
    }
    // release FIFO
    MIOS32_CAN2->RF0R |= CAN_RF0R_RFOM0; // set RFOM1 flag
  }
}

MIOS32_CAN2_RX1_IRQHANDLER_FUNC
{
  while(MIOS32_CAN2->RF1R & CAN_RF1R_FMP1 ){     // FMP1 contains number of messages
    // get EID, MSG and DLC
    can_packet_t p;
    p.id.ALL = MIOS32_CAN2->sFIFOMailBox[1].RIR;
    p.ctrl.ALL = MIOS32_CAN2->sFIFOMailBox[1].RDTR;
    p.data.data_l = MIOS32_CAN2->sFIFOMailBox[1].RDLR;
    p.data.data_h = MIOS32_CAN2->sFIFOMailBox[1].RDHR;
    
    u8 i;
    s32 status = 0;
    if((status = MIOS32_CAN_IsAssignedToMIDI(1))){
      switch(p.id.event){
        case 0x78 ... 0x7f:  // Realtime
          // no data, we forward (event +0x80)
          status = MIOS32_MIDI_SendByteToRxCallback(MCAN0+16, 0x80 + p.id.event);
          break;
        case 0x80 ... 0xe7:  // Channel Voice messages
          // Source and destination are transmited first. We just foward MIDI event bytes
          for( i=2; i< p.ctrl.dlc; i++)status = MIOS32_MIDI_SendByteToRxCallback(MCAN0+16, p.data.bytes[i]);
          break;
        case 0xf0:  // SysEx messages
          // Here we foward all data bytes
          for( i=0; i< p.ctrl.dlc; i++)status = MIOS32_MIDI_SendByteToRxCallback(MCAN0+16, p.data.bytes[i]);
          break;
        case 0xf1 ... 0xf7:  // System Common messages
          if(!p.ctrl.dlc)status = MIOS32_MIDI_SendByteToRxCallback(MCAN0+16, p.id.event);
          else for( i=0; i< p.ctrl.dlc; i++)status = MIOS32_MIDI_SendByteToRxCallback(MCAN0+16, p.data.bytes[i]);
          break;
        case 0xf8 ... 0xff:  // Node System Exclusive and Common messages
          // toDo: foward to Node System Receive
          break;
        default:
          break;
      }
    }
    if( status == 0 ){
      if(MIOS32_CAN_RxBufferPut(1, p) < 0 ){
        // here we could add some error handling
      }
    }
    // release FIFO
    MIOS32_CAN2->RF1R |= CAN_RF1R_RFOM1; // set RFOM1 flag
  }
}

#endif


/////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_CAN_Hlp_ErrorVerbose(CAN_TypeDef* CANx)
{
#if NUM_SUPPORTED_CANS == 0
  return -1; // no CAN available
#else
  
  
  return 0; // no error
#endif
}


/////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_CAN_ReportLastErr(u8 can, can_stat_err_t* err)
{
#if NUM_SUPPORTED_CANS == 0
  return -1; // no CAN available
#else
  *err= can_stat_report[can].bus_last_err;
  return 0; // no error
#endif
}

/////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_CAN_ReportGetCurr(u8 can, can_stat_report_t* report)
{
#if NUM_SUPPORTED_CANS == 0
  return -1; // no CAN available
#else
  CAN_TypeDef* CANx = NULL;
  if(!can)CANx = MIOS32_CAN1;else CANx = MIOS32_CAN2;
  
  can_stat_report[can].bus_curr_err.ALL = CANx->ESR;
  *report = can_stat_report[can];
  
  return 0; // no error
#endif
}

/////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_CAN_ReportReset(u8 can)
{
#if NUM_SUPPORTED_CANS == 0
  return -1; // no CAN available
#else

  can_stat_report[can].tx_packets_ctr = 0;
  can_stat_report[can].rx_packets_err = 0;
  can_stat_report[can].rx_last_buff_err = 0;
  can_stat_report[can].rx_buff_err_ctr = 0;
  can_stat_report[can].rx_packets_ctr = 0;
  
  return 0; // no error
#endif
}


#endif /* MIOS32_USE_CAN */
