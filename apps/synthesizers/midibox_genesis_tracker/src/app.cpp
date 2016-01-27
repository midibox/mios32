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
#include <file.h>

#include "vgmsourcestream.h"
#include "vgmhead.h"
#include "vgmplayer.h"
#include "vgmplayer_ll.h"

u32 DEBUGVAL;

/////////////////////////////////////////////////////////////////////////////
// This hook is called after startup to initialize the application
// It's delcared as "extern C" so that the MIOS32 programming model can
// access this function - you can safely write your own functions in C++
// In other words: there is no need to add "extern C" to your own functions!
/////////////////////////////////////////////////////////////////////////////
extern "C" void APP_Init(void){
    DEBUGVAL = 117;
    // initialize all LEDs
    MIOS32_BOARD_LED_Init(0xffffffff);

    // initialize SD card
    FILE_Init(0);

    //Initialize MBHP_Genesis module
    Genesis_Init();

    //Initialize VGM Player component
    VgmPlayer_Init();
}


/////////////////////////////////////////////////////////////////////////////
// This task is running endless in background
/////////////////////////////////////////////////////////////////////////////
extern "C" void APP_Background(void)
{
    static u32 count = 0;
    static VgmSourceStream* vgms = NULL;
    static VgmHead* vgmh = NULL;
    static s32 res = 0;
    static u8 gotsdcard = 0;
    
    MIOS32_BOARD_LED_Set(0b1000, 0b1000);
    
    if(count % 500000 == 0){
        if(!gotsdcard){
            res = FILE_CheckSDCard();
            if(res == 3){
                gotsdcard = 1;
                //FILE_PrintSDCardInfos();
            }
        }
        if(gotsdcard){
            if(vgms == NULL){
                vgms = new VgmSourceStream();
                char* filename = new char[50];
                sprintf(filename, "RKACREDS.VGM");
                //res = FILE_FileExists(filename);
                //DBG("File existence: %d", res);
                res = vgms->startStream(filename);
                if(res >= 0){
                    DBG("Loaded VGM!");
                    vgms->readHeader();
                    vgmh = new VgmHead(vgms);
                    vgmh->restart(VgmPlayerLL_GetVGMTime());
                    VgmPlayer_AddHead(vgmh);
                }else{
                    DBG("Failed to load VGM");
                }
                delete[] filename;
            }else{
                //
            }
        }
        if(vgmh == NULL){
            //DBG("DEBUGVAL %d, res = %d", DEBUGVAL, res);
        }else{
            //DBG("DEBUGVAL %d, VGM address 0x%x", DEBUGVAL, vgmh->getCurAddress());
        }
    }
    
    if(vgms != NULL){
        //vgms->bg_streamBuffer();
    }
    
    
    
    //Play some things on the PSG
    /*
    Genesis_PSGWrite(0, 0b10010000); while(Genesis_CheckPSGBusy(0));
    Genesis_PSGWrite(0, 0b10000000); while(Genesis_CheckPSGBusy(0));
    Genesis_PSGWrite(0, 0b00010000); while(Genesis_CheckPSGBusy(0));
    MIOS32_DELAY_Wait_uS(0xFF00);
    Genesis_PSGWrite(0, 0b10000000); while(Genesis_CheckPSGBusy(0));
    Genesis_PSGWrite(0, 0b00001000); while(Genesis_CheckPSGBusy(0));
    MIOS32_DELAY_Wait_uS(0xFF00);
    Genesis_PSGWrite(0, 0b10000000); while(Genesis_CheckPSGBusy(0));
    Genesis_PSGWrite(0, 0b00000100); while(Genesis_CheckPSGBusy(0));
    MIOS32_DELAY_Wait_uS(0xFF00);
    Genesis_PSGWrite(0, 0b10000000); while(Genesis_CheckPSGBusy(0));
    Genesis_PSGWrite(0, 0b00000010); while(Genesis_CheckPSGBusy(0));
    MIOS32_DELAY_Wait_uS(0xFF00);
    Genesis_PSGWrite(0, 0b10000000); while(Genesis_CheckPSGBusy(0));
    Genesis_PSGWrite(0, 0b00000001); while(Genesis_CheckPSGBusy(0));
    MIOS32_DELAY_Wait_uS(0xFF00);
    */
    //OPN2 piano test patch
    /*
    Genesis_OPN2Write(0, 0, 0x22, 0x00); while(Genesis_CheckOPN2Busy(0));
    Genesis_OPN2Write(0, 0, 0x27, 0x00); while(Genesis_CheckOPN2Busy(0));
    Genesis_OPN2Write(0, 0, 0x2B, 0x00); while(Genesis_CheckOPN2Busy(0));
    for(u8 ah=0; ah<2; ah++){
        Genesis_OPN2Write(0, ah, 0x30, 0x71); while(Genesis_CheckOPN2Busy(0));
        Genesis_OPN2Write(0, ah, 0x34, 0x0D); while(Genesis_CheckOPN2Busy(0));
        Genesis_OPN2Write(0, ah, 0x38, 0x33); while(Genesis_CheckOPN2Busy(0));
        Genesis_OPN2Write(0, ah, 0x3C, 0x01); while(Genesis_CheckOPN2Busy(0));
        Genesis_OPN2Write(0, ah, 0x40, 0x23); while(Genesis_CheckOPN2Busy(0));
        Genesis_OPN2Write(0, ah, 0x44, 0x2D); while(Genesis_CheckOPN2Busy(0));
        Genesis_OPN2Write(0, ah, 0x48, 0x26); while(Genesis_CheckOPN2Busy(0));
        Genesis_OPN2Write(0, ah, 0x4C, 0x00); while(Genesis_CheckOPN2Busy(0));
        Genesis_OPN2Write(0, ah, 0x50, 0x5F); while(Genesis_CheckOPN2Busy(0));
        Genesis_OPN2Write(0, ah, 0x54, 0x99); while(Genesis_CheckOPN2Busy(0));
        Genesis_OPN2Write(0, ah, 0x58, 0x5F); while(Genesis_CheckOPN2Busy(0));
        Genesis_OPN2Write(0, ah, 0x5C, 0x94); while(Genesis_CheckOPN2Busy(0));
        Genesis_OPN2Write(0, ah, 0x60, 0x05); while(Genesis_CheckOPN2Busy(0));
        Genesis_OPN2Write(0, ah, 0x64, 0x05); while(Genesis_CheckOPN2Busy(0));
        Genesis_OPN2Write(0, ah, 0x68, 0x05); while(Genesis_CheckOPN2Busy(0));
        Genesis_OPN2Write(0, ah, 0x6C, 0x07); while(Genesis_CheckOPN2Busy(0));
        Genesis_OPN2Write(0, ah, 0x70, 0x02); while(Genesis_CheckOPN2Busy(0));
        Genesis_OPN2Write(0, ah, 0x74, 0x02); while(Genesis_CheckOPN2Busy(0));
        Genesis_OPN2Write(0, ah, 0x78, 0x02); while(Genesis_CheckOPN2Busy(0));
        Genesis_OPN2Write(0, ah, 0x7C, 0x02); while(Genesis_CheckOPN2Busy(0));
        Genesis_OPN2Write(0, ah, 0x80, 0x11); while(Genesis_CheckOPN2Busy(0));
        Genesis_OPN2Write(0, ah, 0x84, 0x11); while(Genesis_CheckOPN2Busy(0));
        Genesis_OPN2Write(0, ah, 0x88, 0x11); while(Genesis_CheckOPN2Busy(0));
        Genesis_OPN2Write(0, ah, 0x8C, 0xA6); while(Genesis_CheckOPN2Busy(0));
        Genesis_OPN2Write(0, ah, 0x90, 0x00); while(Genesis_CheckOPN2Busy(0));
        Genesis_OPN2Write(0, ah, 0x94, 0x00); while(Genesis_CheckOPN2Busy(0));
        Genesis_OPN2Write(0, ah, 0x98, 0x00); while(Genesis_CheckOPN2Busy(0));
        Genesis_OPN2Write(0, ah, 0x9C, 0x00); while(Genesis_CheckOPN2Busy(0));
        Genesis_OPN2Write(0, ah, 0xB0, 0x32); while(Genesis_CheckOPN2Busy(0));
        Genesis_OPN2Write(0, ah, 0xB4, 0xC0); while(Genesis_CheckOPN2Busy(0));
    }
    Genesis_OPN2Write(0, 0, 0xA4, 0x22); while(Genesis_CheckOPN2Busy(0));
    Genesis_OPN2Write(0, 0, 0xA0, 0x69); while(Genesis_CheckOPN2Busy(0));
    Genesis_OPN2Write(0, 0, 0x28, 0xF0); while(Genesis_CheckOPN2Busy(0));
    Genesis_OPN2Write(0, 1, 0xA4, 0x2B); while(Genesis_CheckOPN2Busy(0));
    Genesis_OPN2Write(0, 1, 0xA0, 0x47); while(Genesis_CheckOPN2Busy(0));
    Genesis_OPN2Write(0, 0, 0x28, 0xF4); while(Genesis_CheckOPN2Busy(0));
    MIOS32_DELAY_Wait_uS(0xFF00);
    MIOS32_DELAY_Wait_uS(0xFF00);
    MIOS32_DELAY_Wait_uS(0xFF00);
    MIOS32_DELAY_Wait_uS(0xFF00);
    MIOS32_DELAY_Wait_uS(0xFF00);
    MIOS32_DELAY_Wait_uS(0xFF00);
    MIOS32_DELAY_Wait_uS(0xFF00);
    MIOS32_DELAY_Wait_uS(0xFF00);
    MIOS32_DELAY_Wait_uS(0xFF00);
    MIOS32_DELAY_Wait_uS(0xFF00);
    Genesis_OPN2Write(0, 0, 0x28, 0x00); while(Genesis_CheckOPN2Busy(0));
    Genesis_OPN2Write(0, 0, 0x28, 0x04); while(Genesis_CheckOPN2Busy(0));
    MIOS32_DELAY_Wait_uS(0xFF00);
    MIOS32_DELAY_Wait_uS(0xFF00);
    MIOS32_DELAY_Wait_uS(0xFF00);
    MIOS32_DELAY_Wait_uS(0xFF00);
    MIOS32_DELAY_Wait_uS(0xFF00);
    MIOS32_DELAY_Wait_uS(0xFF00);
    MIOS32_DELAY_Wait_uS(0xFF00);
    MIOS32_DELAY_Wait_uS(0xFF00);
    MIOS32_DELAY_Wait_uS(0xFF00);
    MIOS32_DELAY_Wait_uS(0xFF00);
    Genesis_OPN2Write(0, 0, 0xA4, 0x12); while(Genesis_CheckOPN2Busy(0));
    Genesis_OPN2Write(0, 0, 0xA0, 0x69); while(Genesis_CheckOPN2Busy(0));
    Genesis_OPN2Write(0, 0, 0x28, 0xF0); while(Genesis_CheckOPN2Busy(0));
    Genesis_OPN2Write(0, 1, 0xA4, 0x1B); while(Genesis_CheckOPN2Busy(0));
    Genesis_OPN2Write(0, 1, 0xA0, 0x47); while(Genesis_CheckOPN2Busy(0));
    Genesis_OPN2Write(0, 0, 0x28, 0xF4); while(Genesis_CheckOPN2Busy(0));
    MIOS32_DELAY_Wait_uS(0xFF00);
    MIOS32_DELAY_Wait_uS(0xFF00);
    MIOS32_DELAY_Wait_uS(0xFF00);
    MIOS32_DELAY_Wait_uS(0xFF00);
    MIOS32_DELAY_Wait_uS(0xFF00);
    MIOS32_DELAY_Wait_uS(0xFF00);
    MIOS32_DELAY_Wait_uS(0xFF00);
    MIOS32_DELAY_Wait_uS(0xFF00);
    MIOS32_DELAY_Wait_uS(0xFF00);
    MIOS32_DELAY_Wait_uS(0xFF00);
    Genesis_OPN2Write(0, 0, 0x28, 0x00); while(Genesis_CheckOPN2Busy(0));
    Genesis_OPN2Write(0, 0, 0x28, 0x04); while(Genesis_CheckOPN2Busy(0));
    MIOS32_DELAY_Wait_uS(0xFF00);
    MIOS32_DELAY_Wait_uS(0xFF00);
    MIOS32_DELAY_Wait_uS(0xFF00);
    MIOS32_DELAY_Wait_uS(0xFF00);
    MIOS32_DELAY_Wait_uS(0xFF00);
    MIOS32_DELAY_Wait_uS(0xFF00);
    MIOS32_DELAY_Wait_uS(0xFF00);
    MIOS32_DELAY_Wait_uS(0xFF00);
    MIOS32_DELAY_Wait_uS(0xFF00);
    MIOS32_DELAY_Wait_uS(0xFF00);
    Genesis_OPN2Write(0, 0, 0xA4, 0x02); while(Genesis_CheckOPN2Busy(0));
    Genesis_OPN2Write(0, 0, 0xA0, 0x69); while(Genesis_CheckOPN2Busy(0));
    Genesis_OPN2Write(0, 0, 0x28, 0xF0); while(Genesis_CheckOPN2Busy(0));
    Genesis_OPN2Write(0, 1, 0xA4, 0x0B); while(Genesis_CheckOPN2Busy(0));
    Genesis_OPN2Write(0, 1, 0xA0, 0x47); while(Genesis_CheckOPN2Busy(0));
    Genesis_OPN2Write(0, 0, 0x28, 0xF4); while(Genesis_CheckOPN2Busy(0));
    MIOS32_DELAY_Wait_uS(0xFF00);
    MIOS32_DELAY_Wait_uS(0xFF00);
    MIOS32_DELAY_Wait_uS(0xFF00);
    MIOS32_DELAY_Wait_uS(0xFF00);
    MIOS32_DELAY_Wait_uS(0xFF00);
    MIOS32_DELAY_Wait_uS(0xFF00);
    MIOS32_DELAY_Wait_uS(0xFF00);
    MIOS32_DELAY_Wait_uS(0xFF00);
    MIOS32_DELAY_Wait_uS(0xFF00);
    MIOS32_DELAY_Wait_uS(0xFF00);
    Genesis_OPN2Write(0, 0, 0x28, 0x00); while(Genesis_CheckOPN2Busy(0));
    Genesis_OPN2Write(0, 0, 0x28, 0x04); while(Genesis_CheckOPN2Busy(0));
    MIOS32_DELAY_Wait_uS(0xFF00);
    MIOS32_DELAY_Wait_uS(0xFF00);
    MIOS32_DELAY_Wait_uS(0xFF00);
    MIOS32_DELAY_Wait_uS(0xFF00);
    MIOS32_DELAY_Wait_uS(0xFF00);
    MIOS32_DELAY_Wait_uS(0xFF00);
    MIOS32_DELAY_Wait_uS(0xFF00);
    MIOS32_DELAY_Wait_uS(0xFF00);
    MIOS32_DELAY_Wait_uS(0xFF00);
    MIOS32_DELAY_Wait_uS(0xFF00);
    */
    //Flash LEDs
    //MIOS32_BOARD_LED_Set(0xF, ((count >> 12) & 0xF));
    ++count;
    MIOS32_BOARD_LED_Set(0b1000, 0b0000);
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called each mS from the main task which also handles DIN, ENC
// and AIN events. You could add more jobs here, but they shouldn't consume
// more than 300 uS to ensure the responsiveness of buttons, encoders, pots.
// Alternatively you could create a dedicated task for application specific
// jobs as explained in $MIOS32_PATH/apps/tutorials/006_rtos_tasks
/////////////////////////////////////////////////////////////////////////////
extern "C" void APP_Tick(void)
{
  // PWM modulate the status LED (this is a sign of life)
  //u32 timestamp = MIOS32_TIMESTAMP_Get();
  //MIOS32_BOARD_LED_Set(1, (timestamp % 20) <= ((timestamp / 100) % 10));
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called each mS from the MIDI task which checks for incoming
// MIDI events. You could add more MIDI related jobs here, but they shouldn't
// consume more than 300 uS to ensure the responsiveness of incoming MIDI.
/////////////////////////////////////////////////////////////////////////////
extern "C" void APP_MIDI_Tick(void)
{
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called when a MIDI package has been received
/////////////////////////////////////////////////////////////////////////////
extern "C" void APP_MIDI_NotifyPackage(mios32_midi_port_t port, mios32_midi_package_t midi_package)
{
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called before the shift register chain is scanned
/////////////////////////////////////////////////////////////////////////////
extern "C" void APP_SRIO_ServicePrepare(void)
{
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called after the shift register chain has been scanned
/////////////////////////////////////////////////////////////////////////////
extern "C" void APP_SRIO_ServiceFinish(void)
{
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called when a button has been toggled
// pin_value is 1 when button released, and 0 when button pressed
/////////////////////////////////////////////////////////////////////////////
extern "C" void APP_DIN_NotifyToggle(u32 pin, u32 pin_value)
{
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called when an encoder has been moved
// incrementer is positive when encoder has been turned clockwise, else
// it is negative
/////////////////////////////////////////////////////////////////////////////
extern "C" void APP_ENC_NotifyChange(u32 encoder, s32 incrementer)
{
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called when a pot has been moved
/////////////////////////////////////////////////////////////////////////////
extern "C" void APP_AIN_NotifyChange(u32 pin, u32 pin_value)
{
}
