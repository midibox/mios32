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
#define MIOS32_LCD_BOOT_MSG_LINE1 "CV Autotune"
#define MIOS32_LCD_BOOT_MSG_LINE2 "(C) 2011 T.Klose"

// ensure that TIM2_IRQn handler not allocated by MIOS32 (used in frq_meter.c)
#ifdef MIOS32_FAMILY_STM32F10x
#define MIOS32_DONT_ALLOCATE_TIM2_IRQn 1
#elif MIOS32_FAMILY_LPC17xx
#define MIOS32_DONT_ALLOCATE_TIMER2_IRQ 1
#else
# warning "You should disable the timer irq here"
#endif

#endif /* _MIOS32_CONFIG_H */
