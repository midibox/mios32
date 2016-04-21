/*
 * MIDIbox Quad Genesis: Main application
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
#include <blm_x.h>

#include "frontpanel.h"
#include "genesisstate.h"
#include "interface.h"


char* filenamelist;
s32 numfiles;
u8 selfile;
u8 updatescreen;
u8 selgenesis;
u8 vegasactive;
u8 selgvoice;
VgmSource* sources[4];
VgmHead* heads[4];
VgmSource* qsource;
VgmHead* qhead;

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
    
    qsource = VGM_SourceQueue_Create();
    qhead = VGM_Head_Create(qsource);
    qhead->playing = 1;
    
    //Initialize front panel
    BLM_X_Init();
    FrontPanel_Init();
    Interface_Init();
    
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
    
    if(!vegasactive){
        //Draw Genesis states
        u8 g;
        for(g=0; g<GENESIS_COUNT; g++){
            DrawGenesisActivity(g);
        }
    }
    
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
    static u8 vegasstate = 0;
    static u32 vegascounter = 0;
    static u8 vegassub = 0;
    
    BLM_X_BtnHandler((void*)&FrontPanel_ButtonChange);
    
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
    
    if(vegasactive){
        switch(vegasstate){
        case 0:
            BLM_X_LEDSet(((vegascounter & 7) * 88) + (vegascounter >> 3), 0, 1);
            if(++vegascounter == 88*8){
                vegascounter = 0;
                vegasstate = 1;
            }
            break;
        case 1:
            BLM_X_LEDSet(((vegascounter & 7) * 88) + (vegascounter >> 3), 0, 0);
            if(++vegascounter == 88*8){
                vegascounter = 0;
                if(++vegassub == 2){
                    vegassub = 0;
                    vegasstate = 2;
                }else{
                    vegasstate = 0;
                }
            }
            break;
        case 2:
            BLM_X_LEDSet(vegascounter, 0, 1);
            if(++vegascounter == 88*8){
                vegascounter = 0;
                vegasstate = 3;
            }
            break;
        case 3:
            BLM_X_LEDSet(vegascounter, 0, 0);
            if(++vegascounter == 88*8){
                vegascounter = 0;
                if(++vegassub == 2){
                    vegassub = 0;
                    vegasstate = 4;
                }else{
                    vegasstate = 2;
                }
            }
            break;
        case 4:
            for(i=0; i<18; i++){
                FrontPanel_LEDRingSet(i, 0, (i&1) ? (15 - ((vegascounter >> 6) & 0xF)) : ((vegascounter >> 6) & 0xF));
            }
            FrontPanel_GenesisLEDSet((vegascounter >> 6) & 3, (vegascounter & 0xF), (vegascounter >> 8) & 1, 1);
            FrontPanel_GenesisLEDSet((vegascounter >> 6) & 3, (vegascounter & 0xF), ~(vegascounter >> 8) & 1, 0);
            if(++vegascounter == 0x400){
                vegascounter = 0;
                vegasstate = 5;
            }
            break;
        case 5:
            for(i=0; i<18; i++){
                FrontPanel_LEDRingSet(i, 0, !(i&1) ? (15 - ((vegascounter >> 6) & 0xF)) : ((vegascounter >> 6) & 0xF));
            }
            FrontPanel_GenesisLEDSet(3 - ((vegascounter >> 6) & 3), (vegascounter & 0xF), (vegascounter >> 8) & 1, 1);
            FrontPanel_GenesisLEDSet(3 - ((vegascounter >> 6) & 3), (vegascounter & 0xF), ~(vegascounter >> 8) & 1, 0);
            if(++vegascounter == 0x400){
                vegascounter = 0;
                if(++vegassub == 2){
                    vegassub = 0;
                    vegasstate = 6;
                }else{
                    vegasstate = 4;
                }
            }
            break;
        case 6:
            for(i=0; i<18; i++){
                FrontPanel_LEDRingSet(i, 1, ((vegascounter >> 6) & 0xF));
            }
            FrontPanel_GenesisLEDSet(((vegascounter >> 6) & 3), (vegascounter & 0xF), 0, ((vegascounter >> 8) & 1) ^ ((vegascounter >> 6) & 1) ^ (vegascounter & 1));
            FrontPanel_GenesisLEDSet(((vegascounter >> 6) & 3), (vegascounter & 0xF), 1, ((vegascounter >> 8) & 1) ^ ((vegascounter >> 6) & 1) ^ (~vegascounter & 1));
            if(++vegascounter == 0x400){
                vegascounter = 0;
                vegasstate = 7;
            }
            break;
        case 7:
            for(i=0; i<18; i++){
                FrontPanel_LEDRingSet(i, 1, 15 - ((vegascounter >> 6) & 0xF));
            }
            FrontPanel_GenesisLEDSet(((vegascounter >> 6) & 3), (vegascounter & 0xF), 0, ((vegascounter >> 8) & 1) ^ ((vegascounter >> 6) & 1) ^ (vegascounter & 1));
            FrontPanel_GenesisLEDSet(((vegascounter >> 6) & 3), (vegascounter & 0xF), 1, ((vegascounter >> 8) & 1) ^ ((vegascounter >> 6) & 1) ^ (~vegascounter & 1));
            if(++vegascounter == 0x400){
                vegascounter = 0;
                if(++vegassub == 1){
                    vegassub = 0;
                    vegasstate = 0;
                }else{
                    vegasstate = 6;
                }
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
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called when an encoder has been moved
// incrementer is positive when encoder has been turned clockwise, else
// it is negative
/////////////////////////////////////////////////////////////////////////////
void APP_ENC_NotifyChange(u32 encoder, s32 incrementer){
    FrontPanel_EncoderChange(encoder, incrementer);
}


void APP_SRIO_ServicePrepare(void){
    BLM_X_PrepareRow();
}
void APP_SRIO_ServiceFinish(void){
    BLM_X_GetRow();
}
void APP_AIN_NotifyChange(u32 pin, u32 pin_value){}
void APP_MIDI_Tick(void){}
void APP_MIDI_NotifyPackage(mios32_midi_port_t port, mios32_midi_package_t midi_package){}

