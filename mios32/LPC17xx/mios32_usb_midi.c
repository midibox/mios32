// $Id$
//! \defgroup MIOS32_USB_MIDI
//!
//! USB MIDI layer for MIOS32
//! 
//! \{
/* ==========================================================================
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

// this module can be optionally disabled in a local mios32_config.h file (included from mios32.h)
#if !defined(MIOS32_DONT_USE_USB_MIDI)

#include <usbapi.h>
#include <usbhw_lpc.h>


// for debugging Rx buffer usage
#define RX_BUFFER_MAX_ANALYSIS 0


/////////////////////////////////////////////////////////////////////////////
// Local prototypes
/////////////////////////////////////////////////////////////////////////////

static void MIOS32_USB_MIDI_TxBufferHandler(u8 bEP);
static void MIOS32_USB_MIDI_RxBufferHandler(u8 bEP);


/////////////////////////////////////////////////////////////////////////////
// Local Variables
/////////////////////////////////////////////////////////////////////////////

// Rx buffer
static u32 rx_buffer[MIOS32_USB_MIDI_RX_BUFFER_SIZE];
static volatile u16 rx_buffer_tail;
static volatile u16 rx_buffer_head;
static volatile u16 rx_buffer_size;

// Tx buffer
static u32 tx_buffer[MIOS32_USB_MIDI_TX_BUFFER_SIZE];
static volatile u16 tx_buffer_tail;
static volatile u16 tx_buffer_head;
static volatile u16 tx_buffer_size;

// transfer possible?
static u8 transfer_possible = 0;

static volatile u8 tx_buffer_busy;

/** convert from endpoint address to endpoint index */
#define EP2IDX(bEP)     ((((bEP)&0xF)<<1)|(((bEP)&0x80)>>7))
/** convert from endpoint index to endpoint address */
#define IDX2EP(idx)     ((((idx)<<7)&0x80)|(((idx)>>1)&0xF))

static void Wait4DevInt(U32 dwIntr)
{
        // wait for specific interrupt
        while ((LPC_USB->USBDevIntSt & dwIntr) != dwIntr);
        // clear the interrupt bits
        LPC_USB->USBDevIntClr = dwIntr;
}

static void USBHwCmd(U8 bCmd)
{
        // clear CDFULL/CCEMTY
        LPC_USB->USBDevIntClr = CDFULL | CCEMTY;
        // write command code
        LPC_USB->USBCmdCode = 0x00000500 | (bCmd << 16);
        Wait4DevInt(CCEMTY);
}

