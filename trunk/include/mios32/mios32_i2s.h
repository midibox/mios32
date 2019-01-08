// $Id$
/*
 * Header file for I2S functions
 *
 * ==========================================================================
 *
 *  Copyright (C) 2008 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _MIOS32_I2S_H
#define _MIOS32_I2S_H

/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////

// MCLK at J15b:E not enabled by default - has to be done in mios32_config.h
// if I2S chip requires "master clock" (for oversampling)
#ifndef MIOS32_I2S_MCLK_ENABLE
#define MIOS32_I2S_MCLK_ENABLE  0
#endif

// supported by STM32: 
// I2S_Standard_Phillips, I2S_Standard_MSB, I2S_Standard_LSB,
// I2S_Standard_PCMShort, I2S_Standard_PCMLong
// note that "Philips" is written incorrectly (typo in STM32 driver)
#ifndef MIOS32_I2S_STANDARD
#define MIOS32_I2S_STANDARD     I2S_Standard_Phillips
#endif

// supported by STM32: 
// I2S_DataFormat_16b, I2S_DataFormat_16bextended, I2S_DataFormat_24b, I2S_DataFormat_32b
#ifndef MIOS32_I2S_DATA_FORMAT
#define MIOS32_I2S_DATA_FORMAT  I2S_DataFormat_16b;
#endif

// the sample rate in hertz
#ifndef MIOS32_I2S_AUDIO_FREQ
#define MIOS32_I2S_AUDIO_FREQ   48000
#endif


/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 MIOS32_I2S_Init(u32 mode);

extern s32 MIOS32_I2S_Start(u32 *buffer, u16 len, void *_callback);
extern s32 MIOS32_I2S_Stop(void);


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////


#endif /* _MIOS32_I2S_H */
