// $Id: main.c 42 2008-09-30 21:49:43Z tk $
/*
 * Traditional Programming Model
 * Provides similar hooks like PIC based MIOS8 to the application
 *
 * ==========================================================================
 *
 *  Copyright (C) 2008 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

/////////////////////////////////////////////////////////////////////////////
// Include files
/////////////////////////////////////////////////////////////////////////////

#include <mios32.h>
#include <app.h>
#ifndef MIOS32_DONT_USE_LCD
#include <app_lcd.h>
#endif

#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>


/////////////////////////////////////////////////////////////////////////////
// External Prototypes
/////////////////////////////////////////////////////////////////////////////

extern void __libc_init_array(void);  /* calls CTORS of static objects */


/////////////////////////////////////////////////////////////////////////////
// Task Priorities
/////////////////////////////////////////////////////////////////////////////

#define PRIORITY_TASK_HOOKS		( tskIDLE_PRIORITY + 3 )


/////////////////////////////////////////////////////////////////////////////
// Local prototypes
/////////////////////////////////////////////////////////////////////////////
static void TASK_Hooks(void *pvParameters);
static void TASK_MIDI_Hooks(void *pvParameters);


/////////////////////////////////////////////////////////////////////////////
// Dummies for APP_Tick and APP_MIDI_Tick (if not used in app.c)
/////////////////////////////////////////////////////////////////////////////

__attribute__ ((weak)) void APP_Tick(void)
{
}

__attribute__ ((weak)) void APP_MIDI_Tick(void)
{
}


/////////////////////////////////////////////////////////////////////////////
// Main function
/////////////////////////////////////////////////////////////////////////////
int main(void)
{
  // initialize hardware and MIOS32 modules
#ifndef MIOS32_DONT_USE_SYS
  MIOS32_SYS_Init(0);
#endif
#ifndef MIOS32_DONT_USE_DELAY
  MIOS32_DELAY_Init(0);
#endif
#ifndef MIOS32_DONT_USE_TIMESTAMP
  MIOS32_TIMESTAMP_Init(0);
#endif
#ifndef MIOS32_DONT_USE_BOARD
  MIOS32_BOARD_Init(0);
#endif
#ifndef MIOS32_DONT_USE_SPI
  MIOS32_SPI_Init(0);
#endif
#ifndef MIOS32_DONT_USE_SRIO
  MIOS32_SRIO_Init(0);
#endif
#if !defined(MIOS32_DONT_USE_DIN) && !defined(MIOS32_DONT_USE_SRIO)
  MIOS32_DIN_Init(0);
#endif
#if !defined(MIOS32_DONT_USE_DOUT) && !defined(MIOS32_DONT_USE_SRIO)
  MIOS32_DOUT_Init(0);
#endif
#if !defined(MIOS32_DONT_USE_ENC) && !defined(MIOS32_DONT_USE_SRIO)
  MIOS32_ENC_Init(0);
#endif
#if !defined(MIOS32_DONT_USE_MF)
  MIOS32_MF_Init(0);
#endif
#if !defined(MIOS32_DONT_USE_AIN)
  MIOS32_AIN_Init(0);
#endif
#ifndef MIOS32_DONT_USE_IIC_BS
  MIOS32_IIC_BS_Init(0);
#endif
#ifndef MIOS32_DONT_USE_USB
  MIOS32_USB_Init(0);
#endif
#ifndef MIOS32_DONT_USE_MIDI
  MIOS32_MIDI_Init(0);
#endif
#ifndef MIOS32_DONT_USE_OSC
  MIOS32_OSC_Init(0);
#endif
#ifndef MIOS32_DONT_USE_COM
  MIOS32_COM_Init(0);
#endif
#ifndef MIOS32_DONT_USE_LCD
  MIOS32_LCD_Init(0);

# if defined(MIOS32_BOARD_MBHP_CORE_STM32) || defined(MIOS32_BOARD_MBHP_CORE_LPC17) || defined(MIOS32_BOARD_STM32F4DISCOVERY) || defined(MIOS32_BOARD_MBHP_CORE_STM32F4)
  // init second LCD as well (if available)
  MIOS32_LCD_DeviceSet(1);
  APP_LCD_Init(0);
  MIOS32_LCD_DeviceSet(0);
# endif
#endif
#ifdef MIOS32_USE_I2S
  MIOS32_I2S_Init(0);
#endif

  // call C++ constructors
  __libc_init_array();

  // initialize application
  APP_Init();

#if MIOS32_LCD_BOOT_MSG_DELAY
  // print boot message
# ifndef MIOS32_DONT_USE_LCD
  MIOS32_LCD_PrintBootMessage();
# endif

  // wait for given delay (usually 2 seconds)
# ifndef MIOS32_DONT_USE_DELAY
  int delay = 0;
  for(delay=0; delay<MIOS32_LCD_BOOT_MSG_DELAY; ++delay)
    MIOS32_DELAY_Wait_uS(1000);
# endif
#endif

  // start the task which calls the application hooks
  xTaskCreate(TASK_Hooks, (signed portCHAR *)"Hooks", configMINIMAL_STACK_SIZE, NULL, PRIORITY_TASK_HOOKS, NULL);
#if !defined(MIOS32_DONT_USE_MIDI)
  xTaskCreate(TASK_MIDI_Hooks, (signed portCHAR *)"MIDI_Hooks", configMINIMAL_STACK_SIZE, NULL, PRIORITY_TASK_HOOKS, NULL);
#endif

  // start the scheduler
  vTaskStartScheduler();

  // Will only get here if there was not enough heap space to create the idle task
  return 0;
}


