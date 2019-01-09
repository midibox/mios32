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
#define NUM_SUPPORTED_CANS 0

/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////


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
  return 0; // no error
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
  return 0;
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
s32 MIOS32_CAN_Init32bitFilter(u8 bank, u8 fifo, mios32_can_ext_filter_t filter, u8 enabled)
{
#if MIOS32_CAN_NUM == 0
  return -1; // no CAN enabled
#else
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
s32 MIOS32_CAN_Init16bitFilter(u8 bank, u8 fifo, mios32_can_std_filter_t filter1, mios32_can_std_filter_t filter2, u8 enabled)
{
#if MIOS32_CAN_NUM == 0
  return -1; // no CAN enabled
#else
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
s32 MIOS32_CAN_InitPacket(mios32_can_packet_t *packet)
{
#if NUM_SUPPORTED_CANS == 0
  return -1; // no CAN available
#else
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
  return 0;
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
  return 0;
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
s32 MIOS32_CAN_RxBufferGet(u8 can, mios32_can_packet_t *p)
{
#if NUM_SUPPORTED_CANS == 0
  return -1; // no CAN available
#else
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
s32 MIOS32_CAN_RxBufferPeek(u8 can, mios32_can_packet_t *p)
{
#if NUM_SUPPORTED_CANS == 0
  return -1; // no CAN available
#else
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
s32 MIOS32_CAN_RxBufferPut(u8 can, mios32_can_packet_t p)
{
#if NUM_SUPPORTED_CANS == 0
  return -1; // no CAN available
#else
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
s32 MIOS32_CAN_TxBufferGet(u8 can, mios32_can_packet_t *p)
{
#if NUM_SUPPORTED_CANS == 0
  return -1; // no CAN available
#else
  return 0; // no error
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
s32 MIOS32_CAN_TxBufferPutMore_NonBlocking(u8 can, mios32_can_packet_t* p,u16 len)
{
#if NUM_SUPPORTED_CANS == 0
  return -1; // no CAN available
#else
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
s32 MIOS32_CAN_TxBufferPutMore(u8 can, mios32_can_packet_t *packets, u16 len)
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
s32 MIOS32_CAN_TxBufferPut_NonBlocking(u8 can, mios32_can_packet_t p)
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
s32 MIOS32_CAN_TxBufferPut(u8 can, mios32_can_packet_t p)
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
  return 0 ;
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
s32 MIOS32_CAN_Transmit(u8 can, mios32_can_packet_t p, s16 block_time)
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
s32 MIOS32_CAN_ReportLastErr(u8 can, mios32_can_stat_err_t* err)
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
s32 MIOS32_CAN_ReportGetCurr(u8 can, mios32_can_stat_report_t* report)
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
s32 MIOS32_CAN_ReportReset(u8 can)
{
#if NUM_SUPPORTED_CANS == 0
  return -1; // no CAN available
#else
  return 0; // no error
#endif
}


#endif /* MIOS32_USE_CAN */
