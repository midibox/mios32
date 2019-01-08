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
extern const u8 lc_dio_table_layer0[2*128];
extern const u8 lc_dio_table_layer1[2*128];
extern const u8 lc_dio_table_layer2[2*128];

// ==========================================================================
//  command IDs which are provided by the Motormix protocol
//  don't change the numbers here!
//  use the definitions in the IO table to assign functions to buttons and LEDs
//  --> lc_dio_table.c
// ==========================================================================

//				        //  B
// Button/LED indicates if the host	//  U
// application supports the event	//  T
// for input, output or both		//  T  L
//					//  O  E
//					//  N  D
//					//  ----
#define ID_REC_RDY_CHN1		0x00	//  o  o
#define ID_REC_RDY_CHN2		0x01	//  o  o
#define ID_REC_RDY_CHN3		0x02	//  o  o
#define ID_REC_RDY_CHN4		0x03	//  o  o
#define ID_REC_RDY_CHN5		0x04	//  o  o
#define ID_REC_RDY_CHN6		0x05	//  o  o
#define ID_REC_RDY_CHN7		0x06	//  o  o
#define ID_REC_RDY_CHN8		0x07	//  o  o

#define ID_SOLO_CHN1		0x08	//  o  o
#define ID_SOLO_CHN2		0x09	//  o  o
#define ID_SOLO_CHN3		0x0a	//  o  o
#define ID_SOLO_CHN4		0x0b	//  o  o
#define ID_SOLO_CHN5		0x0c	//  o  o
#define ID_SOLO_CHN6		0x0d	//  o  o
#define ID_SOLO_CHN7		0x0e	//  o  o
#define ID_SOLO_CHN8		0x0f	//  o  o

#define ID_MUTE_CHN1		0x10	//  o  o
#define ID_MUTE_CHN2		0x11	//  o  o
#define ID_MUTE_CHN3		0x12	//  o  o
#define ID_MUTE_CHN4		0x13	//  o  o
#define ID_MUTE_CHN5		0x14	//  o  o
#define ID_MUTE_CHN6		0x15	//  o  o
#define ID_MUTE_CHN7		0x16	//  o  o
#define ID_MUTE_CHN8		0x17	//  o  o

#define ID_SELECT_CHN1		0x18	//  o  o
#define ID_SELECT_CHN2		0x19	//  o  o
#define ID_SELECT_CHN3		0x1a	//  o  o
#define ID_SELECT_CHN4		0x1b	//  o  o
#define ID_SELECT_CHN5		0x1c	//  o  o
#define ID_SELECT_CHN6		0x1d	//  o  o
#define ID_SELECT_CHN7		0x1e	//  o  o
#define ID_SELECT_CHN8		0x1f	//  o  o

#define ID_VPOT_SELECT_CHN1	0x20	//  o  -
#define ID_VPOT_SELECT_CHN2	0x21	//  o  -
#define ID_VPOT_SELECT_CHN3	0x22	//  o  -
#define ID_VPOT_SELECT_CHN4	0x23	//  o  -
#define ID_VPOT_SELECT_CHN5	0x24	//  o  -
#define ID_VPOT_SELECT_CHN6	0x25	//  o  -
#define ID_VPOT_SELECT_CHN7	0x26	//  o  -
#define ID_VPOT_SELECT_CHN8	0x27	//  o  -

#define ID_ASSIGN_TRACK		0x28	//  o  o
#define ID_ASSIGN_SEND		0x29	//  o  o
#define ID_ASSIGN_PAN_SURROUND	0x2a	//  o  o
#define ID_ASSIGN_PLUG_IN	0x2b	//  o  o
#define ID_ASSIGN_EQ		0x2c	//  o  o
#define ID_ASSIGN_INSTRUMENT	0x2d	//  o  o

#define ID_BANK_LEFT		0x2e	//  o  -
#define ID_BANK_RIGHT		0x2f	//  o  -
#define ID_BANK_CHANNEL_LEFT	0x30	//  o  -
#define ID_BANK_CHANNEL_RIGHT	0x31	//  o  -

#define	ID_FLIP			0x32	//  o  o
#define ID_GLOBAL_VIEW		0x33	//  o  o
#define ID_NAME_VALUE		0x34	//  o  -
#define ID_SMPTE_BEATS		0x35	//  o  -

#define ID_F1			0x36	//  o  -
#define ID_F2			0x37	//  o  -
#define ID_F3			0x38	//  o  -
#define ID_F4			0x39	//  o  -
#define ID_F5			0x3a	//  o  -
#define ID_F6			0x3b	//  o  -
#define ID_F7			0x3c	//  o  -
#define ID_F8			0x3d	//  o  -

#define ID_GLOBAL_TRACKS	0x3e	//  o  -
#define ID_GLOBAL_INPUTS	0x3f	//  o  -
#define ID_GLOBAL_AUDIO_TRCK	0x40	//  o  -
#define ID_GLOBAL_AUDIO_INSTR	0x41	//  o  -
#define ID_GLOBAL_AUX		0x42	//  o  -
#define ID_GLOBAL_BUSSES	0x43	//  o  -
#define ID_GLOBAL_OUTPUTS	0x44	//  o  -
#define ID_GLOBAL_USER		0x45	//  o  -

