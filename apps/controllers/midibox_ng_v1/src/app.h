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

#define DEBUG_VERBOSE_LEVEL_OFF     0
#define DEBUG_VERBOSE_LEVEL_FATAL   1
#define DEBUG_VERBOSE_LEVEL_ERROR   2
#define DEBUG_VERBOSE_LEVEL_WARNING 3
#define DEBUG_VERBOSE_LEVEL_INFO    4
#define DEBUG_VERBOSE_LEVEL_DEBUG   5


/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern void APP_Init(void);
extern void APP_Background(void);
extern void APP_MIDI_NotifyPackage(mios32_midi_port_t port, mios32_midi_package_t midi_package);
extern s32 APP_SYSEX_Parser(mios32_midi_port_t port, u8 midi_in);
extern void APP_SRIO_ServicePrepare(void);
extern void APP_SRIO_ServiceFinish(void);
extern void APP_DIN_NotifyToggle(u32 pin, u32 pin_value);
extern void APP_ENC_NotifyChange(u32 encoder, s32 incrementer);
extern void APP_AIN_NotifyChange(u32 pin, u32 pin_value);

/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////

extern u8  debug_verbose_level;
extern u32 app_ms_counter;

#endif /* _APP_H */
