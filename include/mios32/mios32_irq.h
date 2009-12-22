// $Id$
/*
 * This file collects all interrupt priorities and provides prototypes to
 * MIOS32_IRQ_* functions
 *
 * ==========================================================================
 *
 *  Copyright (C) 2008 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _MIOS32_IRQ_H
#define _MIOS32_IRQ_H

// we are using 4 bits for pre-emption priority, and no bits for subpriority
// this means: subpriority can always be set to 0, therefore no special 
// define is available for thi setting
#define MIOS32_IRQ_PRIGROUP    NVIC_PriorityGroup_4


// than lower the value, than higher the priority!

// note that FreeRTOS allows priority level < 10 for "SysCalls"
// means: FreeRTOS tasks can be interrupted by level<10 IRQs


// predefined user timer priorities (-> MIOS32_TIMER)
#define MIOS32_IRQ_PRIO_LOW       12  // lower than RTOS
#define MIOS32_IRQ_PRIO_MID        8  // higher than RTOS
#define MIOS32_IRQ_PRIO_HIGH       5  // same like SPI, AIN, etc...
#define MIOS32_IRQ_PRIO_HIGHEST    4  // higher than SPI, AIN, etc...



// DMA Channel IRQ used by MIOS32_SPI
#define MIOS32_IRQ_SPI_DMA_PRIORITY    MIOS32_IRQ_PRIO_HIGH


// DMA Channel IRQ used by MIOS32_I2S, 
// period depends on sample buffer size, but usually 1..2 mS
// relaxed conditions (since samples are transfered in background)
#define MIOS32_IRQ_I2S_DMA_PRIORITY     MIOS32_IRQ_PRIO_HIGH

// DMA Channel IRQ used by MIOS32_AIN, called after 
// all ADC channels have been converted
#define MIOS32_IRQ_AIN_DMA_PRIORITY     MIOS32_IRQ_PRIO_HIGH


// IIC IRQs used by MIOS32_IIC, called rarely on IIC accesses
// should be very high to overcome peripheral flaws (see header of mios32_iic.c)
// estimated requirement for "reaction time": less than 9/400 kHz = 22.5 uS
// EV and ER IRQ should have same priority since they are sharing resources
#define MIOS32_IRQ_IIC_EV_PRIORITY      2
#define MIOS32_IRQ_IIC_ER_PRIORITY      2


// UART IRQs used by MIOS32_UART
// typically called each 320 mS if full MIDI bandwidth is used
// priority should be high to avoid data loss
#define MIOS32_IRQ_UART_PRIORITY        MIOS32_IRQ_PRIO_HIGHEST


// USB provides flow control - this interrupt can run at low priority (but higher than RTOS tasks)
// The interrupt is called at least each mS and takes ca. 1 uS to service the SOF (Start of Frame) flag

#define MIOS32_IRQ_USB_PRIORITY         MIOS32_IRQ_PRIO_MID



/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 MIOS32_IRQ_Disable(void);
extern s32 MIOS32_IRQ_Enable(void);


#endif /* _MIOS32_IRQ_H */
