// $Id$
/*
 * Header file of application
 *
 * ==========================================================================
 *
 *  Copyright (C) 2008 Thorsten Klose (tk@midibox.org)
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

// messages
#define PRINT_MSG_NONE           0
#define PRINT_MSG_INIT           1
#define PRINT_MSG_PATCH_AND_BANK 2
#define PRINT_MSG_DUMP_SENT      3
#define PRINT_MSG_DUMP_RECEIVED  4


// DIN pin assignments for menu interface
#define DIN_NUMBER_EXEC     7
#define DIN_NUMBER_INC      6
#define DIN_NUMBER_DEC      5
#define DIN_NUMBER_SNAPSHOT 4


/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern void APP_Init(void);
extern void APP_Background(void);
extern void APP_MIDI_NotifyPackage(mios32_midi_port_t port, mios32_midi_package_t midi_package);
extern void APP_SRIO_ServicePrepare(void);
extern void APP_SRIO_ServiceFinish(void);
extern void APP_DIN_NotifyToggle(u32 pin, u32 pin_value);
extern void APP_ENC_NotifyChange(u32 encoder, s32 incrementer);
extern void APP_AIN_NotifyChange(u32 pin, u32 pin_value);


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////

extern volatile u8 patch;
extern volatile u8 bank;
extern volatile u8 print_msg;


#endif /* _APP_H */
