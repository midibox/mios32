// $Id: mios32_config.h 387 2009-03-04 23:15:36Z tk $
/*
 * Local MIOS32 configuration file
 *
 * this file allows to disable (or re-configure) default functions of MIOS32
 * available switches are listed in $MIOS32_PATH/modules/mios32/MIOS32_CONFIG.txt
 *
 */

#ifndef _MIOS32_CONFIG_H
#define _MIOS32_CONFIG_H

// The boot message which is print during startup and returned on a SysEx query
#define MIOS32_LCD_BOOT_MSG_LINE1 "uIP Example"
#define MIOS32_LCD_BOOT_MSG_LINE2 "(c) 2009 T.Klose"

// function used to output debug messages (must be printf compatible!)
#define DEBUG_MSG MIOS32_MIDI_SendDebugMessage

// ENC28J60 settings
#define MIOS32_ENC28J60_FULL_DUPLEX 1
// Must be at least 4 bytes larger than UIP_CONF_BUFFER_SIZE
#define MIOS32_ENC28J60_MAX_FRAME_SIZE 1504

// a unique MAC address in your network (6 bytes are required)
// If all bytes are 0, the serial number of STM32 will be taken instead,
// which should be unique in your private network.
#define MIOS32_ENC28J60_MY_MAC_ADDR1 0
#define MIOS32_ENC28J60_MY_MAC_ADDR2 0
#define MIOS32_ENC28J60_MY_MAC_ADDR3 0
#define MIOS32_ENC28J60_MY_MAC_ADDR4 0
#define MIOS32_ENC28J60_MY_MAC_ADDR5 0
#define MIOS32_ENC28J60_MY_MAC_ADDR6 0

#define MIOS32_SDCARD_MUTEX_TAKE    APP_MutexSPI0Take()
#define MIOS32_SDCARD_MUTEX_GIVE    APP_MutexSPI0Give()
#define MIOS32_ENC28J60_MUTEX_TAKE    APP_MutexSPI0Take()
#define MIOS32_ENC28J60_MUTEX_GIVE    APP_MutexSPI0Give()

#define MIOS32_USE_DHCP 	// Use DHCP for IP address configuration

// optional performance measuring
#define configGENERATE_RUN_TIME_STATS           1
#define portCONFIGURE_TIMER_FOR_RUN_TIME_STATS  FREERTOS_UTILS_PerfCounterInit
#define portGET_RUN_TIME_COUNTER_VALUE          FREERTOS_UTILS_PerfCounterGet



// optional task information
#define configUSE_TRACE_FACILITY				1
#define configINCLUDE_vTaskDelete				1
#define configINCLUDE_vTaskSuspend				1


// Disable malloc hook as I want to test if program can survive if malloc fails!
#define configUSE_MALLOC_FAILED_HOOK        1

// Use malloc function. Seems to fail occasionally (best to set the above to 0 as well!)
#define WEBSERVER_USE_MALLOC				0


#endif /* _MIOS32_CONFIG_H */
