/////////////////////////////////////////////////////////////////////////////
// Include files
/////////////////////////////////////////////////////////////////////////////

#include <mios32.h>
#include "stm32f10x_dbgmcu.h"
#include "externalheaders.h"
#include "sysex.h"
#include "app.h"
#include "display.h"
#include "control.h"

/////////////////////////////////////////////////////////////////////////////
// Global Variables
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// This hook is called after startup to initialize the application
/////////////////////////////////////////////////////////////////////////////
void APP_Init(void)
{
	s32 status;

#if DEBUG_VERBOSE_LEVEL >= 1
	MIOS32_MIDI_SendDebugMessage("APP_Init:Hello World!");
#endif

	if( status = DISPLAY_Init(0) < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
		MIOS32_MIDI_SendDebugMessage("APP_Init:Error Knoepfli Init");
#endif
	};

	MIOS32_DELAY_Init(0);
	
	//Controls Init
	CONTROL_Init();
	
	//Sysex Interface Init
	SYSEX_Init();

	//init AIN
	/*
	 s32 errorAIN = MIOS32_AIN_Init(0);
	if (errorAIN < 0) {
		MIOS32_MIDI_SendDebugMessage("AIN InitError");
	}
	*/
	
	//send a reset command to tell the host that we have been reset
	SYSEX_SendCommand(AB_CMD_RESET, FALSE, FALSE, FALSE, 0xff, NULL, 0);
	
	//turn on led on board
	MIOS32_BOARD_LED_Init(1);
	MIOS32_BOARD_LED_Set(1, 1);
	
	//stop timers 2,3,4,5 during debug mode and disable I2C
	u32 DBGMCU_ADDR = 0xe0042000;
	typedef struct
	{
		vu32 IDCODE;
		vu32 CR;	
	} DBGMCU_TypeDef;
	
	DBGMCU_TypeDef* DBGMCU_PTR = (DBGMCU_TypeDef*)DBGMCU_ADDR;
	DBGMCU_PTR->CR |= (DBGMCU_TIM2_STOP+DBGMCU_TIM3_STOP+DBGMCU_TIM4_STOP+DBGMCU_TIM5_STOP+DBGMCU_I2C1_SMBUS_TIMEOUT);
}


/////////////////////////////////////////////////////////////////////////////
// This task is running endless in background
/////////////////////////////////////////////////////////////////////////////
void APP_Background(void)
{
	//go through all leds of one driver and update them
	
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
	CONTROL_ButtonHandler(pin, pin_value);
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called when an encoder has been moved
// incrementer is positive when encoder has been turned clockwise, else
// it is negative
/////////////////////////////////////////////////////////////////////////////
void APP_ENC_NotifyChange(u32 encoder, s32 incrementer)
{
	CONTROL_EncoderHandler(encoder, incrementer);
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called when a pot has been moved
/////////////////////////////////////////////////////////////////////////////
void APP_AIN_NotifyChange(u32 pin, u32 pin_value)
{
}
