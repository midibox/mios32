// $Id$
/*
 * MMC/SD Card Driver for MIOS32
 *
 * Approach:
 * SD Card is accessed via SPI1 (J16) or alternatively via SPI2 (J8/9)
 * A bitbanging solution (for using alternative ports) will be provided later.
 *
 * The card has to be supplied with 3.3V!
 *
 * The SDIO peripheral is not used to ensure compatibility with "mid density"
 * devices of the STM32 family, and future derivatives != STM32
 *
 * MIOS32_SDCARD_Init(0) has to be called only once to initialize the GPIO pins,
 * clocks, SPI and DMA
 *
 * MIOS32_SDCARD_PowerOn() should be called to connect with the SD Card. If an error
 * is returned, it can be assumed that no SD Card is connected. The function
 * can be called again (after a certain time) to retry a connection, resp. for
 * an auto-detection during runtime
 *
 * MIOS32_SDCARD_SectorRead/SectorWrite allow to read/write a 512 byte sector.
 *
 * If such an access returns an error, it can be assumed that the SD Card has
 * been disconnected during the transfer.
 *
 * Since DMA is used for serial transfers, Reading/Writing a sector typically
 * takes ca. 500 uS, accordingly the achievable transfer rate is ca. 1 MByte/s
 * (8 MBit/s)
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
#if !defined(MIOS32_DONT_USE_SDCARD)



/////////////////////////////////////////////////////////////////////////////
// Following local definitions depend on the selected SPI
/////////////////////////////////////////////////////////////////////////////

#if MIOS32_SDCARD_SPI == 1

# define MIOS32_SDCARD_CSN_PORT   GPIOA
# define MIOS32_SDCARD_CSN_PIN    GPIO_Pin_4
# define MIOS32_SDCARD_SCLK_PORT  GPIOA
# define MIOS32_SDCARD_SCLK_PIN   GPIO_Pin_5
# define MIOS32_SDCARD_DIN_PORT   GPIOA
# define MIOS32_SDCARD_DIN_PIN    GPIO_Pin_6   // connected to DO pin of SD Card!
# define MIOS32_SDCARD_DOUT_PORT  GPIOA
# define MIOS32_SDCARD_DOUT_PIN   GPIO_Pin_7   // connected to DI pin of SD Card!

# define MIOS32_SDCARD_SPI_PTR    SPI1
# define MIOS32_SDCARD_DMA_RX_PTR DMA1_Channel2
# define MIOS32_SDCARD_DMA_TX_PTR DMA1_Channel3

#elif MIOS32_SDCARD_SPI == 2

# define MIOS32_SDCARD_CSN_PORT   GPIOB
# define MIOS32_SDCARD_CSN_PIN    GPIO_Pin_12
# define MIOS32_SDCARD_SCLK_PORT  GPIOB
# define MIOS32_SDCARD_SCLK_PIN   GPIO_Pin_13
# define MIOS32_SDCARD_DIN_PORT   GPIOB
# define MIOS32_SDCARD_DIN_PIN    GPIO_Pin_14   // connected to DO pin of SD Card!
# define MIOS32_SDCARD_DOUT_PORT  GPIOB
# define MIOS32_SDCARD_DOUT_PIN   GPIO_Pin_15   // connected to DI pin of SD Card!

# define MIOS32_SDCARD_SPI_PTR    SPI2
# define MIOS32_SDCARD_DMA_RX_PTR DMA1_Channel4
# define MIOS32_SDCARD_DMA_TX_PTR DMA1_Channel5

#else
# error "Unsupported SPI peripheral number"
#endif


/////////////////////////////////////////////////////////////////////////////
// Local definitions
/////////////////////////////////////////////////////////////////////////////

#define SDCMD_GO_IDLE_STATE	0x40
#define SDCMD_GO_IDLE_STATE_CRC	0x95

#define SDCMD_SEND_OP_COND	0x41
#define SDCMD_SEND_OP_COND_CRC	0xf9

#define SDCMD_READ_SINGLE_BLOCK	0x51
#define SDCMD_READ_SINGLE_BLOCK_CRC 0xff

#define SDCMD_WRITE_SINGLE_BLOCK 0x58
#define SDCMD_WRITE_SINGLE_BLOCK_CRC 0xff



/////////////////////////////////////////////////////////////////////////////
// Help Macros
/////////////////////////////////////////////////////////////////////////////

#define PIN_CSN_0  { MIOS32_SDCARD_CSN_PORT->BRR  = MIOS32_SDCARD_CSN_PIN; }
#define PIN_CSN_1  { MIOS32_SDCARD_CSN_PORT->BSRR = MIOS32_SDCARD_CSN_PIN; }


/////////////////////////////////////////////////////////////////////////////
// Global variables
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Local prototypes
/////////////////////////////////////////////////////////////////////////////

static s32 MIOS32_SDCARD_SPI_Init(u8 fast);

static s32 MIOS32_SDCARD_TransferByte(u8 b);


/////////////////////////////////////////////////////////////////////////////
// Initializes SPI pins and peripheral to access MMC/SD Card
// IN: <mode>: currently only mode 0 supported
// OUT: returns < 0 if initialisation failed
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_SDCARD_Init(u32 mode)
{
  // currently only mode 0 supported
  if( mode != 0 )
    return -1; // unsupported mode

  // init GPIO structure
  GPIO_InitTypeDef GPIO_InitStructure;
  GPIO_StructInit(&GPIO_InitStructure);
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;

  // SCLK and DOUT are outputs assigned to alternate functions
  GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF_PP;
  GPIO_InitStructure.GPIO_Pin   = MIOS32_SDCARD_SCLK_PIN;
  GPIO_Init(MIOS32_SDCARD_SCLK_PORT, &GPIO_InitStructure);
  GPIO_InitStructure.GPIO_Pin   = MIOS32_SDCARD_DOUT_PIN;
  GPIO_Init(MIOS32_SDCARD_DOUT_PORT, &GPIO_InitStructure);

  // CSN output assigned to GPIO
  GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;
  GPIO_InitStructure.GPIO_Pin   = MIOS32_SDCARD_CSN_PIN;
  GPIO_Init(MIOS32_SDCARD_CSN_PORT, &GPIO_InitStructure);
  PIN_CSN_1;

  // DIN is input with pull-up
  GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IPU;
  GPIO_InitStructure.GPIO_Pin   = MIOS32_SDCARD_DIN_PIN;
  GPIO_Init(MIOS32_SDCARD_DIN_PORT, &GPIO_InitStructure);

  // enable SPI peripheral clock (APB2 == high speed)
#if MIOS32_SDCARD_SPI == 1
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, ENABLE);
#elif MIOS32_SDCARD_SPI == 2
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI2, ENABLE);
#endif

  // initialize SPI interface
  MIOS32_SDCARD_SPI_Init(0); // slow clock

  // enable DMA1 clock
  RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);

  // DMA Configuration for SPI Rx Event
  DMA_InitTypeDef DMA_InitStructure;
  DMA_StructInit(&DMA_InitStructure);
  DMA_Cmd(MIOS32_SDCARD_DMA_RX_PTR, DISABLE);
  DMA_InitStructure.DMA_PeripheralBaseAddr = (u32)&MIOS32_SDCARD_SPI_PTR->DR;
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
  DMA_Init(MIOS32_SDCARD_DMA_RX_PTR, &DMA_InitStructure);

  // DMA Configuration for SPI Tx Event
  // (partly re-using previous DMA setup)
  DMA_Cmd(MIOS32_SDCARD_DMA_TX_PTR, DISABLE);
  DMA_InitStructure.DMA_MemoryBaseAddr = 0; // will be configured later
  DMA_InitStructure.DMA_BufferSize = 0; // will be configured later
  DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralDST;
  DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
  DMA_Init(MIOS32_SDCARD_DMA_TX_PTR, &DMA_InitStructure);

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Connects to SD Card
// IN: -
// OUT: returns < 0 if initialisation sequence failed
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_SDCARD_PowerOn(void)
{
  s32 status;
  int i;

  // ensure that chip select line deactivated
  PIN_CSN_1;

  // initialize SPI interface
  MIOS32_SDCARD_SPI_Init(0); // slow clock

  // send 80 clock cycles to start up
  for(i=0; i<10; ++i)
    MIOS32_SDCARD_TransferByte(0xff);

  // activate chip select
  PIN_CSN_0;

  // wait for 1 mS
  MIOS32_DELAY_Wait_uS(1000);

  // send CMD0 to reset the media
  if( (status=MIOS32_SDCARD_SendSDCCmd(SDCMD_GO_IDLE_STATE, 0, SDCMD_GO_IDLE_STATE_CRC)) < 0 )
    return status; // return error code

  // deactivate chip select
  PIN_CSN_1;

  // send CMD1 to reset the media - this must be repeated until SD Card is fully initialized
  for(i=0; i<16384; ++i) {
    if( (status=MIOS32_SDCARD_SendSDCCmd(SDCMD_SEND_OP_COND, 0, SDCMD_SEND_OP_COND_CRC)) < 0 )
      return status; // return error code

    if( status == 0x00 )
      break;
  }

  if( i == 16384 ) {
    PIN_CSN_1; // deactivate chip select
    return -2; // return error code
  }

  // initialize SPI interface for fast mode
  MIOS32_SDCARD_SPI_Init(1); // fast clock

  // deactivate chip select
  PIN_CSN_1;

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Disconnects from SD Card
// IN: -
// OUT: returns < 0 if initialisation sequence failed
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_SDCARD_PowerOff(void)
{
  // TODO

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Sends command to SD card
// IN: command in <cmd>
//     32bit address in <addr>
//     precalculated CRC in <crc>
// OUT: returns >= 0x00 if command has been sent successfully (contains received byte)
//      return -1 if no response from SD Card (timeout)
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_SDCARD_SendSDCCmd(u8 cmd, u32 addr, u8 crc)
{
  int i;
  u8 ret;

  // activate chip select
  PIN_CSN_0;

  // transfer to card
  MIOS32_SDCARD_TransferByte(cmd);
  MIOS32_SDCARD_TransferByte((addr >> 24) & 0xff);
  MIOS32_SDCARD_TransferByte((addr >> 16) & 0xff);
  MIOS32_SDCARD_TransferByte((addr >>  8) & 0xff);
  MIOS32_SDCARD_TransferByte((addr >>  0) & 0xff);
  MIOS32_SDCARD_TransferByte(crc);

  // wait for response
  for(i=0; i<8; ++i) {
    if( (ret=MIOS32_SDCARD_TransferByte(0xff)) != 0xff )
      break;
  }

  // required clocking (see spec)
  MIOS32_SDCARD_TransferByte(0xff);

  // deactivate chip-select on timeout, and return error code
  if( i == 8 ) {
    PIN_CSN_1; // deactivate chip select
    return -1;
  }

  // else return received value - don't deactivate chip select (for continuous access)
  return ret;
}


/////////////////////////////////////////////////////////////////////////////
// Reads 512 bytes from selected sector
// IN: 32bit sector in <sector>
//     pointer to buffer in <buffer>
// OUT: returns 0 if whole sector has been successfully read
//      returns -error if error occured during read operation
//        -Bit 0              - In idle state if 1
//        -Bit 1              - Erase Reset if 1
//        -Bit 2              - Illgal Command if 1
//        -Bit 3              - Com CRC Error if 1
//        -Bit 4              - Erase Sequence Error if 1
//        -Bit 5              - Address Error if 1
//        -Bit 6              - Parameter Error if 1
//        -Bit 7              - Not used, always '0'
//      returns -256 if timeout during command has been sent
//      returns -257 if timeout while waiting for start token
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_SDCARD_SectorRead(u32 sector, u8 *buffer)
{
  s32 status;
  int i;

  if( (status=MIOS32_SDCARD_SendSDCCmd(SDCMD_READ_SINGLE_BLOCK, sector << 9, SDCMD_READ_SINGLE_BLOCK_CRC)) )
    return (status < 0) ? -256 : status; // return timeout indicator or error flags

  // wait for start token of the data block
  for(i=0; i<65536; ++i) { // TODO: check if sufficient
    u8 ret = MIOS32_SDCARD_TransferByte(0xff);
    if( ret != 0xff )
      break;
  }
  if( i == 65536 ) {
    PIN_CSN_1; // deactivate chip select
    return -257; // return error code
  }

  // read 512 bytes via DMA
  DMA_Cmd(MIOS32_SDCARD_DMA_RX_PTR, DISABLE);
  MIOS32_SDCARD_DMA_RX_PTR->CMAR = (u32)buffer;
  MIOS32_SDCARD_DMA_RX_PTR->CNDTR = 512;
  MIOS32_SDCARD_DMA_RX_PTR->CCR |= DMA_MemoryInc_Enable; // enable memory addr. increment
  DMA_Cmd(MIOS32_SDCARD_DMA_RX_PTR, ENABLE);

  DMA_Cmd(MIOS32_SDCARD_DMA_TX_PTR, DISABLE);
  u8 dummy_byte = 0xff;
  MIOS32_SDCARD_DMA_TX_PTR->CMAR = (u32)&dummy_byte;
  MIOS32_SDCARD_DMA_TX_PTR->CNDTR = 512;
  MIOS32_SDCARD_DMA_TX_PTR->CCR &= ~DMA_MemoryInc_Enable; // disable memory addr. increment - bytes read from dummy buffer
  DMA_Cmd(MIOS32_SDCARD_DMA_TX_PTR, ENABLE);

  // wait until all bytes have been received
  while( MIOS32_SDCARD_DMA_RX_PTR->CNDTR );

  // read (and ignore) CRC
  MIOS32_SDCARD_TransferByte(0xff);
  MIOS32_SDCARD_TransferByte(0xff);

  // required for clocking (see spec)
  MIOS32_SDCARD_TransferByte(0xff);

  // deactivate chip select
  PIN_CSN_1;

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Writes 512 bytes into selected sector
// IN: 32bit sector in <sector>
//     pointer to buffer in <buffer>
// OUT: returns 0 if whole sector has been successfully written
//      return -error if error occured during write operation
//        -Bit 0              - In idle state if 1
//        -Bit 1              - Erase Reset if 1
//        -Bit 2              - Illgal Command if 1
//        -Bit 3              - Com CRC Error if 1
//        -Bit 4              - Erase Sequence Error if 1
//        -Bit 5              - Address Error if 1
//        -Bit 6              - Parameter Error if 1
//        -Bit 7              - Not used, always '0'
//      returns -256 if timeout during command has been sent
//      returns -257 if write operation not accepted
//      returns -258 if timeout during write operation
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_SDCARD_SectorWrite(u32 sector, u8 *buffer)
{
  s32 status;
  int i;

  if( (status=MIOS32_SDCARD_SendSDCCmd(SDCMD_WRITE_SINGLE_BLOCK, sector << 9, SDCMD_WRITE_SINGLE_BLOCK_CRC)) )
    return (status < 0) ? -256 : status; // return timeout indicator or error flags

  // send start token
  MIOS32_SDCARD_TransferByte(0xfe);

  // send 512 bytes of data via DMA
  DMA_Cmd(MIOS32_SDCARD_DMA_RX_PTR, DISABLE);
  u8 dummy_byte = 0xff;
  MIOS32_SDCARD_DMA_RX_PTR->CMAR = (u32)&dummy_byte;
  MIOS32_SDCARD_DMA_RX_PTR->CNDTR = 512;
  MIOS32_SDCARD_DMA_RX_PTR->CCR &= ~DMA_MemoryInc_Enable; // disable memory addr. increment - bytes written into dummy buffer
  DMA_Cmd(MIOS32_SDCARD_DMA_RX_PTR, ENABLE);

  DMA_Cmd(MIOS32_SDCARD_DMA_TX_PTR, DISABLE);
  MIOS32_SDCARD_DMA_TX_PTR->CMAR = (u32)buffer;
  MIOS32_SDCARD_DMA_TX_PTR->CNDTR = 512;
  MIOS32_SDCARD_DMA_TX_PTR->CCR &= ~DMA_MemoryInc_Enable; // disable memory addr. increment - bytes read from dummy buffer
  MIOS32_SDCARD_DMA_TX_PTR->CCR |= DMA_MemoryInc_Enable; // enable memory addr. increment
  DMA_Cmd(MIOS32_SDCARD_DMA_TX_PTR, ENABLE);

  // wait until all bytes have been transmitted/received
  while( MIOS32_SDCARD_DMA_RX_PTR->CNDTR );

  // send CRC
  MIOS32_SDCARD_TransferByte(0xff);
  MIOS32_SDCARD_TransferByte(0xff);

  // read response
  u8 response = MIOS32_SDCARD_TransferByte(0xff);
  if( (response & 0x0f) != 0x5 ) {
    PIN_CSN_1; // deactivate chip select
    return -257; // return error code (write operation not accepted)
  }

  // wait for write completion
  for(i=0; i<65536; ++i) { // TODO: check if sufficient
    u8 ret = MIOS32_SDCARD_TransferByte(0xff);
    if( ret != 0x00 )
      break;
  }
  if( i == 65536 ) {
    PIN_CSN_1; // deactivate chip select
    return -258; // return error code
  }

  // required for clocking (see spec)
  MIOS32_SDCARD_TransferByte(0xff);

  // deactivate chip select
  PIN_CSN_1;

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Local function: initialize SPI
/////////////////////////////////////////////////////////////////////////////
static s32 MIOS32_SDCARD_SPI_Init(u8 fast)
{
  // SPI configuration
  SPI_InitTypeDef SPI_InitStructure;
  SPI_StructInit(&SPI_InitStructure);
  SPI_InitStructure.SPI_Direction           = SPI_Direction_2Lines_FullDuplex;
  SPI_InitStructure.SPI_Mode                = SPI_Mode_Master;
  SPI_InitStructure.SPI_DataSize            = SPI_DataSize_8b;
  SPI_InitStructure.SPI_CPOL                = SPI_CPOL_High;
  SPI_InitStructure.SPI_CPHA                = SPI_CPHA_2Edge;
  SPI_InitStructure.SPI_NSS                 = SPI_NSS_Soft;
#if MIOS32_SDCARD_SPI == 2
  SPI_InitStructure.SPI_BaudRatePrescaler   = fast ? SPI_BaudRatePrescaler_2 : SPI_BaudRatePrescaler_128; // ca. 55 nS / 2 uS period @ 72/2 MHz (SPI2 located in APB1 domain)
#else
  SPI_InitStructure.SPI_BaudRatePrescaler   = fast ? SPI_BaudRatePrescaler_4 : SPI_BaudRatePrescaler_256; // ca. 55 nS / 2 uS period @ 72 MHz
#endif
  SPI_InitStructure.SPI_FirstBit            = SPI_FirstBit_MSB;
  SPI_InitStructure.SPI_CRCPolynomial       = 7;
  SPI_Init(MIOS32_SDCARD_SPI_PTR, &SPI_InitStructure);

  // enable SPI
  SPI_Cmd(MIOS32_SDCARD_SPI_PTR, ENABLE);

  // enable SPI interrupts to DMA
  SPI_I2S_DMACmd(MIOS32_SDCARD_SPI_PTR, SPI_I2S_DMAReq_Tx | SPI_I2S_DMAReq_Rx, ENABLE);

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Local function: send single byte to SD Card, return received byte
/////////////////////////////////////////////////////////////////////////////
static s32 MIOS32_SDCARD_TransferByte(u8 b)
{
  // send byte
  MIOS32_SDCARD_SPI_PTR->DR = b;
  // wait until SPI transfer finished
  while( MIOS32_SDCARD_SPI_PTR->SR & SPI_I2S_FLAG_BSY );
  // return received byte
  return MIOS32_SDCARD_SPI_PTR->DR;
}


#endif /* MIOS32_DONT_USE_SDCARD */

