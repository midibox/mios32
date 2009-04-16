/*
 * Local MIOS32 configuration file
 *
 * this file allows to disable (or re-configure) default functions of MIOS32
 * available switches are listed in $MIOS32_PATH/modules/mios32/MIOS32_CONFIG.txt
 *
 */

#ifndef _MIOS32_CONFIG_H
#define _MIOS32_CONFIG_H


#define BLM_X_NUM_ROWS 4
#define BLM_X_BTN_NUM_COLS 4
#define BLM_X_LED_NUM_COLS 4
#define BLM_X_LED_NUM_COLORS 3

#define BLM_X_ROWSEL_SR	4
#define BLM_X_LED_SR	5
#define BLM_X_BTN_SR	4

#define BLM_X_ROWSEL_INV_MASK	0x00
#define BLM_X_DEBOUNCE_MODE 2
#define BLM_X_DEBOUNCE_DELAY 1



#endif /* _MIOS32_CONFIG_H */


