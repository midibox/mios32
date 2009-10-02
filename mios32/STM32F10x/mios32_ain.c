// $Id$
//! \defgroup MIOS32_AIN
//!
//! AIN driver for MIOS32
//! 
//! ADC channels which should be converted have to be specified with a
//! mask (MIOS32_AIN_CHANNEL_MASK), which has to be added to the application 
//! specific mios32_config.h file.
//!
//! Conversion results are transfered into the adc_conversion_values[] array
//! by DMA1 Channel 1 to relieve the CPU.
//!
//! After the scan is completed, the DMA channel interrupt will be invoked
//! to calculate the final (optionally oversampled) values, and to transfer 
//! them into the ain_pin_values[] array if the value change is greater than 
//! the defined MIOS32_AIN_DEADBAND.
//!
//! Value changes (within the deadband) will be notified to the MIOS32_AIN_Handler().
//! This function isn't called directly by the application, but it's part 
//! of the programming model framework.
//! E.g., if the "traditional" framework is used, the AIN handler will be called
//! each mS, and it will call the 'APP_AIN_NotifyChange(u32 pin, u32 pin_value)' 
//! hook on pin changes.<BR>
//! The AIN handler will trigger a new scan after all pins have been checked.
//!
//! Analog channels can be multiplexed via MBHP_AINX4 modules. The selection
//! pins can be connected to any free GPIO pin, the assignments have to be
//! added to the mios32_config.h file.<BR>
//! Usually the 3 selection lines are connected to J5C.A0/1/2 of the core module.
//! Together with the 8 analog channels at J5A/B this results into 64 analog pins.
//!
//! The AIN driver is flexible enough to increase the number of ADC channels
//! to not less than 16 (connected to J5A/B/C and J16). Together with 4 AINX4 
//! multiplexers this results into 128 analog channels.
//!
//! It's possible to define an oversampling rate, which leads to an accumulation 
//! of conversion results to increase the resolution and to improve the accuracy.
//!
//! A special idle mechanism has been integrated which avoids sporadical
//! jittering values of AIN pins which could happen due to EMI issues.<BR>
//! MIOS32_AIN_IDLE_CTR defines the number of conversion after which the
//! pin goes into idle state if no conversion exceeded the MIOS32_AIN_DEADBAND.
//! In idle state, MIOS32_AIN_DEADBAND_IDLE will be used instead, which is greater
//! (accordingly the pin will be less sensitive). The pin will use the original
//! MIOS32_AIN_DEADBAND again once MIOS32_AIN_DEADBAND_IDLE has been exceeded.<BR>
//! This feature can be disabled by setting MIOS32_AIN_DEADBAND_IDLE to 0
//! in your mios32_config.h file.
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
#if !defined(MIOS32_DONT_USE_AIN)


/////////////////////////////////////////////////////////////////////////////
// Global variables
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Local defines
/////////////////////////////////////////////////////////////////////////////

// number of channels currently defined locally
// we could derive it from number of bits in MIOS32_AIN_CHANNEL_MASK?
// Currently this number is only available as variable: num_used_channels
#define NUM_CHANNELS_MAX 16

// derive number of AIN pins from number of ADC and mux channels
#define NUM_AIN_PINS (NUM_CHANNELS_MAX * (1 << MIOS32_AIN_MUX_PINS))

// how many words are required for this ain_pin_changed[] array?
// each word contains 32 bits, therefore:
#define NUM_CHANGE_WORDS (1 + (NUM_AIN_PINS>>5))

/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

#if MIOS32_AIN_CHANNEL_MASK

// following two arrays are word aligned, so that DMA can transfer two hwords at once
static u16 adc_conversion_values[NUM_CHANNELS_MAX] __attribute__((aligned(4)));
#if MIOS32_AIN_OVERSAMPLING_RATE >= 2
static u16 adc_conversion_values_sum[NUM_CHANNELS_MAX] __attribute__((aligned(4)));
#endif

static u8  num_used_channels;
static u8  num_channels;

static u8  oversampling_ctr;
static u8  mux_ctr;

static u16 ain_pin_values[NUM_AIN_PINS];
static u32 ain_pin_changed[NUM_CHANGE_WORDS];

#if MIOS32_AIN_DEADBAND_IDLE
static u16 ain_pin_idle_ctr[NUM_AIN_PINS];
#endif

#endif

static s32 (*service_prepare_callback)(void);



/////////////////////////////////////////////////////////////////////////////
// Local constants
/////////////////////////////////////////////////////////////////////////////

#if MIOS32_AIN_CHANNEL_MASK

