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

#define MIOS32_I2S_WS_PORT   GPIOA
#define MIOS32_I2S_WS_PIN    GPIO_Pin_4
#define MIOS32_I2S_WS_PINSRC GPIO_PinSource4

#define MIOS32_I2S_CK_PORT   GPIOC
#define MIOS32_I2S_CK_PIN    GPIO_Pin_10
#define MIOS32_I2S_CK_PINSRC GPIO_PinSource10

#define MIOS32_I2S_SD_PORT   GPIOC
#define MIOS32_I2S_SD_PIN    GPIO_Pin_12
#define MIOS32_I2S_SD_PINSRC GPIO_PinSource12

#define MIOS32_I2S_MCLK_PORT GPIOC
#define MIOS32_I2S_MCLK_PIN  GPIO_Pin_7
#define MIOS32_I2S_MCLK_PINSRC GPIO_PinSource7



#if defined(MIOS32_BOARD_STM32F4DISCOVERY) || defined(MIOS32_BOARD_MBHP_CORE_STM32F4)
// I2C configuration
// Thanks to http://www.mind-dump.net/configuring-the-stm32f4-discovery-for-audio for this info!

#define CODEC_I2C I2C1
#define CODEC_I2C_SCL_PORT GPIOB
#define CODEC_I2C_SCL_PIN  GPIO_Pin_6
#define CODEC_I2C_SDA_PORT GPIOB
#define CODEC_I2C_SDA_PIN  GPIO_Pin_9
#define CODEC_RESET_PORT   GPIOD
#define CODEC_RESET_PIN    GPIO_Pin_4


#define CORE_I2C_ADDRESS 0x33
#define CODEC_I2C_ADDRESS 0x94

#define CODEC_MAPBYTE_INC 0x80

//register map bytes for CS42L22 (see page 35)
#define CODEC_MAP_CHIP_ID 0x01
#define CODEC_MAP_PWR_CTRL1 0x02
#define CODEC_MAP_PWR_CTRL2 0x04
#define CODEC_MAP_CLK_CTRL  0x05
#define CODEC_MAP_IF_CTRL1  0x06
#define CODEC_MAP_IF_CTRL2  0x07
#define CODEC_MAP_PASSTHROUGH_A_SELECT 0x08
#define CODEC_MAP_PASSTHROUGH_B_SELECT 0x09
#define CODEC_MAP_ANALOG_SET 0x0A
#define CODEC_MAP_PASSTHROUGH_GANG_CTRL 0x0C
#define CODEC_MAP_PLAYBACK_CTRL1 0x0D
#define CODEC_MAP_MISC_CTRL 0x0E
#define CODEC_MAP_PLAYBACK_CTRL2 0x0F
#define CODEC_MAP_PASSTHROUGH_A_VOL 0x14
#define CODEC_MAP_PASSTHROUGH_B_VOL 0x15
#define CODEC_MAP_PCMA_VOL 0x1A
#define CODEC_MAP_PCMB_VOL 0x1B
#define CODEC_MAP_BEEP_FREQ_ONTIME 0x1C
#define CODEC_MAP_BEEP_VOL_OFFTIME 0x1D
#define CODEC_MAP_BEEP_TONE_CFG 0x1E
#define CODEC_MAP_TONE_CTRL 0x1F
#define CODEC_MAP_MASTER_A_VOL 0x20
#define CODEC_MAP_MASTER_B_VOL 0x21
#define CODEC_MAP_HP_A_VOL 0x22
#define CODEC_MAP_HP_B_VOL 0x23
#define CODEC_MAP_SPEAK_A_VOL 0x24
#define CODEC_MAP_SPEAK_B_VOL 0x25
#define CODEC_MAP_CH_MIX_SWAP 0x26
#define CODEC_MAP_LIMIT_CTRL1 0x27
#define CODEC_MAP_LIMIT_CTRL2 0x28
#define CODEC_MAP_LIMIT_ATTACK 0x29
#define CODEC_MAP_OVFL_CLK_STATUS 0x2E
#define CODEC_MAP_BATT_COMP 0x2F
#define CODEC_MAP_VP_BATT_LEVEL 0x30
#define CODEC_MAP_SPEAK_STATUS 0x31
#define CODEC_MAP_CHARGE_PUMP_FREQ 0x34

