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
#include <vgm.h>


char* filenamelist;
s32 numfiles;
u8 selfile;
u8 updatescreen;
u8 selgenesis;
VgmSource* sources[4];
VgmHead* heads[4];

static const u8 charset_mbvgm[8*8] = {
  //Unused character 0
  0,0,0,0,0,0,0,0,
  //Play symbol, character 1
  0b00000000,
  0b00001000,
  0b00001100,
  0b00001110,
  0b00001100,
  0b00001000,
  0b00000000,
  0b00000000,
  //Stop symbol, character 2
  0b00000000,
  0b00000000,
  0b00001110,
  0b00001110,
  0b00001110,
  0b00000000,
  0b00000000,
  0b00000000,
  //Filled bullet, character 3
  0b00000000,
  0b00001110,
  0b00011111,
  0b00011111,
  0b00011111,
  0b00001110,
  0b00000000,
  0b00000000,
  //Unused character 4
  0,0,0,0,0,0,0,0,
  //Unused character 5
  0,0,0,0,0,0,0,0,
  //Unused character 6
  0,0,0,0,0,0,0,0,
  //Unused character 7
  0,0,0,0,0,0,0,0
};

/////////////////////////////////////////////////////////////////////////////
// This hook is called after startup to initialize the application
/////////////////////////////////////////////////////////////////////////////

void APP_Init(void){
    // initialize all LEDs
    MIOS32_BOARD_LED_Init(0xffffffff);

    // initialize SD card
    FILE_Init(0);

    //Initialize MBHP_Genesis module
    Genesis_Init();

    //Initialize VGM Player component
    VGM_Init();
    
    //Initialize front panel
    mios32_enc_config_t enc_config = MIOS32_ENC_ConfigGet(0);
    enc_config.cfg.type = DETENTED3;
    enc_config.cfg.speed = NORMAL;
    enc_config.cfg.speed_par = 0;
    enc_config.cfg.sr  = 1;
    enc_config.cfg.pos = 0;
    MIOS32_ENC_ConfigSet(0, enc_config);
    
    //Initialize LCD
    MIOS32_LCD_SpecialCharsInit((u8 *)charset_mbvgm);
    MIOS32_LCD_Clear();
    MIOS32_LCD_CursorSet(0,0);
    MIOS32_LCD_PrintString("Searching for SD card...");
    
    //Initialize global variables
    numfiles = 0;
    updatescreen = 1;
    selgenesis = 0;
    selfile = 0;
    
}


/////////////////////////////////////////////////////////////////////////////
// This task is running endless in background
/////////////////////////////////////////////////////////////////////////////
void APP_Background(void){
    MIOS32_BOARD_LED_Set(0b1000, 0b1000);
    //MIOS32_BOARD_LED_Set(0b1000, 0b0000);
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
    s32 res;
    ++prescaler;
    char* tempbuf; u8 i;
    if(prescaler % 100 == 0){
        if(prescaler == 1000){
            prescaler = 0;
            VGM_PerfMon_Second();
            updatescreen = 1;
        }
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
                updatescreen = 1;
            }
            break;
        case 3:
            if(updatescreen){
                tempbuf = malloc(9);
                for(i=0; i<8; i++){
                    tempbuf[i] = filenamelist[(9*selfile)+i];
                }
                tempbuf[8] = 0;
                MIOS32_LCD_Clear();
                MIOS32_LCD_CursorSet(0,0);
                MIOS32_LCD_PrintFormattedString("%d/%d:%s.vgm", selfile, numfiles, tempbuf);
                for(i=0; i<4; i++){
                    MIOS32_LCD_CursorSet(5*i,1);
                    MIOS32_LCD_PrintFormattedString(" %c%c  ", (sources[i] == NULL ? 2 : 1), (i == selgenesis ? 3 : ' '));
                }
                MIOS32_LCD_CursorSet(31,0);
                MIOS32_LCD_PrintFormattedString("Chip:%3d%%", VGM_PerfMon_GetTaskCPU(VGM_PERFMON_TASK_CHIP));
                MIOS32_LCD_CursorSet(31,1);
                MIOS32_LCD_PrintFormattedString("Card:%3d%%", VGM_PerfMon_GetTaskCPU(VGM_PERFMON_TASK_CARD));
                free(tempbuf);
                updatescreen = 0;
            }
            break;
        }
    }
}



/////////////////////////////////////////////////////////////////////////////
// This hook is called when a button has been toggled
// pin_value is 1 when button released, and 0 when button pressed
/////////////////////////////////////////////////////////////////////////////
void APP_DIN_NotifyToggle(u32 pin, u32 pin_value){
    DBG("DIN pin %d changed %d", pin, pin_value);
    if(pin_value) return; //button just released
    if(pin >= 4 && pin <= 7){
        DBG("Pressed softkey %d", pin-4);
        selgenesis = pin - 4;
        updatescreen = 1;
    }else if(pin == 2){
        DBG("Pressed play");
        u8 i;
        if(sources[selgenesis] != NULL){
            //Stop first
            VGM_Head_Delete(heads[selgenesis]);
            VGM_Source_Delete(sources[selgenesis]);
            sources[selgenesis] = NULL;
            heads[selgenesis] = NULL;
            Genesis_Reset(selgenesis);
        }
        char* tempbuf = malloc(13);
        for(i=0; i<8; i++){
            if(filenamelist[(9*selfile)+i] <= ' ') break;
            tempbuf[i] = filenamelist[(9*selfile)+i];
        }
        tempbuf[i++] = '.';
        tempbuf[i++] = 'v';
        tempbuf[i++] = 'g';
        tempbuf[i++] = 'm';
        tempbuf[i++] = 0;
        VgmSource* vgms = VGM_SourceStream_Create();
        sources[selgenesis] = vgms;
        s32 res = VGM_SourceStream_Start(vgms, tempbuf);
        if(res >= 0){
            VgmHead* vgmh = VGM_Head_Create(vgms);
            heads[selgenesis] = vgmh;
            VGM_Head_Restart(vgmh, VGM_Player_GetVGMTime());
            for(i=0; i<0xC; ++i){
                vgmh->channel[i].map_chip = selgenesis;
            }
            vgmh->playing = 1;
        }else{
            VGM_Source_Delete(vgms);
        }
        updatescreen = 1;
        free(tempbuf);
    }else if(pin == 3){
        DBG("Pressed menu");
        if(sources[selgenesis] != NULL){
            VGM_Head_Delete(heads[selgenesis]);
            VGM_Source_Delete(sources[selgenesis]);
            sources[selgenesis] = NULL;
            heads[selgenesis] = NULL;
            Genesis_Reset(selgenesis);
            updatescreen = 1;
        }
    }
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called when an encoder has been moved
// incrementer is positive when encoder has been turned clockwise, else
// it is negative
/////////////////////////////////////////////////////////////////////////////
void APP_ENC_NotifyChange(u32 encoder, s32 incrementer){
    DBG("Encoder %d change %d", encoder, incrementer);
    if(encoder) return; //Should be only one encoder, index 0
    s32 newselfile = incrementer + (s32)selfile;
    if(newselfile < numfiles && newselfile >= 0){
        selfile = newselfile;
        updatescreen = 1;
    }
}


void APP_MIDI_Tick(void){}
void APP_MIDI_NotifyPackage(mios32_midi_port_t port, mios32_midi_package_t midi_package){}
void APP_SRIO_ServicePrepare(void){}
void APP_SRIO_ServiceFinish(void){}
void APP_AIN_NotifyChange(u32 pin, u32 pin_value){}