/////////////////////////////////////////////////////////////////////////////
// Application Tick Hook (called by FreeRTOS each mS)
/////////////////////////////////////////////////////////////////////////////
void SRIO_ServiceFinish(void)
{
#ifndef MIOS32_DONT_USE_SRIO

# ifndef MIOS32_DONT_USE_ENC
  // update encoder states
  MIOS32_ENC_UpdateStates();
# endif

  // notify application about finished SRIO scan
  APP_SRIO_ServiceFinish();
#endif
}

void vApplicationTickHook(void)
{
#if !defined(MIOS32_DONT_USE_TIMESTAMP)
  MIOS32_TIMESTAMP_Inc();
#endif

#if !defined(MIOS32_DONT_USE_SRIO) && !defined(MIOS32_DONT_SERVICE_SRIO_SCAN)
  // notify application about SRIO scan start
  APP_SRIO_ServicePrepare();

  // start next SRIO scan - IRQ notification to SRIO_ServiceFinish()
  MIOS32_SRIO_ScanStart(SRIO_ServiceFinish);
#endif
}


/////////////////////////////////////////////////////////////////////////////
// Idle Hook (called by FreeRTOS when nothing else to do)
/////////////////////////////////////////////////////////////////////////////
void vApplicationIdleHook(void)
{
  APP_Background();
}


/////////////////////////////////////////////////////////////////////////////
// MIDI task (separated from TASK_Hooks() to ensure parallel handling of
// MIDI events if a hook in TASK_Hooks() blocks)
/////////////////////////////////////////////////////////////////////////////
#if !defined(MIOS32_DONT_USE_MIDI)
static void TASK_MIDI_Hooks(void *pvParameters)
{
  portTickType xLastExecutionTime;

  // Initialise the xLastExecutionTime variable on task entry
  xLastExecutionTime = xTaskGetTickCount();

  while( 1 ) {
    vTaskDelayUntil(&xLastExecutionTime, 1 / portTICK_RATE_MS);

    // skip delay gap if we had to wait for more than 5 ticks to avoid 
    // unnecessary repeats until xLastExecutionTime reached xTaskGetTickCount() again
    portTickType xCurrentTickCount = xTaskGetTickCount();
    if( xLastExecutionTime < (xCurrentTickCount-5) )
      xLastExecutionTime = xCurrentTickCount;

    // handle timeout/expire counters and USB packages
    MIOS32_MIDI_Periodic_mS();

    // check for incoming MIDI packages and call hook
    MIOS32_MIDI_Receive_Handler(APP_MIDI_NotifyPackage);

    // optional application specific hook
    // helps to save memory (re-use the TASK_Hooks for other purposes...)
    APP_MIDI_Tick();
  }
}
#endif


