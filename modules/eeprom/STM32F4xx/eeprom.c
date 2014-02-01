// $Id: eeprom.c 1253 2011-07-14 22:45:50Z tk $
/**
  ******************************************************************************
  * @file    EEPROM_Emulation/src/eeprom.c 
  * @author  originally MCD Application Team, adapted to MIOS32 API by Thorsten Klose
  * @version based on V3.1.0
  * @brief   This file provides all the EEPROM emulation firmware functions.
  ******************************************************************************
  *
  * Modified by TK:
  *   - including <mios32.h>, especially for importing mios32_defines.inc
  *   - removed VirtAddVarTab - we use a linear address space from 
  *     0..EEPROM_EMULATED_SIZE-1
  *   - DataVar not global anymore (bad programming style, no re-entrancy, etc..)
  *   - #define statements which are only used by module are not defined in
  *     eeprom.h anymore
  *   - adapted API to "MIOS32 style"
  *   - flash access locked/unlocked whenever required
  *   - if value already exist in EEPROM, it won't be programmed again
  *   - added EEPROM_SendDebugMessage()
  *   - added the usage infos below
  *
  ******************************************************************************
  * @copy
  *
  * THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
  * WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE
  * TIME. AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY
  * DIRECT, INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING
  * FROM THE CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE
  * CODING INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
  *
  * <h2><center>&copy; COPYRIGHT 2009 STMicroelectronics</center></h2>
  */ 

//! \defgroup EEPROM
//!
//! Usage:
//! <UL>
//!   <LI>EEPROM_Init(u32 mode) should be called after startup to check for valid flash pages.
//!   Pages will be formatted if this hasn't been done before.
//!   <LI>EEPROM_Read(u16 address) reads a 16bit value from EEPROM<BR>
//!       Returns <0 if address hasn't been programmed yet (it's up to the
//!       application, how to handle this, e.g. value could be zeroed)
//!   <LI>EEPROM_Write(u16 address, u16 value): programs the 16bit value
//! </UL>
//!
//! Configuration: optionally EEPROM_EMULATED_SIZE can be overruled in mios32_config.h
//! to change the number of virtual addresses.
//!
//! By default, 128 addresses are available:<BR>
//! \code
//! #define EEPROM_EMULATED_SIZE 128  // -> 128 half words = 256 bytes
//! \endcode
//!
//! Note: each address allocates 4 bytes in flash. The emulation can handle with
//! two pages (STM32F103RB: 2*1k, STM32F103RE: 2*2k), and the number of addresses
//! shouldn't exceed (page size - 4).
//!
//! Accordingly, the maximum EEPROM_EMULATED_SIZE for STM32F103RB is 255, and for
//! STM32F103RE 511.<BR>
//! Than lower the specified size, than faster EEPROM_Write() will work, especially
//! once pages have to be switched.
//!
//! Example application:<BR>
//!   $MIOS32_PATH/apps/tutorials/025_sysex_and_eeprom (see patch.c)
//!
//! Additional informations to EEPROM emulation approach:<BR>
//!   http://www.st.com/stonline/products/literature/an/13718.pdf
//!
//! \{

/* Includes ------------------------------------------------------------------*/
#include <mios32.h>
#include "eeprom.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/

/* Pages 0 and 1 base and end addresses */
#define PAGE0_BASE_ADDRESS      ((uint32_t)(EEPROM_START_ADDRESS + 0x000))
#define PAGE0_END_ADDRESS       ((uint32_t)(EEPROM_START_ADDRESS + (PAGE_SIZE - 1)))

#define PAGE1_BASE_ADDRESS      ((uint32_t)(EEPROM_START_ADDRESS + PAGE_SIZE))
#define PAGE1_END_ADDRESS       ((uint32_t)(EEPROM_START_ADDRESS + (2 * PAGE_SIZE - 1)))

/* Used Flash pages for EEPROM emulation */
#define PAGE0                   ((uint16_t)0x0000)
#define PAGE1                   ((uint16_t)0x0001)

