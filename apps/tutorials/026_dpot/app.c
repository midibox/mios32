// $Id: app.c $
/*
 * MIOS32 Application Template
 *
 * ==========================================================================
 *
 *  Copyright (C) 2010 Jonathan Farmer (JRFarmer.com@gmail.com)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

/////////////////////////////////////////////////////////////////////////////
// Include files
/////////////////////////////////////////////////////////////////////////////

#include <mios32.h>
#include <dpot.h>
#include "app.h"


/////////////////////////////////////////////////////////////////////////////
// Global variables
/////////////////////////////////////////////////////////////////////////////

uint32_t auto_increment;

/////////////////////////////////////////////////////////////////////////////
// This hook is called after startup to initialize the application
/////////////////////////////////////////////////////////////////////////////
void APP_Init(void)
{
    int pot;

    // initialize all LEDs
    MIOS32_BOARD_LED_Init(0xFFFFFFFF);

    // initialize digital potentiometer interface
    DPOT_Init(0);
    MIOS32_MIDI_SendDebugMessage("DPOT module has been initialized!\n");

    // iniitialize digital pots to unique values
    for (pot = 0; pot < DPOT_NUM_POTS; pot++)
    {
        DPOT_Set_Value(pot, (uint16_t)(pot * 0x40));
        MIOS32_MIDI_SendDebugMessage("DPOT 0x%02X initialized to %04X\n",
                                     pot,
                                     DPOT_Get_Value(pot));
    }

    // setup each potentiometer for auto incrememting
    auto_increment = 0xFFFFFFFF;
}


/////////////////////////////////////////////////////////////////////////////
// This task is running endless in background
/////////////////////////////////////////////////////////////////////////////
void APP_Background(void)
{
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called each mS from the main task which also handles DIN, ENC
// and AIN events. You could add more jobs here, but they shouldn't consume
// more than 300 uS to ensure the responsiveness of buttons, encoders, pots.
// Alternatively you could create a dedicated task for application specific
// jobs as explained in $MIOS32_PATH/apps/tutorials/006_rtos_tasks
/////////////////////////////////////////////////////////////////////////////
void APP_Tick(void)
{
    int pot;

    // toggle the status LED (this is a sign of life)
    MIOS32_BOARD_LED_Set(0x0001, ~MIOS32_BOARD_LED_Get());

    // Update potentiometer hardware
    DPOT_Update();

    // increment all the digital pots
    for (pot = 0; pot < DPOT_NUM_POTS; pot++)
    {
      if (auto_increment & (0x00000001 << pot))
      {
	DPOT_Set_Value(pot, DPOT_Get_Value(pot) + 1);
      }
    }
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called each mS from the MIDI task which checks for incoming
// MIDI events. You could add more MIDI related jobs here, but they shouldn't
// consume more than 300 uS to ensure the responsiveness of incoming MIDI.
/////////////////////////////////////////////////////////////////////////////
void APP_MIDI_Tick(void)
{
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called when a MIDI package has been received
/////////////////////////////////////////////////////////////////////////////
void APP_MIDI_NotifyPackage(mios32_midi_port_t port, mios32_midi_package_t midi_package)
{
    switch (midi_package.evnt0 >> 4)
    {
        case CC:
            if (midi_package.evnt1 > 6 && midi_package.evnt1 < 11)
            {
                DPOT_Set_Value((midi_package.evnt1 - 7), (midi_package.evnt2 << 1));
                auto_increment &= ~(0x00000001 << (midi_package.evnt1 - 7));
                MIOS32_MIDI_SendDebugMessage("Pot 0x%02X value set to 0x%02X\n", 
                                             midi_package.evnt1 - 7,
                                             DPOT_Get_Value(midi_package.evnt1 - 7));
                MIOS32_MIDI_SendDebugMessage("auto_increment = 0x%08X\n", auto_increment);
            }
            break;
            
        default:
            if (midi_package.evnt0 != 0xFE)
            {
                // send received MIDI package to MIOS Terminal
                MIOS32_MIDI_SendDebugMessage("Port:%02X  Type:%X  Evnt0:%02X  Evnt1:%02X  Evnt2:%02X\n",
                                             port,
                                             midi_package.type,
                                             midi_package.evnt0, midi_package.evnt1, midi_package.evnt2);
            }
            break;
    }
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
