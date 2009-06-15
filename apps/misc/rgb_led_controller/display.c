/////////////////////////////////////////////////////////////////////////////
// Include files
/////////////////////////////////////////////////////////////////////////////

#import <mios32.h>
#import <FreeRTOS.h>
#import <task.h>
#import <queue.h>
#import "externalheaders.h"
#import "display.h"
#import "control.h"
#import "sysex.h"

/////////////////////////////////////////////////////////////////////////////
// Static definitions
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Global Variables
/////////////////////////////////////////////////////////////////////////////

u8 display_call_blinkflashhandler; //flag for the timer process
u16 display_led_counter; //counter to go through all leds
u8 display_led_blink_counter; //blinking step counter
u8 display_led_blink_direction; //direction on the blink step counter
xTaskHandle LedTaskHandle;

/////////////////////////////////////////////////////////////////////////////
// Local declares
/////////////////////////////////////////////////////////////////////////////
void DISPLAY_LedHandler(void);

#import "display_config.c"

/////////////////////////////////////////////////////////////////////////////
// Local prototypes
/////////////////////////////////////////////////////////////////////////////
static void DISPLAY_LedTask(void *pvParameters);
u8 DISPLAY_Interpolate(u8 value1, u8 value2, u8 step, u8 steps);

/////////////////////////////////////////////////////////////////////////////
// The task that is responsible for blinking the LEDs
/////////////////////////////////////////////////////////////////////////////
static void DISPLAY_LedTask(void *pvParameters)
{
  portTickType xLastExecutionTime;
	u8 change;
	display_led_color_t color;
	control_t	control;
	u8 blinksteps, flashsteps, flashcounter, touchsteps, touchcounter;
	
  // Initialise the xLastExecutionTime variable on task entry
  xLastExecutionTime = xTaskGetTickCount();
	
  while( 1 ) {
    vTaskDelayUntil(&xLastExecutionTime, 10 / portTICK_RATE_MS);
				
		control = controls[display_led_counter];
		blinksteps = control.BlinkSteps;
		flashsteps = control.FlashSteps;
		flashcounter = control.FlashCounter;
		touchsteps = control.TouchSteps;
		touchcounter = control.TouchCounter;
		
		//set change flag to false
		change = 0;
		
		//get the current color
		color = control.ColorNow;
		
		//temp color
		if (control.setTempColor) {
			controls[display_led_counter].setTempColor = 0;
			change = 1;
			color = control.tempColor;
		}
		
		//get the color the led should be at now (in between the two blink colors)
		if( control.Blink == DISPLAY_BLINK_ON) {
			//the color has to be changed, set the flag
			change = 1;
			//check the control type
			if( controllers[display_led_counter] > controllerTypeSmallButton) { //encoder and FSR, only blink very briefly to show that we are notinpreset
				u8 reducedBlink = blinksteps - (blinksteps / 4);
				if (display_led_blink_counter > reducedBlink) { //are we in the last steps?
					color.Red = DISPLAY_Interpolate(control.secondaryColor.Red, control.blinkColor.Red, display_led_blink_counter-reducedBlink, blinksteps/4);
					color.Green = DISPLAY_Interpolate(control.secondaryColor.Green, control.blinkColor.Green, display_led_blink_counter-reducedBlink, blinksteps/4);
					color.Blue = DISPLAY_Interpolate(control.secondaryColor.Blue, control.blinkColor.Blue, display_led_blink_counter-reducedBlink, blinksteps/4);
				} else {
					color = control.secondaryColor;
				}
			} else if (controllers[display_led_counter] == controllerTypeSmallButton) { //momentary and on/off button
				if ( controls[display_led_counter].Value == CONTROL_BUTTON_OFF) { //button is off
					color.Red = DISPLAY_Interpolate(control.mainColor.Red, control.blinkColor.Red, display_led_blink_counter, blinksteps);
					color.Green = DISPLAY_Interpolate(control.mainColor.Green, control.blinkColor.Green, display_led_blink_counter, blinksteps);
					color.Blue = DISPLAY_Interpolate(control.mainColor.Blue, control.blinkColor.Blue, display_led_blink_counter, blinksteps);
				} else { //button is on
					color.Red = DISPLAY_Interpolate(control.secondaryColor.Red, control.blinkColor.Red, display_led_blink_counter, blinksteps);
					color.Green = DISPLAY_Interpolate(control.secondaryColor.Green, control.blinkColor.Green, display_led_blink_counter, blinksteps);
					color.Blue = DISPLAY_Interpolate(control.secondaryColor.Blue, control.blinkColor.Blue, display_led_blink_counter, blinksteps);
				}			
			}
			//The ColorNow is not affected by the flash color nor by changes or touches
			controls[display_led_counter].ColorNow	= color;		
		}
		
		//see if we are at the moment in a transition from noblink to blink, from one color to another in noblink, from blink to noblink or from blink to blink
		if( control.ChangeSteps != 0 ) { //we are in a change transition
			change = 1;
			controls[display_led_counter].ChangeSteps = 0;
			//fade from the saved color to the new one. 
			// color.Red = DISPLAY_Interpolate(color.Red, control.ColorFrom.Red, control.ChangeSteps-1, DISPLAY_CHANGE_STEPS); //-1 so that we reach zero already when changesteps is 1
			// color.Green = DISPLAY_Interpolate(color.Green, control.ColorFrom.Green, control.ChangeSteps-1, DISPLAY_CHANGE_STEPS);
			// color.Blue = DISPLAY_Interpolate(color.Blue, control.ColorFrom.Blue, control.ChangeSteps-1, DISPLAY_CHANGE_STEPS);
			// --controls[display_led_counter].ChangeSteps;
		}
		
		if( control.Flash == DISPLAY_FLASH_ON ) {
			
			//the color has to be changed, set the flag
			change = 1;
			
			//We just reuse the value in color, this way we do not need to save an extra color and the colorchange ist still integrated into the flash
			color.Red = DISPLAY_Interpolate(color.Red, control.flashColor.Red, flashcounter, flashsteps);
			color.Green = DISPLAY_Interpolate(color.Green, control.flashColor.Green, flashcounter, flashsteps);
			color.Blue = DISPLAY_Interpolate(color.Blue, control.flashColor.Blue, flashcounter, flashsteps);
			if( control.FlashCounter == 0 ) {
				controls[display_led_counter].Flash = DISPLAY_FLASH_OFF;
			} else {
				--controls[display_led_counter].FlashCounter;
			}
		}
		
		if( control.Touch == DISPLAY_TOUCH_ON ) {
			
			//the color has to be changed, set the flag
			change = 1;
			
			//We just reuse the value in color, this way we do not need to save an extra color and the colorchange ist still integrated into the flash
			color.Red = DISPLAY_Interpolate(color.Red, control.touchColor.Red, touchcounter, touchsteps);
			color.Green = DISPLAY_Interpolate(color.Green, control.touchColor.Green, touchcounter, touchsteps);
			color.Blue = DISPLAY_Interpolate(color.Blue, control.touchColor.Blue, touchcounter, touchsteps);
			if( control.TouchCounter == 0 ) {
				controls[display_led_counter].Touch = DISPLAY_TOUCH_OFF;
			} else {
				--controls[display_led_counter].TouchCounter;
			}
		}
		
		if( change == 1) {
			DISPLAY_SendColor(display_led_counter, color);
			//see if we have a slave
			if (controllers[display_led_counter] > controllerTypeSmallButton) { //we have a slave
				DISPLAY_SendColor(display_led_counter+1, color);
			}
		}
		
		//increase the counter for the next run of the routine
		++display_led_counter;
		
		if( display_led_counter == DISPLAY_LED_NUMBER ) {
			//reset the counter
			display_led_counter = 0;
			//increase/decrease the blink counter
			if( display_led_blink_direction == 0 ) {
				++display_led_blink_counter;
				if( display_led_blink_counter == DISPLAY_BLINK_STEPS ) {
					display_led_blink_direction = 1;
					--display_led_blink_counter;
					--display_led_blink_counter;
				}
			} else { 
				if( display_led_blink_counter == 0) {
					display_led_blink_direction = 0;
					++display_led_blink_counter;
				} else {
					--display_led_blink_counter;
				}
			}
		}
	}
}


