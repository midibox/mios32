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
//! after a burst scan
//! After the scan is completed, the ADC interrupt will be invoked
//! to calculate the final (optionally oversampled) values, and to transfer 
//! them into the ain_pin_values[] array if the value change is greater than 
//! the defined MIOS32_AIN_DEADBAND (can be changed with MIOS32_AIN_DeadbandSet() during runtime)
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
//! It's possible to define an oversampling rate, which leads to an accumulation 
//! of conversion results to increase the resolution and to improve the accuracy.
//!
//! A special idle mechanism has been integrated which avoids sporadical
//! jittering values of AIN pins which could happen due to EMI issues.<BR>
//! MIOS32_AIN_IDLE_CTR defines the number of conversions after which the
//! pin goes into idle state if no conversion exceeded the MIOS32_AIN_DEADBAND.
//! In idle state, MIOS32_AIN_DEADBAND_IDLE will be used instead, which is greater
//! (accordingly the pin will be less sensitive). The pin will use the original
//! MIOS32_AIN_DEADBAND again once MIOS32_AIN_DEADBAND_IDLE has been exceeded.<BR>
//! This feature can be disabled by setting MIOS32_AIN_DEADBAND_IDLE to 0
//! in your mios32_config.h file.
//!
//! Especially due to the bad layout of the LPCXPRESSO board a simple spike
//! filter has been added which filters values which are much higher (> 64) than
//! the previous value. This filter is currently always enabled (no MIOS32_* flag)
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

// this module can be optionally disabled in a local mios32_config.h file (included from mios32.h)
#if !defined(MIOS32_DONT_USE_AIN)


/////////////////////////////////////////////////////////////////////////////
// Global variables
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Local defines
/////////////////////////////////////////////////////////////////////////////

// ADC peripheral clocked at CCLK/4
#define ADC_PERIPHERAL_FRQ (MIOS32_SYS_CPU_FREQUENCY/4)

// According to the datasheet, the analog part shouldn't be clocked by more than 13 MHz
// For higher impedances, even a lower frequency is desireable
// select 25 MHz / 25 = 1 MHz
# define ADC_CLKDIV (25-1)

// number of channels currently defined locally
// we could derive it from number of bits in MIOS32_AIN_CHANNEL_MASK?
// Currently this number is only available as variable: num_used_channels
#define NUM_CHANNELS_MAX 8

// derive number of AIN pins from number of ADC and mux channels
#define NUM_AIN_PINS (NUM_CHANNELS_MAX * (1 << MIOS32_AIN_MUX_PINS))

// how many words are required for this ain_pin_changed[] array?
// each word contains 32 bits, therefore:
#define NUM_CHANGE_WORDS (1 + (NUM_AIN_PINS>>5))

// we always enable a spike filter which is required due to the bad LPCXPRESSO layout!
#define SPIKE_FILTER 1
// if spike filter enabled: filter if the difference of two consecutive values is greater than SPIKE_FILTER_DIFF
#define SPIKE_FILTER_DIFF 64


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

#if MIOS32_AIN_CHANNEL_MASK

static u16 adc_conversion_values[NUM_CHANNELS_MAX];

static u8 ain_deadband = MIOS32_AIN_DEADBAND;

static u8  num_used_channels;

static u8  oversampling_ctr;
static u8  mux_ctr;

static u16 ain_pin_values[NUM_AIN_PINS];
static u32 ain_pin_changed[NUM_CHANGE_WORDS];

#if MIOS32_AIN_DEADBAND_IDLE
static u16 ain_pin_idle_ctr[NUM_AIN_PINS];
#endif

#if SPIKE_FILTER
// last conversion result before spike filtering
static u16 ain_pin_spike_last_value[NUM_AIN_PINS];
#endif

#endif

static s32 (*service_prepare_callback)(void);



/////////////////////////////////////////////////////////////////////////////
// Local constants
/////////////////////////////////////////////////////////////////////////////

