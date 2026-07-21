#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

#if defined(__ICCARM__) || defined(__CC_ARM) || defined(__GNUC__)
  #include <stdint.h>
  extern uint32_t SystemCoreClock;
#endif

#define CMSIS_device_header "stm32f4xx.h"

#define configENABLE_FPU                         1   /* F411 has FPU */
#define configENABLE_MPU                         0

#define configUSE_PREEMPTION                     1
#define configSUPPORT_STATIC_ALLOCATION          0
#define configSUPPORT_DYNAMIC_ALLOCATION         1
#define configUSE_TIME_SLICING					 1
#define configUSE_IDLE_HOOK                      0
#define configUSE_TICK_HOOK                      0
#define configCPU_CLOCK_HZ                       100000000  /* Verify = 100MHz */
#define configTICK_RATE_HZ                       100
#define configMAX_PRIORITIES                     ( 7 )   /* Reduced from 56 */
#define configMINIMAL_STACK_SIZE                 ((uint16_t)128)
#define configTOTAL_HEAP_SIZE                    ((size_t)(20 * 1024))  /* 20KB for F411 */
#define configMAX_TASK_NAME_LEN                  ( 16 )
#define configUSE_TRACE_FACILITY                 1
#define configUSE_16_BIT_TICKS                   0
#define configUSE_MUTEXES                        1
#define configQUEUE_REGISTRY_SIZE                8
#define configUSE_RECURSIVE_MUTEXES              1
#define configUSE_COUNTING_SEMAPHORES            1
#define configUSE_PORT_OPTIMISED_TASK_SELECTION  1   /* Better performance */
#define configUSE_STATS_FORMATTING_FUNCTIONS     1


 //for SEGGRE sysview
#define INCLUDE_xTaskGetIdleTaskHandle           1
#define INCLUDE_pxTaskGetStackStart				 1

#define configMESSAGE_BUFFER_LENGTH_TYPE         size_t

#define configUSE_CO_ROUTINES                    0
#define configMAX_CO_ROUTINE_PRIORITIES          ( 2 )

#define configUSE_TIMERS                         1
#define configTIMER_TASK_PRIORITY                ( 2 )
#define configTIMER_QUEUE_LENGTH                 10
#define configTIMER_TASK_STACK_DEPTH             256

#define configUSE_NEWLIB_REENTRANT               0

/* CMSIS-RTOS V2 flags */
#define configUSE_OS2_THREAD_SUSPEND_RESUME      1
#define configUSE_OS2_THREAD_ENUMERATE             1
#define configUSE_OS2_EVENTFLAGS_FROM_ISR        1
#define configUSE_OS2_THREAD_FLAGS               1
#define configUSE_OS2_TIMER                      1
#define configUSE_OS2_MUTEX                      1

#define INCLUDE_vTaskPrioritySet                 1
#define INCLUDE_uxTaskPriorityGet                1
#define INCLUDE_vTaskDelete                      1
#define INCLUDE_vTaskCleanUpResources            0
#define INCLUDE_vTaskSuspend                     1
#define INCLUDE_vTaskDelayUntil                  1
#define INCLUDE_vTaskDelay                       1
#define INCLUDE_xTaskGetSchedulerState           1
#define INCLUDE_xTimerPendFunctionCall           1
#define INCLUDE_xQueueGetMutexHolder             1
#define INCLUDE_uxTaskGetStackHighWaterMark      1
#define INCLUDE_xTaskGetCurrentTaskHandle        1
#define INCLUDE_eTaskGetState                    1

#define USE_FreeRTOS_HEAP_4

#define configPRIO_BITS                          4
#define configLIBRARY_LOWEST_INTERRUPT_PRIORITY  15
#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY  5

#define configKERNEL_INTERRUPT_PRIORITY          ( configLIBRARY_LOWEST_INTERRUPT_PRIORITY << (8 - configPRIO_BITS) )
#define configMAX_SYSCALL_INTERRUPT_PRIORITY     ( configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY << (8 - configPRIO_BITS) )

#define configASSERT( x ) if ((x) == 0) {taskDISABLE_INTERRUPTS(); for( ;; );}

#define vPortSVCHandler    	SVC_Handler
#define xPortPendSVHandler 	PendSV_Handler
#define xPortSysTickHandler SysTick_Handler

#define USE_CUSTOM_SYSTICK_HANDLER_IMPLEMENTATION 0

//For segegr sysview
//#include "SEGGER_SYSVIEW_FreeRTOS.h"

#endif /* FREERTOS_CONFIG_H */
