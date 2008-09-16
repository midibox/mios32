// $Id$
/*
 * SRIO Driver for MIOS32
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

// this module can be optionally disabled in a local mios32_config.h file (included from mios32.h)
#if !defined(MIOS32_DONT_USE_SRIO)


#if defined(_STM32x_)
# include <stm32f10x_map.h>
# include <stm32f10x_rcc.h>
# include <stm32f10x_gpio.h>
# include <stm32f10x_spi.h>
# include <stm32f10x_nvic.h>
#else
  XXX unsupported derivative XXX
#endif


/////////////////////////////////////////////////////////////////////////////
// Global variables
/////////////////////////////////////////////////////////////////////////////

volatile u8 mios32_srio_dout[MIOS32_SRIO_NUM_SR];
volatile u8 mios32_srio_din[MIOS32_SRIO_NUM_SR];
volatile u8 mios32_srio_din_changed[MIOS32_SRIO_NUM_SR];


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

u8 srio_irq_state;

void (*srio_scan_finished_hook)(void);

/////////////////////////////////////////////////////////////////////////////
// Initializes SPI pins and peripheral
// IN: <mode>: currently only mode 0 supported
//             later we could provide different pin mappings and operation 
//             modes (e.g. output only)
// OUT: returns < 0 if initialisation failed
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_SRIO_Init(u32 mode)
{
  u8 i;

  SPI_InitTypeDef  SPI_InitStructure;
  GPIO_InitTypeDef GPIO_InitStructure;
  NVIC_InitTypeDef NVIC_InitStructure;

  // disable notification hook
  srio_scan_finished_hook = NULL;

  // currently only mode 0 supported
  if( mode != 0 )
    return -1; // unsupported mode

  // clear chains
  // will be done again in MIOS32_DIN_Init and MIOS32_DOUT_Init
  // we don't reference to these functions here to allow the programmer to remove/replace these driver modules)
  for(i=0; i<MIOS32_SRIO_NUM_SR; ++i) {
    mios32_srio_dout[i] = 0;
    mios32_srio_din[i] = 0xff; // passive state
    mios32_srio_din_changed[i] = 0;
  }

  // STM32 SPI1 pins:
  //   o SPI1_SCLK: A5
  //   o SPI1_MISO: A6
  //   o SPI1_MOSI: A7
  //   o RCLK: A8

  // init GPIO structure
  GPIO_StructInit(&GPIO_InitStructure);

  // A5 and A7 are outputs assigned to alternate functions
  GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_5 | GPIO_Pin_7;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF_PP;
  GPIO_Init(GPIOA, &GPIO_InitStructure);

  // A8 is outputs assigned to GPIO
  GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_8;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;
  GPIO_Init(GPIOA, &GPIO_InitStructure);

  // A6 is input with pull-up
  GPIO_StructInit(&GPIO_InitStructure);
  GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_6;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IPU;
  GPIO_Init(GPIOA, &GPIO_InitStructure);

  // enable SPI1 peripheral clock (APB2 == high speed)
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, ENABLE);

  // enable GPIOA clock
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

  // SPI1 configuration
  SPI_StructInit(&SPI_InitStructure);
  SPI_InitStructure.SPI_Direction           = SPI_Direction_2Lines_FullDuplex;
  SPI_InitStructure.SPI_Mode                = SPI_Mode_Master;
  SPI_InitStructure.SPI_DataSize            = SPI_DataSize_8b;
  SPI_InitStructure.SPI_CPOL                = SPI_CPOL_High;
  SPI_InitStructure.SPI_CPHA                = SPI_CPHA_2Edge;
  SPI_InitStructure.SPI_NSS                 = SPI_NSS_Soft;
  SPI_InitStructure.SPI_BaudRatePrescaler   = SPI_BaudRatePrescaler_128; // ca. 1 uS period
  SPI_InitStructure.SPI_FirstBit            = SPI_FirstBit_MSB;
  SPI_InitStructure.SPI_CRCPolynomial       = 7;
  SPI_Init(SPI1, &SPI_InitStructure);

  // enable SPI1
  SPI_Cmd(SPI1, ENABLE);

  // initial state of RCLK
  GPIO_WriteBit(GPIOA, GPIO_Pin_8, 1);

  // start with initial state
  srio_irq_state = 0xff;

  // Configure and enable SPI1 interrupt
  NVIC_StructInit(&NVIC_InitStructure);
  NVIC_InitStructure.NVIC_IRQChannel = SPI1_IRQChannel;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);

  return 0;
}


/////////////////////////////////////////////////////////////////////////////
// (Re-)Starts the SPI IRQ Handler which scans the SRIO chain
// IN: notification function which will be called after the scan has been finished
//     (all DOUT registers written, all DIN registers read)
//     use NULL if no function should be called
// OUT: returns < 0 if operation failed
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_SRIO_ScanStart(void *_notify_hook)
{
  volatile s32 delay; // ensure, that delay won't be removed by compiler (depends on optimisation level)

#if MIOS32_SRIO_NUM_SR == 0
  return -1; // no SRIO scan required
#endif

  // exit if previous stream hasn't been sent yet (no additional transfer required)
  if( srio_irq_state != 0xff )
    return -2; // notify this special scenario - we could retry here

  // disable SPI1 RXNE interrupt
  SPI_I2S_ITConfig(SPI1, SPI_I2S_IT_RXNE, DISABLE);

  // just for the case that there is an ongoing transfer (should be avoided by calling this routine each mS)
  // loop while DR register in not empty
  while( SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE) == RESET );

  // reset IRQ state
  srio_irq_state = 0;

  // change notification function
  srio_scan_finished_hook = _notify_hook;

  // before first byte will be sent:
  // latch DIN registers by pulsing RCLK: 1->0->1
  GPIO_WriteBit(GPIOA, GPIO_Pin_8, 0);
  // TODO: find a proper way to produce a delay with uS accuracy, vTaskDelay and vTaskDelayUntil only provide mS accuracy
  // TODO: wait longer for touch sensors
  // TODO: maybe we should disable all IRQs here for higher accuracy
  for(delay=0; delay<10; ++delay);
  GPIO_WriteBit(GPIOA, GPIO_Pin_8, 1);

  // Enable SPI1 RXNE interrupt
  SPI_I2S_ITConfig(SPI1, SPI_I2S_IT_RXNE, ENABLE);

  // send first byte
  SPI_I2S_SendData(SPI1, mios32_srio_dout[MIOS32_SRIO_NUM_SR-1]);

  return 0;
}


/////////////////////////////////////////////////////////////////////////////
// SPI IRQ Handler which scans the SRIO chain
/////////////////////////////////////////////////////////////////////////////
void MIOS32_SRIO_SPI_IRQHandler(void)
{
  // if RXNE event has been triggered
  if( SPI_I2S_GetITStatus(SPI1, SPI_I2S_IT_RXNE) != RESET ) {
    u8 b;

    // get byte
    b = SPI_I2S_ReceiveData(SPI1);

    if( srio_irq_state != 0xff ) { // 0xff should never be reached here - just to ensure
      // notify changes
      mios32_srio_din_changed[srio_irq_state] |= mios32_srio_din[srio_irq_state] ^ b;

      // store new pin states
      mios32_srio_din[srio_irq_state] = b;

      // increment state
      ++srio_irq_state;

      // last byte received?
      if( srio_irq_state >= MIOS32_SRIO_NUM_SR ) {
	volatile s32 delay; // ensure, that delay won't be removed by compiler (depends on optimisation level)

	// notify that stream has been completely send
	srio_irq_state = 0xff;

	// disable SPI1 RXNE interrupt
	SPI_I2S_ITConfig(SPI1, SPI_I2S_IT_RXNE, DISABLE);

	// latch DOUT registers by pulsing RCLK: 1->0->1
	GPIO_WriteBit(GPIOA, GPIO_Pin_8, 0);
	// TODO: find a proper way to produce a delay with uS accuracy, vTaskDelay and vTaskDelayUntil only provide mS accuracy
	for(delay=0; delay<10; ++delay);
	GPIO_WriteBit(GPIOA, GPIO_Pin_8, 1);

	// call user specific hook if requested
	if( srio_scan_finished_hook != NULL )
	  srio_scan_finished_hook();

	// next transfer has to be started with MIOS32_SRIO_ScanStart
      } else {
	// loop while DR register in not empty
	while( SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE) == RESET );

	// send next byte
	// TODO: we could use DMA transfers for send/receive data later, and process
	// the incoming data once the stream has been sent to offload the CPU
	SPI_I2S_SendData(SPI1, mios32_srio_dout[MIOS32_SRIO_NUM_SR-srio_irq_state-1]);
      }
    }
  }
}

#endif /* MIOS32_DONT_USE_SRIO */