/////////////////////////////////////////////////////////////////////////////
// Remaining application hooks
/////////////////////////////////////////////////////////////////////////////
static void TASK_Hooks(void *pvParameters)
{
  portTickType xLastExecutionTime;

  // Initialise the xLastExecutionTime variable on task entry
  xLastExecutionTime = xTaskGetTickCount();

  while( 1 ) {
    vTaskDelayUntil(&xLastExecutionTime, 1 / portTICK_RATE_MS);

    // skip delay gap if we had to wait for more than 5 ticks to avoid 
    // unnecessary repeats until xLastExecutionTime reached xTaskGetTickCount() again
    portTickType xCurrentTickCount = xTaskGetTickCount();
    if( xLastExecutionTime < (xCurrentTickCount-5) )
      xLastExecutionTime = xCurrentTickCount;

#if !defined(MIOS32_DONT_USE_DIN) && !defined(MIOS32_DONT_USE_SRIO)
    // check for DIN pin changes, call APP_DIN_NotifyToggle on each toggled pin
    MIOS32_DIN_Handler(APP_DIN_NotifyToggle);

    // check for encoder changes, call APP_ENC_NotifyChanged on each change
# ifndef MIOS32_DONT_USE_ENC
    MIOS32_ENC_Handler(APP_ENC_NotifyChange);
# endif
#endif

#if !defined(MIOS32_DONT_USE_AIN) && !defined(MIOS32_DONT_SERVICE_AIN)
    // check for AIN pin changes, call APP_AIN_NotifyChange on each pin change
    MIOS32_AIN_Handler(APP_AIN_NotifyChange);
#endif

#if !defined(MIOS32_DONT_USE_COM)
    // check for incoming COM messages
    MIOS32_COM_Receive_Handler();
#endif

    // optional APP_Tick() hook
    // helps to save memory (re-use the TASK_Hooks for other purposes...)
    APP_Tick();
  }
}



/////////////////////////////////////////////////////////////////////////////
// This function aborts any operations, but keeps MIDI alive (for uploading
// a new firmware)
// If MIDI isn't enabled, the status LED will be flashed
/////////////////////////////////////////////////////////////////////////////
void _abort(void)
{
#ifndef MIOS32_DONT_USE_MIDI
  // keep MIDI alive, so that program code can be updated
  u32 delay_ctr = 0;
  while( 1 ) {
    ++delay_ctr;

    if( (delay_ctr % 100) == 0 ) {
      // handle timeout/expire counters and USB packages
      MIOS32_MIDI_Periodic_mS();
    }

    // check for incoming MIDI packages and call hook
    MIOS32_MIDI_Receive_Handler(APP_MIDI_NotifyPackage);

    if( (delay_ctr % 10000) == 0 ) {
      // toggle board LED
      MIOS32_BOARD_LED_Set(1, ~MIOS32_BOARD_LED_Get());
    }
  }
#else
  u32 delay_ctr = 0;
  while( 1 ) {
    ++delay_ctr;

    if( (delay_ctr % 1000000) == 0 ) {
      // toggle board LED
      MIOS32_BOARD_LED_Set(1, ~MIOS32_BOARD_LED_Get());
    }
  }
#endif
}


