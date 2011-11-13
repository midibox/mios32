// $Id: mios32_config.h 1171 2011-04-10 18:58:03Z tk $
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
#define MIOS32_LCD_BOOT_MSG_LINE1 "SD card polyphonic sample player"
#define MIOS32_LCD_BOOT_MSG_LINE2 "(C) 2011 L O'Donnell lee@bassmaker.co.uk"

// Local override for sampling rate
#define MIOS32_I2S_AUDIO_FREQ   44100

// Override SD card speed to make it faster
#define MIOS32_SDCARD_SPI_PRESCALER MIOS32_SPI_PRESCALER_4

#if defined(MIOS32_FAMILY_STM32F10x)
// I2S device connected to J8 (-> SPI1), therefore we have to use SPI0 (-> J16) for SRIO chain
# define MIOS32_SRIO_SPI 0
#endif

// avoid disk_read in FILE_ReadReOpen
#define FILE_NO_DISK_READ_ON_READREOPEN 1


// I2S support has to be enabled explicitely
#define MIOS32_USE_I2S

// enable MCLK pin (not for STM32 primer)
#ifdef MIOS32_BOARD_STM32_PRIMER
# define MIOS32_I2S_MCLK_ENABLE  0
#else
# define MIOS32_I2S_MCLK_ENABLE  1
#endif

#endif /* _MIOS32_CONFIG_H */
