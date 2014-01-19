// $Id$
//! \defgroup MIOS32_SPI
//!
//! Hardware Abstraction Layer for SPI ports of STM32F4
//!
//! Three ports are provided at J16 (SPI0), J8/9 (SPI1) and J19 (SPI2) of the 
//! MBHP_CORE_STM32F4 module..
//!
//! J16 provides SPI + two RCLK (alias Chip Select) lines at 3V, and is normaly
//! connected to a SD Card or/and an Ethernet Interface like ENC28J60.
//!
//! J8/9 provides SPI + two RCLK lines at 5V.
//! It is normally connected to the SRIO chain to scan DIN/DOUT modules.
//!
//! J19 provides SPI + two RCLK lines at 5V.
//! It's some kind of general purpose SPI, and used to communicate with
//! various MBHP modules, such as MBHP_AOUT* and MBHP_AINSER*
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
#define MIOS32_SPI0_DMA_RX_PTR DMA2_Stream2
#define MIOS32_SPI0_DMA_RX_CHN DMA_Channel_3
#define MIOS32_SPI0_DMA_RX_IRQ_FLAGS (DMA_FLAG_TCIF2 | DMA_FLAG_TEIF2 | DMA_FLAG_HTIF2 | DMA_FLAG_FEIF2)
#define MIOS32_SPI0_DMA_TX_PTR DMA2_Stream3
#define MIOS32_SPI0_DMA_TX_CHN DMA_Channel_3
#define MIOS32_SPI0_DMA_TX_IRQ_FLAGS (DMA_FLAG_TCIF3 | DMA_FLAG_TEIF3 | DMA_FLAG_HTIF3 | DMA_FLAG_FEIF3)
#define MIOS32_SPI0_DMA_IRQ_CHANNEL DMA2_Stream2_IRQn
#define MIOS32_SPI0_DMA_IRQHANDLER_FUNC void DMA2_Stream2_IRQHandler(void)

#define MIOS32_SPI0_RCLK1_PORT GPIOB
#define MIOS32_SPI0_RCLK1_PIN  GPIO_Pin_2
#define MIOS32_SPI0_RCLK1_AF   { } // slave not supported!
#define MIOS32_SPI0_RCLK2_PORT GPIOD
#define MIOS32_SPI0_RCLK2_PIN  GPIO_Pin_11
#define MIOS32_SPI0_RCLK2_AF   { }
#define MIOS32_SPI0_SCLK_PORT  GPIOA
#define MIOS32_SPI0_SCLK_PIN   GPIO_Pin_5
#define MIOS32_SPI0_SCLK_AF    { GPIO_PinAFConfig(GPIOA, GPIO_PinSource5, GPIO_AF_SPI1); }
#define MIOS32_SPI0_MISO_PORT  GPIOA
#define MIOS32_SPI0_MISO_PIN   GPIO_Pin_6
#define MIOS32_SPI0_MISO_AF    { GPIO_PinAFConfig(GPIOA, GPIO_PinSource6, GPIO_AF_SPI1); }
#define MIOS32_SPI0_MOSI_PORT  GPIOA
#define MIOS32_SPI0_MOSI_PIN   GPIO_Pin_7
#define MIOS32_SPI0_MOSI_AF    { GPIO_PinAFConfig(GPIOA, GPIO_PinSource7, GPIO_AF_SPI1); }


#define MIOS32_SPI1_PTR        SPI2
#define MIOS32_SPI1_DMA_RX_PTR DMA1_Stream3
#define MIOS32_SPI1_DMA_RX_CHN DMA_Channel_0
#define MIOS32_SPI1_DMA_RX_IRQ_FLAGS (DMA_FLAG_TCIF3 | DMA_FLAG_TEIF3 | DMA_FLAG_HTIF3 | DMA_FLAG_FEIF3)
#define MIOS32_SPI1_DMA_TX_PTR DMA1_Stream4
#define MIOS32_SPI1_DMA_TX_CHN DMA_Channel_0
#define MIOS32_SPI1_DMA_TX_IRQ_FLAGS (DMA_FLAG_TCIF4 | DMA_FLAG_TEIF4 | DMA_FLAG_HTIF4 | DMA_FLAG_FEIF4)
#define MIOS32_SPI1_DMA_IRQ_CHANNEL DMA1_Stream3_IRQn
#define MIOS32_SPI1_DMA_IRQHANDLER_FUNC void DMA1_Stream3_IRQHandler(void)

