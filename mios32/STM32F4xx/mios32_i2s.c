// $Id$
//! \defgroup MIOS32_I2S
//!
//! I2S Functions
//!
//! Approach: I2S pins are connected to J8 of the MBHP_CORE_STM32 module
//! Optionally a master clock is available for 8x oversampling, which has
//! to be explicitely enabled via MIOS32_I2S_ENABLE_MCLK, and has to be
//! connected to J15b:E (pinning see below)
//!
//! Due to pin conflicts with default setting of MIOS32_SRIO, MIOS_I2S is 
//! disabled by default, and has to be enabled via MIOS32_USE_I2S in
//! mios32_config.h<BR>
//! MIOS32_SRIO should either be disabled via MIOS32_DONT_USE_SRIO, or 
//! mapped to SPI0 (-> J16 of MBHP_CORE_STM32 module) via MIOS32_SRIO_SPI 0
//!
//! I2S is configured for standard Philips format with 2x16 bits for L/R
//! channel at 48 kHz by default. Other protocols/sample rates can be selected
//! via MIOS32_I2S_* defines
//! 
//! DMA Channel 5 is used to transfer sample values to the SPI in background.
//! An application specific callback function is invoked whenever the transfer
//! of the lower or upper half of the sample buffer is completed. This allows
//! the application to update the already uploaded sample buffer half with new
//! data while the next half is transfered.
//! 
//! Accordingly, the sample buffer size together with the sample rate defines
//! the time span between audio buffer updates.
//!
//! Calculation of the "refill rate" (transfer time of one sample buffer half)<BR>
//!   2 * sample rate / (sample buffer size)
//!
//! e.g.: 1.3 mS @ 48 kHz and 2*64 byte buffer
//! 
//! The sample buffer consists of 32bit words, LSBs for left channel, MSBs for
//! right channel
//! 
//! Demo application: $MIOS32_PATH/apps/examples/i2s
//!
//! \note this module can be optionally *ENABLED* in a local mios32_config.h file (included from mios32.h) by adding '#define MIOS32_USE_I2S'
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

// this module can be optionally *ENABLED* in a local mios32_config.h file (included from mios32.h)
#if defined(MIOS32_USE_I2S)


/////////////////////////////////////////////////////////////////////////////
// Pin definitions
/////////////////////////////////////////////////////////////////////////////

#define MIOS32_I2S_WS_PIN    GPIO_Pin_12;
#define MIOS32_I2S_WS_PORT   GPIOB

#define MIOS32_I2S_CK_PIN    GPIO_Pin_13;
#define MIOS32_I2S_CK_PORT   GPIOB

#define MIOS32_I2S_SD_PIN    GPIO_Pin_15;
#define MIOS32_I2S_SD_PORT   GPIOB

#define MIOS32_I2S_MCLK_PIN  GPIO_Pin_6;
#define MIOS32_I2S_MCLK_PORT GPIOC


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

static void (*buffer_reload_callback)(u32 state) = NULL;


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

  // configure I2S pins
  GPIO_InitTypeDef GPIO_InitStructure;
  GPIO_StructInit(&GPIO_InitStructure);
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;

  GPIO_InitStructure.GPIO_Pin = MIOS32_I2S_WS_PIN;
  GPIO_Init(MIOS32_I2S_WS_PORT, &GPIO_InitStructure);
  GPIO_InitStructure.GPIO_Pin = MIOS32_I2S_CK_PIN;
  GPIO_Init(MIOS32_I2S_CK_PORT, &GPIO_InitStructure);
  GPIO_InitStructure.GPIO_Pin = MIOS32_I2S_SD_PIN;
  GPIO_Init(MIOS32_I2S_SD_PORT, &GPIO_InitStructure);
#if MIOS32_I2S_MCLK_ENABLE
  GPIO_InitStructure.GPIO_Pin = MIOS32_I2S_MCLK_PIN;
  GPIO_Init(MIOS32_I2S_MCLK_PORT, &GPIO_InitStructure);
