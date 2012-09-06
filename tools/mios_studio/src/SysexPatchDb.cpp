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

#include "SysexPatchDb.h"

SysexPatchDb::SysexPatchDb()
{

    // this part is current quick&dirty hardcoded
    // definitions could be read from a XML file in future!

    {
        PatchSpecT ps; 
        ps.specName           = String(T("MIDIbox SID V2 Patch"));
        ps.numBanks           = 8;
        ps.numPatchesPerBank  = 128;
        ps.delayBetweenReads  = 1000;
        ps.delayBetweenWrites = 1000;

        ps.patchHeader.add(0xf0);
        ps.patchHeader.add(0x00);
        ps.patchHeader.add(0x00);
        ps.patchHeader.add(0x7e);
        ps.patchHeader.add(0x4b);

        ps.deviceIdAfterHeader = true;
        ps.deviceIdInCmd       = false;

        ps.patchCmd.add(SYSEX_PATCH_DB_TOKEN_CMD);
        ps.patchCmd.add(SYSEX_PATCH_DB_TOKEN_TYPE);
        ps.patchCmd.add(SYSEX_PATCH_DB_TOKEN_BANK);
        ps.patchCmd.add(SYSEX_PATCH_DB_TOKEN_PATCH);

        ps.cmdPos           = 6;
        ps.cmdPatchRead     = 0x01;
        ps.cmdPatchWrite    = 0x02;
        ps.cmdBufferRead    = 0x01;
        ps.cmdBufferWrite   = 0x02;
        ps.cmdErrorAcknowledge = 0x0e;
        ps.cmdAcknowledge   = 0x0f;

        ps.typePos          = 7;
        ps.typeOffsetPatch  = 0x00;
        ps.typeOffsetBuffer = 0x08;

        ps.bankPos          = 8;
        ps.patchPos         = 9;

        ps.bankBuffer       = 0x00;
        ps.patchBuffer      = 0x00;

        ps.payloadBegin     = 10;
        ps.payloadEnd       = 10 + 2*512-1;
        ps.checksumBegin    = ps.payloadBegin;
        ps.checksumEnd      = ps.payloadEnd;
        ps.checksumPos      = 10 + 2*512;
        ps.patchSize        = 10 + 2*512 + 1 + 1;
        ps.checksumType     = SYSEX_PATCH_DB_CHECKSUM_TWO_COMPLEMENT;

        ps.namePosInPayload = 0; // 0x10 in whole dump
        ps.nameLength = 16;
        ps.nameInNibbles = true;

        ps.numBuffers = 4;
        ps.bufferName = String(T("SID"));

        patchSpec.add(ps);
    }


    {
        PatchSpecT ps;
        ps.specName       = String(T("MIDIbox SID V2 Ensemble"));
        ps.numBanks          = 1;
        ps.numPatchesPerBank = 128;
        ps.delayBetweenReads  = 1000;
        ps.delayBetweenWrites = 1000;

        ps.patchHeader.add(0xf0);
        ps.patchHeader.add(0x00);
        ps.patchHeader.add(0x00);
        ps.patchHeader.add(0x7e);
        ps.patchHeader.add(0x4b);

        ps.deviceIdAfterHeader = true;
        ps.deviceIdInCmd       = false;

        ps.patchCmd.add(SYSEX_PATCH_DB_TOKEN_CMD);
        ps.patchCmd.add(SYSEX_PATCH_DB_TOKEN_TYPE);
        ps.patchCmd.add(SYSEX_PATCH_DB_TOKEN_BANK);
        ps.patchCmd.add(SYSEX_PATCH_DB_TOKEN_PATCH);

        ps.cmdPos           = 6;
        ps.cmdPatchRead     = 0x01;
        ps.cmdPatchWrite    = 0x02;
        ps.cmdBufferRead    = 0x01;
        ps.cmdBufferWrite   = 0x02;
        ps.cmdErrorAcknowledge = 0x0e;
        ps.cmdAcknowledge   = 0x0f;

        ps.typePos          = 7;
        ps.typeOffsetPatch  = 0x70;
        ps.typeOffsetBuffer = 0x78;

        ps.bankPos          = 8;
        ps.patchPos         = 9;

        ps.bankBuffer       = 0x00;
        ps.patchBuffer      = 0x00;

        ps.payloadBegin     = 10;
        ps.payloadEnd       = 10 + 512-1;
        ps.checksumBegin    = ps.payloadBegin;
        ps.checksumEnd      = ps.payloadEnd;
        ps.checksumPos      = 10 + 512;
        ps.patchSize        = 10 + 512 + 1 + 1;
        ps.checksumType     = SYSEX_PATCH_DB_CHECKSUM_TWO_COMPLEMENT;

        ps.namePosInPayload = 0;
        ps.nameLength = 0; // none
        ps.nameInNibbles = true;

        ps.numBuffers = 1;
        ps.bufferName = String(T("Ensemble"));

        patchSpec.add(ps);
    }

    {
        PatchSpecT ps;
        ps.specName       = String(T("MIDIbox FM V1 Voice"));
        ps.numBanks          = 8;
        ps.numPatchesPerBank = 128;
        ps.delayBetweenReads  = 1000;
        ps.delayBetweenWrites = 1000;

        ps.patchHeader.add(0xf0);
        ps.patchHeader.add(0x00);
        ps.patchHeader.add(0x00);
        ps.patchHeader.add(0x7e);
        ps.patchHeader.add(0x49);

        ps.deviceIdAfterHeader = true;
        ps.deviceIdInCmd       = false;

        ps.patchCmd.add(SYSEX_PATCH_DB_TOKEN_CMD);
        ps.patchCmd.add(SYSEX_PATCH_DB_TOKEN_TYPE);
        ps.patchCmd.add(SYSEX_PATCH_DB_TOKEN_BANK);
        ps.patchCmd.add(SYSEX_PATCH_DB_TOKEN_PATCH);

        ps.cmdPos           = 6;
        ps.cmdPatchRead     = 0x01;
        ps.cmdPatchWrite    = 0x02;
        ps.cmdBufferRead    = 0x01;
        ps.cmdBufferWrite   = 0x02;
        ps.cmdErrorAcknowledge = 0x0e;
        ps.cmdAcknowledge   = 0x0f;

        ps.typePos          = 7;
        ps.typeOffsetPatch  = 0x00;
        ps.typeOffsetBuffer = 0x08;

        ps.bankPos          = 8;
        ps.patchPos         = 9;

        ps.bankBuffer       = 0x00;
        ps.patchBuffer      = 0x00;

        ps.payloadBegin     = 10;
        ps.payloadEnd       = 10 + 256-1;
        ps.checksumBegin    = ps.payloadBegin;
        ps.checksumEnd      = ps.payloadEnd;
        ps.checksumPos      = 10 + 256;
        ps.patchSize        = 10 + 256 + 1 + 1;
        ps.checksumType     = SYSEX_PATCH_DB_CHECKSUM_TWO_COMPLEMENT;

        ps.namePosInPayload = 0; // 0x10 in whole dump
        ps.nameLength = 16;
        ps.nameInNibbles = false;

        ps.numBuffers = 4;
        ps.bufferName = String(T("Instrument"));

        patchSpec.add(ps);
    }

    {
        PatchSpecT ps;
        ps.specName       = String(T("MIDIbox FM V1 Drumset"));
        ps.numBanks          = 8;
        ps.numPatchesPerBank = 16;
        ps.delayBetweenReads  = 1000;
        ps.delayBetweenWrites = 1000;

        ps.patchHeader.add(0xf0);
        ps.patchHeader.add(0x00);
        ps.patchHeader.add(0x00);
        ps.patchHeader.add(0x7e);
        ps.patchHeader.add(0x49);

        ps.deviceIdAfterHeader = true;
        ps.deviceIdInCmd       = false;

        ps.patchCmd.add(SYSEX_PATCH_DB_TOKEN_CMD);
        ps.patchCmd.add(SYSEX_PATCH_DB_TOKEN_TYPE);
        ps.patchCmd.add(SYSEX_PATCH_DB_TOKEN_BANK);
        ps.patchCmd.add(SYSEX_PATCH_DB_TOKEN_PATCH);

        ps.cmdPos           = 6;
        ps.cmdPatchRead     = 0x01;
        ps.cmdPatchWrite    = 0x02;
        ps.cmdBufferRead    = 0x01;
        ps.cmdBufferWrite   = 0x02;
        ps.cmdErrorAcknowledge = 0x0e;
        ps.cmdAcknowledge   = 0x0f;

        ps.typePos          = 7;
        ps.typeOffsetPatch  = 0x10;
        ps.typeOffsetBuffer = 0x18;

        ps.bankPos          = 8;
        ps.patchPos         = 9;

        ps.bankBuffer       = 0x00;
        ps.patchBuffer      = 0x00;

        ps.payloadBegin     = 10;
        ps.payloadEnd       = 10 + 256-1;
        ps.checksumBegin    = ps.payloadBegin;
        ps.checksumEnd      = ps.payloadEnd;
        ps.checksumPos      = 10 + 256;
        ps.patchSize        = 10 + 256 + 1 + 1;
        ps.checksumType     = SYSEX_PATCH_DB_CHECKSUM_TWO_COMPLEMENT;

        ps.namePosInPayload = 0;
        ps.nameLength = 0; // no name
        ps.nameInNibbles = false;

        ps.numBuffers = 1;
        ps.bufferName = String(T("Drumset"));

        patchSpec.add(ps);
    }

    {
        PatchSpecT ps;
        ps.specName       = String(T("MIDIbox FM V1 Ensemble"));
        ps.numBanks          = 1;
        ps.numPatchesPerBank = 128;
        ps.delayBetweenReads  = 1000;
        ps.delayBetweenWrites = 1000;

        ps.patchHeader.add(0xf0);
        ps.patchHeader.add(0x00);
        ps.patchHeader.add(0x00);
        ps.patchHeader.add(0x7e);
        ps.patchHeader.add(0x49);

        ps.deviceIdAfterHeader = true;
        ps.deviceIdInCmd       = false;

        ps.patchCmd.add(SYSEX_PATCH_DB_TOKEN_CMD);
        ps.patchCmd.add(SYSEX_PATCH_DB_TOKEN_TYPE);
        ps.patchCmd.add(SYSEX_PATCH_DB_TOKEN_BANK);
        ps.patchCmd.add(SYSEX_PATCH_DB_TOKEN_PATCH);

        ps.cmdPos           = 6;
        ps.cmdPatchRead     = 0x01;
        ps.cmdPatchWrite    = 0x02;
        ps.cmdBufferRead    = 0x01;
        ps.cmdBufferWrite   = 0x02;
        ps.cmdErrorAcknowledge = 0x0e;
        ps.cmdAcknowledge   = 0x0f;

        ps.typePos          = 7;
        ps.typeOffsetPatch  = 0x70;
        ps.typeOffsetBuffer = 0x78;

        ps.bankPos          = 8;
        ps.patchPos         = 9;

        ps.bankBuffer       = 0x00;
        ps.patchBuffer      = 0x00;

        ps.payloadBegin     = 10;
        ps.payloadEnd       = 10 + 256-1;
        ps.checksumBegin    = ps.payloadBegin;
        ps.checksumEnd      = ps.payloadEnd;
        ps.checksumPos      = 10 + 256;
        ps.patchSize        = 10 + 256 + 1 + 1;
        ps.checksumType     = SYSEX_PATCH_DB_CHECKSUM_TWO_COMPLEMENT;

        ps.namePosInPayload = 0;
        ps.nameLength = 0; // no name
        ps.nameInNibbles = false;

        ps.numBuffers = 1;
        ps.bufferName = String(T("Ensemble"));

        patchSpec.add(ps);
    }

    {
        PatchSpecT ps;
        ps.specName       = String(T("MIDIbox 808/Dr Pattern"));
        ps.numBanks          = 7;
        ps.numPatchesPerBank = 128;
        ps.delayBetweenReads  = 1000;
        ps.delayBetweenWrites = 1000;

        ps.patchHeader.add(0xf0);
        ps.patchHeader.add(0x00);
        ps.patchHeader.add(0x00);
        ps.patchHeader.add(0x7e);
        ps.patchHeader.add(0x4c);

        ps.deviceIdAfterHeader = true;
        ps.deviceIdInCmd       = false;

        ps.patchCmd.add(SYSEX_PATCH_DB_TOKEN_CMD);
        ps.patchCmd.add(SYSEX_PATCH_DB_TOKEN_BANK);
        ps.patchCmd.add(SYSEX_PATCH_DB_TOKEN_PATCH);

        ps.cmdPos           = 6;
        ps.cmdPatchRead     = 0x01;
        ps.cmdPatchWrite    = 0x02;
        ps.cmdBufferRead    = 0x01;
        ps.cmdBufferWrite   = 0x02;
        ps.cmdErrorAcknowledge = 0x0e;
        ps.cmdAcknowledge   = 0x0f;

        ps.typePos          = 0;
        ps.typeOffsetPatch  = 0;
        ps.typeOffsetBuffer = 0;

        ps.bankPos          = 7;
        ps.patchPos         = 8;

        ps.bankBuffer       = 0x00;
        ps.patchBuffer      = 0x00;

        ps.payloadBegin     = 9;
        ps.payloadEnd       = 9 + 1024-1;
        ps.checksumBegin    = ps.payloadBegin;
        ps.checksumEnd      = ps.payloadEnd;
        ps.checksumPos      = 9 + 1024;
        ps.patchSize        = 9 + 1024 + 1 + 1;
        ps.checksumType     = SYSEX_PATCH_DB_CHECKSUM_TWO_COMPLEMENT;

        ps.namePosInPayload = 0;
        ps.nameLength = 0; // no name
        ps.nameInNibbles = false;

        ps.numBuffers = 0;
        ps.bufferName = String::empty;

        patchSpec.add(ps);
    }

    {
        PatchSpecT ps;
        ps.specName       = String(T("Waldorf Blofeld Patch"));
        ps.numBanks          = 8;
        ps.numPatchesPerBank = 128;
        ps.delayBetweenReads  = 1000;
        ps.delayBetweenWrites = 1000;

        ps.patchHeader.add(0xf0);
        ps.patchHeader.add(0x3e);
        ps.patchHeader.add(0x13);

        ps.deviceIdAfterHeader = true;
        ps.deviceIdInCmd       = false;

        ps.patchCmd.add(SYSEX_PATCH_DB_TOKEN_CMD);
        ps.patchCmd.add(SYSEX_PATCH_DB_TOKEN_BANK);
        ps.patchCmd.add(SYSEX_PATCH_DB_TOKEN_PATCH);

        ps.cmdPos           = 4;
        ps.cmdPatchRead     = 0x00;
        ps.cmdPatchWrite    = 0x10;
        ps.cmdBufferRead    = 0x00;
        ps.cmdBufferWrite   = 0x10;
        ps.cmdErrorAcknowledge = 0x00; // actually not available
        ps.cmdAcknowledge   = 0x00; // actually not available

        ps.typePos          = 0;
        ps.typeOffsetPatch  = 0x00;
        ps.typeOffsetBuffer = 0x00;

        ps.bankPos          = 5;
        ps.patchPos         = 6;

        ps.bankBuffer       = 0x7f;
        ps.patchBuffer      = 0x00;

        ps.payloadBegin     = 7;
        ps.payloadEnd       = 7 + 383-1;
        ps.checksumBegin    = ps.payloadBegin;
        ps.checksumEnd      = ps.payloadEnd;
        ps.checksumPos      = 7 + 383;
        ps.patchSize        = 7 + 383 + 1 + 1;
        ps.checksumType     = SYSEX_PATCH_DB_CHECKSUM_ACCUMULATED;

        ps.namePosInPayload = 363;
        ps.nameLength = 16;
        ps.nameInNibbles = false;

        ps.numBuffers = 1;
        ps.bufferName = String(T("Buffer"));

        patchSpec.add(ps);
    }
}

