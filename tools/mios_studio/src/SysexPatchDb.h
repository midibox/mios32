/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
// $Id$
/*
 * SysEx Patch Database
 *
 * ==========================================================================
 *
 *  Copyright (C) 2012 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _SYSEX_PATCH_DB_H
#define _SYSEX_PATCH_DB_H

#include "includes.h"

#define SYSEX_PATCH_DB_TOKEN_CMD    0x80
#define SYSEX_PATCH_DB_TOKEN_TYPE   0x81
#define SYSEX_PATCH_DB_TOKEN_BANK   0x82
#define SYSEX_PATCH_DB_TOKEN_PATCH  0x83

class SysexPatchDb
{
public:
    //==============================================================================
    SysexPatchDb();
    ~SysexPatchDb();

    //==============================================================================
    typedef struct PatchSpecT {
        String   specName;

        unsigned numBanks;
        unsigned numPatchesPerBank;

        unsigned patchSize;

        unsigned delayBetweenReads;
        unsigned delayBetweenWrites;

        Array<uint8> patchHeader;

        bool deviceIdAfterHeader;
        bool deviceIdInCmd;

        Array<uint8> patchCmd;

        unsigned cmdPos;
        unsigned cmdPatchRead;
        unsigned cmdPatchWrite;
        unsigned cmdBufferRead;
        unsigned cmdBufferWrite;
        unsigned cmdErrorAcknowledge;
        unsigned cmdAcknowledge;

        unsigned typePos;
        unsigned typeOffsetPatch;
        unsigned typeOffsetBuffer;

        unsigned bankPos;
        unsigned patchPos;

        unsigned payloadBegin;
        unsigned payloadEnd;

        unsigned checksumBegin;
        unsigned checksumEnd;
        unsigned checksumPos;

        unsigned namePosInPayload;
        unsigned nameLength;
        bool nameInNibbles;
    } PatchSpecT;


    //==============================================================================

    //! returns the number of available specs
    unsigned getNumSpecs(void);

    //==============================================================================
    //! returns name of given patch spec
    String getSpecName(const unsigned& spec);

    //! number of banks
    unsigned getNumBanks(const unsigned& spec);

    //! number of patches per bank
    unsigned getNumPatchesPerBank(const unsigned& spec);

    //! the patch size
    unsigned getPatchSize(const unsigned& spec);

    //! delay between reads (in mS)
    unsigned getDelayBetweenReads(const unsigned& spec);

    //! delay between writes (in mS)
    unsigned getDelayBetweenWrites(const unsigned& spec);

    //! various check/create functions
    bool isValidHeader(const unsigned& spec, const uint8 *data, const uint32 &size, const int &deviceId);
    Array<uint8> createHeader(const unsigned& spec, const uint8& deviceId);

    bool isValidCmd(const unsigned& spec, const uint8 *data, const uint32 &size, const int &deviceId, const uint8& cmd, const int &patchType, const int &bank, const int &patch);
    Array<uint8> createCmd(const unsigned& spec, const uint8 &deviceId, const uint8 &cmd, const uint8 &patchType, const uint8 &bank, const uint8 &patch, const uint8 *data, const uint32& size);

    bool isValidReadPatch(const unsigned& spec, const uint8 *data, const uint32 &size, const int &deviceId, const int &patchType, const int &bank, const int &patch);
    Array<uint8> createReadPatch(const unsigned& spec, const uint8 &deviceId, const uint8 &patchType, const uint8 &bank, const uint8 &patch);

    bool isValidWritePatch(const unsigned& spec, const uint8 *data, const uint32 &size, const int &deviceId, const int &patchType, const int &bank, const int &patch);
    Array<uint8> createWritePatch(const unsigned& spec, const uint8 &deviceId, const uint8 &patchType, const uint8 &bank, const uint8 &patch, const uint8 *data, const uint32& size);

    bool isValidReadBuffer(const unsigned& spec, const uint8 *data, const uint32 &size, const int &deviceId, const int &patchType, const int &bank, const int &patch);
    Array<uint8> createReadBuffer(const unsigned& spec, const uint8 &deviceId, const uint8 &patchType, const uint8 &bank, const uint8 &patch);

    bool isValidWriteBuffer(const unsigned& spec, const uint8 *data, const uint32 &size, const int &deviceId, const int &patchType, const int &bank, const int &patch);
    Array<uint8> createWriteBuffer(const unsigned& spec, const uint8 &deviceId, const uint8 &patchType, const uint8 &bank, const uint8 &patch, const uint8 *data, const uint32& size);

    bool isValidErrorAcknowledge(const unsigned& spec, const uint8 *data, const uint32 &size, const int &deviceId);
    bool isValidAcknowledge(const unsigned& spec, const uint8 *data, const uint32 &size, const int &deviceId);

    bool hasValidChecksum(const unsigned& spec, const uint8 *data, const uint32 &size);

    Array<uint8> getPayload(const unsigned& spec, const uint8 *data, const uint32 &size);
    String getPatchNameFromPayload(const unsigned& spec, Array<uint8> payload);
    void replacePatchNameInPayload(const unsigned& spec, Array<uint8>& payload, String patchName);


protected:

    Array<PatchSpecT> patchSpec;
};

#endif /* _SYSEX_PATCH_DB_H */
