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



/////////////////////////////////////////////////////////////////////////////
// Following local definitions depend on the selected SPI
/////////////////////////////////////////////////////////////////////////////

#if MIOS32_SRIO_SPI == 1

# define MIOS32_SRIO_RCLK_PORT  GPIOA
# define MIOS32_SRIO_RCLK_PIN   GPIO_Pin_4
# define MIOS32_SRIO_SCLK_PORT  GPIOA
# define MIOS32_SRIO_SCLK_PIN   GPIO_Pin_5
# define MIOS32_SRIO_DIN_PORT   GPIOA
# define MIOS32_SRIO_DIN_PIN    GPIO_Pin_6
# define MIOS32_SRIO_DOUT_PORT  GPIOA
# define MIOS32_SRIO_DOUT_PIN   GPIO_Pin_7

# define MIOS32_SRIO_SPI_PTR    SPI1
# define MIOS32_SRIO_DMA_RX_PTR DMA1_Channel2
# define MIOS32_SRIO_DMA_RX_IRQ_FLAGS (DMA1_FLAG_TC2 | DMA1_FLAG_TE2 | DMA1_FLAG_HT2 | DMA1_FLAG_GL2)
# define MIOS32_SRIO_DMA_TX_PTR DMA1_Channel3
# define MIOS32_SRIO_DMA_TX_IRQ_FLAGS (DMA1_FLAG_TC3 | DMA1_FLAG_TE3 | DMA1_FLAG_HT3 | DMA1_FLAG_GL3)
# define MIOS32_SRIO_DMA_IRQ_CHANNEL DMA1_Channel2_IRQChannel
# define MIOS32_SRIO_DMA_IRQHANDLER_FUNC void DMAChannel2_IRQHandler(void)

#elif MIOS32_SRIO_SPI == 2

# define MIOS32_SRIO_RCLK_PORT  GPIOB
# define MIOS32_SRIO_RCLK_PIN   GPIO_Pin_12
# define MIOS32_SRIO_SCLK_PORT  GPIOB
# define MIOS32_SRIO_SCLK_PIN   GPIO_Pin_13
# define MIOS32_SRIO_DIN_PORT   GPIOB
# define MIOS32_SRIO_DIN_PIN    GPIO_Pin_14
# define MIOS32_SRIO_DOUT_PORT  GPIOB
# define MIOS32_SRIO_DOUT_PIN   GPIO_Pin_15

# define MIOS32_SRIO_SPI_PTR    SPI2
# define MIOS32_SRIO_DMA_RX_PTR DMA1_Channel4
# define MIOS32_SRIO_DMA_RX_IRQ_FLAGS (DMA1_FLAG_TC4 | DMA1_FLAG_TE4 | DMA1_FLAG_HT4 | DMA1_FLAG_GL4)
# define MIOS32_SRIO_DMA_TX_PTR DMA1_Channel5
# define MIOS32_SRIO_DMA_TX_IRQ_FLAGS (DMA1_FLAG_TC5 | DMA1_FLAG_TE5 | DMA1_FLAG_HT5 | DMA1_FLAG_GL5)
# define MIOS32_SRIO_DMA_IRQ_CHANNEL DMA1_Channel4_IRQChannel
# define MIOS32_SRIO_DMA_IRQHANDLER_FUNC void DMAChannel4_IRQHandler(void)

#else
# error "Unsupported SPI peripheral number"
#endif


/////////////////////////////////////////////////////////////////////////////
// Help Macros
/////////////////////////////////////////////////////////////////////////////

#define PIN_RCLK_0  { MIOS32_SRIO_RCLK_PORT->BRR  = MIOS32_SRIO_RCLK_PIN; }
#define PIN_RCLK_1  { MIOS32_SRIO_RCLK_PORT->BSRR = MIOS32_SRIO_RCLK_PIN; }


/////////////////////////////////////////////////////////////////////////////
// Global variables
/////////////////////////////////////////////////////////////////////////////

// DOUT SR registers in reversed (!) order (since DMA doesn't provide a decrement address function)
// Note that also the bits are in reversed order compared to PIC based MIOS
// So long the array is accessed via MIOS32_DOUT_* functions, they programmer won't notice a difference!
volatile u8 mios32_srio_dout[MIOS32_SRIO_NUM_SR];

// DIN values of last scan
volatile u8 mios32_srio_din[MIOS32_SRIO_NUM_SR];

// DIN values of ongoing scan
// Note: during SRIO scan it is required to copy new DIN values into a temporary buffer
// to avoid that a task already takes a new DIN value before the whole chain has been scanned
// (e.g. relevant for encoder handler: it has to clear the changed flags, so that the DIN handler doesn't take the value)
volatile u8 mios32_srio_din_buffer[MIOS32_SRIO_NUM_SR];

