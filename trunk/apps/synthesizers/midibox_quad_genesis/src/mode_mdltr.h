/*
 * MIDIbox Quad Genesis: Modulator Mode header
 *
 * ==========================================================================
 *
 *  Copyright (C) 2016 Sauraen (sauraen@gmail.com)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _MODE_MDLTR_H
#define _MODE_MDLTR_H

#ifdef __cplusplus
extern "C" {
#endif


extern void Mode_Mdltr_Init();
extern void Mode_Mdltr_GotFocus();

extern void Mode_Mdltr_Tick();
extern void Mode_Mdltr_Background();

extern void Mode_Mdltr_BtnGVoice(u8 gvoice, u8 state);
extern void Mode_Mdltr_BtnSoftkey(u8 softkey, u8 state);
extern void Mode_Mdltr_BtnSelOp(u8 op, u8 state);
extern void Mode_Mdltr_BtnOpMute(u8 op, u8 state);
extern void Mode_Mdltr_BtnSystem(u8 button, u8 state);
extern void Mode_Mdltr_BtnEdit(u8 button, u8 state);

extern void Mode_Mdltr_EncDatawheel(s32 incrementer);
extern void Mode_Mdltr_EncEdit(u8 encoder, s32 incrementer);


#ifdef __cplusplus
}
#endif


#endif /* _MODE_MDLTR_H */
