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
//! number of menu items which are displayed on screen
//! each item allocates 4x2 characters
//! #define SCS_NUM_MENU_ITEMS 5
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


/////////////////////////////////////////////////////////////////////////////
// Local Definitions
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Local types
/////////////////////////////////////////////////////////////////////////////

typedef enum {
  MENU_STATE_MAINPAGE,    // we are in the main page
  MENU_STATE_SELECT_PAGE, // we select a page
  MENU_STATE_INSIDE_PAGE, // we are in a page
  MENU_STATE_EDIT_ITEM,   // we edit an item in the page
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

static u16 scsPinState;
static u16 scsPinStateChanged;

static scs_menu_page_t *rootTable;
static u8 rootTableNumItems;
static u8 rootTableSelectedPage;
static u8 rootTableSelectedItem;

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

  scsPinState = 0xff;
  scsPinStateChanged = 0x00;

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
  u16 newState = depressed ? (scsPinState | mask) : (scsPinState & ~mask);
  scsPinStateChanged |= newState ^ scsPinState;
  scsPinState = newState;
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
  MIOS32_IRQ_Disable(); // should be atomic
  scsPinStateChanged |= scsPinState ^ newState;
  scsPinState = newState;
  MIOS32_IRQ_Enable();

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
  if( scsPinState & (1 << SCS_PIN_ENC_MENU_A) )
    encoderState |= 1;
  if( scsPinState & (1 << SCS_PIN_ENC_MENU_B) )
    encoderState |= 2;
  MIOS32_ENC_StateSet(SCS_ENC_MENU_ID, scsPinState);

  // no state update required for buttons (done from external)

  // cursor timer handling is done here
  // it's only used if an item is edited
  if( scsMenuState == MENU_STATE_EDIT_ITEM ) {
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
  // ignore if button depressed
  if( depressed )
    return 0; // no error

  int softButton = -1;

  switch( pin ) {
  case SCS_PIN_MENU: {
    switch( scsMenuState ) {
    case MENU_STATE_SELECT_PAGE: {
      scsMenuState = MENU_STATE_MAINPAGE;
      displayInitReq = 1;
    } break;
    case MENU_STATE_INSIDE_PAGE: {
      scsMenuState = MENU_STATE_SELECT_PAGE;
      displayInitReq = 1;
    } break;
    case MENU_STATE_EDIT_ITEM: {
      scsMenuState = MENU_STATE_INSIDE_PAGE;
    } break;
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
    switch( scsMenuState ) {
    case MENU_STATE_SELECT_PAGE: {
      int newPage = displayRootOffset + softButton;
      if( newPage < rootTableNumItems ) {
	displayCursorPos = softButton;
	rootTableSelectedPage = newPage;
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

  // should be atomic
  MIOS32_IRQ_Disable();
  u16 pinState = scsPinState;
  u16 changedState = scsPinStateChanged;
  scsPinStateChanged = 0;
  MIOS32_IRQ_Enable();

  // check each button
  int pin;
  u16 mask = (1 << 0);
  for(pin=0; pin<8; ++pin, mask <<= 1) {
    if( pin == SCS_PIN_ENC_MENU_A ||
	pin == SCS_PIN_ENC_MENU_B )
      continue;

    if( changedState & mask )
      SCS_DIN_NotifyToggle(pin, (pinState & mask) ? 1 : 0);
  }


  ///////////////////////////////////////////////////////////////////////////
  // handle LCD
  ///////////////////////////////////////////////////////////////////////////
  if( displayInitReq ) {
    displayInitReq = 0;
    MIOS32_LCD_Clear();
  }

  if( displayUpdateReq ) {
    displayUpdateReq = 0;

    switch( scsMenuState ) {
    /////////////////////////////////////////////////////////////////////////
    case MENU_STATE_SELECT_PAGE: {
      int i;

      char line1[100]; // 21 should be enough, but just to prevent buffer overruns a bit...
      strcpy(line1, "Select Page:");
      if( scsPageSelectStringFunct )
	scsPageSelectStringFunct(line1);

      MIOS32_LCD_CursorSet(0, 0);
      MIOS32_LCD_PrintString(line1);

      MIOS32_LCD_CursorSet(0, 1);
      for(i=0; i<SCS_NUM_MENU_ITEMS; ++i) {
	u8 page = displayRootOffset + i;
	if( page >= rootTableNumItems )
	  MIOS32_LCD_PrintString("    ");
	else {
	  MIOS32_LCD_PrintString(rootTable[page].name);
	}
      }
    } break;

    /////////////////////////////////////////////////////////////////////////
    case MENU_STATE_INSIDE_PAGE:
    case MENU_STATE_EDIT_ITEM: {
      int i;
      scs_menu_item_t *pageItems = NULL;
      u8 numItems = 0;
      if( rootTableSelectedPage < rootTableNumItems ) {
	pageItems = (scs_menu_item_t *)rootTable[rootTableSelectedPage].page;
	numItems = rootTable[rootTableSelectedPage].numItems;
      }

      // check for full screen message
      u8 printCommonPage = 1;
      if( scsMenuState == MENU_STATE_EDIT_ITEM ) {
	scs_menu_item_t *pageItem = NULL;
	if( rootTableSelectedItem < numItems )
	  pageItem = (scs_menu_item_t *)&pageItems[rootTableSelectedItem];

	if( pageItem && pageItem->stringFullFunct ) {
	  printCommonPage = 0;

	  char line1[100]; // 21 should be enough, but just to prevent buffer overruns a bit...
	  strcpy(line1, "???");
	  char line2[100];
	  strcpy(line2, "???");
	  u16 value = pageItem->getFunct(pageItem->ix);
	  pageItem->stringFullFunct(pageItem->ix, value, line1, line2);

	  MIOS32_LCD_CursorSet(0, 0);
	  MIOS32_LCD_PrintString(line1);
	  MIOS32_LCD_CursorSet(0, 1);
	  MIOS32_LCD_PrintString(line2);
	}
      }

      if( printCommonPage ) {
	MIOS32_LCD_CursorSet(0, 0);
	for(i=0; i<SCS_NUM_MENU_ITEMS; ++i) {
	  u8 item = displayPageOffset + i;
	  if( item >= numItems ||
	      (scsMenuState == MENU_STATE_EDIT_ITEM && item == rootTableSelectedItem && !displayLabelOn ) )
	    MIOS32_LCD_PrintString("    ");
	  else {
	    MIOS32_LCD_PrintString(pageItems[item].name);
	  }
	}

	MIOS32_LCD_CursorSet(0, 1);
	for(i=0; i<SCS_NUM_MENU_ITEMS; ++i) {
	  u8 item = displayPageOffset + i;
	  if( item >= numItems ||
	      (scsMenuState == MENU_STATE_EDIT_ITEM && item == rootTableSelectedItem && !displayCursorOn ) )
	    MIOS32_LCD_PrintString("    ");
	  else {
	    scs_menu_item_t *pageItem = (scs_menu_item_t *)&pageItems[item];
	    char label[100]; // 5 should be enough, but just to prevent buffer overruns a bit...
	    strcpy(label, "??? "); // default
	    u16 value = pageItem->getFunct(pageItem->ix);
	    pageItem->stringFunct(pageItem->ix, value, label);
	    MIOS32_LCD_PrintString(label);
	  }
	}
      }
    } break;

    /////////////////////////////////////////////////////////////////////////
    default: { // MENU_STATE_MAINPAGE
      char line1[100]; // 21 should be enough, but just to prevent buffer overruns a bit...
      strncpy(line1, MIOS32_LCD_BOOT_MSG_LINE1, 20);
      char line2[100];
      strcpy(line2, "Press soft button");

      if( scsMainPageStringFunct )
	scsMainPageStringFunct(line1, line2);

      MIOS32_LCD_CursorSet(0, 0);
      MIOS32_LCD_PrintString(line1);
      MIOS32_LCD_CursorSet(0, 1);
      MIOS32_LCD_PrintString(line2);
    }
    }
  }

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

//! \}
