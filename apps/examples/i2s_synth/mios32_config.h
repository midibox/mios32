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

// I2S device connected to J8 (-> SPI2), therefore we have to use SPI1 (-> J16) for SRIO chain
#define MIOS32_SRIO_SPI 1

// I2S support has to be enabled explicitely
#define MIOS32_USE_I2S

// enable MCLK pin (not for STM32 primer)
#ifdef MIOS32_BOARD_STM32_PRIMER
# define MIOS32_I2S_MCLK_ENABLE  0
#else
# define MIOS32_I2S_MCLK_ENABLE  1
#endif

#endif /* _MIOS32_CONFIG_H */
