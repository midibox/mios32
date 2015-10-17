// $Id$
//! \defgroup MBNG_RGBLED
//! RGB LED access functions for MIDIbox NG
//! \{
/* ==========================================================================
 *
 *  Copyright (C) 2012 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

/////////////////////////////////////////////////////////////////////////////
//! Include files
/////////////////////////////////////////////////////////////////////////////

#include <mios32.h>
#include <ws2812.h>

#include "app.h"
#include "mbng_rgbled.h"
#include "mbng_lcd.h"
#include "mbng_patch.h"
#include "mbng_event.h"


/////////////////////////////////////////////////////////////////////////////
//! local defines
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
//! local variables
/////////////////////////////////////////////////////////////////////////////

static u16 rainbow_speed;
static u8  rainbow_brightness;


/////////////////////////////////////////////////////////////////////////////
//! This function initializes the RGB LED handler
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_RGBLED_Init(u32 mode)
{
  if( mode != 0 )
    return -1; // only mode 0 supported

  WS2812_Init(1); // only clear LEDs (HW init only done once in APP_Init()
  
  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! This function is called periodically for the rainbow feature
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_RGBLED_Periodic_1mS(void)
{
  static u16 ctr = 0;

  if( rainbow_speed ) {
    if( ++ctr >= 360 )
      ctr = 0;

    int led;
    float h = ctr;
    for(led=0; led<WS2812_NUM_LEDS; ++led) {
      WS2812_LED_SetHSV(led, h, 1, (float)rainbow_brightness / 100.0);
      h += rainbow_speed;
      if( h >= 360 ) h -= 360;
    }
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! Set/Get Rainbow speed and brightness
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_RGBLED_RainbowSpeedSet(u8 speed) // 0..255
{
  rainbow_speed = speed;
  return 0; // no error
}

s32 MBNG_RGBLED_RainbowSpeedGet(void)
{
  return rainbow_speed;
}

s32 MBNG_RGBLED_RainbowBrightnessSet(u8 brightness) // 0..100
{
  rainbow_brightness = (brightness > 100) ? 100 : brightness;
  return 0; // no error
}

s32 MBNG_RGBLED_RainbowBrightnessGet(void)
{
  return rainbow_brightness;
}


/////////////////////////////////////////////////////////////////////////////
//! This function is called by MBNG_EVENT_ItemReceive when a matching value
//! has been received
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_RGBLED_NotifyReceivedValue(mbng_event_item_t *item)
{
  u16 hw_id = item->hw_id;

  if( debug_verbose_level >= DEBUG_VERBOSE_LEVEL_INFO ) {
    DEBUG_MSG("MBNG_RGBLED_NotifyReceivedValue(%d, %d)\n", hw_id & 0xfff, item->value);
  }

  // TODO

  return 0; // no error
}

//! \}
