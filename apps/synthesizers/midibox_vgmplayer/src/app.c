/*
 * MIDIbox VGM Player: Main application
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
#include <file.h>
#include <blm_x.h>
#include <vgm.h>

#include "frontpanel.h"
#include "interface.h"
#include "genesisstate.h"

u32 DEBUGVAL;
char* filenamelist;
s32 numfiles;
u8 playbackcommand;

/////////////////////////////////////////////////////////////////////////////
// This hook is called after startup to initialize the application
/////////////////////////////////////////////////////////////////////////////

void APP_Init(void){
    DEBUGVAL = 117;
    // initialize all LEDs
    MIOS32_BOARD_LED_Init(0xffffffff);

    // initialize SD card
    FILE_Init(0);

    //Initialize MBHP_Genesis module
    Genesis_Init();

    //Initialize VGM Player component
    VGM_Init();
    
    //Initialize Button-LED Matrix driver
    BLM_X_Init();
    
    //Initialize front panel wrapper
    FrontPanel_Init();
    
    //Initialize main interface
    Interface_Init();
    
    /*
    DEBUG_Ring = 0;
    DEBUG_RingState = 0;
    DEBUG_RingDir = 1;
    */
    MIOS32_LCD_Clear();
    MIOS32_LCD_CursorSet(0,0);
    MIOS32_LCD_PrintString("Searching for SD card...");
    
    
}


