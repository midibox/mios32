// $Id$
//! \defgroup MIOS32_SPI
//!
//! Hardware Abstraction Layer for SPI ports of STM32
//!
//! Three ports are provided at J16 (SPI0), J8/9 (SPI1) and J19 (SPI2) of the 
//! MBHP_CORE_STM32 module..
//!
//! J16 provides two RCLK (alias Chip Select) lines, and is normaly
//! connected to a SD Card or/and an Ethernet Interface like ENC28J60.
//!
//! J8/9 only supports a single RCLK line, and is normaly connected to
//! the SRIO chain to scan DIN/DOUT modules.
//!
//! J19 provides two RCLK (alias Chip Select) lines.<BR>
//! It's a software emulated SPI, therefore the selected spi_prescaler has no
//! effect! Bytes are transfered so fast as possible. The usage of
//! MIOS32_SPI_PIN_DRIVER_STRONG is strongly recommented ;)<BR>
//! DMA transfers are not supported by the emulation, so that 
//! MIOS32_SPI_TransferBlock() will consume CPU time (but the callback handling does work).
//!
//! If SPI low-level functions should be used to access other peripherals,
//! please ensure that the appr. MIOS32_* drivers are disabled (e.g.
//! add '#define MIOS32_DONT_USE_SDCARD' and '#define MIOS32_DONT_USE_ENC28J60'
//! to your mios_config.h file)
//!
//! Note that additional chip select lines can be easily added by using
//! the remaining free GPIOs of the core module. Shared SPI ports should be
//! arbitrated with (FreeRTOS based) Mutexes to avoid collisions!
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

// this module can be optionally disabled in a local mios32_config.h file (included from mios32.h)
#if !defined(MIOS32_DONT_USE_SPI)


/////////////////////////////////////////////////////////////////////////////
// SPI Pin definitions
// (not part of mios32_spi.h file, since overruling would lead to a hardware
// dependency in MIOS32 applications)
/////////////////////////////////////////////////////////////////////////////

#define MIOS32_SPI0_PTR        SPI1
#define MIOS32_SPI0_DMA_RX_PTR DMA1_Channel2
#define MIOS32_SPI0_DMA_TX_PTR DMA1_Channel3
#define MIOS32_SPI0_DMA_RX_IRQ_FLAGS (DMA1_FLAG_TC2 | DMA1_FLAG_TE2 | DMA1_FLAG_HT2 | DMA1_FLAG_GL2)
#define MIOS32_SPI0_DMA_IRQ_CHANNEL DMA1_Channel2_IRQChannel
#define MIOS32_SPI0_DMA_IRQHANDLER_FUNC void DMAChannel2_IRQHandler(void)

#define MIOS32_SPI0_RCLK1_PORT GPIOA
#define MIOS32_SPI0_RCLK1_PIN  GPIO_Pin_4
#define MIOS32_SPI0_RCLK2_PORT GPIOC
#define MIOS32_SPI0_RCLK2_PIN  GPIO_Pin_15
#define MIOS32_SPI0_SCLK_PORT  GPIOA
#define MIOS32_SPI0_SCLK_PIN   GPIO_Pin_5
#define MIOS32_SPI0_MISO_PORT  GPIOA
#define MIOS32_SPI0_MISO_PIN   GPIO_Pin_6
#define MIOS32_SPI0_MOSI_PORT  GPIOA
#define MIOS32_SPI0_MOSI_PIN   GPIO_Pin_7


#define MIOS32_SPI1_PTR        SPI2
#define MIOS32_SPI1_DMA_RX_PTR DMA1_Channel4
#define MIOS32_SPI1_DMA_TX_PTR DMA1_Channel5
#define MIOS32_SPI1_DMA_RX_IRQ_FLAGS (DMA1_FLAG_TC4 | DMA1_FLAG_TE4 | DMA1_FLAG_HT4 | DMA1_FLAG_GL4)
#define MIOS32_SPI1_DMA_IRQ_CHANNEL DMA1_Channel4_IRQChannel
#define MIOS32_SPI1_DMA_IRQHANDLER_FUNC void DMAChannel4_IRQHandler(void)

#define MIOS32_SPI1_RCLK1_PORT GPIOB
#define MIOS32_SPI1_RCLK1_PIN  GPIO_Pin_12
#define MIOS32_SPI1_SCLK_PORT  GPIOB
#define MIOS32_SPI1_SCLK_PIN   GPIO_Pin_13
#define MIOS32_SPI1_MISO_PORT  GPIOB
#define MIOS32_SPI1_MISO_PIN   GPIO_Pin_14
#define MIOS32_SPI1_MOSI_PORT  GPIOB
#define MIOS32_SPI1_MOSI_PIN   GPIO_Pin_15


#define MIOS32_SPI2_PTR        NULL
#define MIOS32_SPI2_DMA_RX_PTR NULL
#define MIOS32_SPI2_DMA_TX_PTR NULL
#define MIOS32_SPI2_DMA_RX_IRQ_FLAGS 0
#define MIOS32_SPI2_DMA_IRQ_CHANNEL NULL
#define MIOS32_SPI2_DMA_IRQHANDLER_FUNC NULL

#define MIOS32_SPI2_RCLK1_PORT GPIOC
#define MIOS32_SPI2_RCLK1_PIN  GPIO_Pin_13
#define MIOS32_SPI2_RCLK2_PORT GPIOC
#define MIOS32_SPI2_RCLK2_PIN  GPIO_Pin_14
#define MIOS32_SPI2_SCLK_PORT  GPIOB
#define MIOS32_SPI2_SCLK_PIN   GPIO_Pin_6
#define MIOS32_SPI2_MISO_PORT  GPIOB
#define MIOS32_SPI2_MISO_PIN   GPIO_Pin_7
#define MIOS32_SPI2_MOSI_PORT  GPIOB
#define MIOS32_SPI2_MOSI_PIN   GPIO_Pin_5

