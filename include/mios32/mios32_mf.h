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
#define MIOS32_MF_NUM 0
#endif


// Which SPI peripheral should be used
// allowed values: 0, 1 or 2
#ifndef MIOS32_MF_SPI
#define MIOS32_MF_SPI 2
#endif

// Which RC pin of the SPI port should be used
// allowed values: 0 or 1 for SPI0 (J16:RC1, J16:RC2), 0 for SPI1 (J8/9:RC), 0 or 1 for SPI2 (J19:RC1, J19:RC2)
#ifndef MIOS32_MF_SPI_RC_PIN
#define MIOS32_MF_SPI_RC_PIN 0
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
