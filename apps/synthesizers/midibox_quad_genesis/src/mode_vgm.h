/*
 * MIDIbox Quad Genesis: VGM Mode header
 *
 * ==========================================================================
 *
 *  Copyright (C) 2016 Sauraen (sauraen@gmail.com)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _MODE_VGM_H
#define _MODE_VGM_H

#ifdef __cplusplus
extern "C" {
#endif


extern void Mode_Vgm_Init();
extern void Mode_Vgm_GotFocus();

extern void Mode_Vgm_Tick();
extern void Mode_Vgm_Background();

extern void Mode_Vgm_BtnGVoice(u8 gvoice, u8 state);
extern void Mode_Vgm_BtnSoftkey(u8 softkey, u8 state);
extern void Mode_Vgm_BtnSelOp(u8 op, u8 state);
extern void Mode_Vgm_BtnOpMute(u8 op, u8 state);
extern void Mode_Vgm_BtnSystem(u8 button, u8 state);
extern void Mode_Vgm_BtnEdit(u8 button, u8 state);

extern void Mode_Vgm_EncDatawheel(s32 incrementer);
extern void Mode_Vgm_EncEdit(u8 encoder, s32 incrementer);


#ifdef __cplusplus
}
#endif


#endif /* _MODE_VGM_H */
