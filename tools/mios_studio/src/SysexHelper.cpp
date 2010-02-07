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


// for decoding error messages (not supported yet)
#define MIOS32_MIDI_SYSEX_DISACK_LESS_BYTES_THAN_EXP  0x01
#define MIOS32_MIDI_SYSEX_DISACK_MORE_BYTES_THAN_EXP  0x02
#define MIOS32_MIDI_SYSEX_DISACK_WRONG_CHECKSUM       0x03
#define MIOS32_MIDI_SYSEX_DISACK_WRITE_FAILED         0x04
#define MIOS32_MIDI_SYSEX_DISACK_WRITE_ACCESS         0x05
#define MIOS32_MIDI_SYSEX_DISACK_MIDI_TIMEOUT         0x06
#define MIOS32_MIDI_SYSEX_DISACK_WRONG_DEBUG_CMD      0x07
#define MIOS32_MIDI_SYSEX_DISACK_WRONG_ADDR_RANGE     0x08
#define MIOS32_MIDI_SYSEX_DISACK_ADDR_NOT_ALIGNED     0x09
#define MIOS32_MIDI_SYSEX_DISACK_BS_NOT_AVAILABLE     0x0a
#define MIOS32_MIDI_SYSEX_DISACK_OVERRUN              0x0b
#define MIOS32_MIDI_SYSEX_DISACK_FRAME_ERROR          0x0c
#define MIOS32_MIDI_SYSEX_DISACK_UNKNOWN_QUERY        0x0d
#define MIOS32_MIDI_SYSEX_DISACK_INVALID_COMMAND      0x0e
#define MIOS32_MIDI_SYSEX_DISACK_PROG_ID_NOT_ALLOWED  0x0f
#define MIOS32_MIDI_SYSEX_DISACK_UNSUPPORTED_DEBUG    0x10



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
        (deviceId < 0 || data[5] == deviceId );
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
        (deviceId < 0 || data[5] == deviceId );
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

Array<uint8> SysexHelper::createMios8WriteBlock(const uint8 &deviceId, const uint32 &address, const uint32 &size, uint8 &checksum)
{
    Array<uint8> dataArray = createMios8Header(deviceId);
    checksum = 0x00;

    // TODO: address extension!
    uint8 cmd = 0x02;
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
    return isValidMios32Header(data, size, deviceId) && data[6] == 0x0d;
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