/* No valid page define */
#define NO_VALID_PAGE           ((uint16_t)0x00AB)

/* Page status definitions */
#define ERASED                  ((uint16_t)0xFFFF)     /* PAGE is empty */
#define RECEIVE_DATA            ((uint16_t)0xEEEE)     /* PAGE is marked to receive data */
#define VALID_PAGE              ((uint16_t)0x0000)     /* PAGE containing valid data */

/* Valid pages in read and write defines */
#define READ_FROM_VALID_PAGE    ((uint8_t)0x00)
#define WRITE_IN_VALID_PAGE     ((uint8_t)0x01)

/* Page full define */
#define PAGE_FULL               ((uint8_t)0x80)

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/

/* Global variable used to store variable value in read sequence */
//uint16_t DataVar = 0;
// TK: made local

/* Virtual address defined by the user: 0xFFFF value is prohibited */
// TK: not used
//extern uint16_t VirtAddVarTab[NumbOfVar];

/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/
static FLASH_Status EE_Format(void);
static uint16_t EE_FindValidPage(uint8_t Operation);
static uint16_t EE_VerifyPageFullWriteVariable(uint16_t VirtAddress, uint16_t Data);
static uint16_t EE_PageTransfer(uint16_t VirtAddress, uint16_t Data);

/**
  * @brief  Restore the pages to a known good state in case of page's status
  *   corruption after a power loss.
  * @param  mode currently only mode 0 supported
  * @retval - < 0: error during writing flash
  *         - 0: on success
  */
