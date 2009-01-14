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
#define DEFAULT_SRM_ENABLED     0
//
// define the shift registers to which the anodes of these LEDs are connected
// Note: they can be equal to DEFAULT_GP_DOUT_SR_[LH], this saves two shift registers, but doesn't allow a separate view of UI selections
#define DEFAULT_SRM_DOUT_L1	6
#define DEFAULT_SRM_DOUT_R1	9
//
// for misc. LED functions
#define DEFAULT_SRM_DOUT_M	0
//
// define the shift register to which the cathodes of these LEDs are connected
// Note that the whole shift register (8 pins) will be allocated! The 4 select lines are duplicated (4 for LED matrix, 4 for button matrix)
// The second DOUT_CATHODES2 selection is optional if LEDs with high power consumption are used - set this to 0 if not used
#define DEFAULT_SRM_DOUT_CATHODES1	5
#define DEFAULT_SRM_DOUT_CATHODES2	8
//
// another select line for misc. button/led functions - all 8 select pins are used for a 8x8 button/led matrix
#define DEFAULT_SRM_DOUT_CATHODESM	0
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
#define DEFAULT_SRM_DOUT_DUOCOLOUR	1
//
// define the shift registers to which the anodes of the "second colour" (red) LEDs are connected
#define DEFAULT_SRM_DOUT_L2	7
#define DEFAULT_SRM_DOUT_R2	10
//
// set this to 1 if a button matrix is connected
#define DEFAULT_SRM_BUTTONS_ENABLED 0
// set this to 1 if these buttons should only control the "step triggers" (gate, and other assigned triggers) - and no UI functions
#define DEFAULT_SRM_BUTTONS_NO_UI   1
// define the DIN shift registers to which the button matrix is connected
#define DEFAULT_SRM_DIN_L	11
#define DEFAULT_SRM_DIN_R	12
//
// 8x8 matrix for misc. button functions
#define DEFAULT_SRM_DIN_M	0


//                          SR    ignore    Pin
#define LED_TRACK1       ((( 1   -1)<<3)+    0)
#define LED_TRACK2       ((( 1   -1)<<3)+    1)
#define LED_TRACK3       ((( 1   -1)<<3)+    2)
#define LED_TRACK4       ((( 1   -1)<<3)+    3)

//                          SR    ignore    Pin
#define LED_PAR_LAYER_A  ((( 1   -1)<<3)+    4)
#define LED_PAR_LAYER_B  ((( 1   -1)<<3)+    5)
#define LED_PAR_LAYER_C  ((( 1   -1)<<3)+    6)

//                          SR    ignore    Pin
#define LED_BEAT         ((( 1   -1)<<3)+    7)

//                          SR    ignore    Pin
#define LED_EDIT         ((( 2   -1)<<3)+    0)
#define LED_MUTE         ((( 2   -1)<<3)+    1)
#define LED_PATTERN      ((( 2   -1)<<3)+    2)
#define LED_SONG         ((( 2   -1)<<3)+    3)

//                          SR    ignore    Pin
#define LED_SOLO         ((( 2   -1)<<3)+    4)
#define LED_FAST         ((( 2   -1)<<3)+    5)
#define LED_ALL          ((( 2   -1)<<3)+    6)

//                          SR    ignore    Pin
#define LED_GROUP1       (((11   -1)<<3)+    0) // OPTIONAL! see CHANGELOG.txt
#define LED_GROUP2       (((11   -1)<<3)+    2) // assigned to pin 2 due to DUO LED
#define LED_GROUP3       (((11   -1)<<3)+    4) // assigned to pin 4 due to DUO LED
#define LED_GROUP4       (((11   -1)<<3)+    6) // assigned to pin 6 due to DUO LED

//                          SR    ignore    Pin
#define LED_TRG_LAYER_A  (((12   -1)<<3)+    0) // OPTIONAL! see CHANGELOG.txt
#define LED_TRG_LAYER_B  (((12   -1)<<3)+    1)
#define LED_TRG_LAYER_C  (((12   -1)<<3)+    2)

//                          SR    ignore    Pin
#define LED_PLAY         (((12   -1)<<3)+    3) // OPTIONAL! see CHANGELOG.txt
#define LED_STOP         (((12   -1)<<3)+    4)
#define LED_PAUSE        (((12   -1)<<3)+    5)
#define LED_REW          (((12   -1)<<3)+    6)
#define LED_FWD          (((12   -1)<<3)+    7)

//                          SR    ignore    Pin
#define LED_MENU         (((13   -1)<<3)+    0)
#define LED_SCRUB        (((13   -1)<<3)+    1)
#define LED_METRONOME    (((13   -1)<<3)+    2)
#define LED_UTILITY      (((13   -1)<<3)+    3)
#define LED_COPY         (((13   -1)<<3)+    4)
#define LED_PASTE        (((13   -1)<<3)+    5)
#define LED_CLEAR        (((13   -1)<<3)+    6)