#define MIOS32_SPI1_RCLK1_PORT GPIOB
#define MIOS32_SPI1_RCLK1_PIN  GPIO_Pin_12
#define MIOS32_SPI1_RCLK1_AF   { GPIO_PinAFConfig(GPIOB, GPIO_PinSource12, GPIO_AF_SPI2); } // only relevant for slave mode
#define MIOS32_SPI1_RCLK2_PORT GPIOD
#define MIOS32_SPI1_RCLK2_PIN  GPIO_Pin_10
#define MIOS32_SPI1_RCLK2_AF   { }
#define MIOS32_SPI1_SCLK_PORT  GPIOB
#define MIOS32_SPI1_SCLK_PIN   GPIO_Pin_13
#define MIOS32_SPI1_SCLK_AF    { GPIO_PinAFConfig(GPIOB, GPIO_PinSource13, GPIO_AF_SPI2); }
#define MIOS32_SPI1_MISO_PORT  GPIOB
#define MIOS32_SPI1_MISO_PIN   GPIO_Pin_14
#define MIOS32_SPI1_MISO_AF    { GPIO_PinAFConfig(GPIOB, GPIO_PinSource14, GPIO_AF_SPI2); }
#define MIOS32_SPI1_MOSI_PORT  GPIOB
#define MIOS32_SPI1_MOSI_PIN   GPIO_Pin_15
#define MIOS32_SPI1_MOSI_AF    { GPIO_PinAFConfig(GPIOB, GPIO_PinSource15, GPIO_AF_SPI2); }


#define MIOS32_SPI2_PTR        SPI3
#define MIOS32_SPI2_DMA_RX_PTR DMA1_Stream2
#define MIOS32_SPI2_DMA_RX_CHN DMA_Channel_0
#define MIOS32_SPI2_DMA_RX_IRQ_FLAGS (DMA_FLAG_TCIF2 | DMA_FLAG_TEIF2 | DMA_FLAG_HTIF2 | DMA_FLAG_FEIF2)
#define MIOS32_SPI2_DMA_TX_PTR DMA1_Stream5
#define MIOS32_SPI2_DMA_TX_CHN DMA_Channel_0
#define MIOS32_SPI2_DMA_TX_IRQ_FLAGS (DMA_FLAG_TCIF5 | DMA_FLAG_TEIF5 | DMA_FLAG_HTIF5 | DMA_FLAG_FEIF5)
#define MIOS32_SPI2_DMA_IRQ_CHANNEL DMA1_Stream2_IRQn
#define MIOS32_SPI2_DMA_IRQHANDLER_FUNC void DMA1_Stream2_IRQHandler(void)

#define MIOS32_SPI2_RCLK1_PORT GPIOA
#define MIOS32_SPI2_RCLK1_PIN  GPIO_Pin_15
#define MIOS32_SPI2_RCLK1_AF   { GPIO_PinAFConfig(GPIOA, GPIO_PinSource15, GPIO_AF_SPI3); } // only relevant for slave mode
#define MIOS32_SPI2_RCLK2_PORT GPIOB
#define MIOS32_SPI2_RCLK2_PIN  GPIO_Pin_8
#define MIOS32_SPI2_RCLK2_AF   { }
#define MIOS32_SPI2_SCLK_PORT  GPIOB
#define MIOS32_SPI2_SCLK_PIN   GPIO_Pin_3
#define MIOS32_SPI2_SCLK_AF    { GPIO_PinAFConfig(GPIOB, GPIO_PinSource3, GPIO_AF_SPI3); }
#define MIOS32_SPI2_MISO_PORT  GPIOB
#define MIOS32_SPI2_MISO_PIN   GPIO_Pin_4
#define MIOS32_SPI2_MISO_AF    { GPIO_PinAFConfig(GPIOB, GPIO_PinSource4, GPIO_AF_SPI3); }
#define MIOS32_SPI2_MOSI_PORT  GPIOB
#define MIOS32_SPI2_MOSI_PIN   GPIO_Pin_5
#define MIOS32_SPI2_MOSI_AF    { GPIO_PinAFConfig(GPIOB, GPIO_PinSource5, GPIO_AF_SPI3); }



/////////////////////////////////////////////////////////////////////////////
// Local Defines
/////////////////////////////////////////////////////////////////////////////
#define CCR_ENABLE              ((uint32_t)0x00000001)



/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

static void (*spi_callback[3])(void);

