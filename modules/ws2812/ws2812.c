// $Id$
//! \defgroup WS2812
//!
//! WS2812 LED strip driver
//!
//! This driver allows to control an (almost) unlimited number of WS2812 based
//! RGB LEDs connected in a chain. Only the available RAM limits the number!
//!
//! Unfortunately the WS2812 protocol can't be serviced by a regular serial
//! peripheral without causing a high CPU load. But there are various methods
//! to overcome this problem, a nice overview can be found at this (german) page:
//! http://www.mikrocontroller.net/articles/WS2812_Ansteuerung
//!
//! In this driver, the PWM based approach is used. A timer is running at
//! 800 kHz (1.25 uS period time), and the high time will be set to
//! 0.35 uS or 0.90 uS in order to send a logic-0 or logic-1
//! 
//! This approach is inspired from: http://mikrocontroller.bplaced.net/wordpress/?page_id=3665
//!
//! In order to avoid that the CPU is loaded with switching the duty cycle for each
//! bit during the serial transfer, we just use the DMA controller to perform the
//! register write operations in background. This results into a high memory consumption,
//! because for each bit a 16bit number has to be stored in memory, resulting into 48 bytes
//! per RGB LED.
//!
//! But the advantages are clearly that the CPU is not involved and can run other
//! (more useful) tasks. Since the DMA is configured in circular mode, the LED strip
//! will be periodically loaded with the current RGB values which are stored in the
//! memory buffer.
//!
//!
//! Currently this driver is only supported for the MBHP_CORE_STM32F4 module.
//! We take TIM4, since it isn't used by MIOS32 (yet), and pin PB6 (available at J4B.SC)
//! since the second IIC port is normally not used by applications.
//! This pin has already a 2.2k pull-up resistor on the MBHP_CORE_STM32F4 board,
//! which is required to pull the output to 5V (the pin is configured in open-drain mode)
//! 
//! Note that the power-supply pins of the LED strip shouldn't be connected to J4B,
//! because it can consume a lot of power depending on the LED brightness.
//! Each RGB LED can draw up to 20 mA!
//! Hence, for the MBHP_CORE_STM32F4 module, an external 5V PSU is recommended,
//! which is either only used for the LED strip, or for the entire module.
//! 
//! 
//! Update 2018-01-05: reset phase has been enhanced from ca. 50 uS to 300 uS
//! See also http://midibox.org/forums/topic/19752-rgb-hue-sweep/?page=2
//! 
//! \{
/* ==========================================================================
 *
 *  Copyright (C) 2015 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

/////////////////////////////////////////////////////////////////////////////
// Include files
/////////////////////////////////////////////////////////////////////////////

#include <mios32.h>
#include <math.h>

#include "ws2812.h"


/////////////////////////////////////////////////////////////////////////////
// Local definitions
/////////////////////////////////////////////////////////////////////////////

#if defined(MIOS32_FAMILY_STM32F4xx)
#define WS2812_SUPPORTED 1
#else
#warning "WS2812 driver not supported for this derivative yet!"
#define WS2812_SUPPORTED 0
#endif


#define WS2812_TIM_OUT_PORT     GPIOB           // J4B.SC (middle pin of the 10 pin socket)
#define WS2812_TIM_OUT_PIN      GPIO_Pin_6
#define WS2812_TIM              TIM4
#define WS2812_TIM_CCR          WS2812_TIM->CCR1
#define WS2812_TIM_DMA_TRG      TIM_DMA_CC1
#define WS2812_TIM_REMAP_FUNC   { GPIO_PinAFConfig(GPIOB, GPIO_PinSource6, GPIO_AF_TIM4); }
#define WS2812_TIM_CLK_FUNC     { RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE); }

// timers clocked at CPU/2 clock, WS2812 protocol requires 800 kHz (1.25 uS period)
#define WS2812_TIM_PERIOD       ((MIOS32_SYS_CPU_FREQUENCY/2) / 800000)
#define WS2812_TIM_CC_RESET     0
#define WS2812_TIM_CC_IDLE      WS2812_TIM_PERIOD
#define WS2812_TIM_CC_LOW       (u16)((WS2812_TIM_PERIOD - 1) * 0.28) // 28% -> ca. 0.35 uS
#define WS2812_TIM_CC_HIGH      (u16)((WS2812_TIM_PERIOD - 1) * 0.74) // 74% -> ca. 0.90 uS

// DMA channel (DMA1 Stream 0, Channel 2 - fortunately DMA1_Stream0 not used by any other MIOS32 driver yet...!)
#define WS2812_DMA_PTR          DMA1_Stream0
#define WS2812_DMA_CHN          DMA_Channel_2
#define WS2812_DMA_IRQ_CHANNEL  DMA1_Stream0_IRQn
#define WS2812_DMA_IRQHandler   DMA1_Stream0_IRQHandler


#define WS2812_RESET_CYCLES 10 // number of cycles required for reset frame


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

// organized as a double buffer, switching between lower and upper half
// one buffer sends 24 words to a LED, which will take 30 uS
static u16 ws2812_send_double_buffer[2*24];

// state counter:
// - first 10 (WS2812_RESET_CYCLES) ticks used for reset frame
// - tick 10..WS2812_NUM_LEDS+10: send RGB values for each led
//   - each even number will use lower half of the buffer
//   - each odd number will use upper half of the buffer
// 
static u16 ws2812_state_ctr;

// RGB values for all LEDs
static u8 ws2812_rgb_values[WS2812_NUM_LEDS][3];


/////////////////////////////////////////////////////////////////////////////
// Local Prototypes
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
//! Initializes WS2812 driver
//! Should be called from Init() during startup
//! \param[in] mode if 0, the entire driver will be configured, if 1 only the LEDs
//! will be cleared
//! \return < 0 if initialisation failed
/////////////////////////////////////////////////////////////////////////////
s32 WS2812_Init(u32 mode)
{
#if !WS2812_SUPPORTED
  return -1;
#else
  ws2812_state_ctr = 0;
  {
    int i;
    for(i=0; i<2*24; ++i) {
      ws2812_send_double_buffer[i] = WS2812_TIM_CC_RESET;
    }

    for(i=0; i<WS2812_NUM_LEDS; ++i) {
      ws2812_rgb_values[i][0] = 0;
      ws2812_rgb_values[i][1] = 0;
      ws2812_rgb_values[i][2] = 0;
    }
  }

  if( mode == 0 ) {
    // WS2812 coding: see following nice overview page: http://www.mikrocontroller.net/articles/WS2812_Ansteuerung
    // We take the timer based approach since TIM4 isn't used by MIOS32 (yet), and J4B.SC is normally not used by apps
    // Programming Example for STM32F4: 
    {
      WS2812_TIM_REMAP_FUNC;

      // configure timer pin
      GPIO_InitTypeDef GPIO_InitStructure;
      GPIO_StructInit(&GPIO_InitStructure);
      GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;

      GPIO_InitStructure.GPIO_Pin = WS2812_TIM_OUT_PIN;
      GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF;
      GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;
      GPIO_Init(WS2812_TIM_OUT_PORT, &GPIO_InitStructure);

      WS2812_TIM_CLK_FUNC;

      // Timer configuration
      TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
      TIM_TimeBaseStructInit(&TIM_TimeBaseStructure);
      TIM_TimeBaseStructure.TIM_Period = WS2812_TIM_PERIOD - 1;
      TIM_TimeBaseStructure.TIM_Prescaler = 0;
      TIM_TimeBaseStructure.TIM_ClockDivision = 0;
      TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
      TIM_TimeBaseInit(WS2812_TIM, &TIM_TimeBaseStructure);

      TIM_OCInitTypeDef TIM_OCInitStructure;
      TIM_OCStructInit(&TIM_OCInitStructure);
      TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
      TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
      TIM_OCInitStructure.TIM_Pulse = WS2812_TIM_CC_RESET; // this is the capture/compare value which defines the duty cycle - start with 0 (reset)
      TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
      TIM_OC1Init(WS2812_TIM, &TIM_OCInitStructure);
      TIM_OC1PreloadConfig(WS2812_TIM, TIM_OCPreload_Enable);
      TIM_ARRPreloadConfig(WS2812_TIM, ENABLE);

      TIM_DMACmd(WS2812_TIM, WS2812_TIM_DMA_TRG, ENABLE);

      TIM_Cmd(WS2812_TIM, ENABLE);
    }

    {
      DMA_Cmd(WS2812_DMA_PTR, DISABLE);
      DMA_ClearFlag(WS2812_DMA_PTR, DMA_FLAG_TCIF0 | DMA_FLAG_TEIF0 | DMA_FLAG_HTIF0 | DMA_FLAG_FEIF0);

      DMA_InitTypeDef DMA_InitStructure;
      DMA_StructInit(&DMA_InitStructure);
      DMA_InitStructure.DMA_Channel = WS2812_DMA_CHN;
      DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;
      DMA_InitStructure.DMA_DIR = DMA_DIR_MemoryToPeripheral;
      DMA_InitStructure.DMA_Memory0BaseAddr = (u32)&ws2812_send_double_buffer[0];
      DMA_InitStructure.DMA_BufferSize = 2*24;
      DMA_InitStructure.DMA_PeripheralBaseAddr = (u32)&WS2812_TIM_CCR;
      DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
      DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
      DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
      DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
      DMA_InitStructure.DMA_Priority = DMA_Priority_VeryHigh;
      DMA_Init(WS2812_DMA_PTR, &DMA_InitStructure);

      // trigger interrupt when transfer half complete/complete
      DMA_ITConfig(WS2812_DMA_PTR, DMA_IT_HT | DMA_IT_TC, ENABLE);

      // Configure and enable DMA interrupt
      MIOS32_IRQ_Install(WS2812_DMA_IRQ_CHANNEL, MIOS32_IRQ_PRIO_HIGHEST);

      DMA_Cmd(WS2812_DMA_PTR, ENABLE);
    }
  }

  return 0; // no error
#endif
}


/////////////////////////////////////////////////////////////////////////////
//! DMA Channel interrupt is triggered on HT and TC interrupts
/////////////////////////////////////////////////////////////////////////////
#if WS2812_SUPPORTED
void WS2812_DMA_IRQHandler_SetPWM(u16 *buffer)
{
  if( ws2812_state_ctr < WS2812_RESET_CYCLES ) {
    int i;
    for(i=0; i<24; ++i)
      *(buffer++) = WS2812_TIM_CC_RESET;
  } else {
    int i, j;
    u8 *rgb_values = &ws2812_rgb_values[ws2812_state_ctr-WS2812_RESET_CYCLES][0];
    u8 mask;

    for(i=0; i<3; ++i, ++rgb_values) {
      u8 value = *(rgb_values);
      for(j=0, mask=0x80; j<8; ++j, mask >>= 1) {
	*(buffer++) = (value & mask) ? WS2812_TIM_CC_HIGH : WS2812_TIM_CC_LOW;
      }
    }
  }

  if( ++ws2812_state_ctr >= (WS2812_RESET_CYCLES + WS2812_NUM_LEDS) ) {
    ws2812_state_ctr = 0;
  }
}

void WS2812_DMA_IRQHandler(void)
{
  // execute SetPWM function depending on pending flag(s)
  if( DMA1->LISR & DMA_FLAG_HTIF0 ) {
    DMA1->LIFCR = DMA_FLAG_HTIF0;

    // state 0: lower buffer range has been transfered and can be updated
    WS2812_DMA_IRQHandler_SetPWM(&ws2812_send_double_buffer[0]);
  }

  if( DMA1->LISR & DMA_FLAG_TCIF0 ) {
    DMA1->LIFCR = DMA_FLAG_TCIF0;

    // state 1: upper buffer range has been transfered and can be updated
    WS2812_DMA_IRQHandler_SetPWM(&ws2812_send_double_buffer[24]);
  }

  DMA1->LIFCR = DMA_FLAG_TEIF0 | DMA_FLAG_FEIF0;

  //DMA_ClearFlag(WS2812_DMA_PTR, (DMA_FLAG_TCIF0 | DMA_FLAG_TEIF0 | DMA_FLAG_HTIF0 | DMA_FLAG_FEIF0));
}
#endif

/////////////////////////////////////////////////////////////////////////////
//! Sets a single colour of the RGB LED
//! \param[in] led should be in the range 0..WS2812_NUM_LEDS-1
//! \param[in] colour 0=R, 1=G, 2=B
//! \param[in] value 0..255
//! \return < 0 if invalid LED or colour
/////////////////////////////////////////////////////////////////////////////
s32 WS2812_LED_SetRGB(u16 led, u8 colour, u8 value)
{
#if !WS2812_SUPPORTED
  return -1;
#else
  if( led >= WS2812_NUM_LEDS )
    return -1; // unsupported LED

  u8 colour_ix = 0;
  if( colour == 0 )
    colour_ix = 1;
  else if( colour == 1 )
    colour_ix = 0;
  else if( colour == 2 )
    colour_ix = 2;
  else
    return -2; // unsupported colour

  u8 *rgb_values = &ws2812_rgb_values[led][0];
  rgb_values[colour_ix] = value;
  return value;
#endif
}


/////////////////////////////////////////////////////////////////////////////
//! Returns a single colour of the RGB LED
//! \param[in] led should be in the range 0..WS2812_NUM_LEDS-1
//! \param[in] colour 0=R, 1=G, 2=B
//! \return < 0 if invalid LED or colour
/////////////////////////////////////////////////////////////////////////////
s32 WS2812_LED_GetRGB(u16 led, u8 colour)
{
#if !WS2812_SUPPORTED
  return -1;
#else
  if( led >= WS2812_NUM_LEDS )
    return -1; // unsupported LED

  u8 colour_ix = 0;
  if( colour == 0 )
    colour_ix = 1;
  else if( colour == 1 )
    colour_ix = 0;
  else if( colour == 2 )
    colour_ix = 2;
  else
    return -2; // unsupported colour

  u8 *rgb_values = &ws2812_rgb_values[led][0];
  return rgb_values[colour_ix];
#endif
}


/////////////////////////////////////////////////////////////////////////////
//! Configures the LED according to a HSV value (Hue/Saturation/Value)
//! \param[in] led should be in the range 0..WS2812_NUM_LEDS-1
//! \param[in] h the hue (0..360.0)
//! \param[in] s the saturation (0.0..1.0)
//! \param[in] v the brightness (0.0..1.0)
//! \return < 0 if invalid LED
/////////////////////////////////////////////////////////////////////////////
s32 WS2812_LED_SetHSV(u16 led, float h, float s, float v)
{
  // from https://www.cs.rit.edu/~ncs/color/t_convert.html
  int i;
  float f, p, q, t;
  float r, g, b;

  if( led >= WS2812_NUM_LEDS )
    return -1; // unsupported LED

  if( s == 0 ) {
    // achromatic (grey)
    r = g = b = v;
  } else {
    h /= 60;			// sector 0 to 5
    i = floor( h );
    f = h - i;			// factorial part of h
    p = v * ( 1 - s );
    q = v * ( 1 - s * f );
    t = v * ( 1 - s * ( 1 - f ) );

    switch( i ) {
    case 0:
      r = v;
      g = t;
      b = p;
      break;
    case 1:
      r = q;
      g = v;
      b = p;
      break;
    case 2:
      r = p;
      g = v;
      b = t;
      break;
    case 3:
      r = p;
      g = q;
      b = v;
      break;
    case 4:
      r = t;
      g = p;
      b = v;
      break;
    default: // case 5:
      r = v;
      g = p;
      b = q;
      break;
    }
  }

  s32 status = 0;
  status |= WS2812_LED_SetRGB(led, 0, r*255);
  status |= WS2812_LED_SetRGB(led, 1, g*255);
  status |= WS2812_LED_SetRGB(led, 2, b*255);

  return status;
}


/////////////////////////////////////////////////////////////////////////////
//! Returns the HSV values of the LED (derived from RGB parameters)
//! \param[in] led should be in the range 0..WS2812_NUM_LEDS-1
//! \param[out] h the hue (0..360.0)
//! \param[out] s the saturation (0.0..1.0)
//! \param[out] v the brightness (0.0..1.0)
//! \return < 0 if invalid LED
/////////////////////////////////////////////////////////////////////////////
s32 WS2812_LED_GetHSV(u16 led, float *h, float *s, float *v)
{
  // from https://www.cs.rit.edu/~ncs/color/t_convert.html
  float min, max, delta;

  if( led >= WS2812_NUM_LEDS )
    return -1; // unsupported LED

  float r = WS2812_LED_GetRGB(led, 0) / 255.0;
  float g = WS2812_LED_GetRGB(led, 1) / 255.0;
  float b = WS2812_LED_GetRGB(led, 2) / 255.0;

  //min = MIN( r, g, b );
  min = r;
  if( g < min ) min = g;
  if( b < min ) min = b;

  //max = MAX( r, g, b );
  max = r;
  if( g > max ) max = g;
  if( b > max ) max = b;

  *v = max;				// v
  delta = max - min;
  if( max != 0 )
    *s = delta / max;		// s
  else {
    // r = g = b = 0		// s = 0, v is undefined
    *s = 0;
    *h = 0; // was -1
    return -2;
  }

  if( r == max )
    *h = ( g - b ) / delta;		// between yellow & magenta
  else if( g == max )
    *h = 2 + ( b - r ) / delta;	// between cyan & yellow
  else
    *h = 4 + ( r - g ) / delta;	// between magenta & cyan
  *h *= 60;				// degrees

  if( *h < 0 )
    *h += 360;

  return 0;
}


//! \}