// help macros for SW emulation
#define MIOS32_SPI2_SET_MOSI(b) { MIOS32_SPI2_MOSI_PORT->BSRR = (b) ? MIOS32_SPI2_MOSI_PIN : (MIOS32_SPI2_MOSI_PIN << 16); }
#define MIOS32_SPI2_GET_MISO    ( MIOS32_SPI2_MISO_PORT->IDR & MIOS32_SPI2_MISO_PIN )
#define MIOS32_SPI2_SET_SCLK_0  { MIOS32_SPI2_SCLK_PORT->BRR  = MIOS32_SPI2_SCLK_PIN; }
#define MIOS32_SPI2_SET_SCLK_1  { MIOS32_SPI2_SCLK_PORT->BSRR = MIOS32_SPI2_SCLK_PIN; }


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

static void (*spi_callback[3])(void);

static u8 tx_dummy_byte;
static u8 rx_dummy_byte;

static mios32_spi_mode_t sw_spi2_mode;


/////////////////////////////////////////////////////////////////////////////
// Local prototypes
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
//! Initializes SPI pins
//! \param[in] mode currently only mode 0 supported
//! \return < 0 if initialisation failed
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_SPI_Init(u32 mode)
{
  // currently only mode 0 supported
  if( mode != 0 )
    return -1; // unsupported mode

  DMA_InitTypeDef DMA_InitStructure;
  DMA_StructInit(&DMA_InitStructure);
  NVIC_InitTypeDef NVIC_InitStructure;
  NVIC_StructInit(&NVIC_InitStructure);

  ///////////////////////////////////////////////////////////////////////////
  // SPI0
  ///////////////////////////////////////////////////////////////////////////
#ifndef MIOS32_DONT_USE_SPI0

  // disable callback function
  spi_callback[0] = NULL;

  // set RC pin(s) to 1
  MIOS32_SPI_RC_PinSet(0, 0, 1); // spi, rc_pin, pin_value
  MIOS32_SPI_RC_PinSet(0, 1, 1); // spi, rc_pin, pin_value

  // IO configuration
  MIOS32_SPI_IO_Init(0, MIOS32_SPI_PIN_DRIVER_WEAK_OD);

  // enable SPI peripheral clock (APB2 == high speed)
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, ENABLE);

  // enable DMA1 clock
  RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);

  // DMA Configuration for SPI Rx Event
  DMA_Cmd(MIOS32_SPI0_DMA_RX_PTR, DISABLE);
  DMA_InitStructure.DMA_PeripheralBaseAddr = (u32)&MIOS32_SPI0_PTR->DR;
  DMA_InitStructure.DMA_MemoryBaseAddr = 0; // will be configured later
  DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;
  DMA_InitStructure.DMA_BufferSize = 0; // will be configured later
  DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
  DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
  DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
  DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
  DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
  DMA_InitStructure.DMA_Priority = DMA_Priority_Medium;
  DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
  DMA_Init(MIOS32_SPI0_DMA_RX_PTR, &DMA_InitStructure);

  // DMA Configuration for SPI Tx Event
  // (partly re-using previous DMA setup)
  DMA_Cmd(MIOS32_SPI0_DMA_TX_PTR, DISABLE);
  DMA_InitStructure.DMA_MemoryBaseAddr = 0; // will be configured later
  DMA_InitStructure.DMA_BufferSize = 0; // will be configured later
  DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralDST;
  DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
  DMA_Init(MIOS32_SPI0_DMA_TX_PTR, &DMA_InitStructure);

  // initial SPI peripheral configuration
  MIOS32_SPI_TransferModeInit(0, MIOS32_SPI_MODE_CLK1_PHASE1, MIOS32_SPI_PRESCALER_128);

  // enable SPI
  SPI_Cmd(MIOS32_SPI0_PTR, ENABLE);

  // enable SPI interrupts to DMA
  SPI_I2S_DMACmd(MIOS32_SPI0_PTR, SPI_I2S_DMAReq_Tx | SPI_I2S_DMAReq_Rx, ENABLE);

  // Configure DMA interrupt
  NVIC_InitStructure.NVIC_IRQChannel = MIOS32_SPI0_DMA_IRQ_CHANNEL;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = MIOS32_IRQ_SPI_DMA_PRIORITY; // defined in mios32_irq.h
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);

#endif /* MIOS32_DONT_USE_SPI0 */


  ///////////////////////////////////////////////////////////////////////////
  // SPI1
  ///////////////////////////////////////////////////////////////////////////
#ifndef MIOS32_DONT_USE_SPI1

  // disable callback function
  spi_callback[1] = NULL;

  // set RC pin(s) to 1
  MIOS32_SPI_RC_PinSet(1, 0, 1); // spi, rc_pin, pin_value
  MIOS32_SPI_RC_PinSet(1, 1, 1); // spi, rc_pin, pin_value

  // IO configuration
  MIOS32_SPI_IO_Init(1, MIOS32_SPI_PIN_DRIVER_WEAK_OD);

  // enable SPI peripheral clock (APB1 == slow speed)
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI2, ENABLE);

  // enable DMA1 clock
  RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);

  // DMA Configuration for SPI Rx Event
  DMA_Cmd(MIOS32_SPI1_DMA_RX_PTR, DISABLE);
  DMA_InitStructure.DMA_PeripheralBaseAddr = (u32)&MIOS32_SPI1_PTR->DR;
  DMA_InitStructure.DMA_MemoryBaseAddr = 0; // will be configured later
  DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;
  DMA_InitStructure.DMA_BufferSize = 0; // will be configured later
  DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
  DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
  DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
  DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
  DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
  DMA_InitStructure.DMA_Priority = DMA_Priority_Medium;
  DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
  DMA_Init(MIOS32_SPI1_DMA_RX_PTR, &DMA_InitStructure);

  // DMA Configuration for SPI Tx Event
  // (partly re-using previous DMA setup)
  DMA_Cmd(MIOS32_SPI1_DMA_TX_PTR, DISABLE);
  DMA_InitStructure.DMA_MemoryBaseAddr = 0; // will be configured later
  DMA_InitStructure.DMA_BufferSize = 0; // will be configured later
  DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralDST;
  DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
  DMA_Init(MIOS32_SPI1_DMA_TX_PTR, &DMA_InitStructure);

  // initial SPI peripheral configuration
  MIOS32_SPI_TransferModeInit(1, MIOS32_SPI_MODE_CLK1_PHASE1, MIOS32_SPI_PRESCALER_128);

  // enable SPI
  SPI_Cmd(MIOS32_SPI1_PTR, ENABLE);

  // enable SPI interrupts to DMA
  SPI_I2S_DMACmd(MIOS32_SPI1_PTR, SPI_I2S_DMAReq_Tx | SPI_I2S_DMAReq_Rx, ENABLE);

  // Configure DMA interrupt
  NVIC_InitStructure.NVIC_IRQChannel = MIOS32_SPI1_DMA_IRQ_CHANNEL;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = MIOS32_IRQ_SPI_DMA_PRIORITY; // defined in mios32_irq.h
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);

