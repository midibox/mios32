// $Id$
//
// Includes the bootloader code into the project
// If no bootloader is provided for the board, reserve the BSL range instead
//
/* ==========================================================================
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

// This switch is only for expert usage - it frees 16k, but can lead to
// big trouble if a user downloads the binary with the MIOS32 BSL, and
// doesn't have access to a JTAG interface or COM port + MBHP_LTC module
// to recover the BSL
#ifndef MIOS32_DONT_INCLUDE_BSL
# if defined(MIOS32_BOARD_STM32F4DISCOVERY) || defined(MIOS32_BOARD_MBHP_CORE_STM32F4)
#  include "mios32_bsl_STM32F4DISCOVERY.inc"
# else
#  warning "This MIOS32_PROCESSOR isn't prepared in mios32_bsl.c - selecting bootloader of STM32F4DISCOVERY"
#  include "mios32_bsl_STM32F4DISCOVERY.inc"
# endif
#endif /* MIOS32_DONT_INCLUDE_BSL */
