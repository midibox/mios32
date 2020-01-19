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

    // for the case that the rainbow effect has been turned off while this
    // function has been executed in a background task
    if( !rainbow_speed ) {
      MBNG_RGBLED_Init(0);
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
//! Internal function to set a pattern
/////////////////////////////////////////////////////////////////////////////
static s32 MBNG_RGBLED_PatternSet(u32 start_led, u8 pattern, u8 is_hsv, float led_value_normalized, float v1, float v2, float v3)
{
  u32 num_leds = 32; // TODO: we could make this configurable!
  u32 pos = num_leds * led_value_normalized; // goes from 0..num_leds-1

  int led;
  for(led=0; led<num_leds; ++led) {
    float local_v1 = v1;
    float local_v2 = v2;
    float local_v3 = v3;

    switch( pattern ) {

    /////////////////////////////////////////////////////////////////////////
    case 1: { // Classical Thermometer Pattern, colour specified via hsv= or rgb=
      if( led > pos ) {
        if( is_hsv ) { local_v3 = 0.0; } else { local_v1 = local_v2 = local_v3 = 0; }
      }
    } break;

    /////////////////////////////////////////////////////////////////////////
    case 2: { // Moving Dot
      if( led != pos ) {
        if( is_hsv ) { local_v3 = 0.0; } else { local_v1 = local_v2 = local_v3 = 0; }
      }
    } break;

    /////////////////////////////////////////////////////////////////////////
    case 3: { // Thermometer Pattern with Rainbow colour scheme (predefined H will be ignored)
      float h_max = 200.0; // starting with blue looks better than violet
      local_v1 = h_max - ((h_max / num_leds) * led);
      if( !is_hsv ) { // RGB setting will be ignored...
        v2 = local_v2 = 1.0;
        v3 = local_v3 = 0.1;
        is_hsv = 1;
      }

      if( led > pos ) {
        local_v3 = 0.0;
      }
    } break;

    /////////////////////////////////////////////////////////////////////////
    default: {
      // nothing to change...
    }

    }

    if( is_hsv ) {
      WS2812_LED_SetHSV(start_led + led, local_v1, local_v2, local_v3);
    } else {
      WS2812_LED_SetRGB(start_led + led, 0, (u8)local_v1);
      WS2812_LED_SetRGB(start_led + led, 1, (u8)local_v2);
      WS2812_LED_SetRGB(start_led + led, 2, (u8)local_v3);
    }

    if( pattern == 0 )
      break; // only a single LED will be modified
  }

  return 0; // no error
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

  float led_value;
  s32 mapped_led_value;
  if( (mapped_led_value=MBNG_EVENT_MapValue(item->map, item->value, 0, 0)) >= 0 ) {
    led_value = (float)mapped_led_value;
  } else {
    //int range = (item->min <= item->max) ? (item->max - item->min + 1) : (item->min - item->max + 1);
    if( item->flags.radio_group ) {
      if( item->min <= item->max )
	led_value = (item->value >= item->min && item->value <= item->max) ? item->value : 0;
      else
	led_value = (item->value >= item->max && item->value <= item->min) ? item->value : 0;
    } else if( item->min == item->max ) {      
      led_value = (item->value == item->min) ? item->value : 0;
    } else {
      if( item->min <= item->max ) {
	led_value = item->min + item->value;
	if( led_value > item->max )
	  led_value = item->max;
	else if( led_value < item->min )
	  led_value = item->min;
      } else {
	led_value = item->min - item->value; // reverse hue sweep
	if( led_value < item->max )
	  led_value = item->max;
	else if( led_value > item->min )
	  led_value = item->min;
      }
    }
  }

  float led_value_range = (item->max >= item->min) ? (float)(item->max - item->min + 1) : (float)(item->min - item->max + 1);
  float led_value_normalized = led_value / led_value_range;

  // set LED
  int led = (hw_id & 0xfff) - 1;
  if( item->rgb.ALL ) {
    u8 r = item->rgb.r * 16;
    u8 g = item->rgb.g * 16;
    u8 b = item->rgb.b * 16;

    if( !led_value ) {
      r = g = b = 0;
    } else if( item->flags.dimmed ) {
      if( led_value > 100 )
	led_value = 100;
      led_value /= 100.0; // value ranges from 0..1

      r = (u8)((float)r * led_value);
      g = (u8)((float)g * led_value);
      b = (u8)((float)b * led_value);
    }

    //DEBUG_MSG("RGBLED:%d %d:%d:%d\n", led+1, r, g, b);

    MBNG_RGBLED_PatternSet(led, item->flags.rgbled_pattern, 0, led_value_normalized, r, g, b);
  } else {
    float hue = 0.0;
    float saturation = 0.0;
    float brightness = 0.0;

    saturation = (float)item->hsv.s / 100.0;
    if( item->flags.dimmed ) {
      hue = item->hsv.h;
      brightness = (float)((led_value > 100) ? 100 : led_value) / 100.0;
    } else {
      if( item->flags.rgbled_pattern == 0 ) {
        hue = led_value + (float)item->hsv.h;
      } else {
        hue = (float)item->hsv.h;
      }
      brightness = (float)item->hsv.v / 100.0;
    }

    //DEBUG_MSG("RGBLED:%d H=%d S=%d V=%d\n", led+1, (int)hue, (int)(saturation*100.0), (int)(brightness*100.0));
    MBNG_RGBLED_PatternSet(led, item->flags.rgbled_pattern, 1, led_value_normalized, hue, saturation, brightness);
  }

  return 0; // no error
}

//! \}