static void codec_init();
static void send_codec_ctrl(uint8_t controlBytes[], uint8_t numBytes);
static uint8_t read_codec_register(uint8_t mapByte);
#endif


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
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;

  GPIO_InitStructure.GPIO_Pin = MIOS32_I2S_WS_PIN;
  GPIO_Init(MIOS32_I2S_WS_PORT, &GPIO_InitStructure);
  GPIO_PinAFConfig(MIOS32_I2S_WS_PORT, MIOS32_I2S_WS_PINSRC, GPIO_AF_SPI3);

  GPIO_InitStructure.GPIO_Pin = MIOS32_I2S_CK_PIN;
  GPIO_Init(MIOS32_I2S_CK_PORT, &GPIO_InitStructure);
  GPIO_PinAFConfig(MIOS32_I2S_CK_PORT, MIOS32_I2S_CK_PINSRC, GPIO_AF_SPI3);

  GPIO_InitStructure.GPIO_Pin = MIOS32_I2S_SD_PIN;
  GPIO_Init(MIOS32_I2S_SD_PORT, &GPIO_InitStructure);
  GPIO_PinAFConfig(MIOS32_I2S_SD_PORT, MIOS32_I2S_SD_PINSRC, GPIO_AF_SPI3);

#if MIOS32_I2S_MCLK_ENABLE
  GPIO_InitStructure.GPIO_Pin = MIOS32_I2S_MCLK_PIN;
  GPIO_Init(MIOS32_I2S_MCLK_PORT, &GPIO_InitStructure);
  GPIO_PinAFConfig(MIOS32_I2S_MCLK_PORT, MIOS32_I2S_MCLK_PINSRC, GPIO_AF_SPI3);
#endif

#if defined(MIOS32_BOARD_STM32F4DISCOVERY) || defined(MIOS32_BOARD_MBHP_CORE_STM32F4)
  // configure I2C pins to access the CS43L22 configuration registers
  // Thanks to http://www.mind-dump.net/configuring-the-stm32f4-discovery-for-audio for this info!
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;

  GPIO_InitStructure.GPIO_Pin = CODEC_I2C_SCL_PIN;
  GPIO_Init(CODEC_I2C_SCL_PORT, &GPIO_InitStructure);
  GPIO_PinAFConfig(CODEC_I2C_SCL_PORT, GPIO_PinSource6, GPIO_AF_I2C1);

  GPIO_InitStructure.GPIO_Pin = CODEC_I2C_SDA_PIN;
  GPIO_Init(CODEC_I2C_SDA_PORT, &GPIO_InitStructure);
  GPIO_PinAFConfig(CODEC_I2C_SDA_PORT, GPIO_PinSource9, GPIO_AF_I2C1);

  // CS43L22 reset pin
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_Pin = CODEC_RESET_PIN;
  GPIO_Init(CODEC_RESET_PORT, &GPIO_InitStructure);

  GPIO_ResetBits(CODEC_RESET_PORT, CODEC_RESET_PIN); // activate reset