/////////////////////////////////////////////////////////////////////////////
// this function inits the LED drivers (returns <0 on errors)
/////////////////////////////////////////////////////////////////////////////
s32 DISPLAY_Init(u8 mode)
{
	u8 i;
	
	// currently only mode 0 supported
  if( mode != 0 )
    return -1; // unsupported mode
		
	//reset the counters
	display_led_counter = 0;
	display_led_blink_counter = 0;
	display_led_blink_direction = 0;
	display_call_blinkflashhandler = 0;
	
	//Init IIC
  MIOS32_IIC_Init(0);
		
	//Init commands for PCA9635
	s32 status;
	u8 pca9635_init[3] = {0x06, 0xa5, 0x5a};			//Reset PCAs (this array can be passed as a pointer to a function!)
	u8 pca9635_wakeup[3] = {0xe0, 0x00, 0x01};		//wakeup the pcas using all call address
	u8 pca9635_allcall[3] = {0xe0, 0x1b, 0x02};		//set the ALLCALL Adress to 00000010 to get a uninterrupted address range for the rest of the I2C addresses
	u8 pca9635_enable0[3] = {0x02, 0x14, 0xFF}; 	//Enable LED Group 0, enable individual and group pwm
	u8 pca9635_enable1[3] = {0x02, 0x15, 0xFF}; 	//Enable LED Group 1, enable individual and group pwm 
	u8 pca9635_enable2[3] = {0x02, 0x16, 0xFF};  	//Enable LED Group 2, enable individual and group pwm
	u8 pca9635_enable3[3] = {0x02, 0x17, 0xFF};  	//Enable LED Group 3, enable individual and group pwm
		
  // currently only mode 0 supported
  if( mode != 0 )
    return -1; // unsupported mode
	
	if( (status=MIOS32_IIC_TransferBegin(0,IIC_Blocking)) < 0 )
    return status;
	
	if( (status = MIOS32_IIC_Transfer(0,IIC_Write, pca9635_init[0], &pca9635_init[1], 2)) < 0 ) 
		return status;
	MIOS32_IIC_TransferWait(0);
	MIOS32_DELAY_Wait_uS(500);
	if( (status = MIOS32_IIC_Transfer(0,IIC_Write, pca9635_wakeup[0], &pca9635_wakeup[1], 2)) < 0 ) 
		return status;
	MIOS32_IIC_TransferWait(0);
	if( (status = MIOS32_IIC_Transfer(0,IIC_Write, pca9635_allcall[0], &pca9635_allcall[1], 2)) < 0 ) 
		return status;
	MIOS32_IIC_TransferWait(0);
	if( (status = MIOS32_IIC_Transfer(0,IIC_Write, pca9635_enable0[0], &pca9635_enable0[1], 2)) < 0 ) 
		return status;
	MIOS32_IIC_TransferWait(0);
	if( (status = MIOS32_IIC_Transfer(0,IIC_Write, pca9635_enable1[0], &pca9635_enable1[1], 2)) < 0 ) 
		return status;
	MIOS32_IIC_TransferWait(0);
	if( (status = MIOS32_IIC_Transfer(0,IIC_Write, pca9635_enable2[0], &pca9635_enable2[1], 2)) < 0 ) 
		return status;
	MIOS32_IIC_TransferWait(0);
	if( (status = MIOS32_IIC_Transfer(0,IIC_Write, pca9635_enable3[0], &pca9635_enable3[1], 2)) < 0 ) 
		return status;
	MIOS32_IIC_TransferWait(0);	
	MIOS32_IIC_TransferFinished(0);	

	//init the task that handles the led changing
	xTaskCreate(DISPLAY_LedTask, (signed portCHAR *)"LedTask", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY+3, &LedTaskHandle);
		
#if DEBUG_VERBOSE_LEVEL == 4
	MIOS32_MIDI_SendDebugMessage("DISPLAY_Init: FINISHED");
#endif		
	
};