static u8 tx_dummy_byte;
static u8 rx_dummy_byte;


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
  MIOS32_SPI_IO_Init(0, MIOS32_SPI_PIN_DRIVER_WEAK);

  // enable SPI peripheral clock
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, ENABLE);

  // enable DMA1 and DMA2 clock
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA1, ENABLE);
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA2, ENABLE);

  // DMA Configuration for SPI Rx Event
  DMA_Cmd(MIOS32_SPI0_DMA_RX_PTR, DISABLE);
  DMA_InitStructure.DMA_Channel = MIOS32_SPI0_DMA_RX_CHN;
  DMA_InitStructure.DMA_PeripheralBaseAddr = (u32)&MIOS32_SPI0_PTR->DR;
  DMA_InitStructure.DMA_Memory0BaseAddr = 0; // will be configured later
  DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralToMemory;
  DMA_InitStructure.DMA_BufferSize = 0; // will be configured later
  DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
  DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
  DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
  DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
  DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
  DMA_InitStructure.DMA_Priority = DMA_Priority_Medium;
  DMA_Init(MIOS32_SPI0_DMA_RX_PTR, &DMA_InitStructure);

  // DMA Configuration for SPI Tx Event
  // (partly re-using previous DMA setup)
  DMA_Cmd(MIOS32_SPI0_DMA_TX_PTR, DISABLE);
  DMA_InitStructure.DMA_Channel = MIOS32_SPI0_DMA_TX_CHN;
  DMA_InitStructure.DMA_Memory0BaseAddr = 0; // will be configured later
  DMA_InitStructure.DMA_BufferSize = 0; // will be configured later
  DMA_InitStructure.DMA_DIR = DMA_DIR_MemoryToPeripheral;
  DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
  DMA_Init(MIOS32_SPI0_DMA_TX_PTR, &DMA_InitStructure);

  // enable SPI
  SPI_Cmd(MIOS32_SPI0_PTR, ENABLE);

  // enable SPI interrupts to DMA
  SPI_I2S_DMACmd(MIOS32_SPI0_PTR, SPI_I2S_DMAReq_Tx | SPI_I2S_DMAReq_Rx, ENABLE);

  // Configure DMA interrupt
  MIOS32_IRQ_Install(MIOS32_SPI0_DMA_IRQ_CHANNEL, MIOS32_IRQ_SPI_DMA_PRIORITY);

  // initial SPI peripheral configuration
  MIOS32_SPI_TransferModeInit(0, MIOS32_SPI_MODE_CLK1_PHASE1, MIOS32_SPI_PRESCALER_128);
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
  MIOS32_SPI_IO_Init(1, MIOS32_SPI_PIN_DRIVER_WEAK);

  // enable SPI peripheral clock
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI2, ENABLE);

  // enable DMA1 and DMA2 clock
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA1, ENABLE);
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA2, ENABLE);

  // DMA Configuration for SPI Rx Event
  DMA_Cmd(MIOS32_SPI1_DMA_RX_PTR, DISABLE);
  DMA_InitStructure.DMA_Channel = MIOS32_SPI1_DMA_RX_CHN;
  DMA_InitStructure.DMA_PeripheralBaseAddr = (u32)&MIOS32_SPI1_PTR->DR;
  DMA_InitStructure.DMA_Memory0BaseAddr = 0; // will be configured later
  DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralToMemory;
  DMA_InitStructure.DMA_BufferSize = 0; // will be configured later
  DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
  DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
  DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
  DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
  DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
  DMA_InitStructure.DMA_Priority = DMA_Priority_Medium;
  DMA_Init(MIOS32_SPI1_DMA_RX_PTR, &DMA_InitStructure);

  // DMA Configuration for SPI Tx Event
  // (partly re-using previous DMA setup)
  DMA_Cmd(MIOS32_SPI1_DMA_TX_PTR, DISABLE);
  DMA_InitStructure.DMA_Channel = MIOS32_SPI1_DMA_TX_CHN;
  DMA_InitStructure.DMA_Memory0BaseAddr = 0; // will be configured later
  DMA_InitStructure.DMA_BufferSize = 0; // will be configured later
  DMA_InitStructure.DMA_DIR = DMA_DIR_MemoryToPeripheral;
  DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
  DMA_Init(MIOS32_SPI1_DMA_TX_PTR, &DMA_InitStructure);

  // enable SPI
  SPI_Cmd(MIOS32_SPI1_PTR, ENABLE);

  // enable SPI interrupts to DMA
  SPI_I2S_DMACmd(MIOS32_SPI1_PTR, SPI_I2S_DMAReq_Tx | SPI_I2S_DMAReq_Rx, ENABLE);

  // Configure DMA interrupt
  MIOS32_IRQ_Install(MIOS32_SPI1_DMA_IRQ_CHANNEL, MIOS32_IRQ_SPI_DMA_PRIORITY);

  // initial SPI peripheral configuration
  MIOS32_SPI_TransferModeInit(1, MIOS32_SPI_MODE_CLK1_PHASE1, MIOS32_SPI_PRESCALER_128);
#endif /* MIOS32_DONT_USE_SPI1 */


  ///////////////////////////////////////////////////////////////////////////
  // SPI2
  ///////////////////////////////////////////////////////////////////////////
