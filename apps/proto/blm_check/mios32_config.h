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
#define MIOS32_LCD_BOOT_MSG_LINE1 "BLM Demo"
#define MIOS32_LCD_BOOT_MSG_LINE2 "(C) 2009 T.Klose"


// see also $MIOS32_PATH/modules/blm/blm.h
#if 1
// BLM directly connected to core

#define BLM_DOUT_L1_SR	2
#define BLM_DOUT_R1_SR	5
#define BLM_DOUT_CATHODES_SR1	1
#define BLM_DOUT_CATHODES_SR2	4
#define BLM_CATHODES_INV_MASK	0x00
#define BLM_DOUT_L2_SR	3
#define BLM_DOUT_R2_SR	6
#define BLM_DOUT_L3_SR	7
#define BLM_DOUT_R3_SR	8
#define BLM_DIN_L_SR	1
#define BLM_DIN_R_SR	2
#define BLM_NUM_COLOURS 2
#define BLM_NUM_ROWS 8
#define BLM_DEBOUNCE_MODE 1

#else

// for testing the BLM driver with a BLM connected to MIDIbox SEQ
#define BLM_DOUT_L1_SR	6
#define BLM_DOUT_R1_SR	9
#define BLM_DOUT_CATHODES_SR1	5
#define BLM_DOUT_CATHODES_SR2	8
#define BLM_CATHODES_INV_MASK	0x00
#define BLM_DOUT_L2_SR	7
#define BLM_DOUT_R2_SR	10
#define BLM_DOUT_L3_SR	0
#define BLM_DOUT_R3_SR	0
#define BLM_DIN_L_SR	11
#define BLM_DIN_R_SR	12
#define BLM_NUM_COLOURS 2
#define BLM_NUM_ROWS 8
#define BLM_DEBOUNCE_MODE 1

#endif



#endif /* _MIOS32_CONFIG_H */
