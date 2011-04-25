// $Id$
/*
 * Bootloader Update
 * See README.txt for details
 *
 * ==========================================================================
 *
 *  Copyright (C) 2009 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

/////////////////////////////////////////////////////////////////////////////
// Include files
/////////////////////////////////////////////////////////////////////////////

#include <mios32.h>
#include "app.h"


#if defined(MIOS32_BOARD_MBHP_CORE_STM32)
#ifdef STM32F10X_CL
# include "bsl_image_MBHP_CORE_STM32_CL.inc"
#else
# include "bsl_image_MBHP_CORE_STM32.inc"
#endif
#elif defined(MIOS32_BOARD_LPCXPRESSO) || defined(MIOS32_BOARD_MBHP_CORE_LPC17)
#if defined(MIOS32_PROCESSOR_LPC1769)
# include "bsl_image_LPCXPRESSO_1769.inc"
#else
# include "bsl_image_LPCXPRESSO_1768.inc"
#endif
#else
# error "BSL update not supported for the selected MIOS32_BOARD"
#endif


/////////////////////////////////////////////////////////////////////////////
// Local Macros
/////////////////////////////////////////////////////////////////////////////

#if defined(MIOS32_FAMILY_STM32F10x)
  // STM32: determine page size (mid density devices: 1k, high density devices: 2k)
  // TODO: find a proper way, as there could be high density devices with less than 256k?)
# define FLASH_PAGE_SIZE   (MIOS32_SYS_FlashSizeGet() >= (256*1024) ? 0x800 : 0x400)

  // STM32: flash memory range of BSL
# define BSL_START_ADDR  0x08000000
# define BSL_END_ADDR    0x08003fff
#elif defined(MIOS32_FAMILY_LPC17xx)

#include <sbl_iap.h>
#include <sbl_config.h>

# undef  FLASH_BUF_SIZE
# define FLASH_BUF_SIZE    BSL_SYSEX_BUFFER_SIZE
# undef  USER_START_SECTOR
# define USER_START_SECTOR  0
# define MAX_USER_SECTOR   29
# define USER_FLASH_START (sector_start_map[USER_START_SECTOR])
# define USER_FLASH_END   (sector_end_map[MAX_USER_SECTOR])
# define USER_FLASH_SIZE  ((USER_FLASH_END - USER_FLASH_START) + 1)

  // LPC17xx: sectors have different sizes
const unsigned sector_start_map[MAX_FLASH_SECTOR] = {SECTOR_0_START,             \
SECTOR_1_START,SECTOR_2_START,SECTOR_3_START,SECTOR_4_START,SECTOR_5_START,      \
SECTOR_6_START,SECTOR_7_START,SECTOR_8_START,SECTOR_9_START,SECTOR_10_START,     \
SECTOR_11_START,SECTOR_12_START,SECTOR_13_START,SECTOR_14_START,SECTOR_15_START, \
SECTOR_16_START,SECTOR_17_START,SECTOR_18_START,SECTOR_19_START,SECTOR_20_START, \
SECTOR_21_START,SECTOR_22_START,SECTOR_23_START,SECTOR_24_START,SECTOR_25_START, \
SECTOR_26_START,SECTOR_27_START,SECTOR_28_START,SECTOR_29_START                  };

const unsigned sector_end_map[MAX_FLASH_SECTOR] = {SECTOR_0_END,SECTOR_1_END,    \
SECTOR_2_END,SECTOR_3_END,SECTOR_4_END,SECTOR_5_END,SECTOR_6_END,SECTOR_7_END,   \
SECTOR_8_END,SECTOR_9_END,SECTOR_10_END,SECTOR_11_END,SECTOR_12_END,             \
SECTOR_13_END,SECTOR_14_END,SECTOR_15_END,SECTOR_16_END,SECTOR_17_END,           \
SECTOR_18_END,SECTOR_19_END,SECTOR_20_END,SECTOR_21_END,SECTOR_22_END,           \
SECTOR_23_END,SECTOR_24_END,SECTOR_25_END,SECTOR_26_END,                         \
SECTOR_27_END,SECTOR_28_END,SECTOR_29_END                                        };


  // expected by flash programming routine: system core clock (100/120 MHz) in kHz
# define SYSTEM_CORE_CLOCK_KHZ (MIOS32_SYS_CPU_FREQUENCY/1000)

  // LPC17xx: flash memory range of BSL
# define BSL_START_ADDR  (0x00000000)
# define BSL_END_ADDR    (0x00003fff)

static void iap_entry(unsigned param_tab[], unsigned result_tab[]);
static s32 write_data(unsigned cclk,unsigned flash_address, unsigned *flash_data_buf, unsigned count);
static s32 find_erase_prepare_sector(unsigned cclk, unsigned flash_address);
static s32 erase_sector(unsigned start_sector, unsigned end_sector, unsigned cclk);
static s32 prepare_sector(unsigned start_sector, unsigned end_sector, unsigned cclk);

#else
# error "Updater not prepared for this family"
#endif


/////////////////////////////////////////////////////////////////////////////
// This hook is called after startup to initialize the application
/////////////////////////////////////////////////////////////////////////////
void APP_Init(void)
{
  // initialize all LEDs
  MIOS32_BOARD_LED_Init(0xffffffff);

  // turn off green LED as a clear indication that core shouldn't be powered-off/rebooted
  MIOS32_BOARD_LED_Set(0xffffffff, 0);
}


/////////////////////////////////////////////////////////////////////////////
// Help functions
/////////////////////////////////////////////////////////////////////////////
s32 Wait1Second(u8 dont_toggle_led)
{
  int i;

  for(i=0; i<50; ++i) {
    if( !dont_toggle_led ) {
      // toggle LED and wait for 20 mS (sign of life)
      MIOS32_BOARD_LED_Set(0xffffffff, ~MIOS32_BOARD_LED_Get());
    }
    MIOS32_DELAY_Wait_uS(20000);
  }

  return 0; // no error
}

s32 CompareBSL(void)
{
  // comparing BSL range with binary image
  u32 addr;
  u8 *bsl_addr_ptr = (u8 *)BSL_START_ADDR;
  int mismatches = 0;

  MIOS32_LCD_CursorSet(0, 0);
  MIOS32_LCD_PrintString("BootloaderUpdate"); // 16 chars
  MIOS32_LCD_CursorSet(0, 1);
  MIOS32_LCD_PrintString("Checking        ");

  MIOS32_MIDI_SendDebugMessage("Checking Bootloader...\n");

  for(addr=0; addr<sizeof(bsl_image); ++addr) {
    // toggle LED (sign of life)
    MIOS32_BOARD_LED_Set(0xffffffff, ~MIOS32_BOARD_LED_Get());

    if( (addr & 0xf) == 0 ) {
      MIOS32_LCD_CursorSet(9, 1);
      MIOS32_LCD_PrintFormattedString("0x%04x", addr);
    }

    if( bsl_addr_ptr[addr] != bsl_image[addr] ) {
      ++mismatches;
      if( mismatches < 10 ) {
	MIOS32_MIDI_SendDebugMessage("Mismatch at address 0x%04x\n", addr);
      } else if( mismatches == 10 ) {
	MIOS32_MIDI_SendDebugMessage("Too many mismatches, no additional messages will be print...\n");
      }
    }
  }

  return mismatches;
}

s32 UpdateBSL(void)
{
  // disable interrupts - this is really a critical section!
  MIOS32_IRQ_Disable();

  // Note: Device ID will be overwritten as well - this is intended, because it
  // can only be programmed once after flash has been erased!
  MIOS32_MIDI_DeviceIDSet(0x00);

#if defined(MIOS32_FAMILY_STM32F10x)
  // FLASH_* routines are part of the STM32 code library
  FLASH_Unlock();

  FLASH_Status status;
  int i;
  u32 addr = BSL_START_ADDR;
  u32 len = BSL_END_ADDR - BSL_START_ADDR + 1;
  
  for(i=0; i<len; addr+=2, i+=2) {
    // toggle LED (sign of life)
    MIOS32_BOARD_LED_Set(0xffffffff, ~MIOS32_BOARD_LED_Get());

    if( (addr % FLASH_PAGE_SIZE) == 0 ) {
      if( (status=FLASH_ErasePage(addr)) != FLASH_COMPLETE ) {
	FLASH_ClearFlag(FLASH_FLAG_PGERR | FLASH_FLAG_WRPRTERR); // clear error flags, otherwise next program attempts will fail
	MIOS32_IRQ_Enable();
	return -1; // erase failed
      }
    }

    if( (status=FLASH_ProgramHalfWord(addr, bsl_image[i+0] | ((u16)bsl_image[i+1] << 8))) != FLASH_COMPLETE ) {
      FLASH_ClearFlag(FLASH_FLAG_PGERR | FLASH_FLAG_WRPRTERR); // clear error flags, otherwise next program attempts will fail
      MIOS32_IRQ_Enable();
      return -2; // programming failed
    }
  }

  // ensure that flash write access is locked
  FLASH_Lock();

#elif defined(MIOS32_FAMILY_LPC17xx)

  int i;
  u32 addr = BSL_START_ADDR;
  u32 len = BSL_END_ADDR - BSL_START_ADDR + 1;
  u32 ram_buffer[256/4];

  for(i=0; i<len; addr+=256, i+=256) {
    MIOS32_MIDI_SendDebugMessage("i %d len %d\n", i, len);

    // toggle LED (sign of life)
    MIOS32_BOARD_LED_Set(0xffffffff, ~MIOS32_BOARD_LED_Get());

    // copy into RAM buffer
    int j;
    u8 *ram_buffer_byte_ptr = (u8 *)ram_buffer;
    for(j=0; j<256; ++j)
      *ram_buffer_byte_ptr++ = bsl_image[i+j];
    
    s32 status;
    if( (status=find_erase_prepare_sector(SYSTEM_CORE_CLOCK_KHZ, addr)) < 0 ) {
      MIOS32_IRQ_Enable();
      MIOS32_MIDI_SendDebugMessage("erase failed for 0x%08x: code %d\n", addr, status);
      return -1; // erase failed
    } else if( (status=write_data(SYSTEM_CORE_CLOCK_KHZ, addr, (unsigned *)ram_buffer, 256)) < 0 ) {
      MIOS32_IRQ_Enable();
      MIOS32_MIDI_SendDebugMessage("write_data failed for 0x%08x: code %d\n", addr, status);
      return -2; // programming failed
    } else {
      MIOS32_MIDI_SendDebugMessage("programmed 0x%08x..0x%08x\n", addr, addr+255);
    }

    MIOS32_IRQ_Enable();
    MIOS32_IRQ_Disable();

  }

#else
# error "BSL not prepared for this family"
#endif

  MIOS32_IRQ_Enable();

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// This task is running endless in background
/////////////////////////////////////////////////////////////////////////////
void APP_Background(void)
{
  // init LCD
  MIOS32_LCD_Clear();

  // print welcome message on MIOS terminal
  MIOS32_MIDI_SendDebugMessage("\n");
  MIOS32_MIDI_SendDebugMessage("====================\n");
  MIOS32_MIDI_SendDebugMessage("%s\n", MIOS32_LCD_BOOT_MSG_LINE1);
  MIOS32_MIDI_SendDebugMessage("====================\n");
  MIOS32_MIDI_SendDebugMessage("\n");

  // no mismatches? Fine! Wait for a new application upload (endless)
  if( CompareBSL() == 0 ) {
    MIOS32_LCD_CursorSet(0, 0);
    MIOS32_LCD_PrintString("Bootloader is   "); // 16 chars
    MIOS32_LCD_CursorSet(0, 1);
    MIOS32_LCD_PrintString("up-to-date! :-) ");

    // turn on green LED as a clear indication that core can be powered-off/rebooted
    MIOS32_BOARD_LED_Set(0xffffffff, 1);

    while( 1 ) {
      MIOS32_MIDI_SendDebugMessage("No mismatches found.\n");
      MIOS32_MIDI_SendDebugMessage("The bootloader is up-to-date!\n");
      MIOS32_MIDI_SendDebugMessage("You can upload another application now!\n");

      // wait for 1 second before printing the message again
      Wait1Second(1); // don't toggle LED!
    }
  }

  MIOS32_LCD_CursorSet(0, 0);
  MIOS32_LCD_PrintString("Bootloader Update"); // 16 chars

  MIOS32_MIDI_SendDebugMessage("Bootloader requires an update...\n");

  int i;
  for(i=10; i>=0; --i) {
    MIOS32_LCD_CursorSet(0, 1);
    MIOS32_LCD_PrintFormattedString("in %2d seconds! ", i);
    MIOS32_MIDI_SendDebugMessage("Bootloader update in %d seconds!\n", i);
    Wait1Second(0);
  }

  MIOS32_LCD_CursorSet(0, 0);
  MIOS32_LCD_PrintString("Starting Update  ");
  MIOS32_LCD_CursorSet(0, 1);
  MIOS32_LCD_PrintString("Don't power off!!");

  MIOS32_MIDI_SendDebugMessage("Starting Update - don't power off!!!\n");

  int num_retries = 5;
  int retry = num_retries;
  int mismatches = 0;
  do {
    // update the bootloader
    s32 status;
    if( (status=UpdateBSL()) < 0 ) {
      if( status == -1 ) {
	MIOS32_MIDI_SendDebugMessage("Oh-oh! Erase failed!\n");
      } else {
	MIOS32_MIDI_SendDebugMessage("Oh-oh! Programming failed!\n");
      }
    }

    // checking again
    mismatches = CompareBSL();

    if( !mismatches )
      retry = 0;
    else {
      --retry;
      if( retry )
	MIOS32_MIDI_SendDebugMessage("Update failed - retry #%d\n", num_retries-retry);
    }
  } while( mismatches != 0 && retry );



  // turn on green LED as a clear indication that core can be powered-off/rebooted
  MIOS32_BOARD_LED_Set(0xffffffff, 1);

  if( !mismatches ) {
    MIOS32_LCD_CursorSet(0, 0);
    MIOS32_LCD_PrintString("Successful Update");
    MIOS32_LCD_CursorSet(0, 1);
    MIOS32_LCD_PrintString("Have fun!        ");

    while( 1 ) {
      MIOS32_MIDI_SendDebugMessage("The bootloader has been successfully updated!\n");
      MIOS32_MIDI_SendDebugMessage("You can upload another application now!\n");

      // wait for 1 second before printing the message again
      Wait1Second(1);
    }
  } else {
    MIOS32_LCD_CursorSet(0, 0);
    MIOS32_LCD_PrintString("Update failed !!! ");
    MIOS32_LCD_CursorSet(0, 1);
    MIOS32_LCD_PrintString("Contact support!  ");

    while( 1 ) {
      MIOS32_MIDI_SendDebugMessage("Bootloader Update failed!\n");
      MIOS32_MIDI_SendDebugMessage("Thats really unexpected - probably it has to be uploaded via JTAG or UART BSL!\n");
      MIOS32_MIDI_SendDebugMessage("Please contact support if required!\n");

      // wait for 1 second before printing the message again
      Wait1Second(1);
    }
  }
}


/////////////////////////////////////////////////////////////////////////////
//  This hook is called when a MIDI package has been received
/////////////////////////////////////////////////////////////////////////////
void APP_MIDI_NotifyPackage(mios32_midi_port_t port, mios32_midi_package_t midi_package)
{
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called before the shift register chain is scanned
/////////////////////////////////////////////////////////////////////////////
void APP_SRIO_ServicePrepare(void)
{
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called after the shift register chain has been scanned
/////////////////////////////////////////////////////////////////////////////
void APP_SRIO_ServiceFinish(void)
{
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called when a button has been toggled
// pin_value is 1 when button released, and 0 when button pressed
/////////////////////////////////////////////////////////////////////////////
void APP_DIN_NotifyToggle(u32 pin, u32 pin_value)
{
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called when an encoder has been moved
// incrementer is positive when encoder has been turned clockwise, else
// it is negative
/////////////////////////////////////////////////////////////////////////////
void APP_ENC_NotifyChange(u32 encoder, s32 incrementer)
{
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called when a pot has been moved
/////////////////////////////////////////////////////////////////////////////
void APP_AIN_NotifyChange(u32 pin, u32 pin_value)
{
}


#if defined(MIOS32_FAMILY_LPC17xx)

// to enter IAP routines
static void iap_entry(unsigned param_tab[], unsigned result_tab[])
{
  void (*iap)(unsigned [],unsigned []);

  iap = (void (*)(unsigned [],unsigned []))IAP_ADDRESS;
  iap(param_tab,result_tab);
}

s32 find_erase_prepare_sector(unsigned cclk, unsigned flash_address)
{
  unsigned i;
  s32 result = 0;

  for(i=USER_START_SECTOR; i<=MAX_USER_SECTOR; i++) {
    if(flash_address < sector_end_map[i]) {
      if( flash_address == sector_start_map[i]) {
	if( prepare_sector(i, i, cclk) < 0 )
	  result = -1;
	if( erase_sector(i, i ,cclk) < 0 )
	  result = -2;
      }
      if( prepare_sector(i, i, cclk) < 0 )
	result = -3;
      break;
    }
  }

  return result;
}

s32 write_data(unsigned cclk, unsigned flash_address, unsigned *flash_data_buf, unsigned count)
{
  unsigned param_table[5];
  unsigned result_table[5];

  param_table[0] = COPY_RAM_TO_FLASH;
  param_table[1] = flash_address;
  param_table[2] = (unsigned)flash_data_buf;
  param_table[3] = count;
  param_table[4] = cclk;
  iap_entry(param_table, result_table);

  return -(s32)result_table[0];
}

s32 erase_sector(unsigned start_sector, unsigned end_sector, unsigned cclk)
{
  unsigned param_table[5];
  unsigned result_table[5];

  param_table[0] = ERASE_SECTOR;
  param_table[1] = start_sector;
  param_table[2] = end_sector;
  param_table[3] = cclk;
  iap_entry(param_table, result_table);

  return -(s32)result_table[0];
}

s32 prepare_sector(unsigned start_sector, unsigned end_sector, unsigned cclk)
{
  unsigned param_table[5];
  unsigned result_table[5];

  param_table[0] = PREPARE_SECTOR_FOR_WRITE;
  param_table[1] = start_sector;
  param_table[2] = end_sector;
  param_table[3] = cclk;
  iap_entry(param_table, result_table);

  return -(s32)result_table[0];
}

#endif
