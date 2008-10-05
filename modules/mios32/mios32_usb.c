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

#include <FreeRTOS.h>
#include <portmacro.h>


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

void MIOS32_USB_TxBufferHandler(void);
void MIOS32_USB_RxBufferHandler(void);

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

// Rx buffer
u32 buffer_rx[MIOS32_USB_RX_BUFFER_SIZE];
volatile u16 buffer_rx_tail;
volatile u16 buffer_rx_head;
volatile u16 buffer_rx_size;
volatile u8 buffer_rx_new_data;

// Tx buffer
u32 buffer_tx[MIOS32_USB_TX_BUFFER_SIZE];
volatile u16 buffer_tx_tail;
volatile u16 buffer_tx_head;
volatile u16 buffer_tx_size;
volatile u8 buffer_tx_busy;

// optional non-blocking mode
u8 non_blocking_mode;


/////////////////////////////////////////////////////////////////////////////
// Initializes the USB interface
// IN: <mode>: 0: MIOS32_MIDI_Send* works in blocking mode - function will
//                (shortly) stall if the output buffer is full
//             1: MIOS32_MIDI_Send* works in non-blocking mode - function will
//                return -2 if buffer is full, the caller has to loop if this
//                value is returned until the transfer was successful
//                A common method is to release the RTOS task for 1 mS
//                so that other tasks can be executed until the sender can
//                continue
// OUT: returns < 0 if initialisation failed (e.g. clock not initialised!)
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_USB_Init(u32 mode)
{
  GPIO_InitTypeDef GPIO_InitStructure;
  GPIO_StructInit(&GPIO_InitStructure);
  u8 i;

  switch( mode ) {
    case 0:
      non_blocking_mode = 0;
      break;
    case 1:
      non_blocking_mode = 1;
      break;
    default:
      return -1; // unsupported mode
  }

  // clear buffer counters and busy/wait signals
  buffer_rx_tail = buffer_rx_head = buffer_rx_size = 0;
  buffer_rx_new_data = 0; // no data received yet
  buffer_tx_tail = buffer_tx_head = buffer_tx_size = 0;
  buffer_tx_busy = 0; // buffer is busy so long no USB connection detected


#ifdef _STM32_PRIMER_
  // configure USB disconnect pin
  // STM32 Primer: pin B12
  // first we hold it low for ca. 50 mS to force a re-enumeration
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12;
  GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOB, &GPIO_InitStructure);

  u32 delay;
  for(delay=0; delay<200000; ++delay) // produces a delay of ca. 50 mS @ 72 MHz (measured with scope)
    GPIOA->BRR = GPIO_Pin_12; // force pin to 0 (without this dummy code, an "empty" for loop could be removed by the compiler)

  GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IPD;
  GPIO_Init(GPIOB, &GPIO_InitStructure);
#else
  // using a "dirty" method to force a re-enumeration:
  // force DPM (Pin PA12) low for ca. 10 mS before USB Tranceiver will be enabled
  // this overrules the external Pull-Up at PA12, and at least Windows & MacOS will enumerate again
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12;
  GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOA, &GPIO_InitStructure);
  u32 delay;
  for(delay=0; delay<200000; ++delay) // produces a delay of ca. 50 mS @ 72 MHz (measured with scope)
    GPIOA->BRR = GPIO_Pin_12; // force pin to 0 (without this dummy code, an "empty" for loop could be removed by the compiler)
#endif

  // remaining initialisation done in STM32 USB driver
  USB_Init();

  return 0;
}


