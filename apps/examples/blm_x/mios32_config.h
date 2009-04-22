/*
 * Local MIOS32 configuration file
 *
 * this file allows to disable (or re-configure) default functions of MIOS32
 * available switches are listed in $MIOS32_PATH/modules/mios32/MIOS32_CONFIG.txt
 *
 */
 
#define MIOS32_LCD_BOOT_MSG_LINE1 "BLM_X Module Test"

#ifndef _MIOS32_CONFIG_H
#define _MIOS32_CONFIG_H


#define BLM_X_NUM_ROWS 4
#define BLM_X_BTN_NUM_COLS 4
#define BLM_X_LED_NUM_COLS 4
#define BLM_X_LED_NUM_COLORS 3

#define BLM_X_ROWSEL_DOUT_SR	4
#define BLM_X_LED_FIRST_DIN_SR	5
#define BLM_X_BTN_FIRST_DOUT_SR	4

#define BLM_X_ROWSEL_INV_MASK	0x00

#define BLM_X_DEBOUNCE_MODE 2


#endif /* _MIOS32_CONFIG_H */