// change notification flags
volatile u8 mios32_srio_din_changed[MIOS32_SRIO_NUM_SR];

// transfer state
volatile u8 srio_values_transfered;


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

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
  DMA_InitTypeDef  DMA_InitStructure;


  // disable notification hook
  srio_scan_finished_hook = NULL;

  // currently only mode 0 supported
  if( mode != 0 )
    return -1; // unsupported mode

  // clear chains
  // will be done again in MIOS32_DIN_Init and MIOS32_DOUT_Init
  // we don't reference to these functions here to allow the programmer to remove/replace these driver modules)
  for(i=0; i<MIOS32_SRIO_NUM_SR; ++i) {
    mios32_srio_dout[i] = 0x00;       // passive state (LEDs off)
    mios32_srio_din[i] = 0xff;        // passive state (Buttons depressed)
    mios32_srio_din_buffer[i] = 0xff; // passive state (Buttons depressed)
    mios32_srio_din_changed[i] = 0;   // no change
  }

  // init GPIO structure
  GPIO_StructInit(&GPIO_InitStructure);
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;

  // SCLK and DOUT are outputs assigned to alternate functions
  GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF_PP;
  GPIO_InitStructure.GPIO_Pin   = MIOS32_SRIO_SCLK_PIN;
  GPIO_Init(MIOS32_SRIO_SCLK_PORT, &GPIO_InitStructure);
  GPIO_InitStructure.GPIO_Pin   = MIOS32_SRIO_DOUT_PIN;
  GPIO_Init(MIOS32_SRIO_DOUT_PORT, &GPIO_InitStructure);

  // RCLK is outputs assigned to GPIO
  GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;
  GPIO_InitStructure.GPIO_Pin   = MIOS32_SRIO_RCLK_PIN;
  GPIO_Init(MIOS32_SRIO_RCLK_PORT, &GPIO_InitStructure);

  // DIN is input with pull-up
  GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IPU;
  GPIO_InitStructure.GPIO_Pin   = MIOS32_SRIO_DIN_PIN;
  GPIO_Init(MIOS32_SRIO_DIN_PORT, &GPIO_InitStructure);

  // enable SPI peripheral clock (APB2 == high speed)
