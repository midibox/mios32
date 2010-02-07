/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
// $Id$
/*
 * Hex File Loader
 *
 * ==========================================================================
 *
 *  Copyright (C) 2010 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _HEX_FILE_LOADER_H
#define _HEX_FILE_LOADER_H

#include "includes.h"
#include <map>
#include <vector>
#include "SysexHelper.h"


class HexFileLoader
{
public:
    //==============================================================================
    HexFileLoader(void);
    ~HexFileLoader();

    //==============================================================================
    bool loadFile(const File &inFile, String &statusMessage);

    MidiMessage createMidiMessageForBlock(const uint8 &deviceId, const uint32 &blockAddress, bool forMios32);

    std::vector<uint32> hexDumpAddressBlocks;

protected:
    std::map<uint32, Array<uint8> > hexDump;
};

#endif /* __HEX_FILE_LOADER_H */
