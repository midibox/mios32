// $Id$
//! \defgroup MIOS32_I2S
//!
//! I2S Functions
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

// this module can be optionally *ENABLED* in a local mios32_config.h file (included from mios32.h)
#if defined(MIOS32_USE_I2S)

// clocked at full speed for highest accuracy
#define MIOS32_I2S_PERIPHERAL_FRQ (MIOS32_SYS_CPU_FREQUENCY)

// TK: if peripheral clocked at full speed, it seems that the 1:2 divider after MCLK is not working
// therefore I'm using a "correcture factor" here:
// set this define to 1 if I2S clocked at full speed
// set this define to 2 if I2S clock divided by 2, 4 or 8
#define MIOS32_I2S_DIV2           1

// TX CLK: P2.11
#define MIOS32_I2S_CLK_INIT  { MIOS32_SYS_LPC_PINSEL(2, 11, 3); }
// TX WS: P2.12
#define MIOS32_I2S_WS_INIT   { MIOS32_SYS_LPC_PINSEL(2, 12, 3); }
// TX SDA: P2.13
#define MIOS32_I2S_SDA_INIT  { MIOS32_SYS_LPC_PINSEL(2, 13, 3); }
// TX MCLK: P4.29
#define MIOS32_I2S_MCLK_INIT { MIOS32_SYS_LPC_PINSEL(4, 29, 1); }

// DMA Request
#define MIOS32_I2S_DMA_REQ 5
// Allocated DMA Channel - it is matching with the request input number, but this is no must!
#define MIOS32_I2S_DMA_CHN 5


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

static void (*i2s_buffer_reload_callback)(u32 state) = NULL;
static u32 *i2s_buffer;
static u16 i2s_buffer_half_len;
static u8 i2s_buffer_half;

/////////////////////////////////////////////////////////////////////////////
// Local prototypes
/////////////////////////////////////////////////////////////////////////////
static void MIOS32_I2S_DMA_Callback(void);


