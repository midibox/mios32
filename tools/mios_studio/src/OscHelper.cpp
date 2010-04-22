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

#include "OscHelper.h"
#include <string.h>


//==============================================================================
OscHelper::OscHelper()
{
}

OscHelper::~OscHelper()
{
}


//==============================================================================
// strnlen() not available for all libc's, therefore we use a local solution here
static size_t my_strnlen(char *str, size_t max_len)
{
  size_t len = 0;

  while( *str++ && (len < max_len) )
    ++len;

  return len;
}

//==============================================================================
// stpcpy is not available in windows, therefore we use a local solution here
#ifdef WIN32
static char *stpcpy(char *dest, const char *src){strcpy(dest,src);return dest +strlen(dest);}
#endif

//==============================================================================
/////////////////////////////////////////////////////////////////////////////
// Gets a word (4 bytes) from buffer
// \param[in] buffer pointer to OSC message buffer 
// \return 32bit unsigned integer
/////////////////////////////////////////////////////////////////////////////
unsigned OscHelper::getWord(unsigned char *buffer)
{
  // taking endianess into account
  return 
    (((unsigned)*(buffer+0)) << 24) | (((unsigned)*(buffer+1)) << 16) |
    (((unsigned)*(buffer+2)) <<  8) | (((unsigned)*(buffer+3)) <<  0);
}

/////////////////////////////////////////////////////////////////////////////
// Puts a word (4 bytes) into buffer
// \param[in] buffer pointer to OSC message buffer 
// \param[in] word 32bit word
// \return buffer pointer behind the inserted entry
/////////////////////////////////////////////////////////////////////////////
unsigned char *OscHelper::putWord(unsigned char *buffer, unsigned word)
{
  *buffer++ = (word >> 24) & 0xff;
  *buffer++ = (word >> 16) & 0xff;
  *buffer++ = (word >>  8) & 0xff;
  *buffer++ = (word >>  0) & 0xff;
  return buffer;
}

/////////////////////////////////////////////////////////////////////////////
// Gets a timetag (8 bytes) from buffer
// \param[in] buffer pointer to OSC message buffer 
// \return timetag (seconds and fraction part)
/////////////////////////////////////////////////////////////////////////////
OscHelper::OscTimetagT OscHelper::getTimetag(unsigned char *buffer)
{
  OscTimetagT timetag;
  timetag.seconds = getWord(buffer);
  timetag.fraction = getWord(buffer+4);
  return timetag;
}

/////////////////////////////////////////////////////////////////////////////
// Puts a timetag (8 bytes) into buffer
// \param[in] buffer pointer to OSC message buffer 
// \param[in] timetag the timetag which should be inserted
// \return buffer pointer behind the inserted entry
/////////////////////////////////////////////////////////////////////////////
unsigned char *OscHelper::putTimetag(unsigned char *buffer, OscTimetagT timetag)
{
  buffer = putWord(buffer, timetag.seconds);
  buffer = putWord(buffer, timetag.fraction);
  return buffer;
}

/////////////////////////////////////////////////////////////////////////////
// Gets a word (4 bytes) from buffer and converts it to a 32bit signed integer.
// \param[in] buffer pointer to OSC message buffer 
// \return 32bit signed integer
/////////////////////////////////////////////////////////////////////////////
int OscHelper::getInt(unsigned char *buffer)
{
  return (int)getWord(buffer);
}

/////////////////////////////////////////////////////////////////////////////
// Puts a 32bit signed integer into buffer
// \param[in] buffer pointer to OSC message buffer 
// \param[in] value the integer value which should be inserted
// \return buffer pointer behind the inserted entry
/////////////////////////////////////////////////////////////////////////////
unsigned char *OscHelper::putInt(unsigned char *buffer, int value)
{
  return putWord(buffer, (unsigned)value);
}

