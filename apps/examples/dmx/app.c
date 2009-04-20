// $Id: app.c 346 2009-02-10 22:10:39Z tk $
/*
 * Demo application for DMX Controller.
 * It used a DOG GLCD but could be adapted for any LCD.
 * It has 2 banks of 12 faders (BANK A and BANKB)
 * Plus 2 master faders (MASTER A and MASTER B
 * It is currently just a simple 12 channel desk
 *
 * ==========================================================================
 *
 *  Copyright (C) 2008 Thorsten Klose (tk@midibox.org)
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
#include "dmx.h"
#include <glcd_font.h>


/////////////////////////////////////////////////////////////////////////////
// This hook is called after startup to initialize the application
/////////////////////////////////////////////////////////////////////////////
void APP_Init(void)
{
  MIOS32_BOARD_LED_Init(0xffffffff); // initialize all LEDs
  DMX_Init(0);
}
u8 faders[26];
u8 banka[12];
u8 bankb[12];
u8 mastera, masterb;
/////////////////////////////////////////////////////////////////////////////
// This task is running endless in background
/////////////////////////////////////////////////////////////////////////////
void APP_Background(void)
{
  // print static screen

  // clear LCD
  MIOS32_LCD_Clear();
  MIOS32_LCD_FontInit((u8 *)GLCD_FONT_NORMAL);
  MIOS32_LCD_Clear();
	MIOS32_LCD_GCursorSet(12, 0);
	MIOS32_LCD_PrintString("Welcome to MB-DMX");


  MIOS32_LCD_FontInit((u8 *)GLCD_FONT_FADERS);
	u8 f,g;
	for (f=0;f<2;f++)
	{
		MIOS32_LCD_GCursorSet(0, (f*16)+16);
		for (g=0;g<12;g++)
			MIOS32_LCD_PrintChar(g);

	}
	MIOS32_LCD_GCursorSet(13*4, 24);
	MIOS32_LCD_PrintChar(0);
	MIOS32_LCD_PrintChar(0);
  MIOS32_LCD_FontInit((u8 *)GLCD_FONT_NORMAL);
 
	for (f=0;f<26;f++)
		faders[f]=0;

  // endless loop
  while( 1 )
	{
		for (f=0;f<24;f++)
		{
			if (f<12) {
				if (faders[f]!=banka[f]) {
					banka[f]=faders[f];
					MIOS32_LCD_GCursorSet(f*4, 16);
					MIOS32_LCD_FontInit((u8 *)GLCD_FONT_FADERS);
					MIOS32_LCD_PrintChar(banka[f]>>4);
					MIOS32_LCD_FontInit((u8 *)GLCD_FONT_NORMAL);
					int q=(((banka[f]*mastera)>>8)|((bankb[f]*(u8)~masterb)>>8));
					DMX_SetChannel(f,(u8)q);
					//MIOS32_MIDI_SendDebugMessage("Channel: %d = %d\n",f,(u8)q);

					
				}
			} else if ((f>11) && (f<24)){
				if (faders[f]!=bankb[f-12]) {
					bankb[f-12]=faders[f];
					MIOS32_LCD_GCursorSet((f-12)*4, 32);
					MIOS32_LCD_FontInit((u8 *)GLCD_FONT_FADERS);
					MIOS32_LCD_PrintChar(bankb[f-12]>>4);
					MIOS32_LCD_FontInit((u8 *)GLCD_FONT_NORMAL);
					int q=(((banka[f-12]*mastera)>>8)|((bankb[f-12]*(u8)~masterb)>>8));
					DMX_SetChannel(f-12,(u8)q);
					//MIOS32_MIDI_SendDebugMessage("Channel: %d = %d\n",f-12,(u8)q);

				}
			}
		}
		if ((faders[24]!=mastera) || (faders[25]!=masterb)) {
			// If either of the master faders have moved, we must recalculate the whole universe.
			mastera=faders[24];
			masterb=faders[25];
			MIOS32_LCD_FontInit((u8 *)GLCD_FONT_FADERS);
			MIOS32_LCD_GCursorSet(13*4, 24);
			MIOS32_LCD_PrintChar((u8)mastera>>4);
			MIOS32_LCD_PrintChar((u8)masterb>>4);
			MIOS32_MIDI_SendDebugMessage("Master Values: A=%d  B=%d\n",mastera,(u8)~masterb);
			MIOS32_LCD_FontInit((u8 *)GLCD_FONT_NORMAL);
			for (g=0;g<12;g++) {
				int q=(((banka[g]*mastera)>>8)|((bankb[g]*(u8)~masterb)>>8));
				DMX_SetChannel(g,(u8)q);
			}
		}
	}
}


/////////////////////////////////////////////////////////////////////////////
//  This hook is called when a complete MIDI event has been received
/////////////////////////////////////////////////////////////////////////////
void APP_NotifyReceivedEvent(mios32_midi_port_t port, mios32_midi_package_t midi_package)
{
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called when a SysEx byte has been received
/////////////////////////////////////////////////////////////////////////////
void APP_NotifyReceivedSysEx(mios32_midi_port_t port, u8 sysex_byte)
{
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called when a byte has been received via COM interface
/////////////////////////////////////////////////////////////////////////////
void APP_NotifyReceivedCOM(mios32_com_port_t port, u8 byte)
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
	pin=((pin%4)<<3)+(pin>>2);	// change muxed pin order.
	pin_value=pin_value>>4;
	
	if (pin<26)
		faders[pin]=pin_value;
		
}
