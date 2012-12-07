// $Id$
//! \defgroup SCS
//!
//! This code handles the Standard Control Surface (SCS)
//! with a 2x20 LCD, 5 soft buttons, one rotary encoder and one MENU button
//!
//! Usage example: apps/tutorials/027_scs
//!
//! Optional compile flags which can be added to mios32_config.h
//! 
//! \code
//! // optionally these 8 pins can be re-assigned
//! // note that you could also assign them to global variables for soft-configuration!
//! // Encoder pins can be disabled by setting value 255 (use SCS_PIN_DEC/INC in this case)
//! #define SCS_PIN_ENC_MENU_A 0
//! #define SCS_PIN_ENC_MENU_B 1
//! #define SCS_PIN_EXIT       2
//! #define SCS_PIN_SOFT1      3
//! #define SCS_PIN_SOFT2      4
//! #define SCS_PIN_SOFT3      5
//! #define SCS_PIN_SOFT4      6
//! #define SCS_PIN_SOFT5      7 // optional if SCS_NUM_MENU_ITEMS >= 5
//! #define SCS_PIN_SOFT6      8 // optional if SCS_NUM_MENU_ITEMS >= 6
//! #define SCS_PIN_SOFT7      9 // optional if SCS_NUM_MENU_ITEMS >= 7
//! #define SCS_PIN_SOFT8      10 // optional if SCS_NUM_MENU_ITEMS >= 8
//! #define SCS_PIN_SOFT9      11 // optional if SCS_NUM_MENU_ITEMS >= 9
//! #define SCS_PIN_SOFT10     12 // optional if SCS_NUM_MENU_ITEMS >= 10
//! 
//! // if set to 1, the menu handler doesn't require a soft button
//! // instead, items are selected with the rotary encoder, and the selection is
//! // confirmed with a "SELECT" button (button connected to SCS_PIN_SOFT1)
//! // The remaining SOFT buttons have no function!
//! #define SCS_MENU_NO_SOFT_BUTTON_MODE 0
//!
//! // Optional Inc/Dec button (e.g. as encoder replacement)
//! // it's save to assign them to the same pins like the encoder (SCS_PIN_ENC_MENU_A and SCS_PIN_ENC_MENU_B)
//! // in order to use the DEC/INC pins, either disable the encoder by assigning SCS_PIN_ENC to invalid values
//! // (e.g. 255), or set SCS_PIN_* to free pins
//! //
//! // the optional DEC button
//! #define SCS_PIN_DEC 0
//! // the optional INC button
//! #define SCS_PIN_INC 1
//!
//! // encoder id which is used for MIOS32_ENC
//! #define SCS_ENC_MENU_ID 0
//!
//! // the encoder type (see mios32_enc.h for available types)
//! #define SCS_ENC_MENU_TYPE DETENTED2
//!
//! // number of menu items which are displayed on screen
//! // each item allocates 5x2 characters
//! #define SCS_NUM_MENU_ITEMS 4
//!
//! // width of an item (5 by default, so that 4 items can be output on a 2x20 LCD)
//! #define SCS_MENU_ITEM_WIDTH 5
//!
//! // threshold for automatic toggle mode
//! // if the maximum value of an item is >0, and <=SCS_MENU_ITEM_TOGGLE_THRESHOLD is
//! // selected with a soft button, the item value will be immediately incremented
//! // and not selected
//! #define SCS_MENU_ITEM_TOGGLE_THRESHOLD 4
//!
//! // maximum width of a temporary message
//! #define SCS_MSG_MAX_CHAR 16
//!
//! // Debounce counter reload value (in mS)
//! // Allowed values 0..255 - 0 turns off debouncing
//! #define SCS_BUTTON_DEBOUNCE_RELOAD 20
//! \endcode
//! 
//! \{
/* ==========================================================================
 *
 *  Copyright (C) 2011 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

/////////////////////////////////////////////////////////////////////////////
// Include files
/////////////////////////////////////////////////////////////////////////////

#include <mios32.h>
#include <string.h>

#include "scs.h"
#include "scs_lcd.h"


/////////////////////////////////////////////////////////////////////////////
// Local Definitions
/////////////////////////////////////////////////////////////////////////////

// add some headroom to prevent buffer overwrites
#define SCS_MAX_STR (SCS_LCD_MAX_COLUMNS+50)


/////////////////////////////////////////////////////////////////////////////
// Local types
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Local prototypes
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

static scs_menu_state_t scsMenuState;
static u8 displayUpdateReq;    // using single variable for atomic access
static u8 displayInitReq;      // using single variable for atomic access
static u16 displayCursorCtr;
static u16 displayFlickerSelectionCtr;
static u8 displayLabelOn;      // using single variable for atomic access
static u8 displayCursorOn;     // using single variable for atomic access
static u8 displayCursorPos;    // cursor position
static u8 displayRootOffset;   // offset in root menu
static u8 displayPageOffset;   // offset in page

static void (*scsDelayedActionCallback)(u32 parameter);
static u32 scsDelayedActionParameter;
static u16 scsDelayedActionCtr;

static char scsMsg[2][SCS_MSG_MAX_CHAR+10+1]; // + some "margin" + zero terminator
static u16 scsMsgCtr;
static scs_msg_type_t scsMsgType;

static char scsActionString[SCS_MENU_ITEM_WIDTH+1];
static char scsEditString[SCS_LCD_COLUMNS_PER_DEVICE+1];
static u8 scsEditStringMaxChars;
static u8 scsEditItemsPerPage;
static u8 scsEditNumItems;
static u32 scsEditIp;
static u8 scsEditPos;
static u8 scsEditOffset;
static u8 scsEditPageUpdateReq;
static void (*scsEditStringCallback)(char *newString);
static void (*scsEditIpCallback)(u32 newIp);
static u8 (*scsEditGetListCallback)(u8 offset, char *line);

static u16 scsPinState;
static u16 scsPinStatePrev;
#if SCS_BUTTON_DEBOUNCE_RELOAD
static u8 scsButtonDebounceCtr;
#endif


static scs_menu_page_t *rootTable;
static u8 rootTableNumItems;

static scs_menu_page_t *previousMenuTable;
static scs_menu_page_t *currentMenuTable;
static u8 currentMenuTableNumItems;
static u8 currentMenuTableSelectedPagePos;
static u8 currentMenuTableSelectedItemPos;

//                                             00112233
static const char animation_l_arrows[2*4+1] = "   >>>> ";
//                                             00112233
static const char animation_r_arrows[2*4+1] = "  < << <";
//                                               00112233
static const char animation_l_brackets[2*4+1] = "   )))) ";
//                                               00112233
static const char animation_r_brackets[2*4+1] = "  ( (( (";
//                                            00112233
static const char animation_l_stars[2*4+1] = "   **** ";
//                                            00112233
static const char animation_r_stars[2*4+1] = "  * ** *";


/////////////////////////////////////////////////////////////////////////////
// Local prototypes
/////////////////////////////////////////////////////////////////////////////
static s32 (*scsDisplayHook)(char *line1, char *line2);
static s32 (*scsEncHook)(s32 incrementer);
static s32 (*scsButtonHook)(u8 button, u8 depressed);




/////////////////////////////////////////////////////////////////////////////
//! Initialisation of Standard Control Surface
//! \param[in] mode currently only mode 0 supported
//! \return < 0 if initialisation failed
/////////////////////////////////////////////////////////////////////////////
s32 SCS_Init(u32 mode)
{
  SCS_LCD_Init(mode);
  SCS_LCD_InitSpecialChars(SCS_LCD_CHARSET_Menu, 1);

  scsMenuState = SCS_MENU_STATE_MAINPAGE;

  displayUpdateReq = 1;
  displayInitReq = 1;
  displayCursorCtr = 0;
  displayFlickerSelectionCtr = 0;
  displayLabelOn = 1;
  displayCursorOn = 1;
  displayCursorPos = 0;
  displayRootOffset = 0;
  displayPageOffset = 0;

  scsDelayedActionCallback = NULL;
  scsDelayedActionParameter = 0;
  scsDelayedActionCtr = 0;

  scsEditStringCallback = NULL;
  scsEditGetListCallback = NULL;
  scsActionString[0] = 0;
  scsEditString[0] = 0;
  scsEditStringMaxChars = 0;
  scsEditItemsPerPage = 0;
  scsEditNumItems = 0;
  scsEditPos = 0;
  scsEditOffset = 0;
  scsEditPageUpdateReq = 0;

  scsMsgCtr = 0;
  scsDelayedActionCtr = 0;

  scsPinState = 0xffff;
  scsPinStatePrev = 0xffff;
#if SCS_BUTTON_DEBOUNCE_RELOAD
  scsButtonDebounceCtr = 0;
#endif

  rootTable = NULL;
  rootTableNumItems = 0;

  previousMenuTable = NULL;
  currentMenuTable = NULL;
  currentMenuTableNumItems = 0;
  currentMenuTableSelectedPagePos = 0;

  scsDisplayHook = NULL;
  scsEncHook = NULL;
  scsButtonHook = NULL;

  // configure encoder
  mios32_enc_config_t enc_config = MIOS32_ENC_ConfigGet(SCS_ENC_MENU_ID);
  enc_config.cfg.type = SCS_ENC_MENU_TYPE; // see mios32_enc.h for available types
  enc_config.cfg.sr = 0; // must be 0 if controlled from application
  enc_config.cfg.pos = 0; // doesn't matter if controlled from application
  enc_config.cfg.speed = NORMAL;
  enc_config.cfg.speed_par = 0;
  MIOS32_ENC_ConfigSet(SCS_ENC_MENU_ID, enc_config);

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! Call this function if the SCS is controlled from DIN pins
//! \param[in] pin the pin number (SCS_PIN_ENC_MENU_A .. SCS_PIN_SOFT5)
//! \param[in] depressed the pin value (0 if pressed, != 0 if depressed)
//! \return < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 SCS_PinSet(u8 pin, u8 depressed)
{
  if( pin >= 8 )
    return -1; // invalid pin

  u16 mask = 1 << pin;
  MIOS32_IRQ_Disable(); // should be atomic
  scsPinState = depressed ? (scsPinState | mask) : (scsPinState & ~mask);
  MIOS32_IRQ_Enable();

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
//! Returns the current pin state of a given pin
//! \param[in] pin the pin number (SCS_PIN_ENC_MENU_A .. SCS_PIN_SOFT5)
//! \return < 0 on errors, 0 if pin pressed, > 0 if pin depressed
/////////////////////////////////////////////////////////////////////////////
s32 SCS_PinGet(u8 pin)
{
  if( pin >= 8 )
    return -1; // invalid pin

  u16 mask = 1 << pin;
  return (scsPinState & mask) ? 1 : 0;
}


/////////////////////////////////////////////////////////////////////////////
//! Call this function to set all SCS pins at once
//! \param[in] newState new state of all pins
//! \return < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 SCS_AllPinsSet(u16 newState)
{
  scsPinState = newState;

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
//! Returns the current state of all pins
//! \return the state of all pins
/////////////////////////////////////////////////////////////////////////////
s32 SCS_AllPinsGet(void)
{
  return scsPinState;
}


/////////////////////////////////////////////////////////////////////////////
//! This handler should be called periodically from APP_SRIO_ServicePrepare
//! \return < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 SCS_EncButtonUpdate_Tick(void)
{
#if SCS_PIN_ENC_MENU_A < 255 && SCS_PIN_ENC_MENU_B < 255
  // pass state of encoder to MIOS32_ENC
  u8 encoderState = 0;
  u16 maskEncA = (1 << SCS_PIN_ENC_MENU_A);
  u16 maskEncB = (1 << SCS_PIN_ENC_MENU_B);

  if( scsPinState & maskEncA )
    encoderState |= 1;
  if( scsPinState & maskEncB )
    encoderState |= 2;
  MIOS32_ENC_StateSet(SCS_ENC_MENU_ID, encoderState);

  // ensure that change won't be propagated to DIN handler
  scsPinState &= ~(maskEncA | maskEncB);
#endif

  // no state update required for buttons (done from external)

#if SCS_BUTTON_DEBOUNCE_RELOAD
  // for button debouncing
  if( scsButtonDebounceCtr )
    --scsButtonDebounceCtr;
#endif


  // cursor timer handling is done here
  // it's only used if an item is edited
  if( scsMenuState == SCS_MENU_STATE_EDIT_ITEM ||
#if SCS_MENU_NO_SOFT_BUTTON_MODE
      scsMenuState == SCS_MENU_STATE_SELECT_PAGE ||
      scsMenuState == SCS_MENU_STATE_INSIDE_PAGE ||
#endif
      scsMenuState == SCS_MENU_STATE_EDIT_STRING ||
      scsMenuState == SCS_MENU_STATE_EDIT_IP ||
      scsMenuState == SCS_MENU_STATE_EDIT_BROWSER ) {
    if( displayFlickerSelectionCtr ) { // new selected parameter flickers
      --displayFlickerSelectionCtr;

      displayCursorCtr = 0;
      displayCursorOn = 1;
      u8 prevDisplayLabelOn = displayLabelOn;
      displayLabelOn = (displayFlickerSelectionCtr % 60) >= 30;

      if( displayLabelOn != prevDisplayLabelOn )
	displayUpdateReq = 1;

      if( !displayFlickerSelectionCtr ) {
	displayLabelOn = 1;
	displayUpdateReq = 1;
      }
    } else {
      displayLabelOn = 1;
      if( ++displayCursorCtr == 1 ) { // displayCursorCtr will be set to 0 on any selection - flicker it for ca. 20 mS to mark the selection
	displayCursorOn = 0;
	displayUpdateReq = 1;
      } else if( displayCursorCtr == 20 ) { // set cursor after 20 ticks
	displayCursorOn = 1;
	displayUpdateReq = 1;
      } else if( displayCursorCtr == 820 ) { // clear cursor after 820 ticks
	displayCursorOn = 0;
	displayUpdateReq = 1;
      } else if( displayCursorCtr >= 1000 ) { // reset to 1 (not 0!!!) after 1000 ticks
	displayCursorCtr = 1;
      }
    }
  } else {
    displayFlickerSelectionCtr = 0;
    displayCursorOn = 1;
    // update screen each second (configuration option required?)
    if( ++displayCursorCtr >= 1000 ) {
      displayCursorCtr = 0;
      displayUpdateReq = 1;
    }
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! This function should be called from APP_ENC_NotifyChange when the encoder
//! has been moved
//! \param[in] incrementer the incrementer (+/- 1)
//! \return < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 SCS_ENC_MENU_NotifyChange(s32 incrementer)
{
  if( incrementer == 0 ) // nothing to do...
    return 0;

  // deinstall callback if it was active
  scsDelayedActionCallback = 0;

  // check for overruling
  if( scsEncHook && scsEncHook(incrementer) > 0 ) {
    displayUpdateReq = 1;
    return 1;
  }

  switch( scsMenuState ) {
  case SCS_MENU_STATE_SELECT_PAGE: {
#if SCS_MENU_NO_SOFT_BUTTON_MODE
    // move cursor with encoder
    // if border is reached, move offset as usual
    int newCursorPos = displayCursorPos + incrementer;
    int pageOffsetIncrementer = 0;
    if( newCursorPos < 0 ) {
      newCursorPos = 0;
      pageOffsetIncrementer = -1;
    } else if( newCursorPos >= SCS_NUM_MENU_ITEMS ) {
      newCursorPos = SCS_NUM_MENU_ITEMS-1;
      pageOffsetIncrementer = 1;
    } else if( newCursorPos >= currentMenuTableNumItems ) {
      newCursorPos = currentMenuTableNumItems-1;
      pageOffsetIncrementer = 0; // NOP
    }

    if( displayCursorPos != newCursorPos ) {
      // flicker menu item for 200 mS
      displayFlickerSelectionCtr = 200;
    }
      
    displayCursorPos = newCursorPos;
    incrementer = pageOffsetIncrementer;
#endif
    int newOffset = displayRootOffset + incrementer;
    if( newOffset < 0 )
      newOffset = 0;
    else if( (newOffset+SCS_NUM_MENU_ITEMS) >= currentMenuTableNumItems ) {
      newOffset = currentMenuTableNumItems - SCS_NUM_MENU_ITEMS;
      if( newOffset < 0 )
	newOffset = 0;
    }
    displayRootOffset = newOffset;
  } break;

  case SCS_MENU_STATE_INSIDE_PAGE: {
    if( currentMenuTableSelectedPagePos >= currentMenuTableNumItems )
      return -1; // fail safe
    scs_menu_page_t *selectedPage = (scs_menu_page_t *)&currentMenuTable[currentMenuTableSelectedPagePos];

#if SCS_MENU_NO_SOFT_BUTTON_MODE
    // move cursor with encoder
    // if border is reached, move offset as usual
    int newCursorPos = displayCursorPos + incrementer;
    int pageOffsetIncrementer = 0;
    if( newCursorPos < 0 ) {
      newCursorPos = 0;
      pageOffsetIncrementer = -1;
    } else if( newCursorPos >= SCS_NUM_MENU_ITEMS ) {
      newCursorPos = SCS_NUM_MENU_ITEMS-1;
      pageOffsetIncrementer = 1;
    } else if( newCursorPos >= selectedPage->numItems ) {
      newCursorPos = selectedPage->numItems - 1;
      pageOffsetIncrementer = 0; // NOP
    }

    if( displayCursorPos != newCursorPos ) {
      // flicker menu item for 200 mS
      displayFlickerSelectionCtr = 200;
    }
      
    displayCursorPos = newCursorPos;
    incrementer = pageOffsetIncrementer;
#endif

    int newOffset = displayPageOffset + incrementer;
    if( newOffset < 0 )
      newOffset = 0;
    else if( (newOffset+SCS_NUM_MENU_ITEMS) >= selectedPage->numItems ) {
      newOffset = selectedPage->numItems - SCS_NUM_MENU_ITEMS;
      if( newOffset < 0 )
	newOffset = 0;
    }
    displayPageOffset = newOffset;
  } break;

  case SCS_MENU_STATE_EDIT_ITEM: {
    scs_menu_item_t *pageItem = NULL;
    if( currentMenuTableSelectedPagePos < currentMenuTableNumItems ) {
      scs_menu_item_t *pageItems = (scs_menu_item_t *)currentMenuTable[currentMenuTableSelectedPagePos].page;
      u8 numItems = currentMenuTable[currentMenuTableSelectedPagePos].numItems;
      if( currentMenuTableSelectedItemPos < numItems )
	pageItem = (scs_menu_item_t *)&pageItems[currentMenuTableSelectedItemPos];
    }

    if( pageItem ) {
      int value = pageItem->getFunct(pageItem->ix);
      value += incrementer;
      if( value < 0 )
	value = 0;
      else if( value >= pageItem->maxValue )
	value = pageItem->maxValue;
      pageItem->setFunct(pageItem->ix, value);
    }

    // reset cursor counter, so that parameter is visible
    displayCursorCtr = 0;
    displayUpdateReq = 1;

  } break;

  case SCS_MENU_STATE_EDIT_STRING: {
    char c = scsEditString[scsEditPos];

    if( incrementer > 0 ) {
      if( c < 32 )
	c = 32;
      else {
	c += incrementer;
	if( c >= 128 )
	  c = 127;
      }
    } else {
      if( c >= 128 )
	c = 127;
      else {
	c += incrementer;
	if( c < 32 )
	  c = 32;
      }
    }

    scsEditString[scsEditPos] = (char)c;

    // reset cursor counter, so that parameter is visible
    displayCursorCtr = 0;
    displayUpdateReq = 1;
  } break;

  case SCS_MENU_STATE_EDIT_IP: {
    int value = (scsEditIp >> (8*(3-scsEditPos))) & 0xff;
    value += incrementer;
    if( value < 0 )
      value = 0;
    else if( value >= 255 )
      value = 255;
    scsEditIp &= ~(0xff << (8*(3-scsEditPos)));
    scsEditIp |= (value << (8*(3-scsEditPos)));

    // reset cursor counter, so that parameter is visible
    displayCursorCtr = 0;
  } break;

  case SCS_MENU_STATE_EDIT_BROWSER: {
    if( incrementer > 0 && scsEditPos < (scsEditItemsPerPage-1) )
      ++scsEditPos;
    else if( incrementer < 0 && scsEditPos > 0 )
      --scsEditPos;
    else {
      int newOffset = scsEditOffset + incrementer;
      if( newOffset < 0 )
	newOffset = 0;
      else if( (newOffset+scsEditItemsPerPage) >= scsEditNumItems ) {
	newOffset = scsEditNumItems - scsEditItemsPerPage;
	if( newOffset < 0 )
	  newOffset = 0;
      }
      scsEditOffset = newOffset;

      // request list update
      scsEditPageUpdateReq = 1;
    }

    // flicker menu item for 200 mS
    displayFlickerSelectionCtr = 200;
  } break;


    //default: // SCS_MENU_STATE_MAINPAGE
  }

  displayUpdateReq = 1;

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! This function changes the encoder speed depending on the item value range
//! It's called when a new parameter has been selected
//! Optionally it can be called from external (selection function) if another
//! speed setting is desired
//! \return < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 SCS_ENC_MENU_AutoSpeedSet(u16 maxValue)
{
  mios32_enc_config_t enc_config = MIOS32_ENC_ConfigGet(SCS_ENC_MENU_ID);

  if( maxValue < 32 ) {
    enc_config.cfg.speed = NORMAL;
    enc_config.cfg.speed_par = 0;
  } else if( maxValue < 128 ) {
    enc_config.cfg.speed = FAST;
    enc_config.cfg.speed_par = 3;
  } else if( maxValue < 512 ) {
    enc_config.cfg.speed = FAST;
    enc_config.cfg.speed_par = 4;
  } else {
    enc_config.cfg.speed = FAST;
    enc_config.cfg.speed_par = 5;
  }

  MIOS32_ENC_ConfigSet(SCS_ENC_MENU_ID, enc_config);

  return 0;
}


/////////////////////////////////////////////////////////////////////////////
//! This function should be called from APP_DIN_NotifyToggle when a button has been toggled
//! \param[in] pin the pin number (SCS_PIN_xxx)
//! \param[in] depressed the pin state
//! \return < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 SCS_DIN_NotifyToggle(u8 pin, u8 depressed)
{
  int softButton = -1;

  // check for overruling
  if( scsButtonHook && scsButtonHook(pin, depressed) > 0 ) {
    displayUpdateReq = 1;
    return 1;
  }

  // note: don't usw switch() here to allow soft-assignments (SCS_PIN_xxx can optionally reference global variables)

  if( pin == SCS_PIN_DEC ) {
    if( depressed )
      return 0;
    return SCS_ENC_MENU_NotifyChange(-1);

  } else if( pin == SCS_PIN_INC ) {
    if( depressed )
      return 0;
    return SCS_ENC_MENU_NotifyChange(1);

  } else if( pin == SCS_PIN_EXIT ) {
    if( depressed ) {
      // deinstall callback if it was active
      scsDelayedActionCallback = 0;
      return 0; // no error
    }

    switch( scsMenuState ) {
    case SCS_MENU_STATE_SELECT_PAGE: {
      if( previousMenuTable == NULL ) {
	scsMenuState = SCS_MENU_STATE_MAINPAGE;
      } else {
	displayRootOffset = 0; // currently not recursively stored...
	if( previousMenuTable == rootTable ) {
	  previousMenuTable = NULL;
	  currentMenuTable = rootTable;
	  currentMenuTableNumItems = rootTableNumItems;
	} else {
	  currentMenuTableNumItems = currentMenuTable->numItems;
	  currentMenuTable = previousMenuTable;
	}
      }
      displayInitReq = 1;
#if SCS_MENU_NO_SOFT_BUTTON_MODE
      displayCursorPos = 0;
#endif
      scsMsgCtr = 0; // disable message
    } break;
    case SCS_MENU_STATE_INSIDE_PAGE: {
      scsMenuState = SCS_MENU_STATE_SELECT_PAGE;
      displayInitReq = 1;
      scsMsgCtr = 0; // disable message
#if SCS_MENU_NO_SOFT_BUTTON_MODE
      displayCursorPos = 0;
#endif
    } break;
    case SCS_MENU_STATE_EDIT_ITEM: {
      scsMenuState = SCS_MENU_STATE_INSIDE_PAGE;
      scsMsgCtr = 0; // disable message
    } break;
    case SCS_MENU_STATE_EDIT_STRING: {
      scsMenuState = SCS_MENU_STATE_INSIDE_PAGE;
      scsMsgCtr = 0; // disable message
      scsEditStringCallback = NULL; // disable edit string callback
    }
    case SCS_MENU_STATE_EDIT_IP: {
      scsMenuState = SCS_MENU_STATE_INSIDE_PAGE;
      scsMsgCtr = 0; // disable message
      scsEditIpCallback = NULL; // disable edit IP callback
    }
    case SCS_MENU_STATE_EDIT_BROWSER: {
      scsMenuState = SCS_MENU_STATE_INSIDE_PAGE;
      scsMsgCtr = 0; // disable message
      scsEditStringCallback = NULL; // disable edit string callback
      scsEditGetListCallback = NULL; // disable edit string callback
    }
      //default: // SCS_MENU_STATE_MAINPAGE
      //scsMenuState = SCS_MENU_STATE_SELECT_PAGE;
      // enter page only via soft buttons
    }
    SCS_ENC_MENU_AutoSpeedSet(1); // slow speed..
    displayUpdateReq = 1;

#if SCS_MENU_NO_SOFT_BUTTON_MODE
  } else if( pin == SCS_PIN_SOFT1 ) {
    softButton = 0; // SELECT button
#else
  } else if( pin == SCS_PIN_SOFT1 ) {
    softButton = 0;
  } else if( pin == SCS_PIN_SOFT2 ) {
    softButton = 1;
  } else if( pin == SCS_PIN_SOFT3 ) {
    softButton = 2;
  } else if( pin == SCS_PIN_SOFT4 ) {
    softButton = 3;
  } else if( pin == SCS_PIN_SOFT5 ) {
    softButton = 4;
  } else if( pin == SCS_PIN_SOFT6 ) {
    softButton = 5;
  } else if( pin == SCS_PIN_SOFT7 ) {
    softButton = 6;
  } else if( pin == SCS_PIN_SOFT8 ) {
    softButton = 7;
  } else if( pin == SCS_PIN_SOFT9 ) {
    softButton = 8;
  } else if( pin == SCS_PIN_SOFT10 ) {
    softButton = 9;
#endif
  } else {
    return -1; // unsupported pin
  }

  if( softButton >= 0 ) {

    if( depressed ) {
      // deinstall callback if it was active
      scsDelayedActionCallback = 0;
      return 0; // no error
    }

    switch( scsMenuState ) {
    case SCS_MENU_STATE_SELECT_PAGE: {
      int newPage = displayRootOffset + softButton;
#if SCS_MENU_NO_SOFT_BUTTON_MODE
      newPage += displayCursorPos;
#endif
      if( newPage < currentMenuTableNumItems ) {
#if !SCS_MENU_NO_SOFT_BUTTON_MODE
	displayCursorPos = softButton;
#endif
	if( currentMenuTable[newPage].isSubPage ) {
	  previousMenuTable = currentMenuTable;
	  currentMenuTable = (scs_menu_page_t *)previousMenuTable[newPage].page;
	  currentMenuTableNumItems = previousMenuTable[newPage].numItems;
	  displayRootOffset = 0;
	} else {
	  currentMenuTableSelectedPagePos = newPage;
	  displayPageOffset = 0; // optionally we got store the last offset somewhere?
	  scsMenuState = SCS_MENU_STATE_INSIDE_PAGE;
	}
#if SCS_MENU_NO_SOFT_BUTTON_MODE
	displayCursorPos = 0;
#endif
	displayInitReq = 1;
      }
    } break;

    case SCS_MENU_STATE_INSIDE_PAGE:
    case SCS_MENU_STATE_EDIT_ITEM: {
      scs_menu_item_t *pageItems = NULL;
      u8 numItems = 0;
      if( currentMenuTableSelectedPagePos < currentMenuTableNumItems ) {
	pageItems = (scs_menu_item_t *)currentMenuTable[currentMenuTableSelectedPagePos].page;
	numItems = currentMenuTable[currentMenuTableSelectedPagePos].numItems;
      }

      int itemPos = displayPageOffset + softButton;
#if SCS_MENU_NO_SOFT_BUTTON_MODE
      itemPos += displayCursorPos;
#endif

      if( itemPos < numItems ) {
	scs_menu_item_t *pageItem = (scs_menu_item_t *)&pageItems[itemPos];

	// edit item if maxValue > SCS_MENU_ITEM_TOGGLE_THRESHOLD
	if( !pageItem->maxValue || pageItem->maxValue >= SCS_MENU_ITEM_TOGGLE_THRESHOLD ) {
#if !SCS_MENU_NO_SOFT_BUTTON_MODE
	  displayCursorPos = softButton;
#endif
	  currentMenuTableSelectedItemPos = itemPos;

	  if( scsMenuState == SCS_MENU_STATE_INSIDE_PAGE ) {
	    scsMenuState = SCS_MENU_STATE_EDIT_ITEM;
	    SCS_ENC_MENU_AutoSpeedSet(pageItem->maxValue);
	  }
	}

	int oldValue = pageItem->getFunct(pageItem->ix);
	int newValue = pageItem->selectFunct(pageItem->ix, oldValue);
	if( oldValue == newValue &&
	    pageItem->maxValue &&
	    pageItem->maxValue < SCS_MENU_ITEM_TOGGLE_THRESHOLD ) {
	  ++newValue;
	  if( newValue > pageItem->maxValue )
	    newValue = 0;
	}
	pageItem->setFunct(pageItem->ix, newValue);
      }

      // flicker menu item for 200 mS
      displayFlickerSelectionCtr = 200;
    } break;

    case SCS_MENU_STATE_EDIT_STRING: {
      switch( softButton ) {
      case 0: {
	if( scsEditStringCallback )
	  scsEditStringCallback(scsEditString);
	scsMenuState = SCS_MENU_STATE_INSIDE_PAGE;
      } break;

      case 1: { // <
	if( scsEditPos > 0 )
	  --scsEditPos;
	// flicker menu item for 200 mS
	displayFlickerSelectionCtr = 200;
      } break;

      case 2: { // >
	if( scsEditPos < scsEditStringMaxChars )
	  ++scsEditPos;

	// flicker menu item for 200 mS
	displayFlickerSelectionCtr = 200;
      } break;

      case 3: { // Del
	int i;
	for(i=scsEditPos; i<scsEditStringMaxChars; ++i)
	  scsEditString[i] = scsEditString[i+1];
	scsEditString[scsEditStringMaxChars] = ' ';

	// flicker menu item for 200 mS
	displayFlickerSelectionCtr = 200;
      } break;

      case 4: { // Clr
	int i;
	for(i=0; i<=scsEditStringMaxChars; ++i)
	  scsEditString[i] = ' ';

	// flicker menu item for 200 mS
	displayFlickerSelectionCtr = 200;
      } break;

      }
    } break;

    case SCS_MENU_STATE_EDIT_IP: {
      switch( softButton ) {
      case 0: {
	if( scsEditIpCallback )
	  scsEditIpCallback(scsEditIp);
	scsMenuState = SCS_MENU_STATE_INSIDE_PAGE;
      } break;

      case 1: { // <
	if( scsEditPos > 0 )
	  --scsEditPos;
	// flicker menu item for 200 mS
	displayFlickerSelectionCtr = 200;
      } break;

      case 2: { // >
	if( scsEditPos < 3 )
	  ++scsEditPos;

	// flicker menu item for 200 mS
	displayFlickerSelectionCtr = 200;
      } break;
      }
    } break;

    case SCS_MENU_STATE_EDIT_BROWSER: {
      switch( softButton ) {
      case 0: {
	if( scsEditStringCallback ) {
	  if( !scsEditNumItems ) {
	    scsEditString[0] = 0;
	  } else {
	    // copy selected item in scsEditString to beginning of scsEditString
	    if( scsEditPos > 0 ) {
	      memcpy(scsEditString, scsEditString+scsEditPos*scsEditStringMaxChars, scsEditStringMaxChars);
	    }
	    scsEditString[scsEditStringMaxChars] = 0;

	    // remove spaces at the end of string
	    int i;
	    for(i=scsEditStringMaxChars-1; i>=0; --i)
	      if( scsEditString[i] == ' ' )
		scsEditString[i] = 0;
	      else if( scsEditString[i] != 0 )
		break;
	  }

	  scsEditStringCallback(scsEditString);
	}
	scsMenuState = SCS_MENU_STATE_INSIDE_PAGE;
      } break;

      case 1: { // <
	SCS_ENC_MENU_NotifyChange(-1);
      } break;

      case 2: { // >
	SCS_ENC_MENU_NotifyChange(1);
      } break;
      }
    } break;

    default: { // SCS_MENU_STATE_MAINPAGE
      displayInitReq = 1;
      scsMenuState = SCS_MENU_STATE_SELECT_PAGE;
      if( previousMenuTable == NULL ) { // root directory
	currentMenuTable = rootTable;
	currentMenuTableNumItems = rootTableNumItems;
      } else {
	currentMenuTable = previousMenuTable;
	currentMenuTableNumItems = previousMenuTable->numItems;
      }
    }
    }
    displayUpdateReq = 1;
  }

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
//! This handler should be called from a FreeRTOS task each millisecond with low priority
//! \return < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 SCS_Tick(void)
{
  ///////////////////////////////////////////////////////////////////////////
  // check for pin changes
  // only relevant if buttons are directly connected to core (LPC17: via J10)
  // STM32 will call SCS_DIN_NotifyToggle from APP_DIN_NotifyToggle (buttons connected to DINX1)
  ///////////////////////////////////////////////////////////////////////////

  u8 checkButtons = 1;
#if SCS_BUTTON_DEBOUNCE_RELOAD
  if( scsButtonDebounceCtr )
    checkButtons = 0;
#endif

  if( checkButtons ) {
    // should be atomic
    MIOS32_IRQ_Disable();
    u16 changedState = scsPinStatePrev ^ scsPinState;
    scsPinStatePrev = scsPinState;
    u16 pinState = scsPinState;
    MIOS32_IRQ_Enable();

    // check each button
    int pin;
    u16 mask = (1 << 0);
    for(pin=0; pin<16; ++pin, mask <<= 1) {
      if( changedState & mask ) {
	SCS_DIN_NotifyToggle(pin, (pinState & mask) ? 1 : 0);
#if SCS_BUTTON_DEBOUNCE_RELOAD
	scsButtonDebounceCtr = SCS_BUTTON_DEBOUNCE_RELOAD;
#endif
      }
    }
  }

  ///////////////////////////////////////////////////////////////////////////
  // delayed action will be triggered once counter reached 0
  ///////////////////////////////////////////////////////////////////////////

  if( !scsDelayedActionCallback )
    scsDelayedActionCtr = 0;
  else if( scsDelayedActionCtr ) {
    if( --scsDelayedActionCtr == 0 ) {
      // must be atomic
      MIOS32_IRQ_Disable();
      void (*_scsDelayedActionCallback)(u32 parameter);
      _scsDelayedActionCallback = scsDelayedActionCallback;
      u32 parameter = scsDelayedActionParameter;
      scsDelayedActionCallback = NULL;
      MIOS32_IRQ_Enable();
      _scsDelayedActionCallback(parameter); // note: it's allowed that the delayed action generates a new delayed action
    }
  }


  ///////////////////////////////////////////////////////////////////////////
  // handle LCD
  ///////////////////////////////////////////////////////////////////////////

  if( displayInitReq ) {
    displayInitReq = 0;
    displayUpdateReq = 1;
    SCS_LCD_Clear();
  }

  if( displayUpdateReq ) {
    displayUpdateReq = 0;

    // call optional display hook for overruling
    char line1[SCS_MAX_STR];
    line1[0] = 0;
    char line2[SCS_MAX_STR];
    line2[0] = 0;
    if( scsDisplayHook && scsDisplayHook(line1, line2) < 1 ) {
      line1[0] = 0; // disable again: no overruling
      line2[0] = 0;
    }

    // notifies if any page has been overruled
    u8 line1AlreadyPrint = 0;
    if( line1[0] ) {
      SCS_LCD_CursorSet(0, 0);
      SCS_LCD_PrintStringPadded(line1, SCS_LCD_MAX_COLUMNS);
      line1AlreadyPrint = 1;
    }

    u8 line2AlreadyPrint = 0;
    if( line2[0] ) {
      SCS_LCD_CursorSet(0, 1);
      SCS_LCD_PrintStringPadded(line2, SCS_LCD_MAX_COLUMNS);
      line2AlreadyPrint = 1;
    }

    switch( scsMenuState ) {
      /////////////////////////////////////////////////////////////////////////
    case SCS_MENU_STATE_SELECT_PAGE: {
      if( !line1AlreadyPrint ) {
	SCS_LCD_CursorSet(0, 0);
	SCS_LCD_PrintStringPadded("Select Page:", SCS_LCD_MAX_COLUMNS);
      }

      if( !line2AlreadyPrint ) {
	SCS_LCD_CursorSet(0, 1);

	int i;
	for(i=0; i<SCS_NUM_MENU_ITEMS; ++i) {
	  u8 page = displayRootOffset + i;
	  if( page >= currentMenuTableNumItems
#if SCS_MENU_NO_SOFT_BUTTON_MODE
	      || (page == (displayRootOffset+displayCursorPos) && (!displayCursorOn || !displayLabelOn))
#endif
	      )
	    SCS_LCD_PrintSpaces(SCS_MENU_ITEM_WIDTH);
	  else {
	    SCS_LCD_PrintStringPadded(currentMenuTable[page].name, SCS_MENU_ITEM_WIDTH);
	  }
	}

	// print arrow at upper right corner
	if( currentMenuTableNumItems > SCS_NUM_MENU_ITEMS ) {
	  SCS_LCD_CursorSet(SCS_LCD_MAX_COLUMNS-1, 0);
	  if( displayRootOffset == 0 )
	    SCS_LCD_PrintChar(3); // right arrow
	  else if( displayRootOffset >= (currentMenuTableNumItems-SCS_NUM_MENU_ITEMS) )
	    SCS_LCD_PrintChar(1); // left arrow
	  else
	    SCS_LCD_PrintChar(2); // left/right arrow
	}
      }
    } break;

    /////////////////////////////////////////////////////////////////////////
    case SCS_MENU_STATE_INSIDE_PAGE:
    case SCS_MENU_STATE_EDIT_ITEM: {
      scs_menu_item_t *pageItems = NULL;
      u8 numItems = 0;
      if( currentMenuTableSelectedPagePos < currentMenuTableNumItems ) {
	pageItems = (scs_menu_item_t *)currentMenuTable[currentMenuTableSelectedPagePos].page;
	numItems = currentMenuTable[currentMenuTableSelectedPagePos].numItems;
      }

      if( !pageItems ) {
	if( !line1AlreadyPrint ) {
	  SCS_LCD_CursorSet(0, 0);
	  SCS_LCD_PrintStringPadded("!! ERROR !!", SCS_LCD_MAX_COLUMNS);
	}
	if( !line2AlreadyPrint ) {
	  SCS_LCD_CursorSet(0, 1);
	  SCS_LCD_PrintStringPadded("!! INVALID PAGE ITEMS !!", SCS_LCD_MAX_COLUMNS);
	}
      } else {
	// check for full screen message
	u8 printCommonPage = 1;
	if( scsMenuState == SCS_MENU_STATE_EDIT_ITEM ) {
	  scs_menu_item_t *pageItem = NULL;
	  if( currentMenuTableSelectedItemPos < numItems )
	    pageItem = (scs_menu_item_t *)&pageItems[currentMenuTableSelectedItemPos];

	  if( pageItem && pageItem->stringFullFunct ) {
	    printCommonPage = 0;

	    strcpy(line1, "???");
	    strcpy(line2, "???");
	    u16 value = pageItem->getFunct(pageItem->ix);
	    pageItem->stringFullFunct(pageItem->ix, value, line1, line2);

	    if( !line1AlreadyPrint ) {
	      SCS_LCD_CursorSet(0, 0);
	      SCS_LCD_PrintStringPadded(line1, SCS_LCD_MAX_COLUMNS);
	    }
	    if( !line2AlreadyPrint ) {
	      SCS_LCD_CursorSet(0, 1);
	      SCS_LCD_PrintStringPadded(line2, SCS_LCD_MAX_COLUMNS);
	    }
	  }
	}

	if( printCommonPage ) {
	  u8 editMode = scsMenuState == SCS_MENU_STATE_EDIT_ITEM;

	  // first line
	  if( !line1AlreadyPrint ) {
	    SCS_LCD_CursorSet(0, 0);

	    int i;
	    for(i=0; i<SCS_NUM_MENU_ITEMS; ++i) {
	      u8 item = displayPageOffset + i;
	      if( item >= numItems ||
#if SCS_MENU_NO_SOFT_BUTTON_MODE
		  (!editMode && (item == displayPageOffset+displayCursorPos) && (!displayCursorOn || !displayLabelOn)) ||
#endif
		  (editMode && item == currentMenuTableSelectedItemPos && !displayLabelOn ) )
		SCS_LCD_PrintSpaces(SCS_MENU_ITEM_WIDTH);
	      else {
		SCS_LCD_PrintStringPadded(pageItems[item].name, SCS_MENU_ITEM_WIDTH);
	      }
	    }
	  }

	  // second line
	  if( !line2AlreadyPrint ) {
	    SCS_LCD_CursorSet(0, 1);

	    int i;
	    for(i=0; i<SCS_NUM_MENU_ITEMS; ++i) {
	      u8 item = displayPageOffset + i;
	      if( item >= numItems ||
		  (editMode && item == currentMenuTableSelectedItemPos && !displayCursorOn ) )
		SCS_LCD_PrintSpaces(SCS_MENU_ITEM_WIDTH);
	      else {
		scs_menu_item_t *pageItem = (scs_menu_item_t *)&pageItems[item];
		char label[SCS_MAX_STR];
		strcpy(label, "??? "); // default
		u16 value = pageItem->getFunct(pageItem->ix);
		pageItem->stringFunct(pageItem->ix, value, label);
		SCS_LCD_PrintStringPadded(label, SCS_MENU_ITEM_WIDTH);
	      }
	    }
	  }

	  // print arrow at upper right corner
	  if( !line1AlreadyPrint && numItems > SCS_NUM_MENU_ITEMS ) {
	    SCS_LCD_CursorSet(SCS_LCD_MAX_COLUMNS-1, 0);
	    if( displayPageOffset == 0 )
	      SCS_LCD_PrintChar(3); // right arrow
	    else if( displayPageOffset >= (numItems-SCS_NUM_MENU_ITEMS) )
	      SCS_LCD_PrintChar(1); // left arrow
	    else
	      SCS_LCD_PrintChar(2); // left/right arrow
	  }
	}
      }
    } break;

    /////////////////////////////////////////////////////////////////////////
    case SCS_MENU_STATE_EDIT_STRING: {
      // check if empty string
      int i;
      u8 stringIsEmpty = 1;
      for(i=0; i<scsEditStringMaxChars; ++i)
	if( scsEditString[i] != ' ' ) {
	  stringIsEmpty = 0;
	  break;
	}

      // edited string
      if( !line1AlreadyPrint ) {
	SCS_LCD_CursorSet(0, 0);      
	SCS_LCD_PrintStringPadded(stringIsEmpty ? "<empty>" : scsEditString, SCS_LCD_COLUMNS_PER_DEVICE);

	// set cursor
	if( !displayCursorOn || (displayFlickerSelectionCtr && displayLabelOn) ) {
	  SCS_LCD_CursorSet(scsEditPos, 0);
	  SCS_LCD_PrintChar((scsEditString[scsEditPos] == '*') ? ' ' : '*');
	}
      }

      // edit functions
      if( !line2AlreadyPrint ) {
	SCS_LCD_CursorSet(0, 1);
	SCS_LCD_PrintStringCentered(scsActionString, SCS_MENU_ITEM_WIDTH);
	if( SCS_NUM_MENU_ITEMS >= 2 )
	  SCS_LCD_PrintStringCentered("<", SCS_MENU_ITEM_WIDTH);
	if( SCS_NUM_MENU_ITEMS >= 3 )
	  SCS_LCD_PrintStringCentered(">", SCS_MENU_ITEM_WIDTH);
	if( SCS_NUM_MENU_ITEMS >= 4 )
	  SCS_LCD_PrintStringCentered("Del", SCS_MENU_ITEM_WIDTH);
	if( SCS_NUM_MENU_ITEMS >= 5 )
	  SCS_LCD_PrintStringCentered("Clr", SCS_MENU_ITEM_WIDTH);
      }
    } break;

    /////////////////////////////////////////////////////////////////////////
    case SCS_MENU_STATE_EDIT_IP: {
      if( !line1AlreadyPrint ) {
	SCS_LCD_CursorSet(0, 0);

	SCS_LCD_PrintStringPadded(scsEditString, SCS_LCD_COLUMNS_PER_DEVICE-15);

	int i;
	for(i=0; i<4; ++i) {
	  if( i == scsEditPos && (!displayCursorOn || (displayFlickerSelectionCtr && displayLabelOn)) )
	    SCS_LCD_PrintSpaces(4);
	  else
	    SCS_LCD_PrintFormattedString("%3d%c",
					 (scsEditIp >> (8*(3-i))) & 0xff,
					 (i < 3) ? '.' : ' ');
	}
      }

      // edit functions
      if( !line2AlreadyPrint ) {
	SCS_LCD_CursorSet(0, 1);
	SCS_LCD_PrintStringCentered("Ok", SCS_MENU_ITEM_WIDTH);
	if( SCS_NUM_MENU_ITEMS >= 2 )
	  SCS_LCD_PrintStringCentered("<", SCS_MENU_ITEM_WIDTH);
	if( SCS_NUM_MENU_ITEMS >= 3 )
	  SCS_LCD_PrintStringCentered(">", SCS_MENU_ITEM_WIDTH);
	SCS_LCD_PrintSpaces(SCS_LCD_COLUMNS_PER_DEVICE-3*SCS_MENU_ITEM_WIDTH);
      }
    } break;

    /////////////////////////////////////////////////////////////////////////
    case SCS_MENU_STATE_EDIT_BROWSER: {
      // browser line
      if( !line1AlreadyPrint ) {
	if( scsEditPageUpdateReq ) {
	  // get new line based on offset
	  scsEditPageUpdateReq = 0;
	  scsEditNumItems = scsEditGetListCallback ? scsEditGetListCallback(scsEditOffset, scsEditString) : 0;
	}

	SCS_LCD_CursorSet(0, 0);
	SCS_LCD_PrintStringPadded(scsEditString, SCS_LCD_COLUMNS_PER_DEVICE);

	if( scsEditNumItems ) {
	  // set cursor
	  if( !displayCursorOn || (displayFlickerSelectionCtr && displayLabelOn) ) {
	    SCS_LCD_CursorSet(scsEditPos*scsEditStringMaxChars, 0);
	    SCS_LCD_PrintSpaces(scsEditStringMaxChars);
	  }

	  // print arrow at upper right corner
	  if( scsEditNumItems > scsEditItemsPerPage ) {
	    SCS_LCD_CursorSet(SCS_LCD_MAX_COLUMNS-1, 0);
	    if( scsEditOffset == 0 )
	      SCS_LCD_PrintChar(3); // right arrow
	    else if( scsEditOffset >= (scsEditNumItems-scsEditItemsPerPage) )
	      SCS_LCD_PrintChar(1); // left arrow
	    else
	      SCS_LCD_PrintChar(2); // left/right arrow
	  }
	}
      }

      // edit functions
      if( !line2AlreadyPrint ) {
	SCS_LCD_CursorSet(0, 1);
	SCS_LCD_PrintStringCentered(scsActionString, SCS_MENU_ITEM_WIDTH);
	if( SCS_NUM_MENU_ITEMS >= 2 )
	  SCS_LCD_PrintStringCentered("<", SCS_MENU_ITEM_WIDTH);
	if( SCS_NUM_MENU_ITEMS >= 3 )
	  SCS_LCD_PrintStringCentered(">", SCS_MENU_ITEM_WIDTH);
      }
    } break;

    /////////////////////////////////////////////////////////////////////////
    default: { // SCS_MENU_STATE_MAINPAGE
      if( !line1AlreadyPrint ) {
	strncpy(line1, MIOS32_LCD_BOOT_MSG_LINE1, SCS_LCD_MAX_COLUMNS);
	SCS_LCD_CursorSet(0, 0);
	SCS_LCD_PrintStringPadded(line1, SCS_LCD_MAX_COLUMNS);
      }
      if( !line2AlreadyPrint ) {
	strcpy(line2, "Press soft button");
	SCS_LCD_CursorSet(0, 1);
	SCS_LCD_PrintStringPadded(line2, SCS_LCD_MAX_COLUMNS);
      }
    }
    }
  }

  // if message active: overrule the common text
  if( scsMsgCtr ) {
    --scsMsgCtr;

    char *animation_l_ptr;
    char *animation_r_ptr;
    u8 msg_x = 0;
    u8 right_aligned = 0;
    u8 disable_message = 0;

    switch( scsMsgType ) {
      case SCS_MSG_ERROR_L:
      case SCS_MSG_ERROR_R: {
	animation_l_ptr = (char *)animation_l_arrows;
	animation_r_ptr = (char *)animation_r_arrows;
	if( scsMsgType == SCS_MSG_ERROR_R ) {
	  msg_x = SCS_LCD_MAX_COLUMNS-1;
	  right_aligned = 1;
	} else {
	  msg_x = 0;
	  right_aligned = 0;
	}
      } break;

      case SCS_MSG_DELAYED_ACTION_L:
      case SCS_MSG_DELAYED_ACTION_R: {
	animation_l_ptr = (char *)animation_l_brackets;
	animation_r_ptr = (char *)animation_r_brackets;
	if( scsMsgType == SCS_MSG_DELAYED_ACTION_R ) {
	  msg_x = SCS_LCD_MAX_COLUMNS-1;
	  right_aligned = 1;
	} else {
	  msg_x = 0;
	  right_aligned = 0;
	}

	if( scsDelayedActionCallback == NULL ) {
	  disable_message = 1; // button has been depressed before delay
	} else {
	  int seconds = (scsDelayedActionCtr / 1000) + 1;
	  if( seconds == 1 )
	    sprintf(scsMsg[0], "Hold 1 second ");
	  else
	    sprintf(scsMsg[0], "Hold %d seconds", seconds);
	}
      } break;

      case SCS_MSG_R: {
	animation_l_ptr = (char *)animation_l_stars;
	animation_r_ptr = (char *)animation_r_stars;
	msg_x = SCS_LCD_MAX_COLUMNS-1;
	right_aligned = 1;
      } break;

      default: { // SCS_MSG_L
	animation_l_ptr = (char *)animation_l_stars;
	animation_r_ptr = (char *)animation_r_stars;
	msg_x = 0;
	right_aligned = 0;
	//MIOS32_MIDI_SendDebugMessage("1");
      } break;

    }

    if( !disable_message ) {
      int anum = (scsMsgCtr % 1000) / 250;

      int len[2];
      len[0] = strlen((char *)scsMsg[0]);
      len[1] = strlen((char *)scsMsg[1]);
      int len_max = len[0];
      if( len[1] > len_max )
	len_max = len[1];

      if( right_aligned )
	msg_x -= (9 + len_max);

      int line;
      for(line=0; line<2; ++line) {

	if( scsMsg[line][0] == 0 )
	  continue;

	SCS_LCD_CursorSet(msg_x, line);

	// ensure that both lines are padded with same number of spaces
	int end_pos = len[line];
	while( end_pos < len_max )
	  scsMsg[line][end_pos++] = ' ';
	scsMsg[line][end_pos] = 0;

#if 0
	SCS_LCD_PrintFormattedString(" %c%c| %s |%c%c ",
				     *(animation_l_ptr + 2*anum + 0), *(animation_l_ptr + 2*anum + 1),
				     (char *)scsMsg[line], 
				     *(animation_r_ptr + 2*anum + 0), *(animation_r_ptr + 2*anum + 1));
#else
	SCS_LCD_PrintFormattedString("%c%c %s %c%c ",
				     *(animation_l_ptr + 2*anum + 0), *(animation_l_ptr + 2*anum + 1),
				     (char *)scsMsg[line], 
				     *(animation_r_ptr + 2*anum + 0), *(animation_r_ptr + 2*anum + 1));
#endif
      }
    }

    if( disable_message || scsMsgCtr == 0 ) {
      // re-init and update display with next tick
      displayInitReq = 1;
    }
  }

  // transfer display changes to LCD if necessary (no force)
  SCS_LCD_Update(0);

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! Installs a root table
//! \param[in] rootTable pointer to table of pages
//! \param[in] numItems number of items in table
//! \return < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 SCS_InstallRoot(scs_menu_page_t *_rootTable, u8 numItems)
{
  rootTable = _rootTable;
  rootTableNumItems = numItems;

  previousMenuTable = NULL;
  currentMenuTable = rootTable;
  currentMenuTableNumItems = rootTableNumItems;
  currentMenuTableSelectedPagePos = 0;
  currentMenuTableSelectedItemPos = 0;

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
//! Installs string function which can overrule the display output
//! If it returns 0, the original SCS output will be print
//! If it returns 1, the output copied into line1 and/or line2 will be print
//! If a line is not changed (line[0] = 0 or line[1] = 0), the original output
//! will be displayed - this allows to overrule only a single line
//! \param[in] stringFunct pointer to print function
//! \return < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 SCS_InstallDisplayHook(s32 (*stringFunct)(char *line1, char *line2))
{
  scsDisplayHook = stringFunct;

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! Installs encoder handler for overruling
//! If it returns 0, the encoder increment will be handled by the SCS
//! If it returns 1, the SCS will ignore the encoder
//! \param[in] encFunct the encoder function
//! \return < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 SCS_InstallEncHook(s32 (*encFunct)(s32 incrementer))
{
  scsEncHook = encFunct;

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! Installs button handler for overruling
//! If it returns 0, the button movement will be handled by the SCS
//! If it returns 1, the SCS will ignore the button event
//! \param[in] buttonFunct the button function
//! \return < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 SCS_InstallButtonHook(s32 (*buttonFunct)(u8 scsButton, u8 depressed))
{
  scsButtonHook = buttonFunct;

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! Can be called from external to force a display update
//! \return < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 SCS_DisplayUpdateRequest(void)
{
  displayUpdateReq = 1;

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! Returns the current display state
/////////////////////////////////////////////////////////////////////////////
scs_menu_state_t SCS_MenuStateGet(void)
{
  return scsMenuState;
}

/////////////////////////////////////////////////////////////////////////////
//! Returns the current page pointer (not menu item as the scs_menu_item_t type
//! could imply) if menu state is SCS_MENU_STATE_INSIDE_PAGE or
//! SCS_MENU_STATE_EDIT_ITEM, otherwise returns NULL\n
//! Can be used in display/button/encoder hook to overrule something if
//! a certain page is displayed\n
//! See apps/controllers/midio128_v3/src/scs_config.c for usage example
/////////////////////////////////////////////////////////////////////////////
scs_menu_item_t *SCS_MenuPageGet(void)
{
  if( scsMenuState != SCS_MENU_STATE_INSIDE_PAGE &&
      scsMenuState != SCS_MENU_STATE_EDIT_ITEM )
    return NULL;

  if( !rootTable || !currentMenuTableNumItems || currentMenuTableSelectedPagePos >= currentMenuTableNumItems )
    return NULL;

  return (scs_menu_item_t *)currentMenuTable[currentMenuTableSelectedPagePos].page;
}

/////////////////////////////////////////////////////////////////////////////
//! Print temporary user messages (e.g. warnings, errors)
//! expects mS delay and two lines, each up to 18 characters
//!
//! Examples:
//! \code
//!     // try *one* of these lines (only last message will be displayed for given delay)
//!     SCS_Msg(SCS_MSG_L, 1000, "Left", "Side");
//! // or:
//!     SCS_Msg(SCS_MSG_R, 1000, "Right", "Side");
//! // or:
//!     SCS_InstallDelayedActionCallback(clearPatch, 2000, selectedPatch);
//!     SCS_Msg(SEQ_UI_MSG_DELAYED_ACTION_L, 2001, "", "to clear patch");
//! \endcode
/////////////////////////////////////////////////////////////////////////////
s32 SCS_Msg(scs_msg_type_t msgType, u16 delay, char *line1, char *line2)
{
  scsMsgType = msgType;
  scsMsgCtr = delay;
  strncpy((char *)scsMsg[0], line1, SCS_MSG_MAX_CHAR);
  strncpy((char *)scsMsg[1], line2, SCS_MSG_MAX_CHAR);

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
//! Stops temporary message if no SD card warning
/////////////////////////////////////////////////////////////////////////////
s32 SCS_MsgStop(void)
{
  scsMsgCtr = 0;

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
//! Function will be called after given delay with given parameter\n
//! See tutorial/027_scs for usage example\n
//! Note that only a single callback function can be handled, if another
//! one was active before, it will be dropped.
/////////////////////////////////////////////////////////////////////////////
s32 SCS_InstallDelayedActionCallback(void (*callback)(u32 parameter), u16 delay_mS, u32 parameter)
{
  // must be atomic
  MIOS32_IRQ_Disable();
  scsDelayedActionParameter = parameter;
  scsDelayedActionCallback = callback;
  scsDelayedActionCtr = delay_mS;
  MIOS32_IRQ_Enable();

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
//! Disables delayed action of the given callback function\n
//! Only relevant if the callback should be disabled before the delay given
//! via SCS_InstallDelayedActionCallback passed.\n
//! After this delay has passed, the callback will be dropped automatically.
/////////////////////////////////////////////////////////////////////////////
s32 SCS_UnInstallDelayedActionCallback(void (*callback)(u32 parameter))
{
  // must be atomic
  MIOS32_IRQ_Disable();
  if( scsDelayedActionCallback == callback )
    scsDelayedActionCallback = 0;
  MIOS32_IRQ_Enable();

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! Can be called from an item select function to enter string editing mode
//! (e.g. to enter a filename).\n
//! See tutorial/027_scs for usage example
/////////////////////////////////////////////////////////////////////////////
s32 SCS_InstallEditStringCallback(void (*selectCallback)(char *newString), char *actionString, char *initialString, u8 maxChars)
{
  if( maxChars >= SCS_LCD_COLUMNS_PER_DEVICE )
    maxChars = SCS_LCD_COLUMNS_PER_DEVICE;
  memcpy(scsEditString, initialString, maxChars);
  scsEditString[maxChars] = 0; // makes sense to set terminator for the case that it isn't available in initialString
  memcpy(scsActionString, actionString, SCS_MENU_ITEM_WIDTH);
  scsActionString[SCS_MENU_ITEM_WIDTH] = 0; // ditto.
  scsEditStringMaxChars = maxChars;
  scsEditItemsPerPage = 1; // not relevant for this function, just a MEMO for future extensions
  scsEditNumItems = 1; // not relevant for this function, just a MEMO for future extensions
  scsEditOffset = 0; // not relevant for this function, just a MEMO for future extensions

  // set edit pos after last char in string
  // search forward for null terminator
  for(scsEditPos=0; scsEditPos<(maxChars-1); ++scsEditPos)
    if( scsEditString[scsEditPos] == 0 )
      break;
  if( scsEditPos ) {
    // search backward for last character
    for(--scsEditPos; scsEditPos>0; --scsEditPos)
      if( scsEditString[scsEditPos] != ' ' ) {
	if( scsEditPos < maxChars )
	  ++scsEditPos;
	break;
      }
  }

  scsEditStringCallback = selectCallback;
  scsMenuState = SCS_MENU_STATE_EDIT_STRING;

  return 0;
}

/////////////////////////////////////////////////////////////////////////////
//! Can be called from an item select function to enter IP editing mode
//! See tutorial/027_scs for usage example
/////////////////////////////////////////////////////////////////////////////
s32 SCS_InstallEditIpCallback(void (*selectCallback)(u32 newIp), char *headerString, u32 initialIp)
{
  scsEditIpCallback = selectCallback;
  memcpy(scsEditString, headerString, SCS_LCD_COLUMNS_PER_DEVICE+1);
  scsEditIp = initialIp;
  scsEditPos = 0;
  scsEditItemsPerPage = 1; // not relevant for this function, just a MEMO for future extensions
  scsEditNumItems = 1; // not relevant for this function, just a MEMO for future extensions
  scsEditOffset = 0; // not relevant for this function, just a MEMO for future extensions
  scsMenuState = SCS_MENU_STATE_EDIT_IP;

  return 0;
}

/////////////////////////////////////////////////////////////////////////////
//! Can be called from an item select function to enter IP editing mode
//! See tutorial/027_scs for usage example
/////////////////////////////////////////////////////////////////////////////
s32 SCS_InstallEditBrowserCallback(void (*selectCallback)(char *newString), u8 (*getListCallback)(u8 offset, char *line), char *actionString, u8 itemWidth, u8 itemsPerPage)
{
  scsEditStringCallback = selectCallback;
  scsEditGetListCallback = getListCallback;
  memcpy(scsActionString, actionString, SCS_MENU_ITEM_WIDTH);
  scsActionString[SCS_MENU_ITEM_WIDTH] = 0; // ditto.
  scsEditItemsPerPage = itemsPerPage;
  scsEditStringMaxChars = itemWidth;
  scsEditPos = 0;
  scsEditOffset = 0;
  scsEditNumItems = scsEditGetListCallback ? scsEditGetListCallback(scsEditOffset, scsEditString) : 0;
  scsMenuState = SCS_MENU_STATE_EDIT_BROWSER;

  return 0;
}


//! \}