SysexPatchDb::~SysexPatchDb()
{
}

unsigned SysexPatchDb::getNumSpecs(void)
{
    return patchSpec.size();
}

String SysexPatchDb::getSpecName(const unsigned& spec)
{
    return patchSpec[spec].specName;
}

unsigned SysexPatchDb::getNumBanks(const unsigned& spec)
{
    return patchSpec[spec].numBanks;
}

unsigned SysexPatchDb::getNumPatchesPerBank(const unsigned& spec)
{
    return patchSpec[spec].numPatchesPerBank;
}

unsigned SysexPatchDb::getDelayBetweenReads(const unsigned& spec)
{
    return patchSpec[spec].delayBetweenReads;
}

unsigned SysexPatchDb::getDelayBetweenWrites(const unsigned& spec)
{
    return patchSpec[spec].delayBetweenWrites;
}

unsigned SysexPatchDb::getPatchSize(const unsigned& spec)
{
    return patchSpec[spec].patchSize;
}

unsigned SysexPatchDb::getNumBuffers(const unsigned& spec)
{
    return patchSpec[spec].numBuffers;
}

String SysexPatchDb::getBufferName(const unsigned& spec)
{
    return patchSpec[spec].bufferName;
}


bool SysexPatchDb::isValidHeader(const unsigned& spec, const uint8 *data, const uint32 &size, const int &deviceId)
{
    PatchSpecT *ps = (PatchSpecT *)&patchSpec.getReference(spec); // we assume that the user has checked the 'spec' index!
    const unsigned minSize = ps->patchHeader.size() + (ps->deviceIdAfterHeader ? 1 : 0);

    if( size < minSize )
        return false;

    for(int i=0; i<ps->patchHeader.size(); ++i)
        if( data[i] != ps->patchHeader[i] )
            return false;

    // deviceId == -1 allows to ignore the ID value
    if( ps->deviceIdAfterHeader && deviceId >= 0 && data[ps->patchHeader.size()] != deviceId )
        return false;

    // special coding into Syx command as used by older MIDIbox firmwares
    if( ps->deviceIdInCmd && deviceId >= 0 && ((data[ps->cmdPos] >> 4) != deviceId) )
        return false;

    return true;
}

