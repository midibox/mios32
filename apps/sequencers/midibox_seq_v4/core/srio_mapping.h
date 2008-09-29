// $Id: mios32_dout.h 10 2008-09-14 21:36:51Z tk $
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

#ifndef _SRIO_MAPPING_H
#define _SRIO_MAPPING_H

/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////

#define DEFAULT_GP_DIN_SR_L     7       // first GP DIN shift register assigned to SR#7
#define DEFAULT_GP_DIN_SR_R     10      // second GP DIN shift register assigned to SR#10


#define DEFAULT_GP_DOUT_SR_L    3       // first GP DOUT shift register assigned to SR#3
#define DEFAULT_GP_DOUT_SR_R    4       // second GP DOUT shift register assigned to SR#4


#define DEFAULT_GP_DOUT_SR_L2   15      // GP dual color: first GP DOUT shift register assigned to SR#14
#define DEFAULT_GP_DOUT_SR_R2   16      // GP dual color: second GP DOUT shift register assigned to SR#15

//                          SR    ignore    Pin
#define LED_TRACK1       ((( 1   -1)<<3)+    7)
#define LED_TRACK2       ((( 1   -1)<<3)+    6)
#define LED_TRACK3       ((( 1   -1)<<3)+    5)
#define LED_TRACK4       ((( 1   -1)<<3)+    4)

//                          SR    ignore    Pin
#define LED_PAR_LAYER_A  ((( 1   -1)<<3)+    3)
#define LED_PAR_LAYER_B  ((( 1   -1)<<3)+    2)
#define LED_PAR_LAYER_C  ((( 1   -1)<<3)+    1)

//                          SR    ignore    Pin
#define LED_BEAT         ((( 1   -1)<<3)+    0)

//                          SR    ignore    Pin
#define LED_EDIT         ((( 2   -1)<<3)+    7)
#define LED_MUTE         ((( 2   -1)<<3)+    6)
#define LED_PATTERN      ((( 2   -1)<<3)+    5)
#define LED_SONG         ((( 2   -1)<<3)+    4)

//                          SR    ignore    Pin
#define LED_SOLO         ((( 2   -1)<<3)+    3)
#define LED_FAST         ((( 2   -1)<<3)+    2)
#define LED_ALL          ((( 2   -1)<<3)+    1)

//                          SR    ignore    Pin
#define LED_GROUP1       (((11   -1)<<3)+    7) // OPTIONAL! see CHANGELOG.txt
#define LED_GROUP2       (((11   -1)<<3)+    5) // assigned to pin 2 due to DUO LED
#define LED_GROUP3       (((11   -1)<<3)+    3) // assigned to pin 4 due to DUO LED
#define LED_GROUP4       (((11   -1)<<3)+    1) // assigned to pin 6 due to DUO LED

//                          SR    ignore    Pin
#define LED_TRG_LAYER_A  (((12   -1)<<3)+    7) // OPTIONAL! see CHANGELOG.txt
#define LED_TRG_LAYER_B  (((12   -1)<<3)+    6)
#define LED_TRG_LAYER_C  (((12   -1)<<3)+    5)

//                          SR    ignore    Pin
#define LED_PLAY         (((12   -1)<<3)+    4) // OPTIONAL! see CHANGELOG.txt
#define LED_STOP         (((12   -1)<<3)+    3)
#define LED_PAUSE        (((12   -1)<<3)+    2)

//                          SR    ignore    Pin
#define LED_STEP_1_16    (((12   -1)<<3)+    1) // OPTIONAL! see CHANGELOG.txt
#define LED_STEP_17_32   (((12   -1)<<3)+    0)




//                          SR    ignore    Pin
#define BUTTON_LEFT      (((1    -1)<<3)+    0)
#define BUTTON_RIGHT     (((1    -1)<<3)+    1)

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

#define BUTTON_GROUP1    (((13   -1)<<3)+    0)
#define BUTTON_GROUP2    (((13   -1)<<3)+    1)
#define BUTTON_GROUP3    (((13   -1)<<3)+    2)
#define BUTTON_GROUP4    (((13   -1)<<3)+    3)

#define BUTTON_TRG_LAYER_A (((13   -1)<<3)+    4)
#define BUTTON_TRG_LAYER_B (((13   -1)<<3)+    5)
#define BUTTON_TRG_LAYER_C (((13   -1)<<3)+    6)

#define BUTTON_STEP_VIEW (((13   -1)<<3)+    7)

#define BUTTON_TAP_TEMPO (((14   -1)<<3)+    0)


#endif /* _SRIO_MAPPING_H */