//                          SR    ignore    Pin
#define LED_F1           (((14   -1)<<3)+    0)
#define LED_F2           (((14   -1)<<3)+    1)
#define LED_F3           (((14   -1)<<3)+    2)
#define LED_F4           (((14   -1)<<3)+    3)

#define LED_STEP_1_16    (((14   -1)<<3)+    4) // was step 1..16 in MBSEQ V3
#define LED_STEPVIEW     (((14   -1)<<3)+    5)

#define LED_DOWN         (((14   -1)<<3)+    6)
#define LED_UP           (((14   -1)<<3)+    7)


//                          SR    ignore    Pin
#define BUTTON_DOWN      (((1    -1)<<3)+    0)
#define BUTTON_UP        (((1    -1)<<3)+    1)
#define BUTTON_LEFT      BUTTON_DISABLED
#define BUTTON_RIGHT     BUTTON_DISABLED

#define BUTTON_SCRUB     (((1    -1)<<3)+    2)
#define BUTTON_METRONOME (((1    -1)<<3)+    3)

#define BUTTON_STOP      (((1    -1)<<3)+    4)
#define BUTTON_PAUSE     (((1    -1)<<3)+    5)
#define BUTTON_PLAY      (((1    -1)<<3)+    6)
#define BUTTON_REW       (((1    -1)<<3)+    7)
#define BUTTON_FWD       (((2    -1)<<3)+    0)

#define BUTTON_F1        (((2    -1)<<3)+    1)
#define BUTTON_F2        (((2    -1)<<3)+    2)
#define BUTTON_F3        (((2    -1)<<3)+    3)
#define BUTTON_F4        (((2    -1)<<3)+    4)

#define BUTTON_MENU      (((2    -1)<<3)+    5)
#define BUTTON_SELECT    (((2    -1)<<3)+    6)
#define BUTTON_EXIT      (((2    -1)<<3)+    7) 

#define BUTTON_TRACK1    (((3    -1)<<3)+    0)
#define BUTTON_TRACK2    (((3    -1)<<3)+    1)
#define BUTTON_TRACK3    (((3    -1)<<3)+    2)
#define BUTTON_TRACK4    (((3    -1)<<3)+    3)

#define BUTTON_PAR_LAYER_A (((3    -1)<<3)+    4)
#define BUTTON_PAR_LAYER_B (((3    -1)<<3)+    5)
#define BUTTON_PAR_LAYER_C (((3    -1)<<3)+    6)

#define BUTTON_EDIT      (((4    -1)<<3)+    0)
#define BUTTON_MUTE      (((4    -1)<<3)+    1)
#define BUTTON_PATTERN   (((4    -1)<<3)+    2)
#define BUTTON_SONG      (((4    -1)<<3)+    3)

#define BUTTON_SOLO      (((4    -1)<<3)+    4)
#define BUTTON_FAST      (((4    -1)<<3)+    5)
#define BUTTON_ALL       (((4    -1)<<3)+    6)

#define BUTTON_GP1       (((7    -1)<<3)+    0)
#define BUTTON_GP2       (((7    -1)<<3)+    1)
#define BUTTON_GP3       (((7    -1)<<3)+    2)
#define BUTTON_GP4       (((7    -1)<<3)+    3)
#define BUTTON_GP5       (((7    -1)<<3)+    4)
#define BUTTON_GP6       (((7    -1)<<3)+    5)
#define BUTTON_GP7       (((7    -1)<<3)+    6)
#define BUTTON_GP8       (((7    -1)<<3)+    7)
#define BUTTON_GP9       (((10   -1)<<3)+    0)
#define BUTTON_GP10      (((10   -1)<<3)+    1)
#define BUTTON_GP11      (((10   -1)<<3)+    2)
#define BUTTON_GP12      (((10   -1)<<3)+    3)
#define BUTTON_GP13      (((10   -1)<<3)+    4)
#define BUTTON_GP14      (((10   -1)<<3)+    5)
#define BUTTON_GP15      (((10   -1)<<3)+    6)
#define BUTTON_GP16      (((10   -1)<<3)+    7)

#define BUTTON_GROUP1    (((13   -1)<<3)+    0)
#define BUTTON_GROUP2    (((13   -1)<<3)+    1)
#define BUTTON_GROUP3    (((13   -1)<<3)+    2)
#define BUTTON_GROUP4    (((13   -1)<<3)+    3)

#define BUTTON_TRG_LAYER_A (((13   -1)<<3)+    4)
#define BUTTON_TRG_LAYER_B (((13   -1)<<3)+    5)
#define BUTTON_TRG_LAYER_C (((13   -1)<<3)+    6)

#define BUTTON_STEP_VIEW (((13   -1)<<3)+    7)

#define BUTTON_TAP_TEMPO (((14   -1)<<3)+    0)

#define BUTTON_UTILITY   (((14   -1)<<3)+    1)
#define BUTTON_COPY      (((14   -1)<<3)+    2)
#define BUTTON_PASTE     (((14   -1)<<3)+    3)
#define BUTTON_CLEAR     (((14   -1)<<3)+    4)

#endif /* _SRIO_MAPPING_V4_H */
