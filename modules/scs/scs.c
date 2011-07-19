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
//! #define SCS_PIN_ENC_MENU_A 0
//! #define SCS_PIN_ENC_MENU_B 1
//! #define SCS_PIN_MENU       2
//! #define SCS_PIN_SOFT1      3
//! #define SCS_PIN_SOFT2      4
//! #define SCS_PIN_SOFT3      5
//! #define SCS_PIN_SOFT4      6
//! #define SCS_PIN_SOFT5      7
//! #define SCS_PIN_SOFT6      8 // optional if SCS_NUM_MENU_ITEMS >= 6
//! #define SCS_PIN_SOFT7      9 // optional if SCS_NUM_MENU_ITEMS >= 7
//! #define SCS_PIN_SOFT8      10 // optional if SCS_NUM_MENU_ITEMS >= 8
//! #define SCS_PIN_SOFT9      11 // optional if SCS_NUM_MENU_ITEMS >= 9
//! #define SCS_PIN_SOFT10     12 // optional if SCS_NUM_MENU_ITEMS >= 10
//! 
//! // encoder id which is used for MIOS32_ENC
//! #define SCS_ENC_MENU_ID 0
//!
//! // the encoder type (see mios32_enc.h for available types)
//! #define SCS_ENC_MENU_TYPE DETENTED2
//!
//! // number of menu items which are displayed on screen
//! // each item allocates 4x2 characters
//! #define SCS_NUM_MENU_ITEMS 5
//!
//! // width of an item (4 by default, so that 5 items can be output on a 2x20 LCD)
//! #define SCS_MENU_ITEM_WIDTH 4
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

typedef enum {
  MENU_STATE_MAINPAGE,    // we are in the main page
  MENU_STATE_SELECT_PAGE, // we select a page
  MENU_STATE_INSIDE_PAGE, // we are in a page
  MENU_STATE_EDIT_ITEM,   // we edit an item in the page
  MENU_STATE_EDIT_STRING, // we edit a string in the page
} scs_menu_state_t;



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

static s32 (*scsDelayedActionCallback)(u32 parameter);
static u32 scsDelayedActionParameter;
static u16 scsDelayedActionCtr;

static char scsMsg[2][SCS_MSG_MAX_CHAR+10+1]; // + some "margin" + zero terminator
static u16 scsMsgCtr;
static scs_msg_type_t scsMsgType;

static char scsActionString[SCS_MENU_ITEM_WIDTH+1];
static char scsEditString[SCS_LCD_COLUMNS_PER_DEVICE+1];
static u8 scsEditStringMaxChars;
static u8 scsEditPos;
static void (*scsEditStringCallback)(char *newString);

static u16 scsPinState;
static u16 scsPinStatePrev;
#if SCS_BUTTON_DEBOUNCE_RELOAD
static u8 scsButtonDebounceCtr;
#endif


static scs_menu_page_t *rootTable;
static u8 rootTableNumItems;
static u8 rootTableSelectedPage;
static u8 rootTableSelectedItem;

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
static s32 (*scsMainPageStringFunct)(char *line1, char *line2);
static s32 (*scsPageSelectStringFunct)(char *line1);
static s32 (*scsEncMainPageFunct)(s32 incrementer);
static s32 (*scsButtonMainPageFunct)(u8 button);




