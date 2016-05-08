/*
 * MIDIbox Quad Genesis: Modulator Mode
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
#include "mode_mdltr.h"

void Mode_Mdltr_Init(){

}
void Mode_Mdltr_GotFocus(){
    MIOS32_LCD_Clear();
    MIOS32_LCD_CursorSet(0,0);
    MIOS32_LCD_PrintString("Mdltr mode not yet implemented");
}

void Mode_Mdltr_Tick(){

}
void Mode_Mdltr_Background(){

}

void Mode_Mdltr_BtnGVoice(u8 gvoice, u8 state){

}
void Mode_Mdltr_BtnSoftkey(u8 softkey, u8 state){

}
void Mode_Mdltr_BtnSelOp(u8 op, u8 state){

}
void Mode_Mdltr_BtnOpMute(u8 op, u8 state){

}
void Mode_Mdltr_BtnSystem(u8 button, u8 state){

}
void Mode_Mdltr_BtnEdit(u8 button, u8 state){

}

void Mode_Mdltr_EncDatawheel(s32 incrementer){

}
void Mode_Mdltr_EncEdit(u8 encoder, s32 incrementer){

}
