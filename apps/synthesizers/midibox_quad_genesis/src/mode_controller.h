/*
 * MIDIbox Quad Genesis: Controller Mode header
 *
 * ==========================================================================
 *
 *  Copyright (C) 2018 Sauraen (sauraen@gmail.com)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _MODE_CONTROLLER_H
#define _MODE_CONTROLLER_H

#ifdef __cplusplus
extern "C" {
#endif


extern void Mode_Controller_Init();
extern void Mode_Controller_GotFocus();

extern void Mode_Controller_Tick();
extern void Mode_Controller_Background();

extern void Mode_Controller_BtnGVoice(u8 gvoice, u8 state);
extern void Mode_Controller_BtnSoftkey(u8 softkey, u8 state);
extern void Mode_Controller_BtnSelOp(u8 op, u8 state);
extern void Mode_Controller_BtnOpMute(u8 op, u8 state);
extern void Mode_Controller_BtnSystem(u8 button, u8 state);
extern void Mode_Controller_BtnEdit(u8 button, u8 state);

extern void Mode_Controller_EncDatawheel(s32 incrementer);
extern void Mode_Controller_EncEdit(u8 encoder, s32 incrementer);


#ifdef __cplusplus
}
#endif


#endif /* _MODE_CONTROLLER_H */
