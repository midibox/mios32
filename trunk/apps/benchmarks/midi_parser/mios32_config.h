// $Id: mios32_config.h 1196 2011-04-30 20:18:46Z tk $
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
#define MIOS32_LCD_BOOT_MSG_LINE1 "MIDI Parser Benchmark"
#define MIOS32_LCD_BOOT_MSG_LINE2 "(c) 2012 T.Klose"


// function used to output debug messages (must be printf compatible!)
#define DEBUG_MSG MIOS32_MIDI_SendDebugMessage

#endif /* _MIOS32_CONFIG_H */
