// $Id$
/*
 * Header file for MF Driver
 *
 * ==========================================================================
 *
 *  Copyright (C) 2008 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _MIOS32_MF_H
#define _MIOS32_MF_H

/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////

// number of motorfaders (0-16)
#ifndef MIOS32_MF_NUM
#define MIOS32_MF_NUM 8
#endif


// define the desired oversampling rate (1..16)
#ifndef MIOS32_AIN_OVERSAMPLING_RATE
#define MIOS32_AIN_OVERSAMPLING_RATE  1
#endif

#ifndef MIOS32_MF_RCLK_PORT
#define MIOS32_MF_RCLK_PORT  GPIOC
#endif
#ifndef MIOS32_MF_RCLK_PIN
#define MIOS32_MF_RCLK_PIN   GPIO_Pin_13
#endif
#ifndef MIOS32_MF_SCLK_PORT
#define MIOS32_MF_SCLK_PORT  GPIOB
#endif
#ifndef MIOS32_MF_SCLK_PIN
#define MIOS32_MF_SCLK_PIN   GPIO_Pin_6
#endif
#ifndef MIOS32_MF_DOUT_PORT
#define MIOS32_MF_DOUT_PORT  GPIOB
#endif
#ifndef MIOS32_MF_DOUT_PIN
#define MIOS32_MF_DOUT_PIN   GPIO_Pin_5
#endif

// should output pins be used in Open Drain mode? (perfect for 3.3V->5V levelshifting)
#ifndef MIOS32_MF_OUTPUTS_OD
#define MIOS32_MF_OUTPUTS_OD 0
#endif


/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////

typedef enum {
  MF_Standby = 0,
  MF_Up      = 1,
  MF_Down    = 2
} mios32_mf_direction_t;


typedef union {
  struct {
    unsigned ALL:32;
  } all;
  struct {
    u8 deadband;
    u8 pwm_period;
    u8 pwm_duty_cycle_up;
    u8 pwm_duty_cycle_down;
  } cfg;
} mios32_mf_config_t;


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 MIOS32_MF_Init(u32 mode);

extern s32 MIOS32_MF_FaderMove(u32 mf, u16 pos);
extern s32 MIOS32_MF_FaderDirectMove(u32 mf, mios32_mf_direction_t direction);

extern s32 MIOS32_MF_SuspendSet(u32 mf, u8 suspend);
extern s32 MIOS32_MF_SuspendGet(u32 mf);

extern s32 MIOS32_MF_TouchDetectionReset(u32 mf);

extern s32 MIOS32_MF_ConfigSet(u32 mf, mios32_mf_config_t config);
extern mios32_mf_config_t MIOS32_MF_ConfigGet(u32 mf);

extern s32 MIOS32_MF_Tick(u16 *ain_values, u16 *ain_deltas);


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////


#endif /* _MIOS32_MF_H */
