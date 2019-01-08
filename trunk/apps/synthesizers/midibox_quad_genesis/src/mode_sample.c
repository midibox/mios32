/*
 * MIDIbox Quad Genesis: Sample Mode
 *
 * ==========================================================================
 *
 *  Copyright (C) 2016 Sauraen (sauraen@gmail.com)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#include <mios32.h>
#include "mode_sample.h"

void Mode_Sample_Init(){

}
void Mode_Sample_GotFocus(){
    MIOS32_LCD_Clear();
    MIOS32_LCD_CursorSet(0,0);
    MIOS32_LCD_PrintString("Sample mode not yet implemented");
}

void Mode_Sample_Tick(){

}
void Mode_Sample_Background(){

}

void Mode_Sample_BtnGVoice(u8 gvoice, u8 state){

}
void Mode_Sample_BtnSoftkey(u8 softkey, u8 state){

}
void Mode_Sample_BtnSelOp(u8 op, u8 state){

}
void Mode_Sample_BtnOpMute(u8 op, u8 state){

}
void Mode_Sample_BtnSystem(u8 button, u8 state){

}
void Mode_Sample_BtnEdit(u8 button, u8 state){

}

void Mode_Sample_EncDatawheel(s32 incrementer){

}
void Mode_Sample_EncEdit(u8 encoder, s32 incrementer){

}
