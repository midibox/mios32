// $Id$
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
#define MIOS32_LCD_BOOT_MSG_LINE1 "BLM_SCALAR Communication"
#define MIOS32_LCD_BOOT_MSG_LINE2 "(C) 2016 T.Klose"


// map MIDI mutex to BLM_SCALAR master
// located in tasks.c to access MIDI IN/OUT mutex from external
extern void TASKS_MUTEX_MIDIOUT_Take(void);
extern void TASKS_MUTEX_MIDIOUT_Give(void);
#define BLM_SCALAR_MASTER_MUTEX_MIDIOUT_TAKE { TASKS_MUTEX_MIDIOUT_Take(); }
#define BLM_SCALAR_MASTER_MUTEX_MIDIOUT_GIVE { TASKS_MUTEX_MIDIOUT_Give(); }


#if defined(MIOS32_FAMILY_STM32F10x)
// enable third UART
# define MIOS32_UART_NUM 3
#else
// enable third and fourth UART
# define MIOS32_UART_NUM 4
#endif


#endif /* _MIOS32_CONFIG_H */
