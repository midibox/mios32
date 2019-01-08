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


// supported: 1..3 (see comments in app_lcd.c of pcd8544 driver)
#define APP_LCD_NUM_X 3

// supported: 1..3 (see comments in app_lcd.c of pcd8544 driver)
#define APP_LCD_NUM_Y 3


#endif /* _MIOS32_CONFIG_H */
