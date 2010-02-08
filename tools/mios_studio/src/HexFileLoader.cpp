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

    if( !inFileStream || inFileStream->isExhausted() ) {
        statusMessage = T("File is empty!");
        return false;
    }

    uint32 lineNumber = 0;
    bool endRead = false;
    uint32 addressExtension;
    unsigned totalBytes = 0;

    std::vector<uint8> record;
    std::map<uint32, uint32> addressBlocks;

    while( !inFileStream->isExhausted() ) {
        String lineBuffer = inFileStream->readNextLine();
        ++lineNumber;

        if( !endRead && lineBuffer.length() >= 11 && (lineBuffer.length() % 2) && lineBuffer[0] == ':' ) {
            record.clear();
            for(int i=1; i<lineBuffer.length(); i+=2)
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
    std::map<uint32, uint32>::iterator it = addressBlocks.begin();
    for(; it!=addressBlocks.end(); ++it) {
        uint32 blockAddress = (*it).first;
        hexDumpAddressBlocks.push_back(blockAddress);

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
    else
        dataArray = SysexHelper::createMios8WriteBlock(deviceId, blockAddress, size, checksum);

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