/////////////////////////////////////////////////////////////////////////////
//! Initializes I2S interface
//! \param[in] mode currently only mode 0 supported
//! \return < 0 if initialisation failed
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_I2S_Init(u32 mode)
{
  // currently only mode 0 supported
  if( mode != 0 )
    return -1; // unsupported mode

  // stop ongoing transfer
  MIOS32_I2S_Stop();

  // configure I2S output pins
  MIOS32_I2S_CLK_INIT;
  MIOS32_I2S_WS_INIT;
  MIOS32_I2S_SDA_INIT;
  MIOS32_I2S_MCLK_INIT;

  // reset I2S peripheral
  LPC_I2S->I2SDAO = (1 << 4);
  LPC_I2S->I2SDAO = 0;

  // initialize I2S peripheral for 16bit wordwidth, stereo, WS halfperiod
  LPC_I2S->I2SDAO = (1 << 0) | (15 << 6);
  // enable DMA for I2S transmit, FIFO level at 0 (since DMA is always running in background)
  LPC_I2S->I2SDMA1 = (1 << 1) | (0 << 16);

  // set audio frequency
  // Inspired from lpc17xx_i2s.c of the IEC60335 Class B library v1.0
  // modified to output MCLK with higher frequency for oversampling
  u8  channels = 2;
  u8  wordwidth = 16;
  u8  bitrate = channels * wordwidth;
  u32 txclk_frequency = MIOS32_I2S_AUDIO_FREQ * bitrate; // e.g. 48 * 2*16 = 1.536 MHz
  u8  mclk_oversampling = 8; // results into 256 * f_sampling
  u32 mclk_frequency = txclk_frequency * mclk_oversampling; // e.g. 48k * 2*16 * 8 = 12.288 MHz

  // set divider for TX clock
  LPC_I2S->I2STXBITRATE = mclk_oversampling-1;

  /* Calculate X and Y divider
   * The MCLK rate for the I2S transmitter is determined by the value
   * in the I2STXRATE/I2SRXRATE register. The required I2STXRATE/I2SRXRATE
   * setting depends on the desired audio sample rate, the format
   * (stereo/mono) used, and the data size.
   * The formula is:
   *              I2S_MCLK = PCLK * (X/Y) / 2
   * We have:
   *              I2S_MCLK = Freq * bit_rate;
   * So: (X/Y) = (Freq * bit_rate)/PCLK*2
   * We use a loop function to chose the most suitable X,Y value
   */

  // thanks to Roger for providing a corrected version to calculate the x_divide value
  // (which has been taken from a NXP coding example).
  // x_divide is now calculated within the iteration below

  // He wrote:
  // The loop uses the absolute value of the error to optimize the X and Y values.
  // However, if the error is negative (>= 0x8000) then we are comparing against
  // X + 1. So we should use that X + 1 for setting the register. The original 
  // code doesn’t do that so that the clock rates will almost always be a few percent too low;
  //  
  // The code below is your code with arrows for new lines (->) and for lines to 
  // take out (<-). It now uses the original ‘x’ to calculate x_divide and will 
  // bump it up if the error is negative. When I run this the clock rates are accurate 
  // within promilles for a number of clock combinations.

  u32 divider = (u32)(((unsigned long long)(mclk_frequency*MIOS32_I2S_DIV2)<<16) / MIOS32_I2S_PERIPHERAL_FRQ);
  u16 ErrorOptimal = 0xffff;
  u8 y_divide = 0;
  u8 x_divide = 0;
  u32 error = 0;
  u32 y;
  for (y = 255; y > 0; y--) {
    u32 x = y * divider;
    u16 dif = x & 0xffff;
    if(dif>0x8000)
      error = 0x10000-dif;
    else
      error = dif;

    if (error == 0) {
      ErrorOptimal = error;
      y_divide = y;
      x_divide = x >> 16;
      break;
    } else if (error < ErrorOptimal) {
      ErrorOptimal = error;
      y_divide = y;
      x_divide = x >> 16;
      if (dif >= 0x8000)
	x_divide++; // Negative error, go one X higher
    }
  }

  // WRONG:
  // u8 x_divide = (y_divide * mclk_frequency*MIOS32_I2S_DIV2) / MIOS32_I2S_PERIPHERAL_FRQ;

  LPC_I2S->I2STXRATE = y_divide | (x_divide << 8);

  // set transmit mode: select TX fractional rate divider clock output as the source, enable MCLK
  LPC_I2S->I2STXMODE = (0 << 0) | (1 << 3);

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! Starts DMA driven I2S transfers
//! \param[in] *buffer pointer to sample buffer (contains L/R halfword)
//! \param[in] len size of audio buffer
//! \param[in] _callback callback function:<BR>
//! \code
//!   void callback(u32 state)
//! \endcode
//!      called when the lower (state == 0) or upper (state == 1)
//!      range of the sample buffer has been transfered, so that it
//!      can be updated
//! \return < 0 if initialisation failed
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_I2S_Start(u32 *buffer, u16 len, void *_callback)
{
  LPC_GPDMACH_TypeDef *dma_chn_ptr = (LPC_GPDMACH_TypeDef *)(LPC_GPDMACH0_BASE + MIOS32_I2S_DMA_CHN*0x20);

  // we send half of the buffer for each iteration...
  i2s_buffer_half_len = len / 2;

  if( i2s_buffer_half_len >= 0x1000 )
    return -4; // too many bytes (len too long)

  // stop ongoing transfer
  MIOS32_I2S_Stop();

  // install DMA callback
  MIOS32_SYS_DMA_CallbackSet(0, MIOS32_I2S_DMA_CHN, MIOS32_I2S_DMA_Callback);
  
  // take over new callback function and buffer location
  i2s_buffer_reload_callback = _callback;
  i2s_buffer = buffer;

  // set source/destination address
  dma_chn_ptr->DMACCSrcAddr = (u32)i2s_buffer;
  dma_chn_ptr->DMACCDestAddr = (u32)(&LPC_I2S->I2STXFIFO);
  // no linked list (TODO: but it would make sense here)
  dma_chn_ptr->DMACCLLI = 0;
  // set transfer size, enable increment for source address, enable terminal count interrupt
  dma_chn_ptr->DMACCControl = (i2s_buffer_half_len << 0) | (2 << 18) | (2 << 21)  | (1 << 26) | (0 << 27) | (1 << 31);
  // enable channel, dest. peripheral is I2S Tx, source is ignored, memory-to-peripheral, enable terminal counter irq
  dma_chn_ptr->DMACCConfig = (1 << 0) | (0 << 1) | (MIOS32_I2S_DMA_REQ << 6) | (1 << 11) | (1 << 15);

  // start with first buffer half
  i2s_buffer_half = 0;

  // start DMA via soft request on Tx channel
  LPC_GPDMA->DMACSoftSReq = (1 << MIOS32_I2S_DMA_CHN);

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! Stops DMA driven I2S transfers
//! \return < 0 if initialisation failed
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_I2S_Stop(void)
{
  LPC_GPDMACH_TypeDef *dma_chn_ptr = (LPC_GPDMACH_TypeDef *)(LPC_GPDMACH0_BASE + MIOS32_I2S_DMA_CHN*0x20);

  // disable DMA callback
  MIOS32_SYS_DMA_CallbackSet(0, MIOS32_I2S_DMA_CHN, NULL);

  // disable DMA channel
  dma_chn_ptr->DMACCConfig = 0;

  // clear pending interrupt requests
  LPC_GPDMA->DMACIntTCClear = (1 << MIOS32_I2S_DMA_CHN);

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! DMA Channel interrupt is triggered when half of the buffer has been processed
//! \note shouldn't be called directly from application
/////////////////////////////////////////////////////////////////////////////
static void MIOS32_I2S_DMA_Callback(void)
{
  LPC_GPDMACH_TypeDef *dma_chn_ptr = (LPC_GPDMACH_TypeDef *)(LPC_GPDMACH0_BASE + MIOS32_I2S_DMA_CHN*0x20);

  // disable DMA channel
  dma_chn_ptr->DMACCConfig = 0;

  if( i2s_buffer_half == 0 ) {
    // state 0: lower sample buffer range has been transfered and can be updated
    i2s_buffer_half = 1;

    // set new source location
    dma_chn_ptr->DMACCSrcAddr = (u32)i2s_buffer + 4*i2s_buffer_half_len;
  } else {
    // state 1: upper sample buffer range has been transfered and can be updated
    i2s_buffer_half = 0;

    // set new source location
    dma_chn_ptr->DMACCSrcAddr = (u32)i2s_buffer + 0;
  }

  // set transfer size, enable increment for source address, enable terminal count interrupt
  dma_chn_ptr->DMACCControl = (i2s_buffer_half_len << 0) | (2 << 18) | (2 << 21)  | (1 << 26) | (0 << 27) | (1 << 31);
  // enable channel, dest. peripheral is I2S Tx, source is ignored, memory-to-peripheral, enable terminal counter irq
  dma_chn_ptr->DMACCConfig = (1 << 0) | (0 << 1) | (MIOS32_I2S_DMA_REQ << 6) | (1 << 11) | (1 << 15);

  // start DMA via soft request on Tx channel
  LPC_GPDMA->DMACSoftSReq = (1 << MIOS32_I2S_DMA_CHN);

  // callback for sample buffer update of next half
  i2s_buffer_reload_callback(i2s_buffer_half ? 0 : 1);
}

//! \}

#endif /* MIOS32_USE_I2S */
