// $Id$
/*
 * Header file for MIOS32
 *
 * ==========================================================================
 *
 *  Copyright (C) 2008 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _MIOS32_H
#define _MIOS32_H

#ifdef __cplusplus
extern "C" {
#endif

/////////////////////////////////////////////////////////////////////////////
// processor specific variable types
/////////////////////////////////////////////////////////////////////////////

#if defined(MIOS32_FAMILY_STM32F10x)
# ifndef __cplusplus
#  include <stm32f10x_conf.h>
# else
  // STM32 drivers currently not enabled for C++ due to typedef conflicts (e.g. "bool")
#  include <mios32_datatypes.h>
# endif
#elif defined(MIOS32_FAMILY_LPC17xx)
# include <LPC17xx.h>
# include <mios32_datatypes.h>
#elif defined(MIOS32_FAMILY_EMULATION)
# include <mios32_datatypes.h>
#elif defined(MIOS32_FAMILY_MIOSJUCE)
# include <mios32_datatypes.h>
#else
# error "Unsupported MIOS32_FAMILY selected!"
#endif


/////////////////////////////////////////////////////////////////////////////
// include C headers
/////////////////////////////////////////////////////////////////////////////

#include <stdlib.h>
#include <stdio.h>


/////////////////////////////////////////////////////////////////////////////
// include local config file
// (see MIOS32_CONFIG.txt for available switches)
/////////////////////////////////////////////////////////////////////////////

#include "mios32_config.h"


/////////////////////////////////////////////////////////////////////////////
// include mios32_*.h files of all MIOS modules
/////////////////////////////////////////////////////////////////////////////

#include <mios32_irq.h>
#include <mios32_sys.h>
#include <mios32_spi.h>
#include <mios32_srio.h>
#include <mios32_din.h>
#include <mios32_dout.h>
#include <mios32_enc.h>
#include <mios32_ain.h>
#include <mios32_mf.h>
#include <mios32_lcd.h>
#include <mios32_midi.h>
#include <mios32_osc.h>
#include <mios32_com.h>
#include <mios32_usb.h>
#include <mios32_usb_midi.h>
#include <mios32_usb_com.h>
#include <mios32_uart.h>
#include <mios32_uart_midi.h>
#include <mios32_iic.h>
#include <mios32_iic_bs.h>
#include <mios32_iic_midi.h>
#include <mios32_i2s.h>
#include <mios32_board.h>
#include <mios32_timer.h>
#include <mios32_stopwatch.h>
#include <mios32_delay.h>
#include <mios32_sdcard.h>
#include <mios32_enc28j60.h>


/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////


#ifdef __cplusplus
}
#endif

#endif /* _MIOS32_H */
