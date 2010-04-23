/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
// $Id$
/*
 * OSC Help Routines
 * (taken from MIOS32_OSC)
 * Documentation: see there
 *
 * ==========================================================================
 *
 *  Copyright (C) 2010 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _OSC_HELPER_H
#define _OSC_HELPER_H

#include "includes.h"


/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////

// OSC: maximum number of path parts (e.g. /a/b/c/d -> 4 parts)
#ifndef MIOS32_OSC_MAX_PATH_PARTS
#define MIOS32_OSC_MAX_PATH_PARTS 8
#endif

// OSC: maximum number of OSC arguments in message
#ifndef MIOS32_OSC_MAX_ARGS
#define MIOS32_OSC_MAX_ARGS 8
#endif


/////////////////////////////////////////////////////////////////////////////
class OscListener; // forward declaration

/////////////////////////////////////////////////////////////////////////////
class OscHelper
{
public:

    typedef struct {
        unsigned seconds;
        unsigned fraction;
    } OscTimetagT;

    typedef struct {
        OscTimetagT timetag; // the timetag (seconds and fraction)
        unsigned char numPathParts; // number of address parts
        char *pathPart[MIOS32_OSC_MAX_PATH_PARTS]; // an array of address paths without wildcards (!) - this allows the method to reconstruct the complete path, e.g. to send parameters to different targets
        unsigned char numArgs; // number of arguments
        char          argType[MIOS32_OSC_MAX_ARGS]; // array of argument tags
        unsigned char *argPtr[MIOS32_OSC_MAX_ARGS]; // pointer to arguments (have to be fetched with MIOS32_OSC_Get*() functions)
    } OscArgsT;

    typedef struct OscSearchTreeT {
        const char *address;    // OSC address part or NULL if there are no more address parts/methods in the "OSC container"
        struct OscSearchTreeT *next; // link to the next address part or NULL if the leaf has been reached (method reached)
        OscListener *oscListener; // if leaf: object that dispatches the addressed OSC method (no function pointers possible in C++... unfortunately)
        unsigned methodArg;  // optional argument for methods (32bit)
    } OscSearchTreeT;

    //==============================================================================
    OscHelper(void);
    ~OscHelper();

    //==============================================================================
    static unsigned getWord(unsigned char *buffer);
    static unsigned char *putWord(unsigned char *buffer, unsigned word);
    static Array<uint8> createWord(const unsigned& word);

    static OscTimetagT getTimetag(unsigned char *buffer);
    static unsigned char *putTimetag(unsigned char *buffer, OscTimetagT timetag);
    static Array<uint8> createTimetag(const OscTimetagT& timetag);

    static int getInt(unsigned char *buffer);
    static unsigned char *putInt(unsigned char *buffer, int value);
    static Array<uint8> createInt(const int& value);

    static float getFloat(unsigned char *buffer);
    static unsigned char *putFloat(unsigned char *buffer, float value);
    static Array<uint8> createFloat(const float& value);

    static char *getString(unsigned char *buffer);
    static unsigned char *putString(unsigned char *buffer, const char *str);
    static Array<uint8> createString(const String& str);
    
    static unsigned getBlobLength(unsigned char *buffer);
    static unsigned char *getBlobData(unsigned char *buffer);
    static unsigned char *putBlob(unsigned char *buffer, unsigned char *data, unsigned len);
    static Array<uint8> createBlob( unsigned char *data, const unsigned& len);

    static long long getLongLong(unsigned char *buffer);
    static unsigned char *putLongLong(unsigned char *buffer, long long value);
    static Array<uint8> createLongLong(const long long& value);

    static double getDouble(unsigned char *buffer);
    static unsigned char *putDouble(unsigned char *buffer, double value);
    static Array<uint8> createDouble(const double& value);

    static char getChar(unsigned char *buffer);
    static unsigned char *putChar(unsigned char *buffer, char c);
    static Array<uint8> createChar(const char& c);

    static unsigned getMIDI(unsigned char *buffer);
    static unsigned char *putMIDI(unsigned char *buffer, unsigned p);
    static Array<uint8> createMIDI(const unsigned& p);

    static int parsePacket(unsigned char *packet, const unsigned& len, OscSearchTreeT *searchTree);

    static String element2String(const OscArgsT& oscArgs);
    static Array<uint8> string2Packet(const String& oscString, String& statusMessage);

protected:
    static int searchElement(unsigned char *buffer, const unsigned& len, OscArgsT *oscArgs, OscSearchTreeT *searchTree);
    static int searchPath(char *path, OscArgsT *oscArgs, const unsigned& methodArg, OscSearchTreeT *searchTree);

    static Array<uint8> createElement(const Array<uint8>& oscPath, const String& oscArgsString, const Array<uint8>& oscArgs);

};

/////////////////////////////////////////////////////////////////////////////
class OscListener
{
public:
    virtual void parsedOscPacket(const OscHelper::OscArgsT& oscArgs, const unsigned& methodArg) = 0;
};

#endif /* _OSC_HELPER_H */
