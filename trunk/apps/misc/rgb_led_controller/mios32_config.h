// $Id: mios32_config.h 23 2008-09-16 17:34:42Z tk $
/*
 * Local MIOS32 configuration file
 *
 * this file allows to disable (or re-configure) default functions of MIOS32
 * available switches are listed in $MIOS32_PATH/modules/mios32/MIOS32_CONFIG.txt
 *
 */

#ifndef _MIOS32_CONFIG_H
#define _MIOS32_CONFIG_H

// AIN Configuration
#define MIOS32_DONT_USE_AIN
//#define MIOS32_AIN_CHANNEL_MASK 0x00ff

#define MIOS32_DONT_USE_MF
#define MIOS32_DONT_USE_LCD
#define MIOS32_DONT_USE_OSC
#define MIOS32_DONT_USE_IIC_MIDI
#define MIOS32_DONT_USE_DOUT
#define MIOS32_DONT_USE_SDCARD
#define MIOS32_DONT_USE_ENC28J60
//one UART is needed for the Midibox Link Feature
#define MIOS32_DONT_USE_UART
#define MIOS32_DONT_USE_UART_MIDI
// sets the IIC bus frequency in kHz (400000 max!)
#define MIOS32_IIC_BUS_FREQUENCY 400000

// sets the timeout value for IIC transactions (default: 5000 = ca. 5 mS)
#define MIOS32_IIC_TIMEOUT_VALUE 2000

// buffer size (should be at least >= MIOS32_USB_MIDI_DATA_*_SIZE/4)
#define MIOS32_USB_MIDI_RX_BUFFER_SIZE   512 // packages
#define MIOS32_USB_MIDI_TX_BUFFER_SIZE   512 // packages

// number of scanned SR registers on SRIO chain
// default value 16 (see mios32_srio.h)
// Knöpfli uses max. 256 DINs, that is 32. 
#define MIOS32_SRIO_NUM_SR 32

//maximum number of encoders. We try to match this with the control number, therefore we set it to 256 here
//well, 256 causes an endless loop. but we only need 255, so we use that here, because the last will be a slave anyway
#define MIOS32_ENC_NUM_MAX 255

//disable boot delay
#define MIOS32_LCD_BOOT_MSG_DELAY 0

// Following settings allow to customize the USB device descriptor
#define MIOS32_USB_VENDOR_STR   "ander.fm" // you will see this in the USB device description
#define MIOS32_USB_PRODUCT_STR  "Knoepfli"      // you will see this in the MIDI device list
#define MIOS32_USB_VERSION_ID   0x0100        // v1.00

// The boot message which is print during startup and returned on a SysEx query
#define MIOS32_LCD_BOOT_MSG_LINE1 "Knoepfli Buttonmatrix"
#define MIOS32_LCD_BOOT_MSG_LINE2 "ander.fm"

#endif /* _MIOS32_CONFIG_H */
