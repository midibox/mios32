// $Id$
/*
 * Header file for IIC BankStick functions
 *
 * ==========================================================================
 *
 *  Copyright (C) 2008 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _MIOS32_IIC_BS_H
#define _MIOS32_IIC_BS_H

/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////

// base address of first BankStick
#ifndef MIOS32_IIC_BS_ADDR_BASE
#define MIOS32_IIC_BS_ADDR_BASE 0xa0
#endif

// max number of BankSticks (0..8)
// has to be set to >0 in mios32_config.h to enable BankStick Support
#ifndef MIOS32_IIC_BS_NUM
#define MIOS32_IIC_BS_NUM 0
#endif

// _SIZE: 0 = BankStick disabled, 32768 or 65536 = BankStick enabled with given size in kilobyte
#ifndef MIOS32_IIC_BS0_SIZE
#define MIOS32_IIC_BS0_SIZE    32768
#endif
#ifndef MIOS32_IIC_BS1_SIZE
#define MIOS32_IIC_BS1_SIZE    32768
#endif
#ifndef MIOS32_IIC_BS2_SIZE
#define MIOS32_IIC_BS2_SIZE    32768
#endif
#ifndef MIOS32_IIC_BS3_SIZE
#define MIOS32_IIC_BS3_SIZE    32768
#endif
#ifndef MIOS32_IIC_BS4_SIZE
#define MIOS32_IIC_BS4_SIZE    32768
#endif
#ifndef MIOS32_IIC_BS5_SIZE
#define MIOS32_IIC_BS5_SIZE    32768
#endif
#ifndef MIOS32_IIC_BS6_SIZE
#define MIOS32_IIC_BS6_SIZE    32768
#endif
#ifndef MIOS32_IIC_BS7_SIZE
#define MIOS32_IIC_BS7_SIZE    32768
#endif


/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 MIOS32_IIC_BS_Init(u32 mode);

extern s32 MIOS32_IIC_BS_ScanBankSticks(void);
extern s32 MIOS32_IIC_BS_CheckAvailable(u8 bs);

extern s32 MIOS32_IIC_BS_Read(u8 bs, u16 address, u8 *buffer, u8 len);
extern s32 MIOS32_IIC_BS_Write(u8 bs, u16 address, u8 *buffer, u8 len);
extern s32 MIOS32_IIC_BS_CheckWriteFinished(u8 bs);


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////



#endif /* _MIOS32_IIC_MIDI_H */
