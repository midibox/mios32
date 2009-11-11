// $Id: app.c 690 2009-08-04 22:49:33Z tk $
/*
 * MIOS32 Application Template
 *
 * ==========================================================================
 *
 *  Copyright (C) <year> <your name> (<your email address>)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

/////////////////////////////////////////////////////////////////////////////
// Include files
/////////////////////////////////////////////////////////////////////////////

#include <mios32.h>
#include "app.h"
#include <uwindows.h>

static UW_Window	HelloIcon;
static UW_Window	HelloForm;
static UW_Window	HelloBtn,HelloLabel;
static const char IconCaption[]="Welcome to MB UWindows";
static char LabelCaption[] = "Hello UWindows!";
static char BtnCaption[]= "Show/Hide";

/////////////////////////////////////////////////////////////////////////////
// This hook is called after startup to initialize the application
/////////////////////////////////////////////////////////////////////////////
void APP_Init(void)
{
    // initialize all LEDs
    MIOS32_BOARD_LED_Init(0xffffffff);
    UW_Init();

}

/* Handler of Icon Click to show Form	*/
void HelloEnter(UW_Window * pWindow)
{
	UW_SetVisible(&HelloForm,1);
}

/* Handler of Form Click to Hide Form	*/
void HelloExit(UW_Window * pWindow)
{
	UW_SetVisible(&HelloForm,0);
}

/* Handler of Btn Click to toggle vision of Label	*/
void HelloBtnClick(UW_Window * PWindow)
{
	if (HelloLabel.visible == 1)
		UW_SetVisible(&HelloLabel,0);
	else
		UW_SetVisible(&HelloLabel,1);
}

/////////////////////////////////////////////////////////////////////////////
// This task is running endless in background
/////////////////////////////////////////////////////////////////////////////
void APP_Background(void)
{

  	UW_Add(&HelloIcon, UW_ICON, &UWindows, NULL);
	/* Set Position of the Icon	in Main Window	*/
	UW_SetPosition(&HelloIcon, 4, 55);
	/* Set Icon	caption in Main Window	*/
	UW_SetCaption(&HelloIcon, IconCaption, 22);
	/* SetThe Click Handler routine for the Icon to show form	*/
	UW_SetOnClick(&HelloIcon, HelloEnter);

	/* Add Form to Main Window	and Hide it	*/
	UW_Add(&HelloForm, UW_FORM, &UWindows, NULL);
	/* Set Form	caption */
	UW_SetCaption(&HelloForm, IconCaption, 22);
	/* Hide the Form in Main Window	*/
	UW_SetVisible(&HelloForm, 1);
	/* SetThe Click Handler routine for the form to hide the form	*/
	UW_SetOnClick(&HelloForm, HelloExit);

	/* Add Label to Hello Form		*/
	UW_Add(&HelloLabel,UW_LABEL, &HelloForm, NULL);
	/* Set Form	caption */
	UW_SetCaption(&HelloLabel, LabelCaption, 15);
	UW_SetPosition(&HelloLabel, 5, 56);
	UW_SetSize(&HelloLabel, 115, 20);
	
	/* Add Btn to Hello Form			*/
	UW_Add(&HelloBtn,UW_BUTTON, &HelloForm, NULL);
	/* Set Button	caption */
	UW_SetCaption(&HelloBtn, BtnCaption, 9);
	UW_SetPosition(&HelloBtn, 24, 20);
	UW_SetSize(&HelloBtn, 80, 30);
	/* Set the Click Handler routine for the button to Toggle Lable visibility	*/
	UW_SetOnClick(&HelloBtn,HelloBtnClick);
	// endless loop
  while( 1 ) {
	UW_Run();
    // toggle the state of all LEDs (allows to measure the execution speed with a scope)
    MIOS32_BOARD_LED_Set(0xffffffff, ~MIOS32_BOARD_LED_Get());

  }
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called when a MIDI package has been received
/////////////////////////////////////////////////////////////////////////////
void APP_MIDI_NotifyPackage(mios32_midi_port_t port, mios32_midi_package_t midi_package)
{
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called before the shift register chain is scanned
/////////////////////////////////////////////////////////////////////////////
void APP_SRIO_ServicePrepare(void)
{
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called after the shift register chain has been scanned
/////////////////////////////////////////////////////////////////////////////
void APP_SRIO_ServiceFinish(void)
{
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called when a button has been toggled
// pin_value is 1 when button released, and 0 when button pressed
/////////////////////////////////////////////////////////////////////////////
void APP_DIN_NotifyToggle(u32 pin, u32 pin_value)
{
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called when an encoder has been moved
// incrementer is positive when encoder has been turned clockwise, else
// it is negative
/////////////////////////////////////////////////////////////////////////////
void APP_ENC_NotifyChange(u32 encoder, s32 incrementer)
{
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called when a pot has been moved
/////////////////////////////////////////////////////////////////////////////
void APP_AIN_NotifyChange(u32 pin, u32 pin_value)
{
}
