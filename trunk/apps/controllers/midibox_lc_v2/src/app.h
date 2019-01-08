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


/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////

// status of LC emulation
typedef union {
  struct {
    unsigned ALL:8;
  };
  struct {
    unsigned LAYER_SEL:2;      // selects layer 0..2
    unsigned GPC_SEL:1;        // if cleared: logic control emulation mode
                               // if set: general purpose controller mode
    unsigned LED_UPDATE_REQ:1; // if set, the LEDs will be updated
    unsigned LEDRING_METERS:1; // if set, LEDrings are working as meters
  };
} lc_flags_t;


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

extern lc_flags_t lc_flags;

#endif /* _APP_H */
