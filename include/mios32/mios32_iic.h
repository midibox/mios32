// $Id$
/*
 * Header file for IIC functions
 *
 * ==========================================================================
 *
 *  Copyright (C) 2008 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _MIOS32_IIC_H
#define _MIOS32_IIC_H

/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////

// number of IIC devices
// STM32: (1: I2C2(iic_port 0) available ; 2: I2C2 and I2C1(iic_port 1) available)
// LPC17: (1: I2C0(iic_port 0) available ; 2: I2C1 (EEPROM) in addition, 3: I2C2 assigned to iic_port 1 available)
#ifndef MIOS32_IIC_NUM
#if defined(MIOS32_FAMILY_STM32F10x) || defined(MIOS32_FAMILY_STM32F4xx)
#define MIOS32_IIC_NUM 1
#elif defined(MIOS32_FAMILY_LPC17xx)
// The third IIC port at J4B is disabled by default so that the app can decide if it's used for UART or IIC
#define MIOS32_IIC_NUM 2
#else
#define MIOS32_IIC_NUM 1
# warning "mios32_iic.h not prepared for this derivative"
#endif
#endif

// sets the IIC bus frequency in Hz (max. 400000, bus frequencies > 400kHz don't work stable)
#ifndef MIOS32_IIC0_BUS_FREQUENCY
#define MIOS32_IIC0_BUS_FREQUENCY 400000
#endif

// bus frequency for I2C1 device in Hz (max. 400000, bus frequencies > 400kHz don't work stable)
#ifndef MIOS32_IIC1_BUS_FREQUENCY
#define MIOS32_IIC1_BUS_FREQUENCY 400000
#endif

// bus frequency for I2C2 device in Hz (max. 400000, bus frequencies > 400kHz don't work stable)
#ifndef MIOS32_IIC2_BUS_FREQUENCY
#define MIOS32_IIC2_BUS_FREQUENCY 400000
#endif

// sets the timeout value for IIC transactions (default: 5000 = ca. 5 mS)
#ifndef MIOS32_IIC_TIMEOUT_VALUE
#define MIOS32_IIC_TIMEOUT_VALUE 5000
#endif


// error flags
#define MIOS32_IIC_ERROR_INVALID_PORT               -1
#define MIOS32_IIC_ERROR_GENERAL                    -2
#define MIOS32_IIC_ERROR_UNSUPPORTED_TRANSFER_TYPE  -3
#define MIOS32_IIC_ERROR_TIMEOUT                    -4
#define MIOS32_IIC_ERROR_ARBITRATION_LOST           -5
#define MIOS32_IIC_ERROR_BUS                        -6
#define MIOS32_IIC_ERROR_SLAVE_NOT_CONNECTED        -7
#define MIOS32_IIC_ERROR_UNEXPECTED_EVENT           -8
#define MIOS32_IIC_ERROR_RX_BUFFER_OVERRUN          -9


// if previous transfer failed, this offset will be added to error status
#define MIOS32_IIC_ERROR_PREV_OFFSET -128


/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////

typedef enum {
  IIC_Blocking,
  IIC_Non_Blocking
} mios32_iic_semaphore_t;

typedef enum {
  IIC_Read,
  IIC_Write,
  IIC_Read_AbortIfFirstByteIs0,
  IIC_Write_WithoutStop
} mios32_iic_transfer_t;


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 MIOS32_IIC_Init(u32 mode);

extern s32 MIOS32_IIC_TransferBegin(u8 iic_port, mios32_iic_semaphore_t semaphore_type);
extern s32 MIOS32_IIC_TransferFinished(u8 iic_port);

extern s32 MIOS32_IIC_Transfer(u8 iic_port, mios32_iic_transfer_t transfer, u8 address, u8 *buffer, u16 len);
extern s32 MIOS32_IIC_TransferCheck(u8 iic_port);
extern s32 MIOS32_IIC_TransferWait(u8 iic_port);

extern s32 MIOS32_IIC_LastErrorGet(u8 iic_port);

/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////



#endif /* _MIOS32_IIC_H */
