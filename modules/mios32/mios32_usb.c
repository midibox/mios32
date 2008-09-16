// $Id$
/*
 * USB driver for MIOS32
 * Based on driver included in STM32 USB library
 * Most code copied and modified from Virtual_COM_Port demo
 *
 * ==========================================================================
 *
 *  Copyright (C) 2008 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

/////////////////////////////////////////////////////////////////////////////
// Include files
/////////////////////////////////////////////////////////////////////////////

#include <mios32.h>

// this module can be optionally disabled in a local mios32_config.h file (included from mios32.h)
#if !defined(MIOS32_DONT_USE_USB)

#include <usb_lib.h>


/////////////////////////////////////////////////////////////////////////////
// Local definitions
/////////////////////////////////////////////////////////////////////////////

// number of endpoints
#define EP_NUM              (4)

// buffer table base address
#define BTABLE_ADDRESS      (0x00)

// EP0 rx/tx buffer base address
#define ENDP0_RXADDR        (0x40)
#define ENDP0_TXADDR        (0x80)

// EP1 Rx/Tx buffer base address
#define ENDP1_TXADDR        (0xc0)
#define ENDP1_RXADDR        (0x100)

// ISTR events
// mask defining which events has to be handled by the device application software
#define IMR_MSK (CNTR_CTRM  | CNTR_SOFM  | CNTR_RESETM )


/////////////////////////////////////////////////////////////////////////////
// Local types
/////////////////////////////////////////////////////////////////////////////

typedef enum _RESUME_STATE
{
  RESUME_EXTERNAL,
  RESUME_INTERNAL,
  RESUME_LATER,
  RESUME_WAIT,
  RESUME_START,
  RESUME_ON,
  RESUME_OFF,
  RESUME_ESOF
} RESUME_STATE;

typedef enum _DEVICE_STATE
{
  UNCONNECTED,
  ATTACHED,
  POWERED,
  SUSPENDED,
  ADDRESSED,
  CONFIGURED
} DEVICE_STATE;


/////////////////////////////////////////////////////////////////////////////
// Local prototypes
/////////////////////////////////////////////////////////////////////////////

void MIOS32_USB_EP1_IN_Callback(void);
void MIOS32_USB_EP1_OUT_Callback(void);

void MIOS32_USB_CB_Init(void);
void MIOS32_USB_CB_Reset(void);
void MIOS32_USB_CB_SetConfiguration(void);
void MIOS32_USB_CB_SetDeviceAddress (void);
void MIOS32_USB_CB_Status_In(void);
void MIOS32_USB_CB_Status_Out(void);
RESULT MIOS32_USB_CB_Data_Setup(u8 RequestNo);
RESULT MIOS32_USB_CB_NoData_Setup(u8 RequestNo);
u8 *MIOS32_USB_CB_GetDeviceDescriptor(u16 Length);
u8 *MIOS32_USB_CB_GetConfigDescriptor(u16 Length);
u8 *MIOS32_USB_CB_GetStringDescriptor(u16 Length);
RESULT MIOS32_USB_CB_Get_Interface_Setting(u8 Interface, u8 AlternateSetting);


/////////////////////////////////////////////////////////////////////////////
// USB callback vectors
/////////////////////////////////////////////////////////////////////////////

void (*pEpInt_IN[7])(void) = {
  MIOS32_USB_EP1_IN_Callback,
  NOP_Process,
  NOP_Process,
  NOP_Process,
  NOP_Process,
  NOP_Process,
  NOP_Process
};

void (*pEpInt_OUT[7])(void) = {
  MIOS32_USB_EP1_OUT_Callback,
  NOP_Process,
  NOP_Process,
  NOP_Process,
  NOP_Process,
  NOP_Process,
  NOP_Process
};

DEVICE Device_Table = {
  EP_NUM,
  1
};

DEVICE_PROP Device_Property = {
  MIOS32_USB_CB_Init,
  MIOS32_USB_CB_Reset,
  MIOS32_USB_CB_Status_In,
  MIOS32_USB_CB_Status_Out,
  MIOS32_USB_CB_Data_Setup,
  MIOS32_USB_CB_NoData_Setup,
  MIOS32_USB_CB_Get_Interface_Setting,
  MIOS32_USB_CB_GetDeviceDescriptor,
  MIOS32_USB_CB_GetConfigDescriptor,
  MIOS32_USB_CB_GetStringDescriptor,
  0,
  0x40 /*MAX PACKET SIZE*/
};