/////////////////////////////////////////////////////////////////////////////
// enabled in FreeRTOSConfig.h
/////////////////////////////////////////////////////////////////////////////
void vApplicationMallocFailedHook(void)
{
#ifndef MIOS32_DONT_USE_LCD
  // TODO: here we should select the normal font - but only if available!
  // MIOS32_LCD_FontInit((u8 *)GLCD_FONT_NORMAL);
  MIOS32_LCD_BColourSet(0xffffff);
  MIOS32_LCD_FColourSet(0x000000);

  MIOS32_LCD_DeviceSet(0);
  MIOS32_LCD_Clear();
  MIOS32_LCD_CursorSet(0, 0);
  MIOS32_LCD_PrintString("FATAL: FreeRTOS "); // 16 chars
  MIOS32_LCD_CursorSet(0, 1);
  MIOS32_LCD_PrintString("Malloc Error!!! "); // 16 chars
#endif

#ifndef MIOS32_DONT_USE_MIDI
  // Note: message won't be sent if MIDI task cannot be created!
  MIOS32_MIDI_SendDebugMessage("FATAL: FreeRTOS Malloc Error!!!\n");
#endif

  _abort();
}


/////////////////////////////////////////////////////////////////////////////
// _exit() for newer newlib versions
/////////////////////////////////////////////////////////////////////////////
void exit(int par)
{
#ifndef MIOS32_DONT_USE_LCD
  // TODO: here we should select the normal font - but only if available!
  // MIOS32_LCD_FontInit((u8 *)GLCD_FONT_NORMAL);
  MIOS32_LCD_BColourSet(0xffffff);
  MIOS32_LCD_FColourSet(0x000000);

  MIOS32_LCD_DeviceSet(0);
  MIOS32_LCD_Clear();
  MIOS32_LCD_CursorSet(0, 0);
  MIOS32_LCD_PrintString("Goodbye!");
#endif

#ifndef MIOS32_DONT_USE_MIDI
  // Note: message won't be sent if MIDI task cannot be created!
  MIOS32_MIDI_SendDebugMessage("Goodbye!\n");
#endif

  // pro forma: since this is a noreturn function, loop endless and call _abort (which will never exit)
  while( 1 )
    _abort();
}

// see http://www.linuxquestions.org/questions/programming-9/fyi-shared-libs-and-iostream-c-331113/
void *__dso_handle = NULL;


