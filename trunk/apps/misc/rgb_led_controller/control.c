/////////////////////////////////////////////////////////////////////////////
// Include files
/////////////////////////////////////////////////////////////////////////////

#import <mios32.h>
#import "externalheaders.h"
#import "control.h"
#import "sysex.h"

/////////////////////////////////////////////////////////////////////////////
// Static definitions
/////////////////////////////////////////////////////////////////////////////

const control_t initcontrol = {0x0f};

const u8 control_rows = 8;
const u8 control_columns = 8;
//the configuration of the board. the numbering starts in the upper left corner, goes down first and then right, and starts from 0
//	|0|5| 
//	|1|6|
//	|2|7|
//	|3|8|
//	|4|9|
//	etc..
//
const u8 controllers[DISPLAY_LED_NUMBER] = 
{
controllerTypeSmallButton, 
controllerTypeSmallButton, 
controllerTypeSmallButton, 
controllerTypeSmallButton, 
controllerTypeSmallButton, 
controllerTypeSmallButton, 
controllerTypeSmallButton, 
controllerTypeSmallButton, 
//
controllerTypeSmallButton, 
controllerTypeSmallButton, 
controllerTypeSmallButton, 
controllerTypeSmallButton, 
controllerTypeSmallButton, 
controllerTypeSmallButton, 
controllerTypeSmallButton, 
controllerTypeSmallButton,
//
controllerTypeSmallButton, 
controllerTypeSmallButton, 
controllerTypeSmallButton, 
controllerTypeSmallButton, 
controllerTypeSmallButton, 
controllerTypeSmallButton, 
controllerTypeSmallButton, 
controllerTypeSmallButton, 
//
controllerTypeSmallButton, 
controllerTypeSmallButton, 
controllerTypeSmallButton, 
controllerTypeSmallButton, 
controllerTypeSmallButton, 
controllerTypeSmallButton, 
controllerTypeSmallButton, 
controllerTypeSmallButton,
//
controllerTypeSmallButton, 
controllerTypeSmallButton, 
controllerTypeSmallButton, 
controllerTypeSmallButton, 
controllerTypeSmallButton, 
controllerTypeSmallButton, 
controllerTypeSmallButton, 
controllerTypeSmallButton, 
//
controllerTypeSmallButton, 
controllerTypeSmallButton, 
controllerTypeSmallButton, 
controllerTypeSmallButton, 
controllerTypeSmallButton, 
controllerTypeSmallButton, 
controllerTypeSmallButton, 
controllerTypeSmallButton,
//
controllerTypeSmallButton, 
controllerTypeSmallButton, 
controllerTypeSmallButton, 
controllerTypeSmallButton, 
controllerTypeSmallButton, 
controllerTypeSmallButton, 
controllerTypeSmallButton, 
controllerTypeSmallButton, 
//
controllerTypeSmallButton, 
controllerTypeSmallButton, 
controllerTypeSmallButton, 
controllerTypeSmallButton, 
controllerTypeSmallButton, 
controllerTypeSmallButton, 
controllerTypeSmallButton, 
controllerTypeSmallButton,
};

/////////////////////////////////////////////////////////////////////////////
// Global Variables
/////////////////////////////////////////////////////////////////////////////

control_t controls[DISPLAY_LED_NUMBER]; //the data for all controls
u16 doubleClickTime = CONTROL_DOUBLECLICK_TIME;
u16 longClickTime = CONTROL_LONGCLICK_TIME;
u8 inputsEnabled = 1;

#import "control_config.c"

/////////////////////////////////////////////////////////////////////////////
// Enable/Disable Inputs
/////////////////////////////////////////////////////////////////////////////
void CONTROL_SetInputState(u8 state)
{
	inputsEnabled = state;
}