#if MIOS32_SRIO_SPI == 1
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, ENABLE);
#elif MIOS32_SRIO_SPI == 2
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI2, ENABLE);
#endif

  // SPI configuration
  SPI_StructInit(&SPI_InitStructure);
  SPI_InitStructure.SPI_Direction           = SPI_Direction_2Lines_FullDuplex;
  SPI_InitStructure.SPI_Mode                = SPI_Mode_Master;
  SPI_InitStructure.SPI_DataSize            = SPI_DataSize_8b;
  SPI_InitStructure.SPI_CPOL                = SPI_CPOL_Low;
  SPI_InitStructure.SPI_CPHA                = SPI_CPHA_1Edge;
  SPI_InitStructure.SPI_NSS                 = SPI_NSS_Soft;
  SPI_InitStructure.SPI_BaudRatePrescaler   = SPI_BaudRatePrescaler_128; // ca. 1 uS period
  SPI_InitStructure.SPI_FirstBit            = SPI_FirstBit_MSB;
  SPI_InitStructure.SPI_CRCPolynomial       = 7;
  SPI_Init(MIOS32_SRIO_SPI_PTR, &SPI_InitStructure);

  // enable SPI
  SPI_Cmd(MIOS32_SRIO_SPI_PTR, ENABLE);

  // initial state of RCLK
  PIN_RCLK_1;


  // enable DMA1 clock
  RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);

  // DMA Configuration for SPI Rx Event
  DMA_StructInit(&DMA_InitStructure);
  DMA_ClearFlag(MIOS32_SRIO_DMA_RX_IRQ_FLAGS);
  DMA_Cmd(MIOS32_SRIO_DMA_RX_PTR, DISABLE);
  DMA_InitStructure.DMA_PeripheralBaseAddr = (u32)&MIOS32_SRIO_SPI_PTR->DR;
  DMA_InitStructure.DMA_MemoryBaseAddr = (u32)&mios32_srio_din_buffer;
  DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;
  DMA_InitStructure.DMA_BufferSize = MIOS32_SRIO_NUM_SR;
  DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
  DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
  DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
  DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
  DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
  DMA_InitStructure.DMA_Priority = DMA_Priority_Medium;
  DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
  DMA_Init(MIOS32_SRIO_DMA_RX_PTR, &DMA_InitStructure);
  DMA_Cmd(MIOS32_SRIO_DMA_RX_PTR, ENABLE);

  // trigger interrupt when transfer complete
  DMA_ITConfig(MIOS32_SRIO_DMA_RX_PTR, DMA_IT_TC, ENABLE);

  // DMA Configuration for SPI Tx Event
  // (partly re-using previous DMA setup)
  DMA_ClearFlag(MIOS32_SRIO_DMA_TX_IRQ_FLAGS);
  DMA_Cmd(MIOS32_SRIO_DMA_TX_PTR, DISABLE);
  DMA_InitStructure.DMA_MemoryBaseAddr = (u32)&mios32_srio_dout;
  DMA_InitStructure.DMA_BufferSize = MIOS32_SRIO_NUM_SR;
  DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralDST;
  DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
  DMA_Init(MIOS32_SRIO_DMA_TX_PTR, &DMA_InitStructure);
  DMA_Cmd(MIOS32_SRIO_DMA_TX_PTR, ENABLE);

  // enable SPI interrupts to DMA
  SPI_I2S_DMACmd(MIOS32_SRIO_SPI_PTR, SPI_I2S_DMAReq_Tx | SPI_I2S_DMAReq_Rx, ENABLE);

  // Configure and enable DMA interrupt
  NVIC_StructInit(&NVIC_InitStructure);
  NVIC_InitStructure.NVIC_IRQChannel = MIOS32_SRIO_DMA_IRQ_CHANNEL;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);

  // notify that SRIO values have been transfered
  // (cleared on each ScanStart, set on each DMA IRQ invokation for proper synchronisation)
  srio_values_transfered = 1;

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
  // THIS IS A FAILSAVE MEASURE ONLY!
  // should never happen if MIOS32_SRIO_ScanStart is called each mS
  // the transfer itself takes ca. 225 uS (if 16 SRIOs are scanned)
  if( !srio_values_transfered )
    return -2; // notify this special scenario - we could retry here

  // notify that new values have to be transfered
  srio_values_transfered = 0;

  // change notification function
  srio_scan_finished_hook = _notify_hook;

  // before first byte will be sent:
  // latch DIN registers by pulsing RCLK: 1->0->1
  // TODO: find a proper way to produce a delay with uS accuracy, vTaskDelay and vTaskDelayUntil only provide mS accuracy
  // TODO: wait longer for touch sensors
  // TODO: maybe we should disable all IRQs here for higher accuracy
  for(delay=0; delay<5; ++delay) PIN_RCLK_0;
  PIN_RCLK_1;

  // reload DMA byte counters
  DMA_Cmd(MIOS32_SRIO_DMA_RX_PTR, DISABLE);
  MIOS32_SRIO_DMA_RX_PTR->CNDTR = MIOS32_SRIO_NUM_SR;
  DMA_Cmd(MIOS32_SRIO_DMA_RX_PTR, ENABLE);

  DMA_Cmd(MIOS32_SRIO_DMA_TX_PTR, DISABLE);
  MIOS32_SRIO_DMA_TX_PTR->CNDTR = MIOS32_SRIO_NUM_SR;
  DMA_Cmd(MIOS32_SRIO_DMA_TX_PTR, ENABLE);

  // transfer will start now ("empty buffer" event already active)

  return 0;
}


/////////////////////////////////////////////////////////////////////////////
// DMA1 Channel interrupt is triggered once the complete SRIO chain
// has been scanned
/////////////////////////////////////////////////////////////////////////////
MIOS32_SRIO_DMA_IRQHANDLER_FUNC
{
  s32 i;
  volatile s32 delay; // ensure, that delay won't be removed by compiler (depends on optimisation level)

  // clear the pending flag(s)
  DMA_ClearFlag(MIOS32_SRIO_DMA_RX_IRQ_FLAGS);

  // notify that new values have been transfered
  srio_values_transfered = 1;

  // latch DOUT registers by pulsing RCLK: 1->0->1
  // TODO: find a proper way to produce a delay with uS accuracy, vTaskDelay and vTaskDelayUntil only provide mS accuracy
  for(delay=0; delay<5; ++delay) PIN_RCLK_0;
  PIN_RCLK_1;

  // copy/or buffered DIN values/changed flags
  for(i=0; i<MIOS32_SRIO_NUM_SR; ++i) {
    mios32_srio_din_changed[i] |= mios32_srio_din[i] ^ mios32_srio_din_buffer[i];
    mios32_srio_din[i] = mios32_srio_din_buffer[i];
  }

  // call user specific hook if requested
  if( srio_scan_finished_hook != NULL )
    srio_scan_finished_hook();

  // next transfer has to be started with MIOS32_SRIO_ScanStart
}

#endif /* MIOS32_DONT_USE_SRIO */