USER_STANDARD_REQUESTS User_Standard_Requests = {
  NOP_Process, // MIOS32_USB_CB_GetConfiguration,
  MIOS32_USB_CB_SetConfiguration,
  NOP_Process, // MIOS32_USB_CB_GetInterface,
  NOP_Process, // MIOS32_USB_CB_SetInterface,
  NOP_Process, // MIOS32_USB_CB_GetStatus,
  NOP_Process, // MIOS32_USB_CB_ClearFeature,
  NOP_Process, // MIOS32_USB_CB_SetEndPointFeature,
  NOP_Process, // MIOS32_USB_CB_SetDeviceFeature,
  MIOS32_USB_CB_SetDeviceAddress
};

ONE_DESCRIPTOR Device_Descriptor = {
  (u8*)MIOS32_USB_DESC_DeviceDescriptor,
  MIOS32_USB_DESC_SIZ_DEVICE_DESC
};

ONE_DESCRIPTOR Config_Descriptor = {
  (u8*)MIOS32_USB_DESC_ConfigDescriptor,
  MIOS32_USB_DESC_SIZ_CONFIG_DESC
};

ONE_DESCRIPTOR String_Descriptor[4] = {
  {(u8*)MIOS32_USB_DESC_StringLangID, MIOS32_USB_DESC_SIZ_STRING_LANGID},
  {(u8*)MIOS32_USB_DESC_StringVendor, MIOS32_USB_DESC_SIZ_STRING_VENDOR},
  {(u8*)MIOS32_USB_DESC_StringProduct, MIOS32_USB_DESC_SIZ_STRING_PRODUCT},
  0
};


/////////////////////////////////////////////////////////////////////////////
// Local Variables
/////////////////////////////////////////////////////////////////////////////

// global interrupt status (also used by USB driver)
volatile u16 wIstr;

// USB device status
vu32 bDeviceState = UNCONNECTED;


/////////////////////////////////////////////////////////////////////////////
// Initializes the USB interface
// IN: <mode>: currently only mode 0 supported
// OUT: returns < 0 if initialisation failed (e.g. clock not initialised!)
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_USB_Init(u32 mode)
{
  GPIO_InitTypeDef GPIO_InitStructure;

  // currently only mode 0 supported
  if( mode != 0 )
    return -1; // unsupported mode

  // configure USB disconnect pin
  // STM32 Primer: pin B12
  GPIO_StructInit(&GPIO_InitStructure);
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12;
  GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IPD;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOB, &GPIO_InitStructure);

  // remaining initialisation done in STM32 USB driver
  USB_Init();

  return 0;
}

/////////////////////////////////////////////////////////////////////////////
// This handler should be called from a low priority task to react on
// USB interrupt notifications
// OUT: returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_USB_LowPrio_Handler(void)
{
  // MEMO: could also be called from USB_LP_CAN_RX0_IRQHandler
  // if IRQ vector configured for USB

  wIstr = _GetISTR();

  if( wIstr & ISTR_RESET & wInterrupt_Mask ) {
    _SetISTR((u16)CLR_RESET);
    Device_Property.Reset();
  }

  if( wIstr & ISTR_SOF & wInterrupt_Mask ) {
    _SetISTR((u16)CLR_SOF);
  }

  if( wIstr & ISTR_CTR & wInterrupt_Mask ) {
    // servicing of the endpoint correct transfer interrupt
    // clear of the CTR flag into the sub
    CTR_LP();
  }

  return 0;
}


/////////////////////////////////////////////////////////////////////////////
// Called by STM32 USB driver to check for IN streams
/////////////////////////////////////////////////////////////////////////////
void MIOS32_USB_EP1_IN_Callback(void)
{
}

/////////////////////////////////////////////////////////////////////////////
// Called by STM32 USB driver to check for OUT streams
/////////////////////////////////////////////////////////////////////////////
void MIOS32_USB_EP1_OUT_Callback(void)
{
  SetEPRxValid(ENDP1);
}


/////////////////////////////////////////////////////////////////////////////
// Hooks of STM32 USB library
/////////////////////////////////////////////////////////////////////////////

