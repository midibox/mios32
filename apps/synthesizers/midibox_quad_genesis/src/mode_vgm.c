/*
 * MIDIbox Quad Genesis: VGM Editor Mode
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
#include "mode_vgm.h"

void Mode_Vgm_Init(){

}
void Mode_Vgm_GotFocus(){
    MIOS32_LCD_Clear();
    MIOS32_LCD_CursorSet(0,0);
    MIOS32_LCD_PrintString("Vgm mode not yet implemented");
}

void Mode_Vgm_Tick(){

}
void Mode_Vgm_Background(){

}

void Mode_Vgm_BtnGVoice(u8 gvoice, u8 state){

}
void Mode_Vgm_BtnSoftkey(u8 softkey, u8 state){

}
void Mode_Vgm_BtnSelOp(u8 op, u8 state){

}
void Mode_Vgm_BtnOpMute(u8 op, u8 state){

}
void Mode_Vgm_BtnSystem(u8 button, u8 state){

}
void Mode_Vgm_BtnEdit(u8 button, u8 state){

}

void Mode_Vgm_EncDatawheel(s32 incrementer){

}
void Mode_Vgm_EncEdit(u8 encoder, s32 incrementer){

}
