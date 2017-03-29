/*
 * MIDIbox Quad Genesis: Capturer dialog header
 *
 * ==========================================================================
 *
 *  Copyright (C) 2016 Sauraen (sauraen@gmail.com)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _CAPTURER_H
#define _CAPTURER_H

#ifdef __cplusplus
extern "C" {
#endif

extern void Capturer_Start(u8 origvoice, void (*callback)(u8 success));

extern void Capturer_BtnSystem(u8 button, u8 state);
extern void Capturer_BtnGVoice(u8 gvoice, u8 state);


#ifdef __cplusplus
}
#endif


#endif /* _CAPTURER_H */
