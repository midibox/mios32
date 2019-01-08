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
#define MIOS32_LCD_BOOT_MSG_LINE1 "AINSER JitterMon V1.002"
#define MIOS32_LCD_BOOT_MSG_LINE2 "(C) 2014 T.Klose"

// enable two AINSER modules
#define AINSER_NUM_MODULES 2

#endif /* _MIOS32_CONFIG_H */
