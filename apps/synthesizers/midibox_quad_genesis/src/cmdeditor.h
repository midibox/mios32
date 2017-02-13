/*
 * MIDIbox Quad Genesis: VGM Command Editor Header
 *
 * ==========================================================================
 *
 *  Copyright (C) 2016 Sauraen (sauraen@gmail.com)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _CMDEDITOR_H
#define _CMDEDITOR_H

#include <vgm.h>
#include "syeng.h"

extern void DrawCmdLine(VgmChipWriteCmd cmd, u8 row, u8 ctrlled);
extern void DrawCmdContent(VgmChipWriteCmd cmd, u8 clear);
extern void GetCmdDescription(VgmChipWriteCmd cmd, char* desc);

extern VgmChipWriteCmd EditCmd(VgmChipWriteCmd cmd, u8 encoder, s32 incrementer, u8 button, u8 state, u8 reqvoice, u8 reqop);

extern u8 EditState(VgmSource* vgs, VgmHead* head, u8 encoder, s32 incrementer, u8 button, u8 state, u8 voice, u8 op);

extern VgmSource* CreateNewVGM(u8 type, VgmUsageBits usage);

#endif /* _CMDEDITOR_H */