/////////////////////////////////////////////////////////////////////////////
// Customized HardFault Handler which prints out debugging informations
/////////////////////////////////////////////////////////////////////////////
void HardFault_Handler_c(unsigned int * hardfault_args)
{
  // from the book: "The definiteve guide to the ARM Cortex-M3"
  volatile unsigned int stacked_r0;
  volatile unsigned int stacked_r1;
  volatile unsigned int stacked_r2;
  volatile unsigned int stacked_r3;
  volatile unsigned int stacked_r12;
  volatile unsigned int stacked_lr;
  volatile unsigned int stacked_pc;
  volatile unsigned int stacked_psr;

  stacked_r0 = ((unsigned long) hardfault_args[0]);
  stacked_r1 = ((unsigned long) hardfault_args[1]);
  stacked_r2 = ((unsigned long) hardfault_args[2]);
  stacked_r3 = ((unsigned long) hardfault_args[3]);

  stacked_r12 = ((unsigned long) hardfault_args[4]);
  stacked_lr = ((unsigned long) hardfault_args[5]);
  stacked_pc = ((unsigned long) hardfault_args[6]);
  stacked_psr = ((unsigned long) hardfault_args[7]);
  
  MIOS32_MIDI_SendDebugMessage("Hard Fault PC = %08x\n", stacked_pc); // ensure that at least the PC will be sent
  MIOS32_MIDI_SendDebugMessage("==================\n");
  MIOS32_MIDI_SendDebugMessage("!!! HARD FAULT !!!\n");
  MIOS32_MIDI_SendDebugMessage("==================\n");
  MIOS32_MIDI_SendDebugMessage("R0 = %08x\n", stacked_r0);
  MIOS32_MIDI_SendDebugMessage("R1 = %08x\n", stacked_r1);
  MIOS32_MIDI_SendDebugMessage("R2 = %08x\n", stacked_r2);
  MIOS32_MIDI_SendDebugMessage("R3 = %08x\n", stacked_r3);
  MIOS32_MIDI_SendDebugMessage("R12 = %08x\n", stacked_r12);
  MIOS32_MIDI_SendDebugMessage("LR = %08x\n", stacked_lr);
  MIOS32_MIDI_SendDebugMessage("PC = %08x\n", stacked_pc);
  MIOS32_MIDI_SendDebugMessage("PSR = %08x\n", stacked_psr);
  MIOS32_MIDI_SendDebugMessage("BFAR = %08x\n", (*((volatile unsigned long *)(0xE000ED38))));
  MIOS32_MIDI_SendDebugMessage("CFSR = %08x\n", (*((volatile unsigned long *)(0xE000ED28))));
  MIOS32_MIDI_SendDebugMessage("HFSR = %08x\n", (*((volatile unsigned long *)(0xE000ED2C))));
  MIOS32_MIDI_SendDebugMessage("DFSR = %08x\n", (*((volatile unsigned long *)(0xE000ED30))));
  MIOS32_MIDI_SendDebugMessage("AFSR = %08x\n", (*((volatile unsigned long *)(0xE000ED3C))));

#ifndef MIOS32_DONT_USE_LCD
  // TODO: here we should select the normal font - but only if available!
  // MIOS32_LCD_FontInit((u8 *)GLCD_FONT_NORMAL);
  MIOS32_LCD_BColourSet(0xffffff);
  MIOS32_LCD_FColourSet(0x000000);

  MIOS32_LCD_DeviceSet(0);
  MIOS32_LCD_Clear();
  MIOS32_LCD_CursorSet(0, 0);
  MIOS32_LCD_PrintString("!! HARD FAULT !!");
  MIOS32_LCD_CursorSet(0, 1);
  MIOS32_LCD_PrintFormattedString("at PC=0x%08x", stacked_pc);
#endif

  _abort();
}


void HardFault_Handler(void)
{
  __asm("TST LR, #4");
  __asm("ITE EQ");
  __asm("MRSEQ R0, MSP");
  __asm("MRSNE R0, PSP");
  __asm("B HardFault_Handler_c");
}

// used if configCHECK_FOR_STACK_OVERFLOW enabled (set to 1 or 2) in FreeRTOSConfig.h
#if configCHECK_FOR_STACK_OVERFLOW
void vApplicationStackOverflowHook(xTaskHandle xTask, signed portCHAR *pcTaskName)
{
  MIOS32_MIDI_SendDebugMessage("======================\n");
  MIOS32_MIDI_SendDebugMessage("!!! STACK OVERFLOW !!!\n");
  MIOS32_MIDI_SendDebugMessage("======================\n");
  MIOS32_MIDI_SendDebugMessage("Function: %s\n", pcTaskName);

#ifndef MIOS32_DONT_USE_LCD
  // TODO: here we should select the normal font - but only if available!
  // MIOS32_LCD_FontInit((u8 *)GLCD_FONT_NORMAL);
  MIOS32_LCD_BColourSet(0xffffff);
  MIOS32_LCD_FColourSet(0x000000);

  MIOS32_LCD_DeviceSet(0);
  MIOS32_LCD_Clear();
  MIOS32_LCD_CursorSet(0, 0);
  MIOS32_LCD_PrintString("!! STACK OVERFLOW !!");
  MIOS32_LCD_CursorSet(0, 1);
  MIOS32_LCD_PrintFormattedString("in Task %s", pcTaskName);
#endif

  _abort();
}
#endif