Array<uint8> SysexPatchDb::createHeader(const unsigned& spec, const uint8& deviceId)
{
    PatchSpecT *ps = (PatchSpecT *)&patchSpec.getReference(spec); // we assume that the user has checked the 'spec' index!

    Array<uint8> dataArray = ps->patchHeader;
    if( ps->deviceIdAfterHeader )
        dataArray.add(deviceId);

    return dataArray;
}

bool SysexPatchDb::isValidCmd(const unsigned& spec, const uint8 *data, const uint32 &size, const int &deviceId, const uint8& cmd, const int &patchType, const int &bank, const int &patch)
{
    PatchSpecT *ps = (PatchSpecT *)&patchSpec.getReference(spec); // we assume that the user has checked the 'spec' index!

    if( !isValidHeader(spec, data, size, deviceId) )
        return false;

    if( size < ps->cmdPos )
        return false;

    if( ps->deviceIdInCmd ) {
        if( (data[ps->cmdPos] & 0x0f) != cmd )
            return false;
    } else {
        if( data[ps->cmdPos] != cmd )
            return false;
    }

    if( patchType >= 0 && ps->typePos > 0 && size >= ps->typePos && data[ps->typePos] != patchType )
        return false;

    if( bank >= 0 && ps->bankPos > 0 && size >= ps->bankPos && data[ps->bankPos] != bank )
        return false;

    if( patch >= 0 && ps->patchPos > 0 && size >= ps->patchPos && data[ps->patchPos] != patch )
        return false;

    return true;
}


