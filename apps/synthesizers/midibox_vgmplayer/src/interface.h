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

/*
extern u8 DEBUG_Ring;
extern u8 DEBUG_RingState;
extern s8 DEBUG_RingDir;
*/

extern void Interface_Init();


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


#endif /* _INTERFACE_H */