s32 EEPROM_Init(u32 mode)
{
#if 0
  uint16_t PageStatus0 = 6, PageStatus1 = 6;
  uint16_t VarIdx = 0;
  uint16_t EepromStatus = 0;
  s32 ReadStatus = 0;
  int16_t x = -1;
  uint16_t  FlashStatus;

  if( mode > 0 )
    return -1; // currently only mode 0 supported

  /* Unlock the Flash Program Erase controller */
  FLASH_Unlock();

  /* Get Page0 status */
  PageStatus0 = (*(__IO uint16_t*)PAGE0_BASE_ADDRESS);
  /* Get Page1 status */
  PageStatus1 = (*(__IO uint16_t*)PAGE1_BASE_ADDRESS);

  /* Check for invalid header states and repair if necessary */
  switch (PageStatus0)
  {
    case ERASED:
      if (PageStatus1 == VALID_PAGE) /* Page0 erased, Page1 valid */
      {
        /* Erase Page0 */
        FlashStatus = FLASH_ErasePage(PAGE0_BASE_ADDRESS);
        /* If erase operation was failed, a Flash error code is returned */
        if (FlashStatus != FLASH_COMPLETE)
        {
          return -2; // FlashStatus;
        }
      }
      else if (PageStatus1 == RECEIVE_DATA) /* Page0 erased, Page1 receive */
      {
        /* Erase Page0 */
        FlashStatus = FLASH_ErasePage(PAGE0_BASE_ADDRESS);
        /* If erase operation was failed, a Flash error code is returned */
        if (FlashStatus != FLASH_COMPLETE)
        {
          return -2; // FlashStatus;
        }
        /* Mark Page1 as valid */
        FlashStatus = FLASH_ProgramHalfWord(PAGE1_BASE_ADDRESS, VALID_PAGE);
        /* If program operation was failed, a Flash error code is returned */
        if (FlashStatus != FLASH_COMPLETE)
        {
          return -2; // FlashStatus;
        }
      }
      else /* First EEPROM access (Page0&1 are erased) or invalid state -> format EEPROM */
      {
        /* Erase both Page0 and Page1 and set Page0 as valid page */
        FlashStatus = EE_Format();
        /* If erase/program operation was failed, a Flash error code is returned */
        if (FlashStatus != FLASH_COMPLETE)
        {
          return -2; // FlashStatus;
        }
      }
      break;

    case RECEIVE_DATA:
      if (PageStatus1 == VALID_PAGE) /* Page0 receive, Page1 valid */
      {
        /* Transfer data from Page1 to Page0 */
        for (VarIdx = 0; VarIdx < EEPROM_EMULATED_SIZE; VarIdx++)
        {
          if (( *(__IO uint16_t*)(PAGE0_BASE_ADDRESS + 6)) == VarIdx)
          {
            x = VarIdx;
          }
          if (VarIdx != x)
          {
            /* Read the last variables' updates */
            ReadStatus = EEPROM_Read(VarIdx);
            /* In case variable corresponding to the virtual address was found */
            if (ReadStatus >= 0)
            {
              /* Transfer the variable to the Page0 */
              EepromStatus = EE_VerifyPageFullWriteVariable(VarIdx, ReadStatus);
              /* If program operation was failed, a Flash error code is returned */
              if (EepromStatus != FLASH_COMPLETE)
              {
		return -2; // FlashStatus;
              }
            }
          }
        }
        /* Mark Page0 as valid */
        FlashStatus = FLASH_ProgramHalfWord(PAGE0_BASE_ADDRESS, VALID_PAGE);
        /* If program operation was failed, a Flash error code is returned */
        if (FlashStatus != FLASH_COMPLETE)
        {
	  return -2; // FlashStatus;
        }
        /* Erase Page1 */
        FlashStatus = FLASH_ErasePage(PAGE1_BASE_ADDRESS);
        /* If erase operation was failed, a Flash error code is returned */
        if (FlashStatus != FLASH_COMPLETE)
        {
	  return -2; // FlashStatus;
        }
      }
      else if (PageStatus1 == ERASED) /* Page0 receive, Page1 erased */
      {
        /* Erase Page1 */
        FlashStatus = FLASH_ErasePage(PAGE1_BASE_ADDRESS);
        /* If erase operation was failed, a Flash error code is returned */
        if (FlashStatus != FLASH_COMPLETE)
        {
	  return -2; // FlashStatus;
        }
        /* Mark Page0 as valid */
        FlashStatus = FLASH_ProgramHalfWord(PAGE0_BASE_ADDRESS, VALID_PAGE);
        /* If program operation was failed, a Flash error code is returned */
        if (FlashStatus != FLASH_COMPLETE)
        {
	  return -2; // FlashStatus;
        }
      }
      else /* Invalid state -> format eeprom */
      {
        /* Erase both Page0 and Page1 and set Page0 as valid page */
        FlashStatus = EE_Format();
        /* If erase/program operation was failed, a Flash error code is returned */
        if (FlashStatus != FLASH_COMPLETE)
        {
	  return -2; // FlashStatus;
        }
      }
      break;

    case VALID_PAGE:
      if (PageStatus1 == VALID_PAGE) /* Invalid state -> format eeprom */
      {
        /* Erase both Page0 and Page1 and set Page0 as valid page */
        FlashStatus = EE_Format();
        /* If erase/program operation was failed, a Flash error code is returned */
        if (FlashStatus != FLASH_COMPLETE)
        {
	  return -2; // FlashStatus;
        }
      }
      else if (PageStatus1 == ERASED) /* Page0 valid, Page1 erased */
      {
        /* Erase Page1 */
        FlashStatus = FLASH_ErasePage(PAGE1_BASE_ADDRESS);
        /* If erase operation was failed, a Flash error code is returned */
        if (FlashStatus != FLASH_COMPLETE)
        {
	  return -2; // FlashStatus;
        }
      }
      else /* Page0 valid, Page1 receive */
      {
        /* Transfer data from Page0 to Page1 */
        for (VarIdx = 0; VarIdx < EEPROM_EMULATED_SIZE; VarIdx++)
        {
          if ((*(__IO uint16_t*)(PAGE1_BASE_ADDRESS + 6)) == VarIdx)
          {
            x = VarIdx;
          }
          if (VarIdx != x)
          {
            /* Read the last variables' updates */
            ReadStatus = EEPROM_Read(VarIdx);
            /* In case variable corresponding to the virtual address was found */
            if (ReadStatus >= 0)
            {
              /* Transfer the variable to the Page1 */
              EepromStatus = EE_VerifyPageFullWriteVariable(VarIdx, ReadStatus);
              /* If program operation was failed, a Flash error code is returned */
              if (EepromStatus != FLASH_COMPLETE)
              {
		return -2; // FlashStatus;
              }
            }
          }
        }
        /* Mark Page1 as valid */
        FlashStatus = FLASH_ProgramHalfWord(PAGE1_BASE_ADDRESS, VALID_PAGE);
        /* If program operation was failed, a Flash error code is returned */
        if (FlashStatus != FLASH_COMPLETE)
        {
	  return -2; // FlashStatus;
        }
        /* Erase Page0 */
        FlashStatus = FLASH_ErasePage(PAGE0_BASE_ADDRESS);
        /* If erase operation was failed, a Flash error code is returned */
        if (FlashStatus != FLASH_COMPLETE)
        {
	  return -2; // FlashStatus;
        }
      }
      break;

    default:  /* Any other state -> format eeprom */
      /* Erase both Page0 and Page1 and set Page0 as valid page */
      FlashStatus = EE_Format();
      /* If erase/program operation was failed, a Flash error code is returned */
      if (FlashStatus != FLASH_COMPLETE)
      {
	return -2; // FlashStatus;
      }
      break;
  }

  /* Lock the Flash Program Erase controller */
  FLASH_Lock();
#endif

  return 0; // no error
}

