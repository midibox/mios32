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
#define MIOS32_LCD_BOOT_MSG_LINE1 "App. Template"
#define MIOS32_LCD_BOOT_MSG_LINE2 "(C) 2009 <your name>"

#define MIOS32_DONT_SERVICE_SRIO_SCAN 1	//jm
//#define MIOS32_DONT_USE_SRIO 1 	//jm
#define MIOS32_SRIO_NUM_SR 8		//jm
#define MIOS32_DONT_USE_ENC 0		//jm
#define MIOS32_DONT_USE_DIN 0		//jm
#define MIOS32_DONT_USE_AIN 0		//jm
#define MIOS32_DONT_USE_COM 0		//jm
#define MIOS32_SRIO_OUTPUTS_OD 0	//jm


#endif /* _MIOS32_CONFIG_H */
