/*
 * MIDIbox Quad Genesis: Main Interface
 *
 * ==========================================================================
 *
 *  Copyright (C) 2016 Sauraen (sauraen@gmail.com)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _INTERFACE_H
#define _INTERFACE_H

#ifdef __cplusplus
extern "C" {
#endif


//Data

#define MODE_SYSTEM 0
#define MODE_VOICE 1
#define MODE_CHAN 2
#define MODE_PROG 3
#define MODE_VGM 4
#define MODE_MDLTR 5
#define MODE_SAMPLE 6
extern u8 interfacemode;

// Event hooks
extern void Interface_Init();
extern void Interface_Tick();
extern void Interface_Background();

// Button hooks
extern void Interface_BtnGVoice(u8 gvoice, u8 state);
extern void Interface_BtnSoftkey(u8 softkey, u8 state);
extern void Interface_BtnSelOp(u8 op, u8 state);
extern void Interface_BtnOpMute(u8 op, u8 state);
extern void Interface_BtnSystem(u8 button, u8 state);
extern void Interface_BtnEdit(u8 button, u8 state);

// Encoder hooks
extern void Interface_EncDatawheel(s32 incrementer);
extern void Interface_EncEdit(u8 encoder, s32 incrementer);

#ifdef __cplusplus
}
#endif


#endif /* _INTERFACE_H */
