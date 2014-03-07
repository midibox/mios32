// $Id$
/*
 * Header file for EEPROM functions
 *
 * ==========================================================================
 */

#ifndef _EEPROM_H
#define _EEPROM_H

/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////

/* number of half words */
#ifndef EEPROM_EMULATED_SIZE
#define EEPROM_EMULATED_SIZE 128  // -> 128 half words = 256 bytes
#endif


/* Define the STM32F10Xxx Flash page size depending on the used STM32 device */
// TODO: find a better way how to define this in MIOS32
#if defined(MIOS32_FAMILY_STM32F10x)
# if defined(MIOS32_PROCESSOR_STM32F103RB)
#  define PAGE_SIZE ((uint16_t)0x400)  /* Page size = 1KByte */
#  ifndef EEPROM_START_ADDRESS
#   define EEPROM_START_ADDRESS   ((uint32_t)0x0801F800)  // 2*1k at top of flash memory
#  endif
# elif defined(MIOS32_PROCESSOR_STM32F103RE)
#  define PAGE_SIZE ((uint16_t)0x800)  /* Page size = 1KByte */
#  ifndef EEPROM_START_ADDRESS
#   define EEPROM_START_ADDRESS    ((uint32_t)0x0807F000)  // 2*2k at top of flash memory
#  endif
# else
#  error "Processor not prepared for STM32 EEPROM emulation"
# endif

#elif defined(MIOS32_FAMILY_LPC17xx)
// using on-board EEPROM of LPCXPRESSO module which is connected to P0.19 and P0.20
  // set this either to 0 (for first IIC port) or 2 (for second IIC port)
  // IIC1 selects the on-board EEPROM
# ifndef EEPROM_IIC_DEVICE
#  define EEPROM_IIC_DEVICE 1
# endif
  // Set a device number if the 3 address pins are not tied to ground
# ifndef EEPROM_IIC_CS
#  define EEPROM_IIC_CS     0
# endif

#elif defined(MIOS32_FAMILY_STM32F4xx)
# if defined(MIOS32_PROCESSOR_STM32F407VG)
#  define PAGE_SIZE ((uint16_t)0x4000)  /* Page size = 16KByte */
#  ifdef EEPROM_START_ADDRESS
#   error "EEPROM_START_ADDRESS can't be changed for STM32F4 devices!"
#  endif
#   define EEPROM_START_ADDRESS    ((uint32_t)0x08008000)  // the linker script reserves 0x08008000..0x0800ffff for two EEPROM pages
#   define EEPROM_PAGE0_SECTOR     FLASH_Sector_2
#   define EEPROM_PAGE1_SECTOR     FLASH_Sector_3
# else
#  error "Processor not prepared for STM32 EEPROM emulation"
# endif
#else
# error "This MIOS32_FAMILY not prepared for EEPROM (emulation)"
#endif

/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 EEPROM_Init(u32 mode);
extern s32 EEPROM_Read(u16 VirtAddress);
extern s32 EEPROM_Write(u16 VirtAddress, u16 Data);

extern s32 EEPROM_SendDebugMessage(u32 mode);


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////


#endif /* _EEPROM_H */