// init routine
void MIOS32_USB_CB_Init(void)
{
  u16 wRegVal;

  pInformation->Current_Configuration = 0;

  // Connect the device

  // CNTR_PWDN = 0
  wRegVal = CNTR_FRES;
  _SetCNTR(wRegVal);

  // CNTR_FRES = 0
  wInterrupt_Mask = 0;
  _SetCNTR(wInterrupt_Mask);

  // Clear pending interrupts
  _SetISTR(0);

  // Set interrupt mask
  wInterrupt_Mask = CNTR_RESETM | CNTR_SUSPM | CNTR_WKUPM;
  _SetCNTR(wInterrupt_Mask);

  // USB interrupts initialization
  // clear pending interrupts
  _SetISTR(0);
  wInterrupt_Mask = IMR_MSK;

  // set interrupts mask
  _SetCNTR(wInterrupt_Mask);

  bDeviceState = UNCONNECTED;
}

// reset routine
void MIOS32_USB_CB_Reset(void)
{
  // Set MIOS32 Device as not configured
  pInformation->Current_Configuration = 0;

  // Current Feature initialization
  pInformation->Current_Feature = MIOS32_USB_DESC_ConfigDescriptor[7];

  // Set MIOS32 Device with the default Interface
  pInformation->Current_Interface = 0;
  SetBTABLE(BTABLE_ADDRESS);

  // Initialize Endpoint 0
  SetEPType(ENDP0, EP_CONTROL);
  SetEPTxStatus(ENDP0, EP_TX_STALL);
  SetEPRxAddr(ENDP0, ENDP0_RXADDR);
  SetEPTxAddr(ENDP0, ENDP0_TXADDR);
  Clear_Status_Out(ENDP0);
  SetEPRxCount(ENDP0, Device_Property.MaxPacketSize);
  SetEPRxValid(ENDP0);

  // Initialize Endpoint 1
  SetEPType(ENDP1, EP_BULK);
  SetEPTxAddr(ENDP1, ENDP1_TXADDR);
  SetEPRxAddr(ENDP1, ENDP1_RXADDR);
  SetEPTxCount(ENDP1, 0x40);
  SetEPRxCount(ENDP1, 0x40);
  SetEPRxStatus(ENDP1, EP_RX_VALID);
  SetEPTxStatus(ENDP1, EP_TX_NAK);

  // Set this device to response on default address
  SetDeviceAddress(0);

  bDeviceState = ATTACHED;
}

// update the device state to configured.
void MIOS32_USB_CB_SetConfiguration(void)
{
  DEVICE_INFO *pInfo = &Device_Info;

  if (pInfo->Current_Configuration != 0) {
    bDeviceState = CONFIGURED;
  }
}

// update the device state to addressed
void MIOS32_USB_CB_SetDeviceAddress (void)
{
  bDeviceState = ADDRESSED;
}

// status IN routine
void MIOS32_USB_CB_Status_In(void)
{
}

// status OUT routine
void MIOS32_USB_CB_Status_Out(void)
{
}

// handles the data class specific requests
RESULT MIOS32_USB_CB_Data_Setup(u8 RequestNo)
{
  return USB_UNSUPPORT;
}

// handles the non data class specific requests.
RESULT MIOS32_USB_CB_NoData_Setup(u8 RequestNo)
{
  return USB_UNSUPPORT;
}

// gets the device descriptor.
u8 *MIOS32_USB_CB_GetDeviceDescriptor(u16 Length)
{
  return Standard_GetDescriptorData(Length, &Device_Descriptor);
}

// gets the configuration descriptor.
u8 *MIOS32_USB_CB_GetConfigDescriptor(u16 Length)
{
  return Standard_GetDescriptorData(Length, &Config_Descriptor);
}

// gets the string descriptors according to the needed index
u8 *MIOS32_USB_CB_GetStringDescriptor(u16 Length)
{
  u8 wValue0 = pInformation->USBwValue0;

  if (wValue0 > 4) {
    return NULL;
  } else {
    return Standard_GetDescriptorData(Length, &String_Descriptor[wValue0]);
  }
}

// test the interface and the alternate setting according to the supported one.
RESULT MIOS32_USB_CB_Get_Interface_Setting(u8 Interface, u8 AlternateSetting)
{
  if (AlternateSetting > 0) {
    return USB_UNSUPPORT;
  } else if (Interface > 1) {
    return USB_UNSUPPORT;
  }

  return USB_SUCCESS;
}

#endif /* MIOS32_DONT_USE_USB */
