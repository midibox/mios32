/*
 * MIDIbox Quad Genesis: File browser header
 *
 * ==========================================================================
 *
 *  Copyright (C) 2016 Sauraen (sauraen@gmail.com)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _FILEBROWSER_H
#define _FILEBROWSER_H

#ifdef __cplusplus
extern "C" {
#endif

extern void Filebrowser_Init();
extern void Filebrowser_Start(const char* initpath, const char* extension, u8 save, void (*callback)(char* filename));

extern void Filebrowser_BtnSoftkey(u8 softkey, u8 state);
extern void Filebrowser_BtnSystem(u8 button, u8 state);
extern void Filebrowser_EncDatawheel(s32 incrementer);


#ifdef __cplusplus
}
#endif


#endif /* _FILEBROWSER_H */