#endif


  // I2S initialisation
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI3, ENABLE);
  RCC_PLLI2SCmd(ENABLE); // new for STM32F4: enable I2S PLL
  I2S_InitTypeDef I2S_InitStructure;
  I2S_StructInit(&I2S_InitStructure);
  I2S_InitStructure.I2S_Standard = MIOS32_I2S_STANDARD;
  I2S_InitStructure.I2S_DataFormat = MIOS32_I2S_DATA_FORMAT;
  I2S_InitStructure.I2S_MCLKOutput = MIOS32_I2S_MCLK_ENABLE ? I2S_MCLKOutput_Enable : I2S_MCLKOutput_Disable;
  I2S_InitStructure.I2S_AudioFreq  = (u16)(MIOS32_I2S_AUDIO_FREQ);
  I2S_InitStructure.I2S_CPOL = I2S_CPOL_Low; // configuration required as well?
  I2S_InitStructure.I2S_Mode = I2S_Mode_MasterTx;
  I2S_Init(SPI3, &I2S_InitStructure);
  I2S_Cmd(SPI3, ENABLE);

  // DMA Configuration for SPI Tx Event
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA1, ENABLE);

  DMA_InitTypeDef DMA_InitStructure;
  DMA_StructInit(&DMA_InitStructure);

  DMA_Cmd(DMA1_Stream5, DISABLE);
  DMA_ClearFlag(DMA1_Stream5, DMA_FLAG_TCIF5 | DMA_FLAG_TEIF5 | DMA_FLAG_HTIF5 | DMA_FLAG_FEIF5);
  DMA_InitStructure.DMA_Channel = DMA_Channel_0;
  DMA_InitStructure.DMA_PeripheralBaseAddr = (u32)&SPI3->DR;
  //  DMA_InitStructure.DMA_MemoryBaseAddr = ...; // configured in MIOS32_I2S_Start()
  DMA_InitStructure.DMA_DIR = DMA_DIR_MemoryToPeripheral;
  //  DMA_InitStructure.DMA_BufferSize = ...; // configured in MIOS32_I2S_Start()
  DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
  DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
  DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
  DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
  DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;
  DMA_InitStructure.DMA_Priority = DMA_Priority_Medium;
  DMA_Init(DMA1_Stream5, &DMA_InitStructure);
  // DMA_Cmd(DMA1_Stream5, ENABLE); // done on MIOS32_I2S_Start()

  // trigger interrupt when transfer half complete/complete
  DMA_ITConfig(DMA1_Stream5, DMA_IT_HT | DMA_IT_TC, ENABLE);

  // enable SPI interrupts to DMA
  SPI_I2S_DMACmd(SPI3, SPI_I2S_DMAReq_Tx, ENABLE);

  // Configure and enable DMA interrupt
  MIOS32_IRQ_Install(DMA1_Stream5_IRQn, MIOS32_IRQ_I2S_DMA_PRIORITY);

#if defined(MIOS32_BOARD_STM32F4DISCOVERY) || defined(MIOS32_BOARD_MBHP_CORE_STM32F4)
  // configure I2C
  // Thanks to http://www.mind-dump.net/configuring-the-stm32f4-discovery-for-audio for this info!
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1, ENABLE);
  I2C_DeInit(CODEC_I2C);
  I2C_InitTypeDef I2C_InitStructure;
  I2C_StructInit(&I2C_InitStructure);
  I2C_InitStructure.I2C_ClockSpeed = 100000;
  I2C_InitStructure.I2C_Mode = I2C_Mode_I2C;
  I2C_InitStructure.I2C_OwnAddress1 = CORE_I2C_ADDRESS;
  I2C_InitStructure.I2C_Ack = I2C_Ack_Enable;
  I2C_InitStructure.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
  I2C_InitStructure.I2C_DutyCycle = I2C_DutyCycle_2;

  I2C_Cmd(CODEC_I2C, ENABLE);
  I2C_Init(CODEC_I2C, &I2C_InitStructure);

  codec_init();
#endif

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
  DMA_Cmd(DMA1_Stream5, DISABLE);

  // ensure that IRQ flag is cleared (so that DMA IRQ isn't invoked by accident while this function is called)
  DMA_ClearFlag(DMA1_Stream5, DMA_FLAG_TCIF5 | DMA_FLAG_TEIF5 | DMA_FLAG_HTIF5 | DMA_FLAG_FEIF5);

  // take over new callback function
  buffer_reload_callback = _callback;

  // take over new buffer pointer/length
  DMA1_Stream5->M0AR = (u32)buffer;
  DMA1_Stream5->NDTR = 2 * len;
  DMA_Cmd(DMA1_Stream5, ENABLE);

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! Stops DMA driven I2S transfers
//! \return < 0 if initialisation failed
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_I2S_Stop(void)
{
  // disable DMA
  DMA_Cmd(DMA1_Stream5, DISABLE);

  return 0; // no error
}



