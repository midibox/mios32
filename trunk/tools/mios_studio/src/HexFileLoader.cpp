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

#include "HexFileLoader.h"


//==============================================================================
HexFileLoader::HexFileLoader()
    : checkMios8Ranges(true)
    , qualifiedForMios8(0)
    , disqualifiedForMios8(0)
    , requiresMios8Reboot(0)
    , qualifiedForMios32_STM32(0)
    , disqualifiedForMios32_STM32(0)
    , qualifiedForMios32_LPC17(0)
    , disqualifiedForMios32_LPC17(0)
{
}

HexFileLoader::~HexFileLoader()
{
}


//==============================================================================
bool HexFileLoader::loadFile(const File &inFile, String &statusMessage)
{
    std::map<uint32, uint8> dumpMap;

    // algorithm taken over from hex2syx.pl
    // data stored in a map for uncomplicated overlap checks

    hexDumpAddressBlocks.clear();

    FileInputStream *inFileStream = inFile.createInputStream();

    if( !inFileStream ) {
        statusMessage = T("File doesn't exist!");
        deleteAndZero(inFileStream);
        return false;
    }

    if( inFileStream->isExhausted() ) {
        statusMessage = T("File is empty!");
        deleteAndZero(inFileStream);
        return false;
    }

    // read complete file (it turned out, that this is much faster than using inFileStream->readNextLine())
    // note: it seems that Windows can't read more than 100k bytes at once... are we still in the 20th century?!?
    size_t fileSize = inFileStream->getTotalLength();
    MemoryBlock readBufferBlock(fileSize);
    char *readBuffer = static_cast<char*>(readBufferBlock.getData());
    {
        size_t readBytes = 0;
        size_t readOffset = 0;
        while( 1 ) {
            size_t bytesToRead = fileSize - readOffset;
            size_t chunkSize = (bytesToRead >= 50000) ? 50000 : bytesToRead;

            if( !chunkSize )
                break;

            readBytes = inFileStream->read(&readBuffer[readOffset], chunkSize);
            if( readBytes != chunkSize ) {
                char buffer[100];
                sprintf(buffer, "Failed to read file (only %d of %d bytes read)\n", readOffset, fileSize);
                statusMessage = String(buffer);
                deleteAndZero(inFileStream);
                return false;
            }
            readOffset += readBytes;
        }
    }
    deleteAndZero(inFileStream);

    size_t filePosition = 0;
    uint32 lineNumber = 0;
    bool endRead = false;
    uint32 addressExtension = 0;
    unsigned totalBytes = 0;

    std::vector<uint8> record;
    std::map<uint32, uint32> addressBlocks;

    while( filePosition < fileSize ) {
        char *startBuffer = (char *)&readBuffer[filePosition];
        char *endBuffer = startBuffer;
        size_t pos = 0;
        while( ++filePosition < fileSize ) {
            if( *endBuffer == '\n' ) {
                break;
            }

            if( *endBuffer == '\r' ) {
                if( endBuffer[1] == '\n' ) {
                    ++filePosition;
                }
                break;
            }
            ++endBuffer;
            ++pos;
        }
        String lineBuffer(startBuffer, pos);
        ++lineNumber;

        if( !endRead && lineBuffer.length() >= 11 && (lineBuffer.length() % 2) && lineBuffer[0] == ':' ) {
            record.clear();

            for(int i=1; i<lineBuffer.length() ; i+=2)
                record.push_back(lineBuffer.substring(i, i+2).getHexValue32());

#if 0
            printf(":");
            for(int i=0; i<record.size(); ++i)
                printf("%02x ", record[i]);
            printf("\n");
#endif

            unsigned numberBytes = record[0];
            uint16 address16 = (record[1] << 8) | record[2];
            uint8 recordType = record[3];

            if( record.size() != (numberBytes+5) ) {
                statusMessage = String::formatted(T("Wrong number of bytes in line %u"), lineNumber);
                return false;
            }

            uint8 checksum = 0;
            for(int i=0; i<record.size(); ++i)
                checksum += record[i];

            if( checksum != 0 ) {
                statusMessage = String::formatted(T("Wrong checksum in line %u"), lineNumber);
                return false;
            }

            if( recordType == 0x00 ) {
                for(int i=0; i<numberBytes; ++i) {
                    uint32 address32 = addressExtension + address16 + i;

                    if( dumpMap.count(address32) > 0 ) {
                        statusMessage = String::formatted(T("in line %u: address 0x%08x already allocated!"), lineNumber, address32);
                        return false;
                    }

                    dumpMap[address32] = record[4+i];
                    ++totalBytes;

                    uint32 addressBlock = address32 & 0xffffff00;
                    ++addressBlocks[addressBlock];
                }

            } else if( recordType == 0x01 ) {
                endRead = true;
            } else if( recordType == 0x02 ) {
                if( record.size() != (5+2) ) {
                    statusMessage = String::formatted(T("in line %u: for an INHX32 record (0x02) expecting 2 address bytes!"), lineNumber);
                    return false;
                }

                addressExtension = ((record[4] << 8) | (record[5])) << 4;
            } else if( recordType == 0x04 ) {
                if( record.size() != (5+2) ) {
                    statusMessage = String::formatted(T("in line %u: for an INHX32 record (0x04) expecting 2 address bytes!"), lineNumber);
                    return false;
                }

                addressExtension = ((record[4] << 8) | (record[5])) << 16;
            } else {
                // just ignore - e.g. GNU generates record type 0x05
                //printf("Unsupported record type 0x%02x in line %u\n", recordType, lineNumber);
                //return false;
            }
        }
    }

    // transform maps into vectors for easier usage outside this function
    // while doing this, try to qualify the content for Mios8/32
    qualifiedForMios8 = false;
    disqualifiedForMios8 = false;
    requiresMios8Reboot = false;
    qualifiedForMios32_STM32 = false;
    disqualifiedForMios32_STM32 = false;
    qualifiedForMios32_LPC17 = false;
    disqualifiedForMios32_LPC17 = false;

    std::map<uint32, uint32>::iterator it = addressBlocks.begin();
    for(; it!=addressBlocks.end(); ++it) {
        uint32 blockAddress = (*it).first;

        // filter PIC config block
        if( checkMios8Ranges &&
            (blockAddress >= HEX_RANGE_PIC_CONFIG_START && blockAddress <= HEX_RANGE_PIC_CONFIG_END) ) {
            continue;
        }

        hexDumpAddressBlocks.push_back(blockAddress);

        if( checkMios8Ranges &&
            ((blockAddress >= HEX_RANGE_MIOS8_FLASH_START && blockAddress <= HEX_RANGE_MIOS8_FLASH_END) ||
             (blockAddress >= HEX_RANGE_MIOS8_EEPROM_START && blockAddress <= HEX_RANGE_MIOS8_EEPROM_END) ||
             (blockAddress >= HEX_RANGE_MIOS8_BANKSTICK_START && blockAddress <= HEX_RANGE_MIOS8_BANKSTICK_END)) ) {
            qualifiedForMios8 = true;
            disqualifiedForMios32_STM32 = true;
            disqualifiedForMios32_LPC17 = true;

            if( blockAddress >= HEX_RANGE_MIOS8_OS_START && blockAddress <= HEX_RANGE_MIOS8_OS_END )
                requiresMios8Reboot = true;
        } else if( (blockAddress >= HEX_RANGE_MIOS32_STM32_FLASH_START && blockAddress <= HEX_RANGE_MIOS32_STM32_FLASH_END) ) {
            qualifiedForMios32_STM32 = true; // note: upload to RAM supported by bootloader, but too dangerous, e.g. if used RAM is overwritten!
            disqualifiedForMios32_LPC17 = true;
            disqualifiedForMios8 = true;
        } else if( (blockAddress >= HEX_RANGE_MIOS32_LPC17_FLASH_START && blockAddress <= HEX_RANGE_MIOS32_LPC17_FLASH_END) ) {
            qualifiedForMios32_LPC17 = true; // note: upload to RAM supported by bootloader, but too dangerous, e.g. if used RAM is overwritten!
            disqualifiedForMios32_STM32 = true;
            disqualifiedForMios8 = true;
        } else {
            disqualifiedForMios8 = true;
            disqualifiedForMios32_STM32 = true;
            disqualifiedForMios32_LPC17 = true;
        }

        Array<uint8> dataArray;
        for(int offset=0; offset<256; ++offset)
            dataArray.add(dumpMap[blockAddress + offset]);

        hexDump[blockAddress] = dataArray;
    }

    statusMessage << inFile.getFileName()
                  << String::formatted(T(" contains %u bytes (%u blocks)."),
                                       totalBytes,
                                       hexDumpAddressBlocks.size());

    return true;
}