/////////////////////////////////////////////////////////////////////////////
// This function puts a new MIDI package into the Tx buffer
// IN: MIDI package in <package>
// OUT: 0: no error
//      -1: USB not connected
//      -2: if non-blocking mode activated: buffer is full
//          caller should retry until buffer is free again
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_USB_MIDIPackageSend(u32 package)
{
  // device available?
  // MEMO: when sending to MIOS Studio, bDeviceState can toggle between CONFIGURED and ADDRESSED
  if( bDeviceState == UNCONNECTED )
    return -1;

  // buffer full?
  if( buffer_tx_size >= (MIOS32_USB_TX_BUFFER_SIZE-1) ) {
    // call USB handler, so that we are able to get the buffer free again on next execution
    // (this call simplifies polling loops!)
    MIOS32_USB_Handler();

    // device still available?
    // (ensures that polling loop terminates if cable has been disconnected)
    if( bDeviceState == UNCONNECTED )
      return -1;

    // notify that buffer was full (request retry)
    if( non_blocking_mode )
      return -2;
  }

  // put package into buffer - this operation should be atomic!
  portDISABLE_INTERRUPTS(); // port specific FreeRTOS macro
  buffer_tx[buffer_tx_head++] = package;
  if( buffer_tx_head >= MIOS32_USB_TX_BUFFER_SIZE )
    buffer_tx_head = 0;
  ++buffer_tx_size;
  portENABLE_INTERRUPTS(); // port specific FreeRTOS macro

  return 0;
}


/////////////////////////////////////////////////////////////////////////////
// This function checks for a new package
// IN: pointer to MIDI package in <package> (received package will be put into the given variable)
// OUT: returns -1 if no package in buffer
//      otherwise returns number of packages which are still in the buffer
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_USB_MIDIPackageReceive(u32 *package)
{
  u8 i;

  // package received?
  if( !buffer_rx_size )
    return -1;

  // get package - this operation should be atomic!
  portDISABLE_INTERRUPTS(); // port specific FreeRTOS macro
  *package = buffer_rx[buffer_rx_tail];
  if( ++buffer_rx_tail >= MIOS32_USB_RX_BUFFER_SIZE )
    buffer_rx_tail = 0;
  --buffer_rx_size;
  portENABLE_INTERRUPTS(); // port specific FreeRTOS macro

  return buffer_rx_size;
}


/////////////////////////////////////////////////////////////////////////////
// This handler should be called from a RTOS task to react on
// USB interrupt notifications
// OUT: returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_USB_Handler(void)
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


  // do we need to react on the remaining events?
  // they are currently masked out via IMR_MSK


  // now check if something has to be sent or received
  MIOS32_USB_RxBufferHandler();
  MIOS32_USB_TxBufferHandler();

  return 0;
}


/////////////////////////////////////////////////////////////////////////////
// This handler sends the new packages through the IN pipe if the buffer 
// is not empty
/////////////////////////////////////////////////////////////////////////////
void MIOS32_USB_TxBufferHandler(void)
{
  // send buffered packages if
  //   - last transfer finished
  //   - new packages are in the buffer
  //   - the device is configured

  if( !buffer_tx_busy && buffer_tx_size && bDeviceState != UNCONNECTED ) {
    u32 *pma_addr = (u32 *)(PMAAddr + (ENDP1_TXADDR<<1));
    s16 count = (buffer_tx_size > (MIOS32_USB_DESC_DATA_IN_SIZE/4)) ? (MIOS32_USB_DESC_DATA_IN_SIZE/4) : buffer_tx_size;

    // notify that new package is sent
    buffer_tx_busy = 1;

    // send to IN pipe
    SetEPTxCount(ENDP1, 4*count);

    // atomic operation to avoid conflict with other interrupts
    portDISABLE_INTERRUPTS(); // port specific FreeRTOS macro
    buffer_tx_size -= count;

    // copy into PMA buffer (16bit word with, only 32bit addressable)
    do {
      *pma_addr++ = buffer_tx[buffer_tx_tail] & 0xffff;
      *pma_addr++ = (buffer_tx[buffer_tx_tail]>>16) & 0xffff;
      if( ++buffer_tx_tail >= MIOS32_USB_TX_BUFFER_SIZE )
	buffer_tx_tail = 0;
    } while( --count );

    portENABLE_INTERRUPTS(); // port specific FreeRTOS macro

    // send buffer
    SetEPTxValid(ENDP1);
  }
}


