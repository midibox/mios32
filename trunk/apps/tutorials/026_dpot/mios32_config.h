// $Id: mios32_config.h $
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
#define MIOS32_LCD_BOOT_MSG_LINE1 "Tutorial #026"
#define MIOS32_LCD_BOOT_MSG_LINE2 "(C) 2010 JRFarmer"

// We are using Microchip MCP42XXX digital potentiometer ICs
#define DPOT_MCP42XXX
#define DPOT_NUM_POTS 4

// Digital potentiometer on SPI2, using RC 0
#define DPOT_SPI 2
#define DPOT_SPI_RC_PIN 0


#endif /* _MIOS32_CONFIG_H */