/////////////////////////////////////////////////////////////////////////////
// Initialize the Controls
/////////////////////////////////////////////////////////////////////////////
void CONTROL_Init()
{
	//initialize stopwatch for doubleclick and long click measurement
	MIOS32_STOPWATCH_Init(1000); // 1 ms resolution is more than enough
	
	u16 i;	
	//Initialize the control structure
	for( i = 0; i < control_rows*control_columns;i++) {
		controls[i] = initcontrol;
		controls[i].FlashSteps = DISPLAY_FLASH_STEPS;  //this is fix now, but could be changed on a per-control basis
		controls[i].BlinkSteps = DISPLAY_BLINK_STEPS;
		controls[i].TouchSteps = DISPLAY_TOUCH_STEPS;
		controls[i].ChangeSteps = DISPLAY_CHANGE_STEPS;  
	}
	
	//go through the controllers array and initialize the encoders
	mios32_enc_config_t	encoder;
	u8 number;
	u8 j = 0;
	encoder.cfg.type = DETENTED2;
	encoder.cfg.speed = FAST;
	encoder.cfg.speed_par = 1;
	for (i=0;i<control_rows*control_columns;i++) {
		number = control_pin_config[i];
		//do we have an encoder?
		if (controllers[number] == controllerTypeEncoder) {
#if DEBUG_VERBOSE_LEVEL >= 1
			MIOS32_MIDI_SendDebugMessage("found encoder for number %02d pin %02d", number, i);
#endif
			encoder.cfg.sr = div(i,8).quot + 1;
			encoder.cfg.pos = div(i,8).rem;
			//if the order of the pins is opposite to that of the numbers we need to get the other pin of the encoder.
			if (div(encoder.cfg.pos,2).rem == 1) {
#if DEBUG_VERBOSE_LEVEL >= 1
				MIOS32_MIDI_SendDebugMessage("adjusting pos...");				
#endif
				encoder.cfg.pos -= 1;
			}
#if DEBUG_VERBOSE_LEVEL >= 1
			MIOS32_MIDI_SendDebugMessage("sr is %02d, pos is %02d", encoder.cfg.sr, encoder.cfg.pos);
#endif
			MIOS32_ENC_ConfigSet(number, encoder);
		}
	}
}

/////////////////////////////////////////////////////////////////////////////
// Set a control to a value
/////////////////////////////////////////////////////////////////////////////
void CONTROL_SetValue(u8 number, u8 value)
{
	//we are now set by a preset
	controls[number].Settings |= CONTROL_INPRESET_FLAG;
	controls[number].Value = value;
#if DEBUG_VERBOSE_LEVEL >= 2
	MIOS32_MIDI_SendDebugMessage("CONTROL_SetValue %d", controls[number].Value);
#endif	
	DISPLAY_UpdateColor(number);
	
	if (controls[number].Type == controlTypeValue) {
		//disable blinking to signalize "not in preset"
		DISPLAY_SetBlink(number, DISPLAY_BLINK_OFF);
	}	
}

/////////////////////////////////////////////////////////////////////////////
// Update time Intervals for multiple and long clicks
/////////////////////////////////////////////////////////////////////////////
extern void CONTROL_setIntervals(u8* ptr)
{
	doubleClickTime = *((u16*)ptr);
	ptr += 2;
	longClickTime = *((u16*)ptr);
#if DEBUG_VERBOSE_LEVEL >= 1
	MIOS32_MIDI_SendDebugMessage("CONTROL_SetIntervals: doubleclicktime %d holdinterval %d", doubleClickTime, longClickTime);
#endif
}

/////////////////////////////////////////////////////////////////////////////
// Update internal control values after receiving a control
/////////////////////////////////////////////////////////////////////////////
extern void CONTROL_UpdateControl(u8 number)
{
	//this routine is called for each control that is newly set by a setchange or other update
	//makes all the settings that are encoded in the settings variable, 
	//calculates extra data that would be to expensive to calculate during interaction
	
	//calculate minValue and maxValue colors for encoder
	
	//for now, we take black and white
	controls[number].maxValueColor.Red = 255;
	controls[number].maxValueColor.Green = 255;
	controls[number].maxValueColor.Blue= 255;

	controls[number].minValueColor.Red = 0;
	controls[number].minValueColor.Green = 0;
	controls[number].minValueColor.Blue = 0;
		
	//see if we need to blink if we are a clip button
	if (controls[number].Settings & CONTROL_CLIPSET_FLAG) {
		if (controls[number].Value == CONTROL_BUTTON_OFF) {
			CONTROL_ClipHandler(number, AB_CMD_CLIP_SET);
		} else {
			CONTROL_ClipHandler(number, AB_CMD_CLIP_RERUN);			
		}
	}
	
	//see if we need to blink if we are a "Set Value" type
	if (controls[number].Settings & CONTROL_INPRESET_FLAG) {
		DISPLAY_SetBlink(number, DISPLAY_BLINK_OFF);
	} else {
		DISPLAY_SetBlink(number, DISPLAY_BLINK_ON);		
	}
	
	DISPLAY_UpdateColor(number);
}

