// $Id$
/*
 * Header file for AIN Driver
 *
 * ==========================================================================
 *
 *  Copyright (C) 2008 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _MIOS32_AIN_H
#define _MIOS32_AIN_H

/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////

// bit mask to enable channels
//
// Pin mapping on MBHP_CORE_STM32 module:
//   15       14      13     12     11     10      9      8   
// J16.SO  J16.SI  J16.SC J16.RC J5C.A11 J5C.A10 J5C.A9 J5C.A8
//   7        6       5      4      3      2      1       0
// J5B.A7  J5B.A6  J5B.A5 J5B.A4 J5A.A3 J5A.A2 J5A.A1  J5A.A0
//
// Examples:
//   mask 0x000f will enable all J5A channels
//   mask 0x00f0 will enable all J5B channels
//   mask 0x0f00 will enable all J5C channels
//   mask 0x0fff will enable all J5A/B/C channels
// (all channels are disabled by default)
#ifndef MIOS32_AIN_CHANNEL_MASK
#define MIOS32_AIN_CHANNEL_MASK 0
#endif


// define the desired oversampling rate (1..16)
#ifndef MIOS32_AIN_OVERSAMPLING_RATE
#define MIOS32_AIN_OVERSAMPLING_RATE  1
#endif


// define the deadband (min. difference to report a change to the application hook)
// 31 is enough for 7bit resolution at 12bit sampling resolution (1x oversampling)
#ifndef MIOS32_AIN_DEADBAND
#define MIOS32_AIN_DEADBAND 31
#endif


// define the deadband which is used when an AIN pin is in "idle" state
// this helps to avoid sporadical jittering values
// Set this value to 0 to disable the feature (it's enabled by default)
#ifndef MIOS32_AIN_DEADBAND_IDLE
#define MIOS32_AIN_DEADBAND_IDLE 127
#endif

// define after how many conversions the AIN pin should go into "idle" state
// - "idle" state is left once MIOS32_AIN_DEADBAND_IDLE is exceeded.
// - "idle" state is entered once MIOS32_AIN_DEADBAND hasn't been exceeded for 
//   MIOS32_AIN_IDLE_CTR conversions
// 3000 conversions are done in ca. 3 seconds (depends on number of pins!)
// allowed range: 1..65535
#ifndef MIOS32_AIN_IDLE_CTR
#define MIOS32_AIN_IDLE_CTR 3000
#endif


// muxed or unmuxed mode (0..3)?
// 0 == unmuxed mode
// 1 == 1 mux control line -> *2 channels
// 2 == 2 mux control line -> *4 channels
// 3 == 3 mux control line -> *8 channels
#ifndef MIOS32_AIN_MUX_PINS
#define MIOS32_AIN_MUX_PINS 0
#endif


// control pins to select the muxed channel
// only relevant if MIOS32_AIN_MUX_PINS > 0
#ifndef MIOS32_AIN_MUX0_PIN
#define MIOS32_AIN_MUX0_PIN   GPIO_Pin_4 // J5C.A8
#endif
#ifndef MIOS32_AIN_MUX0_PORT
#define MIOS32_AIN_MUX0_PORT  GPIOC
#endif

#ifndef MIOS32_AIN_MUX1_PIN
#define MIOS32_AIN_MUX1_PIN   GPIO_Pin_5 // J5C.A9
#endif
#ifndef MIOS32_AIN_MUX1_PORT
#define MIOS32_AIN_MUX1_PORT  GPIOC
#endif

#ifndef MIOS32_AIN_MUX2_PIN
#define MIOS32_AIN_MUX2_PIN   GPIO_Pin_0 // J5C.A10
#endif
#ifndef MIOS32_AIN_MUX2_PORT
#define MIOS32_AIN_MUX2_PORT  GPIOB
#endif


/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 MIOS32_AIN_Init(u32 mode);

extern s32 MIOS32_AIN_ServicePrepareCallback_Init(void *_service_prepare_callback);

extern s32 MIOS32_AIN_PinGet(u32 pin);

extern s32 MIOS32_AIN_DeadbandGet(void);
extern s32 MIOS32_AIN_DeadbandSet(u8 deadband);

extern s32 MIOS32_AIN_Handler(void *callback);


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////


#endif /* _MIOS32_AIN_H */