Array<uint8> SysexPatchDb::createCmd(const unsigned& spec, const uint8 &deviceId, const uint8 &cmd, const uint8 &patchType, const uint8 &bank, const uint8 &patch, const uint8 *data, const uint32& size)
{
    PatchSpecT *ps = (PatchSpecT *)&patchSpec.getReference(spec); // we assume that the user has checked the 'spec' index!
    Array<uint8> dataArray = createHeader(spec, deviceId);

    for(int i=0; i<ps->patchCmd.size(); ++i) {
        uint8 b = ps->patchCmd[i];

        switch( b ) {
        case SYSEX_PATCH_DB_TOKEN_CMD:
            if( ps->deviceIdInCmd ) {
                dataArray.add((cmd & 0x0f) | ((deviceId << 4) & 0x70));
            } else {
                dataArray.add(cmd);
            }
            break;
        case SYSEX_PATCH_DB_TOKEN_TYPE:
            dataArray.add(patchType);
            break;
        case SYSEX_PATCH_DB_TOKEN_BANK:
            dataArray.add(bank);
            break;
        case SYSEX_PATCH_DB_TOKEN_PATCH:
            dataArray.add(patch);
            break;

        default:
            if( b < 0x80 )
                dataArray.add(patch);
        }
    }

    if( data != NULL && size ) {
        dataArray.addArray(data, size);

        if( ps->checksumPos ) {
            uint8 checksum = 0x00;
            for(int pos=ps->payloadBegin; pos<=ps->payloadEnd; ++pos) {
                checksum += dataArray[pos];
            }

            switch( ps->checksumType ) {
            case SYSEX_PATCH_DB_CHECKSUM_TWO_COMPLEMENT:
                dataArray.add(-(int)checksum & 0x7f);
                break;
            default: // SYSEX_PATCH_DB_CHECKSUM_ACCUMULATED:
                dataArray.add(checksum & 0x7f);
            }
        }
    }

    dataArray.add(0xf7);

    return dataArray;
}


