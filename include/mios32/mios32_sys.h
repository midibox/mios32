// $Id$
/*
 * Header file for MIOS32 System Initialisation
 *
 * ==========================================================================
 *
 *  Copyright (C) 2008 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _MIOS32_SYS_H
#define _MIOS32_SYS_H

/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////


// constants which define the CPU frequency
// please note: changing this constant won't lead to any frequency change, instead
// timers used by MIOS32 based components (and also FreeRTOS) will be configured
// wrongly.
// In order to change the frequency please add a clock option to MIOS32_SYS_Init()
#ifndef MIOS32_SYS_CPU_FREQUENCY
#if defined(MIOS32_FAMILY_STM32F10x)
# define MIOS32_SYS_CPU_FREQUENCY 72000000
#elif defined(MIOS32_FAMILY_LPC17xx) && defined(MIOS32_PROCESSOR_LPC1768)
# define MIOS32_SYS_CPU_FREQUENCY 100000000
#elif defined(MIOS32_FAMILY_LPC17xx) && defined(MIOS32_PROCESSOR_LPC1769)
# define MIOS32_SYS_CPU_FREQUENCY 120000000
#else
  // dummy
# define MIOS32_SYS_CPU_FREQUENCY 100000000
#endif
#endif


#if defined(MIOS32_FAMILY_LPC17xx)
//! LPC17xx specific help macros for pin initialisation
# define MIOS32_SYS_LPC_PINSEL(port, pin, sel) { LPC_PINCON_TypeDef *LPC_PINSELx = (LPC_PINCON_TypeDef*)(LPC_PINCON_BASE+8*port+4*(pin/16)); LPC_PINSELx->PINSEL0 &= ~(3 << (2*(pin%16))); LPC_PINSELx->PINSEL0 |= (sel << (2*(pin%16))); }
# define MIOS32_SYS_LPC_PINMODE(port, pin, mode) { LPC_PINCON_TypeDef *LPC_PINMODEx = (LPC_PINCON_TypeDef*)(LPC_PINCON_BASE+8*port+4*(pin/16)); LPC_PINMODEx->PINMODE0 &= ~(3 << (2*(pin%16))); LPC_PINMODEx->PINMODE0 |= (mode << (2*(pin%16))); }
# define MIOS32_SYS_LPC_PINMODE_OD(port, pin, od) { LPC_PINCON_TypeDef *LPC_PINMODEx = (LPC_PINCON_TypeDef*)(LPC_PINCON_BASE+4*port); if( od ) LPC_PINMODEx->PINMODE_OD0 |= (1 << pin); else LPC_PINMODEx->PINMODE_OD0 &= ~(1 << pin); }
# define MIOS32_SYS_LPC_PINDIR(port, pin, dir) { LPC_GPIO_TypeDef *LPC_GPIOx = (LPC_GPIO_TypeDef*)(LPC_GPIO0_BASE+port*0x20); if( dir ) LPC_GPIOx->FIODIR |= (1 << pin); else LPC_GPIOx->FIODIR &= ~(1 << pin); }
# define MIOS32_SYS_LPC_PINSET(port, pin, v) { LPC_GPIO_TypeDef *LPC_GPIOx = (LPC_GPIO_TypeDef*)(LPC_GPIO0_BASE+port*0x20); if( v ) LPC_GPIOx->FIOSET = (1 << pin); else LPC_GPIOx->FIOCLR = (1 << pin); }
# define MIOS32_SYS_LPC_PINSET_1(port, pin) { LPC_GPIO_TypeDef *LPC_GPIOx = (LPC_GPIO_TypeDef*)(LPC_GPIO0_BASE+port*0x20); LPC_GPIOx->FIOSET = (1 << pin); }
# define MIOS32_SYS_LPC_PINSET_0(port, pin) { LPC_GPIO_TypeDef *LPC_GPIOx = (LPC_GPIO_TypeDef*)(LPC_GPIO0_BASE+port*0x20); LPC_GPIOx->FIOCLR = (1 << pin); }
// this macro reads FIOPIN of the given port, and masks the value with the given pin
# define MIOS32_SYS_LPC_PINGET(port, pin) (*((volatile u32 *)(LPC_GPIO0_BASE+port*0x20+0x14)) & (1 << pin))
#endif

// STM32 only:
// The DBGMCU_CR register allows to suspend peripherals when CPU is in halt
// state to simplify debugging (e.g. no timer interrupt is triggered each
// time the program is stepped)
// See STM32 reference manual for the meaning of these flags.
// By default, we suspend all peripherals which are provided by DBGMCU_CR
#ifndef MIOS32_SYS_STM32_DBGMCU_CR
#define MIOS32_SYS_STM32_DBGMCU_CR 0x001fff00
#endif


// location of the Device ID and USB device name
// The bootloader update tool allows to change these values from MIOS terminal
#if defined(MIOS32_FAMILY_STM32F10x)
# define MIOS32_SYS_ADDR_BSL_INFO_BEGIN    0x08003f00
#elif defined(MIOS32_FAMILY_LPC17xx)
# define MIOS32_SYS_ADDR_BSL_INFO_BEGIN    0x00003f00
#else
// no warning or error for other families... just don't support these features
#endif

#ifdef MIOS32_SYS_ADDR_BSL_INFO_BEGIN
// NOTE: a change here will mean that:
//   - the bootloader update application (which programs the parameters) has to be re-released
//   - all applications have to be re-released
//   -> better never change the addresses!!!
// New parameters can be added by:
//   - asking TK
//   - searching for an unused offset
//   - adding a confirmation code (parameters will only be taken if confirmation code is 0x42)
//   - adding parameter addresses
# define MIOS32_SYS_ADDR_LCD_PAR_CONFIRM    (MIOS32_SYS_ADDR_BSL_INFO_BEGIN+0xc0) // 0x42 to confirm value
# define MIOS32_SYS_ADDR_LCD_PAR_TYPE       (MIOS32_SYS_ADDR_BSL_INFO_BEGIN+0xc1)
# define MIOS32_SYS_ADDR_LCD_PAR_NUM_X      (MIOS32_SYS_ADDR_BSL_INFO_BEGIN+0xc2)
# define MIOS32_SYS_ADDR_LCD_PAR_NUM_Y      (MIOS32_SYS_ADDR_BSL_INFO_BEGIN+0xc3)
# define MIOS32_SYS_ADDR_LCD_PAR_WIDTH      (MIOS32_SYS_ADDR_BSL_INFO_BEGIN+0xc4)
# define MIOS32_SYS_ADDR_LCD_PAR_HEIGHT     (MIOS32_SYS_ADDR_BSL_INFO_BEGIN+0xc5)

# define MIOS32_SYS_ADDR_DEVICE_ID_CONFIRM  (MIOS32_SYS_ADDR_BSL_INFO_BEGIN+0xd0) // 0x42 to confirm value
# define MIOS32_SYS_ADDR_DEVICE_ID          (MIOS32_SYS_ADDR_BSL_INFO_BEGIN+0xd1)
# define MIOS32_SYS_ADDR_FASTBOOT_CONFIRM   (MIOS32_SYS_ADDR_BSL_INFO_BEGIN+0xd2) // 0x42 to confirm value
# define MIOS32_SYS_ADDR_FASTBOOT           (MIOS32_SYS_ADDR_BSL_INFO_BEGIN+0xd3)
# define MIOS32_SYS_ADDR_SINGLE_USB_CONFIRM (MIOS32_SYS_ADDR_BSL_INFO_BEGIN+0xd4) // 0x42 to confirm value
# define MIOS32_SYS_ADDR_SINGLE_USB         (MIOS32_SYS_ADDR_BSL_INFO_BEGIN+0xd5)


# define MIOS32_SYS_ADDR_USB_DEV_NAME      (MIOS32_SYS_ADDR_BSL_INFO_BEGIN+0xe0) // all chars != 0x00 and 0x20...0x7f to confirm string
# define MIOS32_SYS_USB_DEV_NAME_LEN       0x20

# define MIOS32_SYS_ADDR_BSL_INFO_END      (MIOS32_SYS_ADDR_BSL_INFO_BEGIN+0xff)
#endif


/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////

// *not* compatible to NTP timestamp format, the fraction has mS accuracy
typedef struct {
  u32 seconds;
  u32 fraction_ms;
} mios32_sys_time_t;


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 MIOS32_SYS_Init(u32 mode);

extern s32 MIOS32_SYS_Reset(void);

extern u32 MIOS32_SYS_ChipIDGet(void);
extern u32 MIOS32_SYS_FlashSizeGet(void);
extern u32 MIOS32_SYS_RAMSizeGet(void);
extern s32 MIOS32_SYS_SerialNumberGet(char *str);

extern s32 MIOS32_SYS_TimeSet(mios32_sys_time_t t);
extern mios32_sys_time_t MIOS32_SYS_TimeGet(void);

extern s32 MIOS32_SYS_DMA_CallbackSet(u8 dma, u8 chn, void *callback);


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////


#endif /* _MIOS32_SYS_H */
