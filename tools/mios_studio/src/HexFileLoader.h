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

    // check if address ranges are allowed for Mios8 and/or Mios32
    bool checkMios8Ranges; // false if LPC17 is detected via query
    bool qualifiedForMios8;
    bool disqualifiedForMios8;
    bool requiresMios8Reboot;
    bool qualifiedForMios32_STM32;
    bool disqualifiedForMios32_STM32;
    bool qualifiedForMios32_LPC17;
    bool disqualifiedForMios32_LPC17;

    //==============================================================================
    static const uint32 HEX_RANGE_MIOS8_BL_START           = 0x00000000; // Bootloader range
    static const uint32 HEX_RANGE_MIOS8_BL_END             = 0x000003ff; // Upload usually not allowed here!
    static const uint32 HEX_RANGE_MIOS8_BL_CHECK           = 1;          // but this check can be optionally disabled (should we change this to an hidden user configurable option?)
    static const uint32 HEX_RANGE_MIOS8_OS_START           = 0x00000000; // MIOS8 range
    static const uint32 HEX_RANGE_MIOS8_OS_END             = 0x00002fff; // allowed flash range, but reboot required
    static const uint32 HEX_RANGE_MIOS8_FLASH_START        = 0x00000400;
    static const uint32 HEX_RANGE_MIOS8_FLASH_END          = 0x0003ffff;
    static const uint32 HEX_RANGE_MIOS8_EEPROM_START       = 0x00f00000;
    static const uint32 HEX_RANGE_MIOS8_EEPROM_END         = 0x00f00fff;
    static const uint32 HEX_RANGE_PIC_CONFIG_START         = 0x00300000; // PIC config range will be filtered (not transfered via MIDI)
    static const uint32 HEX_RANGE_PIC_CONFIG_END           = 0x003000ff;
    static const uint32 HEX_RANGE_MIOS8_BANKSTICK_START    = 0x00400000;
    static const uint32 HEX_RANGE_MIOS8_BANKSTICK_END      = 0x0047ffff;

    static const uint32 HEX_RANGE_MIOS32_STM32_FLASH_START = 0x08000000;
    static const uint32 HEX_RANGE_MIOS32_STM32_FLASH_END   = 0x08ffffff;
    static const uint32 HEX_RANGE_MIOS32_STM32_BL_START    = 0x08000000; // Bootloader within
    static const uint32 HEX_RANGE_MIOS32_STM32_BL_END      = 0x08003fff; // allowed flash range - will be excluded automatically

    static const uint32 HEX_RANGE_MIOS32_LPC17_FLASH_START = 0x00000000;
    static const uint32 HEX_RANGE_MIOS32_LPC17_FLASH_END   = 0x00ffffff;
    static const uint32 HEX_RANGE_MIOS32_LPC17_BL_START    = 0x00000000; // Bootloader within
    static const uint32 HEX_RANGE_MIOS32_LPC17_BL_END      = 0x00003fff; // allowed flash range - will be excluded automatically


protected:
    std::map<uint32, Array<uint8> > hexDump;
};

#endif /* __HEX_FILE_LOADER_H */