/////////////////////////////////////////////////////////////////////////////
// Gets a word (4 bytes) from buffer and converts it to a float with 
// normal precession
// \param[in] buffer pointer to OSC message buffer 
// \return float with normal procession
/////////////////////////////////////////////////////////////////////////////
float OscHelper::getFloat(unsigned char *buffer)
{
#if 0
  unsigned word = getWord(buffer);
  return *(float *)(&word);
#else
  // TK: doesn't work with my gcc installation (i686-apple-darwin9-gcc-4.0.1):
  // float not converted correctly - it works when optimisation is disabled!
  // according to http://gcc.gnu.org/ml/gcc-bugs/2003-02/msg01128.html this isn't a bug...
  // workaround:
  union { unsigned word; float f; } converted;
  converted.word = getWord(buffer);
  return converted.f;
#endif
}


/////////////////////////////////////////////////////////////////////////////
// Puts a float with normal precission into buffer
// \param[in] buffer pointer to OSC message buffer 
// \param[in] value the float value which should be inserted
// \return buffer pointer behind the inserted entry
/////////////////////////////////////////////////////////////////////////////
unsigned char *OscHelper::putFloat(unsigned char *buffer, float value)
{
  union { unsigned word; float f; } converted;
  converted.f = value;
  return putWord(buffer, converted.word);
}

/////////////////////////////////////////////////////////////////////////////
// Returns pointer to a string in message buffer
// \param[in] buffer pointer to OSC message buffer 
// \return zero-terminated string
/////////////////////////////////////////////////////////////////////////////
char *OscHelper::getString(unsigned char *buffer)
{
  return (char *)buffer; // OSC protocol ensures zero termination (checked in MIOS32_OSC_SearchElement)
}

/////////////////////////////////////////////////////////////////////////////
// Puts a string into buffer and pads with 0 until word boundary has been reached
// \param[in] buffer pointer to OSC message buffer 
// \param[in] value the string which should be inserted
// \return buffer pointer behind the inserted entry
/////////////////////////////////////////////////////////////////////////////
unsigned char *OscHelper::putString(unsigned char *buffer, char *str)
{
  unsigned char *buffer_start = buffer;

  buffer = (unsigned char *)stpcpy((char *)buffer, str);
  *buffer++ = 0;

  // pad with zeroes until word boundary is reached
  while( (unsigned)(buffer-buffer_start) % 4 )
    *buffer++ = 0;

  return buffer;
}

/////////////////////////////////////////////////////////////////////////////
// Returns the length of a Blob
// \param[in] buffer pointer to OSC message buffer 
// \return blob length
/////////////////////////////////////////////////////////////////////////////
unsigned OscHelper::getBlobLength(unsigned char *buffer)
{
  return *buffer;
}

/////////////////////////////////////////////////////////////////////////////
// Returns the data part of a Blob
// \param[in] buffer pointer to OSC message buffer 
// \return pointer to data part of a Blob
/////////////////////////////////////////////////////////////////////////////
unsigned char *OscHelper::getBlobData(unsigned char *buffer)
{
  return buffer+4;
}


/////////////////////////////////////////////////////////////////////////////
// Puts an OSC-Blob into buffer and pads with 0 until word boundary has been reached
// \param[in] buffer pointer to OSC message buffer 
// \param[in] data blob data
// \param[in] len blob size
// \return buffer pointer behind the inserted entry
/////////////////////////////////////////////////////////////////////////////
unsigned char *OscHelper::putBlob(unsigned char *buffer, unsigned char *data, unsigned len)
{
  // ensure that length considers word alignment
  unsigned aligned_len = (len+3) & 0xfffffffc;

  // add length
  buffer = putWord(buffer, aligned_len);

  // add bytes
  int i;
  for(i=0; i<len; ++i)
    *buffer++ = *data++;

  // pad with zeroes
  while( i % 4 )
    *buffer++ = 0;

  return buffer;
}


