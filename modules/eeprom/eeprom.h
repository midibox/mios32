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
#if defined(MIOS32_PROCESSOR_STM32F103RB)
# define PAGE_SIZE ((uint16_t)0x400)  /* Page size = 1KByte */
# ifndef EEPROM_START_ADDRESS
#  define EEPROM_START_ADDRESS   ((uint32_t)0x0801F800)  // 2*1k at top of flash memory
# endif
#elif defined(MIOS32_PROCESSOR_STM32F103RE)
# define PAGE_SIZE ((uint16_t)0x800)  /* Page size = 1KByte */
# ifndef EEPROM_START_ADDRESS
#  define EEPROM_START_ADDRESS    ((uint32_t)0x0807F000)  // 2*2k at top of flash memory
# endif
#else
# error "Processor not prepared for EEPROM emulation"
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