bool SysexPatchDb::isValidReadPatch(const unsigned& spec, const uint8 *data, const uint32 &size, const int &deviceId, const int &patchType, const int &bank, const int &patch)
{
    PatchSpecT *ps = (PatchSpecT *)&patchSpec.getReference(spec); // we assume that the user has checked the 'spec' index!
    return isValidCmd(spec, data, size, deviceId, ps->cmdPatchRead, ((patchType >= 0) ? patchType : 0) + ps->typeOffsetPatch, bank, patch);
}

Array<uint8> SysexPatchDb::createReadPatch(const unsigned& spec, const uint8 &deviceId, const uint8 &patchType, const uint8 &bank, const uint8 &patch)
{
    PatchSpecT *ps = (PatchSpecT *)&patchSpec.getReference(spec); // we assume that the user has checked the 'spec' index!
    return createCmd(spec, deviceId, ps->cmdPatchRead, patchType + ps->typeOffsetPatch, bank, patch, NULL, 0);
}

bool SysexPatchDb::isValidWritePatch(const unsigned& spec, const uint8 *data, const uint32 &size, const int &deviceId, const int &patchType, const int &bank, const int &patch)
{
    PatchSpecT *ps = (PatchSpecT *)&patchSpec.getReference(spec); // we assume that the user has checked the 'spec' index!
    return isValidCmd(spec, data, size, deviceId, ps->cmdPatchWrite, ((patchType >= 0) ? patchType : 0) + ps->typeOffsetPatch, bank, patch);
}

