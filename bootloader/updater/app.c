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
#include <string.h>
#include <app_lcd.h>

#include "app.h"


#if defined(MIOS32_BOARD_MBHP_CORE_STM32)
# ifdef STM32F10X_CL
#  include "bsl_image_MBHP_CORE_STM32_CL.inc"
# else
#  include "bsl_image_MBHP_CORE_STM32.inc"
#endif
#elif defined(MIOS32_BOARD_STM32F4DISCOVERY) || defined(MIOS32_BOARD_MBHP_CORE_STM32F4)
# include "bsl_image_STM32F4DISCOVERY.inc"
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

// send debug messages to USB0 and UART0
#define DEBUG_MSG(msg) { MIOS32_MIDI_DebugPortSet(UART0); MIOS32_MIDI_SendDebugMessage(msg); MIOS32_MIDI_DebugPortSet(USB0); MIOS32_MIDI_SendDebugMessage(msg); }
#define DEBUG_MSG1(msg, p1) { MIOS32_MIDI_DebugPortSet(UART0); MIOS32_MIDI_SendDebugMessage(msg, p1); MIOS32_MIDI_DebugPortSet(USB0); MIOS32_MIDI_SendDebugMessage(msg, p1); }
#define DEBUG_MSG2(msg, p1, p2) { MIOS32_MIDI_DebugPortSet(UART0); MIOS32_MIDI_SendDebugMessage(msg, p1, p2); MIOS32_MIDI_DebugPortSet(USB0); MIOS32_MIDI_SendDebugMessage(msg, p1, p2); }


#if defined(MIOS32_FAMILY_STM32F10x)
  // STM32: determine page size (mid density devices: 1k, high density devices: 2k)
  // TODO: find a proper way, as there could be high density devices with less than 256k?)
# define FLASH_PAGE_SIZE   (MIOS32_SYS_FlashSizeGet() >= (256*1024) ? 0x800 : 0x400)

  // STM32: flash memory range of BSL
# define BSL_START_ADDR  0x08000000
# define BSL_END_ADDR    0x08003fff
#elif defined(MIOS32_FAMILY_STM32F4xx)
  // STM32F4: flash memory range of BSL
# define FLASH_PAGE_SIZE 0x4000
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
// Local prototypes
/////////////////////////////////////////////////////////////////////////////

static s32 RetrieveBootInfos(void);

static s32 LCD_Update(void);

static s32 TERMINAL_Parse(mios32_midi_port_t port, char byte);
static s32 TERMINAL_ParseLine(char *input, void *_output_function);


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

#define STRING_MAX 80
static char line_buffer[STRING_MAX];
static u16 line_ix;

#ifdef MIOS32_SYS_ADDR_BSL_INFO_BEGIN
static mios32_lcd_parameters_t BSL_lcd_parameters;
static u8 BSL_fastboot;
static char BSL_usb_dev_name[MIOS32_SYS_USB_DEV_NAME_LEN];
static u8 BSL_device_id;
static u8 BSL_single_usb;
#endif


/////////////////////////////////////////////////////////////////////////////
// This hook is called after startup to initialize the application
/////////////////////////////////////////////////////////////////////////////
void APP_Init(void)
{
  // retrieve informations from BSL range
  RetrieveBootInfos();

  // initialize all LEDs
  MIOS32_BOARD_LED_Init(0xffffffff);

  // turn off green LED as a clear indication that core shouldn't be powered-off/rebooted
  MIOS32_BOARD_LED_Set(0xffffffff, 0);

  // install the callback function which is called on incoming characters
  // from MIOS Terminal
  MIOS32_MIDI_DebugCommandCallback_Init(TERMINAL_Parse);

  // clear line buffer
  line_buffer[0] = 0;
  line_ix = 0;
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

  DEBUG_MSG("Checking Bootloader...\n");

  for(addr=0; addr<sizeof(bsl_image); ++addr) {
    // toggle LED (sign of life)
    MIOS32_BOARD_LED_Set(0xffffffff, ~MIOS32_BOARD_LED_Get());

    if( (addr & 0xf) == 0 ) {
      MIOS32_LCD_CursorSet(9, 1);
      MIOS32_LCD_PrintFormattedString("0x%04x", addr);
    }

    if( bsl_addr_ptr[addr] != bsl_image[addr]
#ifdef MIOS32_SYS_ADDR_BSL_INFO_BEGIN
	&& !(addr >= (MIOS32_SYS_ADDR_BSL_INFO_BEGIN & 0xffff) &&
	     addr <= (MIOS32_SYS_ADDR_BSL_INFO_END & 0xffff))
#endif
	) {
      ++mismatches;
      if( mismatches < 10 ) {
	DEBUG_MSG1("Mismatch at address 0x%04x\n", addr);
      } else if( mismatches == 10 ) {
	DEBUG_MSG("Too many mismatches, no additional messages will be print...\n");
      }
    }
  }

  return mismatches;
}

