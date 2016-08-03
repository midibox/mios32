/*
 * MIDIbox Quad Genesis: Genesis State Drawing Functions
 *
 * ==========================================================================
 *
 *  Copyright (C) 2016 Sauraen (sauraen@gmail.com)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _GENESISSTATE_H
#define _GENESISSTATE_H

#ifdef __cplusplus
extern "C" {
#endif

extern void GenesisState_Tick();

extern void DrawGenesisActivity(u8 g);

extern void DrawGenesisState_Op(u8 g, u8 chan, u8 op);
extern void DrawGenesisState_Chan(u8 g, u8 chan);
extern void DrawGenesisState_DAC(u8 g);
extern void DrawGenesisState_OPN2(u8 g);
extern void DrawGenesisState_PSG(u8 g, u8 voice);
extern void DrawGenesisState_All(u8 g, u8 voice, u8 op);

extern void DrawOpVUMeter(u8 g, u8 chan, u8 op);
extern void DrawChanVUMeter(u8 g, u8 chan);
extern void DrawDACVUMeter(u8 g);

extern void ClearGenesisState_Op();
extern void ClearGenesisState_Chan();
extern void ClearGenesisState_DAC();
extern void ClearGenesisState_OPN2();
extern void ClearGenesisState_PSG();

#ifdef __cplusplus
}
#endif


#endif /* _GENESISSTATE_H */