/////////////////////////////////////////////////////////////////////////////
// Gets two words (8 bytes) from buffer and converts them to a 64bit signed integer.
// \param[in] buffer pointer to OSC message buffer 
// \return 64bit signed integer
/////////////////////////////////////////////////////////////////////////////
long long OscHelper::getLongLong(unsigned char *buffer)
{
  return ((long long)getWord(buffer) << 32) | getWord(buffer+4);
}

/////////////////////////////////////////////////////////////////////////////
// Puts a 64bit signed integer into buffer
// \param[in] buffer pointer to OSC message buffer 
// \param[in] value the "long long" value which should be inserted
// \return buffer pointer behind the inserted entry
/////////////////////////////////////////////////////////////////////////////
unsigned char *OscHelper::putLongLong(unsigned char *buffer, long long value)
{
  buffer = putWord(buffer, (unsigned)(value >> 32));
  buffer = putWord(buffer, (unsigned)value);
  return buffer;
}


/////////////////////////////////////////////////////////////////////////////
// Gets two words (8 bytes) from buffer and converts them to a float with 
// double precession
// \param[in] buffer pointer to OSC message buffer 
// \return float with double procession
/////////////////////////////////////////////////////////////////////////////
double OscHelper::getDouble(unsigned char *buffer)
{
#if 0
  long long word = ((long long)getWord(buffer) << 32) | getWord(buffer+4);
  return *(double *)(&word);
#else
  // TK: doesn't work with my gcc installation (i686-apple-darwin9-gcc-4.0.1):
  // float not converted correctly - it works when optimisation is disabled!
  // according to http://gcc.gnu.org/ml/gcc-bugs/2003-02/msg01128.html this isn't a bug...
  // workaround:
  union { long long word; double d; } converted;
  converted.word = getLongLong(buffer);
  return converted.d;
#endif
}

/////////////////////////////////////////////////////////////////////////////
// Puts a float with double precission into buffer
// \param[in] buffer pointer to OSC message buffer 
// \param[in] value the double value which should be inserted
// \return buffer pointer behind the inserted entry
/////////////////////////////////////////////////////////////////////////////
unsigned char *OscHelper::putDouble(unsigned char *buffer, double value)
{
  union { long long word; double d; } converted;
  converted.d = value;
  return putLongLong(buffer, converted.word);
}


/////////////////////////////////////////////////////////////////////////////
// Returns a character
// \param[in] buffer pointer to OSC message buffer 
// \return a single character
/////////////////////////////////////////////////////////////////////////////
char OscHelper::getChar(unsigned char *buffer)
{
  return *buffer; // just for completeness..
}

/////////////////////////////////////////////////////////////////////////////
// Puts a character into buffer and pads with 3 zeros (word aligned)
// \param[in] buffer pointer to OSC message buffer 
// \param[in] c the character which should be inserted
// \return buffer pointer behind the inserted entry
/////////////////////////////////////////////////////////////////////////////
unsigned char *OscHelper::putChar(unsigned char *buffer, char c)
{
  return putWord(buffer, (unsigned)c);
}



/////////////////////////////////////////////////////////////////////////////
// Returns a MIDI package
// \param[in] buffer pointer to OSC message buffer 
// \return a MIOS32 compliant MIDI package
/////////////////////////////////////////////////////////////////////////////
unsigned OscHelper::getMIDI(unsigned char *buffer)
{
    // note: no extra conversion to MIOS32 MIDI package format
    return getWord(buffer);
}

/////////////////////////////////////////////////////////////////////////////
// Puts a MIDI package into buffer
// \param[in] buffer pointer to OSC message buffer 
// \param[in] p the MIDI package which should be inserted
// \return buffer pointer behind the inserted entry
/////////////////////////////////////////////////////////////////////////////
unsigned char *OscHelper::putMIDI(unsigned char *buffer, unsigned p)
{
    // note: no extra conversion to MIOS32 MIDI package format
    return putWord(buffer, p);
}