Array<uint8> SysexPatchDb::createWritePatch(const unsigned& spec, const uint8 &deviceId, const uint8 &patchType, const uint8 &bank, const uint8 &patch, const uint8 *data, const uint32& size)
{
    PatchSpecT *ps = (PatchSpecT *)&patchSpec.getReference(spec); // we assume that the user has checked the 'spec' index!
    return createCmd(spec, deviceId, ps->cmdPatchWrite, patchType + ps->typeOffsetPatch, bank, patch, data, size);
}


bool SysexPatchDb::isValidReadBuffer(const unsigned& spec, const uint8 *data, const uint32 &size, const int &deviceId, const int &patchType, const int &bank, const int &patch)
{
    // note: bank and patch are currently ignored - we take bankBuffer and patchBuffer instead. Could be made optional in future
    PatchSpecT *ps = (PatchSpecT *)&patchSpec.getReference(spec); // we assume that the user has checked the 'spec' index!
    return isValidCmd(spec, data, size, deviceId, ps->cmdBufferRead, ((patchType >= 0) ? patchType : 0) + ps->typeOffsetBuffer, ps->bankBuffer, ps->patchBuffer);
}

Array<uint8> SysexPatchDb::createReadBuffer(const unsigned& spec, const uint8 &deviceId, const uint8 &patchType, const uint8 &bank, const uint8 &patch)
{
    // note: bank and patch are currently ignored - we take bankBuffer and patchBuffer instead. Could be made optional in future
    PatchSpecT *ps = (PatchSpecT *)&patchSpec.getReference(spec); // we assume that the user has checked the 'spec' index!
    return createCmd(spec, deviceId, ps->cmdBufferRead, patchType + ps->typeOffsetBuffer, ps->bankBuffer, ps->patchBuffer, NULL, 0);
}

bool SysexPatchDb::isValidWriteBuffer(const unsigned& spec, const uint8 *data, const uint32 &size, const int &deviceId, const int &patchType, const int &bank, const int &patch)
{
    // note: bank and patch are currently ignored - we take bankBuffer and patchBuffer instead. Could be made optional in future
    PatchSpecT *ps = (PatchSpecT *)&patchSpec.getReference(spec); // we assume that the user has checked the 'spec' index!
    return isValidCmd(spec, data, size, deviceId, ps->cmdBufferWrite, ((patchType >= 0) ? patchType : 0) + ps->typeOffsetBuffer, ps->bankBuffer, ps->patchBuffer);
}

