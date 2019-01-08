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
#define MIOS32_LCD_BOOT_MSG_LINE1 "SPI MIDI Test"
#define MIOS32_LCD_BOOT_MSG_LINE2 "(C) 2014 T.Klose"

// how many SPI MIDI ports are available?
// if 0: interface disabled (default)
// other allowed values: 1..8
#define MIOS32_SPI_MIDI_NUM_PORTS 4

// enable 4 USB MIDI ports
#define MIOS32_USB_MIDI_NUM_PORTS 4


#endif /* _MIOS32_CONFIG_H */
