/*
 * MIDIbox Quad Genesis: Name editor header
 *
 * ==========================================================================
 *
 *  Copyright (C) 2016 Sauraen (sauraen@gmail.com)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _NAMEEDITOR_H
#define _NAMEEDITOR_H

#ifdef __cplusplus
extern "C" {
#endif

extern void NameEditor_Start(char* nametoedit, u8 namelen, const char* prompt, void (*callback)(void));

extern void NameEditor_BtnSoftkey(u8 softkey, u8 state);
extern void NameEditor_BtnSystem(u8 button, u8 state);
extern void NameEditor_EncDatawheel(s32 incrementer);


#ifdef __cplusplus
}
#endif


#endif /* _NAMEEDITOR_H */