/////////////////////////////////////////////////////////////////////////////
// This task is running endless in background
/////////////////////////////////////////////////////////////////////////////
void APP_Background(void)
{
    MIOS32_BOARD_LED_Set(0b1000, 0b1000);
    
    //Draw Genesis states
    u8 g;
    for(g=0; g<GENESIS_COUNT; g++){
        DrawGenesisActivity(g);
    }
    
    //Flash LEDs
    //MIOS32_BOARD_LED_Set(0xF, ((count >> 12) & 0xF));
    MIOS32_BOARD_LED_Set(0b1000, 0b0000);
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called each mS from the main task which also handles DIN, ENC
// and AIN events. You could add more jobs here, but they shouldn't consume
// more than 300 uS to ensure the responsiveness of buttons, encoders, pots.
// Alternatively you could create a dedicated task for application specific
// jobs as explained in $MIOS32_PATH/apps/tutorials/006_rtos_tasks
/////////////////////////////////////////////////////////////////////////////
void APP_Tick(void){
    static u32 prescaler = 0;
    static u8 sdstate = 0;
    static u8 selfile = 0;
    static VgmSource* vgms = NULL;
    static VgmHead* vgmh = NULL;
    //static u8 row = 0, sr = 0, pin = 0, state = 1;
    //TODO move to its own task
    BLM_X_BtnHandler((void*)&FrontPanel_ButtonChange);
    
    s32 res;
    
    ++prescaler;
    char* tempbuf; u8 i;
    if(prescaler == 100){
        prescaler = 0;
        switch(sdstate){
        case 0:
            res = FILE_CheckSDCard();
            if(res == 3){
                MIOS32_LCD_Clear();
                MIOS32_LCD_CursorSet(0,0);
                MIOS32_LCD_PrintString("Loading list of VGM files...");
                sdstate = 2;
            }else if(res >= 0){
                MIOS32_LCD_Clear();
                MIOS32_LCD_CursorSet(0,0);
                MIOS32_LCD_PrintString("Waiting for SD card to speed up...");
            }else{
                MIOS32_LCD_Clear();
                MIOS32_LCD_CursorSet(0,0);
                MIOS32_LCD_PrintFormattedString("SD Card error %d", res);
                sdstate = 1;
            }
            break;
        case 1:
            //do nothing
            break;
        case 2:
            //Load list of VGM files
            filenamelist = malloc(9*MAXNUMFILES);
            numfiles = FILE_GetFiles("/", "vgm", filenamelist, MAXNUMFILES, 0);
            if(numfiles < 0){
                MIOS32_LCD_Clear();
                MIOS32_LCD_CursorSet(0,0);
                MIOS32_LCD_PrintFormattedString("Error %d finding files", numfiles);
                sdstate = 1;
            }else{
                MIOS32_LCD_Clear();
                MIOS32_LCD_CursorSet(0,0);
                MIOS32_LCD_PrintFormattedString("Found %d files", numfiles);
                selfile = 0;
                sdstate = 3;
            }
            break;
        case 3:
            if(playbackcommand != 0){
                switch(playbackcommand){
                case 1:
                    if(selfile < numfiles - 1){
                        ++selfile;
                    }
                    break;
                case 2:
                    if(selfile > 0){
                        --selfile;
                    }
                    break;
                case 3:
                    BLM_X_LEDSet((3 * 88) + (1 * 8) + 4, 0, 1); //Play LED
                    sdstate = 4;
                    break;
                };
                playbackcommand = 0;
            }
            tempbuf = malloc(9);
            for(i=0; i<8; i++){
                tempbuf[i] = filenamelist[(9*selfile)+i];
            }
            tempbuf[8] = 0;
            MIOS32_LCD_Clear();
            MIOS32_LCD_CursorSet(0,0);
            MIOS32_LCD_PrintFormattedString("Found %d files", numfiles);
            MIOS32_LCD_CursorSet(0,1);
            MIOS32_LCD_PrintFormattedString("File %d: %s.vgm", selfile, tempbuf);
            free(tempbuf);
            break;
        case 4:
            //Load VGM file
            tempbuf = malloc(13);
            for(i=0; i<8; i++){
                if(filenamelist[(9*selfile)+i] <= ' ') break;
                tempbuf[i] = filenamelist[(9*selfile)+i];
            }
            tempbuf[i++] = '.';
            tempbuf[i++] = 'v';
            tempbuf[i++] = 'g';
            tempbuf[i++] = 'm';
            tempbuf[i++] = 0;
            vgms = VGM_SourceStream_Create();
            res = VGM_SourceStream_Start(vgms, tempbuf);
            if(res >= 0){
                MIOS32_LCD_Clear();
                MIOS32_LCD_CursorSet(0,0);
                MIOS32_LCD_PrintFormattedString("Loaded VGM!");
                vgmh = VGM_Head_Create(vgms);
                VGM_Head_Restart(vgmh, VGM_Player_GetVGMTime());
                vgmh->playing = 1;
                sdstate = 5;
            }else{
                MIOS32_LCD_Clear();
                MIOS32_LCD_CursorSet(0,0);
                MIOS32_LCD_PrintFormattedString("Error loading %s", tempbuf);
                VGM_Source_Delete(vgms);
                sdstate = 3;
            }
            free(tempbuf);
        case 5:
            if(playbackcommand == 3){
                BLM_X_LEDSet((3 * 88) + (1 * 8) + 4, 0, 0); //Play LED
                VGM_Head_Delete(vgmh);
                VGM_Source_Delete(vgms);
                Genesis_Reset(USE_GENESIS);
                playbackcommand = 0;
                sdstate = 3;
            }
            MIOS32_LCD_Clear();
            MIOS32_LCD_CursorSet(0,0);
            MIOS32_LCD_PrintFormattedString("Playing...");
            break;
        }
    }
    
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called each mS from the MIDI task which checks for incoming
// MIDI events. You could add more MIDI related jobs here, but they shouldn't
// consume more than 300 uS to ensure the responsiveness of incoming MIDI.
/////////////////////////////////////////////////////////////////////////////
void APP_MIDI_Tick(void){
    //Nothing here
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called when a MIDI package has been received
/////////////////////////////////////////////////////////////////////////////
void APP_MIDI_NotifyPackage(mios32_midi_port_t port, mios32_midi_package_t midi_package){

}


/////////////////////////////////////////////////////////////////////////////
// This hook is called before the shift register chain is scanned
/////////////////////////////////////////////////////////////////////////////
void APP_SRIO_ServicePrepare(void){
    BLM_X_PrepareRow();
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called after the shift register chain has been scanned
/////////////////////////////////////////////////////////////////////////////
void APP_SRIO_ServiceFinish(void){
    BLM_X_GetRow();
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called when a button has been toggled
// pin_value is 1 when button released, and 0 when button pressed
/////////////////////////////////////////////////////////////////////////////
void APP_DIN_NotifyToggle(u32 pin, u32 pin_value){
    DBG("DIN pin %d changed %d", pin, pin_value);
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called when an encoder has been moved
// incrementer is positive when encoder has been turned clockwise, else
// it is negative
/////////////////////////////////////////////////////////////////////////////
void APP_ENC_NotifyChange(u32 encoder, s32 incrementer){
    FrontPanel_EncoderChange(encoder, incrementer);
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called when a pot has been moved
/////////////////////////////////////////////////////////////////////////////
void APP_AIN_NotifyChange(u32 pin, u32 pin_value){
    //Nothing here
}