Array<uint8> SysexPatchDb::createWriteBuffer(const unsigned& spec, const uint8 &deviceId, const uint8 &patchType, const uint8 &bank, const uint8 &patch, const uint8 *data, const uint32& size)
{
    // note: bank and patch are currently ignored - we take bankBuffer and patchBuffer instead. Could be made optional in future
    PatchSpecT *ps = (PatchSpecT *)&patchSpec.getReference(spec); // we assume that the user has checked the 'spec' index!
    return createCmd(spec, deviceId, ps->cmdBufferWrite, patchType + ps->typeOffsetBuffer, ps->bankBuffer, ps->patchBuffer, data, size);
}

bool SysexPatchDb::isValidErrorAcknowledge(const unsigned& spec, const uint8 *data, const uint32 &size, const int &deviceId)
{
    PatchSpecT *ps = (PatchSpecT *)&patchSpec.getReference(spec); // we assume that the user has checked the 'spec' index!
    return isValidCmd(spec, data, size, deviceId, ps->cmdErrorAcknowledge, -1, -1, -1);
}

bool SysexPatchDb::isValidAcknowledge(const unsigned& spec, const uint8 *data, const uint32 &size, const int &deviceId)
{
    PatchSpecT *ps = (PatchSpecT *)&patchSpec.getReference(spec); // we assume that the user has checked the 'spec' index!
    return isValidCmd(spec, data, size, deviceId, ps->cmdAcknowledge, -1, -1, -1);
}

bool SysexPatchDb::hasValidChecksum(const unsigned& spec, const uint8 *data, const uint32 &size)
{
    PatchSpecT *ps = (PatchSpecT *)&patchSpec.getReference(spec); // we assume that the user has checked the 'spec' index!

    if( ps->checksumPos ) {
        uint8 checksum = 0x00;

        if( size < ps->patchSize )
            return false;

        int pos;
        for(pos=ps->checksumBegin; pos<=ps->checksumEnd; ++pos)
            checksum += data[pos];

        switch( ps->checksumType ) {
        case SYSEX_PATCH_DB_CHECKSUM_TWO_COMPLEMENT:
            if( data[pos] != (-(int)checksum & 0x7f) )
                return false;
        default: // SYSEX_PATCH_DB_CHECKSUM_ACCUMULATED:
            if( data[pos] != (checksum & 0x7f) )
                return false;
        }
    }

    return true;
}


Array<uint8> SysexPatchDb::getPayload(const unsigned& spec, const uint8 *data, const uint32 &size)
{
    PatchSpecT *ps = (PatchSpecT *)&patchSpec.getReference(spec); // we assume that the user has checked the 'spec' index!
    if( size > ps->payloadEnd ) {
        int payloadSize = ps->payloadEnd - ps->payloadBegin + 1;
        if( payloadSize > 0 ) {
            Array<uint8> dataArray((uint8 *)&data[ps->payloadBegin], payloadSize);
            return dataArray;
        }
    }

    Array<uint8> emptyArray;
    return emptyArray;
}

String SysexPatchDb::getPatchNameFromPayload(const unsigned& spec, Array<uint8> payload)
{
    PatchSpecT *ps = (PatchSpecT *)&patchSpec.getReference(spec); // we assume that the user has checked the 'spec' index!

    if( ps->nameLength ) {
        String tmp;
        uint8 *name = (uint8 *)&payload.getReference(ps->namePosInPayload);
        for(int i=0; i<ps->nameLength; ++i) {
            if( ps->nameInNibbles ) {
                tmp += (char)((name[2*i+0] & 0xf) | ((name[2*i+1] & 0xf) << 4));
            } else {
                tmp += (char)name[i];
            }
        }

        return tmp;
    }

    return String(T("no name available"));
}

void SysexPatchDb::replacePatchNameInPayload(const unsigned& spec, Array<uint8>& payload, String patchName)
{
    PatchSpecT *ps = (PatchSpecT *)&patchSpec.getReference(spec); // we assume that the user has checked the 'spec' index!

    if( ps->nameLength ) {
        uint8 *name = (uint8 *)&payload.getReference(ps->namePosInPayload);
        for(int i=0; i<ps->nameLength; ++i) {
            char c = (i < patchName.length()) ? patchName.toUTF8()[i] : 0;
            if( ps->nameInNibbles ) {
                name[2*i+0] = c & 0x0f;
                name[2*i+1] = (c >> 4) & 0x0f;
            } else {
                name[i] = c & 0x7f;
            }
        }
    }
}

