// $Id: app.h 674 2009-07-29 19:54:48Z tk $
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

# include <FreeRTOS.h>
# include <portmacro.h>
# include <task.h>
# include <queue.h>
# include <semphr.h>

/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////

// messages
#define PRINT_MSG_NONE           0
#define PRINT_MSG_INIT           1
#define PRINT_MSG_STATUS         2


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
extern void APP_MutexSPI0Take(void);
extern void APP_MutexSPI0Give(void);

static void TASK_Period1S(void *pvParameters);

/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////

extern volatile u8 print_msg;
extern volatile u8 sdcard_available;

extern xSemaphoreHandle xSDCardSemaphore;
#define MUTEX_SDCARD_TAKE { while( xSemaphoreTakeRecursive(xSDCardSemaphore, (portTickType)1) != pdTRUE ); }
#define MUTEX_SDCARD_GIVE { xSemaphoreGiveRecursive(xSDCardSemaphore); }


#endif /* _APP_H */
