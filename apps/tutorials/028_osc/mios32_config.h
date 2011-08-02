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
#define MIOS32_LCD_BOOT_MSG_LINE1 "Tutorial #028"
#define MIOS32_LCD_BOOT_MSG_LINE2 "(C) 2011 T.Klose"


// for EEPROM emulation (resp. LPCXPRESSO/MBHP_CORE_LPC17: external IIC EEPROM)
#define EEPROM_EMULATED_SIZE 128

// magic number in EEPROM - if it doesn't exist at address 0x00..0x03, the EEPROM will be cleared
#define EEPROM_MAGIC_NUMBER 0x47114200


#endif /* _MIOS32_CONFIG_H */