void DISPLAY_SuspendLedTask() {
	vTaskSuspend(LedTaskHandle);
}

void DISPLAY_ResumeLedTask() {
	vTaskResume(LedTaskHandle);
}

///////////////////////////////////////////////////////////////////////////
// Interpolate: Gives back an interpolated value between value1 and value2
///////////////////////////////////////////////////////////////////////////
u8 DISPLAY_Interpolate(u8 value1, u8 value2, u8 step, u8 steps)
{
	u8 swap, diff;
	
	//send out value2 if step is the max value
	if( step == steps-1 ) {
		return value2;
	}
	//send out value1 if step is 0
	if( step == 0 ) {
		return value1;
	}
	
	//invert the interpolation if value1 > value2
	if( value1 > value2 ) {
		//swap the values
		swap = value2;
		value2 = value1;
		value1 = swap;
		//++step because we are counting from 0 to steps
		++step;
		//invert the step counter
		step = steps-step;
	}
	
	//get the difference
	diff = value2 - value1;
	
	//multiply steps with the diff
	//now divide by the total number of steps
	//add the lower value
	return value1 + (step * diff / (steps-1));
}


/////////////////////////////////////////////////////////////////////////////
// this function sets the color of one led
/////////////////////////////////////////////////////////////////////////////
void DISPLAY_SendColor(u8 number, display_led_color_t color) 
{
	u8 message[2];
	s32 error;
	//MIOS32_MIDI_SendDebugMessage("SetColor: number: %d red %d green %d blue %d", number, color.Red, color.Green, color.Blue);
	error = MIOS32_IIC_TransferBegin(0,IIC_Blocking);
	if (error<0) {
		MIOS32_MIDI_SendDebugMessage("begin %d", error);
	} else {
		
		error = MIOS32_IIC_TransferWait(0);
		if (error<0) MIOS32_MIDI_SendDebugMessage("wait before red %d", error);	
		message[0] = display_led_address_red[number].Led;
		message[1] = color.Red;
		error = MIOS32_IIC_Transfer(0,IIC_Write, display_led_address_red[number].Driver, message, 2);
		if (error<0) MIOS32_MIDI_SendDebugMessage("transfer red %d", error);
		
		error = MIOS32_IIC_TransferWait(0);
		if (error<0) MIOS32_MIDI_SendDebugMessage("wait before green %d", error);	
		message[0] = display_led_address_green[number].Led;
		message[1] = color.Green;
		MIOS32_IIC_Transfer(0,IIC_Write, display_led_address_green[number].Driver, message, 2);
		if (error<0) MIOS32_MIDI_SendDebugMessage("transfer green %d", error);
		
		error = MIOS32_IIC_TransferWait(0);
		if (error<0) MIOS32_MIDI_SendDebugMessage("wait before blue %d", error);	
		message[0] = display_led_address_blue[number].Led;
		message[1] = color.Blue;
		MIOS32_IIC_Transfer(0,IIC_Write, display_led_address_blue[number].Driver, message, 2);
		if (error<0) MIOS32_MIDI_SendDebugMessage("transfer blue %d", error);
		
		error = MIOS32_IIC_TransferWait(0);
		if (error<0) MIOS32_MIDI_SendDebugMessage("wait after blue %d", error);	
		MIOS32_IIC_TransferFinished(0);	
	}
};