/**
  * @brief  Returns the last stored variable data, if found, which correspond to
  *   the passed virtual address
  * @param  VirtAddress: Variable virtual address
  * @retval Success or error status:
  *           - >= 0: the 16bit variable if it has been found
  *           - -1: if the variable was not found (not programmed yet)
  *           - -2: if no valid page was found.
  */
s32 EEPROM_Read(u16 VirtAddress)
{
#if 0
  uint16_t ValidPage = PAGE0;
  uint16_t AddressValue = 0x5555;
  s32 ReadStatus = -1;
  uint32_t Address = 0x08010000, PageStartAddress = 0x08010000;
  uint16_t Data = 0;

  /* Get active Page for read operation */
  ValidPage = EE_FindValidPage(READ_FROM_VALID_PAGE);

  /* Check if there is no valid page */
  if (ValidPage == NO_VALID_PAGE)
  {
    return -2; // no valid page
  }

  /* Get the valid Page start Address */
  PageStartAddress = (uint32_t)(EEPROM_START_ADDRESS + (uint32_t)(ValidPage * PAGE_SIZE));

  /* Get the valid Page end Address */
  Address = (uint32_t)((EEPROM_START_ADDRESS - 2) + (uint32_t)((1 + ValidPage) * PAGE_SIZE));

  /* Check each active page address starting from end */
  while (Address > (PageStartAddress + 2))
  {
    /* Get the current location content to be compared with virtual address */
    AddressValue = (*(__IO uint16_t*)Address);

    /* Compare the read address with the virtual address */
    if (AddressValue == VirtAddress)
    {
      /* Get content of Address-2 which is variable value */
      Data = (*(__IO uint16_t*)(Address - 2));

      /* In case variable value is read, reset ReadStatus flag */
      ReadStatus = 0;

      break;
    }
    else
    {
      /* Next address location */
      Address = Address - 4;
    }
  }

  if( ReadStatus < 0 )
    return ReadStatus;

  return Data; // return value of variable
#else
  return 0;
#endif
}

/**
  * @brief  Writes/upadtes variable data in EEPROM.
  * @param  VirtAddress: Variable virtual address
  * @param  Data: 16 bit data to be written
  * @retval Success or error status:
  *           - 0: on success
  *           - -1: if valid page is full
  *           - -2: if no valid page was found
  *           - -3: on write flash error
  */
