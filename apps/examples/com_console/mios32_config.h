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


// temporary workaround for Windows: disable USB MIDI! :-(
#define MIOS32_DONT_USE_USB_MIDI

// enable USB COM instead
#define MIOS32_USE_USB_COM

#endif /* _MIOS32_CONFIG_H */
