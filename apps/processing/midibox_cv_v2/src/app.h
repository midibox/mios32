// $Id$
/*
 * Header file of application
 *
 * ==========================================================================
 *
 *  Copyright (C) <year> <your name> (<your email address>)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _APP_H
#define _APP_H


/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////

#define APP_CV_UPDATE_RATE_FACTOR_MAX 20


/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
// returns pointer to MbCv objects
#include <MbCvEnvironment.h>
MbCvEnvironment *APP_GetEnv();
#endif


#ifdef __cplusplus
extern "C" {
#endif

extern void APP_Init(void);
extern void APP_Background(void);
extern void APP_MIDI_NotifyPackage(mios32_midi_port_t port, mios32_midi_package_t midi_package);
extern s32  APP_SYSEX_Parser(mios32_midi_port_t port, u8 midi_in);
extern void APP_SRIO_ServicePrepare(void);
extern void APP_SRIO_ServiceFinish(void);
extern void APP_DIN_NotifyToggle(u32 pin, u32 pin_value);
extern void APP_ENC_NotifyChange(u32 encoder, s32 incrementer);
extern void APP_AIN_NotifyChange(u32 pin, u32 pin_value);


extern void APP_TASK_Period_1mS_LP(void);
extern void APP_TASK_Period_1mS_LP2(void);
extern void APP_TASK_Period_1mS_SD(void);
extern void APP_TASK_Period_1mS(void);

extern s32 APP_StopwatchInit(void);
extern s32 APP_StopwatchReset(void);
extern s32 APP_StopwatchCapture(void);

extern s32 APP_SelectScopeLCDs(void);
extern s32 APP_SelectMainLCD(void);

extern s32 APP_CvUpdateRateFactorSet(u8 factor);
extern s32 APP_CvUpdateRateFactorGet(void);
extern s32 APP_CvUpdateOverloadStatusGet(void);


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
}
#endif

#endif /* _APP_H */