s32 EEPROM_Write(u16 VirtAddress, u16 Data)
{
#if 0
  uint16_t Status = 0;
  s32 ReadStatus = 0;

  // TK: if value already programmed, don't do this again
  if( ((ReadStatus=EEPROM_Read(VirtAddress)) >= 0) && ReadStatus == Data )
    return 0; // programming not required

  /* Unlock the Flash Program Erase controller */
  FLASH_Unlock();

  /* Write the variable virtual address and value in the EEPROM */
  Status = EE_VerifyPageFullWriteVariable(VirtAddress, Data);

  /* In case the EEPROM active page is full */
  if (Status == PAGE_FULL)
  {
    /* Perform Page transfer */
    Status = EE_PageTransfer(VirtAddress, Data);
  }

  /* Lock the Flash Program Erase controller */
  FLASH_Lock();

  /* Return last operation status */
  switch( Status ) {
    case FLASH_COMPLETE: return 0;
    case PAGE_FULL: return -1;
    case NO_VALID_PAGE: return -2;
  }
#endif

  return -3; // flash write error
}

/**
  * @brief  Erases PAGE0 and PAGE1 and writes VALID_PAGE header to PAGE0
  * @param  None
  * @retval Status of the last operation (Flash write or erase) done during
  *         EEPROM formating
  */
static FLASH_Status EE_Format(void)
{
#if 0
  FLASH_Status FlashStatus = FLASH_COMPLETE;

  /* Erase Page0 */
  FlashStatus = FLASH_ErasePage(PAGE0_BASE_ADDRESS);

  /* If erase operation was failed, a Flash error code is returned */
  if (FlashStatus != FLASH_COMPLETE)
  {
    return FlashStatus;
  }

  /* Set Page0 as valid page: Write VALID_PAGE at Page0 base address */
  FlashStatus = FLASH_ProgramHalfWord(PAGE0_BASE_ADDRESS, VALID_PAGE);

  /* If program operation was failed, a Flash error code is returned */
  if (FlashStatus != FLASH_COMPLETE)
  {
    return FlashStatus;
  }

  /* Erase Page1 */
  FlashStatus = FLASH_ErasePage(PAGE1_BASE_ADDRESS);

  /* Return Page1 erase operation status */
  return FlashStatus;
#else
  return 0;
#endif
}

/**
  * @brief  Find valid Page for write or read operation
  * @param  Operation: operation to achieve on the valid page.
  *   This parameter can be one of the following values:
  *     @arg READ_FROM_VALID_PAGE: read operation from valid page
  *     @arg WRITE_IN_VALID_PAGE: write operation from valid page
  * @retval Valid page number (PAGE0 or PAGE1) or NO_VALID_PAGE in case
  *   of no valid page was found
  */
static uint16_t EE_FindValidPage(uint8_t Operation)
{
#if 0
  uint16_t PageStatus0 = 6, PageStatus1 = 6;

  /* Get Page0 actual status */
  PageStatus0 = (*(__IO uint16_t*)PAGE0_BASE_ADDRESS);

  /* Get Page1 actual status */
  PageStatus1 = (*(__IO uint16_t*)PAGE1_BASE_ADDRESS);

  /* Write or read operation */
  switch (Operation)
  {
    case WRITE_IN_VALID_PAGE:   /* ---- Write operation ---- */
      if (PageStatus1 == VALID_PAGE)
      {
        /* Page0 receiving data */
        if (PageStatus0 == RECEIVE_DATA)
        {
          return PAGE0;         /* Page0 valid */
        }
        else
        {
          return PAGE1;         /* Page1 valid */
        }
      }
      else if (PageStatus0 == VALID_PAGE)
      {
        /* Page1 receiving data */
        if (PageStatus1 == RECEIVE_DATA)
        {
          return PAGE1;         /* Page1 valid */
        }
        else
        {
          return PAGE0;         /* Page0 valid */
        }
      }
      else
      {
        return NO_VALID_PAGE;   /* No valid Page */
      }

    case READ_FROM_VALID_PAGE:  /* ---- Read operation ---- */
      if (PageStatus0 == VALID_PAGE)
      {
        return PAGE0;           /* Page0 valid */
      }
      else if (PageStatus1 == VALID_PAGE)
      {
        return PAGE1;           /* Page1 valid */
      }
      else
      {
        return NO_VALID_PAGE ;  /* No valid Page */
      }

    default:
      return PAGE0;             /* Page0 valid */
  }
#endif
}

