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
#define MIOS32_LCD_BOOT_MSG_LINE1 "IIC Access Benchmark"
#define MIOS32_LCD_BOOT_MSG_LINE2 "(c) 2009 T.Klose"


// sets the IIC bus frequency in kHz (400000 for common "fast speed" devices, 1000000 for high-speed devices)
#define MIOS32_IIC_BUS_FREQUENCY 400000
//#define MIOS32_IIC_BUS_FREQUENCY 1000000


#endif /* _MIOS32_CONFIG_H */