///////////////////////////////////////////////////////////////////////////
// Send Config Array
///////////////////////////////////////////////////////////////////////////
void DISPLAY_SendConfig(void) 
{	
#if DEBUG_VERBOSE_LEVEL >= 3
	MIOS32_MIDI_SendDebugMessage("DISPLAY_SendConfig: START");
#endif		
	u8 configArray[7]; //this buffer is filled repeatedly to send the data
	u16 configCounter = 0;   //counts through the config bytes
	u8 configArrayCounter = 0;  //counts through the config array
	u8 endOfConfig = 0;
	
	MIOS32_MIDI_SendSysEx(DEFAULT, (u8* const)ab_sysex_header, sizeof(ab_sysex_header));
	//the rest is scrambled, except for the footer
	while (!endOfConfig) {
		//copy bytes into the buffer, depending on position
		switch (configCounter) {
			case 0: //type
				configArray[configArrayCounter] = AB_CMD_SENDCONFIG;
				break;
			case 1: //device
				configArray[configArrayCounter] = MIOS32_MIDI_DeviceIDGet() & 0x3f; //the upper two bytes have to be zero
				break;
			case 2: //control
				configArray[configArrayCounter] = 0xff; //does not really matter				
				break;
			case 3: //columns
				configArray[configArrayCounter] = control_columns;
				break;
			case 4: //rows
				configArray[configArrayCounter] = control_rows;
				break;
			default:
				configArray[configArrayCounter] = controllers[configCounter-5];
				break;
		}
		configArrayCounter++;
		configCounter++;
		//check if we are at the end of the data
		if (configCounter == ((control_rows*control_columns)+5)) {
			endOfConfig = 1;
		}
		//check if the buffer is full
		if (configArrayCounter == 7 || endOfConfig) {		
			SYSEX_SendData(configArray, configArrayCounter);
			configArrayCounter = 0;
		}				
	}
	//send the footer
	MIOS32_MIDI_SendSysEx(DEFAULT, (u8* const)&ab_sysex_footer, 1);
#if DEBUG_VERBOSE_LEVEL >= 3
	MIOS32_MIDI_SendDebugMessage("DISPLAY_SendConfig: END");
#endif	
}