s32 UpdateBSL(void)
{
  // disable interrupts - this is really a critical section!
  MIOS32_IRQ_Disable();

#if defined(MIOS32_FAMILY_STM32F10x) || defined(MIOS32_FAMILY_STM32F4xx)
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
#if defined(MIOS32_FAMILY_STM32F4xx)
      if( (status=FLASH_EraseSector(FLASH_Sector_0, VoltageRange_3)) != FLASH_COMPLETE ) {
	FLASH_ClearFlag(0xffffffff);
	MIOS32_IRQ_Enable();
	return -1; // erase failed
      }
#else
      if( (status=FLASH_ErasePage(addr)) != FLASH_COMPLETE ) {
	FLASH_ClearFlag(0xffffffff);
	MIOS32_IRQ_Enable();
	return -1; // erase failed
      }
#endif
    }

    u16 data_value = bsl_image[i+0] | ((u16)bsl_image[i+1] << 8);

    if( addr >= MIOS32_SYS_ADDR_BSL_INFO_BEGIN &&
	addr <= MIOS32_SYS_ADDR_BSL_INFO_END ) {

      if       ( addr >= MIOS32_SYS_ADDR_LCD_PAR_CONFIRM && (addr < (MIOS32_SYS_ADDR_LCD_PAR_CONFIRM+2)) ) {
	data_value = 0x42 | (BSL_lcd_parameters.lcd_type << 8);
      } else if( addr >= MIOS32_SYS_ADDR_LCD_PAR_NUM_X && (addr < (MIOS32_SYS_ADDR_LCD_PAR_NUM_X+2)) ) {
	data_value = BSL_lcd_parameters.num_x | (BSL_lcd_parameters.num_y << 8);
      } else if( addr >= MIOS32_SYS_ADDR_LCD_PAR_WIDTH && (addr < (MIOS32_SYS_ADDR_LCD_PAR_WIDTH+2)) ) {
	data_value = BSL_lcd_parameters.width | (BSL_lcd_parameters.height << 8);
      } else if( addr >= MIOS32_SYS_ADDR_DEVICE_ID_CONFIRM && (addr < (MIOS32_SYS_ADDR_DEVICE_ID_CONFIRM+2)) ) {
	data_value = 0x42 | (BSL_device_id << 8);
      } else if( addr >= MIOS32_SYS_ADDR_FASTBOOT_CONFIRM && (addr < (MIOS32_SYS_ADDR_FASTBOOT_CONFIRM+2)) ) {
	data_value = 0x42 | (BSL_fastboot << 8);
      } else if( addr >= MIOS32_SYS_ADDR_SINGLE_USB_CONFIRM && (addr < (MIOS32_SYS_ADDR_SINGLE_USB_CONFIRM+2)) ) {
	data_value = 0x42 | (BSL_single_usb << 8);
      } else if( addr >= MIOS32_SYS_ADDR_USB_DEV_NAME && (addr < MIOS32_SYS_ADDR_USB_DEV_NAME+MIOS32_SYS_USB_DEV_NAME_LEN) ) {
	u8 offset = addr-MIOS32_SYS_ADDR_USB_DEV_NAME;
	data_value = BSL_usb_dev_name[offset+0] | ((u16)BSL_usb_dev_name[offset+1] << 8);
      }
    }

    if( (status=FLASH_ProgramHalfWord(addr, data_value)) != FLASH_COMPLETE ) {
      FLASH_ClearFlag(0xffffffff); // clear error flags, otherwise next program attempts will fail
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
    // toggle LED (sign of life)
    MIOS32_BOARD_LED_Set(0xffffffff, ~MIOS32_BOARD_LED_Get());

    // copy into RAM buffer
    int j;
    u8 *ram_buffer_byte_ptr = (u8 *)ram_buffer;
    for(j=0; j<256; ++j)
      *ram_buffer_byte_ptr++ = bsl_image[i+j];

    if( addr >= MIOS32_SYS_ADDR_BSL_INFO_BEGIN &&
	addr <= MIOS32_SYS_ADDR_BSL_INFO_END ) {
#if (MIOS32_SYS_ADDR_BSL_INFO_BEGIN+0xff) != MIOS32_SYS_ADDR_BSL_INFO_END
# error "please update to new address range!"
#endif
      u8 *ram_buffer_byte_ptr = (u8 *)ram_buffer;

      ram_buffer_byte_ptr[0xff & MIOS32_SYS_ADDR_LCD_PAR_CONFIRM] = 0x42;
      ram_buffer_byte_ptr[0xff & MIOS32_SYS_ADDR_LCD_PAR_TYPE] = BSL_lcd_parameters.lcd_type;
      ram_buffer_byte_ptr[0xff & MIOS32_SYS_ADDR_LCD_PAR_NUM_X] = BSL_lcd_parameters.num_x;
      ram_buffer_byte_ptr[0xff & MIOS32_SYS_ADDR_LCD_PAR_NUM_Y] = BSL_lcd_parameters.num_y;
      ram_buffer_byte_ptr[0xff & MIOS32_SYS_ADDR_LCD_PAR_WIDTH] = BSL_lcd_parameters.width;
      ram_buffer_byte_ptr[0xff & MIOS32_SYS_ADDR_LCD_PAR_HEIGHT] = BSL_lcd_parameters.height;

      ram_buffer_byte_ptr[0xff & MIOS32_SYS_ADDR_DEVICE_ID_CONFIRM] = 0x42;
      ram_buffer_byte_ptr[0xff & MIOS32_SYS_ADDR_DEVICE_ID] = BSL_device_id;

      ram_buffer_byte_ptr[0xff & MIOS32_SYS_ADDR_FASTBOOT_CONFIRM] = 0x42;
      ram_buffer_byte_ptr[0xff & MIOS32_SYS_ADDR_FASTBOOT] = BSL_fastboot;

      ram_buffer_byte_ptr[0xff & MIOS32_SYS_ADDR_SINGLE_USB_CONFIRM] = 0x42;
      ram_buffer_byte_ptr[0xff & MIOS32_SYS_ADDR_SINGLE_USB] = BSL_single_usb;

      for(j=0; j<MIOS32_SYS_USB_DEV_NAME_LEN; ++j)
	ram_buffer_byte_ptr[(0xff & MIOS32_SYS_ADDR_USB_DEV_NAME) + j] = BSL_usb_dev_name[j];
    }
    
    s32 status;
    if( (status=find_erase_prepare_sector(SYSTEM_CORE_CLOCK_KHZ, addr)) < 0 ) {
      MIOS32_IRQ_Enable();
      DEBUG_MSG2("erase failed for 0x%08x: code %d\n", addr, status);
      return -1; // erase failed
    } else if( (status=write_data(SYSTEM_CORE_CLOCK_KHZ, addr, (unsigned *)ram_buffer, 256)) < 0 ) {
      MIOS32_IRQ_Enable();
      DEBUG_MSG2("write_data failed for 0x%08x: code %d\n", addr, status);
      return -2; // programming failed
    } else {
      //DEBUG_MSG2("programmed 0x%08x..0x%08x\n", addr, addr+255);
    }

    MIOS32_IRQ_Enable();
    MIOS32_IRQ_Disable();

  }

