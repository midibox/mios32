
#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

// MIOS32 specific predefines - can be overruled in your local mios32_config.h file
#include "mios32_config.h"

#ifndef MIOS32_MINIMAL_STACK_SIZE
#define MIOS32_MINIMAL_STACK_SIZE 1024
#endif

#ifndef MIOS32_HEAP_SIZE
#define MIOS32_HEAP_SIZE 10*1024
#endif


/*-----------------------------------------------------------
 * Application specific definitions.
 *
 * These definitions should be adjusted for your particular hardware and
 * application requirements.
 *
 * THESE PARAMETERS ARE DESCRIBED WITHIN THE 'CONFIGURATION' SECTION OF THE
 * FreeRTOS API DOCUMENTATION AVAILABLE ON THE FreeRTOS.org WEB SITE.
 *----------------------------------------------------------*/

#define configUSE_PREEMPTION                1
#define configUSE_IDLE_HOOK                 1
#define configUSE_TICK_HOOK                 1
#define configCPU_CLOCK_HZ                  ( ( unsigned portLONG ) 72000000 )
#define configTICK_RATE_HZ                  ( ( portTickType ) 1000 )
#define configMAX_PRIORITIES                ( ( unsigned portBASE_TYPE ) 5 )
#define configMINIMAL_STACK_SIZE            ( ( unsigned portSHORT ) (MIOS32_MINIMAL_STACK_SIZE/4) )
#define configTOTAL_HEAP_SIZE               ( ( size_t ) ( MIOS32_HEAP_SIZE ) )
#define configMAX_TASK_NAME_LEN             ( 16 )

// philetaylor: Added optional trace facility to allow tasks to be displayed
// This is only used on the webserver example at the moment
// It is very slow so shouldn't be used unless necessary. Default is off.
#ifndef configUSE_TRACE_FACILITY
#define configUSE_TRACE_FACILITY            0
#endif

#define configUSE_16_BIT_TICKS              0
#define configIDLE_SHOULD_YIELD             1
#define configUSE_MUTEXES                   1
#define configUSE_RECURSIVE_MUTEXES         1
#define configUSE_COUNTING_SEMAPHORES       0
#define configUSE_ALTERNATIVE_API           0
#define configCHECK_FOR_STACK_OVERFLOW      0
#define configQUEUE_REGISTRY_SIZE           10

// see http://www.freertos.org/index.html?http://www.freertos.org/rtos-run-time-stats.html
// and http://www.midibox.org/mios32/manual/group___f_r_e_e_r_t_o_s___u_t_i_l_s.html
//
// The appr. utility functions for MIOS32 applications are located under
//   $MIOS32_PATH/modules/freertos_utils
// 
#ifndef configGENERATE_RUN_TIME_STATS
#define configGENERATE_RUN_TIME_STATS       0
#endif

// vApplicationMallocFailedHook located in main.c (debug message will be sent to debug terminal and LCD)
// philetaylor: Changed to allow this to be configured in mios32_config.h 
#ifndef configUSE_MALLOC_FAILED_HOOK
#define configUSE_MALLOC_FAILED_HOOK        1
#endif


#define INCLUDE_vTaskPrioritySet            1
#define INCLUDE_uxTaskPriorityGet           1
#define INCLUDE_vTaskDelete                 1
#define INCLUDE_vTaskCleanUpResources       0
#define INCLUDE_vTaskSuspend                1
#define INCLUDE_vResumeFromISR              1
#define INCLUDE_vTaskDelayUntil             1
#define INCLUDE_vTaskDelay                  1
#define INCLUDE_xTaskGetSchedulerState      1
#define INCLUDE_xTaskGetCurrentTaskHandle   1
#define INCLUDE_uxTaskGetStackHighWaterMark 0

#define configUSE_CO_ROUTINES               0 
#define configMAX_CO_ROUTINE_PRIORITIES     ( 2 )

/* This is the raw value as per the Cortex-M3 NVIC.  Values can be 255
(lowest) to 0 (1?) (highest). */
#define configKERNEL_INTERRUPT_PRIORITY 		255
#define configMAX_SYSCALL_INTERRUPT_PRIORITY 	191 /* equivalent to 0xa0, or priority 5. */


/* This is the value being used as per the ST library which permits 16
priority values, 0 to 15.  This must correspond to the
configKERNEL_INTERRUPT_PRIORITY setting.  Here 15 corresponds to the lowest
NVIC value of 255. */
#define configLIBRARY_KERNEL_INTERRUPT_PRIORITY	15

#endif /* FREERTOS_CONFIG_H */

