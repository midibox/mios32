/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
// $Id$
/*
 * Sysex Help Routines
 *
 * ==========================================================================
 *
 *  Copyright (C) 2010 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _SYSEX_HELPER_H
#define _SYSEX_HELPER_H

#include "includes.h"


class SysexHelper
{
public:
    //==============================================================================
    SysexHelper(void);
    ~SysexHelper();

    //==============================================================================
    static bool isValidMios8Header(const uint8 *data, const uint32 &size, const int &deviceId); // if deviceId < 0, it won't be checked
    static Array<uint8> createMios8Header(const uint8 &deviceId);
    static bool isValidMios32Header(const uint8 *data, const uint32 &size, const int &deviceId); // if deviceId < 0, it won't be checked
    static Array<uint8> createMios32Header(const uint8 &deviceId);

    //==============================================================================
    static bool isValidMios8Acknowledge(const uint8 *data, const uint32 &size, const int &deviceId); // if deviceId < 0, it won't be checked
    static Array<uint8> createMios8Acknowledge(const uint8 &deviceId);
    static bool isValidMios32Acknowledge(const uint8 *data, const uint32 &size, const int &deviceId); // if deviceId < 0, it won't be checked
    static Array<uint8> createMios32Acknowledge(const uint8 &deviceId);

    //==============================================================================
    static bool isValidMios8Error(const uint8 *data, const uint32 &size, const int &deviceId); // if deviceId < 0, it won't be checked
    static Array<uint8> createMios8Error(const uint8 &deviceId);
    static bool isValidMios32Error(const uint8 *data, const uint32 &size, const int &deviceId); // if deviceId < 0, it won't be checked
    static Array<uint8> createMios32Error(const uint8 &deviceId);

    //==============================================================================
    static bool isValidMios8DebugMessage(const uint8 *data, const uint32 &size, const int &deviceId); // if deviceId < 0, it won't be checked
    static Array<uint8> createMios8DebugMessage(const uint8 &deviceId);
    static bool isValidMios32DebugMessage(const uint8 *data, const uint32 &size, const int &deviceId); // if deviceId < 0, it won't be checked
    static Array<uint8> createMios32DebugMessage(const uint8 &deviceId);

    //==============================================================================
    static bool isValidMios8WriteBlock(const uint8 *data, const uint32 &size, const int &deviceId);
    static Array<uint8> createMios8WriteBlock(const uint8 &deviceId, const uint32 &address, const uint32 &size, uint8 &checksum);
    static bool isValidMios32WriteBlock(const uint8 *data, const uint32 &size, const int &deviceId);
    static Array<uint8> createMios32WriteBlock(const uint8 &deviceId, const uint32 &address, const uint32 &size, uint8 &checksum);

    //==============================================================================
    static bool isValidMios8UploadRequest(const uint8 *data, const uint32 &size, const int &deviceId);
    static bool isValidMios32UploadRequest(const uint8 *data, const uint32 &size, const int &deviceId);

    //==============================================================================
    static bool isValidMios32Query(const uint8 *data, const uint32 &size, const int &deviceId); // if deviceId < 0, it won't be checked
    static Array<uint8> createMios32Query(const uint8 &deviceId);

    //==============================================================================
    static MidiMessage createMidiMessage(Array<uint8> &dataArray);


protected:
};

#endif /* _SYSEX_HELPER_H */
