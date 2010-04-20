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


class OscHelper
{
public:

    typedef struct {
        unsigned seconds;
        unsigned fraction;
    } OscTimetagT;

    typedef struct {
        OscTimetagT timetag; // the timetag (seconds and fraction)
        String path;
        unsigned char numArgs; // number of arguments
        char          argType[MIOS32_OSC_MAX_ARGS]; // array of argument tags
        unsigned char *argPtr[MIOS32_OSC_MAX_ARGS]; // pointer to arguments (have to be fetched with MIOS32_OSC_Get*() functions)
    } OscArgsT;


    //==============================================================================
    OscHelper(void);
    ~OscHelper();

    //==============================================================================
    static unsigned getWord(unsigned char *buffer);
    static unsigned char *putWord(unsigned char *buffer, unsigned word);
    static OscTimetagT getTimetag(unsigned char *buffer);
    static unsigned char *putTimetag(unsigned char *buffer, OscTimetagT timetag);
    static int getInt(unsigned char *buffer);
    static unsigned char *putInt(unsigned char *buffer, int value);
    static float getFloat(unsigned char *buffer);
    static unsigned char *putFloat(unsigned char *buffer, float value);
    static char *getString(unsigned char *buffer);
    static unsigned char *putString(unsigned char *buffer, char *str);
    static unsigned getBlobLength(unsigned char *buffer);
    static unsigned char *getBlobData(unsigned char *buffer);
    static unsigned char *putBlob(unsigned char *buffer, unsigned char *data, unsigned len);
    static long long getLongLong(unsigned char *buffer);
    static unsigned char *putLongLong(unsigned char *buffer, long long value);
    static double getDouble(unsigned char *buffer);
    static unsigned char *putDouble(unsigned char *buffer, double value);
    static char getChar(unsigned char *buffer);
    static unsigned char *putChar(unsigned char *buffer, char c);
    static unsigned getMIDI(unsigned char *buffer);
    static unsigned char *putMIDI(unsigned char *buffer, unsigned p);

protected:

};

#endif /* _OSC_HELPER_H */