#else
# error "BSL not prepared for this family"
#endif

  MIOS32_IRQ_Enable();

  RetrieveBootInfos();

  return 0; // no error
}


static s32 RetrieveBootInfos(void)
{
#ifdef MIOS32_SYS_ADDR_BSL_INFO_BEGIN
  int i;

  BSL_lcd_parameters.lcd_type = MIOS32_LCD_TYPE_CLCD;
  BSL_lcd_parameters.num_x = 2; // since MBHP_CORE_STM32 and MBHP_CORE_LPC17 has two J15 ports
  BSL_lcd_parameters.num_y = 1;
  BSL_lcd_parameters.width = 16; // take most common LCD size by default
  BSL_lcd_parameters.height = 2;

  u8 *lcd_par_confirm = (u8 *)MIOS32_SYS_ADDR_LCD_PAR_CONFIRM;
  if( *lcd_par_confirm == 0x42 ) {
    u8 *lcd_par_type = (u8 *)MIOS32_SYS_ADDR_LCD_PAR_TYPE;
    BSL_lcd_parameters.lcd_type = *lcd_par_type;
    u8 *lcd_par_num_x = (u8 *)MIOS32_SYS_ADDR_LCD_PAR_NUM_X;
    BSL_lcd_parameters.num_x = *lcd_par_num_x;
    u8 *lcd_par_num_y = (u8 *)MIOS32_SYS_ADDR_LCD_PAR_NUM_Y;
    BSL_lcd_parameters.num_y = *lcd_par_num_y;
    u8 *lcd_par_width = (u8 *)MIOS32_SYS_ADDR_LCD_PAR_WIDTH;
    BSL_lcd_parameters.width = *lcd_par_width;
    u8 *lcd_par_height = (u8 *)MIOS32_SYS_ADDR_LCD_PAR_HEIGHT;
    BSL_lcd_parameters.height = *lcd_par_height;
  }

  BSL_fastboot = 0x00;
  u8 *fastboot_confirm = (u8 *)MIOS32_SYS_ADDR_FASTBOOT_CONFIRM;
  u8 *fastboot = (u8 *)MIOS32_SYS_ADDR_FASTBOOT;
  if( *fastboot_confirm == 0x42 && *fastboot < 0x80 )
    BSL_fastboot = *fastboot;

  BSL_single_usb = 0x00;
  u8 *single_usb_confirm = (u8 *)MIOS32_SYS_ADDR_SINGLE_USB_CONFIRM;
  u8 *single_usb = (u8 *)MIOS32_SYS_ADDR_SINGLE_USB;
  if( *single_usb_confirm == 0x42 && *single_usb < 0x80 )
    BSL_single_usb = *single_usb;

  BSL_device_id = 0x00;
  u8 *device_id_confirm = (u8 *)MIOS32_SYS_ADDR_DEVICE_ID_CONFIRM;
  u8 *device_id = (u8 *)MIOS32_SYS_ADDR_DEVICE_ID;
  if( *device_id_confirm == 0x42 && *device_id < 0x80 )
    BSL_device_id = *device_id;
  
  if( BSL_device_id != MIOS32_MIDI_DeviceIDGet() ) {
    DEBUG_MSG2("Device ID changed to %d (resp. 0x%02x)!\n", BSL_device_id, BSL_device_id);
    DEBUG_MSG("Please change the Device ID in MIOS Studio accordingly - NOW!");
  }
  MIOS32_MIDI_DeviceIDSet(BSL_device_id);

  u16 *usb_dev_name = (u16 *)MIOS32_SYS_ADDR_USB_DEV_NAME;
  memcpy(BSL_usb_dev_name, usb_dev_name, MIOS32_SYS_USB_DEV_NAME_LEN);

  for(i=0; i<MIOS32_SYS_USB_DEV_NAME_LEN; ++i)
    if( BSL_usb_dev_name[i] < 0x20 || BSL_usb_dev_name[i] >= 0x80 )
      BSL_usb_dev_name[i] = 0x00;
  BSL_usb_dev_name[MIOS32_SYS_USB_DEV_NAME_LEN-1] = 0;
#endif  

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// This task is running endless in background
/////////////////////////////////////////////////////////////////////////////
void APP_Background(void)
{
  // init all specified LCDs
  LCD_Update();

  // print welcome message on MIOS terminal
  MIOS32_LCD_DeviceSet(0);
  DEBUG_MSG("\n");
  DEBUG_MSG("====================\n");
  DEBUG_MSG1("%s\n", MIOS32_LCD_BOOT_MSG_LINE1);
  DEBUG_MSG("====================\n");
  DEBUG_MSG("\n");

  // no mismatches? Fine! Wait for a new application upload (endless)
  if( CompareBSL() == 0 ) {
    MIOS32_LCD_CursorSet(0, 0);
    MIOS32_LCD_PrintString("Bootloader is   "); // 16 chars
    MIOS32_LCD_CursorSet(0, 1);
    MIOS32_LCD_PrintString("up-to-date! :-) ");

    // turn on green LED as a clear indication that core can be powered-off/rebooted
    MIOS32_BOARD_LED_Set(0xffffffff, 1);

    DEBUG_MSG("No mismatches found.\n");
    DEBUG_MSG("The bootloader is up-to-date!\n");
    DEBUG_MSG("You can upload another application now!\n");
    DEBUG_MSG("Or type 'help' in MIOS Terminal for additional options!\n");

    // wait endless
    while( 1 );
  }

  MIOS32_LCD_CursorSet(0, 0);
  MIOS32_LCD_PrintString("Bootloader Update"); // 16 chars

  DEBUG_MSG("Bootloader requires an update...\n");

  MIOS32_LCD_CursorSet(0, 0);
  MIOS32_LCD_PrintString("Starting Update  ");
  MIOS32_LCD_CursorSet(0, 1);
  MIOS32_LCD_PrintString("Don't power off!!");

  DEBUG_MSG("Starting Update - don't power off!!!\n");

  int num_retries = 5;
  int retry = num_retries;
  int mismatches = 0;
  do {
    // update the bootloader
    s32 status;
    if( (status=UpdateBSL()) < 0 ) {
      if( status == -1 ) {
	DEBUG_MSG("Oh-oh! Erase failed!\n");
      } else {
	DEBUG_MSG("Oh-oh! Programming failed!\n");
      }
    }

    // checking again
    mismatches = CompareBSL();

    if( !mismatches )
      retry = 0;
    else {
      --retry;
      if( retry )
	DEBUG_MSG1("Update failed - retry #%d\n", num_retries-retry);
    }
  } while( mismatches != 0 && retry );



  // turn on green LED as a clear indication that core can be powered-off/rebooted
  MIOS32_BOARD_LED_Set(0xffffffff, 1);

  if( !mismatches ) {
    MIOS32_LCD_CursorSet(0, 0);
    MIOS32_LCD_PrintString("Successful Update");
    MIOS32_LCD_CursorSet(0, 1);
    MIOS32_LCD_PrintString("Have fun!        ");

    DEBUG_MSG("The bootloader has been successfully updated!\n");
    DEBUG_MSG("You can upload another application now!\n");
    DEBUG_MSG("Or type 'help' in MIOS Terminal for additional options!\n");

  } else {
    MIOS32_LCD_CursorSet(0, 0);
    MIOS32_LCD_PrintString("Update failed !!! ");
    MIOS32_LCD_CursorSet(0, 1);
    MIOS32_LCD_PrintString("Contact support!  ");


    while( 1 ) {
      DEBUG_MSG("Bootloader Update failed!\n");
      DEBUG_MSG("Thats really unexpected - probably it has to be uploaded via JTAG or UART BSL!\n");
      DEBUG_MSG("Please contact support if required!\n");

      // wait for 1 second before printing the message again
      Wait1Second(1);
    }
  }

  // wait endless
  while( 1 );
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called each mS from the main task which also handles DIN, ENC
// and AIN events. You could add more jobs here, but they shouldn't consume
// more than 300 uS to ensure the responsiveness of buttons, encoders, pots.
// Alternatively you could create a dedicated task for application specific
// jobs as explained in $MIOS32_PATH/apps/tutorials/006_rtos_tasks
/////////////////////////////////////////////////////////////////////////////
void APP_Tick(void)
{
  // toggle the status LED (this is a sign of life)
  MIOS32_BOARD_LED_Set(0x0001, ~MIOS32_BOARD_LED_Get());
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called each mS from the MIDI task which checks for incoming
// MIDI events. You could add more MIDI related jobs here, but they shouldn't
// consume more than 300 uS to ensure the responsiveness of incoming MIDI.
/////////////////////////////////////////////////////////////////////////////
void APP_MIDI_Tick(void)
{
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


/////////////////////////////////////////////////////////////////////////////
// Updates all specified LCDs
/////////////////////////////////////////////////////////////////////////////
static s32 LCD_Update(void)
{
  MIOS32_LCD_Init(0); // retrieve new LCD configuration...

  int num_lcds = mios32_lcd_parameters.num_x * mios32_lcd_parameters.num_y;

  // initialize all LCDs (although programming_models/traditional/main.c will only initialize the first two)
  int lcd;
  for(lcd=0; lcd<num_lcds; ++lcd) {
    MIOS32_MIDI_SendDebugMessage("Initialize LCD #%d\n", lcd+1);
    MIOS32_LCD_DeviceSet(lcd);
    if( MIOS32_LCD_Init(0) < 0 ) {
      MIOS32_MIDI_SendDebugMessage("Failed - no response from LCD #%d.%d\n",
				   (lcd % mios32_lcd_parameters.num_x) + 1,
				   (lcd / mios32_lcd_parameters.num_x) + 1);
    }
  }

  // print text on all LCDs
  for(lcd=0; lcd<num_lcds; ++lcd) {
    MIOS32_LCD_DeviceSet(lcd);
    MIOS32_LCD_CursorSet(0, 0);
    MIOS32_LCD_PrintFormattedString("LCD #%d.%d",
				    (lcd % mios32_lcd_parameters.num_x) + 1,
				    (lcd / mios32_lcd_parameters.num_x) + 1);
    MIOS32_LCD_CursorSet(0, 1);
    MIOS32_LCD_PrintFormattedString("READY.");
  }

  // switch back to first LCD
  MIOS32_LCD_DeviceSet(0);

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// Terminal
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// help function which parses a decimal or hex value
// returns >= 0 if value is valid
// returns -1 if value is invalid
/////////////////////////////////////////////////////////////////////////////
static s32 get_dec(char *word)
{
  if( word == NULL )
    return -1;

  char *next;
  long l = strtol(word, &next, 0);

  if( word == next )
    return -1;

  return l; // value is valid
}

/////////////////////////////////////////////////////////////////////////////
// Parser
/////////////////////////////////////////////////////////////////////////////
static s32 TERMINAL_Parse(mios32_midi_port_t port, char byte)
{
  // temporary change debug port (will be restored at the end of this function)
  mios32_midi_port_t prev_debug_port = MIOS32_MIDI_DebugPortGet();
  MIOS32_MIDI_DebugPortSet(port);

  if( byte == '\r' ) {
    // ignore
  } else if( byte == '\n' ) {
    //MUTEX_MIDIOUT_TAKE;
    TERMINAL_ParseLine(line_buffer, MIOS32_MIDI_SendDebugMessage);
    //MUTEX_MIDIOUT_GIVE;
    line_ix = 0;
    line_buffer[line_ix] = 0;
  } else if( line_ix < (STRING_MAX-1) ) {
    line_buffer[line_ix++] = byte;
    line_buffer[line_ix] = 0;
  }

  // restore debug port
  MIOS32_MIDI_DebugPortSet(prev_debug_port);

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Parser for a complete line - also used by shell.c for telnet
/////////////////////////////////////////////////////////////////////////////
static s32 TERMINAL_ParseLine(char *input, void *_output_function)
{
  void (*out)(char *format, ...) = _output_function;
  char *separators = " \t";
  char *brkt;
  char *parameter;

#ifdef MIOS32_LCD_universal
  if( APP_LCD_TerminalParseLine(input, _output_function) >= 1 )
    return 0; // command parsed
#endif

  if( (parameter = strtok_r(input, separators, &brkt)) ) {
    if( strcmp(parameter, "help") == 0 ) {
      out("Welcome to " MIOS32_LCD_BOOT_MSG_LINE1 "!");
      out("Following commands are available:");
      out("  set fastboot <1 or 0>:   if 1, the initial bootloader wait phase will be skipped (current: %d)\n", BSL_fastboot);
      out("  set single_usb <1 or 0>: if 1, USB will only be initialized for a single port (current: %d)\n", BSL_single_usb);
      out("  set device_id <value>:   sets MIOS32 Device ID to given value (current: %d resp. 0x%02x)\n",
	  BSL_device_id, BSL_device_id);
      out("  set usb_name <name>:     sets USB device name (current: '%s')\n", BSL_usb_dev_name);
      out("  set lcd_type <value>:    sets LCD type ID (current: 0x%02x - %s)\n",
	  BSL_lcd_parameters.lcd_type,
	  MIOS32_LCD_LcdTypeName(BSL_lcd_parameters.lcd_type)
	  ? MIOS32_LCD_LcdTypeName(BSL_lcd_parameters.lcd_type)
	  : "unknown");
      out("  set lcd_num_x <value>:   sets number of LCD devices (X direction, current: %d)\n", BSL_lcd_parameters.num_x);
      out("  set lcd_num_y <value>:   sets number of LCD devices (Y direction, current: %d)\n", BSL_lcd_parameters.num_y);
      out("  set lcd_width <value>:   sets width of a single LCD (current: %d)\n", BSL_lcd_parameters.width);
      out("  set lcd_height <value>:  sets height of a single LCD (current: %d)\n", BSL_lcd_parameters.height);
      out("  lcd_types:               lists all known LCD types\n");
#ifdef MIOS32_LCD_universal
      APP_LCD_TerminalHelp(_output_function);
#endif
      out("  store:                   stores the changed settings in flash (and updates all LCDs)\n");
      out("  restore:                 restores previous settings from flash\n");
      out("  reset:                   resets the MIDIbox (!)\n");
      out("  help:                    this page");
    } else if( strcmp(parameter, "lcd_types") == 0 ) {
      out("List of known LCD types:\n");
      u8 lcd_type;
      for(lcd_type=0; lcd_type<255; ++lcd_type) {
	if( MIOS32_LCD_LcdTypeName(lcd_type) ) {
	  out("0x%02x: %s\n", lcd_type, MIOS32_LCD_LcdTypeName(lcd_type));
	}
      }
      out("You can change a LCD type with 'set lcd_type <value>'\n");
      out("Please note that newer types could have been integrated after this application has been released!");
    } else if( strcmp(parameter, "reset") == 0 ) {
      MIOS32_SYS_Reset();
    } else if( strcmp(parameter, "store") == 0 ) {
      
      int retry;
      s32 status = -1;
      for(retry=0; status < 0 && retry<5; ++retry) {
	if( (status = UpdateBSL()) < 0 ) {
	  out("Failed to store new settings - retry #%d", retry+1);
	}
      }

      if( status >= 0 )
	out("New settings stored.");
      else
	out("Failed to store new settings!");

      LCD_Update();
    } else if( strcmp(parameter, "restore") == 0 ) {
      RetrieveBootInfos();
      out("Previous settings restored.");
      LCD_Update();
    } else if( strcmp(parameter, "set") == 0 ) {
      if( (parameter = strtok_r(NULL, separators, &brkt)) ) {
	if( strcmp(parameter, "fastboot") == 0 ) {
	  s32 value = -1;
	  if( (parameter = strtok_r(NULL, separators, &brkt)) )
	    value = get_dec(parameter);

	  if( value < 0 || value > 1 ) {
	    out("Expecting Fastboot 0 or 1!");
	  } else {
	    BSL_fastboot = value;
	    if( BSL_fastboot )
	      out("Fastboot enabled after 'store'!");
	    else
	      out("Fastboot disabled after 'store'!");
	  }
	} else if( strcmp(parameter, "single_usb") == 0 ) {
	  s32 value = -1;
	  if( (parameter = strtok_r(NULL, separators, &brkt)) )
	    value = get_dec(parameter);

	  if( value < 0 || value > 1 ) {
	    out("Expecting single_usb 0 or 1!");
	  } else {
	    BSL_single_usb = value;
	    if( BSL_single_usb )
	      out("Forcing of a single USB port enabled after 'store'!");
	    else
	      out("Forcing of a single USB port disabled after 'store'!");
	  }
	} else if( strcmp(parameter, "id") == 0 || strcmp(parameter, "device_id") == 0 ) {
	  s32 value = -1;
	  if( (parameter = strtok_r(NULL, separators, &brkt)) )
	    value = get_dec(parameter);

	  if( value < 0 || value > 127 ) {
	    out("Expecting Device ID between 0..127 (resp. 0x00..0x7f)!");
	  } else {
	    BSL_device_id = value;
	    out("Device ID will be changed to %d (0x%02x) after 'store'!",
		BSL_device_id, BSL_device_id);
	    out("Enter 'store' to permanently save this setting in flash!");
	    out("Please change the Device ID in MIOS Studio accordingly after 'store'!");
	  }
	} else if( strcmp(parameter, "lcd_type") == 0 ) {
	  s32 value = -1;
	  if( (parameter = strtok_r(NULL, separators, &brkt)) )
	    value = get_dec(parameter);

	  if( value < 0 ) { // try the name
	    u8 lcd_type;
	    for(lcd_type=0; lcd_type<255; ++lcd_type) {
	      if( strcasecmp(parameter, MIOS32_LCD_LcdTypeName(lcd_type)) == 0 ) {
		value = lcd_type;
		break;
	      }
	    }
	  }

	  if( value < 0 || value > 255 ) {
	    out("Expecting LCD type between 0x00 and 0xff or a matching LCD name!");
	    out("Enter 'lcd_types' to get a list of valid IDs!");
	  } else {
	    BSL_lcd_parameters.lcd_type = value;
	    out("LCD type set to 0x%02x (%s) after 'store'!",
		BSL_lcd_parameters.lcd_type,
		MIOS32_LCD_LcdTypeName(BSL_lcd_parameters.lcd_type)
		? MIOS32_LCD_LcdTypeName(BSL_lcd_parameters.lcd_type)
		: "unknown");
	  }
	} else if( strcmp(parameter, "lcd_num_x") == 0 ) {
	  s32 value = -1;
	  if( (parameter = strtok_r(NULL, separators, &brkt)) )
	    value = get_dec(parameter);

	  if( value < 1 || value > 255 ) {
	    out("Expecting num_x between 1 and 255!");
	  } else {
	    BSL_lcd_parameters.num_x = value;
	    out("LCD num_x set to %d after 'store'!", BSL_lcd_parameters.num_x);
	  }
	} else if( strcmp(parameter, "lcd_num_y") == 0 ) {
	  s32 value = -1;
	  if( (parameter = strtok_r(NULL, separators, &brkt)) )
	    value = get_dec(parameter);

	  if( value < 1 || value > 255 ) {
	    out("Expecting num_y between 1 and 255!");
	  } else {
	    BSL_lcd_parameters.num_y = value;
	    out("LCD num_y set to %d after 'store'!", BSL_lcd_parameters.num_y);
	  }
	} else if( strcmp(parameter, "lcd_width") == 0 ) {
	  s32 value = -1;
	  if( (parameter = strtok_r(NULL, separators, &brkt)) )
	    value = get_dec(parameter);

	  if( value < 1 || value > 65535 ) {
	    out("Expecting LCD width between 1 and 65535!");
	  } else {
	    BSL_lcd_parameters.width = value;
	    out("LCD width set to %d after 'store'!", BSL_lcd_parameters.width);
	  }
	} else if( strcmp(parameter, "lcd_height") == 0 ) {
	  s32 value = -1;
	  if( (parameter = strtok_r(NULL, separators, &brkt)) )
	    value = get_dec(parameter);

	  if( value < 1 || value > 65535 ) {
	    out("Expecting LCD height between 1 and 65535!");
	  } else {
	    BSL_lcd_parameters.height = value;
	    out("LCD height set to %d after 'store'!", BSL_lcd_parameters.height);
	  }
	} else if( strcmp(parameter, "name") == 0 ||
		   strcmp(parameter, "usb_name") == 0 ||
		   strcmp(parameter, "usb_dev_name") == 0) {
	  if( strlen(brkt) > (MIOS32_SYS_USB_DEV_NAME_LEN-1) ) {
	    out("This name is too long! %d characters maximum.", MIOS32_SYS_USB_DEV_NAME_LEN-1);
	  } else {
	    memcpy(BSL_usb_dev_name, brkt, MIOS32_SYS_USB_DEV_NAME_LEN);
	    if( BSL_usb_dev_name[0] )
	      out("Customized USB device name changed to '%s' after 'store'.\n", BSL_usb_dev_name);
	    else
	      out("Customized USB device name disabled after 'store'.");
	    out("Enter 'store' to permanently save this setting in flash!");
	  }
	} else {
	  out("Unknown set parameter: '%s'!", parameter);
	}
      } else {
	out("Missing parameter after 'set'!");
      }
    } else {
      out("Unknown command - type 'help' to list available commands!");
    }
  }

  return 0; // no error
}
