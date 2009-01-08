//
//  MIOS32_SYS_Wrapper.m
//
//  Created by Thorsten Klose on 02.01.09.
//  Copyright 2009 __MyCompanyName__. All rights reserved.
//

#import "MIOS32_SYS_Wrapper.h"

#include <mios32.h>

@implementation MIOS32_SYS_Wrapper

// local variables to bridge objects to C functions
static NSObject *_self;


//////////////////////////////////////////////////////////////////////////////
// init local variables
//////////////////////////////////////////////////////////////////////////////
- (void) awakeFromNib
{
	_self = self;
}


/////////////////////////////////////////////////////////////////////////////
//! Initializes the System for MIOS32:<BR>
//! \param[in] mode currently only mode 0 supported
//! \return < 0 if initialisation failed
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_SYS_Init(u32 mode)
{
	NSLog(@"MIOS32 System Initialized");

  return 0; // nothing to do here
  
}


/////////////////////////////////////////////////////////////////////////////
//! Shutdown MIOS32 and reset the microcontroller:<BR>
//! \return < 0 if reset failed
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_SYS_Reset(void)
{
  // print reboot message if LCD enabled
#ifndef MIOS32_DONT_USE_LCD
  // TODO: here we should select the normal font - but only if available!
  // MIOS32_LCD_FontInit((u8 *)GLCD_FONT_NORMAL);
  MIOS32_LCD_BColourSet(0xff, 0xff, 0xff);
  MIOS32_LCD_FColourSet(0x00, 0x00, 0x00);

  MIOS32_LCD_DeviceSet(0);
  MIOS32_LCD_Clear();
  MIOS32_LCD_CursorSet(0, 0);
  MIOS32_LCD_PrintString("Rebooting MIOS32...");
  NSLog(@"Rebooting MIOS32...");

#endif

  return 0;
}


/////////////////////////////////////////////////////////////////////////////
//! Returns the Chip ID of the core
//! \return the chip ID
/////////////////////////////////////////////////////////////////////////////
u32 MIOS32_SYS_ChipIDGet(void)
{
  return 0x00000000;
}

/////////////////////////////////////////////////////////////////////////////
//! Returns the Flash size of the core
//! \return the Flash size in bytes
/////////////////////////////////////////////////////////////////////////////
u32 MIOS32_SYS_FlashSizeGet(void)
{
  return 0xffffffff;
}

/////////////////////////////////////////////////////////////////////////////
//! Returns the (data) RAM size of the core
//! \return the RAM size in bytes
/////////////////////////////////////////////////////////////////////////////
u32 MIOS32_SYS_RAMSizeGet(void)
{
  return 0xffffffff;
}

/////////////////////////////////////////////////////////////////////////////
//! Returns the serial number as a string
//! \param[out] str pointer to a string which can store at least 32 digits + zero terminator!
//! (24 digits returned for STM32)
//! \return < 0 if feature not supported
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_SYS_SerialNumberGet(char *str)
{
  int i;

  for(i=0; i<24; ++i) {
    u8 b = 0xff;
    str[i] = ((b > 9) ? ('A'-10) : '0') + b;
  }
  str[i] = 0;

  return 0; // no error
}


@end