///////////////////////////////////////////////////////////////////////////
// Update Color
///////////////////////////////////////////////////////////////////////////
void DISPLAY_UpdateColor(u8 number)
{
	//update the current color of the control
	control_t control = controls[number];
	controls[number].ColorFrom = controls[number].ColorNow;
	//check the control type
	if( controllers[number] > controllerTypeSmallButton ) { //encoder and FSR
		//calculate the current color
		if (control.Value < 63) {
			//the secondary value saves the current color modified by the value for encoders and FSRs
			controls[number].secondaryColor.Red =	DISPLAY_Interpolate(control.minValueColor.Red, control.mainColor.Red, control.Value, 63);
			controls[number].secondaryColor.Green =	DISPLAY_Interpolate(control.minValueColor.Green, control.mainColor.Green, control.Value, 63);
			controls[number].secondaryColor.Blue =	DISPLAY_Interpolate(control.minValueColor.Blue, control.mainColor.Blue, control.Value, 63);
		}
		else if (control.Value > 64) {
			controls[number].secondaryColor.Red =	DISPLAY_Interpolate(control.mainColor.Red, control.maxValueColor.Red, control.Value - 65, 63);
			controls[number].secondaryColor.Green =	DISPLAY_Interpolate(control.mainColor.Green, control.maxValueColor.Green, control.Value - 65, 63);
			controls[number].secondaryColor.Blue =	DISPLAY_Interpolate(control.mainColor.Blue, control.maxValueColor.Blue, control.Value -65, 63);				
		}
		else {
			controls[number].secondaryColor = controls[number].mainColor;
		}
		controls[number].ColorNow = controls[number].secondaryColor;
	} else { //momentary and on/off button
		if ( control.Value == CONTROL_BUTTON_OFF) { //button is off
			controls[number].ColorNow = controls[number].mainColor;
		} else { //button is on
			controls[number].ColorNow = controls[number].secondaryColor;
		}			
	}
	
	//set the changesteps of this control to update it
	controls[number].ChangeSteps = DISPLAY_CHANGE_STEPS;
}

///////////////////////////////////////////////////////////////////////////
// set brightness
///////////////////////////////////////////////////////////////////////////
void DISPLAY_SetBrightness(u8 brightness)
{
	u8 message[2];
#if DEBUG_VERBOSE_LEVEL >= 3
	MIOS32_MIDI_SendDebugMessage("PCA9685_Brightness: %d", brightness);
#endif
	MIOS32_IIC_TransferBegin(0,IIC_Blocking);
	message[0] = 0x12; //group pwm register, no auto increment
	message[1] = brightness; //brightness
	MIOS32_IIC_Transfer(0,IIC_Write, DISPLAY_ALLCALL, message, 2);
	MIOS32_IIC_TransferWait(0);
	MIOS32_IIC_TransferFinished(0);	
}