#endif /* MIOS32_DONT_USE_SPI1 */


  ///////////////////////////////////////////////////////////////////////////
  // SPI2 (software emulated)
  ///////////////////////////////////////////////////////////////////////////
#ifndef MIOS32_DONT_USE_SPI2

  // disable callback function
  spi_callback[2] = NULL;

  // set RC pin(s) to 1
  MIOS32_SPI_RC_PinSet(2, 0, 1); // spi, rc_pin, pin_value
  MIOS32_SPI_RC_PinSet(2, 1, 1); // spi, rc_pin, pin_value

  // IO configuration
  MIOS32_SPI_IO_Init(2, MIOS32_SPI_PIN_DRIVER_WEAK_OD);

  // initial SPI peripheral configuration
  MIOS32_SPI_TransferModeInit(2, MIOS32_SPI_MODE_CLK1_PHASE1, MIOS32_SPI_PRESCALER_128);

#endif /* MIOS32_DONT_USE_SPI2 */

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! (Re-)initializes SPI IO Pins
//! By default, all output pins are configured with weak open drain drivers for 2 MHz
//! \param[in] spi SPI number (0, 1 or 2)
//! \param[in] spi_pin_driver configures the driver strength:
//! <UL>
//!   <LI>MIOS32_SPI_PIN_DRIVER_STRONG: configures outputs for up to 50 MHz
//!   <LI>MIOS32_SPI_PIN_DRIVER_STRONG_OD: configures outputs as open drain 
//!       for up to 50 MHz (allows voltage shifting via pull-resistors)
//!   <LI>MIOS32_SPI_PIN_DRIVER_WEAK: configures outputs for up to 2 MHz (better EMC)
//!   <LI>MIOS32_SPI_PIN_DRIVER_WEAK_OD: configures outputs as open drain for 
//!       up to 2 MHz (allows voltage shifting via pull-resistors)
//! </UL>
//! \return 0 if no error
//! \return -1 if disabled SPI port selected
//! \return -2 if unsupported SPI port selected
//! \return -3 if unsupported pin driver mode
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_SPI_IO_Init(u8 spi, mios32_spi_pin_driver_t spi_pin_driver)
{
  // init GPIO structure
  GPIO_InitTypeDef GPIO_InitStructure;
  GPIO_StructInit(&GPIO_InitStructure);

  // select pin driver and output mode
  u32 af_mode;
  u32 gp_mode;

  switch( spi_pin_driver ) {
    case MIOS32_SPI_PIN_DRIVER_STRONG:
      GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
      af_mode = GPIO_Mode_AF_PP;
      gp_mode = GPIO_Mode_Out_PP;
      break;

    case MIOS32_SPI_PIN_DRIVER_STRONG_OD:
      GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
      af_mode = GPIO_Mode_AF_OD;
      gp_mode = GPIO_Mode_Out_OD;
      break;

    case MIOS32_SPI_PIN_DRIVER_WEAK:
      GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
      af_mode = GPIO_Mode_AF_PP;
      gp_mode = GPIO_Mode_Out_PP;
      break;

    case MIOS32_SPI_PIN_DRIVER_WEAK_OD:
      GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
      af_mode = GPIO_Mode_AF_OD;
      gp_mode = GPIO_Mode_Out_OD;
      break;

    default:
      return -3; // unsupported pin driver mode
  }

  switch( spi ) {
    case 0:
#ifdef MIOS32_DONT_USE_SPI0
      return -1; // disabled SPI port
#else
      // SCLK and DOUT are outputs assigned to alternate functions
      GPIO_InitStructure.GPIO_Mode = af_mode;
      GPIO_InitStructure.GPIO_Pin  = MIOS32_SPI0_SCLK_PIN;
      GPIO_Init(MIOS32_SPI0_SCLK_PORT, &GPIO_InitStructure);
      GPIO_InitStructure.GPIO_Pin  = MIOS32_SPI0_MOSI_PIN;
      GPIO_Init(MIOS32_SPI0_MOSI_PORT, &GPIO_InitStructure);
    
      // RCLK outputs assigned to GPIO
      GPIO_InitStructure.GPIO_Mode = gp_mode;
      GPIO_InitStructure.GPIO_Pin  = MIOS32_SPI0_RCLK1_PIN;
      GPIO_Init(MIOS32_SPI0_RCLK1_PORT, &GPIO_InitStructure);
      GPIO_InitStructure.GPIO_Pin  = MIOS32_SPI0_RCLK2_PIN;
      GPIO_Init(MIOS32_SPI0_RCLK2_PORT, &GPIO_InitStructure);
    
      // DIN is input with pull-up
      GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
      GPIO_InitStructure.GPIO_Pin  = MIOS32_SPI0_MISO_PIN;
      GPIO_Init(MIOS32_SPI0_MISO_PORT, &GPIO_InitStructure);
      break;
#endif

    case 1:
#ifdef MIOS32_DONT_USE_SPI1
      return -1; // disabled SPI port
#else
      // SCLK and DOUT are outputs assigned to alternate functions
      GPIO_InitStructure.GPIO_Mode = af_mode;
      GPIO_InitStructure.GPIO_Pin  = MIOS32_SPI1_SCLK_PIN;
      GPIO_Init(MIOS32_SPI1_SCLK_PORT, &GPIO_InitStructure);
      GPIO_InitStructure.GPIO_Pin  = MIOS32_SPI1_MOSI_PIN;
      GPIO_Init(MIOS32_SPI1_MOSI_PORT, &GPIO_InitStructure);
    
      // RCLK outputs assigned to GPIO
      GPIO_InitStructure.GPIO_Mode = gp_mode;
      GPIO_InitStructure.GPIO_Pin  = MIOS32_SPI1_RCLK1_PIN;
      GPIO_Init(MIOS32_SPI1_RCLK1_PORT, &GPIO_InitStructure);
#if 0
      // not available for J8/9
      GPIO_InitStructure.GPIO_Pin  = MIOS32_SPI1_RCLK2_PIN;
      GPIO_Init(MIOS32_SPI1_RCLK2_PORT, &GPIO_InitStructure);
#endif
    
      // DIN is input with pull-up
      GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
      GPIO_InitStructure.GPIO_Pin  = MIOS32_SPI1_MISO_PIN;
      GPIO_Init(MIOS32_SPI1_MISO_PORT, &GPIO_InitStructure);

      break;
#endif

    case 2:
#ifdef MIOS32_DONT_USE_SPI2
      return -1; // disabled SPI port
#else
      // RCLK, SCLK and DOUT assigned to GPIO (due to SW emulation)
      GPIO_InitStructure.GPIO_Mode = gp_mode;
      GPIO_InitStructure.GPIO_Pin  = MIOS32_SPI2_SCLK_PIN;
      GPIO_Init(MIOS32_SPI2_SCLK_PORT, &GPIO_InitStructure);
      GPIO_InitStructure.GPIO_Pin  = MIOS32_SPI2_MOSI_PIN;
      GPIO_Init(MIOS32_SPI2_MOSI_PORT, &GPIO_InitStructure);
      GPIO_InitStructure.GPIO_Pin  = MIOS32_SPI2_RCLK1_PIN;
      GPIO_Init(MIOS32_SPI2_RCLK1_PORT, &GPIO_InitStructure);
      GPIO_InitStructure.GPIO_Pin  = MIOS32_SPI2_RCLK2_PIN;
      GPIO_Init(MIOS32_SPI2_RCLK2_PORT, &GPIO_InitStructure);
    
      // DIN is input with pull-up
      GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
      GPIO_InitStructure.GPIO_Pin  = MIOS32_SPI2_MISO_PIN;
      GPIO_Init(MIOS32_SPI2_MISO_PORT, &GPIO_InitStructure);

      break;
#endif

    default:
      return -2; // unsupported SPI port
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! (Re-)initializes SPI peripheral transfer mode
//! By default, all SPI peripherals are configured with 
//! MIOS32_SPI_MODE_CLK1_PHASE1 and MIOS32_SPI_PRESCALER_128
//!
//! \param[in] spi SPI number (0, 1 or 2)
//! \param[in] spi_mode configures clock and capture phase:
//! <UL>
//!   <LI>MIOS32_SPI_MODE_CLK0_PHASE0: Idle level of clock is 0, data captured at rising edge
//!   <LI>MIOS32_SPI_MODE_CLK0_PHASE1: Idle level of clock is 0, data captured at falling edge
//!   <LI>MIOS32_SPI_MODE_CLK1_PHASE0: Idle level of clock is 1, data captured at falling edge
//!   <LI>MIOS32_SPI_MODE_CLK1_PHASE1: Idle level of clock is 1, data captured at rising edge
//! </UL>
//! \param[in] spi_prescaler configures the SPI speed:
//! <UL>
//!   <LI>MIOS32_SPI_PRESCALER_2: sets clock rate 27.7~ nS @ 72 MHz (36 MBit/s) (only supported for spi==0, spi1 uses 4 instead)
//!   <LI>MIOS32_SPI_PRESCALER_4: sets clock rate 55.5~ nS @ 72 MHz (18 MBit/s)
//!   <LI>MIOS32_SPI_PRESCALER_8: sets clock rate 111.1~ nS @ 72 MHz (9 MBit/s)
//!   <LI>MIOS32_SPI_PRESCALER_16: sets clock rate 222.2~ nS @ 72 MHz (4.5 MBit/s)
//!   <LI>MIOS32_SPI_PRESCALER_32: sets clock rate 444.4~ nS @ 72 MHz (2.25 MBit/s)
//!   <LI>MIOS32_SPI_PRESCALER_64: sets clock rate 888.8~ nS @ 72 MHz (1.125 MBit/s)
//!   <LI>MIOS32_SPI_PRESCALER_128: sets clock rate 1.7~ nS @ 72 MHz (0.562 MBit/s)
//!   <LI>MIOS32_SPI_PRESCALER_256: sets clock rate 3.5~ nS @ 72 MHz (0.281 MBit/s)
//! </UL>
//! \return 0 if no error
//! \return -1 if disabled SPI port selected
//! \return -2 if unsupported SPI port selected
//! \return -3 if invalid spi_prescaler selected
//! \return -4 if invalid spi_mode selected
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_SPI_TransferModeInit(u8 spi, mios32_spi_mode_t spi_mode, mios32_spi_prescaler_t spi_prescaler)
{
  // SPI configuration
  SPI_InitTypeDef SPI_InitStructure;
  SPI_InitStructure.SPI_Direction     = SPI_Direction_2Lines_FullDuplex;
  SPI_InitStructure.SPI_Mode          = SPI_Mode_Master;
  SPI_InitStructure.SPI_DataSize      = SPI_DataSize_8b;
  SPI_InitStructure.SPI_NSS           = SPI_NSS_Soft;
  SPI_InitStructure.SPI_FirstBit      = SPI_FirstBit_MSB;
  SPI_InitStructure.SPI_CRCPolynomial = 7;

  switch( spi_mode ) {
    case MIOS32_SPI_MODE_CLK0_PHASE0:
      SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;
      SPI_InitStructure.SPI_CPHA = SPI_CPHA_1Edge;
      break;
    case MIOS32_SPI_MODE_CLK0_PHASE1:
      SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;
      SPI_InitStructure.SPI_CPHA = SPI_CPHA_2Edge;
      break;
    case MIOS32_SPI_MODE_CLK1_PHASE0:
      SPI_InitStructure.SPI_CPOL = SPI_CPOL_High;
      SPI_InitStructure.SPI_CPHA = SPI_CPHA_1Edge;
      break;
    case MIOS32_SPI_MODE_CLK1_PHASE1:
      SPI_InitStructure.SPI_CPOL = SPI_CPOL_High;
      SPI_InitStructure.SPI_CPHA = SPI_CPHA_2Edge;
      break;
    default:
      return -4; // invalid SPI clock/phase mode
  }

  if( spi_prescaler >= 8 )
    return -3; // invalid prescaler selected

  switch( spi ) {
    case 0:
#ifdef MIOS32_DONT_USE_SPI0
      return -1; // disabled SPI port
#else
      // SPI1 perpipheral is located in APB2 domain and clocked at full speed
      // therefore we have to add +1 to the prescaler
      SPI_InitStructure.SPI_BaudRatePrescaler = ((u16)spi_prescaler&7) << 3;
      SPI_Init(MIOS32_SPI0_PTR, &SPI_InitStructure);
      break;
#endif

    case 1:
#ifdef MIOS32_DONT_USE_SPI1
      return -1; // disabled SPI port
#else
      // SPI2 perpipheral is located in APB1 domain and clocked at half speed
      if( spi_prescaler == 0 )
	SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_2;
      else
	SPI_InitStructure.SPI_BaudRatePrescaler = (((u16)spi_prescaler&7)-1) << 3;
      SPI_Init(MIOS32_SPI1_PTR, &SPI_InitStructure);
      break;
#endif

    case 2:
#ifdef MIOS32_DONT_USE_SPI2
      return -1; // disabled SPI port
#else
      // no clock prescaler for SW emulated SPI
      // remember mode settings
      sw_spi2_mode = spi_mode;

      // set clock idle level
      switch( sw_spi2_mode ) {
        case MIOS32_SPI_MODE_CLK0_PHASE0:
        case MIOS32_SPI_MODE_CLK0_PHASE1:
	  MIOS32_SPI2_SET_SCLK_0;
	  break;
        case MIOS32_SPI_MODE_CLK1_PHASE0:
        case MIOS32_SPI_MODE_CLK1_PHASE1:
	  MIOS32_SPI2_SET_SCLK_1;
	  break;
        default:
	  return -4; // invalid SPI clock/phase mode
      }

      break;
#endif

    default:
      return -2; // unsupported SPI port
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! Controls the RC (Register Clock alias Chip Select) pin of a SPI port
//! \param[in] spi SPI number (0, 1 or 2)
//! \param[in] rc_pin RCLK pin (0 or 1 for RCLK1 or RCLK2)
//! \param[in] pin_value 0 or 1
//! \return 0 if no error
//! \return -1 if disabled SPI port selected
//! \return -2 if unsupported SPI port selected
//! \return -3 if unsupported RCx pin selected
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_SPI_RC_PinSet(u8 spi, u8 rc_pin, u8 pin_value)
{
  switch( spi ) {
    case 0:
#ifdef MIOS32_DONT_USE_SPI0
      return -1; // disabled SPI port
#else
      switch( rc_pin ) {
        case 0:
	  if( pin_value )
	    MIOS32_SPI0_RCLK1_PORT->BSRR = MIOS32_SPI0_RCLK1_PIN;
	  else
	    MIOS32_SPI0_RCLK1_PORT->BRR  = MIOS32_SPI0_RCLK1_PIN;
	  break;

        case 1:
	  if( pin_value )
	    MIOS32_SPI0_RCLK2_PORT->BSRR = MIOS32_SPI0_RCLK2_PIN;
	  else
	    MIOS32_SPI0_RCLK2_PORT->BRR  = MIOS32_SPI0_RCLK2_PIN;
	  break;

        default:
	  return -3; // unsupported RC pin
      }
      break;
#endif

    case 1:
#ifdef MIOS32_DONT_USE_SPI1
      return -1; // disabled SPI port
#else
      switch( rc_pin ) {
        case 0:
	  if( pin_value )
	    MIOS32_SPI1_RCLK1_PORT->BSRR = MIOS32_SPI1_RCLK1_PIN;
	  else
	    MIOS32_SPI1_RCLK1_PORT->BRR  = MIOS32_SPI1_RCLK1_PIN;
	  break;

        default:
	  return -3; // unsupported RC pin
      }
      break;
#endif

    case 2:
#ifdef MIOS32_DONT_USE_SPI2
      return -1; // disabled SPI port
#else
      switch( rc_pin ) {
        case 0:
	  if( pin_value )
	    MIOS32_SPI2_RCLK1_PORT->BSRR = MIOS32_SPI2_RCLK1_PIN;
	  else
	    MIOS32_SPI2_RCLK1_PORT->BRR  = MIOS32_SPI2_RCLK1_PIN;
	  break;

        case 1:
	  if( pin_value )
	    MIOS32_SPI2_RCLK2_PORT->BSRR = MIOS32_SPI2_RCLK2_PIN;
	  else
	    MIOS32_SPI2_RCLK2_PORT->BRR  = MIOS32_SPI2_RCLK2_PIN;
	  break;

        default:
	  return -3; // unsupported RC pin
      }
      break;
#endif

    default:
      return -2; // unsupported SPI port
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! Transfers a byte to SPI output and reads back the return value from SPI input
//! \param[in] spi SPI number (0, 1 or 2)
//! \param[in] b the byte which should be transfered
//! \return >= 0: the read byte
//! \return -1 if disabled SPI port selected
//! \return -2 if unsupported SPI port selected
//! \return -3 if unsupported SPI mode configured via MIOS32_SPI_TransferModeInit()
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_SPI_TransferByte(u8 spi, u8 b)
{
  SPI_TypeDef *spi_ptr;

  switch( spi ) {
    case 0:
#ifdef MIOS32_DONT_USE_SPI0
      return -1; // disabled SPI port
#else
      spi_ptr = MIOS32_SPI0_PTR;
      break;
#endif

    case 1:
#ifdef MIOS32_DONT_USE_SPI1
      return -1; // disabled SPI port
#else
      spi_ptr = MIOS32_SPI1_PTR;
      break;
#endif

    case 2:
#ifdef MIOS32_DONT_USE_SPI2
      return -1; // disabled SPI port
#else
    // Software Emulation
    {
      u8 in_data = 0;

      switch( sw_spi2_mode ) {
        case MIOS32_SPI_MODE_CLK0_PHASE0:
	  // not tested on real HW yet!
	  MIOS32_SPI2_SET_MOSI(b & 0x01); // D0
	  in_data |= MIOS32_SPI2_GET_MISO ? 0x01 : 0x00;
	  MIOS32_SPI2_SET_SCLK_1;
	  MIOS32_SPI2_SET_SCLK_0;
	  MIOS32_SPI2_SET_MOSI(b & 0x02); // D1
	  in_data |= MIOS32_SPI2_GET_MISO ? 0x02 : 0x00;
	  MIOS32_SPI2_SET_SCLK_1;
	  MIOS32_SPI2_SET_SCLK_0;
	  MIOS32_SPI2_SET_MOSI(b & 0x04); // D2
	  in_data |= MIOS32_SPI2_GET_MISO ? 0x04 : 0x00;
	  MIOS32_SPI2_SET_SCLK_1;
	  MIOS32_SPI2_SET_SCLK_0;
	  MIOS32_SPI2_SET_MOSI(b & 0x08); // D3
	  in_data |= MIOS32_SPI2_GET_MISO ? 0x08 : 0x00;
	  MIOS32_SPI2_SET_SCLK_1;
	  MIOS32_SPI2_SET_SCLK_0;
	  MIOS32_SPI2_SET_MOSI(b & 0x10); // D4
	  in_data |= MIOS32_SPI2_GET_MISO ? 0x10 : 0x00;
	  MIOS32_SPI2_SET_SCLK_1;
	  MIOS32_SPI2_SET_SCLK_0;
	  MIOS32_SPI2_SET_MOSI(b & 0x20); // D5
	  in_data |= MIOS32_SPI2_GET_MISO ? 0x20 : 0x00;
	  MIOS32_SPI2_SET_SCLK_1;
	  MIOS32_SPI2_SET_SCLK_0;
	  MIOS32_SPI2_SET_MOSI(b & 0x40); // D6
	  in_data |= MIOS32_SPI2_GET_MISO ? 0x40 : 0x00;
	  MIOS32_SPI2_SET_SCLK_1;
	  MIOS32_SPI2_SET_SCLK_0;
	  MIOS32_SPI2_SET_MOSI(b & 0x80); // D7
	  in_data |= MIOS32_SPI2_GET_MISO ? 0x80 : 0x00;
	  MIOS32_SPI2_SET_SCLK_1;
	  MIOS32_SPI2_SET_SCLK_0;
	  break;

        case MIOS32_SPI_MODE_CLK0_PHASE1:
	  // not tested on real HW yet!
	  MIOS32_SPI2_SET_MOSI(b & 0x01); // D0
	  MIOS32_SPI2_SET_SCLK_1;
	  in_data |= MIOS32_SPI2_GET_MISO ? 0x01 : 0x00;
	  MIOS32_SPI2_SET_SCLK_0;
	  MIOS32_SPI2_SET_MOSI(b & 0x02); // D1
	  MIOS32_SPI2_SET_SCLK_1;
	  in_data |= MIOS32_SPI2_GET_MISO ? 0x02 : 0x00;
	  MIOS32_SPI2_SET_SCLK_0;
	  MIOS32_SPI2_SET_MOSI(b & 0x04); // D2
	  MIOS32_SPI2_SET_SCLK_1;
	  in_data |= MIOS32_SPI2_GET_MISO ? 0x04 : 0x00;
	  MIOS32_SPI2_SET_SCLK_0;
	  MIOS32_SPI2_SET_MOSI(b & 0x08); // D3
	  MIOS32_SPI2_SET_SCLK_1;
	  in_data |= MIOS32_SPI2_GET_MISO ? 0x08 : 0x00;
	  MIOS32_SPI2_SET_SCLK_0;
	  MIOS32_SPI2_SET_MOSI(b & 0x10); // D4
	  MIOS32_SPI2_SET_SCLK_1;
	  in_data |= MIOS32_SPI2_GET_MISO ? 0x10 : 0x00;
	  MIOS32_SPI2_SET_SCLK_0;
	  MIOS32_SPI2_SET_MOSI(b & 0x20); // D5
	  MIOS32_SPI2_SET_SCLK_1;
	  in_data |= MIOS32_SPI2_GET_MISO ? 0x20 : 0x00;
	  MIOS32_SPI2_SET_SCLK_0;
	  MIOS32_SPI2_SET_MOSI(b & 0x40); // D6
	  MIOS32_SPI2_SET_SCLK_1;
	  in_data |= MIOS32_SPI2_GET_MISO ? 0x40 : 0x00;
	  MIOS32_SPI2_SET_SCLK_0;
	  MIOS32_SPI2_SET_MOSI(b & 0x80); // D7
	  MIOS32_SPI2_SET_SCLK_1;
	  in_data |= MIOS32_SPI2_GET_MISO ? 0x80 : 0x00;
	  MIOS32_SPI2_SET_SCLK_0;
	  break;

        case MIOS32_SPI_MODE_CLK1_PHASE0:
	  // not tested on real HW yet!
	  MIOS32_SPI2_SET_MOSI(b & 0x01); // D0
	  in_data |= MIOS32_SPI2_GET_MISO ? 0x01 : 0x00;
	  MIOS32_SPI2_SET_SCLK_0;
	  MIOS32_SPI2_SET_SCLK_1;
	  MIOS32_SPI2_SET_MOSI(b & 0x02); // D1
	  in_data |= MIOS32_SPI2_GET_MISO ? 0x02 : 0x00;
	  MIOS32_SPI2_SET_SCLK_0;
	  MIOS32_SPI2_SET_SCLK_1;
	  MIOS32_SPI2_SET_MOSI(b & 0x04); // D2
	  in_data |= MIOS32_SPI2_GET_MISO ? 0x04 : 0x00;
	  MIOS32_SPI2_SET_SCLK_0;
	  MIOS32_SPI2_SET_SCLK_1;
	  MIOS32_SPI2_SET_MOSI(b & 0x08); // D3
	  in_data |= MIOS32_SPI2_GET_MISO ? 0x08 : 0x00;
	  MIOS32_SPI2_SET_SCLK_0;
	  MIOS32_SPI2_SET_SCLK_1;
	  MIOS32_SPI2_SET_MOSI(b & 0x10); // D4
	  in_data |= MIOS32_SPI2_GET_MISO ? 0x10 : 0x00;
	  MIOS32_SPI2_SET_SCLK_0;
	  MIOS32_SPI2_SET_SCLK_1;
	  MIOS32_SPI2_SET_MOSI(b & 0x20); // D5
	  in_data |= MIOS32_SPI2_GET_MISO ? 0x20 : 0x00;
	  MIOS32_SPI2_SET_SCLK_0;
	  MIOS32_SPI2_SET_SCLK_1;
	  MIOS32_SPI2_SET_MOSI(b & 0x40); // D6
	  in_data |= MIOS32_SPI2_GET_MISO ? 0x40 : 0x00;
	  MIOS32_SPI2_SET_SCLK_0;
	  MIOS32_SPI2_SET_SCLK_1;
	  MIOS32_SPI2_SET_MOSI(b & 0x80); // D7
	  in_data |= MIOS32_SPI2_GET_MISO ? 0x80 : 0x00;
	  MIOS32_SPI2_SET_SCLK_0;
	  MIOS32_SPI2_SET_SCLK_1;
	  break;

        case MIOS32_SPI_MODE_CLK1_PHASE1:
	  MIOS32_SPI2_SET_MOSI(b & 0x01); // D0
	  MIOS32_SPI2_SET_SCLK_0;
	  in_data |= MIOS32_SPI2_GET_MISO ? 0x01 : 0x00;
	  MIOS32_SPI2_SET_SCLK_1;
	  MIOS32_SPI2_SET_MOSI(b & 0x02); // D1
	  MIOS32_SPI2_SET_SCLK_0;
	  in_data |= MIOS32_SPI2_GET_MISO ? 0x02 : 0x00;
	  MIOS32_SPI2_SET_SCLK_1;
	  MIOS32_SPI2_SET_MOSI(b & 0x04); // D2
	  MIOS32_SPI2_SET_SCLK_0;
	  in_data |= MIOS32_SPI2_GET_MISO ? 0x04 : 0x00;
	  MIOS32_SPI2_SET_SCLK_1;
	  MIOS32_SPI2_SET_MOSI(b & 0x08); // D3
	  MIOS32_SPI2_SET_SCLK_0;
	  in_data |= MIOS32_SPI2_GET_MISO ? 0x08 : 0x00;
	  MIOS32_SPI2_SET_SCLK_1;
	  MIOS32_SPI2_SET_MOSI(b & 0x10); // D4
	  MIOS32_SPI2_SET_SCLK_0;
	  in_data |= MIOS32_SPI2_GET_MISO ? 0x10 : 0x00;
	  MIOS32_SPI2_SET_SCLK_1;
	  MIOS32_SPI2_SET_MOSI(b & 0x20); // D5
	  MIOS32_SPI2_SET_SCLK_0;
	  in_data |= MIOS32_SPI2_GET_MISO ? 0x20 : 0x00;
	  MIOS32_SPI2_SET_SCLK_1;
	  MIOS32_SPI2_SET_MOSI(b & 0x40); // D6
	  MIOS32_SPI2_SET_SCLK_0;
	  in_data |= MIOS32_SPI2_GET_MISO ? 0x40 : 0x00;
	  MIOS32_SPI2_SET_SCLK_1;
	  MIOS32_SPI2_SET_MOSI(b & 0x80); // D7
	  MIOS32_SPI2_SET_SCLK_0;
	  in_data |= MIOS32_SPI2_GET_MISO ? 0x80 : 0x00;
	  MIOS32_SPI2_SET_SCLK_1;
	  break;

        default:
	  return -3; // unsupported SPI mode
      }

      return in_data; // END of SW emulation - EXIT here!
#endif
    } break;

    default:
      return -2; // unsupported SPI port
  }

  // send byte
  spi_ptr->DR = b;

  if( spi_ptr->SR ); // dummy read due to undocumented pipelining issue :-/
  // TK: without this read (which can be done to any bus location) we could sporadically
  // get the status byte at the moment where DR is written. Accordingly, the busy bit 
  // will be 0.
  // you won't see this dummy read in STM drivers, as they never have a DR write 
  // followed by SR read, or as they are using SPI1/SPI2 pointers, which results into
  // some additional CPU instructions between strh/ldrh accesses.
  // We use a bus access instead of NOPs to avoid any risk for back-to-back transactions
  // over AHB (if SPI1/SPI2 pointers are used, there is still a risk for such a scenario,
  // e.g. if DMA loads the bus!)

  // wait until SPI transfer finished
  while( spi_ptr->SR & SPI_I2S_FLAG_BSY );

  // return received byte
  return spi_ptr->DR;
}


/////////////////////////////////////////////////////////////////////////////
//! Transfers a block of bytes via DMA.
//! \param[in] spi SPI number (0, 1 or 2)
//! \param[in] send_buffer pointer to buffer which should be sent.<BR>
//! If NULL, 0xff (all-one) will be sent.
//! \param[in] receive_buffer pointer to buffer which should get the received values.<BR>
//! If NULL, received bytes will be discarded.
//! \param[in] len number of bytes which should be transfered
//! \param[in] callback pointer to callback function which will be executed
//! from DMA channel interrupt once the transfer is finished.
//! If NULL, no callback function will be used, and MIOS32_SPI_TransferBlock() will
//! block until the transfer is finished.
//! \return >= 0 if no error during transfer
//! \return -1 if disabled SPI port selected
//! \return -2 if unsupported SPI port selected
//! \return -3 if function has been called during an ongoing DMA transfer
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_SPI_TransferBlock(u8 spi, u8 *send_buffer, u8 *receive_buffer, u16 len, void *callback)
{
  DMA_Channel_TypeDef *dma_tx_ptr, *dma_rx_ptr;

  switch( spi ) {
    case 0:
#ifdef MIOS32_DONT_USE_SPI0
      return -1; // disabled SPI port
#else
      dma_tx_ptr = MIOS32_SPI0_DMA_TX_PTR;
      dma_rx_ptr = MIOS32_SPI0_DMA_RX_PTR;
      break;
#endif

    case 1:
#ifdef MIOS32_DONT_USE_SPI1
      return -1; // disabled SPI port
#else
      dma_tx_ptr = MIOS32_SPI1_DMA_TX_PTR;
      dma_rx_ptr = MIOS32_SPI1_DMA_RX_PTR;
      break;
#endif

    case 2:
#ifdef MIOS32_DONT_USE_SPI2
      return -1; // disabled SPI port
#else
    // Software Emulation
    {
      u32 pos;

      // we have 4 cases:
      if( receive_buffer != NULL ) {
	if( send_buffer != NULL ) {
	  for(pos=0; pos<len; ++len)
	    *receive_buffer++ = MIOS32_SPI_TransferByte(spi, *send_buffer++);
	} else {
	  for(pos=0; pos<len; ++len)
	    *receive_buffer++ = MIOS32_SPI_TransferByte(spi, 0xff);
	}
      } else {
	// TODO: we could use an optimized "send only" function to speed up the SW emulation!
	if( send_buffer != NULL ) {
	  for(pos=0; pos<len; ++len)
	    MIOS32_SPI_TransferByte(spi, *send_buffer++);
	} else {
	  // nothing to do...
	}
      }

      // set callback function
      spi_callback[spi] = callback;
      if( spi_callback[spi] != NULL )
	spi_callback[spi]();

      return 0;; // END of SW emulation - EXIT here!
    } break;
#endif

    default:
      return -2; // unsupported SPI port
  }

  // exit if ongoing transfer
  if( dma_rx_ptr->CNDTR )
    return -3;

  // set callback function
  spi_callback[spi] = callback;

  // configure Rx channel
  DMA_Cmd(dma_rx_ptr, DISABLE);
  if( receive_buffer != NULL ) {
    // enable memory addr. increment - bytes written into receive buffer
    dma_rx_ptr->CMAR = (u32)receive_buffer;
    dma_rx_ptr->CCR |= DMA_MemoryInc_Enable;
  } else {
    // disable memory addr. increment - bytes written into dummy buffer
    rx_dummy_byte = 0xff;
    dma_rx_ptr->CMAR = (u32)&rx_dummy_byte;
    dma_rx_ptr->CCR &= ~DMA_MemoryInc_Enable;
  }
  dma_rx_ptr->CNDTR = len;
  DMA_Cmd(dma_rx_ptr, ENABLE);


  // configure Tx channel
  DMA_Cmd(dma_tx_ptr, DISABLE);
  if( send_buffer != NULL ) {
    // enable memory addr. increment - bytes read from send buffer
    dma_tx_ptr->CMAR = (u32)send_buffer;
    dma_tx_ptr->CCR |= DMA_MemoryInc_Enable;
  } else {
    // disable memory addr. increment - bytes read from dummy buffer
    tx_dummy_byte = 0xff;
    dma_tx_ptr->CMAR = (u32)&tx_dummy_byte;
    dma_tx_ptr->CCR &= ~DMA_MemoryInc_Enable;
  }
  dma_tx_ptr->CNDTR = len;

  // enable DMA interrupt if callback function active
  DMA_ITConfig(dma_rx_ptr, DMA_IT_TC, (callback != NULL) ? ENABLE : DISABLE);

  // start DMA transfer
  DMA_Cmd(dma_tx_ptr, ENABLE);

  // wait until all bytes have been transmitted/received
  while( dma_rx_ptr->CNDTR );

  return 0; // no error;
}


/////////////////////////////////////////////////////////////////////////////
// Called when callback function has been defined and SPI transfer has finished
/////////////////////////////////////////////////////////////////////////////
MIOS32_SPI0_DMA_IRQHANDLER_FUNC
{
  DMA_ClearFlag(MIOS32_SPI0_DMA_RX_IRQ_FLAGS);

  if( spi_callback[0] != NULL )
    spi_callback[0]();
}

MIOS32_SPI1_DMA_IRQHANDLER_FUNC
{
  DMA_ClearFlag(MIOS32_SPI1_DMA_RX_IRQ_FLAGS);

  if( spi_callback[1] != NULL )
    spi_callback[1]();
}

//! \}

#endif /* MIOS32_DONT_USE_SPI */