#if MIOS32_AIN_MUX_PINS >= 1
// MUX selection:
// to match with the layout driven selection order used 
// by MBHP_AINX4 modules, we have to output following values:
const u8 mux_selection_order[8] = { 5, 7, 3, 1, 2, 4, 0, 6 };
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

  ain_deadband = MIOS32_AIN_DEADBAND;

  // disable service prepare callback function
  service_prepare_callback = NULL;

  // clear arrays and variables
  for(i=0; i<NUM_CHANNELS_MAX; ++i)
    adc_conversion_values[i] = 0;

  for(i=0; i<NUM_AIN_PINS; ++i) {
    ain_pin_values[i] = 0;
#if MIOS32_AIN_DEADBAND_IDLE
    ain_pin_idle_ctr[i] = 0;
#endif
#if SPIKE_FILTER
    ain_pin_spike_last_value[i] = 0;
#endif
  }
  for(i=0; i<NUM_CHANGE_WORDS; ++i) {
    ain_pin_changed[i] = 0;
  }
  oversampling_ctr = mux_ctr = 0;

  // check how many channels are used according to the CHANNEL_MASK
  num_used_channels = 0;
  int last_channel = 0;
  for(i=0; i<NUM_CHANNELS_MAX; ++i) {
    if( MIOS32_AIN_CHANNEL_MASK & (1 << i) ) {
      last_channel = i;
      ++num_used_channels;
    }
  }

  if( !num_used_channels )
    return -2; // no (valid) channel selected for conversion

  // enable ADC pins (let unselected pins untouched)
  if( MIOS32_AIN_CHANNEL_MASK & (1 << 0) ) {
    MIOS32_SYS_LPC_PINSEL (0, 23, 1); // AD0.0
    MIOS32_SYS_LPC_PINMODE(0, 23, 2);
  }
  if( MIOS32_AIN_CHANNEL_MASK & (1 << 1) ) {
    MIOS32_SYS_LPC_PINSEL (0, 24, 1); // AD0.1
    MIOS32_SYS_LPC_PINMODE(0, 24, 2);
  }
  if( MIOS32_AIN_CHANNEL_MASK & (1 << 2) ) {
    MIOS32_SYS_LPC_PINSEL (0, 25, 1); // AD0.2
    MIOS32_SYS_LPC_PINMODE(0, 25, 2);
  }
  if( MIOS32_AIN_CHANNEL_MASK & (1 << 3) ) {
    MIOS32_SYS_LPC_PINSEL (0, 26, 1); // AD0.3
    MIOS32_SYS_LPC_PINMODE(0, 26, 2);
  }
  if( MIOS32_AIN_CHANNEL_MASK & (1 << 4) ) {
    MIOS32_SYS_LPC_PINSEL (1, 30, 3); // AD0.4
    MIOS32_SYS_LPC_PINMODE(1, 30, 2);
  }
  if( MIOS32_AIN_CHANNEL_MASK & (1 << 5) ) {
    MIOS32_SYS_LPC_PINSEL (1, 31, 3); // AD0.5
    MIOS32_SYS_LPC_PINMODE(1, 31, 2);
  }
  if( MIOS32_AIN_CHANNEL_MASK & (1 << 6) ) {
    MIOS32_SYS_LPC_PINSEL (0,  3, 2); // AD0.6
    MIOS32_SYS_LPC_PINMODE(0,  3, 2);
  }
  if( MIOS32_AIN_CHANNEL_MASK & (1 << 7) ) {
    MIOS32_SYS_LPC_PINSEL (0,  2, 2); // AD0.7
    MIOS32_SYS_LPC_PINMODE(0,  2, 2);
  }

  // select channels, clock divider; enable BURST mode and disable power-down mode
  LPC_ADC->ADCR = (((MIOS32_AIN_CHANNEL_MASK)&0xff) << 0) | ((ADC_CLKDIV) << 8) | (1 << 16) | (1 << 21);

  // trigger interrupt when last channel has been converted
  LPC_ADC->ADINTEN = (1 << last_channel);

  // TODO: use NVIC_SetPriority; will require some encoding...
  MIOS32_IRQ_Install(ADC_IRQn, MIOS32_IRQ_AIN_DMA_PRIORITY);

  return 0; // no error
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
//! \return the deadband which is used to notify changes
//! \return < 0 on error
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_AIN_DeadbandGet(void)
{
#if !MIOS32_AIN_CHANNEL_MASK
  return -1; // no analog input selected
#else
  return ain_deadband;
#endif
}

/////////////////////////////////////////////////////////////////////////////
//! Sets the difference between last and current pot value which has to
//! be achieved to trigger the callback function passed to AINSER_Handler()
//! \return < 0 on error
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_AIN_DeadbandSet(u8 deadband)
{
#if !MIOS32_AIN_CHANNEL_MASK
  return -1; // no analog input selected
#else
  ain_deadband = deadband;

  return 0; // no error
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
    for(chn=0; chn<num_used_channels; ++chn) {
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
	callback(pin, pin_value);
      }
    }
  }

  // execute optional "service prepare" callback function
  // skip scan if it returns a value >= 1
  if( service_prepare_callback != NULL && service_prepare_callback() >= 1 )
    return 0; // scan skipped - no error

  return 0; // no error
#endif
}