/////////////////////////////////////////////////////////////////////////////
//! Initializes USB MIDI layer
//! \param[in] mode currently only mode 0 supported
//! \return < 0 if initialisation failed
//! \note Applications shouldn't call this function directly, instead please use \ref MIOS32_MIDI layer functions
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_USB_MIDI_Init(u32 mode)
{
  // currently only mode 0 supported
  if( mode != 0 )
    return -1; // unsupported mode

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! This function is called by the USB driver on cable connection/disconnection
//! \param[in] connected status (1 if connected)
//! \return < 0 on errors
//! \note Applications shouldn't call this function directly, instead please use \ref MIOS32_MIDI layer functions
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_USB_MIDI_ChangeConnectionState(u8 connected)
{
  // in all cases: re-initialize USB MIDI driver
  // clear buffer counters and busy/wait signals again (e.g., so that no invalid data will be sent out)
  rx_buffer_tail = rx_buffer_head = rx_buffer_size = 0;
  tx_buffer_tail = tx_buffer_head = tx_buffer_size = 0;

  if( connected ) {
    transfer_possible = 1;
    tx_buffer_busy = 0; // buffer not busy anymore
  } else {
    // cable disconnected: disable transfers
    transfer_possible = 0;
    tx_buffer_busy = 1; // buffer busy
  }

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
//! This function returns the connection status of the USB MIDI interface
//! \param[in] cable number
//! \return 1: interface available
//! \return 0: interface not available
//! \note Applications shouldn't call this function directly, instead please use \ref MIOS32_MIDI layer functions
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_USB_MIDI_CheckAvailable(u8 cable)
{
#ifdef MIOS32_SYS_ADDR_BSL_INFO_BEGIN
  if( MIOS32_USB_ForceSingleUSB() && cable >= 1 )
    return 0;
#endif

  if( cable >= MIOS32_USB_MIDI_NUM_PORTS )
    return 0;

  return transfer_possible ? 1 : 0;
}


/////////////////////////////////////////////////////////////////////////////
//! This function puts a new MIDI package into the Tx buffer
//! \param[in] package MIDI package
//! \return 0: no error
//! \return -1: USB not connected
//! \return -2: buffer is full
//!             caller should retry until buffer is free again
//! \note Applications shouldn't call this function directly, instead please use \ref MIOS32_MIDI layer functions
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_USB_MIDI_PackageSend_NonBlocking(mios32_midi_package_t package)
{
  // device available?
  if( !transfer_possible )
    return -1;

  // buffer full?
  if( tx_buffer_size >= (MIOS32_USB_MIDI_TX_BUFFER_SIZE-1) ) {
    // call USB handler, so that we are able to get the buffer free again on next execution
    // (this call simplifies polling loops!)
    MIOS32_USB_MIDI_TxBufferHandler(MIOS32_USB_MIDI_DATA_IN_EP);

    // device still available?
    // (ensures that polling loop terminates if cable has been disconnected)
    if( !transfer_possible )
      return -1;

    // notify that buffer was full (request retry)
    return -2;
  }

  // put package into buffer - this operation should be atomic!
  MIOS32_IRQ_Disable();
  tx_buffer[tx_buffer_head++] = package.ALL;
  if( tx_buffer_head >= MIOS32_USB_MIDI_TX_BUFFER_SIZE )
    tx_buffer_head = 0;
  ++tx_buffer_size;
  MIOS32_IRQ_Enable();

  return 0;
}

/////////////////////////////////////////////////////////////////////////////
//! This function puts a new MIDI package into the Tx buffer
//! (blocking function)
//! \param[in] package MIDI package
//! \return 0: no error
//! \return -1: USB not connected
//! \note Applications shouldn't call this function directly, instead please use \ref MIOS32_MIDI layer functions
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_USB_MIDI_PackageSend(mios32_midi_package_t package)
{
  static u16 timeout_ctr = 0;
  // this function could hang up if USB is available, but MIDI port won't be
  // serviced by the host (e.g. windows: no program uses the MIDI IN port)
  // Therefore we time out the polling after 10000 tries
  // Once the timeout value is reached, each new MIDI_PackageSend call will
  // try to access the USB port only a single time anymore. Once the try
  // was successfull (MIDI port will be used by host), timeout value is
  // reset again

  s32 error;

  while( (error=MIOS32_USB_MIDI_PackageSend_NonBlocking(package)) == -2 ) {
    if( timeout_ctr >= 10000 )
      break;
    ++timeout_ctr;
  }

  if( error >= 0 ) // no error: reset timeout counter
    timeout_ctr = 0;

  return error;
}


/////////////////////////////////////////////////////////////////////////////
//! This function checks for a new package
//! \param[out] package pointer to MIDI package (received package will be put into the given variable)
//! \return -1 if no package in buffer
//! \return >= 0: number of packages which are still in the buffer
//! \note Applications shouldn't call this function directly, instead please use \ref MIOS32_MIDI layer functions
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_USB_MIDI_PackageReceive(mios32_midi_package_t *package)
{
#if RX_BUFFER_MAX_ANALYSIS
  static u16 rx_buffer_max_size = 0;
#endif

  // package received?
  if( !rx_buffer_size )
    return -1;

#if RX_BUFFER_MAX_ANALYSIS
  if( rx_buffer_size > rx_buffer_max_size ) {
    rx_buffer_max_size = rx_buffer_size;
    MIOS32_MIDI_SendDebugMessage("[MIOS32_USB_MIDI] Max Rx Buffer: %d\n", rx_buffer_max_size);
  }
#endif

  // get package - this operation should be atomic!
  MIOS32_IRQ_Disable();
  package->ALL = rx_buffer[rx_buffer_tail];
  if( ++rx_buffer_tail >= MIOS32_USB_MIDI_RX_BUFFER_SIZE )
    rx_buffer_tail = 0;
  --rx_buffer_size;
  MIOS32_IRQ_Enable();

  return rx_buffer_size;
}



/////////////////////////////////////////////////////////////////////////////
//! This function should be called periodically each mS to handle timeout
//! and expire counters.
//!
//! For USB MIDI it also checks for incoming/outgoing USB packages!
//!
//! Not for use in an application - this function is called from
//! MIOS32_MIDI_Periodic_mS(), which is called by a task in the programming
//! model!
//! 
//! \return < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_USB_MIDI_Periodic_mS(void)
{
  if( transfer_possible ) {
    // check for received packages
    MIOS32_USB_MIDI_RxBufferHandler(MIOS32_USB_MIDI_DATA_OUT_EP);
  
    // check for packages which should be transmitted
    MIOS32_USB_MIDI_TxBufferHandler(MIOS32_USB_MIDI_DATA_IN_EP);
  }

  return 0;
}


/////////////////////////////////////////////////////////////////////////////
// This handler sends the new packages through the IN pipe if the buffer 
// is not empty
/////////////////////////////////////////////////////////////////////////////
static void MIOS32_USB_MIDI_TxBufferHandler(u8 bEP)
{
  // send buffered packages if
  //   - last transfer finished
  //   - new packages are in the buffer
  //   - the device is configured

  // atomic operation to avoid conflict with other interrupts
  MIOS32_IRQ_Disable();
  if( !tx_buffer_busy && tx_buffer_size && transfer_possible ) {
    s16 count = (tx_buffer_size > (MIOS32_USB_MIDI_DATA_IN_SIZE/4)) ? (MIOS32_USB_MIDI_DATA_IN_SIZE/4) : tx_buffer_size;

    // from USBHwEPWrite
        
    // set write enable for specific endpoint
    LPC_USB->USBCtrl = WR_EN | ((bEP & 0xF) << 2);

    // set packet length
    LPC_USB->USBTxPLen = 4*count;
        
    // write data
    int real_count = 0;
    while( count && (LPC_USB->USBCtrl & WR_EN) ) {
      ++real_count;
      LPC_USB->USBTxData = tx_buffer[tx_buffer_tail];
      if( ++tx_buffer_tail >= MIOS32_USB_MIDI_TX_BUFFER_SIZE )
	tx_buffer_tail = 0;
    }

    // notify that new package is sent
    tx_buffer_busy = 1;

    // send to IN pipe
    tx_buffer_size -= real_count;

    // select endpoint and validate buffer
    USBHwCmd(CMD_EP_SELECT | EP2IDX(bEP));
    USBHwCmd(CMD_EP_VALIDATE_BUFFER);
  }

  MIOS32_IRQ_Enable();
}


/////////////////////////////////////////////////////////////////////////////
// This handler receives new packages if the Tx buffer is not full
/////////////////////////////////////////////////////////////////////////////
static void MIOS32_USB_MIDI_RxBufferHandler(u8 bEP)
{
  // atomic operation to avoid conflict with other interrupts
  MIOS32_IRQ_Disable();

  // from USBHwEPRead

  // set read enable bit for specific endpoint
  LPC_USB->USBCtrl = RD_EN | ((bEP & 0xF) << 2);

  // wait for PKT_RDY
  u32 dwLen;
  do {
    dwLen = LPC_USB->USBRxPLen;
  } while( (dwLen & PKT_RDY) == 0 );
        
  // packet valid?
  if( dwLen & DV ) {

    // get length
    s16 count = (dwLen & PKT_LNGTH_MASK) >> 2;

    // check if buffer is free
    if( count && count < (MIOS32_USB_MIDI_RX_BUFFER_SIZE-rx_buffer_size) ) {

      while( count-- > 0 ) {
	// copy received packages into receive buffer
	mios32_midi_package_t package;
	package.ALL = LPC_USB->USBRxData;

	if( MIOS32_MIDI_SendPackageToRxCallback(USB0 + package.cable, package) == 0 ) {
	  rx_buffer[rx_buffer_head] = package.ALL;

	  if( ++rx_buffer_head >= MIOS32_USB_MIDI_RX_BUFFER_SIZE )
	    rx_buffer_head = 0;
	  ++rx_buffer_size;
	}
      }

      // make sure RD_EN is clear
      LPC_USB->USBCtrl = 0;

      // select endpoint and clear buffer
      USBHwCmd(CMD_EP_SELECT | EP2IDX(bEP));
      USBHwCmd(CMD_EP_CLEAR_BUFFER);
    }
  }

  // make sure RD_EN is clear
  LPC_USB->USBCtrl = 0;

  MIOS32_IRQ_Enable();
}


/////////////////////////////////////////////////////////////////////////////
//! Called by USB driver to check for IN streams
//! \note Applications shouldn't call this function directly, instead please use \ref MIOS32_MIDI layer functions
/////////////////////////////////////////////////////////////////////////////
void MIOS32_USB_MIDI_EP1_IN_Callback(u8 bEP, u8 bEPStatus)
{
  // package has been sent
  tx_buffer_busy = 0;

  // check for next package
  MIOS32_USB_MIDI_TxBufferHandler(bEP);
}

/////////////////////////////////////////////////////////////////////////////
//! Called by USB driver to check for OUT streams
//! \note Applications shouldn't call this function directly, instead please use \ref MIOS32_MIDI layer functions
/////////////////////////////////////////////////////////////////////////////
void MIOS32_USB_MIDI_EP2_OUT_Callback(u8 bEP, u8 bEPStatus)
{
  // package has been sent
  if( bEP & 0x80 ) {
    // note: shared callback, IN and OUT irq can trigger it
    MIOS32_USB_MIDI_EP1_IN_Callback(bEP, bEPStatus);
  } else {
    // put package into buffer
    MIOS32_USB_MIDI_RxBufferHandler(bEP);
  }
}

//! \}

#endif /* MIOS32_DONT_USE_USB_MIDI */
