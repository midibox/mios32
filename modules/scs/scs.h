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

/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////

// optionally these 8 pins can be re-assigned
// note that you could also assign them to global variables for soft-configuration!
#ifndef SCS_PIN_ENC_MENU_A
#define SCS_PIN_ENC_MENU_A 0
#endif

#ifndef SCS_PIN_ENC_MENU_B
#define SCS_PIN_ENC_MENU_B 1
#endif

#ifndef SCS_PIN_MENU
#define SCS_PIN_MENU       2
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
#define SCS_PIN_SOFT5      7
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


// encoder id which is used for MIOS32_ENC
#ifndef SCS_ENC_MENU_ID
#define SCS_ENC_MENU_ID 0
#endif

// the encoder type (see mios32_enc.h for available types)
#ifndef SCS_ENC_MENU_TYPE
#define SCS_ENC_MENU_TYPE DETENTED2
#endif

// number of menu items which are displayed on screen
#ifndef SCS_NUM_MENU_ITEMS
#define SCS_NUM_MENU_ITEMS 5
#endif

// width of an item (4 by default, so that 5 items can be output on a 2x20 LCD)
#ifndef SCS_MENU_ITEM_WIDTH
#define SCS_MENU_ITEM_WIDTH 4
#endif

// maximum width of a temporary message
#ifndef SCS_MSG_MAX_CHAR
#define SCS_MSG_MAX_CHAR 14
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
#define SCS_PAGE(name, items) { name, (scs_menu_item_t *)items, sizeof(items)/sizeof(scs_menu_item_t) }
#define SCS_ITEM(name, id, maxValue, setFunct, getFunct, selectFunct, stringFunct, stringFullFunct) { name, id, maxValue, setFunct, getFunct, selectFunct, stringFunct, stringFullFunct }

/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////

typedef enum {
  SCS_MENU_CHOICE,
  SCS_MENU_PARAMETERS,
} scs_menu_type_t;

typedef struct scs_menu_item_t {
  char name[5];
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
  scs_menu_item_t  *page;
  u8               numItems;
} scs_menu_page_t;


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
extern s32 SCS_InstallMainPageStringHook(s32 (*stringFunct)(char *line1, char *line2));
extern s32 SCS_InstallPageSelectStringHook(s32 (*stringFunct)(char *line1));
extern s32 SCS_InstallEncMainPageHook(s32 (*encFunct)(s32 incrementer));
extern s32 SCS_InstallButtonMainPageHook(s32 (*buttonFunct)(u8 softButton));

extern s32 SCS_DisplayUpdateRequest(void);

extern s32 SCS_Msg(scs_msg_type_t msgType, u16 delay, char *line1, char *line2);
extern s32 SCS_MsgStop(void);

extern s32 SCS_InstallDelayedActionCallback(void *callback, u16 delay_mS, u32 parameter);
extern s32 SCS_UnInstallDelayedActionCallback(void *callback);

extern s32 SCS_InstallEditStringCallback(void *callback, char *actionString, char *initialString, u8 maxChars);

/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////


#endif /* _SCS_H */
