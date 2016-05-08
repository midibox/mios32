/*
 * MIDIbox Quad Genesis: System Mode header
 *
 * ==========================================================================
 *
 *  Copyright (C) 2016 Sauraen (sauraen@gmail.com)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _MODE_SYSTEM_H
#define _MODE_SYSTEM_H

#ifdef __cplusplus
extern "C" {
#endif


extern void Mode_System_Init();
extern void Mode_System_GotFocus();

extern void Mode_System_Tick();
extern void Mode_System_Background();

extern void Mode_System_BtnGVoice(u8 gvoice, u8 state);
extern void Mode_System_BtnSoftkey(u8 softkey, u8 state);
extern void Mode_System_BtnSelOp(u8 op, u8 state);
extern void Mode_System_BtnOpMute(u8 op, u8 state);
extern void Mode_System_BtnSystem(u8 button, u8 state);
extern void Mode_System_BtnEdit(u8 button, u8 state);

extern void Mode_System_EncDatawheel(s32 incrementer);
extern void Mode_System_EncEdit(u8 encoder, s32 incrementer);


#ifdef __cplusplus
}
#endif


#endif /* _MODE_SYSTEM_H */
