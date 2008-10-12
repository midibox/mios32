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

// enable one IIC_MIDI interfae (for IIC collision testing)
#define MIOS32_IIC_MIDI_NUM 1

// enable 1 BankStick
#define MIOS32_IIC_BS_NUM 1

#endif /* _MIOS32_CONFIG_H */
