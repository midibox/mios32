
#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

// See also: http://www.freertos.org/a00110.html

// MIOS32 specific predefines - can be overruled in your local mios32_config.h file
// don't include complete mios.h, but only relevant parts!
#include "mios32_datatypes.h"
#include "mios32_config.h"
#include "mios32_sys.h"

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

#define configUSE_PREEMPTION                    1
#define configUSE_PORT_OPTIMISED_TASK_SELECTION 1
#define configUSE_TICKLESS_IDLE                 0
#define configCPU_CLOCK_HZ                      ((unsigned portLONG) (MIOS32_SYS_CPU_FREQUENCY) )
#define configTICK_RATE_HZ                      ((portTickType) 1000 )
#define configMAX_PRIORITIES                    5
#define configMINIMAL_STACK_SIZE                ((unsigned portSHORT) ((MIOS32_MINIMAL_STACK_SIZE)/4) )
#define configMAX_TASK_NAME_LEN                 16
#define configUSE_16_BIT_TICKS                  0
#define configIDLE_SHOULD_YIELD                 1
#ifndef configUSE_TASK_NOTIFICATIONS // can be changed in mios32_config.h -- will add some additional processing overhead
#define configUSE_TASK_NOTIFICATIONS            0
#endif
#define configUSE_MUTEXES                       1
#define configUSE_RECURSIVE_MUTEXES             1
#define configUSE_COUNTING_SEMAPHORES           0
#define configUSE_ALTERNATIVE_API               0 /* Deprecated! */
#define configQUEUE_REGISTRY_SIZE               10
#define configUSE_QUEUE_SETS                    0
#define configUSE_TIME_SLICING                  1
#define configUSE_NEWLIB_REENTRANT              0
#define configENABLE_BACKWARD_COMPATIBILITY     1 // MIOS32 Legacy...
#define configNUM_THREAD_LOCAL_STORAGE_POINTERS 0

/* Memory allocation related definitions. */
#ifndef configSUPPORT_STATIC_ALLOCATION // can be changed in mios32_config.h
#define configSUPPORT_STATIC_ALLOCATION         0
#endif
#define configSUPPORT_DYNAMIC_ALLOCATION        1
#define configTOTAL_HEAP_SIZE                   ((size_t)((MIOS32_HEAP_SIZE)))
#define configAPPLICATION_ALLOCATED_HEAP        1

/* Hook function related definitions. */
#define configUSE_IDLE_HOOK                     1
#define configUSE_TICK_HOOK                     1
#define configCHECK_FOR_STACK_OVERFLOW          0
#define configUSE_MALLOC_FAILED_HOOK            1 // vApplicationMallocFailedHook located in main.c (debug message will be sent to debug terminal and LCD)
#define configUSE_DAEMON_TASK_STARTUP_HOOK      0

/* Run time and task stats gathering related definitions. */
#ifndef configGENERATE_RUN_TIME_STATS // can be changed in mios32_config.h
#define configGENERATE_RUN_TIME_STATS           0
#endif
#define configUSE_TRACE_FACILITY                0
#define configUSE_STATS_FORMATTING_FUNCTIONS    0

/* Co-routine related definitions. */
#define configUSE_CO_ROUTINES                   0
#define configMAX_CO_ROUTINE_PRIORITIES         1

/* Software timer related definitions. */
#ifndef configUSE_TIMERS // can be changed in mios32_config.h
#define configUSE_TIMERS                        0
#endif
#define configTIMER_TASK_PRIORITY               3
#define configTIMER_QUEUE_LENGTH                10
#define configTIMER_TASK_STACK_DEPTH            configMINIMAL_STACK_SIZE

/* Interrupt nesting behaviour configuration. */
#define configKERNEL_INTERRUPT_PRIORITY         255
#define configMAX_SYSCALL_INTERRUPT_PRIORITY    191 /* equivalent to 0xa0, or priority 5. */
#define configMAX_API_CALL_INTERRUPT_PRIORITY   [dependent on processor and application]

/* Define to trap errors during development. */
//#define configASSERT( ( x ) ) if( ( x ) == 0 ) vAssertCalled( __FILE__, __LINE__ )

/* FreeRTOS MPU specific definitions. */
#define configINCLUDE_APPLICATION_DEFINED_PRIVILEGED_FUNCTIONS 0

/* Optional functions - most linkers will remove unused functions anyway. */
#define INCLUDE_vTaskPrioritySet                1
#define INCLUDE_uxTaskPriorityGet               1
#define INCLUDE_vTaskDelete                     1
#define INCLUDE_vTaskSuspend                    1
#define INCLUDE_xResumeFromISR                  1
#define INCLUDE_vTaskDelayUntil                 1
#define INCLUDE_vTaskDelay                      1
#define INCLUDE_xTaskGetSchedulerState          1
#define INCLUDE_xTaskGetCurrentTaskHandle       1
#define INCLUDE_uxTaskGetStackHighWaterMark     1
#define INCLUDE_xTaskGetIdleTaskHandle          1
#define INCLUDE_eTaskGetState                   1
#define INCLUDE_xEventGroupSetBitFromISR        1
#if configUSE_TIMERS
#define INCLUDE_xTimerPendFunctionCall          1
#endif
#define INCLUDE_xTaskAbortDelay                 1
#define INCLUDE_xTaskGetHandle                  1
#define INCLUDE_xTaskResumeFromISR              1

#endif /* FREERTOS_CONFIG_H */