#ifndef MIOS32_DONT_USE_SPI2

  // disable callback function
  spi_callback[2] = NULL;

  // set RC pin(s) to 1
  MIOS32_SPI_RC_PinSet(2, 0, 1); // spi, rc_pin, pin_value
  MIOS32_SPI_RC_PinSet(2, 1, 1); // spi, rc_pin, pin_value

  // IO configuration
  MIOS32_SPI_IO_Init(2, MIOS32_SPI_PIN_DRIVER_WEAK);

  // enable SPI peripheral clock
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI3, ENABLE);

  // enable DMA1 and DMA2 clock
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA1, ENABLE);
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA2, ENABLE);

  // DMA Configuration for SPI Rx Event
  DMA_Cmd(MIOS32_SPI2_DMA_RX_PTR, DISABLE);
  DMA_InitStructure.DMA_Channel = MIOS32_SPI2_DMA_RX_CHN;
  DMA_InitStructure.DMA_PeripheralBaseAddr = (u32)&MIOS32_SPI2_PTR->DR;
  DMA_InitStructure.DMA_Memory0BaseAddr = 0; // will be configured later
  DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralToMemory;
  DMA_InitStructure.DMA_BufferSize = 0; // will be configured later
  DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
  DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
  DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
  DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
  DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
  DMA_InitStructure.DMA_Priority = DMA_Priority_Medium;
  DMA_Init(MIOS32_SPI2_DMA_RX_PTR, &DMA_InitStructure);

  // DMA Configuration for SPI Tx Event
  // (partly re-using previous DMA setup)
  DMA_Cmd(MIOS32_SPI2_DMA_TX_PTR, DISABLE);
  DMA_InitStructure.DMA_Channel = MIOS32_SPI2_DMA_TX_CHN;
  DMA_InitStructure.DMA_Memory0BaseAddr = 0; // will be configured later
  DMA_InitStructure.DMA_BufferSize = 0; // will be configured later
  DMA_InitStructure.DMA_DIR = DMA_DIR_MemoryToPeripheral;
  DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
  DMA_Init(MIOS32_SPI2_DMA_TX_PTR, &DMA_InitStructure);

  // enable SPI
  SPI_Cmd(MIOS32_SPI2_PTR, ENABLE);

  // enable SPI interrupts to DMA
  SPI_I2S_DMACmd(MIOS32_SPI2_PTR, SPI_I2S_DMAReq_Tx | SPI_I2S_DMAReq_Rx, ENABLE);

  // Configure DMA interrupt
  MIOS32_IRQ_Install(MIOS32_SPI2_DMA_IRQ_CHANNEL, MIOS32_IRQ_SPI_DMA_PRIORITY);

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
  u8 slave = 0;
  switch( spi_pin_driver ) {
    case MIOS32_SPI_PIN_SLAVE_DRIVER_STRONG:
      slave = 1;
    case MIOS32_SPI_PIN_DRIVER_STRONG:
      GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
      GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
      break;

    case MIOS32_SPI_PIN_SLAVE_DRIVER_STRONG_OD:
      slave = 1;
    case MIOS32_SPI_PIN_DRIVER_STRONG_OD:
      GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
      GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;
      break;

    case MIOS32_SPI_PIN_SLAVE_DRIVER_WEAK:
      slave = 1;
    case MIOS32_SPI_PIN_DRIVER_WEAK:
      GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
      GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
      break;

    case MIOS32_SPI_PIN_SLAVE_DRIVER_WEAK_OD:
      slave = 1;
    case MIOS32_SPI_PIN_DRIVER_WEAK_OD:
      GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
      GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;
      break;

    default:
      return -3; // unsupported pin driver mode
  }

  switch( spi ) {
    case 0:
#ifdef MIOS32_DONT_USE_SPI0
      return -1; // disabled SPI port
#else
      MIOS32_SPI0_RCLK1_AF;
      MIOS32_SPI0_RCLK2_AF;
      MIOS32_SPI0_SCLK_AF;
      MIOS32_SPI0_MISO_AF;
      MIOS32_SPI0_MOSI_AF;

      if( slave ) {
#if 1
	return -3; // slave mode not supported for this pin
#else
	// SCLK and DOUT are inputs assigned to alternate functions
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_Pin  = MIOS32_SPI0_SCLK_PIN;
	GPIO_Init(MIOS32_SPI0_SCLK_PORT, &GPIO_InitStructure);
	GPIO_InitStructure.GPIO_Pin  = MIOS32_SPI0_MOSI_PIN;
	GPIO_Init(MIOS32_SPI0_MOSI_PORT, &GPIO_InitStructure);
    
	// RCLK (resp. CS) are configured as inputs as well
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_Pin  = MIOS32_SPI0_RCLK1_PIN;
	GPIO_Init(MIOS32_SPI0_RCLK1_PORT, &GPIO_InitStructure);
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT; // GPIO!
	GPIO_InitStructure.GPIO_Pin  = MIOS32_SPI0_RCLK2_PIN;
	GPIO_Init(MIOS32_SPI0_RCLK2_PORT, &GPIO_InitStructure);
    
	// DOUT is output assigned to alternate function
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_Pin  = MIOS32_SPI0_MISO_PIN;
	GPIO_Init(MIOS32_SPI0_MISO_PORT, &GPIO_InitStructure);
#endif
      } else {
	// SCLK and DOUT are outputs assigned to alternate functions
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_Pin  = MIOS32_SPI0_SCLK_PIN;
	GPIO_Init(MIOS32_SPI0_SCLK_PORT, &GPIO_InitStructure);
	GPIO_InitStructure.GPIO_Pin  = MIOS32_SPI0_MOSI_PIN;
	GPIO_Init(MIOS32_SPI0_MOSI_PORT, &GPIO_InitStructure);
    
	// RCLK outputs assigned to GPIO
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_Pin  = MIOS32_SPI0_RCLK1_PIN;
	GPIO_Init(MIOS32_SPI0_RCLK1_PORT, &GPIO_InitStructure);
	GPIO_InitStructure.GPIO_Pin  = MIOS32_SPI0_RCLK2_PIN;
	GPIO_Init(MIOS32_SPI0_RCLK2_PORT, &GPIO_InitStructure);

#if defined(MIOS32_BOARD_STM32F4DISCOVERY) || defined(MIOS32_BOARD_MBHP_CORE_STM32F4)
	// set RE3=1 to ensure that the on-board MEMs is disabled
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
	GPIO_Init(GPIOE, &GPIO_InitStructure);	
	MIOS32_SYS_STM_PINSET_1(GPIOE, GPIO_Pin_3);
#else
# warning "Please doublecheck if RE3 has to be set to 1 to disable MEMs"
#endif
    
	// DIN is input with pull-up
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_InitStructure.GPIO_Pin  = MIOS32_SPI0_MISO_PIN;
	GPIO_Init(MIOS32_SPI0_MISO_PORT, &GPIO_InitStructure);
      }
      break;
#endif

    case 1:
#ifdef MIOS32_DONT_USE_SPI1
      return -1; // disabled SPI port
#else
      MIOS32_SPI1_RCLK1_AF;
      MIOS32_SPI1_RCLK2_AF;
      MIOS32_SPI1_SCLK_AF;
      MIOS32_SPI1_MISO_AF;
      MIOS32_SPI1_MOSI_AF;

      if( slave ) {
	// SCLK and DOUT are inputs assigned to alternate functions
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_Pin  = MIOS32_SPI1_SCLK_PIN;
	GPIO_Init(MIOS32_SPI1_SCLK_PORT, &GPIO_InitStructure);
	GPIO_InitStructure.GPIO_Pin  = MIOS32_SPI1_MOSI_PIN;
	GPIO_Init(MIOS32_SPI1_MOSI_PORT, &GPIO_InitStructure);
    
	// RCLK (resp. CS) are configured as inputs as well
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_Pin  = MIOS32_SPI1_RCLK1_PIN;
	GPIO_Init(MIOS32_SPI1_RCLK1_PORT, &GPIO_InitStructure);
	GPIO_InitStructure.GPIO_Pin  = MIOS32_SPI1_RCLK2_PIN;
	GPIO_Init(MIOS32_SPI1_RCLK2_PORT, &GPIO_InitStructure);

	// DOUT is output assigned to alternate function
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_Pin  = MIOS32_SPI1_MISO_PIN;
	GPIO_Init(MIOS32_SPI1_MISO_PORT, &GPIO_InitStructure);    
      } else {
	// SCLK and DIN are inputs
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_Pin  = MIOS32_SPI1_SCLK_PIN;
	GPIO_Init(MIOS32_SPI1_SCLK_PORT, &GPIO_InitStructure);
	GPIO_InitStructure.GPIO_Pin  = MIOS32_SPI1_MOSI_PIN;
	GPIO_Init(MIOS32_SPI1_MOSI_PORT, &GPIO_InitStructure);
    
	// RCLK (resp. CS) are configured as inputs as well
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_Pin  = MIOS32_SPI1_RCLK1_PIN;
	GPIO_Init(MIOS32_SPI1_RCLK1_PORT, &GPIO_InitStructure);
	GPIO_InitStructure.GPIO_Pin  = MIOS32_SPI1_RCLK2_PIN;
	GPIO_Init(MIOS32_SPI1_RCLK2_PORT, &GPIO_InitStructure);
    
	// DIN is input with pull-up
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_InitStructure.GPIO_Pin  = MIOS32_SPI1_MISO_PIN;
	GPIO_Init(MIOS32_SPI1_MISO_PORT, &GPIO_InitStructure);
      }

      break;
#endif

    case 2:
#ifdef MIOS32_DONT_USE_SPI2
      return -1; // disabled SPI port
#else
      MIOS32_SPI2_RCLK1_AF;
      MIOS32_SPI2_RCLK2_AF;
      MIOS32_SPI2_SCLK_AF;
      MIOS32_SPI2_MISO_AF;
      MIOS32_SPI2_MOSI_AF;

      if( slave ) {
	// SCLK and DOUT are inputs assigned to alternate functions
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_Pin  = MIOS32_SPI2_SCLK_PIN;
	GPIO_Init(MIOS32_SPI2_SCLK_PORT, &GPIO_InitStructure);
	GPIO_InitStructure.GPIO_Pin  = MIOS32_SPI2_MOSI_PIN;
	GPIO_Init(MIOS32_SPI2_MOSI_PORT, &GPIO_InitStructure);
    
	// RCLK (resp. CS) are configured as inputs as well
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_Pin  = MIOS32_SPI2_RCLK1_PIN;
	GPIO_Init(MIOS32_SPI2_RCLK1_PORT, &GPIO_InitStructure);
	GPIO_InitStructure.GPIO_Pin  = MIOS32_SPI2_RCLK2_PIN;
	GPIO_Init(MIOS32_SPI2_RCLK2_PORT, &GPIO_InitStructure);

	// DOUT is output assigned to alternate function
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_Pin  = MIOS32_SPI2_MISO_PIN;
	GPIO_Init(MIOS32_SPI2_MISO_PORT, &GPIO_InitStructure);    
      } else {
	// SCLK and DIN are inputs
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_Pin  = MIOS32_SPI2_SCLK_PIN;
	GPIO_Init(MIOS32_SPI2_SCLK_PORT, &GPIO_InitStructure);
	GPIO_InitStructure.GPIO_Pin  = MIOS32_SPI2_MOSI_PIN;
	GPIO_Init(MIOS32_SPI2_MOSI_PORT, &GPIO_InitStructure);
    
	// RCLK (resp. CS) are configured as inputs as well
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_Pin  = MIOS32_SPI2_RCLK1_PIN;
	GPIO_Init(MIOS32_SPI2_RCLK1_PORT, &GPIO_InitStructure);
	GPIO_InitStructure.GPIO_Pin  = MIOS32_SPI2_RCLK2_PIN;
	GPIO_Init(MIOS32_SPI2_RCLK2_PORT, &GPIO_InitStructure);
    
	// DIN is input with pull-up
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_InitStructure.GPIO_Pin  = MIOS32_SPI2_MISO_PIN;
	GPIO_Init(MIOS32_SPI2_MISO_PORT, &GPIO_InitStructure);
      }

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
//!   <LI>MIOS32_SPI_PRESCALER_2: sets clock rate 23.4 nS @ 84 MHz (42.67 MBit/s)
//!   <LI>MIOS32_SPI_PRESCALER_4: sets clock rate 46,8 nS @ 84 MHz (21.33 MBit/s)
//!   <LI>MIOS32_SPI_PRESCALER_8: sets clock rate 93.8 nS @ 84 MHz (10.67 MBit/s)
//!   <LI>MIOS32_SPI_PRESCALER_16: sets clock rate 187 nS @ 84 MHz (5.333 MBit/s)
//!   <LI>MIOS32_SPI_PRESCALER_32: sets clock rate 375 nS @ 84 MHz (2.667 MBit/s)
//!   <LI>MIOS32_SPI_PRESCALER_64: sets clock rate 750 nS @ 84 MHz (1.333 MBit/s)
//!   <LI>MIOS32_SPI_PRESCALER_128: sets clock rate 1.5 uS @ 84 MHz (0.667 MBit/s)
//!   <LI>MIOS32_SPI_PRESCALER_256: sets clock rate 3 uS @ 84 MHz (0.333 MBit/s)
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
    case MIOS32_SPI_MODE_SLAVE_CLK0_PHASE0:
      SPI_InitStructure.SPI_Mode = SPI_Mode_Slave;
      SPI_InitStructure.SPI_NSS  = SPI_NSS_Hard;
    case MIOS32_SPI_MODE_CLK0_PHASE0:
      SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;
      SPI_InitStructure.SPI_CPHA = SPI_CPHA_1Edge;
      break;

    case MIOS32_SPI_MODE_SLAVE_CLK0_PHASE1:
      SPI_InitStructure.SPI_Mode = SPI_Mode_Slave;
      SPI_InitStructure.SPI_NSS  = SPI_NSS_Hard;
    case MIOS32_SPI_MODE_CLK0_PHASE1:
      SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;
      SPI_InitStructure.SPI_CPHA = SPI_CPHA_2Edge;
      break;

    case MIOS32_SPI_MODE_SLAVE_CLK1_PHASE0:
      SPI_InitStructure.SPI_Mode = SPI_Mode_Slave;
      SPI_InitStructure.SPI_NSS  = SPI_NSS_Hard;
    case MIOS32_SPI_MODE_CLK1_PHASE0:
      SPI_InitStructure.SPI_CPOL = SPI_CPOL_High;
      SPI_InitStructure.SPI_CPHA = SPI_CPHA_1Edge;
      break;

    case MIOS32_SPI_MODE_SLAVE_CLK1_PHASE1:
      SPI_InitStructure.SPI_Mode = SPI_Mode_Slave;
      SPI_InitStructure.SPI_NSS  = SPI_NSS_Hard;
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
    case 0: {
#ifdef MIOS32_DONT_USE_SPI0
      return -1; // disabled SPI port
#else
      if( SPI_InitStructure.SPI_Mode == SPI_Mode_Slave ) {
	return -3; // slave mode not supported for this SPI
      }
      u16 prev_cr1 = MIOS32_SPI0_PTR->CR1;
      SPI_InitStructure.SPI_BaudRatePrescaler = ((u16)spi_prescaler&7) << 3;
      SPI_Init(MIOS32_SPI0_PTR, &SPI_InitStructure);

      if( SPI_InitStructure.SPI_Mode == SPI_Mode_Master ) {
	if( (prev_cr1 ^ MIOS32_SPI0_PTR->CR1) & 3 ) { // CPOL and CPHA located at bit #1 and #0
	  // clock configuration has been changed - we should send a dummy byte
	  // before the application activates chip select.
	  // this solves a dependency between SDCard and ENC28J60 driver
	  MIOS32_SPI_TransferByte(spi, 0xff);
	}
      }
#endif
    } break;

    case 1: {
#ifdef MIOS32_DONT_USE_SPI1
      return -1; // disabled SPI port
#else
      u16 prev_cr1 = MIOS32_SPI1_PTR->CR1;

      SPI_InitStructure.SPI_BaudRatePrescaler = (((u16)spi_prescaler&7)-1) << 3;
      SPI_Init(MIOS32_SPI1_PTR, &SPI_InitStructure);

      if( SPI_InitStructure.SPI_Mode == SPI_Mode_Master ) {
	if( (prev_cr1 ^ MIOS32_SPI1_PTR->CR1) & 3 ) { // CPOL and CPHA located at bit #1 and #0
	  // clock configuration has been changed - we should send a dummy byte
	  // before the application activates chip select.
	  // this solves a dependency between SDCard and ENC28J60 driver
	  MIOS32_SPI_TransferByte(spi, 0xff);
	}
      }
#endif
    } break;

    case 2: {
#ifdef MIOS32_DONT_USE_SPI2
      return -1; // disabled SPI port
#else
      u16 prev_cr1 = MIOS32_SPI2_PTR->CR1;

      SPI_InitStructure.SPI_BaudRatePrescaler = (((u16)spi_prescaler&7)-1) << 3;
      SPI_Init(MIOS32_SPI2_PTR, &SPI_InitStructure);

      if( SPI_InitStructure.SPI_Mode == SPI_Mode_Master ) {
	if( (prev_cr1 ^ MIOS32_SPI2_PTR->CR1) & 3 ) { // CPOL and CPHA located at bit #1 and #0
	  // clock configuration has been changed - we should send a dummy byte
	  // before the application activates chip select.
	  // this solves a dependency between SDCard and ENC28J60 driver
	  MIOS32_SPI_TransferByte(spi, 0xff);
	}
      }
#endif
    } break;

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
    case 0: MIOS32_SYS_STM_PINSET(MIOS32_SPI0_RCLK1_PORT, MIOS32_SPI0_RCLK1_PIN, pin_value); break;
    case 1: MIOS32_SYS_STM_PINSET(MIOS32_SPI0_RCLK2_PORT, MIOS32_SPI0_RCLK2_PIN, pin_value); break;
    default: return -3; // unsupported RC pin
    }
    break;
