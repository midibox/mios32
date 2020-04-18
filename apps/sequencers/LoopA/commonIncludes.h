//
// Created by hawkeye on 9/26/18.
//

#ifndef LOOPA_COMMONINCLUDES_H
#define LOOPA_COMMONINCLUDES_H

#define DEBUG_MSG MIOS32_MIDI_SendDebugMessage

#include "loopa_datatypes.h"


// This section is only necessary for automatic clion IDE Macro expansion (and to get rid of clion IDE warnings)
#include "FreeRTOS.h"
#define configUSE_RECURSIVE_MUTEXES 1
#include <semphr.h>
typedef long BaseType_t;
typedef u32 TickType_t;
extern void vPortEnterCritical(void);
extern void vPortExitCritical(void);
#define portENTER_CRITICAL() vPortEnterCritical()
#define portEXIT_CRITICAL() vPortExitCritical()
// ---

#include <string.h>
#include <stdarg.h>

#include <mios32.h>
#include <mios32_midi.h>
#include <app_lcd.h>
#include <seq_bpm.h>
#include "midi_out.h"
#include <midi_port.h>
#include <midi_router.h>

#include "app_lcd.h"
#include "file.h"
#include "tasks.h"

#endif //LOOPA_COMMONINCLUDES_H
