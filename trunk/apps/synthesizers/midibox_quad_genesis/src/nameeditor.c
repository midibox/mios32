/*
 * MIDIbox Quad Genesis: Name editor
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
#include "nameeditor.h"

#include <string.h>

#include "frontpanel.h"
#include "interface.h"

static void (*callback_f)(void);
static char* name;
u8 name_len;
static const char* prompt_msg;
u8 prompt_len;
u8 cursor;

static void DrawMenu(){
    MIOS32_LCD_Clear();
    MIOS32_LCD_CursorSet(0,0);
    MIOS32_LCD_PrintFormattedString("%s: %s", prompt_msg, name);
    MIOS32_LCD_CursorSet(prompt_len+2+cursor,1);
    MIOS32_LCD_PrintChar('^');
    MIOS32_LCD_CursorSet(27,1);
    MIOS32_LCD_PrintString("<    >   Aa");
}

void NameEditor_Start(char* nametoedit, u8 namelen, const char* prompt, void (*callback)(void)){
    subscreen = SUBSCREEN_NAMEEDITOR;
    callback_f = callback;
    name = nametoedit;
    name_len = namelen;
    prompt_msg = prompt;
    prompt_len = strlen(prompt_msg);
    cursor = 0;
    DrawMenu();
}

void NameEditor_BtnSoftkey(u8 softkey, u8 state){
    if(!state) return;
    switch(softkey){
        case 5:
            if(cursor == 0) return;
            u8 i;
            for(i=cursor; i<name_len; ++i){
                if(name[i] > 0x20) break;
            }
            if(i == name_len) name[cursor] = 0;
            --cursor;
            DrawMenu();
            break;
        case 6:
            if(cursor >= name_len-1) return;
            ++cursor;
            if(name[cursor] == 0){
                name[cursor] = name[cursor-1];
                name[cursor+1] = 0;
            }
            DrawMenu();
            break;
        case 7:
            if(name[cursor] >= 'A' && name[cursor] <= 'Z'){
                name[cursor] += 'a' - 'A';
            }else if(name[cursor] >= 'a' && name[cursor] <= 'z'){
                name[cursor] -= 'a' - 'A';
            }else if(name[cursor] >= 'a'){
                name[cursor] = 'n';
            }else{
                name[cursor] = 'N';
            }
            MIOS32_LCD_CursorSet(prompt_len+2+cursor,0);
            MIOS32_LCD_PrintChar(name[cursor]);
            break;
    }
}
void NameEditor_BtnSystem(u8 button, u8 state){
    if(!state) return;
    if(button == FP_B_ENTER){
        subscreen = 0;
        callback_f();
        return;
    }
}
void NameEditor_EncDatawheel(s32 incrementer){
    if(        (incrementer < 0 && (s32)name[cursor] + incrementer >= 0x20) 
            || (incrementer > 0 && (s32)name[cursor] + incrementer <= 0x7F)){
        name[cursor] =        (u8)((s32)name[cursor] + incrementer);
        MIOS32_LCD_CursorSet(prompt_len+2+cursor,0);
        MIOS32_LCD_PrintChar(name[cursor]);
    }
}