#endif


  // I2S initialisation
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI2, ENABLE);
  I2S_InitTypeDef I2S_InitStructure;
  I2S_StructInit(&I2S_InitStructure);
  I2S_InitStructure.I2S_Standard = MIOS32_I2S_STANDARD;
  I2S_InitStructure.I2S_DataFormat = MIOS32_I2S_DATA_FORMAT;
  I2S_InitStructure.I2S_MCLKOutput = MIOS32_I2S_MCLK_ENABLE ? I2S_MCLKOutput_Enable : I2S_MCLKOutput_Disable;
  I2S_InitStructure.I2S_AudioFreq  = (u16)(MIOS32_I2S_AUDIO_FREQ);
  I2S_InitStructure.I2S_CPOL = I2S_CPOL_Low; // configuration required as well?
  I2S_InitStructure.I2S_Mode = I2S_Mode_MasterTx;
  I2S_Init(SPI2, &I2S_InitStructure);
  I2S_Cmd(SPI2, ENABLE);

  // DMA Configuration for SPI Tx Event
  RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
  DMA_InitTypeDef DMA_InitStructure;
  DMA_StructInit(&DMA_InitStructure);
  DMA_Cmd(DMA1_Channel5, DISABLE);
  DMA_ClearFlag(DMA1_FLAG_TC5 | DMA1_FLAG_TE5 | DMA1_FLAG_HT5 | DMA1_FLAG_GL5);
  DMA_InitStructure.DMA_PeripheralBaseAddr = (u32)&SPI2->DR;
  //  DMA_InitStructure.DMA_MemoryBaseAddr = ...; // configured in MIOS32_I2S_Start()
  DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralDST;
  //  DMA_InitStructure.DMA_BufferSize = ...; // configured in MIOS32_I2S_Start()
  DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
  DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
  DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
  DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
  DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;
  DMA_InitStructure.DMA_Priority = DMA_Priority_Medium;
  DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
  DMA_Init(DMA1_Channel5, &DMA_InitStructure);
  // DMA_Cmd(DMA1_Channel5, ENABLE); // done on MIOS32_I2S_Start()

  // trigger interrupt when transfer half complete/complete
  DMA_ITConfig(DMA1_Channel5, DMA_IT_HT | DMA_IT_TC, ENABLE);

  // enable SPI interrupts to DMA
  SPI_I2S_DMACmd(SPI2, SPI_I2S_DMAReq_Tx, ENABLE);

  // Configure and enable DMA interrupt
  MIOS32_IRQ_Install(DMA1_Channel5_IRQn, MIOS32_IRQ_I2S_DMA_PRIORITY);

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
  // reload DMA source address and counter
  DMA_Cmd(DMA1_Channel5, DISABLE);

  // ensure that IRQ flag is cleared (so that DMA IRQ isn't invoked by accident while this function is called)
  DMA_ClearFlag(DMA1_FLAG_TC5 | DMA1_FLAG_TE5 | DMA1_FLAG_HT5 | DMA1_FLAG_GL5);

  // take over new callback function
  buffer_reload_callback = _callback;

  // take over new buffer pointer/length
  DMA1_Channel5->CMAR = (u32)buffer;
  DMA1_Channel5->CNDTR = 2 * len;
  DMA_Cmd(DMA1_Channel5, ENABLE);

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! Stops DMA driven I2S transfers
//! \return < 0 if initialisation failed
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_I2S_Stop(void)
{
  // disable DMA
  DMA_Cmd(DMA1_Channel5, DISABLE);

  return 0; // no error
}



/////////////////////////////////////////////////////////////////////////////
//! DMA1 Channel interrupt is triggered on HT and TC interrupts
//! \note shouldn't be called directly from application
/////////////////////////////////////////////////////////////////////////////
void DMA1_Channel5_IRQHandler(void)
{
  // execute callback function depending on pending flag(s)

  if( DMA1->ISR & DMA1_FLAG_HT5 ) {
    DMA1->IFCR = DMA1_FLAG_HT5;
    // state 0: lower sample buffer range has been transfered and can be updated
    buffer_reload_callback(0);
  }

  if( DMA1->ISR & DMA1_FLAG_TC5 ) {
    DMA1->IFCR = DMA1_FLAG_TC5;
    // state 1: upper sample buffer range has been transfered and can be updated
    buffer_reload_callback(1);
  }
}

//! \}

#endif /* MIOS32_USE_I2S */
