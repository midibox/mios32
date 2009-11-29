// $Id$
/*
 * Local MIOS32 configuration file
 *
 * this file allows to disable (or re-configure) default functions of MIOS32
 * available switches are listed in $MIOS32_PATH/modules/mios32/MIOS32_CONFIG.txt
 *
 */

#ifndef _MIOS32_CONFIG_H
#define _MIOS32_CONFIG_H

// The boot message which is print during startup and returned on a SysEx query
#if BUILD_FOR_DUT
# define MIOS32_LCD_BOOT_MSG_LINE1 "IO Scan test (DUT)"
#else
# define MIOS32_LCD_BOOT_MSG_LINE1 "IO Scan test (Tester)"
#endif
#define MIOS32_LCD_BOOT_MSG_LINE2 "Open MIOS Terminal to see the results"


// ensure that modules which are initialising pins are disabled
#define MIOS32_DONT_USE_SRIO
#define MIOS32_DONT_USE_AIN
#define MIOS32_DONT_USE_MF
#define MIOS32_DONT_USE_IIC

// LCD pins are disabled after a warning message has been print
// UARTs is enabled/disabled on the fly to allow MIDI queries over these ports

#endif /* _MIOS32_CONFIG_H */