/**
  * @brief  Verify if active page is full and Writes variable in EEPROM.
  * @param  VirtAddress: 16 bit virtual address of the variable
  * @param  Data: 16 bit data to be written as variable value
  * @retval Success or error status:
  *           - FLASH_COMPLETE: on success
  *           - PAGE_FULL: if valid page is full
  *           - NO_VALID_PAGE: if no valid page was found
  *           - Flash error code: on write Flash error
  */
static uint16_t EE_VerifyPageFullWriteVariable(uint16_t VirtAddress, uint16_t Data)
{
#if 0
  FLASH_Status FlashStatus = FLASH_COMPLETE;
  uint16_t ValidPage = PAGE0;
  uint32_t Address = 0x08010000, PageEndAddress = 0x080107FF;

  /* Get valid Page for write operation */
  ValidPage = EE_FindValidPage(WRITE_IN_VALID_PAGE);

  /* Check if there is no valid page */
  if (ValidPage == NO_VALID_PAGE)
  {
    return  NO_VALID_PAGE;
  }

  /* Get the valid Page start Address */
  Address = (uint32_t)(EEPROM_START_ADDRESS + (uint32_t)(ValidPage * PAGE_SIZE));

  /* Get the valid Page end Address */
  PageEndAddress = (uint32_t)((EEPROM_START_ADDRESS - 2) + (uint32_t)((1 + ValidPage) * PAGE_SIZE));

  /* Check each active page address starting from begining */
  while (Address < PageEndAddress)
  {
    /* Verify if Address and Address+2 contents are 0xFFFFFFFF */
    if ((*(__IO uint32_t*)Address) == 0xFFFFFFFF)
    {
      /* Set variable data */
      FlashStatus = FLASH_ProgramHalfWord(Address, Data);
      /* If program operation was failed, a Flash error code is returned */
      if (FlashStatus != FLASH_COMPLETE)
      {
        return FlashStatus;
      }
      /* Set variable virtual address */
      FlashStatus = FLASH_ProgramHalfWord(Address + 2, VirtAddress);
      /* Return program operation status */
      return FlashStatus;
    }
    else
    {
      /* Next address location */
      Address = Address + 4;
    }
  }

  /* Return PAGE_FULL in case the valid page is full */
  return PAGE_FULL;
#endif
}

/**
  * @brief  Transfers last updated variables data from the full Page to
  *   an empty one.
  * @param  VirtAddress: 16 bit virtual address of the variable
  * @param  Data: 16 bit data to be written as variable value
  * @retval Success or error status:
  *           - FLASH_COMPLETE: on success
  *           - PAGE_FULL: if valid page is full
  *           - NO_VALID_PAGE: if no valid page was found
  *           - Flash error code: on write Flash error
  */