///////////////////////////////////////////////////////////////////////////
// Turn Blinking on/off for one control
///////////////////////////////////////////////////////////////////////////
void DISPLAY_SetBlink(u8 number, u8 state)
{
	//TODO: switch off handling of blinking and flashing while making new settings
	if( controls[number].Blink != state ) { //only change if we are actually changing state
		controls[number].Blink = state;
	}
	DISPLAY_UpdateColor(number);
}

///////////////////////////////////////////////////////////////////////////
// Flash one control
///////////////////////////////////////////////////////////////////////////
void DISPLAY_Flash(u8 number, display_led_color_t *color) 
{
	controls[number].flashColor = *color;
	controls[number].Flash = DISPLAY_FLASH_ON;
	controls[number].FlashCounter = controls[number].FlashSteps-1;
}

///////////////////////////////////////////////////////////////////////////
// Check LEDs
///////////////////////////////////////////////////////////////////////////
void DISPLAY_LedCheck() 
{
	u8 message[2];
	u8 i;
	MIOS32_MIDI_SendDebugMessage("flashing all");
	for (i=0;i<16;i++) {
		MIOS32_IIC_TransferBegin(0,IIC_Blocking);
		message[0] = 0x02 + i;
		message[1] = 0xff;
		MIOS32_IIC_Transfer(0,IIC_Write, DISPLAY_ALLCALL, message, 2);
		MIOS32_IIC_TransferWait(0);
		message[0] = 0x02 + i;
		message[1] = 0xff;
		MIOS32_IIC_Transfer(0,IIC_Write, DISPLAY_ALLCALL, message, 2);
		MIOS32_IIC_TransferWait(0);
		message[0] = 0x02 + i;
		message[1] = 0xff;
		MIOS32_IIC_Transfer(0,IIC_Write, DISPLAY_ALLCALL, message, 2);
		MIOS32_IIC_TransferWait(0);
		MIOS32_IIC_TransferFinished(0);	
	}
	for (i=0;i<30;i++) {
		MIOS32_DELAY_Wait_uS(40000);
	}
	for (i=0;i<16;i++) {
		MIOS32_IIC_TransferBegin(0,IIC_Blocking);
		message[0] = 0x02 + i;
		message[1] = 0x00;
		MIOS32_IIC_Transfer(0,IIC_Write, DISPLAY_ALLCALL, message, 2);
		MIOS32_IIC_TransferWait(0);
		message[0] = 0x02 + i;
		message[1] = 0x00;
		MIOS32_IIC_Transfer(0,IIC_Write, DISPLAY_ALLCALL, message, 2);
		MIOS32_IIC_TransferWait(0);
		message[0] = 0x02 + i;
		message[1] = 0x00;
		MIOS32_IIC_Transfer(0,IIC_Write, DISPLAY_ALLCALL, message, 2);
		MIOS32_IIC_TransferWait(0);
		MIOS32_IIC_TransferFinished(0);	
	}
}

///////////////////////////////////////////////////////////////////////////
// Touch one control
///////////////////////////////////////////////////////////////////////////
void DISPLAY_Touch(u8 number) 
{
	controls[number].Touch = DISPLAY_TOUCH_ON;
	controls[number].TouchCounter = controls[number].TouchSteps-1;
}

///////////////////////////////////////////////////////////////////////////
// MeterFlash one control
///////////////////////////////////////////////////////////////////////////
void DISPLAY_MeterFlash(u8 number, display_led_color_t *color)
{
	controls[number].flashColor = *color;
	controls[number].Flash = DISPLAY_FLASH_ON;
	controls[number].FlashCounter = 3;
}

/////////////////////////////////////////////////////////////////////////////
// Set a temporary  color
/////////////////////////////////////////////////////////////////////////////
extern void DISPLAY_SetColor(u8 number, display_led_color_t *color)
{
	controls[number].tempColor = *color;
	controls[number].setTempColor = 1;
}
