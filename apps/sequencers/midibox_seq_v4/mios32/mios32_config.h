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

// for debugging via UART1 (application uses printf() to output helpful debugging messages)
#define COM_DEBUG 1


#ifdef COM_DEBUG
  // enable COM via UART1
# define MIOS32_UART1_ASSIGNMENT 2
# define MIOS32_UART1_BAUDRATE 115200

  // use UART1 as default COM port
# define MIOS32_COM_DEFAULT_PORT UART1
#endif


#define MID_PLAYER_TEST 0


// the speed value for the datawheel (#0) which is used when the "FAST" button is activated:
#define DEFAULT_DATAWHEEL_SPEED_VALUE	3

// the speed value for the additional encoders (#1-#16) which is used when the "FAST" button is activated:
#define DEFAULT_ENC_SPEED_VALUE		3

// Auto FAST mode: if a layer is assigned to velocity or CC, the fast button will be automatically
// enabled - in other cases (e.g. Note or Length), the fast button will be automatically disabled
#define DEFAULT_AUTO_FAST_BUTTON        1


// Toggle behaviour of various buttons
// 0: active mode so long button pressed
// 1: pressing button toggles the mode
#define DEFAULT_BEHAVIOUR_BUTTON_FAST	1
#define DEFAULT_BEHAVIOUR_BUTTON_ALL	1
#define DEFAULT_BEHAVIOUR_BUTTON_SOLO	1
#define DEFAULT_BEHAVIOUR_BUTTON_METRON	1
#define DEFAULT_BEHAVIOUR_BUTTON_SCRUB	0
#define DEFAULT_BEHAVIOUR_BUTTON_MENU	0


// include SRIO setup here, so that we can propagate values to external modules
#include "srio_mapping.h"

// forward to BLM8x8 driver
#define BLM8X8_DOUT	          DEFAULT_SRM_DOUT_M
#define BLM8X8_DOUT_CATHODES	  DEFAULT_SRM_DOUT_CATHODESM
#define BLM8X8_CATHODES_INV_MASK  DEFAULT_SRM_CATHODES_INV_MASK_M
#define BLM8X8_DIN	          DEFAULT_SRM_DIN_M

#endif /* _MIOS32_CONFIG_H */