/////////////////////////////////////////////////////////////////////////////
//! ADC interrupt is triggered when all ADC channels have been converted
//! \note shouldn't be called directly from application
/////////////////////////////////////////////////////////////////////////////
#if MIOS32_AIN_CHANNEL_MASK
void ADC_IRQHandler(void)
{
  // transfer values into result registers
  if( oversampling_ctr == 0 ) { // init sum whenever we restarted
    volatile u32 *src_ptr = (volatile u32*)&LPC_ADC->ADDR0;
    u16 *dst_ptr = (u16 *)adc_conversion_values;
    int i;
    for(i=0; i<num_used_channels; ++i)
      *dst_ptr++ = ((*src_ptr++) >> 4) & 0xfff;
  } else {
    volatile u32 *src_ptr = (volatile u32*)&LPC_ADC->ADDR0;
    u16 *dst_ptr = (u16 *)adc_conversion_values;
    int i;
    for(i=0; i<num_used_channels; ++i) // add to sum
      *dst_ptr++ += ((*src_ptr++) >> 4) & 0xfff;
  }

  // increment oversampling counter, reset if sample rate reached
  if( ++oversampling_ctr >= MIOS32_AIN_OVERSAMPLING_RATE )
    oversampling_ctr = 0;

  // whenever we reached the last sample:
  // copy conversion values to ain_pin_values if difference > deadband
  if( oversampling_ctr == 0 ) {
    int i;
#if MIOS32_MF_NUM && !defined(MIOS32_DONT_USE_MF)
    u16 ain_deltas[NUM_CHANNELS_MAX];
    u16 *ain_deltas_ptr = (u16 *)ain_deltas;
#endif
    u8 pin_offset = num_used_channels * mux_ctr;
    u8 bit_offset = pin_offset & 0x1f;
    u8 word_offset = pin_offset >> 5;
    u16 *src_ptr = (u16 *)adc_conversion_values;
    u16 *dst_ptr = (u16 *)&ain_pin_values[pin_offset];

#if MIOS32_AIN_DEADBAND_IDLE
    u16 *idle_ctr_ptr = (u16 *)&ain_pin_idle_ctr[pin_offset];
#endif
#if SPIKE_FILTER
    u16 *spike_last_value_ptr = (u16 *)&ain_pin_spike_last_value[pin_offset];
#endif

    for(i=0; i<num_used_channels; ++i) {
#if MIOS32_AIN_DEADBAND_IDLE
      u16 deadband = *idle_ctr_ptr ? (ain_deadband) : (MIOS32_AIN_DEADBAND_IDLE);
#else
      u16 deadband = ain_deadband;
#endif

#if SPIKE_FILTER
      int spike_delta = 0;
      spike_delta = *spike_last_value_ptr - *src_ptr;
      if( spike_delta < 0 ) spike_delta = -spike_delta;
      *spike_last_value_ptr = *src_ptr;

      if( spike_delta < SPIKE_FILTER_DIFF ) {
#else
      if( 1 ) {
#endif
	// takeover new value if difference to old value is outside the deadband
        int delta = *src_ptr - *dst_ptr;
        if( delta < 0 ) delta = -delta;

#if MIOS32_MF_NUM && !defined(MIOS32_DONT_USE_MF)
	if( (*ain_deltas_ptr++ = delta) > deadband ) {
#else
	if( delta > deadband ) {
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
      }

      // switch to next results
      ++dst_ptr;
      ++src_ptr;
#if MIOS32_AIN_DEADBAND_IDLE
      ++idle_ctr_ptr;
#endif
#if SPIKE_FILTER
      ++spike_last_value_ptr;
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
#error "TODO"
    //MIOS32_AIN_MUX0_PORT->BSRR = (mux_value & (1 << 0)) ? MIOS32_AIN_MUX0_PIN : (MIOS32_AIN_MUX0_PIN<<16);
#endif
#if MIOS32_AIN_MUX_PINS >= 2
#error "TODO"
    //MIOS32_AIN_MUX1_PORT->BSRR = (mux_value & (1 << 1)) ? MIOS32_AIN_MUX1_PIN : (MIOS32_AIN_MUX1_PIN<<16);
#endif
#if MIOS32_AIN_MUX_PINS >= 3
#error "TODO"
    //MIOS32_AIN_MUX2_PORT->BSRR = (mux_value & (1 << 2)) ? MIOS32_AIN_MUX2_PIN : (MIOS32_AIN_MUX2_PIN<<16);
#endif

    // TODO: we need a special state here: MUX settings are changed during running BURST
    // this will lead to unstable results
    // instead we should do a "dummy conversion"
  }
#endif

}
#endif

//! \}

#endif /* MIOS32_DONT_USE_AIN */