// this table maps ADC channels to J5.Ax and J16 pins
static const u8 adc_chn_map[16] = {
  ADC_Channel_10, // J5A.A0
  ADC_Channel_11, // J5A.A1
  ADC_Channel_12, // J5A.A2
  ADC_Channel_13, // J5A.A3
  ADC_Channel_0,  // J5B.A4
  ADC_Channel_1,  // J5B.A5
  ADC_Channel_2,  // J5B.A6
  ADC_Channel_3,  // J5B.A7
  ADC_Channel_14, // J5C.A8
  ADC_Channel_15, // J5C.A9
  ADC_Channel_8,  // J5C.A10
  ADC_Channel_9,  // J5C.A11
  ADC_Channel_4,  // J16.RC
  ADC_Channel_5,  // J16.SC
  ADC_Channel_6,  // J16.SI
  ADC_Channel_7   // J16.SO
};


#if MIOS32_AIN_MUX_PINS >= 1
// MUX selection:
// to match with the layout driven selection order used 
// by MBHP_AINX4 modules, we have to output following values:
const u8 mux_selection_order[8] = { 5, 7, 3, 1, 2, 4, 0, 6 };
#endif

#endif


/////////////////////////////////////////////////////////////////////////////
//! Initializes AIN driver
//! \param[in] mode currently only mode 0 supported
//! \return < 0 if initialisation failed
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_AIN_Init(u32 mode)
{
  // currently only mode 0 supported
  if( mode != 0 )
    return -1; // unsupported mode

#if !MIOS32_AIN_CHANNEL_MASK
  return -1; // no AIN pins selected
#else
  int i;

  // disable service prepare callback function
  service_prepare_callback = NULL;

  // clear arrays and variables
  for(i=0; i<NUM_CHANNELS_MAX; ++i) {
    adc_conversion_values[i] = 0;
#if MIOS32_AIN_OVERSAMPLING_RATE >= 2
    adc_conversion_values_sum[i] = 0;
#endif
  }
  for(i=0; i<NUM_AIN_PINS; ++i) {
    ain_pin_values[i] = 0;
#if MIOS32_AIN_DEADBAND_IDLE
    ain_pin_idle_ctr[i] = 0;
#endif
  }
  for(i=0; i<NUM_CHANGE_WORDS; ++i) {
    ain_pin_changed[i] = 0;
  }
  oversampling_ctr = mux_ctr = 0;


  // set analog pins
  GPIO_InitTypeDef GPIO_InitStructure;
  GPIO_StructInit(&GPIO_InitStructure);
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
  GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AIN;

  // J5A.0..3 -> Channel 10..13 -> Pin C0..3
  GPIO_InitStructure.GPIO_Pin = (MIOS32_AIN_CHANNEL_MASK & 0x000f) >> 0;
  GPIO_Init(GPIOC, &GPIO_InitStructure);

  // J5B.4..7 -> Channel 0..3 -> Pin A0..3
  GPIO_InitStructure.GPIO_Pin = (MIOS32_AIN_CHANNEL_MASK & 0x00f0) >> 4;
  GPIO_Init(GPIOA, &GPIO_InitStructure);

  // J5C.8..9 -> Channel 14..15 -> Pin C4..5
  GPIO_InitStructure.GPIO_Pin = (MIOS32_AIN_CHANNEL_MASK & 0x0300) >> 4;
  GPIO_Init(GPIOC, &GPIO_InitStructure);

  // J5C.A10..11 -> Channel 8..9 -> Pin B0..1
  GPIO_InitStructure.GPIO_Pin = (MIOS32_AIN_CHANNEL_MASK & 0x0c00) >> 10;
  GPIO_Init(GPIOB, &GPIO_InitStructure);

  // J5C.A8..11 -> Channel 10..13 -> Pin C0..C3
  GPIO_InitStructure.GPIO_Pin = (MIOS32_AIN_CHANNEL_MASK & 0x0f00) >> 8;
  GPIO_Init(GPIOC, &GPIO_InitStructure);

  // J16 -> Channel 4..7 -> Pin A4..7
  GPIO_InitStructure.GPIO_Pin = (MIOS32_AIN_CHANNEL_MASK & 0xf000) >> 8;
  GPIO_Init(GPIOA, &GPIO_InitStructure);


  // configure MUX pins if enabled
#if MIOS32_AIN_MUX_PINS >= 0
  GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;
#if MIOS32_AIN_MUX_PINS >= 1
  GPIO_InitStructure.GPIO_Pin = MIOS32_AIN_MUX0_PIN;
  GPIO_Init(MIOS32_AIN_MUX0_PORT, &GPIO_InitStructure);
#endif
#if MIOS32_AIN_MUX_PINS >= 2
  GPIO_InitStructure.GPIO_Pin = MIOS32_AIN_MUX1_PIN;
  GPIO_Init(MIOS32_AIN_MUX1_PORT, &GPIO_InitStructure);
#endif
#if MIOS32_AIN_MUX_PINS >= 3
  GPIO_InitStructure.GPIO_Pin = MIOS32_AIN_MUX2_PIN;
  GPIO_Init(MIOS32_AIN_MUX2_PORT, &GPIO_InitStructure);
#endif
#endif


  // enable ADC1/2 clock
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1 | RCC_APB2Periph_ADC2, ENABLE);

  // map channels to conversion slots depending on the channel selection mask
  // distribute this over the two ADCs, so that channels can be converted in parallel
  num_channels = 0;
  for(i=0; i<NUM_CHANNELS_MAX; ++i) {
    if( MIOS32_AIN_CHANNEL_MASK & (1 << i) ) {
      ADC_RegularChannelConfig(
        (num_channels & 1) ? ADC2 : ADC1, 
	adc_chn_map[i], 
	(num_channels>>1)+1, 
	ADC_SampleTime_239Cycles5);
      ++num_channels;
    }
  }

  // ensure that num_used_channels is an even value to keep ADC2 in synch with ADC1
  num_used_channels = num_channels;
  if( num_used_channels & 1 )
      ++num_used_channels;

  // configure ADCs
  ADC_InitTypeDef ADC_InitStructure;
  ADC_StructInit(&ADC_InitStructure);
  ADC_InitStructure.ADC_Mode = ADC_Mode_RegSimult;
  ADC_InitStructure.ADC_ScanConvMode = ENABLE;
  ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;
  ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
  ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
  ADC_InitStructure.ADC_NbrOfChannel = num_used_channels >> 1;
  ADC_Init(ADC1, &ADC_InitStructure);
  ADC_Init(ADC2, &ADC_InitStructure);

  // enable ADC2 external trigger conversion (to synch with ADC1)
  ADC_ExternalTrigConvCmd(ADC2, ENABLE);

  // enable ADC1->DMA request
  ADC_DMACmd(ADC1, ENABLE);


  // ADC1 calibration
  ADC_Cmd(ADC1, ENABLE);
  ADC_ResetCalibration(ADC1);
  while( ADC_GetResetCalibrationStatus(ADC1) );
  ADC_StartCalibration(ADC1);
  while( ADC_GetCalibrationStatus(ADC1) );

  // ADC2 calibration
  ADC_Cmd(ADC2, ENABLE);
  ADC_ResetCalibration(ADC2);
  while( ADC_GetResetCalibrationStatus(ADC2) );
  ADC_StartCalibration(ADC2);
  while( ADC_GetCalibrationStatus(ADC2) );


  // enable DMA1 clock
  RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);

  // configure DMA1 channel 1 to fetch data from ADC result register
  DMA_InitTypeDef DMA_InitStructure;
  DMA_StructInit(&DMA_InitStructure);
  DMA_DeInit(DMA1_Channel1);
  DMA_InitStructure.DMA_PeripheralBaseAddr = (u32)&ADC1->DR;
  DMA_InitStructure.DMA_MemoryBaseAddr = (u32)&adc_conversion_values;
  DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;
  DMA_InitStructure.DMA_BufferSize = num_used_channels >> 1; // number of conversions depends on number of used channels
  DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
  DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
  DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Word;
  DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Word;
  DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;
  DMA_InitStructure.DMA_Priority = DMA_Priority_High;
  DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
  DMA_Init(DMA1_Channel1, &DMA_InitStructure);
  DMA_Cmd(DMA1_Channel1, ENABLE);

  // trigger interrupt when all conversion values have been fetched
  DMA_ITConfig(DMA1_Channel1, DMA_IT_TC, ENABLE);

  // Configure and enable DMA interrupt  
  NVIC_InitTypeDef NVIC_InitStructure;
  NVIC_InitStructure.NVIC_IRQChannel = DMA1_Channel1_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = MIOS32_IRQ_AIN_DMA_PRIORITY; // defined in mios32_irq.h
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);

  // finally start initial conversion
  ADC_SoftwareStartConvCmd(ADC1, ENABLE);

  return 0;
