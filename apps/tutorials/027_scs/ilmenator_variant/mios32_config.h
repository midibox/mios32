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
#define MIOS32_LCD_BOOT_MSG_LINE1 "Tutorial #027"
#define MIOS32_LCD_BOOT_MSG_LINE2 "(C) 2011 T.Klose"


// the used encoder type (see mios32_enc.h)
#define SCS_ENC_TYPE DETENTED2

// define this to display 4 items with a with of 2x5 chars on a 2x20 LCD
//#define SCS_MENU_ITEM_WIDTH 5
//#define SCS_NUM_MENU_ITEMS 4


// Buttons and encoder are connected to a DIN shift register and not to J10

// disable encoder pins at J10
#define SCS_PIN_ENC_MENU_A 255
#define SCS_PIN_ENC_MENU_B 255

// if set to 1, the menu handler doesn't require a soft button
// instead, items are selected with the rotary encoder, and the selection is
// confirmed with a "SELECT" button (button connected to SCS_PIN_SOFT1)
// The remaining SOFT buttons have no function!
#define SCS_MENU_NO_SOFT_BUTTON_MODE 1

// to which DIN SR and Pin (first one) is the encoder connected?
#define APP_SCS_ENC_SR        1
#define APP_SCS_ENC_FIRST_PIN 0

// pin assignments:
// encoder connected to DIN #1 pin D0 and D1
// remaining pins:
#define SCS_PIN_DEC 2
#define SCS_PIN_INC 3
#define SCS_PIN_EXIT       4
#define SCS_PIN_SOFT1      5 // selects an item

#endif /* _MIOS32_CONFIG_H */
