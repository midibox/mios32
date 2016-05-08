/*
 * MIDIbox Quad Genesis: Channel Mode
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
#include "mode_chan.h"

void Mode_Chan_Init(){

}
void Mode_Chan_GotFocus(){
    MIOS32_LCD_Clear();
    MIOS32_LCD_CursorSet(0,0);
    MIOS32_LCD_PrintString("Chan mode not yet implemented");
}

void Mode_Chan_Tick(){

}
void Mode_Chan_Background(){

}

void Mode_Chan_BtnGVoice(u8 gvoice, u8 state){

}
void Mode_Chan_BtnSoftkey(u8 softkey, u8 state){

}
void Mode_Chan_BtnSelOp(u8 op, u8 state){

}
void Mode_Chan_BtnOpMute(u8 op, u8 state){

}
void Mode_Chan_BtnSystem(u8 button, u8 state){

}
void Mode_Chan_BtnEdit(u8 button, u8 state){

}

void Mode_Chan_EncDatawheel(s32 incrementer){

}
void Mode_Chan_EncEdit(u8 encoder, s32 incrementer){

}