#endif
}


/////////////////////////////////////////////////////////////////////////////
//! Installs an optional "Service Prepare" callback function, which is called
//! before all ADC channels are scanned.
//!
//! It is useful to switch additional multiplexers, to reconfigure ADC
//! pins of touch panels, etc.
//!
//! The scan will be started if the callback function returns 0
//!
//! The scan will be skipped if the callback function returns a value >= 1
//! so that it is possible to insert setup times while switching analog inputs.
//!
//! An usage example can be found unter $MIOS32_PATH/apps/examples/dog_g_touchpanel
//!
//! \param[in] *_callback_service_prepare pointer to callback function:<BR>
//! \code
//!    s32 AIN_ServicePrepare(void);
//! \endcode
//! \return < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_AIN_ServicePrepareCallback_Init(void *_service_prepare_callback)
{
  service_prepare_callback = _service_prepare_callback;

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
//! Returns value of an AIN Pin
//! \param[in] pin number
//! \return AIN pin value - resolution depends on the selected MIOS32_AIN_OVERSAMPLING_RATE!
//! \return -1 if pin doesn't exist
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_AIN_PinGet(u32 pin)
{
#if !MIOS32_AIN_CHANNEL_MASK
  return -1; // no analog input selected
#else
  // check if pin exists
  if( pin >= NUM_AIN_PINS )
    return -1;

  // return last conversion result which was outside the deadband
  return ain_pin_values[pin];
#endif
}


/////////////////////////////////////////////////////////////////////////////
//! Checks for pin changes, and calls given callback function with following parameters on pin changes:
//! \code
//!   void AIN_NotifyChanged(u32 pin, u16 value)
//! \endcode
//! \param[in] _callback pointer to callback function
//! \return < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_AIN_Handler(void *_callback)
{
  // no callback function?
  if( _callback == NULL )
    return -1;

#if !MIOS32_AIN_CHANNEL_MASK
  return -1; // no analog input selected
#else
  int chn, mux;
  void (*callback)(s32 pin, u16 value) = _callback;

  // exit if scan hasn't been finished yet
  if( mux_ctr || oversampling_ctr )
    return 0;

  // check for changed AIN conversion values
  for(mux=0; mux<(1 << MIOS32_AIN_MUX_PINS); ++mux) {
    for(chn=0; chn<num_channels; ++chn) {
      u32 pin = mux * num_used_channels + chn;
      u32 mask = 1 << (pin & 0x1f);
      if( ain_pin_changed[pin >> 5] & mask ) {
	MIOS32_IRQ_Disable();
	u32 pin_value = ain_pin_values[pin];
	ain_pin_changed[pin>>5] &= ~mask;
	MIOS32_IRQ_Enable();

	// call application hook
	// note that due to dual conversion approach, we have to convert the pin number
	// if an uneven number number of channels selected
	u8 app_pin = (num_channels & 1) ? (pin>>1) : pin;
	callback(app_pin, pin_value);
      }
    }
  }

  // execute optional "service prepare" callback function
  // skip scan if it returns a value >= 1
  if( service_prepare_callback != NULL && service_prepare_callback() >= 1 )
    return 0; // scan skipped - no error

  // start next scan
  ADC_SoftwareStartConvCmd(ADC1, ENABLE);

  return 0; // no error
#endif
}


/////////////////////////////////////////////////////////////////////////////
//! DMA channel interrupt is triggered when all ADC channels have been converted
//! \note shouldn't be called directly from application
/////////////////////////////////////////////////////////////////////////////
#if MIOS32_AIN_CHANNEL_MASK
void DMA1_Channel1_IRQHandler(void)
{
  int i;
  u16 *src_ptr, *dst_ptr;

  // clear the pending flag(s)
  DMA_ClearFlag(DMA1_FLAG_TC1 | DMA1_FLAG_TE1 | DMA1_FLAG_HT1 | DMA1_FLAG_GL1);

#if MIOS32_AIN_OVERSAMPLING_RATE >= 2
  // accumulate conversion result
  src_ptr = (u16 *)adc_conversion_values;
  dst_ptr = (u16 *)adc_conversion_values_sum;

  if( oversampling_ctr == 0 ) { // init sum whenever we restarted
    for(i=0; i<num_channels; ++i)
      *dst_ptr++ = *src_ptr++;
  } else {
    for(i=0; i<num_channels; ++i) // add to sum
      *dst_ptr++ += *src_ptr++;
  }

  // increment oversampling counter, reset if sample rate reached
  if( ++oversampling_ctr >= MIOS32_AIN_OVERSAMPLING_RATE )
    oversampling_ctr = 0;
#endif

  // whenever we reached the last sample:
  // copy conversion values to ain_pin_values if difference > deadband
  if( oversampling_ctr == 0 ) {
#if MIOS32_MF_NUM && !defined(MIOS32_DONT_USE_MF)
    u16 ain_deltas[NUM_CHANNELS_MAX];
    u16 *ain_deltas_ptr = (u16 *)ain_deltas;
#endif
    u8 pin_offset = num_used_channels * mux_ctr;
    u8 bit_offset = pin_offset & 0x1f;
    u8 word_offset = pin_offset >> 5;
#if MIOS32_AIN_OVERSAMPLING_RATE >= 2
    src_ptr = (u16 *)adc_conversion_values_sum;
#else
    src_ptr = (u16 *)adc_conversion_values;
#endif
    dst_ptr = (u16 *)&ain_pin_values[pin_offset];

#if MIOS32_AIN_DEADBAND_IDLE
    u16 *idle_ctr_ptr = (u16 *)&ain_pin_idle_ctr[pin_offset];
#endif

    for(i=0; i<num_channels; ++i) {
#if MIOS32_AIN_DEADBAND_IDLE
      u16 deadband = *idle_ctr_ptr ? (MIOS32_AIN_DEADBAND) : (MIOS32_AIN_DEADBAND_IDLE);
#else
      u16 deadband = MIOS32_AIN_DEADBAND;
#endif

      // takeover new value if difference to old value is outside the deadband
#if MIOS32_MF_NUM && !defined(MIOS32_DONT_USE_MF)
      if( (*ain_deltas_ptr++ = abs(*src_ptr - *dst_ptr)) > deadband ) {
#else
      if( abs(*src_ptr - *dst_ptr) > deadband ) {
#endif
	*dst_ptr = *src_ptr;
	ain_pin_changed[word_offset] |= (1 << bit_offset);
#if MIOS32_AIN_DEADBAND_IDLE
	*idle_ctr_ptr = MIOS32_AIN_IDLE_CTR;
#endif
      } else {
#if MIOS32_AIN_DEADBAND_IDLE
	if( *idle_ctr_ptr )
	  *idle_ctr_ptr -= 1;
#endif
      }

      // switch to next results
      ++dst_ptr;
      ++src_ptr;
#if MIOS32_AIN_DEADBAND_IDLE
      ++idle_ctr_ptr;
#endif

      // switch to next bit/word offset for "changed" flags
      if( ++bit_offset >= 32 ) {
	bit_offset = 0;
	++word_offset;
      }
    }

#if MIOS32_MF_NUM && !defined(MIOS32_DONT_USE_MF)
    // if motorfader driver enabled: forward conversion values + deltas
#if MIOS32_AIN_OVERSAMPLING_RATE >= 2
    u16 change_flag_mask = MIOS32_MF_Tick((u16 *)adc_conversion_values_sum, (u16 *)ain_deltas);
#else
    u16 change_flag_mask = MIOS32_MF_Tick((u16 *)adc_conversion_values, (u16 *)ain_deltas);
#endif
    // do an AND operation on all "changed" flags (MF driver takes control over these flags)
    ain_pin_changed[0] &= 0xffff0000 | change_flag_mask;
#endif
  }

#if MIOS32_AIN_MUX_PINS >= 1
  // select next mux channel whenever oversampling has finished
  if( oversampling_ctr == 0 ) {
    if( ++mux_ctr >= (1 << MIOS32_AIN_MUX_PINS) )
      mux_ctr = 0;

    // forward to GPIOs (used mapped value to ensure a correct order)
    u8 mux_value = mux_selection_order[mux_ctr];

#if MIOS32_AIN_MUX_PINS >= 1
    MIOS32_AIN_MUX0_PORT->BSRR = (mux_value & (1 << 0)) ? MIOS32_AIN_MUX0_PIN : (MIOS32_AIN_MUX0_PIN<<16);
#endif
#if MIOS32_AIN_MUX_PINS >= 2
    MIOS32_AIN_MUX1_PORT->BSRR = (mux_value & (1 << 1)) ? MIOS32_AIN_MUX1_PIN : (MIOS32_AIN_MUX1_PIN<<16);
#endif
#if MIOS32_AIN_MUX_PINS >= 3
    MIOS32_AIN_MUX2_PORT->BSRR = (mux_value & (1 << 2)) ? MIOS32_AIN_MUX2_PIN : (MIOS32_AIN_MUX2_PIN<<16);
#endif

    // TODO: check with MBHP_CORE_STM32 board, if we need a "settle" time before
    // starting conversion of new selected channels
  }
#endif

  // request next conversion so long oversampling/mux counter haven't reached the end
  if( mux_ctr || oversampling_ctr )
    ADC_SoftwareStartConvCmd(ADC1, ENABLE);
}
#endif

//! \}

#endif /* MIOS32_DONT_USE_AIN */
