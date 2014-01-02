// $Id$
/*
 * Header file for Standard Control Surface Handler
 *
 * ==========================================================================
 *
 *  Copyright (C) 2011 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _SCS_H
#define _SCS_H

#ifdef __cplusplus
extern "C" {
#endif

/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////

// optionally these 8 pins can be re-assigned
// note that you could also assign them to global variables for soft-configuration!
// Encoder pins can be disabled by setting value 255 (use SCS_PIN_DEC/INC in this case)
#ifndef SCS_PIN_ENC_MENU_A
#define SCS_PIN_ENC_MENU_A 0
#endif

#ifndef SCS_PIN_ENC_MENU_B
#define SCS_PIN_ENC_MENU_B 1
#endif

#ifndef SCS_PIN_EXIT
#define SCS_PIN_EXIT       2
#endif

#ifndef SCS_PIN_SOFT1
#define SCS_PIN_SOFT1      3
#endif

#ifndef SCS_PIN_SOFT2
#define SCS_PIN_SOFT2      4
#endif

#ifndef SCS_PIN_SOFT3
#define SCS_PIN_SOFT3      5
#endif

#ifndef SCS_PIN_SOFT4
#define SCS_PIN_SOFT4      6
#endif

#ifndef SCS_PIN_SOFT5
#define SCS_PIN_SOFT5      7 // optional if SCS_NUM_MENU_ITEMS >= 5
#endif

#ifndef SCS_PIN_SOFT6
#define SCS_PIN_SOFT6      8 // optional if SCS_NUM_MENU_ITEMS >= 6
#endif

#ifndef SCS_PIN_SOFT7
#define SCS_PIN_SOFT7      9 // optional if SCS_NUM_MENU_ITEMS >= 7
#endif

#ifndef SCS_PIN_SOFT8
#define SCS_PIN_SOFT8      10 // optional if SCS_NUM_MENU_ITEMS >= 8
#endif

#ifndef SCS_PIN_SOFT9
#define SCS_PIN_SOFT9      11 // optional if SCS_NUM_MENU_ITEMS >= 9
#endif

#ifndef SCS_PIN_SOFT10
#define SCS_PIN_SOFT10     12 // optional if SCS_NUM_MENU_ITEMS >= 10
#endif

// if set to 1, the menu handler doesn't require a soft button
// instead, items are selected with the rotary encoder, and the selection is
// confirmed with a "SELECT" button (button connected to SCS_PIN_SOFT1)
// The remaining SOFT buttons have no function!
#ifndef SCS_MENU_NO_SOFT_BUTTON_MODE
#define SCS_MENU_NO_SOFT_BUTTON_MODE 0
#endif

// currently only used by MIDIbox NG:
// the EXIT button enters the menu if the SCS shows the main page
// this measure is required, since soft buttons are assigned to alternative functions in main page
#ifndef SCS_MENU_ENTERED_VIA_EXIT_BUTTON
#define SCS_MENU_ENTERED_VIA_EXIT_BUTTON 0
#endif

// Optional Inc/Dec button (e.g. as encoder replacement)
// it's save to assign them to the same pins like the encoder (SCS_PIN_ENC_MENU_A and SCS_PIN_ENC_MENU_B)
// in order to use the DEC/INC pins, either disable the encoder by assigning SCS_PIN_ENC to invalid values
// (e.g. 255), or set SCS_PIN_* to free pins
//
// the optional DEC button
#ifndef SCS_PIN_DEC
#define SCS_PIN_DEC 0
#endif
// the optional INC button
#ifndef SCS_PIN_INC
#define SCS_PIN_INC 1
#endif


// encoder id which is used for MIOS32_ENC
#ifndef SCS_ENC_MENU_ID
#define SCS_ENC_MENU_ID 0
#endif

// the encoder type (see mios32_enc.h for available types)
#ifndef SCS_ENC_MENU_TYPE
#define SCS_ENC_MENU_TYPE DETENTED2
#endif

// width of an item (5 by default, so that 4 items can be output on a 2x20 LCD)
#ifndef SCS_MENU_ITEM_WIDTH
#define SCS_MENU_ITEM_WIDTH 5
#endif

// number of menu items which are displayed on screen
// each item allocates 5x2 characters
// new: derived from mios32_lcd_parameters to give the user control over LCD width
#ifndef SCS_NUM_MENU_ITEMS
#if 0
// TK: theoretically this is possible, but how to handle the additional soft buttons if J10 only provides 8 inputs?
# define SCS_NUM_MENU_ITEMS ((MIOS32_LCD_TypeIsGLCD() ? ((mios32_lcd_parameters.width*mios32_lcd_parameters.num_x) / 5) : mios32_lcd_parameters.width) / SCS_MENU_ITEM_WIDTH)
#else
# define SCS_NUM_MENU_ITEMS 4
#endif
#endif

// threshold for automatic toggle mode
// if the maximum value of an item is >0, and <=SCS_MENU_ITEM_TOGGLE_THRESHOLD is
// selected with a soft button, the item value will be immediately incremented
// and not selected
#ifndef SCS_MENU_ITEM_TOGGLE_THRESHOLD
#define SCS_MENU_ITEM_TOGGLE_THRESHOLD 4
#endif

// maximum width of a temporary message
#ifndef SCS_MSG_MAX_CHAR
#define SCS_MSG_MAX_CHAR 20
#endif

// Debounce counter reload value (in mS)
// Allowed values 0..255 - 0 turns off debouncing
#ifndef SCS_BUTTON_DEBOUNCE_RELOAD
#define SCS_BUTTON_DEBOUNCE_RELOAD 20
#endif


/////////////////////////////////////////////////////////////////////////////
// Help Macros
/////////////////////////////////////////////////////////////////////////////

#define SCS_INSTALL_ROOT(rootTable) SCS_InstallRoot((scs_menu_page_t *)rootTable, sizeof(rootTable)/sizeof(scs_menu_page_t))
#define SCS_PAGE(name, items) { name, (scs_menu_item_t *)items, 0, sizeof(items)/sizeof(scs_menu_item_t) }
#define SCS_SUBPAGE(name, pages) { name, (scs_menu_page_t *)pages, 1, sizeof(pages)/sizeof(scs_menu_page_t) }
#define SCS_ITEM(name, id, maxValue, setFunct, getFunct, selectFunct, stringFunct, stringFullFunct) { name, id, maxValue, setFunct, getFunct, selectFunct, stringFunct, stringFullFunct }

/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////

typedef enum {
  SCS_MENU_CHOICE,
  SCS_MENU_PARAMETERS,
} scs_menu_type_t;

typedef struct scs_menu_item_t {
  char name[6];
  u32  ix;
  u16  maxValue;
  u16  (*getFunct)(u32 ix);
  void (*setFunct)(u32 ix, u16 value);
  u16  (*selectFunct)(u32 ix, u16 value);
  void (*stringFunct)(u32 ix, u16 value, char *label);
  void (*stringFullFunct)(u32 ix, u16 value, char *line1, char *line2);
} scs_menu_item_t;

typedef struct scs_menu_page_t {
  char             name[SCS_MENU_ITEM_WIDTH+1];
  void             *page; // either scs_menu_item_t or scs_menu_page_t
  u8               isSubPage;
  u8               numItems;
} scs_menu_page_t;


typedef enum {
  SCS_MENU_STATE_MAINPAGE,     // we are in the main page
  SCS_MENU_STATE_SELECT_PAGE,  // we select a page
  SCS_MENU_STATE_INSIDE_PAGE,  // we are in a page
  SCS_MENU_STATE_EDIT_ITEM,    // we edit an item in the page
  SCS_MENU_STATE_EDIT_STRING,  // we edit a string in the page
  SCS_MENU_STATE_EDIT_IP,      // we edit an IP in the page
  SCS_MENU_STATE_EDIT_BROWSER, // dynamic browser callbacks (e.g. to scroll through a directory or file list)
} scs_menu_state_t;


typedef enum {
  SCS_MSG_L,
  SCS_MSG_R,
  SCS_MSG_ERROR_L,
  SCS_MSG_ERROR_R,
  SCS_MSG_DELAYED_ACTION_L,
  SCS_MSG_DELAYED_ACTION_R
} scs_msg_type_t;


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 SCS_Init(u32 mode);

extern s32 SCS_NumMenuItemsGet(void);
extern s32 SCS_NumMenuItemsSet(u8 num_items);

extern s32 SCS_PinSet(u8 pin, u8 depressed);
extern s32 SCS_PinGet(u8 pin);
extern s32 SCS_AllPinsSet(u16 new_state);
extern s32 SCS_AllPinsGet(void);

extern s32 SCS_EncButtonUpdate_Tick(void);
extern s32 SCS_Tick(void);

extern s32 SCS_ENC_MENU_NotifyChange(s32 incrementer);
extern s32 SCS_ENC_MENU_AutoSpeedSet(u16 maxValue);

extern s32 SCS_DIN_NotifyToggle(u8 pin, u8 depressed);

extern s32 SCS_InstallRoot(scs_menu_page_t *rootTable, u8 numItems);
extern s32 SCS_InstallDisplayHook(s32 (*stringFunct)(char *line1, char *line2));
extern s32 SCS_InstallEncHook(s32 (*encFunct)(s32 incrementer));
extern s32 SCS_InstallButtonHook(s32 (*buttonFunct)(u8 scsButton, u8 depressed));

extern s32 SCS_DisplayUpdateRequest(void);
extern s32 SCS_DisplayUpdateInMainPage(u8 enable);

extern scs_menu_state_t SCS_MenuStateGet(void);
extern scs_menu_item_t *SCS_MenuPageGet(void);

extern s32 SCS_Msg(scs_msg_type_t msgType, u16 delay, char *line1, char *line2);
extern s32 SCS_MsgStop(void);

extern s32 SCS_InstallDelayedActionCallback(void (*callback)(u32 parameter), u16 delay_mS, u32 parameter);
extern s32 SCS_UnInstallDelayedActionCallback(void (*callback)(u32 parameter));

extern s32 SCS_InstallEditStringCallback(void (*selectCallback)(char *newString), char *actionString, char *initialString, u8 maxChars);
extern s32 SCS_InstallEditIpCallback(void (*selectCallback)(u32 newIp), char *headerString, u32 initialIp);
extern s32 SCS_InstallEditBrowserCallback(void (*selectCallback)(char *newString), u8 (*getListCallback)(u8 offset, char *line), char *actionString, u8 itemWidth, u8 itemsPerPage);

/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
}
#endif

#endif /* _SCS_H */
