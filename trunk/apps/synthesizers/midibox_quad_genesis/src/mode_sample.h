/*
 * MIDIbox Quad Genesis: Sample Mode header
 *
 * ==========================================================================
 *
 *  Copyright (C) 2016 Sauraen (sauraen@gmail.com)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _MODE_SAMPLE_H
#define _MODE_SAMPLE_H

#ifdef __cplusplus
extern "C" {
#endif


extern void Mode_Sample_Init();
extern void Mode_Sample_GotFocus();

extern void Mode_Sample_Tick();
extern void Mode_Sample_Background();

extern void Mode_Sample_BtnGVoice(u8 gvoice, u8 state);
extern void Mode_Sample_BtnSoftkey(u8 softkey, u8 state);
extern void Mode_Sample_BtnSelOp(u8 op, u8 state);
extern void Mode_Sample_BtnOpMute(u8 op, u8 state);
extern void Mode_Sample_BtnSystem(u8 button, u8 state);
extern void Mode_Sample_BtnEdit(u8 button, u8 state);

extern void Mode_Sample_EncDatawheel(s32 incrementer);
extern void Mode_Sample_EncEdit(u8 encoder, s32 incrementer);


#ifdef __cplusplus
}
#endif


#endif /* _MODE_SAMPLE_H */