/////////////////////////////////////////////////////////////////////////////
//! DMA1 Channel interrupt is triggered on HT and TC interrupts
//! \note shouldn't be called directly from application
/////////////////////////////////////////////////////////////////////////////
void DMA1_Stream5_IRQHandler(void)
{
  // execute callback function depending on pending flag(s)

  if( DMA1->HISR & DMA_FLAG_HTIF5 ) {
    DMA1->HIFCR = DMA_FLAG_HTIF5;
    // state 0: lower sample buffer range has been transfered and can be updated
    buffer_reload_callback(0);
  }

  if( DMA1->HISR & DMA_FLAG_TCIF5 ) {
    DMA1->HIFCR = DMA_FLAG_TCIF5;
    // state 1: upper sample buffer range has been transfered and can be updated
    buffer_reload_callback(1);
  }
}



#if defined(MIOS32_BOARD_STM32F4DISCOVERY) || defined(MIOS32_BOARD_MBHP_CORE_STM32F4)
// I2C configuration
// Thanks to http://www.mind-dump.net/configuring-the-stm32f4-discovery-for-audio for this code!
static void codec_init()
{
  uint8_t CodecCommandBuffer[5];

  uint8_t regValue = 0xFF;

  GPIO_SetBits(GPIOD, CODEC_RESET_PIN);
  MIOS32_DELAY_Wait_uS(1000);

  //keep codec OFF
  CodecCommandBuffer[0] = CODEC_MAP_PLAYBACK_CTRL1;
  CodecCommandBuffer[1] = 0x01;
  send_codec_ctrl(CodecCommandBuffer, 2);

  //begin initialization sequence (p. 32)
  CodecCommandBuffer[0] = 0x00;
  CodecCommandBuffer[1] = 0x99;
  send_codec_ctrl(CodecCommandBuffer, 2);

  CodecCommandBuffer[0] = 0x47;
  CodecCommandBuffer[1] = 0x80;
  send_codec_ctrl(CodecCommandBuffer, 2);

  regValue = read_codec_register(0x32);

  CodecCommandBuffer[0] = 0x32;
  CodecCommandBuffer[1] = regValue | 0x80;
  send_codec_ctrl(CodecCommandBuffer, 2);

  regValue = read_codec_register(0x32);

  CodecCommandBuffer[0] = 0x32;
  CodecCommandBuffer[1] = regValue & (~0x80);
  send_codec_ctrl(CodecCommandBuffer, 2);

  CodecCommandBuffer[0] = 0x00;
  CodecCommandBuffer[1] = 0x00;
  send_codec_ctrl(CodecCommandBuffer, 2);
  //end of initialization sequence

  CodecCommandBuffer[0] = CODEC_MAP_PWR_CTRL2;
  CodecCommandBuffer[1] = 0xAF;
  send_codec_ctrl(CodecCommandBuffer, 2);

  CodecCommandBuffer[0] = CODEC_MAP_PLAYBACK_CTRL1;
  CodecCommandBuffer[1] = 0x70;
  send_codec_ctrl(CodecCommandBuffer, 2);

  CodecCommandBuffer[0] = CODEC_MAP_CLK_CTRL;
  CodecCommandBuffer[1] = 0x81; //auto detect clock
  send_codec_ctrl(CodecCommandBuffer, 2);

  CodecCommandBuffer[0] = CODEC_MAP_IF_CTRL1;
  CodecCommandBuffer[1] = 0x07;
  send_codec_ctrl(CodecCommandBuffer, 2);

  CodecCommandBuffer[0] = 0x0A;
  CodecCommandBuffer[1] = 0x00;
  send_codec_ctrl(CodecCommandBuffer, 2);

  CodecCommandBuffer[0] = 0x27;
  CodecCommandBuffer[1] = 0x00;
  send_codec_ctrl(CodecCommandBuffer, 2);

  CodecCommandBuffer[0] = 0x1A | CODEC_MAPBYTE_INC;
  CodecCommandBuffer[1] = 0x0A;
  CodecCommandBuffer[2] = 0x0A;
  send_codec_ctrl(CodecCommandBuffer, 3);

  CodecCommandBuffer[0] = 0x1F;
  CodecCommandBuffer[1] = 0x0F;
  send_codec_ctrl(CodecCommandBuffer, 2);

  CodecCommandBuffer[0] = CODEC_MAP_PWR_CTRL1;
  CodecCommandBuffer[1] = 0x9E;
  send_codec_ctrl(CodecCommandBuffer, 2);
}

