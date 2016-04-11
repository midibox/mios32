/*
 * MIDIbox Genesis Tracker: Main application
 *
 * ==========================================================================
 *
 *  Copyright (C) 2016 Sauraen (sauraen@gmail.com)
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

#include <genesis.h>
#include <vgm.h>



/////////////////////////////////////////////////////////////////////////////
// This hook is called after startup to initialize the application
/////////////////////////////////////////////////////////////////////////////

void APP_Init(void){
    // initialize all LEDs
    MIOS32_BOARD_LED_Init(0xffffffff);

    //Initialize MBHP_Genesis module
    Genesis_Init();

    //Initialize VGM Player component
    VGM_Init();
    
    
}


/////////////////////////////////////////////////////////////////////////////
// This task is running endless in background
/////////////////////////////////////////////////////////////////////////////
void APP_Background(void){
    MIOS32_BOARD_LED_Set(0b1000, 0b1000);
}

/////////////////////////////////////////////////////////////////////////////
// This hook is called when a MIDI package has been received
/////////////////////////////////////////////////////////////////////////////
void APP_MIDI_NotifyPackage(mios32_midi_port_t port, mios32_midi_package_t midi_package){
    
}


// Not used
void APP_Tick(void){}
void APP_MIDI_Tick(void){}
void APP_SRIO_ServicePrepare(void){}
void APP_SRIO_ServiceFinish(void){}
void APP_DIN_NotifyToggle(u32 pin, u32 pin_value){}
void APP_ENC_NotifyChange(u32 encoder, s32 incrementer){}
void APP_AIN_NotifyChange(u32 pin, u32 pin_value){}

