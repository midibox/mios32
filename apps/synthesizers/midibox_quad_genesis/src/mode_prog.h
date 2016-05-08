/*
 * MIDIbox Quad Genesis: Program Mode header
 *
 * ==========================================================================
 *
 *  Copyright (C) 2016 Sauraen (sauraen@gmail.com)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _MODE_PROG_H
#define _MODE_PROG_H

#ifdef __cplusplus
extern "C" {
#endif


extern void Mode_Prog_Init();
extern void Mode_Prog_GotFocus();

extern void Mode_Prog_Tick();
extern void Mode_Prog_Background();

extern void Mode_Prog_BtnGVoice(u8 gvoice, u8 state);
extern void Mode_Prog_BtnSoftkey(u8 softkey, u8 state);
extern void Mode_Prog_BtnSelOp(u8 op, u8 state);
extern void Mode_Prog_BtnOpMute(u8 op, u8 state);
extern void Mode_Prog_BtnSystem(u8 button, u8 state);
extern void Mode_Prog_BtnEdit(u8 button, u8 state);

extern void Mode_Prog_EncDatawheel(s32 incrementer);
extern void Mode_Prog_EncEdit(u8 encoder, s32 incrementer);


#ifdef __cplusplus
}
#endif


#endif /* _MODE_PROG_H */
