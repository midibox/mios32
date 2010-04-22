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


/////////////////////////////////////////////////////////////////////////////
// Parses an incoming OSC packet and calls OSC methods defined in searchTree
// on matching addresses
// \param[in] packet pointer to OSC packet
// \param[in] len length of packet
// \param[in] searchTree a tree which defines address parts and methods to be called
// \return 0 if packet has been parsed w/o errors
// \return -1 if packet format invalid
// \return -2 if the packet contains an OSC element with invalid format
// \return -3 if the packet contains an OSC element with an unsupported format
// returns -4 if MIOS32_OSC_MAX_PATH_PARTS has been exceeded
/////////////////////////////////////////////////////////////////////////////
int OscHelper::parsePacket(unsigned char *packet, const unsigned& len, OscHelper::OscSearchTreeT *searchTree)
{
    // store osc arguments (and more...) into oscArgs variable
    OscArgsT oscArgs;

    // check if we got a bundle
    if( strncmp((char *)packet, "#bundle", len) == 0 ) {
        unsigned pos = 8;

        // we expect at least 8 bytes for the timetag
        if( (pos+8) > len )
            return -1; // invalid format


        // get timetag
        oscArgs.timetag = getTimetag((unsigned char *)packet+pos);
        pos += 8;

        // parse elements
        while( (pos+4) <= len ) {
            // get element size
            unsigned elem_size = getWord((unsigned char *)(packet+pos));
            pos += 4;

            // invalid packet if elem_size exceeds packet length
            if( (pos+elem_size) > len )
                return -1; // invalid packet

            // parse element if size > 0
            if( elem_size ) {
                int status = searchElement((unsigned char *)(packet+pos), elem_size, &oscArgs, searchTree);
                if( status < 0 )
                    return status;
            }

            // switch to next element
            pos += elem_size;
        }
    
    } else {
        // no timetag
        oscArgs.timetag.seconds = 0;
        oscArgs.timetag.fraction = 0;

        // get element size
        unsigned elemSize = getWord(packet);

        // invalid packet if elemSize exceeds packet length
        if( elemSize > len )
            return -1; // invalid packet

        // parse element if size > 0
        if( elemSize ) {
            int status = searchElement((unsigned char *)(packet+4), elemSize, &oscArgs, searchTree);
            if( status < 0 )
                return status;
        }
    }

    return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Internal function:
// parses a single OSC element
// returns -2 if element has invalid format
// returns -3 if element contains an unsupported format
// returns -4 if MIOS32_OSC_MAX_PATH_PARTS has been exceeded
/////////////////////////////////////////////////////////////////////////////
int OscHelper::searchElement(unsigned char *buffer, const unsigned& len, OscHelper::OscArgsT *oscArgs, OscHelper::OscSearchTreeT *searchTree)
{
    // exit immediately if element is empty
    if( !len )
        return 0;

    // according to OSC spec, the path could be ommitted, and the element could start with the argument list
    // I don't know how to handle this correctly, but we should at least exit in this case
    unsigned char *path = buffer;
    if( *path == ',' )
        return -3; // unsupported element format

    // path: determine string length
    size_t pathLen = my_strnlen((char *)path, len);

    // check for valid string
    if( pathLen < 2 || path[pathLen] != 0 ) // expect at least two characters, e.g. "/*"
        return -2; // invalid element format

    // path must start with '/'
    if( *path != '/' )
        return -2; // invalid element format

    // tags are starting at word aligned offset
    // add +1 to pathLen, since \0 terminator is counted as well
    size_t tagsPos = (pathLen+1 + 3) & 0xfffffc;

    // check that tags are at valid position
    if( tagsPos >= len )
        return -2; // invalid element format

    // tags: determine string length
    unsigned char *tags = (unsigned char *)(buffer + tagsPos);
    size_t tagsLen = my_strnlen((char *)tags, len-tagsPos);

    // check for valid string
    if( tagsLen == 0 || tags[tagsLen] != 0 )
        return -2; // invalid element format

    // check that tags are starting with comma
    if( *tags != ',' )
        return -2; // invalid element format

    // number of arguments:
    unsigned numArgs = tagsLen - 1;

    // limit by max number of args which can be stored in oscArgs structure
    if( numArgs > MIOS32_OSC_MAX_ARGS )
        numArgs = MIOS32_OSC_MAX_ARGS;

    // arguments are starting at word aligned offset
    // add +1 to tagsLen, since \0 terminator is counted as well
    size_t argPos = (tagsPos + tagsLen+1 + 3) & 0xfffffc;

    // parse arguments
    oscArgs->numArgs = 0;
    unsigned arg;
    for(arg=0; arg<numArgs; ++arg) {
        // check that argument is at valid position
        if( argPos > len ) // TK: use > instead of >= to cover non-value parameters like T/F/I/...
            return -2; // invalid element format

        // store type and pointer to argument
        oscArgs->argType[oscArgs->numArgs] = tags[arg+1];
        oscArgs->argPtr[oscArgs->numArgs] = (unsigned char *)(buffer + argPos);

        // branch depending on argument tag
        unsigned char knownArg = 0;
        switch( tags[arg+1] ) {
        case 'i': // int32
        case 'f': // float32
        case 'c': // ASCII character
        case 'r': // 32bit RGBA color
        case 'm': // 4 byte MIDI message
            knownArg = 1;
            argPos += 4;
            break;

        case 's':   // OSC-string
        case 'S': { // OSC alternate string
            knownArg = 1;
            char *str = (char *)oscArgs->argPtr[oscArgs->numArgs];
            size_t strLen = my_strnlen(str, len-argPos);
            // check for valid string
            if( strLen == 0 || str[strLen] != 0 )
                return -2; // invalid element format
            // next argument at word aligned offset
            // add +1 to strLen, since \0 terminator is counted as well
            argPos = (argPos + strLen+1 + 3) & 0xfffffc;
        } break;

        case 'b': { // OSC-blob
            knownArg = 1;
            unsigned blobLen = getBlobLength((unsigned char *)(buffer + argPos));
            // next argument at word aligned offset
            argPos = (argPos + 4 + blobLen + 3) & 0xfffffc;
        } break;

        case 'h': // long64
        case 't': // OSC timetag
        case 'd': // float64 (double)
            knownArg = 1;
            argPos += 8;
            break;

        case 'T': // TRUE
        case 'F': // FALSE
        case 'N': // NIL
        case 'I': // Infinitum
        case '[': // Begin of Array
        case ']': // End of Array
            knownArg = 1;
            break;
        }

        // according to OSC V1.0 spec, nonstandard arguments should be discarded (don't report error)
        // since we don't know the position to the next argument, we have to stop parsing here!
        if( knownArg )
            ++oscArgs->numArgs;
        else
            break;
    }

    // finally parse for elements which are matching the OSC address
    oscArgs->numPathParts = 0;
    return searchPath((char *)&path[1], oscArgs, 0x00000000, searchTree);
}


/////////////////////////////////////////////////////////////////////////////
// Internal function:
// searches in searchTree for matching OSC addresses
// returns -4 if MIOS32_OSC_MAX_PATH_PARTS has been exceeded
/////////////////////////////////////////////////////////////////////////////
int OscHelper::searchPath(char *path, OscHelper::OscArgsT *oscArgs, const unsigned& methodArg, OscHelper::OscSearchTreeT *searchTree)
{
    if( oscArgs->numPathParts >= MIOS32_OSC_MAX_PATH_PARTS )
        return -4; // maximum number of path parts exceeded

    while( searchTree->address != NULL ) {
        // compare OSC address with name of tree item
        unsigned char match = 1;
        unsigned char wildcard = 0;

        char *str1 = path;
        char *str2 = (char *)searchTree->address;
        size_t sepPos = 0;

        while( *str1 != 0 && *str1 != '/' ) {
            if( *str1 == '*' ) {
                // '*' wildcard: continue to end of address part
                while( *str1 != 0 && *str1 != '/' ) {
                    ++sepPos;
                    ++str1;
                }
                wildcard = 1;
                break;
            } else {
                // no wildcard: check for matching characters
                ++sepPos;
                if( *str2 == 0 || (*str2 != *str1 && *str1 != '?') ) {
                    match = 0;
                    break;
                }
                ++str1;
                ++str2;
            }
        }
    
        if( !wildcard && *str2 != 0 ) // we haven't parsed the complete string
            match = 0;

        if( match ) {
            // store number of path parts in local variable, since content of oscArgs is changed recursively
            // we don't want to copy the whole structure to save (a lot of...) memory
            unsigned char numPathParts = oscArgs->numPathParts;
            // add pointer to path part
            oscArgs->pathPart[numPathParts] = (char *)searchTree->address;
            oscArgs->numPathParts = numPathParts + 1;

            // OR method args of current node to the args to propagate optional parameters
            unsigned combinedMethodArg = methodArg | searchTree->methodArg;

            if( searchTree->oscListener ) {
                searchTree->oscListener->parsedOscPacket(*oscArgs, combinedMethodArg);
            } else if( searchTree->next ) {
                
                // continue search in next hierarchy level
                int status = searchPath((char *)&path[sepPos+1], oscArgs, combinedMethodArg, searchTree->next);
                if( status < 0 )
                    return status;
            }

            // restore number of path parts (which has been changed recursively)
            oscArgs->numPathParts = numPathParts;
        }

        ++searchTree;
    }

    // Table Terminator has been reached - check if method is defined that will be executed on any received OSC element
    // TODO: bring this into MIOS32 as well?
    if( searchTree->oscListener ) {
        oscArgs->numPathParts = 1;
        oscArgs->pathPart[0] = (char *)path;
        searchTree->oscListener->parsedOscPacket(*oscArgs, 0);
    }

    return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Converts an OSC element in readable format.
//
// Usage Example:
// \code
// void parsedOscPacket(const OscHelper::OscArgsT& oscArgs, const const unsigned& methodArg)
// {
//     if( oscString == String::empty )
//         oscString = T("@") + String::formatted(T("%d.%d "), oscArgs.timetag.seconds, oscArgs.timetag.fraction);
//     else
//         oscString += " ";
// 
//     oscString += OscHelper::element2String(oscArgs);
// }
// \endcode
//
// \param[in] oscArgs pointer to OSC argument list as forwarded by parsePacket()
// \return < 0 on errors
/////////////////////////////////////////////////////////////////////////////
String OscHelper::element2String(const OscArgsT& oscArgs)
{
    String str;

    for(int i=0; i<oscArgs.numPathParts; ++i)
        str += T("/") + String(oscArgs.pathPart[i]);

    for(int i=0; i < oscArgs.numArgs; ++i) {
        str += T(" ") + String::formatted(T("%c"), oscArgs.argType[i]);

        switch( oscArgs.argType[i] ) {
        case 'i': // int32
            str += String(getInt(oscArgs.argPtr[i]));
            break;

        case 'f': // float32
            str += String::formatted(T("%05.3f"), getFloat(oscArgs.argPtr[i]));
            break;

        case 's': // string
        case 'S': // alternate string
            str += String(getString(oscArgs.argPtr[i]));
            break;

        case 'b': // blob
            str += String(T("(blob with length ") + String(getWord(oscArgs.argPtr[i])) + ")");
            break;

        case 'h': // int64
            str += String(getLongLong(oscArgs.argPtr[i]));
            break;

        case 't': { // timetag
            OscTimetagT timetag = getTimetag(oscArgs.argPtr[i]);
            str += String::formatted(T("%d.%d"), timetag.seconds, timetag.fraction);
        } break;

        case 'd': // float64 (double)
            str += String::formatted(T("%05.3f"), getDouble(oscArgs.argPtr[i]));
            break;

        case 'c': // ASCII character
            str += String::formatted(T("%c"), getChar(oscArgs.argPtr[i]));
            break;

        case 'r': // 32 bit RGBA color
            str += String::formatted(T("0x%08x"), getWord(oscArgs.argPtr[i]));
            break;

        case 'm': // MIDI message
            str += String::formatted(T("0x%08x"), getMIDI(oscArgs.argPtr[i]));
            break;

        case 'T': // TRUE
        case 'F': // FALSE
        case 'N': // NIL
        case 'I': // Infinitum
        case '[': // beginning of array
        case ']': // end of array
            break;

        default:
            str += T("(unknown)");
        }
    }

    return str;
}
