/*
 * MIDIbox Quad Genesis: Program Mode
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
#include "mode_prog.h"

void Mode_Prog_Init(){

}
void Mode_Prog_GotFocus(){
    MIOS32_LCD_Clear();
    MIOS32_LCD_CursorSet(0,0);
    MIOS32_LCD_PrintString("Prog mode not yet implemented");
}

void Mode_Prog_Tick(){

}
void Mode_Prog_Background(){

}

void Mode_Prog_BtnGVoice(u8 gvoice, u8 state){

}
void Mode_Prog_BtnSoftkey(u8 softkey, u8 state){

}
void Mode_Prog_BtnSelOp(u8 op, u8 state){

}
void Mode_Prog_BtnOpMute(u8 op, u8 state){

}
void Mode_Prog_BtnSystem(u8 button, u8 state){

}
void Mode_Prog_BtnEdit(u8 button, u8 state){

}

void Mode_Prog_EncDatawheel(s32 incrementer){

}
void Mode_Prog_EncEdit(u8 encoder, s32 incrementer){

}
