// $Id$
//! \defgroup MSD
//!
//! USB Mass Storage Device Driver
//!
//! Mainly based on STM32 example code and adapted to MIOS32.
//!
//! Application examples:
//! <UL CLASS=CL>
//!   <LI>$MIOS32_PATH/apps/misc/usb_mass_storage_device
//!   <LI>$MIOS32_PATH/apps/sequencers/midibox_seq_v4/mios32/tasks.c
//! </UL>
//!
//! \{
/* ==========================================================================
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
#include <usb_lib.h>

#include <string.h>

#include "msd.h"
#include "msd_desc.h"
#include "msd_bot.h"
#include "msd_memory.h"


/////////////////////////////////////////////////////////////////////////////
// Local definitions
/////////////////////////////////////////////////////////////////////////////

/* MASS Storage Requests */
#define GET_MAX_LUN                0xFE
#define MASS_STORAGE_RESET         0xFF
#define LUN_DATA_LENGTH            1


/* ISTR events */
/* IMR_MSK */
/* mask defining which events has to be handled */
/* by the device application software */
//#define MSD_IMR_MSK (CNTR_CTRM  | CNTR_RESETM)
#define MSD_IMR_MSK (CNTR_RESETM)


/////////////////////////////////////////////////////////////////////////////
// Local prototypes
/////////////////////////////////////////////////////////////////////////////

static void MSD_MASS_Reset(void);
static void MSD_Mass_Storage_SetConfiguration(void);
static void MSD_Mass_Storage_ClearFeature(void);
static void MSD_Mass_Storage_SetDeviceAddress (void);
static void MSD_MASS_Status_In (void);
static void MSD_MASS_Status_Out (void);
static RESULT MSD_MASS_Data_Setup(uint8_t);
static RESULT MSD_MASS_NoData_Setup(uint8_t);
static RESULT MSD_MASS_Get_Interface_Setting(uint8_t Interface, uint8_t AlternateSetting);
static uint8_t *MSD_MASS_GetDeviceDescriptor(uint16_t );
static uint8_t *MSD_MASS_GetConfigDescriptor(uint16_t);
static uint8_t *MSD_MASS_GetStringDescriptor(uint16_t);
static uint8_t *Get_Max_Lun(uint16_t Length);


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

static const DEVICE My_Device_Table = {
  MSD_EP_NUM,
  1
};

static const DEVICE_PROP My_Device_Property = {
  0, // Init hook not used, done by MIOS32_USB module!
  MSD_MASS_Reset,
  MSD_MASS_Status_In,
  MSD_MASS_Status_Out,
  MSD_MASS_Data_Setup,
  MSD_MASS_NoData_Setup,
  MSD_MASS_Get_Interface_Setting,
  MSD_MASS_GetDeviceDescriptor,
  MSD_MASS_GetConfigDescriptor,
  MSD_MASS_GetStringDescriptor,
  0,
  0x40 /*MAX PACKET SIZE*/
};

static const USER_STANDARD_REQUESTS My_User_Standard_Requests = {
  NOP_Process,
  MSD_Mass_Storage_SetConfiguration,
  NOP_Process,
  NOP_Process,
  NOP_Process,
  MSD_Mass_Storage_ClearFeature,
  NOP_Process,
  NOP_Process,
  MSD_Mass_Storage_SetDeviceAddress
};

static ONE_DESCRIPTOR Device_Descriptor = {
  (uint8_t*)MSD_MASS_DeviceDescriptor,
  MSD_MASS_SIZ_DEVICE_DESC
};

static ONE_DESCRIPTOR Config_Descriptor = {
  (uint8_t*)MSD_MASS_ConfigDescriptor,
  MSD_MASS_SIZ_CONFIG_DESC
};

static ONE_DESCRIPTOR String_Descriptor[5] = {
  {(uint8_t*)MSD_MASS_StringLangID, MSD_MASS_SIZ_STRING_LANGID},
  {(uint8_t*)MSD_MASS_StringVendor, MSD_MASS_SIZ_STRING_VENDOR},
  {(uint8_t*)MSD_MASS_StringProduct, MSD_MASS_SIZ_STRING_PRODUCT},
  {(uint8_t*)MSD_MASS_StringSerial, MSD_MASS_SIZ_STRING_SERIAL},
  {(uint8_t*)MSD_MASS_StringInterface, MSD_MASS_SIZ_STRING_INTERFACE},
};

static u8 lun_available;