/////////////////////////////////////////////////////////////////////////////
// This handler receives new packages if the Tx buffer is not full
/////////////////////////////////////////////////////////////////////////////
void MIOS32_USB_RxBufferHandler(void)
{
  s16 count;

  // check if we can receive new data and get packages to be received from OUT pipe
  if( buffer_rx_new_data && (count=GetEPRxCount(ENDP1)>>2) ) {

    // check if buffer is free
    if( count < (MIOS32_USB_RX_BUFFER_SIZE-buffer_rx_size) ) {
      u32 *pma_addr = (u32 *)(PMAAddr + (ENDP1_RXADDR<<1));

      // copy received packages into receive buffer
      // this operation should be atomic
      portDISABLE_INTERRUPTS(); // port specific FreeRTOS macro
      do {
	u16 pl = *pma_addr++;
	u16 ph = *pma_addr++;
	buffer_rx[buffer_rx_head] = (ph << 16) | pl;
	if( ++buffer_rx_head >= MIOS32_USB_RX_BUFFER_SIZE )
	  buffer_rx_head = 0;
	++buffer_rx_size;
      } while( --count >= 0 );

      // notify, that data has been put into buffer
      buffer_rx_new_data = 0;

      portENABLE_INTERRUPTS(); // port specific FreeRTOS macro

      // release OUT pipe
      SetEPRxValid(ENDP1);
    }
  }
}


/////////////////////////////////////////////////////////////////////////////
// Called by STM32 USB driver to check for IN streams
/////////////////////////////////////////////////////////////////////////////
void MIOS32_USB_EP1_IN_Callback(void)
{
  // package has been sent
  buffer_tx_busy = 0;

  // check for next package
  MIOS32_USB_TxBufferHandler();
}

/////////////////////////////////////////////////////////////////////////////
// Called by STM32 USB driver to check for OUT streams
/////////////////////////////////////////////////////////////////////////////
void MIOS32_USB_EP1_OUT_Callback(void)
{
  // put package into buffer
  buffer_rx_new_data = 1;
  MIOS32_USB_RxBufferHandler();
}


/////////////////////////////////////////////////////////////////////////////
// Hooks of STM32 USB library
/////////////////////////////////////////////////////////////////////////////

// init routine
void MIOS32_USB_CB_Init(void)
{
  u32 delay;
  u16 wRegVal;

  pInformation->Current_Configuration = 0;

  // Connect the device

  // CNTR_PWDN = 0
  wRegVal = CNTR_FRES;
  _SetCNTR(wRegVal);

  // according to the reference manual, we have to wait at least for tSTARTUP = 1 uS before releasing reset
  for(delay=0; delay<10; ++delay) GPIOA->BRR = 0; // should be more than sufficient - add some dummy code here to ensure that the compiler doesn't optimize the empty for loop away

  // CNTR_FRES = 0
  wInterrupt_Mask = 0;
  _SetCNTR(wInterrupt_Mask);

  // Clear pending interrupts
  _SetISTR(0);

  // Configure USB clock
  // USBCLK = PLLCLK / 1.5
  RCC_USBCLKConfig(RCC_USBCLKSource_PLLCLK_1Div5);
  // Enable USB clock
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_USB, ENABLE);

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
  SetEPTxCount(ENDP1, MIOS32_USB_DESC_DATA_OUT_SIZE);
  SetEPTxStatus(ENDP1, EP_TX_NAK);

  SetEPRxAddr(ENDP1, ENDP1_RXADDR);
  SetEPRxCount(ENDP1, MIOS32_USB_DESC_DATA_IN_SIZE);
  SetEPRxValid(ENDP1);

  // Set this device to response on default address
  SetDeviceAddress(0);

  // clear buffer counters and busy/wait signals again (e.g., so that no invalid data will be sent out)
  buffer_rx_tail = buffer_rx_head = buffer_rx_size = 0;
  buffer_rx_new_data = 0; // no data received yet
  buffer_tx_tail = buffer_tx_head = buffer_tx_size = 0;
  buffer_tx_busy = 0; // buffer not busy anymore

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
