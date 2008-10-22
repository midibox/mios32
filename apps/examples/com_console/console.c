// $Id$
/*
 * Demo application for ST7637 GLCD, stuffed on a STM32 Primer
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
#include <string.h>

#include "app.h"
#include "console.h"


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 CONSOLE_Init(void)
{
  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// Parser
/////////////////////////////////////////////////////////////////////////////
s32 CONSOLE_Parse(mios32_com_port_t port, u8 byte)
{
  u8 buffer[20];

  // parsing: TODO!

  sprintf(buffer, "Echo: '%c' (%02x)\n\r", byte, byte);
  MIOS32_COM_SendBuffer(port, buffer, strlen(buffer));

  return 0; // no error
}
