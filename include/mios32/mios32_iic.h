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

// sets the IIC bus frequency in kHz (400000 for common "fast speed" devices, 1000000 for high-speed devices)
#ifndef MIOS32_IIC_BUS_FREQUENCY
#define MIOS32_IIC_BUS_FREQUENCY 400000
#endif

// sets the timeout value for IIC transactions (default: 5000 = ca. 5 mS)
#ifndef MIOS32_IIC_TIMEOUT_VALUE
#define MIOS32_IIC_TIMEOUT_VALUE 5000
#endif

// sets the buffer size used for background transmit/receive
// (66 for EEPROM page writes: 2 bytes for address, 64 bytes for page)
#ifndef MIOS32_IIC_BUFFER_SIZE
#define MIOS32_IIC_BUFFER_SIZE 66
#endif


// error flags
#define MIOS32_IIC_ERROR_GENERAL                    -1
#define MIOS32_IIC_ERROR_UNSUPPORTED_TRANSFER_TYPE  -2
#define MIOS32_IIC_ERROR_TIMEOUT                    -3
#define MIOS32_IIC_ERROR_ARBITRATION_LOST           -4
#define MIOS32_IIC_ERROR_BUS                        -5
#define MIOS32_IIC_ERROR_SLAVE_NOT_CONNECTED        -6
#define MIOS32_IIC_ERROR_UNEXPECTED_EVENT           -7
#define MIOS32_IIC_ERROR_RX_BUFFER_OVERRUN          -8
#define MIOS32_IIC_ERROR_TX_BUFFER_NOT_BIG_ENOUGH   -9


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

extern s32 MIOS32_IIC_TransferBegin(mios32_iic_semaphore_t semaphore_type);
extern s32 MIOS32_IIC_TransferFinished(void);

extern s32 MIOS32_IIC_Transfer(mios32_iic_transfer_t transfer, u8 address, u8 *buffer, u8 len);
extern s32 MIOS32_IIC_TransferCheck(void);
extern s32 MIOS32_IIC_TransferWait(void);

extern s32 MIOS32_IIC_LastErrorGet(void);

/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////



#endif /* _MIOS32_IIC_H */
