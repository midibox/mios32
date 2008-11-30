// $Id: mios32_config.h 56 2008-10-04 00:46:12Z tk $
/*
 * Local MIOS32 configuration file
 *
 * this file allows to disable (or re-configure) default functions of MIOS32
 * available switches are listed in $MIOS32_PATH/modules/mios32/MIOS32_CONFIG.txt
 *
 */

#ifndef _MIOS32_CONFIG_H
#define _MIOS32_CONFIG_H



#define MIOS32_DONT_USE_USB_MIDI // FIXME TESTING
//#define MIOS32_DONT_USE_BOARD // FIXME TESTING

// enable USB COM instead
#define MIOS32_USE_USB_COM // FIXME TESTING

#endif /* _MIOS32_CONFIG_H */
