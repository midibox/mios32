// $Id$
/*
 * Temporary file which describes the DIN/DOUT mapping (later part of setup_* file)
 *
 * ==========================================================================
 *
 *  Copyright (C) 2008 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _SRIO_MAPPING_V4_H
#define _SRIO_MAPPING_V4_H

/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////

#define DEFAULT_GP_DOUT_SR_L    3       // first GP DOUT shift register assigned to SR#3
#define DEFAULT_GP_DOUT_SR_R    4       // second GP DOUT shift register assigned to SR#4


#define DEFAULT_GP_DOUT_SR_L2   15      // GP dual color: first GP DOUT shift register assigned to SR#14
#define DEFAULT_GP_DOUT_SR_R2   16      // GP dual color: second GP DOUT shift register assigned to SR#15

// === Shift Register Matrix ===
//
// set this value to 1 if each track has its own set of 16 LEDs to display unmuted steps and current sequencer position
// or if you are using a button/led matrix for misc. button/LED functions
#define DEFAULT_SRM_ENABLED     1
//
// define the shift registers to which the anodes of these LEDs are connected
// Note: they can be equal to DEFAULT_GP_DOUT_SR_[LH], this saves two shift registers, but doesn't allow a separate view of UI selections
#define DEFAULT_SRM_DOUT_L1	0
#define DEFAULT_SRM_DOUT_R1	0
//
// for misc. LED functions
#define DEFAULT_SRM_DOUT_M	2
//
// define the shift register to which the cathodes of these LEDs are connected
// Note that the whole shift register (8 pins) will be allocated! The 4 select lines are duplicated (4 for LED matrix, 4 for button matrix)
// The second DOUT_CATHODES2 selection is optional if LEDs with high power consumption are used - set this to 0 if not used
#define DEFAULT_SRM_DOUT_CATHODES1	0
#define DEFAULT_SRM_DOUT_CATHODES2	0
//
// another select line for misc. button/led functions - all 8 select pins are used for a 8x8 button/led matrix
#define DEFAULT_SRM_DOUT_CATHODESM	1
//
// 0: no mapping of Misc LEDs
// 1: enable GP LED -> DOUT_M mapping for Wilba's MB-SEQ PCB
#define DEFAULT_SRM_DOUT_M_MAPPING      1
//
// set an inversion mask for the DOUT shift registers if sink drivers (transistors)
// have been added to the cathode lines
// Settings: 0x00 - no sink drivers
//           0xf0 - sink drivers connected to D0..D3
//           0x0f - sink drivers connected to D7..D4
#define DEFAULT_SRM_CATHODES_INV_MASK   0x00
//
// same for misc. button/led functions
#define DEFAULT_SRM_CATHODES_INV_MASK_M 0x00
//
// set this to 1, if DUO colour LEDs are connected to the LED matrix
#define DEFAULT_SRM_DOUT_DUOCOLOUR	0
//
// define the shift registers to which the anodes of the "second colour" (red) LEDs are connected
#define DEFAULT_SRM_DOUT_L2	0
#define DEFAULT_SRM_DOUT_R2	0
//
// set this to 1 if a button matrix is connected
#define DEFAULT_SRM_BUTTONS_ENABLED 1
// set this to 1 if these buttons should only control the "step triggers" (gate, and other assigned triggers) - and no UI functions
#define DEFAULT_SRM_BUTTONS_NO_UI   1
// define the DIN shift registers to which the button matrix is connected
#define DEFAULT_SRM_DIN_L	0
#define DEFAULT_SRM_DIN_R	0
//
// 8x8 matrix for misc. button functions
#define DEFAULT_SRM_DIN_M	2


//                          SR    ignore    Pin
#define LED_TRACK1       (((23   -1)<<3)+    2)
#define LED_TRACK2       (((23   -1)<<3)+    1)
#define LED_TRACK3       (((21   -1)<<3)+    2)
#define LED_TRACK4       (((21   -1)<<3)+    1)

//                          SR    ignore    Pin
#define LED_PAR_LAYER_A  (((20   -1)<<3)+    2)
#define LED_PAR_LAYER_B  (((20   -1)<<3)+    1)
#define LED_PAR_LAYER_C  (((20   -1)<<3)+    0)

//                          SR    ignore    Pin
#define LED_BEAT         (((17   -1)<<3)+    1)

//                          SR    ignore    Pin
#define LED_EDIT         (((21   -1)<<3)+    3)
#define LED_MUTE         (((22   -1)<<3)+    3)
#define LED_PATTERN      (((22   -1)<<3)+    2)
#define LED_SONG         (((23   -1)<<3)+    3)

//                          SR    ignore    Pin
#define LED_SOLO         (((22   -1)<<3)+    1)
#define LED_FAST         (((22   -1)<<3)+    0)
#define LED_ALL          (((23   -1)<<3)+    0)

//                          SR    ignore    Pin
#define LED_GROUP1       (((24   -1)<<3)+    3)
#define LED_GROUP2       (((24   -1)<<3)+    2)
#define LED_GROUP3       (((24   -1)<<3)+    1)
#define LED_GROUP4       (((24   -1)<<3)+    0)

//                          SR    ignore    Pin
#define LED_TRG_LAYER_A  (((18   -1)<<3)+    2)
#define LED_TRG_LAYER_B  (((18   -1)<<3)+    1)
#define LED_TRG_LAYER_C  (((18   -1)<<3)+    0)

//                          SR    ignore    Pin
#define LED_PLAY         (((17   -1)<<3)+    3)
#define LED_STOP         (((19   -1)<<3)+    3)
#define LED_PAUSE        (((18   -1)<<3)+    3)
#define LED_REW          (((19   -1)<<3)+    2)
#define LED_FWD          (((17   -1)<<3)+    2)

//                          SR    ignore    Pin
#define LED_STEP_1_16    (((19   -1)<<3)+    0) // OPTIONAL! see CHANGELOG.txt
#define LED_STEP_17_32   (((19   -1)<<3)+    1)

//                          SR    ignore    Pin
#define LED_MENU         ((( 0   -1)<<3)+    0)
#define LED_SCRUB        ((( 0   -1)<<3)+    0)
#define LED_METRONOME    ((( 0   -1)<<3)+    0)


//                          SR    ignore    Pin
#define BUTTON_LEFT      (((20   -1)<<3)+    2)
#define BUTTON_RIGHT     (((20   -1)<<3)+    3)

#define BUTTON_SCRUB     (((20   -1)<<3)+    4)
#define BUTTON_METRONOME (((20   -1)<<3)+    5)

#define BUTTON_STOP      (((19   -1)<<3)+    5)
#define BUTTON_PAUSE     (((18   -1)<<3)+    4)
#define BUTTON_PLAY      (((18   -1)<<3)+    5)
#define BUTTON_REW       (((19   -1)<<3)+    4)
#define BUTTON_FWD       (((17   -1)<<3)+    5)

#define BUTTON_F1        (((19   -1)<<3)+    3)
#define BUTTON_F2        (((18   -1)<<3)+    2)
#define BUTTON_F3        (((18   -1)<<3)+    3)
#define BUTTON_F4        (((17   -1)<<3)+    2)

#define BUTTON_MENU      (((23   -1)<<3)+    5)
#define BUTTON_SELECT    (((24   -1)<<3)+    4)
#define BUTTON_EXIT      (((24   -1)<<3)+    5) 

#define BUTTON_TRACK1    (((23   -1)<<3)+    6)
#define BUTTON_TRACK2    (((23   -1)<<3)+    7)
#define BUTTON_TRACK3    (((21   -1)<<3)+    6)
#define BUTTON_TRACK4    (((21   -1)<<3)+    7)

#define BUTTON_PAR_LAYER_A (((20   -1)<<3)+    6)
#define BUTTON_PAR_LAYER_B (((20   -1)<<3)+    7)
#define BUTTON_PAR_LAYER_C (((19   -1)<<3)+    6)

#define BUTTON_EDIT      (((21   -1)<<3)+    3)
#define BUTTON_MUTE      (((22   -1)<<3)+    2)
#define BUTTON_PATTERN   (((22   -1)<<3)+    3)
#define BUTTON_SONG      (((23   -1)<<3)+    2)

#define BUTTON_SOLO      (((22   -1)<<3)+    4)
#define BUTTON_FAST      (((22   -1)<<3)+    5)
#define BUTTON_ALL       (((23   -1)<<3)+    4)

#define BUTTON_GP1       (((21   -1)<<3)+    0)
#define BUTTON_GP2       (((21   -1)<<3)+    1)
#define BUTTON_GP3       (((22   -1)<<3)+    0)
#define BUTTON_GP4       (((22   -1)<<3)+    1)
#define BUTTON_GP5       (((23   -1)<<3)+    0)
#define BUTTON_GP6       (((23   -1)<<3)+    1)
#define BUTTON_GP7       (((24   -1)<<3)+    0)
#define BUTTON_GP8       (((24   -1)<<3)+    1)
#define BUTTON_GP9       (((20   -1)<<3)+    0)
#define BUTTON_GP10      (((20   -1)<<3)+    1)
#define BUTTON_GP11      (((19   -1)<<3)+    0)
#define BUTTON_GP12      (((19   -1)<<3)+    1)
#define BUTTON_GP13      (((18   -1)<<3)+    0)
#define BUTTON_GP14      (((18   -1)<<3)+    1)
#define BUTTON_GP15      (((17   -1)<<3)+    0)
#define BUTTON_GP16      (((17   -1)<<3)+    1)

#define BUTTON_GROUP1    (((24   -1)<<3)+    6)
#define BUTTON_GROUP2    (((24   -1)<<3)+    7)
#define BUTTON_GROUP3    (((22   -1)<<3)+    6)
#define BUTTON_GROUP4    (((22   -1)<<3)+    7)

#define BUTTON_TRG_LAYER_A (((18   -1)<<3)+    6)
#define BUTTON_TRG_LAYER_B (((18   -1)<<3)+    7)
#define BUTTON_TRG_LAYER_C (((17   -1)<<3)+    6)

#define BUTTON_STEP_VIEW (((21   -1)<<3)+    5)

#define BUTTON_TAP_TEMPO ((( 0   -1)<<3)+    0)

#define BUTTON_UTILITY   (((19   -1)<<3)+    2)
#define BUTTON_COPY      (((23   -1)<<3)+    3)
#define BUTTON_PASTE     (((24   -1)<<3)+    2)
#define BUTTON_CLEAR     (((24   -1)<<3)+    3)

#endif /* _SRIO_MAPPING_V4_H */
