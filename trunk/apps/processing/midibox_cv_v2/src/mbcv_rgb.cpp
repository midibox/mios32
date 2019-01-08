// $Id$
/*
 * MIDIbox CV V2 RGB LED functions
 *
 * ==========================================================================
 *
 *  Copyright (C) 2016 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

/////////////////////////////////////////////////////////////////////////////
// Include files
/////////////////////////////////////////////////////////////////////////////

#include <mios32.h>

#include <app.h>
#include <MbCvEnvironment.h>
#include <ws2812.h>

#include "mbcv_rgb.h"
#include "mbcv_lre.h"
#include "mbcv_hwcfg.h"
#include "scs_config.h"


/////////////////////////////////////////////////////////////////////////////
// Global variables
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Local prototypes
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 MBCV_RGB_Init(u32 mode)
{
  WS2812_Init(0);

  MBCV_RGB_UpdateAllLeds();

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// RGB LED update
/////////////////////////////////////////////////////////////////////////////
s32 MBCV_RGB_UpdateLed(u32 led)
{
  if( led >= MBCV_RGB_LED_NUM )
    return -1; // invalid LED number

  mbcv_hwcfg_ws2812_t *ws2812 = &mbcv_hwcfg_ws2812[led];
  u8 led_pos = ws2812->pos;

  if( led_pos == 0 )
    return -2; // LED not mapped
  led_pos -= 1; // counting from 0

  mbcv_lre_enc_cfg_t e_cfg = MBCV_LRE_EncCfgGet(led, MBCV_LRE_BankGet());
  mbcv_lre_enc_cfg_t *e = &e_cfg; // shortcut

  MbCvEnvironment* env = APP_GetEnv();

  // determine value based on ENC bank
  float value = 0.0;
  {
    u16 value16;
    if( !env->getNRPN(e->nrpn, &value16) ) {
      value = 0.0;
    } else {
      float max = (e->min <= e->max) ? (e->max - e->min) : (e->min - e->max);
      value = (float)value16 / max;
    }
  }

  // optional effective value affects the saturation
  float saturation = 0.75;
  {
    float effective;
    if( env->getNRPNEffectiveValue(e->nrpn, &effective) ) {
      saturation += effective * 0.50; // add only 50%
      if( saturation < 0.0 )
	saturation = 0.0;
      else if( saturation > 1.0 )
	saturation = 1.0;
    }
  }

  // set colour
  switch( ws2812->mode ) {
  case MBCV_RGB_MODE_CHANNEL_HUE: {
#if 0
    float hue = (float)SCS_CONFIG_CvGet() * (360.0/(float)CV_SE_NUM);
#else
    float hue = (float)MBCV_LRE_BankGet() * (360.0/(float)CV_SE_NUM); // quickfix: should depend on bank so that it corresponds with the encoder function
#endif

    WS2812_LED_SetHSV(led_pos, hue, saturation, value);
  } break;

  default: // e.g. MBCV_RGB_MODE_DISABLED
    WS2812_LED_SetRGB(led_pos, 0, 0);
    WS2812_LED_SetRGB(led_pos, 1, 0);
    WS2812_LED_SetRGB(led_pos, 2, 0);
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Update all RGB LEDs
/////////////////////////////////////////////////////////////////////////////
s32 MBCV_RGB_UpdateAllLeds(void)
{
  for(int led=0; led<MBCV_RGB_LED_NUM; ++led) {
    MBCV_RGB_UpdateLed(led);
  }

  return 0; // no error
}