/////////////////////////////////////////////////////////////////////////////
// Handle Clip States
/////////////////////////////////////////////////////////////////////////////
void CONTROL_ClipHandler(u8 number, u8 clipState)
{
	//TODO: For now the flash color here is touch color, maybe use an extra color in the future?
	control_t control = controls[number];
	if (clipState == AB_CMD_CLIP_SET) {
		//stop blinking if receiving this a second time. that means invert blinking here
		controls[number].Value = CONTROL_BUTTON_OFF; //transition from off to run
		//invert the flag, invert the blink state
		controls[number].Settings ^= CONTROL_CLIPSET_FLAG;
		DISPLAY_SetBlink(number, controls[number].Blink == DISPLAY_BLINK_OFF ? DISPLAY_BLINK_ON : DISPLAY_BLINK_OFF);
	}
	if (clipState == AB_CMD_CLIP_RERUN) {
		controls[number].Value = CONTROL_BUTTON_ON; //re-run
		controls[number].Settings |= CONTROL_CLIPSET_FLAG;
		DISPLAY_SetBlink(number, DISPLAY_BLINK_ON);
	}
	if (clipState == AB_CMD_CLIP_RUN) {
		//if coming from button off then flash
		if (controls[number].Value == CONTROL_BUTTON_OFF) {
			DISPLAY_Flash(number, &controls[number].touchColor);
		}
		controls[number].Value = CONTROL_BUTTON_ON;
		DISPLAY_SetBlink(number, DISPLAY_BLINK_OFF);
		controls[number].Settings &= !CONTROL_CLIPSET_FLAG;
	}
	if (clipState == AB_CMD_CLIP_STOP) {
		controls[number].Value = CONTROL_BUTTON_OFF;
		DISPLAY_SetBlink(number, DISPLAY_BLINK_OFF);
		controls[number].Settings &= !CONTROL_CLIPSET_FLAG;
	}
	if (clipState == AB_CMD_CLIP_START) {
		//it can be that we did not know about this running clip. to be sure, set the value here again
		if (controls[number].Value != CONTROL_BUTTON_ON) {
			controls[number].Value = CONTROL_BUTTON_ON;
			DISPLAY_UpdateColor(number);
			controls[number].Settings &= !CONTROL_CLIPSET_FLAG;
		}
		DISPLAY_Flash(number, &controls[number].touchColor);
	}
}