/////////////////////////////////////////////////////////////////////////////
//! Initialisation of Standard Control Surface
//! \param[in] mode currently only mode 0 supported
//! \return < 0 if initialisation failed
/////////////////////////////////////////////////////////////////////////////
s32 SCS_Init(u32 mode)
{
  SCS_LCD_Init(mode);
  SCS_LCD_InitSpecialChars(SCS_LCD_CHARSET_Menu);

  scsMenuState = MENU_STATE_MAINPAGE;

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
  scsActionString[0] = 0;
  scsEditString[0] = 0;
  scsEditStringMaxChars = 0;
  scsEditPos = 0;

  scsMsgCtr = 0;
  scsDelayedActionCtr = 0;

  scsPinState = 0xffff;
  scsPinStatePrev = 0xffff;
#if SCS_BUTTON_DEBOUNCE_RELOAD
  scsButtonDebounceCtr = 0;
#endif

  rootTable = NULL;
  rootTableNumItems = 0;
  rootTableSelectedPage = 0;

  scsMainPageStringFunct = NULL;
  scsPageSelectStringFunct = NULL;
  scsEncMainPageFunct = NULL;
  scsButtonMainPageFunct = NULL;

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
  // pass state of encoder to MIOS32_ENC
  u8 encoderState = 0;
  u16 maskEncA = (1 << SCS_PIN_ENC_MENU_A);
  u16 maskEncB = (1 << SCS_PIN_ENC_MENU_B);

  if( scsPinState & maskEncA )
    encoderState |= 1;
  if( scsPinState & maskEncB )
    encoderState |= 2;
  MIOS32_ENC_StateSet(SCS_ENC_MENU_ID, scsPinState);

  // ensure that change won't be propagated to DIN handler
  scsPinState &= ~(maskEncA | maskEncB);

  // no state update required for buttons (done from external)

#if SCS_BUTTON_DEBOUNCE_RELOAD
  // for button debouncing
  if( scsButtonDebounceCtr )
    --scsButtonDebounceCtr;
#endif


  // cursor timer handling is done here
  // it's only used if an item is edited
  if( scsMenuState == MENU_STATE_EDIT_ITEM || MENU_STATE_EDIT_STRING ) {
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
      if( ++displayCursorCtr < 800 ) {
	displayCursorOn = 1;
      } else if( displayCursorCtr == 800 ) {
	displayCursorOn = 0;
	displayUpdateReq = 1;
      } else if( displayCursorCtr < 1000 ) {
	displayCursorOn = 0;
      } else {
	displayCursorCtr = 0;
	displayCursorOn = 1;
	displayUpdateReq = 1;
      }
    }
  } else {
    displayCursorCtr = 0;
    displayFlickerSelectionCtr = 0;
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

  switch( scsMenuState ) {
  case MENU_STATE_SELECT_PAGE: {
    int newOffset = displayRootOffset + incrementer;
    if( newOffset < 0 )
      newOffset = 0;
    else if( (newOffset+SCS_NUM_MENU_ITEMS) >= rootTableNumItems ) {
      newOffset = rootTableNumItems - SCS_NUM_MENU_ITEMS;
      if( newOffset < 0 )
	newOffset = 0;
    }
    displayRootOffset = newOffset;
  } break;

  case MENU_STATE_INSIDE_PAGE: {
    if( rootTableSelectedPage >= rootTableNumItems )
      return -1; // fail safe
    scs_menu_page_t *selectedPage = (scs_menu_page_t *)&rootTable[rootTableSelectedPage];

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

  case MENU_STATE_EDIT_ITEM: {
    scs_menu_item_t *pageItem = NULL;
    if( rootTableSelectedPage < rootTableNumItems ) {
      scs_menu_item_t *pageItems = (scs_menu_item_t *)rootTable[rootTableSelectedPage].page;
      u8 numItems = rootTable[rootTableSelectedPage].numItems;
      if( rootTableSelectedItem < numItems )
	pageItem = (scs_menu_item_t *)&pageItems[rootTableSelectedItem];
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

  } break;

  case MENU_STATE_EDIT_STRING: {
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
  } break;

  default: // MENU_STATE_MAINPAGE
    if( scsEncMainPageFunct )
      scsEncMainPageFunct(incrementer);
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

  switch( pin ) {
  case SCS_PIN_MENU: {

    if( depressed ) {
      // deinstall callback if it was active
      scsDelayedActionCallback = 0;
      return 0; // no error
    }

    switch( scsMenuState ) {
    case MENU_STATE_SELECT_PAGE: {
      scsMenuState = MENU_STATE_MAINPAGE;
      displayInitReq = 1;
      scsMsgCtr = 0; // disable message
    } break;
    case MENU_STATE_INSIDE_PAGE: {
      scsMenuState = MENU_STATE_SELECT_PAGE;
      displayInitReq = 1;
      scsMsgCtr = 0; // disable message
    } break;
    case MENU_STATE_EDIT_ITEM: {
      scsMenuState = MENU_STATE_INSIDE_PAGE;
      scsMsgCtr = 0; // disable message
    } break;
    case MENU_STATE_EDIT_STRING: {
      scsMenuState = MENU_STATE_INSIDE_PAGE;
      scsMsgCtr = 0; // disable message
      scsEditStringCallback = NULL; // disable edit string callback
    }
      //default: // MENU_STATE_MAINPAGE
      //scsMenuState = MENU_STATE_SELECT_PAGE;
      // enter page only via soft buttons
    }
    SCS_ENC_MENU_AutoSpeedSet(1); // slow speed..
    displayUpdateReq = 1;
  } break;
  case SCS_PIN_SOFT1:  softButton = 0; break; // (allows pin mapping)
  case SCS_PIN_SOFT2:  softButton = 1; break;
  case SCS_PIN_SOFT3:  softButton = 2; break;
  case SCS_PIN_SOFT4:  softButton = 3; break;
  case SCS_PIN_SOFT5:  softButton = 4; break;
  case SCS_PIN_SOFT6:  softButton = 5; break;
  case SCS_PIN_SOFT7:  softButton = 6; break;
  case SCS_PIN_SOFT8:  softButton = 7; break;
  case SCS_PIN_SOFT9:  softButton = 8; break;
  case SCS_PIN_SOFT10: softButton = 9; break;
  default:
    return -1; // unsupported pin
  }

  if( softButton >= 0 ) {

    if( depressed ) {
      // deinstall callback if it was active
      scsDelayedActionCallback = 0;
      return 0; // no error
    }

    switch( scsMenuState ) {
    case MENU_STATE_SELECT_PAGE: {
      int newPage = displayRootOffset + softButton;
      if( newPage < rootTableNumItems ) {
	displayCursorPos = softButton;
	rootTableSelectedPage = newPage;
	displayPageOffset = 0; // optionally we got store the last offset somewhere?
	scsMenuState = MENU_STATE_INSIDE_PAGE;
	displayInitReq = 1;
      }
    } break;

    case MENU_STATE_INSIDE_PAGE:
    case MENU_STATE_EDIT_ITEM: {
      scs_menu_item_t *pageItems = NULL;
      u8 numItems = 0;
      if( rootTableSelectedPage < rootTableNumItems ) {
	pageItems = (scs_menu_item_t *)rootTable[rootTableSelectedPage].page;
	numItems = rootTable[rootTableSelectedPage].numItems;
      }

      int itemPos = displayPageOffset + softButton;
      if( itemPos < numItems ) {
	scs_menu_item_t *pageItem = (scs_menu_item_t *)&pageItems[itemPos];

	// edit item if maxValue >= 2
	// if maxValue == 1, we will toggle the value
	if( pageItem->maxValue != 1 ) {
	  displayCursorPos = softButton;
	  rootTableSelectedItem = itemPos;

	  if( scsMenuState == MENU_STATE_INSIDE_PAGE ) {
	    scsMenuState = MENU_STATE_EDIT_ITEM;
	    SCS_ENC_MENU_AutoSpeedSet(pageItem->maxValue);
	  }
	}

	u16 oldValue = pageItem->getFunct(pageItem->ix);
	u16 newValue = pageItem->selectFunct(pageItem->ix, oldValue);
	if( newValue == oldValue && pageItem->maxValue == 1 )
	  newValue ^= 1; // auto-toggle
	pageItem->setFunct(pageItem->ix, newValue);
      }

      // flicker menu item for 200 mS
      displayFlickerSelectionCtr = 200;
    } break;

    case MENU_STATE_EDIT_STRING: {
      switch( softButton ) {
      case 0: {
	if( scsEditStringCallback )
	  scsEditStringCallback(scsEditString);
	scsMenuState = MENU_STATE_INSIDE_PAGE;
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

    default: { // MENU_STATE_MAINPAGE
      u8 enterMenu = 1;

      // this hook can be used to change the root menu depending on the pressed soft button
      // if it returns < 0, the menu won't be entered!
      if( scsButtonMainPageFunct && scsButtonMainPageFunct(softButton) < 0 )
	enterMenu = 0;

      if( enterMenu ) {
	displayInitReq = 1;
	scsMenuState = MENU_STATE_SELECT_PAGE;
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
      s32 (*_scsDelayedActionCallback)(u32 parameter);
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

    switch( scsMenuState ) {
      /////////////////////////////////////////////////////////////////////////
    case MENU_STATE_SELECT_PAGE: {
      int i;

      char line1[SCS_MAX_STR];
      strcpy(line1, "Select Page:");
      if( scsPageSelectStringFunct )
	scsPageSelectStringFunct(line1);

      SCS_LCD_CursorSet(0, 0);
      SCS_LCD_PrintStringPadded(line1, SCS_LCD_MAX_COLUMNS);

      SCS_LCD_CursorSet(0, 1);
      for(i=0; i<SCS_NUM_MENU_ITEMS; ++i) {
	u8 page = displayRootOffset + i;
	if( page >= rootTableNumItems )
	  SCS_LCD_PrintSpaces(SCS_MENU_ITEM_WIDTH);
	else {
	  SCS_LCD_PrintStringPadded(rootTable[page].name, SCS_MENU_ITEM_WIDTH);
	}
      }

      // print arrow at upper right corner
      if( rootTableNumItems > SCS_NUM_MENU_ITEMS ) {
	SCS_LCD_CursorSet(SCS_LCD_MAX_COLUMNS-1, 0);
	if( displayRootOffset == 0 )
	  SCS_LCD_PrintChar(1); // right arrow
	else if( displayRootOffset >= (rootTableNumItems-SCS_NUM_MENU_ITEMS) )
	  SCS_LCD_PrintChar(0); // left arrow
	else
	  SCS_LCD_PrintChar(2); // left/right arrow
      }
    } break;

    /////////////////////////////////////////////////////////////////////////
    case MENU_STATE_INSIDE_PAGE:
    case MENU_STATE_EDIT_ITEM: {
      scs_menu_item_t *pageItems = NULL;
      u8 numItems = 0;
      if( rootTableSelectedPage < rootTableNumItems ) {
	pageItems = (scs_menu_item_t *)rootTable[rootTableSelectedPage].page;
	numItems = rootTable[rootTableSelectedPage].numItems;
      }

      if( !pageItems ) {
	SCS_LCD_CursorSet(0, 0);
	SCS_LCD_PrintStringPadded("!! ERROR !!", SCS_LCD_MAX_COLUMNS);
	SCS_LCD_CursorSet(0, 1);
	SCS_LCD_PrintStringPadded("!! INVALID PAGE ITEMS !!", SCS_LCD_MAX_COLUMNS);
      } else {
	// check for full screen message
	u8 printCommonPage = 1;
	if( scsMenuState == MENU_STATE_EDIT_ITEM ) {
	  scs_menu_item_t *pageItem = NULL;
	  if( rootTableSelectedItem < numItems )
	    pageItem = (scs_menu_item_t *)&pageItems[rootTableSelectedItem];

	  if( pageItem && pageItem->stringFullFunct ) {
	    printCommonPage = 0;

	    char line1[SCS_MAX_STR];
	    strcpy(line1, "???");
	    char line2[SCS_MAX_STR];
	    strcpy(line2, "???");
	    u16 value = pageItem->getFunct(pageItem->ix);
	    pageItem->stringFullFunct(pageItem->ix, value, line1, line2);

	    SCS_LCD_CursorSet(0, 0);
	    SCS_LCD_PrintStringPadded(line1, SCS_LCD_MAX_COLUMNS);
	    SCS_LCD_CursorSet(0, 1);
	    SCS_LCD_PrintStringPadded(line2, SCS_LCD_MAX_COLUMNS);
	  }
	}

	if( printCommonPage ) {
	  void (*itemsLineFunct)(u8 editMode, char *line) = NULL;
	  void (*valuesLineFunct)(u8 editMode, char *line) = NULL;
      
	  if( rootTableSelectedPage < rootTableNumItems ) {
	    itemsLineFunct = rootTable[rootTableSelectedPage].itemsLineFunct;
	    valuesLineFunct = rootTable[rootTableSelectedPage].valuesLineFunct;
	  }

	  u8 editMode = scsMenuState == MENU_STATE_EDIT_ITEM;

	  // first line
	  {
	    SCS_LCD_CursorSet(0, 0);
	    char line1[SCS_MAX_STR];
	    strcpy(line1, "???");
	    if( itemsLineFunct )
	      itemsLineFunct(editMode, line1);
	    if( line1[0] != 0 )
	      SCS_LCD_PrintStringPadded(line1, SCS_LCD_MAX_COLUMNS);
	  }

	  // second line
	  {
	    SCS_LCD_CursorSet(0, 1);
	    char line2[SCS_MAX_STR];
	    strcpy(line2, "???");
	    if( valuesLineFunct )
	      valuesLineFunct(editMode, line2);
	    if( line2[0] != 0 )
	      SCS_LCD_PrintStringPadded(line2, SCS_LCD_MAX_COLUMNS);
	  }

	  // print arrow at upper right corner
	  if( numItems > SCS_NUM_MENU_ITEMS ) {
	    SCS_LCD_CursorSet(SCS_LCD_MAX_COLUMNS-1, 0);
	    if( displayPageOffset == 0 )
	      SCS_LCD_PrintChar(1); // right arrow
	    else if( displayPageOffset >= (numItems-SCS_NUM_MENU_ITEMS) )
	      SCS_LCD_PrintChar(0); // left arrow
	    else
	      SCS_LCD_PrintChar(2); // left/right arrow
	  }
	}
      }
    } break;

    /////////////////////////////////////////////////////////////////////////
    case MENU_STATE_EDIT_STRING: {
      // check if empty string
      int i;
      u8 stringIsEmpty = 1;
      for(i=0; i<scsEditStringMaxChars; ++i)
	if( scsEditString[i] != ' ' ) {
	  stringIsEmpty = 0;
	  break;
	}

      // edited string
      SCS_LCD_CursorSet(0, 0);      
      SCS_LCD_PrintStringPadded(stringIsEmpty ? "<empty>" : scsEditString, SCS_LCD_COLUMNS_PER_DEVICE);

      // set cursor
      if( !displayCursorOn || (displayFlickerSelectionCtr && displayLabelOn) ) {
	SCS_LCD_CursorSet(scsEditPos, 0);
	SCS_LCD_PrintChar((scsEditString[scsEditPos] == '*') ? ' ' : '*');
      }

      // edit functions
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
    } break;

    /////////////////////////////////////////////////////////////////////////
    default: { // MENU_STATE_MAINPAGE
      char line1[SCS_MAX_STR];
      strncpy(line1, MIOS32_LCD_BOOT_MSG_LINE1, SCS_LCD_MAX_COLUMNS);
      char line2[SCS_MAX_STR];
      strcpy(line2, "Press soft button");

      if( scsMainPageStringFunct )
	scsMainPageStringFunct(line1, line2);

      SCS_LCD_CursorSet(0, 0);
      SCS_LCD_PrintStringPadded(line1, SCS_LCD_MAX_COLUMNS);
      SCS_LCD_CursorSet(0, 1);
      SCS_LCD_PrintStringPadded(line2, SCS_LCD_MAX_COLUMNS);
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
//! Standard function to print the items of a page
//! \param[in] editMode 0 if page is in scroll mode, 1 if value is edited
//! \param[out] line copy the line into this string
/////////////////////////////////////////////////////////////////////////////
void SCS_StringStandardItems(u8 editMode, char *line)
{
  scs_menu_item_t *pageItems = (scs_menu_item_t *)rootTable[rootTableSelectedPage].page;
  u8 numItems = rootTable[rootTableSelectedPage].numItems;

  // TK: the user would put the string into *line
  // but internally we can also use SCS_LCD_* to simplify the output if line[0] set to 0
  line[0] = 0;

  // (shouldn't be used externally to allow a proper implementation later)
  int i;
  for(i=0; i<SCS_NUM_MENU_ITEMS; ++i) {
    u8 item = displayPageOffset + i;
    if( item >= numItems ||
	(editMode && item == rootTableSelectedItem && !displayLabelOn ) )
      SCS_LCD_PrintSpaces(SCS_MENU_ITEM_WIDTH);
    else {
      SCS_LCD_PrintStringPadded(pageItems[item].name, SCS_MENU_ITEM_WIDTH);
    }
  }
}

/////////////////////////////////////////////////////////////////////////////
//! Standard function to print the values of a page
//! \param[in] editMode 0 if page is in scroll mode, 1 if value is edited
//! \param[out] line copy the line into this string
/////////////////////////////////////////////////////////////////////////////
void SCS_StringStandardValues(u8 editMode, char *line)
{
  scs_menu_item_t *pageItems = (scs_menu_item_t *)rootTable[rootTableSelectedPage].page;
  u8 numItems = rootTable[rootTableSelectedPage].numItems;

  // TK: the user would put the string into *line
  // but internally we can also use SCS_LCD_* to simplify the output if line[0] set to 0
  line[0] = 0;

  // (shouldn't be used externally to allow a proper implementation later)
  int i;
  for(i=0; i<SCS_NUM_MENU_ITEMS; ++i) {
    u8 item = displayPageOffset + i;
    if( item >= numItems ||
	(editMode && item == rootTableSelectedItem && !displayCursorOn ) )
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

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
//! Installs string function for main page (2x20 chars in both lines)
//! \param[in] printFunct pointer to print function
//! \return < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 SCS_InstallMainPageStringHook(s32 (*stringFunct)(char *line1, char *line2))
{
  scsMainPageStringFunct = stringFunct;

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! Installs string function for selection page (20 chars in upper line)
//! \param[in] printFunct pointer to print function
//! \return < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 SCS_InstallPageSelectStringHook(s32 (*stringFunct)(char *line1))
{
  scsPageSelectStringFunct = stringFunct;

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! Installs encoder handler for main page
//! \param[in] encFunct the encoder function
//! \return < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 SCS_InstallEncMainPageHook(s32 (*encFunct)(s32 incrementer))
{
  scsEncMainPageFunct = encFunct;

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! Installs button handler for main page
//! \param[in] buttonFunct the button function
//! \return < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 SCS_InstallButtonMainPageHook(s32 (*buttonFunct)(u8 softButton))
{
  scsButtonMainPageFunct = buttonFunct;

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
s32 SCS_InstallDelayedActionCallback(void *callback, u16 delay_mS, u32 parameter)
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
s32 SCS_UnInstallDelayedActionCallback(void *callback)
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
s32 SCS_InstallEditStringCallback(void *callback, char *actionString, char *initialString, u8 maxChars)
{
  if( maxChars >= SCS_LCD_COLUMNS_PER_DEVICE )
    maxChars = SCS_LCD_COLUMNS_PER_DEVICE;
  memcpy(scsEditString, initialString, maxChars);
  scsEditString[maxChars] = 0; // makes sense to set terminator for the case that it isn't available in initialString
  memcpy(scsActionString, actionString, SCS_MENU_ITEM_WIDTH);
  scsActionString[SCS_MENU_ITEM_WIDTH] = 0; // ditto.
  scsEditStringMaxChars = maxChars;

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

  scsEditStringCallback = callback;
  scsMenuState = MENU_STATE_EDIT_STRING;

  return 0;
}


//! \}