#define ID_SHIFT		0x46	//  o  -
#define ID_OPTION		0x47	//  o  -
#define ID_CONTROL		0x48	//  o  -
#define ID_CMD_ALT		0x49	//  o  -

#define ID_AUTOM_READ_OFF	0x4a	//  o  o
#define ID_AUTOM_WRITE		0x4b	//  o  o
#define ID_AUTOM_TRIM		0x4c	//  o  o
#define ID_AUTOM_TOUCH		0x4d	//  o  o
#define ID_AUTOM_LATCH		0x4e	//  o  o

#define ID_GROUP		0x4f	//  o  o

#define ID_UTIL_SAVE		0x50	//  o  o
#define ID_UTIL_UNDO		0x51	//  o  o
#define ID_UTIL_CANCEL		0x52	//  o  -
#define ID_UTIL_ENTER		0x53	//  o  -
	
#define ID_MARKER		0x54	//  o  o
#define ID_NUDGE		0x55	//  o  o
#define ID_CYCLE		0x56	//  o  o
#define ID_DROP			0x57	//  o  o
#define ID_REPLACE		0x58	//  o  o
#define ID_CLICK		0x59	//  o  o
#define ID_SOLO			0x5a	//  o  o
	
#define ID_REWIND		0x5b	//  o  o
#define ID_FAST_FWD		0x5c	//  o  o
#define ID_STOP			0x5d	//  o  o
#define ID_PLAY			0x5e	//  o  o
#define ID_RECORD		0x5f	//  o  o
	
#define ID_CURSOR_UP		0x60	//  o  -
#define ID_CURSOR_DOWN		0x61	//  o  -
#define ID_CURSOR_LEFT		0x62	//  o  -
#define ID_CURSOR_RIGHT		0x63	//  o  -

#define ID_ZOOM			0x64	//  o  o
#define ID_SCRUB		0x65	//  o  o

#define ID_USER_SWITCH_1	0x66	//  o  -
#define ID_USER_SWITCH_2	0x67	//  o  -

#define ID_FADER_TOUCH_CHN1	0x68	//  o  -
#define ID_FADER_TOUCH_CHN2	0x69	//  o  -
#define ID_FADER_TOUCH_CHN3	0x6a	//  o  -
#define ID_FADER_TOUCH_CHN4	0x6b	//  o  -
#define ID_FADER_TOUCH_CHN5	0x6c	//  o  -
#define ID_FADER_TOUCH_CHN6	0x6d	//  o  -
#define ID_FADER_TOUCH_CHN7	0x6e	//  o  -
#define ID_FADER_TOUCH_CHN8	0x6f	//  o  -
#define ID_FADER_TOUCH_MASTER	0x70	//  o  -

#define ID_SMPTE_LED		0x71	//  -  o
#define ID_BEATS_LED		0x72	//  -  o
#define ID_RUDE_SOLO_LIGHT	0x73	//  -  o
#define ID_RELAY_CLICK		0x74	//  -  o


// ==========================================================================
//  command IDs which are provided by the MIDIbox LC emulation
//  use the definitions in the IO table to assign MBLC specific functions
//  to buttons and LEDs
// ==========================================================================

#define ID_MBLC_DISPLAY_PAGE0	0x80	// switch to page 0
#define ID_MBLC_DISPLAY_PAGE1	0x81	// switch to page 1
#define ID_MBLC_DISPLAY_PAGE2	0x82	// switch to page 2
#define ID_MBLC_DISPLAY_PAGE3	0x83	// switch to page 3
#define ID_MBLC_DISPLAY_NEXT	0x84	// switch to next page

#define ID_MBLC_SWITCH_LAYER0	0x90	// switch to layer 0 (like radiobutton)
#define ID_MBLC_SWITCH_LAYER1	0x91	// switch to layer 1 (like radiobutton)
#define ID_MBLC_SWITCH_LAYER2	0x92	// switch to layer 1 (like radiobutton)
#define ID_MBLC_TOGGLE_LAYER1	0x93	// like a "caps lock" key
#define ID_MBLC_TOGGLE_LAYER2	0x94	// like a "caps lock" key
#define ID_MBLC_HOLD_LAYER1	0x95	// like a common "shift" key
#define ID_MBLC_HOLD_LAYER2	0x96	// like a common "shift" key

#define ID_MBLC_SWITCH_LC	0x98	// switch to Logic/Mackie Control Emulation (like radiobutton)
#define ID_MBLC_SWITCH_GPC	0x99	// switch to General Purpose Controller mode (like radiobutton)
#define ID_MBLC_TOGGLE_GPC	0x9a	// like a "caps lock" key
#define ID_MBLC_HOLD_GPC	0x9b	// like a common "shift" key

#define ID_MBLC_SWITCH_LEDMETER0 0xa0	// ledrings are working as normal
#define ID_MBLC_SWITCH_LEDMETER1 0xa1	// ledrings are working as meters
#define ID_MBLC_TOGGLE_LEDMETER	 0xa2	// like a "caps lock" key
#define ID_MBLC_HOLD_LEDMETER	 0xa3	// like a common "shift" key

#define	ID_IGNORE		0xff	// use this ID to ignore a function
