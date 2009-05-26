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


// use printf instead of MIOS32_MIDI_SendDebugMessage to print debug messages
#define MIOS32_OSC_DEBUG_MSG printf


#endif /* _MIOS32_CONFIG_H */