static void send_codec_ctrl(uint8_t controlBytes[], uint8_t numBytes)
{
  uint8_t bytesSent=0;

  while (I2C_GetFlagStatus(CODEC_I2C, I2C_FLAG_BUSY)) {
    //just wait until no longer busy
  }

  I2C_GenerateSTART(CODEC_I2C, ENABLE);
  while (!I2C_GetFlagStatus(CODEC_I2C, I2C_FLAG_SB)) {
    //wait for generation of start condition
  }
  I2C_Send7bitAddress(CODEC_I2C, CODEC_I2C_ADDRESS, I2C_Direction_Transmitter);
  while (!I2C_CheckEvent(CODEC_I2C, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED)) {
    //wait for end of address transmission
  }
  while (bytesSent < numBytes) {
    I2C_SendData(CODEC_I2C, controlBytes[bytesSent]);
    bytesSent++;
    while (!I2C_CheckEvent(CODEC_I2C, I2C_EVENT_MASTER_BYTE_TRANSMITTING)) {
      //wait for transmission of byte
    }
  }
  while(!I2C_GetFlagStatus(CODEC_I2C, I2C_FLAG_BTF)) {
    //wait until it's finished sending before creating STOP
  }
  I2C_GenerateSTOP(CODEC_I2C, ENABLE);

}

static uint8_t read_codec_register(uint8_t mapbyte)
{
  uint8_t receivedByte = 0;

  while (I2C_GetFlagStatus(CODEC_I2C, I2C_FLAG_BUSY)) {
    //just wait until no longer busy
  }

  I2C_GenerateSTART(CODEC_I2C, ENABLE);
  while (!I2C_GetFlagStatus(CODEC_I2C, I2C_FLAG_SB)) {
    //wait for generation of start condition
  }

  I2C_Send7bitAddress(CODEC_I2C, CODEC_I2C_ADDRESS, I2C_Direction_Transmitter);
  while (!I2C_CheckEvent(CODEC_I2C, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED)) {
    //wait for end of address transmission
  }

  I2C_SendData(CODEC_I2C, mapbyte); //sets the transmitter address
  while (!I2C_CheckEvent(CODEC_I2C, I2C_EVENT_MASTER_BYTE_TRANSMITTING)) {
    //wait for transmission of byte
  }

  I2C_GenerateSTOP(CODEC_I2C, ENABLE);

  while (I2C_GetFlagStatus(CODEC_I2C, I2C_FLAG_BUSY)) {
    //just wait until no longer busy
  }

  I2C_AcknowledgeConfig(CODEC_I2C, DISABLE);

  I2C_GenerateSTART(CODEC_I2C, ENABLE);
  while (!I2C_GetFlagStatus(CODEC_I2C, I2C_FLAG_SB)) {
    //wait for generation of start condition
  }

  I2C_Send7bitAddress(CODEC_I2C, CODEC_I2C_ADDRESS, I2C_Direction_Receiver);
  while (!I2C_CheckEvent(CODEC_I2C, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED)) {
    //wait for end of address transmission
  }

  while (!I2C_CheckEvent(CODEC_I2C, I2C_EVENT_MASTER_BYTE_RECEIVED)) {
    //wait until byte arrived
  }
  receivedByte = I2C_ReceiveData(CODEC_I2C);

  I2C_GenerateSTOP(CODEC_I2C, ENABLE);

  return receivedByte;
}
#endif


//! \}

#endif /* MIOS32_USE_I2S */
