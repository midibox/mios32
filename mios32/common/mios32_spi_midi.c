// $Id$
//! \defgroup MIOS32_SPI_MIDI
//!
//! SPI MIDI layer for MIOS32
//! 
//! Applications shouldn't call these functions directly, instead please
//! use \ref MIOS32_MIDI layer functions
//! 
//! \{
/* ==========================================================================
 *
 *  Copyright (C) 2014 Thorsten Klose (tk@midibox.org)
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
#if !defined(MIOS32_DONT_USE_SPI_MIDI)


/////////////////////////////////////////////////////////////////////////////
// Local definitions
/////////////////////////////////////////////////////////////////////////////

#if !defined(MIOS32_SPI_MIDI_MUTEX_TAKE)
#define MIOS32_SPI_MIDI_MUTEX_TAKE {}
#define MIOS32_SPI_MIDI_MUTEX_GIVE {}
#endif


/////////////////////////////////////////////////////////////////////////////
// Local Variables
/////////////////////////////////////////////////////////////////////////////

#if MIOS32_SPI_MIDI_NUM_PORTS > 0
// TX double buffer toggles between each scan
static u32 tx_upstream_buffer[2][MIOS32_SPI_MIDI_SCAN_BUFFER_SIZE];
static u8 tx_upstream_buffer_select;

// RX downstream buffer used to temporary store new words from current scan
static u32 rx_downstream_buffer[MIOS32_SPI_MIDI_SCAN_BUFFER_SIZE];

// RX ring buffer
static u32 rx_ringbuffer[MIOS32_SPI_MIDI_RX_RINGBUFFER_SIZE];

#if MIOS32_SPI_MIDI_SCAN_BUFFER_SIZE > 255 || MIOS32_SPI_MIDI_RX_RINGBUFFER_SIZE > 255
# error "Please adapt size pointers!"
#endif
static u8 tx_buffer_head;

static u8 rx_ringbuffer_tail;
static u8 rx_ringbuffer_head;
static u8 rx_ringbuffer_size;

// indicates ongoing scan
static u8 transfer_done;
#endif


/////////////////////////////////////////////////////////////////////////////
// Local Prototypes
/////////////////////////////////////////////////////////////////////////////

#if MIOS32_SPI_MIDI_NUM_PORTS > 0
static s32 MIOS32_SPI_MIDI_InitScanBuffer(u32 *buffer);
static void MIOS32_SPI_MIDI_DMA_Callback(void);
#endif


/////////////////////////////////////////////////////////////////////////////
// Initializes the SPI Master
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_SPI_MIDI_Init(u32 mode)
{
#if MIOS32_SPI_MIDI_NUM_PORTS == 0
  return -1; // SPI MIDI not activated
#else
  s32 status = 0;

  if( mode != 0 )
    return -1; // currently only mode 0 supported

  // deactivate CS output
  MIOS32_SPI_RC_PinSet(MIOS32_SPI_MIDI_SPI, MIOS32_SPI_MIDI_SPI_RC_PIN, 1); // spi, rc_pin, pin_value

  // ensure that fast pin drivers are activated
  MIOS32_SPI_IO_Init(MIOS32_SPI_MIDI_SPI, MIOS32_SPI_PIN_DRIVER_STRONG);

  // starting with first half of the double buffer
  tx_upstream_buffer_select = 0;

  // last transfer done (to allow next transfer)
  transfer_done = 1;

  // init buffer pointers
  tx_buffer_head = 0;
  rx_ringbuffer_tail = rx_ringbuffer_head = rx_ringbuffer_size = 0;

  // init double buffers
  MIOS32_SPI_MIDI_InitScanBuffer((u32 *)&tx_upstream_buffer[0]);
  MIOS32_SPI_MIDI_InitScanBuffer((u32 *)&tx_upstream_buffer[1]);

  return status;
#endif
}


/////////////////////////////////////////////////////////////////////////////
//! This function checks the availability of a SPI MIDI port as configured
//! with MIOS32_SPI_MIDI_NUM_PORTS
//!
//! \param[in] spi_midi_port module number (0..7)
//! \return 1: interface available
//! \return 0: interface not available
//! \note Applications shouldn't call this function directly, instead please use \ref MIOS32_MIDI layer functions
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_SPI_MIDI_CheckAvailable(u8 spi_midi_port)
{
#if MIOS32_SPI_MIDI_NUM_PORTS == 0
  return 0; // SPI MIDI interface not explicitely enabled in mios32_config.h
#else
  return (spi_midi_port < MIOS32_SPI_MIDI_NUM_PORTS) ? 1 : 0;
#endif
}


/////////////////////////////////////////////////////////////////////////////
//! This function should be called periodically each mS to initiate a new
//! SPI scan
//!
//! Not for use in an application - this function is called from
//! MIOS32_MIDI_Periodic_mS(), which is called by a task in the programming
//! model!
//! 
//! \return < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_SPI_MIDI_Periodic_mS(void)
{
#if MIOS32_SPI_MIDI_NUM_PORTS == 0
  return 0; // SPI MIDI not activated (no error)
#else
  if( !transfer_done )
    return -2; // previous transfer not finished yet

  // following operation should be atomic
  MIOS32_IRQ_Disable();

  // last TX buffer
  u32 *last_tx = (u32 *)&tx_upstream_buffer[tx_upstream_buffer_select];

  // next TX buffer
  tx_upstream_buffer_select = tx_upstream_buffer_select ? 0 : 1;
  u32 *next_tx = (u32 *)&tx_upstream_buffer[tx_upstream_buffer_select];

  // init last TX buffer for next words
  MIOS32_SPI_MIDI_InitScanBuffer(last_tx);
  tx_buffer_head = 0;

  // start next transfer
  transfer_done = 0;

  MIOS32_IRQ_Enable();

  // take over access over SPI port
  MIOS32_SPI_MIDI_MUTEX_TAKE;

  // init SPI
  MIOS32_SPI_TransferModeInit(MIOS32_SPI_MIDI_SPI,
			      MIOS32_SPI_MODE_CLK1_PHASE1,
			      MIOS32_SPI_MIDI_SPI_PRESCALER);

  // activate CS output
  MIOS32_SPI_RC_PinSet(MIOS32_SPI_MIDI_SPI, MIOS32_SPI_MIDI_SPI_RC_PIN, 0); // spi, rc_pin, pin_value

  // start next transfer
  MIOS32_SPI_TransferBlock(MIOS32_SPI_MIDI_SPI,
			   (u8 *)next_tx, (u8 *)rx_downstream_buffer,
			   4*MIOS32_SPI_MIDI_SCAN_BUFFER_SIZE,
			   MIOS32_SPI_MIDI_DMA_Callback);

  return 0; // no error
#endif
}


/////////////////////////////////////////////////////////////////////////////
// Called after DMA transfer finished
/////////////////////////////////////////////////////////////////////////////
#if MIOS32_SPI_MIDI_NUM_PORTS > 0
static void MIOS32_SPI_MIDI_DMA_Callback(void)
{
  // deactivate CS output
  MIOS32_SPI_RC_PinSet(MIOS32_SPI_MIDI_SPI, MIOS32_SPI_MIDI_SPI_RC_PIN, 1); // spi, rc_pin, pin_value

  // release access over SPI port
  MIOS32_SPI_MIDI_MUTEX_GIVE;

  // transfer RX values into ringbuffer (if possible)
  if( rx_ringbuffer_size < MIOS32_SPI_MIDI_RX_RINGBUFFER_SIZE ) {
    int i;

    // atomic operation to avoid conflict with other interrupts
    MIOS32_IRQ_Disable();

    // search for valid MIDI events in downstream buffer, and put them into the receive ringbuffer
    u32 *rx_buffer = (u32 *)&rx_downstream_buffer[0];
    for(i=0; i<MIOS32_SPI_MIDI_SCAN_BUFFER_SIZE; ++i) {
      u32 word = *rx_buffer++;

      if( word != 0xffffffff && word != 0x00000000 ) {
	// since data has been received bytewise, we've to swap the order
	mios32_midi_package_t p;
	p.cin_cable = word >> 24;
	p.evnt0 = word >> 16;
	p.evnt1 = word >> 8;
	p.evnt2 = word >> 0;

	rx_ringbuffer[rx_ringbuffer_head] = p.ALL;

	if( ++rx_ringbuffer_head >= MIOS32_SPI_MIDI_RX_RINGBUFFER_SIZE )
	  rx_ringbuffer_head = 0;

	if( ++rx_ringbuffer_size >= MIOS32_SPI_MIDI_RX_RINGBUFFER_SIZE )
	  break; // ringbuffer full :-( - TODO: add rx error counter
      }
    }

    MIOS32_IRQ_Enable();
  }

  // transfer finished
  transfer_done = 1;
}
#endif


/////////////////////////////////////////////////////////////////////////////
// This function puts a new MIDI package into the Tx buffer
// \param[in] package MIDI package
// \return 0: no error
// \return -1: SPI not configured
// \return -2: buffer is full
//             caller should retry until buffer is free again
// \note Applications shouldn't call this function directly, instead please use \ref MIOS32_MIDI layer functions
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_SPI_MIDI_PackageSend_NonBlocking(mios32_midi_package_t package)
{
#if MIOS32_SPI_MIDI_NUM_PORTS == 0
  return -1; // SPI MIDI not activated
#else
  // buffer full?
  if( tx_buffer_head >= MIOS32_SPI_MIDI_SCAN_BUFFER_SIZE ) {
    // flush buffer if possible
    // (this call simplifies polling loops!)
    MIOS32_SPI_MIDI_Periodic_mS();

    // notify that buffer was full (request retry)
    return -2;
  }

  // since data will be transmitted bytewise, we've to swap the order
  u32 word = (package.cin_cable << 24) | (package.evnt0 << 16) | (package.evnt1 << 8) | (package.evnt2 << 0);

  // put package into buffer - this operation should be atomic!
  MIOS32_IRQ_Disable();
  u8 next_select = tx_upstream_buffer_select ? 0 : 1;
  tx_upstream_buffer[next_select][tx_buffer_head] = word;
  ++tx_buffer_head;
  MIOS32_IRQ_Enable();

  return 0;
#endif
}

/////////////////////////////////////////////////////////////////////////////
//! This function puts a new MIDI package into the Tx buffer
//! (blocking function)
//! \param[in] package MIDI package
//! \return 0: no error
//! \return -1: SPI not configured
//! \note Applications shouldn't call this function directly, instead please use \ref MIOS32_MIDI layer functions
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_SPI_MIDI_PackageSend(mios32_midi_package_t package)
{
#if MIOS32_SPI_MIDI_NUM_PORTS == 0
  return -1; // SPI MIDI not activated
#else
  static u16 timeout_ctr = 0;
  // this function could hang up if SPI receive buffer not empty and data
  // should be sent.
  // Therefore we time out the polling after 10000 tries
  // Once the timeout value is reached, each new MIDI_PackageSend call will
  // try to access the SPI only a single time anymore. Once the try
  // was successfull (transfer done and receive buffer empty), timeout value is
  // reset again

  s32 error;

  while( (error=MIOS32_SPI_MIDI_PackageSend_NonBlocking(package)) == -2 ) {
    if( timeout_ctr >= 10000 )
      break;
    ++timeout_ctr;
  }

  if( error >= 0 ) // no error: reset timeout counter
    timeout_ctr = 0;

  return error;
#endif
}


/////////////////////////////////////////////////////////////////////////////
//! This function checks for a new package
//! \param[out] package pointer to MIDI package (received package will be put into the given variable)
//! \return -1 if no package in buffer
//! \return >= 0: number of packages which are still in the buffer
//! \note Applications shouldn't call this function directly, instead please use \ref MIOS32_MIDI layer functions
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_SPI_MIDI_PackageReceive(mios32_midi_package_t *package)
{
#if MIOS32_SPI_MIDI_NUM_PORTS == 0
  return -1; // SPI MIDI not activated
#else
  // package received?
  if( !rx_ringbuffer_size )
    return -1;

  // get package - this operation should be atomic!
  MIOS32_IRQ_Disable();
  package->ALL = rx_ringbuffer[rx_ringbuffer_tail];
  if( ++rx_ringbuffer_tail >= MIOS32_SPI_MIDI_RX_RINGBUFFER_SIZE )
    rx_ringbuffer_tail = 0;
  --rx_ringbuffer_size;
  MIOS32_IRQ_Enable();

  return rx_ringbuffer_size;
#endif
}



/////////////////////////////////////////////////////////////////////////////
// Invalidates a buffer with all-1
/////////////////////////////////////////////////////////////////////////////
#if MIOS32_SPI_MIDI_NUM_PORTS > 0
static s32 MIOS32_SPI_MIDI_InitScanBuffer(u32 *buffer)
{
  int i;

  for(i=0; i<MIOS32_SPI_MIDI_SCAN_BUFFER_SIZE; ++i) {
    *buffer++ = 0xffffffff;
  }

  return 0; // no error
}
#endif

//! \}

#endif /* MIOS32_DONT_USE_SPI_MIDI */