static uint16_t EE_PageTransfer(uint16_t VirtAddress, uint16_t Data)
{
#if 0
  FLASH_Status FlashStatus = FLASH_COMPLETE;
  uint32_t NewPageAddress = 0x080103FF, OldPageAddress = 0x08010000;
  uint16_t ValidPage = PAGE0, VarIdx = 0;
  uint16_t EepromStatus = 0;
  s32 ReadStatus = 0;

  /* Get active Page for read operation */
  ValidPage = EE_FindValidPage(READ_FROM_VALID_PAGE);

  if (ValidPage == PAGE1)       /* Page1 valid */
  {
    /* New page address where variable will be moved to */
    NewPageAddress = PAGE0_BASE_ADDRESS;

    /* Old page address where variable will be taken from */
    OldPageAddress = PAGE1_BASE_ADDRESS;
  }
  else if (ValidPage == PAGE0)  /* Page0 valid */
  {
    /* New page address where variable will be moved to */
    NewPageAddress = PAGE1_BASE_ADDRESS;

    /* Old page address where variable will be taken from */
    OldPageAddress = PAGE0_BASE_ADDRESS;
  }
  else
  {
    return NO_VALID_PAGE;       /* No valid Page */
  }

  /* Set the new Page status to RECEIVE_DATA status */
  FlashStatus = FLASH_ProgramHalfWord(NewPageAddress, RECEIVE_DATA);
  /* If program operation was failed, a Flash error code is returned */
  if (FlashStatus != FLASH_COMPLETE)
  {
    return FlashStatus;
  }

  /* Write the variable passed as parameter in the new active page */
  EepromStatus = EE_VerifyPageFullWriteVariable(VirtAddress, Data);
  /* If program operation was failed, a Flash error code is returned */
  if (EepromStatus != FLASH_COMPLETE)
  {
    return EepromStatus;
  }

  /* Transfer process: transfer variables from old to the new active page */
  for (VarIdx = 0; VarIdx < EEPROM_EMULATED_SIZE; VarIdx++)
  {
    if (VarIdx != VirtAddress)  /* Check each variable except the one passed as parameter */
    {
      /* Read the other last variable updates */
      ReadStatus = EEPROM_Read(VarIdx);
      /* In case variable corresponding to the virtual address was found */
      if (ReadStatus >= 0)
      {
        /* Transfer the variable to the new active page */
        EepromStatus = EE_VerifyPageFullWriteVariable(VarIdx, ReadStatus);
        /* If program operation was failed, a Flash error code is returned */
        if (EepromStatus != FLASH_COMPLETE)
        {
          return EepromStatus;
        }
      }
    }
  }

  /* Erase the old Page: Set old Page status to ERASED status */
  FlashStatus = FLASH_ErasePage(OldPageAddress);
  /* If erase operation was failed, a Flash error code is returned */
  if (FlashStatus != FLASH_COMPLETE)
  {
    return FlashStatus;
  }

  /* Set new Page status to VALID_PAGE status */
  FlashStatus = FLASH_ProgramHalfWord(NewPageAddress, VALID_PAGE);
  /* If program operation was failed, a Flash error code is returned */
  if (FlashStatus != FLASH_COMPLETE)
  {
    return FlashStatus;
  }

  /* Return last operation flash status */
  return FlashStatus;
#endif
}



/////////////////////////////////////////////////////////////////////////////
//! Sends the EEPROM content and optionally the whole flash page content
//! to the MIOS Terminal
//! \param[in] mode following modes are provided:
//!    <UL>
//!      <LI>0: send EEPROM content
//!      <LI>1: send content of flash pages
//!      <LI>2: send EEPROM content and flash pages
//!    </UL>
//! \return < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 EEPROM_SendDebugMessage(u32 mode)
{
  s32 status = 0;

  if( mode >= 1 ) {
    status |= MIOS32_MIDI_SendDebugMessage("Page 0 (0x%08x):\n", PAGE0_BASE_ADDRESS);
    status |= MIOS32_MIDI_SendDebugHexDump((u8 *)PAGE0_BASE_ADDRESS, PAGE_SIZE);
    status |= MIOS32_MIDI_SendDebugMessage("Page 1 (0x%08x):\n", PAGE1_BASE_ADDRESS);
    status |= MIOS32_MIDI_SendDebugHexDump((u8 *)PAGE1_BASE_ADDRESS, PAGE_SIZE);
  }

  if( mode != 1 ) {
    u8 buffer[2*EEPROM_EMULATED_SIZE];
    int i;

    for(i=0; i<EEPROM_EMULATED_SIZE; ++i) {
      u16 value = EEPROM_Read(i);
      buffer[i*2+0] = value & 0xff;
      buffer[i*2+1] = (value >> 8) & 0xff;
    }
      
    status |= MIOS32_MIDI_SendDebugMessage("EEPROM Content:\n");
    status |= MIOS32_MIDI_SendDebugHexDump(buffer, 2*EEPROM_EMULATED_SIZE);
  }

  return status;
}

/**
  * @}
  */ 

/******************* (C) COPYRIGHT 2009 STMicroelectronics *****END OF FILE****/
