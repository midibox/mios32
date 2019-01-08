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
#define MIOS32_LCD_BOOT_MSG_LINE1 "IIC_MIDI Test V1.002"
#define MIOS32_LCD_BOOT_MSG_LINE2 "(C) 2014 T.Klose"


// IIC port J4A (=0) or J4B (=1)?
#define MIOS32_IIC_MIDI_PORT 0

// enable 8 interfaces
#define MIOS32_IIC_MIDI_NUM 8


#endif /* _MIOS32_CONFIG_H */