#endif

  case 1:
#ifdef MIOS32_DONT_USE_SPI1
    return -1; // disabled SPI port
#else
    switch( rc_pin ) {
    case 0: MIOS32_SYS_STM_PINSET(MIOS32_SPI1_RCLK1_PORT, MIOS32_SPI1_RCLK1_PIN, pin_value); break;
    case 1: MIOS32_SYS_STM_PINSET(MIOS32_SPI1_RCLK2_PORT, MIOS32_SPI1_RCLK2_PIN, pin_value); break;
    default: return -3; // unsupported RC pin
    }
    break;
#endif

  case 2:
#ifdef MIOS32_DONT_USE_SPI2
    return -1; // disabled SPI port
#else
    switch( rc_pin ) {
    case 0: MIOS32_SYS_STM_PINSET(MIOS32_SPI2_RCLK1_PORT, MIOS32_SPI2_RCLK1_PIN, pin_value); break;
    case 1: MIOS32_SYS_STM_PINSET(MIOS32_SPI2_RCLK2_PORT, MIOS32_SPI2_RCLK2_PIN, pin_value); break;
    default: return -3; // unsupported RC pin
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
      spi_ptr = MIOS32_SPI2_PTR;
      break;
#endif

    default:
      return -2; // unsupported SPI port
  }

  // send byte
  spi_ptr->DR = b;

  //if( spi_ptr->SR ); // dummy read due to undocumented pipelining issue :-/
  // TK: without this read (which can be done to any bus location) we could sporadically
  // get the status byte at the moment where DR is written. Accordingly, the busy bit 
  // will be 0.
  // you won't see this dummy read in STM drivers, as they never have a DR write 
  // followed by SR read, or as they are using SPI1/SPI2 pointers, which results into
  // some additional CPU instructions between strh/ldrh accesses.
  // We use a bus access instead of NOPs to avoid any risk for back-to-back transactions
  // over AHB (if SPI1/SPI2 pointers are used, there is still a risk for such a scenario,
  // e.g. if DMA loads the bus!)

  // TK update: the dummy read above becomes obsolete since we are checking for SPI Master mode now
  // which requires a read operation as well

  // wait until SPI transfer finished
  if( spi_ptr->CR1 & SPI_Mode_Master ) {
    while( spi_ptr->SR & SPI_I2S_FLAG_BSY );
  } else {
    while( !(spi_ptr->SR & SPI_I2S_FLAG_RXNE) );
  }

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
  SPI_TypeDef *spi_ptr;
  DMA_Stream_TypeDef *dma_tx_ptr, *dma_rx_ptr;
  u32 dma_tx_irq_flags, dma_rx_irq_flags;

  switch( spi ) {
    case 0:
#ifdef MIOS32_DONT_USE_SPI0
      return -1; // disabled SPI port
#else
      spi_ptr = MIOS32_SPI0_PTR;
      dma_tx_ptr = MIOS32_SPI0_DMA_TX_PTR;
      dma_tx_irq_flags = MIOS32_SPI0_DMA_TX_IRQ_FLAGS;
      dma_rx_ptr = MIOS32_SPI0_DMA_RX_PTR;
      dma_rx_irq_flags = MIOS32_SPI0_DMA_RX_IRQ_FLAGS;
      break;
#endif

    case 1:
#ifdef MIOS32_DONT_USE_SPI1
      return -1; // disabled SPI port
#else
      spi_ptr = MIOS32_SPI1_PTR;
      dma_tx_ptr = MIOS32_SPI1_DMA_TX_PTR;
      dma_tx_irq_flags = MIOS32_SPI1_DMA_TX_IRQ_FLAGS;
      dma_rx_ptr = MIOS32_SPI1_DMA_RX_PTR;
      dma_rx_irq_flags = MIOS32_SPI1_DMA_RX_IRQ_FLAGS;
      break;
#endif

    case 2:
#ifdef MIOS32_DONT_USE_SPI2
      return -1; // disabled SPI port
#else
      spi_ptr = MIOS32_SPI2_PTR;
      dma_tx_ptr = MIOS32_SPI2_DMA_TX_PTR;
      dma_tx_irq_flags = MIOS32_SPI2_DMA_TX_IRQ_FLAGS;
      dma_rx_ptr = MIOS32_SPI2_DMA_RX_PTR;
      dma_rx_irq_flags = MIOS32_SPI2_DMA_RX_IRQ_FLAGS;
      break;
#endif

    default:
      return -2; // unsupported SPI port
  }

  // exit if ongoing transfer
  if( dma_rx_ptr->NDTR )
    return -3;

  // set callback function
  spi_callback[spi] = callback;

  // ensure that previously received value doesn't cause DMA access
  if( spi_ptr->DR );

  // configure Rx channel
  // TK: optimization method: read rx_CCR once, write back only when required
  // the channel must be disabled to configure new values
  u32 rx_CCR = dma_rx_ptr->CR & ~CCR_ENABLE;
  dma_rx_ptr->CR = rx_CCR;
  if( receive_buffer != NULL ) {
    // enable memory addr. increment - bytes written into receive buffer
    dma_rx_ptr->M0AR = (u32)receive_buffer;
    rx_CCR |= DMA_MemoryInc_Enable;
  } else {
    // disable memory addr. increment - bytes written into dummy buffer
    rx_dummy_byte = 0xff;
    dma_rx_ptr->M0AR = (u32)&rx_dummy_byte;
    rx_CCR &= ~DMA_MemoryInc_Enable;
  }
  dma_rx_ptr->NDTR = len;
  rx_CCR |= CCR_ENABLE;


  // configure Tx channel
  // TK: optimization method: read tx_CCR once, write back only when required
  // the channel must be disabled to configure new values
  u32 tx_CCR = dma_tx_ptr->CR & ~CCR_ENABLE;
  dma_tx_ptr->CR = tx_CCR;
  if( send_buffer != NULL ) {
    // enable memory addr. increment - bytes read from send buffer
    dma_tx_ptr->M0AR = (u32)send_buffer;
    tx_CCR |= DMA_MemoryInc_Enable;
  } else {
    // disable memory addr. increment - bytes read from dummy buffer
    tx_dummy_byte = 0xff;
    dma_tx_ptr->M0AR = (u32)&tx_dummy_byte;
    tx_CCR &= ~DMA_MemoryInc_Enable;
  }
  dma_tx_ptr->NDTR = len;

  // new for STM32F4 DMA: it's required to clear interrupt flags before DMA channel is enabled again
  DMA_ClearFlag(dma_rx_ptr, dma_rx_irq_flags);
  DMA_ClearFlag(dma_tx_ptr, dma_tx_irq_flags);

  // enable DMA interrupt if callback function active
  if( callback != NULL ) {
    rx_CCR |= DMA_IT_TC;
    dma_rx_ptr->CR = rx_CCR;

    // start DMA transfer
    dma_tx_ptr->CR = tx_CCR | CCR_ENABLE;
  } else {
    rx_CCR &= ~DMA_IT_TC;
    dma_rx_ptr->CR = rx_CCR;

    // start DMA transfer
    dma_tx_ptr->CR = tx_CCR | CCR_ENABLE;

    // if no callback: wait until all bytes have been transmitted/received
    while( dma_rx_ptr->NDTR );
  }

  return 0; // no error;
}


/////////////////////////////////////////////////////////////////////////////
// Called when callback function has been defined and SPI transfer has finished
/////////////////////////////////////////////////////////////////////////////
MIOS32_SPI0_DMA_IRQHANDLER_FUNC
{
  DMA_ClearFlag(MIOS32_SPI0_DMA_RX_PTR, MIOS32_SPI0_DMA_RX_IRQ_FLAGS);

  if( spi_callback[0] != NULL )
    spi_callback[0]();
}

MIOS32_SPI1_DMA_IRQHANDLER_FUNC
{
  DMA_ClearFlag(MIOS32_SPI1_DMA_RX_PTR, MIOS32_SPI1_DMA_RX_IRQ_FLAGS);

  if( spi_callback[1] != NULL )
    spi_callback[1]();
}

MIOS32_SPI2_DMA_IRQHANDLER_FUNC
{
  DMA_ClearFlag(MIOS32_SPI2_DMA_RX_PTR, MIOS32_SPI2_DMA_RX_IRQ_FLAGS);

  if( spi_callback[2] != NULL )
    spi_callback[2]();
}

//! \}

#endif /* MIOS32_DONT_USE_SPI */

