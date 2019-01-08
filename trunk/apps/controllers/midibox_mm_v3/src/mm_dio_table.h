// $Id$
/*
 * Header File for IO Configuration
 *
 * ==========================================================================
 *
 *  Copyright (C) 2009 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

// export the tables
extern const u8 mm_dio_table_layer0[2*128];
extern const u8 mm_dio_table_layer1[2*128];
extern const u8 mm_dio_table_layer2[2*128];

// ==========================================================================
//  command IDs which are provided by the Motormix protocol
//  don't change the numbers here!
//  use the definitions in the IO table to assign functions to buttons and LEDs
//  --> mm_dio_table.c
// ==========================================================================

//				        //  B
// Button/LED indicates if the host	//  U
// application supports the event	//  T
// for input, output or both		//  T  L
//					//  O  E
//					//  N  D
//					//  ----
#define ID_FADER_TOUCH_CHN1	0x00	//  o  -  
#define ID_FADER_TOUCH_CHN2	0x01	//  o  -
#define ID_FADER_TOUCH_CHN3	0x02	//  o  -
#define ID_FADER_TOUCH_CHN4	0x03	//  o  -
#define ID_FADER_TOUCH_CHN5	0x04	//  o  -
#define ID_FADER_TOUCH_CHN6	0x05	//  o  -
#define ID_FADER_TOUCH_CHN7	0x06	//  o  -
#define ID_FADER_TOUCH_CHN8	0x07	//  o  -

#define ID_SELECT_CHN1		0x08	//  o  o
#define ID_SELECT_CHN2		0x09	//  o  o
#define ID_SELECT_CHN3		0x0a	//  o  o
#define ID_SELECT_CHN4		0x0b	//  o  o
#define ID_SELECT_CHN5		0x0c	//  o  o
#define ID_SELECT_CHN6		0x0d	//  o  o
#define ID_SELECT_CHN7		0x0e	//  o  o
#define ID_SELECT_CHN8		0x0f	//  o  o

#define ID_MUTE_CHN1		0x10	//  o  o
#define ID_MUTE_CHN2		0x11	//  o  o
#define ID_MUTE_CHN3		0x12	//  o  o
#define ID_MUTE_CHN4		0x13	//  o  o
#define ID_MUTE_CHN5		0x14	//  o  o
#define ID_MUTE_CHN6		0x15	//  o  o
#define ID_MUTE_CHN7		0x16	//  o  o
#define ID_MUTE_CHN8		0x17	//  o  o

#define ID_SOLO_CHN1		0x18	//  o  o
#define ID_SOLO_CHN2		0x19	//  o  o
#define ID_SOLO_CHN3		0x1a	//  o  o
#define ID_SOLO_CHN4		0x1b	//  o  o
#define ID_SOLO_CHN5		0x1c	//  o  o
#define ID_SOLO_CHN6		0x1d	//  o  o
#define ID_SOLO_CHN7		0x1e	//  o  o
#define ID_SOLO_CHN8		0x1f	//  o  o

#define ID_MULTI_CHN1		0x20	//  o  -
#define ID_MULTI_CHN2		0x21	//  o  -
#define ID_MULTI_CHN3		0x22	//  o  -
#define ID_MULTI_CHN4		0x23	//  o  -
#define ID_MULTI_CHN5		0x24	//  o  -
#define ID_MULTI_CHN6		0x25	//  o  -
#define ID_MULTI_CHN7		0x26	//  o  -
#define ID_MULTI_CHN8		0x27	//  o  -

#define ID_REC_RDY_CHN1		0x28	//  o  o
#define ID_REC_RDY_CHN2		0x29	//  o  o
#define ID_REC_RDY_CHN3		0x2a	//  o  o
#define ID_REC_RDY_CHN4		0x2b	//  o  o
#define ID_REC_RDY_CHN5		0x2c	//  o  o
#define ID_REC_RDY_CHN6		0x2d	//  o  o
#define ID_REC_RDY_CHN7		0x2e	//  o  o
#define ID_REC_RDY_CHN8		0x2f	//  o  o

#define ID_SHIFT		0x30	//  o  o -- Cubase: Shift
#define ID_UNDO_x_DISK		0x31	//  o  o -- Cubase: L1 / L1+Shift
#define ID_DEFAULT_x_BYPASS	0x32	//  o  o -- Cubase: L2 / L2+Shift
#define ID_ALL_x_FINE		0x33	//  o  o -- Cubase: L3 / L3+Shift
#define ID_WINDOW_x_TOOLS	0x34	//  o  o -- Cubase: L4 / L4+Shift
#define ID_PLUGIN_x_COMPARE	0x35	//  o  o -- Cubase: L5 / L5+Shift
#define ID_SUSPEND_x_CREATE	0x36	//  o  o -- Cubase: L6 / L6+Shift
#define ID_AUTOENBL_x_MODE	0x37	//  o  o -- Cubase: L7 / L7+Shift

#define ID_ESCAPE		0x38	//  o  o
#define ID_ENTER_x_UTILITY	0x39	//  o  o -- Cubase: R1 / R1+Shift
#define ID_LAST_x_ASSIGN	0x3a	//  o  o -- Cubase: R2 / R2+Shift
#define ID_NEXT_x_CONFIGURE	0x3b	//  o  o -- Cubase: R3 / R3+Shift
#define ID_REWIND_x_STATUS	0x3c	//  o  o -- Cubase: R4 / R4+Shift
#define ID_FFWD_x_MONITOR	0x3d	//  o  o -- Cubase: R5 / R5+Shift
#define ID_STOP_x_LOCATE	0x3e	//  o  o -- Cubase: R6 / R6+Shift
#define ID_PLAY_x_TRANSPORT	0x3f	//  o  o -- Cubase: R7 / R7+Shift

#define ID_LEFT_ARROW		0x40	//  o  -
#define ID_RIGHT_ARROW		0x41	//  o  -
#define ID_BANK			0x42	//  o  o
#define ID_GROUP		0x43	//  o  o
#define ID_RECRDY_x_FUNCTA	0x44	//  o  o
#define ID_WRITE_x_FUNCTB	0x45	//  o  o
#define ID_OTHER_x_FUNCTC	0x46	//  o  o
#define ID_FUNCTA_LED		0x47	//  -  o
#define ID_FUNCTB_LED		0x48	//  -  o
#define ID_FUNCTC_LED		0x49	//  -  o

#define ID_FX_BYPASS_x_EFFECT1	0x50	//  o  o
#define ID_SEND_MUTE_x_EFFECT2	0x51	//  o  o
#define ID_PRE_POST_x_EFFECT3	0x52	//  o  o
#define ID_SELECT_x_EFFECT4	0x53	//  o  o
#define ID_EFFECT1_LED		0x54	//  -  o
#define ID_EFFECT2_LED		0x55	//  -  o
#define ID_EFFECT3_LED		0x56	//  -  o
#define ID_EFFECT4_LED		0x57	//  -  o

#define ID_BYPASS_LED_CHN1	0x20	//  -  o	  -- (same ID like multi buttons, but different function)
#define ID_BYPASS_LED_CHN2	0x21	//  -  o
#define ID_BYPASS_LED_CHN3	0x22	//  -  o
#define ID_BYPASS_LED_CHN4	0x23	//  -  o
#define ID_BYPASS_LED_CHN5	0x24	//  -  o
#define ID_BYPASS_LED_CHN6	0x25	//  -  o
#define ID_BYPASS_LED_CHN7	0x26	//  -  o
#define ID_BYPASS_LED_CHN8	0x27	//  -  o


// ==========================================================================
//  command IDs which are provided by the MIDIbox MM emulation
//  use the definitions in the IO table to assign MBMM specific functions
//  to buttons and LEDs
// ==========================================================================


#define ID_MBMM_SWITCH_LAYER0	0x90	// switch to layer 0 (like radiobutton)
#define ID_MBMM_SWITCH_LAYER1	0x91	// switch to layer 1 (like radiobutton)
#define ID_MBMM_SWITCH_LAYER2	0x92	// switch to layer 1 (like radiobutton)
#define ID_MBMM_TOGGLE_LAYER1	0x93	// like a "caps lock" key
#define ID_MBMM_TOGGLE_LAYER2	0x94	// like a "caps lock" key
#define ID_MBMM_HOLD_LAYER1	0x95	// like a common "shift" key
#define ID_MBMM_HOLD_LAYER2	0x96	// like a common "shift" key

#define ID_MBMM_SWITCH_MM	0x97	// switch to Motormix Emulation (like radiobutton)
#define ID_MBMM_SWITCH_GPC	0x98	// switch to General Purpose Controller mode (like radiobutton)
#define ID_MBMM_TOGGLE_GPC	0x99	// like a "caps lock" key
#define ID_MBMM_HOLD_GPC	0x9a	// like a common "shift" key

#define ID_MBMM_SWITCH_LEDMETER0 0xa0	// ledrings are working as normal
#define ID_MBMM_SWITCH_LEDMETER1 0xa1	// ledrings are working as meters
#define ID_MBMM_TOGGLE_LEDMETER	 0xa2	// like a "caps lock" key
#define ID_MBMM_HOLD_LEDMETER	 0xa3	// like a common "shift" key

#define	ID_IGNORE		0xff	// use this ID to ignore a function
