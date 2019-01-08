// $Id: mios32_config.h 358 2009-02-21 23:21:17Z tk $
/*
 * Local MIOS32 configuration file
 *
 * this file allows to disable (or re-configure) default functions of MIOS32
 * available switches are listed in $MIOS32_PATH/modules/mios32/MIOS32_CONFIG.txt
 *
 */

#ifndef _MIOS32_CONFIG_H
#define _MIOS32_CONFIG_H

// I2S device connected to J8 (-> SPI1), therefore we have to use SPI0 (-> J16) for SRIO chain
#define MIOS32_SRIO_SPI 0

// I2S support has to be enabled explicitely
#define MIOS32_USE_I2S

// enable one IIC_MIDI interface (for IIC collision testing)
#define MIOS32_IIC_MIDI_NUM 1

// enable all BankSticks
#define MIOS32_IIC_BS_NUM 8

// enable MCLK pin (not for STM32 primer)
//#ifdef MIOS32_BOARD_STM32_PRIMER
//# define MIOS32_I2S_MCLK_ENABLE  0
//#else
#define MIOS32_I2S_MCLK_ENABLE  1
//#endif


// TK: reduced delay buffer size for LPC1768/LPC1769 to allow compile without linker tricks
#if defined(MIOS32_PROCESSOR_LPC1768) || defined(MIOS32_PROCESSOR_LPC1769)
# define DELAY_BUFFER_SIZE 8192
#else
# define DELAY_BUFFER_SIZE 16384
#endif

#endif /* _MIOS32_CONFIG_H */