/////////////////////////////////////////////////////////////////////////////
//! Initializes the USB Device Driver for a Mass Storage Device
//!
//! Should be called during runtime once a SD Card has been connected.<BR>
//! It is possible to switch back to the original device driver provided
//! by MIOS32 (e.g. MIOS32_USB_MIDI) by calling MIOS32_USB_Init(1)
//!
//! \param[in] mode currently only mode 0 supported
//! \return < 0 if initialisation failed
/////////////////////////////////////////////////////////////////////////////
s32 MSD_Init(u32 mode)
{
  /* Update the serial number string descriptor with the data from the unique
  ID*/
  u8 serial_number_str[40];
  int i, len;
  MIOS32_SYS_SerialNumberGet((char *)serial_number_str);
  for(i=0, len=0; serial_number_str[i] != '\0' && len<25; ++i) {
    MSD_MASS_StringSerial[len++] = serial_number_str[i];
    MSD_MASS_StringSerial[len++] = 0;
  }

  lun_available = 0;

  // all LUNs available after USB init
  for(i=0; i<MSD_NUM_LUN; ++i)
    MSD_LUN_AvailableSet(i, 1);

  msd_memory_rd_led_ctr = 0;
  msd_memory_wr_led_ctr = 0;

  MIOS32_IRQ_Disable();

  // clear all USB interrupt requests
  _SetCNTR(0); // Interrupt Mask
  _SetISTR(0); // clear pending requests

  // switch to MSD driver hooks
  memcpy(&Device_Table, (DEVICE *)&My_Device_Table, sizeof(Device_Table));
  pProperty = (DEVICE_PROP *)&My_Device_Property;
  pUser_Standard_Requests = (USER_STANDARD_REQUESTS *)&My_User_Standard_Requests;

  // change endpoints
  pEpInt_IN[0] = MSD_Mass_Storage_In;
  pEpInt_OUT[1] = MSD_Mass_Storage_Out;

  // force re-enumeration w/o overwriting MIOS32 hooks
  MIOS32_USB_Init(2);

  // clear pending interrupts (again)
  _SetISTR(0);

  // set interrupts mask
  // _SetCNTR(CNTR_CTRM | CNTR_RESETM);
  _SetCNTR(CNTR_RESETM); // CTRM handled by MSD_Periodic_mS()

  MIOS32_IRQ_Enable();

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! Should be called periodically each millisecond so long this driver is
//! active (and only then!) to handle USBtransfers.
//!
//! Take care that no other task accesses SD Card while this function is
//! processed!
//!
//! Ensure that this function isn't called when a MIOS32 USB driver like
//! MIOS32_USB_MIDI is running!
//!
//! \return < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 MSD_Periodic_mS(void)
{
  // handle LED counters
  if( msd_memory_rd_led_ctr < 65535 )
    ++msd_memory_rd_led_ctr;

  if( msd_memory_wr_led_ctr < 65535 )
    ++msd_memory_wr_led_ctr;

  // call endpoint handler of STM32 USB driver
  CTR_LP();

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! This function returns the connection status of the USB MIDI interface
//! \return 1: interface available
//! \return 0: interface not available
/////////////////////////////////////////////////////////////////////////////
s32 MSD_CheckAvailable(void)
{
  return pInformation->Current_Configuration ? 1 : 0;
}



/////////////////////////////////////////////////////////////////////////////
//! The logical unit is available whenever MSD_Init() is called, or the USB
//! cable has been reconnected.
//!
//! It will be disabled when the host unmounts the file system (like if the
//! SD Card would be removed.
//!
//! When this happens, the application can either call MIOS32_USB_Init(1)
//! again, e.g. to switch to USB MIDI, or it can make the LUN available
//! again by calling MSD_LUN_AvailableSet(0, 1)
//! \param[in] lun Logical Unit number (0)
//! \param[in] available 0 or 1
//! \return < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 MSD_LUN_AvailableSet(u8 lun, u8 available)
{
  if( lun >= MSD_NUM_LUN )
    return -1;

  if( available )
    lun_available |= (1 << lun);
  else
    lun_available &= ~(1 << lun);

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! \return 1 if device is mounted by host
//! \return 0 if device is not mounted by host
/////////////////////////////////////////////////////////////////////////////
s32 MSD_LUN_AvailableGet(u8 lun)
{
  if( lun >= MSD_NUM_LUN )
    return 0;

  return (lun_available & (1 << lun)) ? 1 : 0;
}


/////////////////////////////////////////////////////////////////////////////
//! Returns status of "Read LED"
//! \param[in] lag_ms The "lag" of the LED (e.g. 250 for 250 mS)
//! \return 1: a read operation has happened in the last x mS (x == time defined by "lag")
//! \return 0: no read operation has happened in the last x mS (x == time defined by "lag")
/////////////////////////////////////////////////////////////////////////////
s32 MSD_RdLEDGet(u16 lag_ms)
{
  return (msd_memory_rd_led_ctr < lag_ms) ? 1 : 0;
}

/////////////////////////////////////////////////////////////////////////////
//! Returns status of "Write LED"
//! \param[in] lag_ms The "lag" of the LED (e.g. 250 for 250 mS)
//! \return 1: a write operation has happened in the last x mS (x == time defined by "lag")
//! \return 0: no write operation has happened in the last x mS (x == time defined by "lag")
/////////////////////////////////////////////////////////////////////////////
s32 MSD_WrLEDGet(u16 lag_ms)
{
  return (msd_memory_wr_led_ctr < lag_ms) ? 1 : 0;
}



/*******************************************************************************
* Function Name  : MSD_MASS_Reset
* Description    : Mass Storage reset routine.
* Input          : None.
* Output         : None.
* Return         : None.
*******************************************************************************/
static void MSD_MASS_Reset()
{
  /* Set the device as not configured */
  pInformation->Current_Configuration = 0;

  /* Current Feature initialization */
  pInformation->Current_Feature = MSD_MASS_ConfigDescriptor[7];

  SetBTABLE(MSD_BTABLE_ADDRESS);

  /* Initialize Endpoint 0 */
  SetEPType(ENDP0, EP_CONTROL);
  SetEPTxStatus(ENDP0, EP_TX_NAK);
  SetEPRxAddr(ENDP0, MSD_ENDP0_RXADDR);
  SetEPRxCount(ENDP0, pProperty->MaxPacketSize);
  SetEPTxAddr(ENDP0, MSD_ENDP0_TXADDR);
  Clear_Status_Out(ENDP0);
  SetEPRxValid(ENDP0);

  /* Initialize Endpoint 1 */
  SetEPType(ENDP1, EP_BULK);
  SetEPTxAddr(ENDP1, MSD_ENDP1_TXADDR);
  SetEPTxStatus(ENDP1, EP_TX_NAK);
  SetEPRxStatus(ENDP1, EP_RX_DIS);

  /* Initialize Endpoint 2 */
  SetEPType(ENDP2, EP_BULK);
  SetEPRxAddr(ENDP2, MSD_ENDP2_RXADDR);
  SetEPRxCount(ENDP2, pProperty->MaxPacketSize);
  SetEPRxStatus(ENDP2, EP_RX_VALID);
  SetEPTxStatus(ENDP2, EP_TX_DIS);


  SetEPRxCount(ENDP0, pProperty->MaxPacketSize);
  SetEPRxValid(ENDP0);

  /* Set the device to response on default address */
  SetDeviceAddress(0);

  MSD_CBW.dSignature = BOT_CBW_SIGNATURE;
  MSD_Bot_State = BOT_IDLE;
}

/*******************************************************************************
* Function Name  : MSD_Mass_Storage_SetConfiguration
* Description    : Handle the SetConfiguration request.
* Input          : None.
* Output         : None.
* Return         : None.
*******************************************************************************/
static void MSD_Mass_Storage_SetConfiguration(void)
{
  if (pInformation->Current_Configuration != 0)
  {
    ClearDTOG_TX(ENDP1);
    ClearDTOG_RX(ENDP2);
    MSD_Bot_State = BOT_IDLE; /* set the Bot state machine to the IDLE state */

    // all LUNs available after USB (re-)connection
    int i;
    for(i=0; i<MSD_NUM_LUN; ++i)
      MSD_LUN_AvailableSet(i, 1);
  }
}

/*******************************************************************************
* Function Name  : MSD_Mass_Storage_ClearFeature
* Description    : Handle the ClearFeature request.
* Input          : None.
* Output         : None.
* Return         : None.
*******************************************************************************/
static void MSD_Mass_Storage_ClearFeature(void)
{
  /* when the host send a CBW with invalid signature or invalid length the two
     Endpoints (IN & OUT) shall stall until receiving a Mass Storage Reset     */
  if (MSD_CBW.dSignature != BOT_CBW_SIGNATURE)
    MSD_Bot_Abort(BOTH_DIR);
}

/*******************************************************************************
* Function Name  : MSD_Mass_Storage_SetConfiguration.
* Description    : Udpade the device state to addressed.
* Input          : None.
* Output         : None.
* Return         : None.
*******************************************************************************/
static void MSD_Mass_Storage_SetDeviceAddress (void)
{
}

/*******************************************************************************
* Function Name  : MSD_MASS_Status_In
* Description    : Mass Storage Status IN routine.
* Input          : None.
* Output         : None.
* Return         : None.
*******************************************************************************/
static void MSD_MASS_Status_In(void)
{
  return;
}

/*******************************************************************************
* Function Name  : MSD_MASS_Status_Out
* Description    : Mass Storage Status OUT routine.
* Input          : None.
* Output         : None.
* Return         : None.
*******************************************************************************/
static void MSD_MASS_Status_Out(void)
{
  return;
}

/*******************************************************************************
* Function Name  : MSD_MASS_Data_Setup.
* Description    : Handle the data class specific requests..
* Input          : RequestNo.
* Output         : None.
* Return         : RESULT.
*******************************************************************************/
static RESULT MSD_MASS_Data_Setup(uint8_t RequestNo)
{
  uint8_t    *(*CopyRoutine)(uint16_t);

  CopyRoutine = NULL;
  if ((Type_Recipient == (CLASS_REQUEST | INTERFACE_RECIPIENT))
      && (RequestNo == GET_MAX_LUN) && (pInformation->USBwValue == 0)
      && (pInformation->USBwIndex == 0) && (pInformation->USBwLength == 0x01))
  {
    CopyRoutine = Get_Max_Lun;
  }
  else
  {
    return USB_UNSUPPORT;
  }

  if (CopyRoutine == NULL)
  {
    return USB_UNSUPPORT;
  }

  pInformation->Ctrl_Info.CopyData = CopyRoutine;
  pInformation->Ctrl_Info.Usb_wOffset = 0;
  (*CopyRoutine)(0);

  return USB_SUCCESS;

}

/*******************************************************************************
* Function Name  : MSD_MASS_NoData_Setup.
* Description    : Handle the no data class specific requests.
* Input          : RequestNo.
* Output         : None.
* Return         : RESULT.
*******************************************************************************/
static RESULT MSD_MASS_NoData_Setup(uint8_t RequestNo)
{
  if ((Type_Recipient == (CLASS_REQUEST | INTERFACE_RECIPIENT))
      && (RequestNo == MASS_STORAGE_RESET) && (pInformation->USBwValue == 0)
      && (pInformation->USBwIndex == 0) && (pInformation->USBwLength == 0x00))
  {
    /* Initialize Endpoint 1 */
    ClearDTOG_TX(ENDP1);

    /* Initialize Endpoint 2 */
    ClearDTOG_RX(ENDP2);

    /*intialise the CBW signature to enable the clear feature*/
    MSD_CBW.dSignature = BOT_CBW_SIGNATURE;
    MSD_Bot_State = BOT_IDLE;

    return USB_SUCCESS;
  }
  return USB_UNSUPPORT;
}

/*******************************************************************************
* Function Name  : MSD_MASS_Get_Interface_Setting
* Description    : Test the interface and the alternate setting according to the
*                  supported one.
* Input          : uint8_t Interface, uint8_t AlternateSetting.
* Output         : None.
* Return         : RESULT.
*******************************************************************************/
static RESULT MSD_MASS_Get_Interface_Setting(uint8_t Interface, uint8_t AlternateSetting)
{
  if (AlternateSetting > 0)
  {
    return USB_UNSUPPORT;/* in this application we don't have AlternateSetting*/
  }
  else if (Interface > 0)
  {
    return USB_UNSUPPORT;/*in this application we have only 1 interfaces*/
  }
  return USB_SUCCESS;
}

/*******************************************************************************
* Function Name  : MSD_MASS_GetDeviceDescriptor
* Description    : Get the device descriptor.
* Input          : uint16_t Length.
* Output         : None.
* Return         : None.
*******************************************************************************/
static uint8_t *MSD_MASS_GetDeviceDescriptor(uint16_t Length)
{
  return Standard_GetDescriptorData(Length, &Device_Descriptor );
}

/*******************************************************************************
* Function Name  : MSD_MASS_GetConfigDescriptor
* Description    : Get the configuration descriptor.
* Input          : uint16_t Length.
* Output         : None.
* Return         : None.
*******************************************************************************/
static uint8_t *MSD_MASS_GetConfigDescriptor(uint16_t Length)
{
  return Standard_GetDescriptorData(Length, &Config_Descriptor );
}

/*******************************************************************************
* Function Name  : MSD_MASS_GetStringDescriptor
* Description    : Get the string descriptors according to the needed index.
* Input          : uint16_t Length.
* Output         : None.
* Return         : None.
*******************************************************************************/
static uint8_t *MSD_MASS_GetStringDescriptor(uint16_t Length)
{
  uint8_t wValue0 = pInformation->USBwValue0;

  if (wValue0 > 5)
  {
    return NULL;
  }
  else
  {
    return Standard_GetDescriptorData(Length, &String_Descriptor[wValue0]);
  }
}

/*******************************************************************************
* Function Name  : Get_Max_Lun
* Description    : Handle the Get Max Lun request.
* Input          : uint16_t Length.
* Output         : None.
* Return         : None.
*******************************************************************************/
static uint8_t *Get_Max_Lun(uint16_t Length)
{
  static u32 Max_Lun = MSD_NUM_LUN-1;

  if (Length == 0)
  {
    pInformation->Ctrl_Info.Usb_wLength = LUN_DATA_LENGTH;
    return 0;
  }
  else
  {
    // this copy concept requires statically allocated data... grr!
    return((uint8_t*)(&Max_Lun));
  }
}

//! \}
