/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
// $Id$
/*
 * SysEx help routines
 *
 * ==========================================================================
 *
 *  Copyright (C) 2010 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#include "SysexHelper.h"


//==============================================================================
SysexHelper::SysexHelper()
{
}

SysexHelper::~SysexHelper()
{
}


//==============================================================================
bool SysexHelper::isValidMios8Header(const uint8 *data, const uint32 &size, const int &deviceId)
{
    return
        size >= 8 && // the shortest valid header is F0 00 00 7E 40 <deviceId> xx F7
        data[0] == 0xf0 &&
        data[1] == 0x00 &&
        data[2] == 0x00 &&
        data[3] == 0x7e &&
        data[4] == 0x40 &&
        (deviceId < 0 || data[5] == deviceId);
}

Array<uint8> SysexHelper::createMios8Header(const uint8 &deviceId)
{
    Array<uint8> dataArray;
    dataArray.add(0xf0);
    dataArray.add(0x00);
    dataArray.add(0x00);
    dataArray.add(0x7e);
    dataArray.add(0x40);
    dataArray.add(deviceId);
    return dataArray;
}

bool SysexHelper::isValidMios32Header(const uint8 *data, const uint32 &size, const int &deviceId)
{
    return
        size >= 8 && // the shortest valid header is F0 00 00 7E 32 <deviceId> xx F7
        data[0] == 0xf0 &&
        data[1] == 0x00 &&
        data[2] == 0x00 &&
        data[3] == 0x7e &&
        data[4] == 0x32 &&
        (deviceId < 0 || data[5] == deviceId);
}

Array<uint8> SysexHelper::createMios32Header(const uint8 &deviceId)
{
    Array<uint8> dataArray;
    dataArray.add(0xf0);
    dataArray.add(0x00);
    dataArray.add(0x00);
    dataArray.add(0x7e);
    dataArray.add(0x32);
    dataArray.add(deviceId);
    return dataArray;
}


//==============================================================================
bool SysexHelper::isValidMios8Acknowledge(const uint8 *data, const uint32 &size, const int &deviceId)
{
    return isValidMios8Header(data, size, deviceId) && data[6] == 0x0f;
}

Array<uint8> SysexHelper::createMios8Acknowledge(const uint8 &deviceId)
{
    Array<uint8> dataArray = createMios8Header(deviceId);
    dataArray.add(0x0f);
    return dataArray;
}

bool SysexHelper::isValidMios32Acknowledge(const uint8 *data, const uint32 &size, const int &deviceId)
{
    return isValidMios32Header(data, size, deviceId) && data[6] == 0x0f;
}

Array<uint8> SysexHelper::createMios32Acknowledge(const uint8 &deviceId)
{
    Array<uint8> dataArray = createMios32Header(deviceId);
    dataArray.add(0x0f);
    return dataArray;
}


//==============================================================================
bool SysexHelper::isValidMios8Error(const uint8 *data, const uint32 &size, const int &deviceId)
{
    return isValidMios8Header(data, size, deviceId) && data[6] == 0x0e;
}

Array<uint8> SysexHelper::createMios8Error(const uint8 &deviceId)
{
    Array<uint8> dataArray = createMios8Header(deviceId);
    dataArray.add(0x0e);
    return dataArray;
}

bool SysexHelper::isValidMios32Error(const uint8 *data, const uint32 &size, const int &deviceId)
{
    return isValidMios32Header(data, size, deviceId) && data[6] == 0x0e;
}

Array<uint8> SysexHelper::createMios32Error(const uint8 &deviceId)
{
    Array<uint8> dataArray = createMios32Header(deviceId);
    dataArray.add(0x0e);
    return dataArray;
}

//==============================================================================
bool SysexHelper::isValidMios8DebugMessage(const uint8 *data, const uint32 &size, const int &deviceId)
{
    return isValidMios8Header(data, size, deviceId) && data[6] == 0x0d;
}

Array<uint8> SysexHelper::createMios8DebugMessage(const uint8 &deviceId)
{
    Array<uint8> dataArray = createMios8Header(deviceId);
    dataArray.add(0x0d);
    return dataArray;
}

bool SysexHelper::isValidMios32DebugMessage(const uint8 *data, const uint32 &size, const int &deviceId)
{
    return isValidMios32Header(data, size, deviceId) && data[6] == 0x0d;
}

Array<uint8> SysexHelper::createMios32DebugMessage(const uint8 &deviceId)
{
    Array<uint8> dataArray = createMios32Header(deviceId);
    dataArray.add(0x0d);
    return dataArray;
}


//==============================================================================
bool SysexHelper::isValidMios8WriteBlock(const uint8 *data, const uint32 &size, const int &deviceId)
{
    return isValidMios8Header(data, size, deviceId) && data[6] == 0x02;
}

Array<uint8> SysexHelper::createMios8WriteBlock(const uint8 &deviceId, const uint32 &address, const uint8 &extension, const uint32 &size, uint8 &checksum)
{
    Array<uint8> dataArray = createMios8Header(deviceId);
    checksum = 0x00;

    uint8 cmd = 0x02 | ((extension << 4) & 0x70);
    uint8 b;
    dataArray.add(cmd);
    dataArray.add(b = (address >> 10) & 0x7f); checksum += b;
    dataArray.add(b = (address >> 3) & 0x7f); checksum += b;
    dataArray.add(b = (size >> 10) & 0x7f); checksum += b;
    dataArray.add(b = (size >> 3) & 0x7f); checksum += b;

    return dataArray;
}

bool SysexHelper::isValidMios32WriteBlock(const uint8 *data, const uint32 &size, const int &deviceId)
{
    return isValidMios32Header(data, size, deviceId) && data[6] == 0x02;
}

Array<uint8> SysexHelper::createMios32WriteBlock(const uint8 &deviceId, const uint32 &address, const uint32 &size, uint8 &checksum)
{
    Array<uint8> dataArray = createMios32Header(deviceId);
    checksum = 0x00;
    uint8 b;

    dataArray.add(0x02);
    dataArray.add(b = (address >> 25) & 0x7f); checksum += b;
    dataArray.add(b = (address >> 18) & 0x7f); checksum += b;
    dataArray.add(b = (address >> 11) & 0x7f); checksum += b;
    dataArray.add(b = (address >> 4) & 0x7f); checksum += b;
    dataArray.add(b = (size >> 25) & 0x7f); checksum += b;
    dataArray.add(b = (size >> 18) & 0x7f); checksum += b;
    dataArray.add(b = (size >> 11) & 0x7f); checksum += b;
    dataArray.add(b = (size >> 4) & 0x7f); checksum += b;

    return dataArray;
}


//==============================================================================
bool SysexHelper::isValidMios8UploadRequest(const uint8 *data, const uint32 &size, const int &deviceId)
{
    return isValidMios8Header(data, size, deviceId) && data[6] == 0x01 && data[7] == 0xf7;
}

bool SysexHelper::isValidMios32UploadRequest(const uint8 *data, const uint32 &size, const int &deviceId)
{
    return isValidMios32Header(data, size, deviceId) && data[6] == 0x01 && data[7] == 0xf7;
}


//==============================================================================
bool SysexHelper::isValidMios32Query(const uint8 *data, const uint32 &size, const int &deviceId)
{
    return isValidMios32Header(data, size, deviceId) && data[6] == 0x00;
}

Array<uint8> SysexHelper::createMios32Query(const uint8 &deviceId)
{
    Array<uint8> dataArray = createMios32Header(deviceId);
    dataArray.add(0x00);
    return dataArray;
}


//==============================================================================
MidiMessage SysexHelper::createMidiMessage(Array<uint8> &dataArray)
{
    uint8 *data = &dataArray.getReference(0);
    return MidiMessage(data, dataArray.size());
}


//==============================================================================
MidiMessage SysexHelper::createMidiMessage(Array<uint8> &dataArray, const int &lastPos)
{
    uint8 *data = &dataArray.getReference(0);
    return MidiMessage(data, (lastPos < dataArray.size()) ? lastPos : dataArray.size());
}


//==============================================================================
String SysexHelper::decodeMiosErrorCode(uint8 errorCode)
{
    switch( errorCode ) {
    case 0x01: return "Less bytes than expected have been received";
    case 0x02: return "More bytes than expected have been received";
    case 0x03: return "Checksum mismatch";
    case 0x04: return "Write failed (verify error or invalid address)";
    case 0x05: return "Write access failed (invalid address range)";
    case 0x06: return "MIDI Time Out";
    case 0x07: return "Wrong Debug Command";
    case 0x08: return "Read/Write command tried to access an invalid address range";
    case 0x09: return "Read/Write address not correctly aligned";
    case 0x0a: return "BankStick not available";
    case 0x0b: return "MIDI IN Overrun Error";
    case 0x0c: return "MIDI IN Frame Error";
    case 0x0d: return "Unknown Query";
    case 0x0e: return "Invalid SysEx command";
    case 0x0f: return "Device ID cannot be programmed again";
    case 0x10: return "Unsupported Debug Command";
    }

    return "Unknown Error Code";
}


//==============================================================================
bool SysexHelper::isValidMidio128Header(const uint8 *data, const uint32 &size, const int &deviceId)
{
    return
        size >= 7 && // the shortest valid header is F0 00 00 7E 44 ((<deviceId> << 4) | <cmd>) F7
        data[0] == 0xf0 &&
        data[1] == 0x00 &&
        data[2] == 0x00 &&
        data[3] == 0x7e &&
        data[4] == 0x44 &&
        (deviceId < 0 || (data[5] >> 4) == deviceId);
}

Array<uint8> SysexHelper::createMidio128Header()
{
    Array<uint8> dataArray;
    dataArray.add(0xf0);
    dataArray.add(0x00);
    dataArray.add(0x00);
    dataArray.add(0x7e);
    dataArray.add(0x44);
    return dataArray;
}

bool SysexHelper::isValidMidio128ReadBlock(const uint8 *data, const uint32 &size, const int &deviceId)
{
    return isValidMidio128Header(data, size, deviceId) && ((data[5] & 0x0f) == 0x03);
}

Array<uint8> SysexHelper::createMidio128ReadBlock(const uint8 &deviceId, const uint8 &block)
{
    Array<uint8> dataArray = createMidio128Header();

    uint8 cmd = 0x03 | ((deviceId << 4) & 0x70);
    dataArray.add(cmd);
    dataArray.add(block);
    dataArray.add(0xf7);

    return dataArray;
}

bool SysexHelper::isValidMidio128WriteBlock(const uint8 *data, const uint32 &size, const int &deviceId)
{
    return isValidMidio128Header(data, size, deviceId) && ((data[5] & 0x0f) == 0x04);
}

Array<uint8> SysexHelper::createMidio128WriteBlock(const uint8 &deviceId, const uint8 &block, const uint8 *data)
{
    Array<uint8> dataArray = createMidio128Header();
    uint8 checksum = 0x00;

    uint8 cmd = 0x04 | ((deviceId << 4) & 0x70);
    dataArray.add(cmd);
    uint8 b;
    dataArray.add(b = block); checksum += b;

    for(int i=0; i<256; ++i) {
        dataArray.add(b = data[i]);
        checksum += b;
    }

    dataArray.add(-(int)checksum & 0x7f);
    dataArray.add(0xf7);

    return dataArray;
}

bool SysexHelper::isValidMidio128Acknowledge(const uint8 *data, const uint32 &size, const int &deviceId)
{
    return isValidMidio128Header(data, size, deviceId) && ((data[5] & 0x0f) == 0x0f);
}

Array<uint8> SysexHelper::createMidio128Ping(const uint8 &deviceId)
{
    Array<uint8> dataArray = createMidio128Header();
    //uint8 checksum = 0x00;

    uint8 cmd = 0x0f | ((deviceId << 4) & 0x70);
    dataArray.add(cmd);
    dataArray.add(0xf7);

    return dataArray;
}


//==============================================================================
bool SysexHelper::isValidMbCvHeader(const uint8 *data, const uint32 &size, const int &deviceId)
{
    return
        size >= 7 && // the shortest valid header is F0 00 00 7E 48 ((<deviceId> << 4) | <cmd>) F7
        data[0] == 0xf0 &&
        data[1] == 0x00 &&
        data[2] == 0x00 &&
        data[3] == 0x7e &&
        data[4] == 0x48 &&
        (deviceId < 0 || data[5] == deviceId);
}

Array<uint8> SysexHelper::createMbCvHeader(const int &deviceId)
{
    Array<uint8> dataArray;
    dataArray.add(0xf0);
    dataArray.add(0x00);
    dataArray.add(0x00);
    dataArray.add(0x7e);
    dataArray.add(0x48);
    dataArray.add(deviceId & 0x7f);
    return dataArray;
}

bool SysexHelper::isValidMbCvReadPatch(const uint8 *data, const uint32 &size, const int &deviceId)
{
    return isValidMbCvHeader(data, size, deviceId) && data[6] == 0x01;
}

Array<uint8> SysexHelper::createMbCvReadPatch(const uint8 &deviceId, const uint8 &patch)
{
    Array<uint8> dataArray = createMbCvHeader(deviceId);

    dataArray.add(0x01);
    dataArray.add(patch);
    dataArray.add(0xf7);

    return dataArray;
}

bool SysexHelper::isValidMbCvWritePatch(const uint8 *data, const uint32 &size, const int &deviceId)
{
    return isValidMbCvHeader(data, size, deviceId) && data[6] == 0x02;
}

Array<uint8> SysexHelper::createMbCvWritePatch(const uint8 &deviceId, const uint8 &patch, const uint8 *data)
{
    Array<uint8> dataArray = createMbCvHeader(deviceId);
    uint8 checksum = 0x00;

    dataArray.add(0x02);
    dataArray.add(patch);

    uint8 b;
    for(int i=0; i<256; ++i) {
        dataArray.add(b = data[i]);
        checksum += b;
    }

    dataArray.add(-(int)checksum & 0x7f);
    dataArray.add(0xf7);

    return dataArray;
}

bool SysexHelper::isValidMbCvAcknowledge(const uint8 *data, const uint32 &size, const int &deviceId)
{
    return isValidMbCvHeader(data, size, deviceId) && data[6] == 0x0f;
}

Array<uint8> SysexHelper::createMbCvPing(const uint8 &deviceId)
{
    Array<uint8> dataArray = createMbCvHeader(deviceId);
    //uint8 checksum = 0x00;

    dataArray.add(0x0f);
    dataArray.add(0xf7);

    return dataArray;
}


//==============================================================================
bool SysexHelper::isValidMbhpMfHeader(const uint8 *data, const uint32 &size, const int &deviceId)
{
    return
        size >= 7 && // the shortest valid header is F0 00 00 7E 4F ((<deviceId> << 4) | <cmd>) F7
        data[0] == 0xf0 &&
        data[1] == 0x00 &&
        data[2] == 0x00 &&
        data[3] == 0x7e &&
        data[4] == 0x4f &&
        (deviceId < 0 || data[5] == deviceId);
}

Array<uint8> SysexHelper::createMbhpMfHeader(const int &deviceId)
{
    Array<uint8> dataArray;
    dataArray.add(0xf0);
    dataArray.add(0x00);
    dataArray.add(0x00);
    dataArray.add(0x7e);
    dataArray.add(0x4f);
    dataArray.add(deviceId & 0x7f);
    return dataArray;
}

bool SysexHelper::isValidMbhpMfReadPatch(const uint8 *data, const uint32 &size, const int &deviceId)
{
    return isValidMbhpMfHeader(data, size, deviceId) && data[6] == 0x01;
}

Array<uint8> SysexHelper::createMbhpMfReadPatch(const uint8 &deviceId, const uint8 &patch)
{
    Array<uint8> dataArray = createMbhpMfHeader(deviceId);

    dataArray.add(0x01);
    dataArray.add(patch);
    dataArray.add(0xf7);

    return dataArray;
}

bool SysexHelper::isValidMbhpMfWritePatch(const uint8 *data, const uint32 &size, const int &deviceId)
{
    return isValidMbhpMfHeader(data, size, deviceId) && data[6] == 0x02;
}

Array<uint8> SysexHelper::createMbhpMfWritePatch(const uint8 &deviceId, const uint8 &patch, const uint8 *data)
{
    Array<uint8> dataArray = createMbhpMfHeader(deviceId);
    uint8 checksum = 0x00;

    dataArray.add(0x02);
    dataArray.add(patch);

    uint8 b;
    for(int i=0; i<256; ++i) {
        dataArray.add(b = ((data[i] >> 0) & 0x0f));
        checksum += b;
        dataArray.add(b = ((data[i] >> 4) & 0x0f));
        checksum += b;
    }

    dataArray.add(-(int)checksum & 0x7f);
    dataArray.add(0xf7);

    return dataArray;
}

bool SysexHelper::isValidMbhpMfWriteDirect(const uint8 *data, const uint32 &size, const int &deviceId)
{
    return isValidMbhpMfHeader(data, size, deviceId) && data[6] == 0x06;
}

Array<uint8> SysexHelper::createMbhpMfWriteDirect(const uint8 &deviceId, const uint8 &addr, const uint8 *data, const uint32 &size)
{
    Array<uint8> dataArray = createMbhpMfHeader(deviceId);

    dataArray.add(0x06);
    dataArray.add((addr >> 7) & 0x7f);
    dataArray.add(addr & 0x7f);

    for(uint32 i=0; i<size; ++i) {
        dataArray.add((data[i] >> 0) & 0x0f);
        dataArray.add((data[i] >> 4) & 0x0f);
    }

    dataArray.add(0xf7);

    return dataArray;
}

bool SysexHelper::isValidMbhpMfFadersGet(const uint8 *data, const uint32 &size, const int &deviceId)
{
    return isValidMbhpMfHeader(data, size, deviceId) && data[6] == 0x09;
}

Array<uint8> SysexHelper::createMbhpMfFadersGet(const uint8 &deviceId, const uint8& startFader)
{
    Array<uint8> dataArray = createMbhpMfHeader(deviceId);

    dataArray.add(0x09);
    dataArray.add(startFader);
    dataArray.add(0xf7);

    return dataArray;
}

bool SysexHelper::isValidMbhpMfFadersSet(const uint8 *data, const uint32 &size, const int &deviceId)
{
    return isValidMbhpMfHeader(data, size, deviceId) && data[6] == 0x0a;
}

Array<uint8> SysexHelper::createMbhpMfFadersSet(const uint8 &deviceId, const uint8& startFader, const Array<uint16> mfValues)
{
    Array<uint8> dataArray = createMbhpMfHeader(deviceId);

    dataArray.add(0x0a);
    dataArray.add(startFader);
    for(int mf=0; mf<mfValues.size(); ++mf) {
        dataArray.add((mfValues[mf] >> 0) & 0x7f);
        dataArray.add((mfValues[mf] >> 7) & 0x7f);
    }

    dataArray.add(0xf7);

    return dataArray;
}

bool SysexHelper::isValidMbhpMfTraceRequest(const uint8 *data, const uint32 &size, const int &deviceId)
{
    return isValidMbhpMfHeader(data, size, deviceId) && data[6] == 0x0b;
}

Array<uint8> SysexHelper::createMbhpMfTraceRequest(const uint8 &deviceId, const uint8 &traceFader, const uint8 &traceScale)
{
    Array<uint8> dataArray = createMbhpMfHeader(deviceId);

    dataArray.add(0x0b);
    dataArray.add(traceFader);
    dataArray.add(traceScale);
    dataArray.add(0xf7);

    return dataArray;
}

bool SysexHelper::isValidMbhpMfTraceDump(const uint8 *data, const uint32 &size, const int &deviceId)
{
    return isValidMbhpMfHeader(data, size, deviceId) && data[6] == 0x0c;
}

Array<uint8> SysexHelper::createMbhpMfTraceDump(const uint8 &deviceId, const Array<uint16> dump)
{
    Array<uint8> dataArray = createMbhpMfHeader(deviceId);

    dataArray.add(0x0c);
    for(int i=0; i<dump.size(); ++i) {
        dataArray.add((dump[i] >> 0) & 0x7f);
        dataArray.add((dump[i] >> 7) & 0x7f);
    }

    dataArray.add(0xf7);

    return dataArray;
}


bool SysexHelper::isValidMbhpMfAcknowledge(const uint8 *data, const uint32 &size, const int &deviceId)
{
    return isValidMbhpMfHeader(data, size, deviceId) && data[6] == 0x0f;
}

Array<uint8> SysexHelper::createMbhpMfPing(const uint8 &deviceId)
{
    Array<uint8> dataArray = createMbhpMfHeader(deviceId);
    //uint8 checksum = 0x00;

    dataArray.add(0x0f);
    dataArray.add(0xf7);

    return dataArray;
}