//==============================================================================
MidiMessage HexFileLoader::createMidiMessageForBlock(const uint8 &deviceId, const uint32 &blockAddress, bool forMios32)
{
    Array<uint8> dataArray;
    Array<uint8> dumpArray = hexDump[blockAddress];
    int size = 0x100;
    uint8 checksum = 0x00;

    if( forMios32 )
        dataArray = SysexHelper::createMios32WriteBlock(deviceId, blockAddress, size, checksum);
    else {
        uint32 miosBlockAddress = 0xffffffff; // invalid
        uint8 miosBlockExtension = 0x7; // invalid
        if( blockAddress >= 0x0000 && blockAddress <= 0x7fff ) {
            miosBlockAddress = blockAddress; // lower flash range
            miosBlockExtension = 0x0;
        } else if( blockAddress >= 0x8000 && blockAddress <= 0x3ffff ) {
            miosBlockAddress = blockAddress & 0x7fff; // upper flash range
            miosBlockExtension = blockAddress >> 15;
        } else if( blockAddress >= 0xf00000 && blockAddress <= 0xf00fff ) {
            miosBlockAddress = 0x8000 | (blockAddress & 0x7fff); // EEPROM range
            miosBlockExtension = 0x0;
        } else if( blockAddress >= 0x400000 && blockAddress <= 0x47ffff ) {
            miosBlockAddress = 0x10000 | (blockAddress & 0xffff); // BankStick range
            miosBlockExtension = blockAddress >> 16;
        }

        dataArray = SysexHelper::createMios8WriteBlock(deviceId, miosBlockAddress, miosBlockExtension, size, checksum);
    }

    uint8 m = 0x00;
    int mCounter = 0;
    for(int offset=0; offset<size; ++offset) {
        uint8 b = dumpArray[offset];

        for(int bCounter=0; bCounter<8; ++bCounter) {
            m = (m << 1) | (b & 0x80 ? 0x01 : 0x00);
            b <<= 1;
            ++mCounter;
            if( mCounter == 7 ) {
                dataArray.add(m);
                checksum += m;
                m = 0;
                mCounter = 0;
            }
        }
    }

    if( mCounter > 0 ) {
        while( mCounter < 7 ) {
            m <<= 1;
            ++mCounter;
        }
        dataArray.add(m);
        checksum += m;
    }

    checksum = -(int)checksum;

    dataArray.add(checksum & 0x7f);
    dataArray.add(0xf7);
    return SysexHelper::createMidiMessage(dataArray);
}
