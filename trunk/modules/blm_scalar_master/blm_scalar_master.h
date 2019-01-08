// $Id$
/*
 * Header file for BLM_SCALAR_MASTER driver
 *
 * ==========================================================================
 *
 *  Copyright (C) 2010 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _BLM_SCALAR_MASTER_H
#define _BLM_SCALAR_MASTER_H

/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////

// don't touch these defines
// instead, overrule them from external during gcc is called (e.g. ("-DBLM_DOUT_L1=12")
// or write the defines into your mios32_config.h file
#include <mios32_config.h>

// for 16x16 LEDs
// rows should match with number of tracks
#ifndef BLM_SCALAR_MASTER_NUM_ROWS
#define BLM_SCALAR_MASTER_NUM_ROWS 16
#endif

// columns should match with bitwidth of seq_blm_led* variables
// Note: previously we supported wide screen displays (e.g. with 16, 32, 64, 128 LEDs per row)
// this has been removed to simplify and speed up the routines!
// However, a 32 LED mode is still feasible with some changes... have fun! ;)
#ifndef BLM_SCALAR_MASTER_NUM_COLUMNS
#define BLM_SCALAR_MASTER_NUM_COLUMNS 16
#endif

// enable this switch if the application supports OSC (based on osc_server module)
#ifndef BLM_SCALAR_MASTER_OSC_SUPPORT
#define BLM_SCALAR_MASTER_OSC_SUPPORT 0
#endif


// it's recommended to assign the MIDIOUT mutex used by the application in mios32_config.h
#ifndef BLM_SCALAR_MASTER_MUTEX_MIDIOUT_TAKE
#define BLM_SCALAR_MASTER_MUTEX_MIDIOUT_TAKE { }
#endif
#ifndef BLM_SCALAR_MASTER_MUTEX_MIDIOUT_GIVE
#define BLM_SCALAR_MASTER_MUTEX_MIDIOUT_GIVE { }
#endif



/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////

// command states
typedef enum {
  BLM_SCALAR_MASTER_CONNECTION_STATE_IDLE = 0,
  BLM_SCALAR_MASTER_CONNECTION_STATE_SYSEX,
  BLM_SCALAR_MASTER_CONNECTION_STATE_LEMUR
} blm_scalar_master_connection_state_t;


// extra button types
typedef enum {
  BLM_SCALAR_MASTER_ELEMENT_GRID = 0,
  BLM_SCALAR_MASTER_ELEMENT_EXTRA_ROW,
  BLM_SCALAR_MASTER_ELEMENT_EXTRA_COLUMN,
  BLM_SCALAR_MASTER_ELEMENT_SHIFT
} blm_scalar_master_element_t;

// colour encoding
// don't change the value, will be used for logical combination!
typedef enum {
  BLM_SCALAR_MASTER_COLOUR_OFF = 0,
  BLM_SCALAR_MASTER_COLOUR_GREEN = 1, 
  BLM_SCALAR_MASTER_COLOUR_RED = 2,
  BLM_SCALAR_MASTER_COLOUR_YELLOW = 3,
} blm_scalar_master_colour_t;


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 BLM_SCALAR_MASTER_Init(u32 mode);

extern s32 BLM_SCALAR_MASTER_MIDI_PortSet(u8 blm, mios32_midi_port_t port);
extern mios32_midi_port_t BLM_SCALAR_MASTER_MIDI_PortGet(u8 blm);
extern blm_scalar_master_connection_state_t BLM_SCALAR_MASTER_ConnectionStateGet(u8 blm);

extern s32 BLM_SCALAR_MASTER_ButtonCallback_Init(s32 (*button_callback_func)(u8 blm, blm_scalar_master_element_t element_id, u8 button_x, u8 button_y, u8 button_depressed));
extern s32 BLM_SCALAR_MASTER_FaderCallback_Init(s32 (*fader_callback_func)(u8 blm, u8 fader, u8 value));

extern s32 BLM_SCALAR_MASTER_LED_Set(u8 blm, blm_scalar_master_element_t element_id, u8 led_x, u8 led_y, blm_scalar_master_colour_t colour);
extern blm_scalar_master_colour_t BLM_SCALAR_MASTER_LED_Get(u8 blm, blm_scalar_master_element_t element_id, u8 led_x, u8 led_y);

extern s32 BLM_SCALAR_MASTER_RotateViewSet(u8 blm, u8 rotate_view);
extern s32 BLM_SCALAR_MASTER_RotateViewGet(u8 blm);
extern s32 BLM_SCALAR_MASTER_RowOffsetSet(u8 blm, u8 row_offset);
extern s32 BLM_SCALAR_MASTER_RowOffsetGet(u8 blm);

extern s32 BLM_SCALAR_MASTER_NumColumnsGet(u8 blm);
extern s32 BLM_SCALAR_MASTER_NumRowsGet(u8 blm);
extern s32 BLM_SCALAR_MASTER_NumColoursGet(u8 blm);

extern s32 BLM_SCALAR_MASTER_TimeoutCtrSet(u8 blm, u16 ctr);
extern s32 BLM_SCALAR_MASTER_TimeoutCtrGet(u8 blm);

extern s32 BLM_SCALAR_MASTER_ForceDisplayUpdate(u8 blm);

extern s32 BLM_SCALAR_MASTER_SendRequest(u8 blm, u8 req);

extern s32 BLM_SCALAR_MASTER_MIDI_Receive(mios32_midi_port_t port, mios32_midi_package_t midi_package);
extern s32 BLM_SCALAR_MASTER_SYSEX_Parser(mios32_midi_port_t port, u8 midi_in);
extern s32 BLM_SCALAR_MASTER_MIDI_TimeOut(mios32_midi_port_t port);

extern s32 BLM_SCALAR_MASTER_Periodic_mS(void);


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////


// for direct access
u16 blm_scalar_master_leds_green[BLM_SCALAR_MASTER_NUM_ROWS];
u16 blm_scalar_master_leds_red[BLM_SCALAR_MASTER_NUM_ROWS];

u16 blm_scalar_master_leds_extracolumn_green;
u16 blm_scalar_master_leds_extracolumn_red;
u16 blm_scalar_master_leds_extracolumn_shift_green;
u16 blm_scalar_master_leds_extracolumn_shift_red;
u16 blm_scalar_master_leds_extrarow_green;
u16 blm_scalar_master_leds_extrarow_red;
u8  blm_scalar_master_leds_extra_green;
u8  blm_scalar_master_leds_extra_red;


#endif /* _BLM_SCALAR_MASTER_H */
