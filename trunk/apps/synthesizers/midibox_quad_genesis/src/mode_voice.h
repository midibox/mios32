/*
 * MIDIbox Quad Genesis: Voice Mode header
 *
 * ==========================================================================
 *
 *  Copyright (C) 2016 Sauraen (sauraen@gmail.com)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _MODE_VOICE_H
#define _MODE_VOICE_H

#ifdef __cplusplus
extern "C" {
#endif


extern const char* GetVoiceName(u8 subvoice);

extern void Mode_Voice_Init();
extern void Mode_Voice_GotFocus();

extern void Mode_Voice_Tick();
extern void Mode_Voice_Background();

extern void Mode_Voice_BtnGVoice(u8 gvoice, u8 state);
extern void Mode_Voice_BtnSoftkey(u8 softkey, u8 state);
extern void Mode_Voice_BtnSelOp(u8 op, u8 state);
extern void Mode_Voice_BtnOpMute(u8 op, u8 state);
extern void Mode_Voice_BtnSystem(u8 button, u8 state);
extern void Mode_Voice_BtnEdit(u8 button, u8 state);

extern void Mode_Voice_EncDatawheel(s32 incrementer);
extern void Mode_Voice_EncEdit(u8 encoder, s32 incrementer);


#ifdef __cplusplus
}
#endif


#endif /* _MODE_VOICE_H */
