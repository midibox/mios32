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

#include <blm_x.h>
#include <file.h>
#include <genesis.h>
#include <vgm.h>
#include "frontpanel.h"
#include "genesisstate.h"
#include "interface.h"
#include "syeng.h"
#include "tracker.h"

u8 DEBUG;
u8 DEBUG2;

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
    //Initialize VGM Second Heap
    vgmh2_init();
    
    // initialize all LEDs
    MIOS32_BOARD_LED_Init(0xffffffff);

    // initialize SD card
    FILE_Init(0);

    //Initialize MBHP_Genesis module
    Genesis_Init();

    //Initialize VGM Player component
    VGM_Init();
    
    //Initialize LCD
    MIOS32_LCD_SpecialCharsInit((u8 *)charset_mbvgm);
    
    //Initialize front panel
    BLM_X_Init();
    FrontPanel_Init();
    
    //Initialize interface
    Interface_Init();
    
    
}


/////////////////////////////////////////////////////////////////////////////
// This task is running endless in background
/////////////////////////////////////////////////////////////////////////////
void APP_Background(void){
    static u8 didsdcard = 0;
    s32 res;
    if(!didsdcard){
        while(1){
            res = FILE_CheckSDCard();
            if(res == 3 || res < 0) break;
            for(res = 0; res < 100; ++res){
                MIOS32_DELAY_Wait_uS(1000);
            }
        }
        if(res < 0){
            MIOS32_LCD_Clear();
            MIOS32_LCD_CursorSet(0,0);
            MIOS32_LCD_PrintString("ERRORERRORERRORERRORERRORERRORERRORERROR");
        }
        DEBUG = res;
        for(res = 0; res < 1000; ++res){
            MIOS32_DELAY_Wait_uS(1000);
        }
        
        //Initialize synth engine
        SyEng_Init();
        
        didsdcard = 1;
    }
    MIOS32_BOARD_LED_Set(0b1000, 0b1000);
    Interface_Background();
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
    static u16 prescaler = 0;
    BLM_X_BtnHandler((void*)&FrontPanel_ButtonChange);
    SyEng_Tick();
    GenesisState_Tick();
    Interface_Tick();
    ++prescaler;
    if(prescaler == 100){
        prescaler = 0;
        VGM_PerfMon_Periodic();
        vgm_meminfo_t m = VGM_PerfMon_GetMemInfo();
        FrontPanel_DrawLoad(0, VGM_PerfMon_GetTaskCPU(VGM_PERFMON_TASK_CHIP) / 10); // 50% load shows as 100%
        FrontPanel_DrawLoad(1, VGM_PerfMon_GetTaskCPU(VGM_PERFMON_TASK_CARD) / 2); // 10% load shows as 100%
        FrontPanel_DrawLoad(2, (u8)(((u32)m.main_used + (u32)m.vgmh2_used) * 9ul / ((u32)m.main_total + (u32)m.vgmh2_total)));
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
void APP_MIDI_NotifyPackage(mios32_midi_port_t port, mios32_midi_package_t midi_package){
    if((midi_package.event == 0x8 || midi_package.event == 0x9 || midi_package.event == 0xB)
            && channels[midi_package.chn].trackermode){
        u8 trackervoice = channels[midi_package.chn].trackervoice;
        u8 g = trackervoice >> 4;
        u8 v = trackervoice & 0xF;
        VGM_MidiToGenesis(midi_package, g, v, 0, 0); //TODO
        return;
    }
    if(midi_package.event == 0x8 || (midi_package.event == 0x9 && midi_package.velocity == 0)){
        SyEng_Note_Off(midi_package);
    }else if(midi_package.event == 0x9){
        SyEng_Note_On(midi_package);
    }
}