/////////////////////////////////////////////////////////////////////////////
// Handle buttons
/////////////////////////////////////////////////////////////////////////////
void CONTROL_ButtonHandler(u32 pin, u32 pin_value)
{
	MIOS32_MIDI_SendDebugMessage("Button");	
	
	if (inputsEnabled == 1) {

		MIOS32_MIDI_SendDebugMessage("Enabled");	

		u8 number;
		u8 command;
		number = control_pin_config[pin];
		control_t control = controls[number];
	
#if DEBUG_VERBOSE_LEVEL >= 2
		MIOS32_MIDI_SendDebugMessage("Button pressed pin %d number %d value %d", pin,number, pin_value);
		MIOS32_MIDI_SendDebugMessage("control type %02d", control.Type);
#endif
	
		if (pin_value == 0) { //button down
	#if DEBUG_VERBOSE_LEVEL >= 2		
			MIOS32_MIDI_SendDebugMessage("--DOWN-NUMBER-%d----------------", number);			
	#endif		
			//see if we want a touch flash
			if (control.Settings & CONTROL_USETOUCH_FLAG) {
				DISPLAY_Touch(number);
			}	
			
			//double,triple and long click handling
			u32 time = MIOS32_STOPWATCH_ValueGet();
			u32 delta = time - control.clickTime;
			if (delta < doubleClickTime && control.clickTime != 0) {

				//set the doubleclick flag;
				controls[number].doubleClick = TRUE;
				//set the tripleclick flag
				controls[number].tripleClick++; //second click occured			
			} else {
				controls[number].doubleClick = FALSE;
				controls[number].tripleClick = 0; //reset counter			
				//we are not in a double or tripleclick, check if the timer needs to be reset
				if (time > 5000000) { 
					//reset the stopwatch
					MIOS32_STOPWATCH_Reset();
				}			
			}
			
	#if DEBUG_VERBOSE_LEVEL >= 2		
			if (controls[number].doubleClick) {
				MIOS32_MIDI_SendDebugMessage("DOUBLECLICK");
			}
			if (controls[number].tripleClick == 2) {
				MIOS32_MIDI_SendDebugMessage("TRIPLECLICK");
			}
	#endif
			
			//save the time
			controls[number].clickTime = time+1; //to make it for sure differrent from 0
			
			switch (control.Type) {
					
				case controlTypeSetchange:
	#if DEBUG_VERBOSE_LEVEL >= 3
					MIOS32_MIDI_SendDebugMessage("Sending SetChange");
	#endif				
					SYSEX_SendCommand(controlTypeSetchange, FALSE, FALSE, FALSE, number, NULL, 0);
					break;
					
				case controlTypeClip:
	#if DEBUG_VERBOSE_LEVEL >= 3
					MIOS32_MIDI_SendDebugMessage("Sending Clip");
	#endif				
					SYSEX_SendCommand(controlTypeClip, controls[number].doubleClick, FALSE, FALSE, number, NULL, 0);
					break;

				case controlTypeMute:
	#if DEBUG_VERBOSE_LEVEL >= 3
					MIOS32_MIDI_SendDebugMessage("Sending Mute");
	#endif				
					SYSEX_SendCommand(controlTypeMute, FALSE, FALSE, FALSE, number, NULL, 0);
					break;
					
				case controlTypeSolo:
	#if DEBUG_VERBOSE_LEVEL >= 3
					MIOS32_MIDI_SendDebugMessage("Sending Solo");
	#endif								
					SYSEX_SendCommand(controlTypeSolo, FALSE, FALSE, FALSE, number, NULL, 0);
					break;
					
				default:
					break;
			}

			//reset triple click if necessary
			if (controls[number].tripleClick > 2) {
				controls[number].tripleClick = 0;
			}
			
		} else { //button up
	#if DEBUG_VERBOSE_LEVEL >= 2				
			MIOS32_MIDI_SendDebugMessage("--UP---NUMBER-%d----------------", number);			
	#endif		
			//long click handling
			u32 time = MIOS32_STOPWATCH_ValueGet();
			u32 delta = time - control.clickTime;
			if (delta > longClickTime) {

	#if DEBUG_VERBOSE_LEVEL >= 2		
				if (control.tripleClick == 2) {
					MIOS32_MIDI_SendDebugMessage("LONG TRIPLECLICK");				
				} else if (control.doubleClick) {
					MIOS32_MIDI_SendDebugMessage("LONG DOUBLECLICK");
				} else {
					MIOS32_MIDI_SendDebugMessage("LONG CLICK");
				}
	#endif
				
				//we do not need a longclick flag, it is all done here in this block
				switch (control.Type) {
						
						case controlTypeClip:
	#if DEBUG_VERBOSE_LEVEL >= 3
						MIOS32_MIDI_SendDebugMessage("Sending Clip");
	#endif				
						SYSEX_SendCommand(controlTypeClip, control.doubleClick, FALSE, TRUE, number, NULL, 0);
						break;
						
					case controlTypePreset:
	#if DEBUG_VERBOSE_LEVEL >= 3
						MIOS32_MIDI_SendDebugMessage("Sending Preset");
	#endif				
						SYSEX_SendCommand(controlTypePreset, controls[number].doubleClick, controls[number].tripleClick >> 1, TRUE, number, NULL, 0);
						break;
						
					default:
						break;
				}
			} else { //preset buttons send their message only at button up

				switch (control.Type) {
						
					case controlTypePreset:
	#if DEBUG_VERBOSE_LEVEL >= 3
						MIOS32_MIDI_SendDebugMessage("Sending Preset");
	#endif				
						SYSEX_SendCommand(controlTypePreset, controls[number].doubleClick, controls[number].tripleClick >> 1, FALSE, number, NULL, 0);
						break;
					default:
						break;
				}
			}
		}
	}
}

/////////////////////////////////////////////////////////////////////////////
// Handle encoders
/////////////////////////////////////////////////////////////////////////////
void CONTROL_EncoderHandler(u32 encoder, s32 incrementer)
{
	if (inputsEnabled ==1) {

#ifdef PCB_BUG_SRIO_EXCHANGED
		//we have to invert the encoder nr. 12 because the DIN pcb layout is wrong
		if (encoder == 12) {
			incrementer = -incrementer;
		}
#endif
		
		//check if the encoder is activated 
		if (controls[encoder].Type == controlTypeValue) {
			//set our value
			u8 newValue = controls[encoder].Value;
			if ((newValue - incrementer) > 127) {
				newValue = 127;
			} 
			else if (newValue < incrementer) {
				newValue = 0;
			}	else {
				newValue -= incrementer;
			}
			controls[encoder].Value = newValue;
			
			//by definition we are now not set by a preset. start blinking
			DISPLAY_SetBlink(encoder, DISPLAY_BLINK_ON);
			//clear the inPreset Flag
			controls[encoder].Settings &= !CONTROL_INPRESET_FLAG;
			
			//send the new value
			SYSEX_SendCommand(controlTypeValue, FALSE, FALSE, FALSE, encoder, &controls[encoder].Value, 1);
			
#if DEBUG_VERBOSE_LEVEL >= 4
			MIOS32_MIDI_SendDebugMessage("encoder number %d incrementer %d newvalue %d",encoder, -incrementer, newValue);
#endif
		}
	}
}
