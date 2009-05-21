// $Id$
//! \defgroup MIOS32_OSC
//!
//! Functions to send and parse OSC packets
//!
//! Based on OSC Spec v1.0 http://opensoundcontrol.org/spec-1_0<BR>
//! (please read for background info and nomenclature)
//!
//! Optimized for embedded systems which provide more code memory (flash) than RAM.
//!
//! Server Part (parsing incoming OSC packets):<BR>
//! MIOS32_OSC_ParsePacket() has to be called with the OSC package content, packet length
//! and a pointer to the "search tree".
//!
//! The search tree allows the parser to match parts of the OSC address path which
//! are separated with the '/' delimiter. At the end of a search tree we will find
//! the OSC method which is called with the arguments that are part of the OSC packet.
//!
//! Usually the search tree is located in flash memory, since it won't change
//! during runtime. Each node of the tree consists of a mios32_osc_search_tree_t
//! structure which defines:
//! <UL>
//!   <LI>the OSC address part or NULL if there are no more address parts/methods in the "OSC container"</LI>
//!   <LI>a link to the next address part or NULL if the leaf has been reached (method reached)</LI>
//!   <LI>if leaf: pointer to function which dispatches the addressed OSC method</LI>
//!   <LI>optional arguments for method (32bit)
//! </UL>
//!
//! So, a node either defines the link to the next node which is part of the
//! address path, or it defines the method which should be called.
//!
//! The optional 32bit argument could be useful for re-using the same function for
//! multiple purposes to save memory.
//! Each node contains such an argument value.<BR>
//! It is ORed for each individual path while scanning the search tree, so that it
//! is possible to allocate certain bitfields for each node (e.g. /sid* will add the SID
//! number to bit [31:30] of the argument value, and /sid?/*/&lt;method&gt; will add the parameter
//! address)
//!
//! OSC allows to use wildcards in the address path like *, ?, [] and {}.<BR>
//! The MIOS32 implementation currently only supports '*' and '?'<BR>
//! Examples: '/sid?/osc/finetune' or '/sid?/osc/fine*' or '/cs/led/*'
//! 
//! While searching through the tree, the appr. functions of all matching methods 
//! will be called with the OSC arguments which are part of the packet (+ the method argument):<BR>
//! \code
//!   s32 osc_method(mios32_osc_args_t *osc_args, u32 method_arg)
//! \endcode
//!
//! All specified OSC arguments are supported, such as following "tags":
//! <UL>
//!   <LI>'i': signed 32bit integer
//!   <LI>'f': IEEE 754 floating point number
//!   <LI>'s': OSC-string
//!   <LI>'b': OSC-blob
//!   <LI>'h': signed 64bit integer
//!   <LI>'t': OSC-timetag
//!   <LI>'d': 64bit ("double") IEEE floating point number
//!   <LI>'S': alternate type represented as an OSC-string (for example, for systems that differentiate "symbols" from "strings")
//!   <LI>'c': an ascii character, sent as 32 bits
//!   <LI>'r': 32 bit RGBA color
//!   <LI>'m': 4 byte MIDI message (can be converted to mios32_midi_package_t)
//!   <LI>'T': TRUE
//!   <LI>'F': FALSE
//!   <LI>'N': NIL
//!   <LI>'I': Infinitum
//!   <LI>'[': Indicates the beginning of an array. The tags following are for data in the Array until a close brace tag is reached.
//!   <LI>']': Indicates the end of an array.
//! </UL>
//!
//! Arguments are stored in mios32_osc_args_t before the OSC method function is called.<BR>
//! This structure contains:
//! <UL>
//!   <LI>the timetag (seconds and fraction)
//!   <LI>number of address parts
//!   <LI>an array of address parts without wildcards (!) - this allows the method to reconstruct the complete path, e.g. to send parameters to different targets
//!   <LI>number of arguments
//!   <LI>an array of argument tags
//!   <LI>pointer to arguments (have to be fetched with MIOS32_OSC_Get*() functions)
//! </UL>
//!
//! An example for a search tree construction and OSC method handling can be found
//! under $MIOS32_PATH/apps/examples/ethernet/osc
//!
//!
//! Client Part (sending OSC packets):
//!
//! Outgoing OSC packets can be "constructed" on-the-fly with MIOS32_OSC_Put* functions.
//!
//! No high-level functions (with large argument lists) are provided to save stack space,
//! and to give the application the highest flexibility. Even unspecified OSC packages
//! can be created if desired.
//!
//! Following example demonstrates the usage. Note that each MIOS32_OSC_Put* function
//! returns the pointer to the next entry, so that items can be appended without
//! the need for additional pointer calculations.
//!
//! \code
//! /////////////////////////////////////////////////////////////////////////////
//! // Send a Button Value
//! // Path: /cs/button/state <button-number> <0 or 1>
//! /////////////////////////////////////////////////////////////////////////////
//! s32 OSC_CLIENT_SendButtonState(mios32_osc_timetag_t timetag, u32 button, u8 pressed)
//! {
//!   // create the OSC packet
//!   u8 packet[128];
//!   u8 *end_ptr = packet;
//!   u8 *insert_len_ptr;
//! 
//!   end_ptr = MIOS32_OSC_PutString(end_ptr, "#bundle");
//!   end_ptr = MIOS32_OSC_PutTimetag(end_ptr, timetag);
//!   insert_len_ptr = end_ptr; // remember this address - we will insert the length later
//!   end_ptr += 4;
//!   end_ptr = MIOS32_OSC_PutString(end_ptr, "/cs/button/state");
//!   end_ptr = MIOS32_OSC_PutString(end_ptr, ",ii");
//!   end_ptr = MIOS32_OSC_PutInt(end_ptr, button);
//!   end_ptr = MIOS32_OSC_PutInt(end_ptr, pressed);
//! 
//!   // now insert the message length
//!   MIOS32_OSC_PutWord(insert_len_ptr, (u32)(end_ptr-insert_len_ptr-4));
//! 
//!   // send packet and exit
//!   return OSC_SERVER_SendPacket(packet, (u32)(end_ptr-packet));
//! }
//! \endcode
//!
//! \{
/* ==========================================================================
 *
 *  Copyright (C) 2009 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

/////////////////////////////////////////////////////////////////////////////
// Include files
/////////////////////////////////////////////////////////////////////////////
#include <mios32.h>
#include <string.h>

// this module can be optionally disabled in a local mios32_config.h file (included from mios32.h)
#if !defined(MIOS32_DONT_USE_OSC)


/////////////////////////////////////////////////////////////////////////////
// Local prototypes
/////////////////////////////////////////////////////////////////////////////

static s32 MIOS32_OSC_SearchElement(u8 *buffer, u32 len, mios32_osc_args_t *osc_args, mios32_osc_search_tree_t *search_tree);
static s32 MIOS32_OSC_SearchPath(char *path, mios32_osc_args_t *osc_args, u32 method_arg, mios32_osc_search_tree_t *search_tree);

static size_t my_strnlen(char *str, size_t max_len);


/////////////////////////////////////////////////////////////////////////////
//! Initializes OSC layer
//! \param[in] mode currently only mode 0 supported
//! \return < 0 if initialisation failed
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_OSC_Init(u32 mode)
{
  if( mode > 0 )
    return -1; // only mode 0 supported yet

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! Gets a word (4 bytes) from buffer
//! \param[in] buffer pointer to OSC message buffer 
//! \return 32bit unsigned integer
/////////////////////////////////////////////////////////////////////////////
u32 MIOS32_OSC_GetWord(u8 *buffer)
{
  // taking endianess into account
  return 
    (((u32)*(buffer+0)) << 24) | (((u32)*(buffer+1)) << 16) |
    (((u32)*(buffer+2)) <<  8) | (((u32)*(buffer+3)) <<  0);
}

/////////////////////////////////////////////////////////////////////////////
//! Puts a word (4 bytes) into buffer
//! \param[in] buffer pointer to OSC message buffer 
//! \param[in] word 32bit word
//! \return buffer pointer behind the inserted entry
/////////////////////////////////////////////////////////////////////////////
u8 *MIOS32_OSC_PutWord(u8 *buffer, u32 word)
{
  *buffer++ = (word >> 24) & 0xff;
  *buffer++ = (word >> 16) & 0xff;
  *buffer++ = (word >>  8) & 0xff;
  *buffer++ = (word >>  0) & 0xff;
  return buffer;
}

/////////////////////////////////////////////////////////////////////////////
//! Gets a timetag (8 bytes) from buffer
//! \param[in] buffer pointer to OSC message buffer 
//! \return timetag (seconds and fraction part)
/////////////////////////////////////////////////////////////////////////////
mios32_osc_timetag_t MIOS32_OSC_GetTimetag(u8 *buffer)
{
  mios32_osc_timetag_t timetag;
  timetag.seconds = MIOS32_OSC_GetWord(buffer);
  timetag.fraction = MIOS32_OSC_GetWord(buffer+4);
  return timetag;
}

/////////////////////////////////////////////////////////////////////////////
//! Puts a timetag (8 bytes) into buffer
//! \param[in] buffer pointer to OSC message buffer 
//! \param[in] timetag the timetag which should be inserted
//! \return buffer pointer behind the inserted entry
/////////////////////////////////////////////////////////////////////////////
u8 *MIOS32_OSC_PutTimetag(u8 *buffer, mios32_osc_timetag_t timetag)
{
  buffer = MIOS32_OSC_PutWord(buffer, timetag.seconds);
  buffer = MIOS32_OSC_PutWord(buffer, timetag.fraction);
  return buffer;
}

/////////////////////////////////////////////////////////////////////////////
//! Gets a word (4 bytes) from buffer and converts it to a 32bit signed integer.
//! \param[in] buffer pointer to OSC message buffer 
//! \return 32bit signed integer
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_OSC_GetInt(u8 *buffer)
{
  return (s32)MIOS32_OSC_GetWord(buffer);
}

/////////////////////////////////////////////////////////////////////////////
//! Puts a 32bit signed integer into buffer
//! \param[in] buffer pointer to OSC message buffer 
//! \param[in] value the integer value which should be inserted
//! \return buffer pointer behind the inserted entry
/////////////////////////////////////////////////////////////////////////////
u8 *MIOS32_OSC_PutInt(u8 *buffer, s32 value)
{
  return MIOS32_OSC_PutWord(buffer, (u32)value);
}

/////////////////////////////////////////////////////////////////////////////
//! Gets a word (4 bytes) from buffer and converts it to a float with 
//! normal precession
//! \param[in] buffer pointer to OSC message buffer 
//! \return float with normal procession
/////////////////////////////////////////////////////////////////////////////
float MIOS32_OSC_GetFloat(u8 *buffer)
{
#if 0
  u32 word = MIOS32_OSC_GetWord(buffer);
  return *(float *)(&word);
#else
  // TK: doesn't work with my gcc installation (i686-apple-darwin9-gcc-4.0.1):
  // float not converted correctly - it works when optimisation is disabled!
  // according to http://gcc.gnu.org/ml/gcc-bugs/2003-02/msg01128.html this isn't a bug...
  // workaround:
  union { u32 word; float f; } converted;
  converted.word = MIOS32_OSC_GetWord(buffer);
  return converted.f;
#endif
}


/////////////////////////////////////////////////////////////////////////////
//! Puts a float with normal precission into buffer
//! \param[in] buffer pointer to OSC message buffer 
//! \param[in] value the float value which should be inserted
//! \return buffer pointer behind the inserted entry
/////////////////////////////////////////////////////////////////////////////
u8 *MIOS32_OSC_PutFloat(u8 *buffer, float value)
{
  union { u32 word; float f; } converted;
  converted.f = value;
  return MIOS32_OSC_PutWord(buffer, converted.word);
}

/////////////////////////////////////////////////////////////////////////////
//! Returns pointer to a string in message buffer
//! \param[in] buffer pointer to OSC message buffer 
//! \return zero-terminated string
/////////////////////////////////////////////////////////////////////////////
char *MIOS32_OSC_GetString(u8 *buffer)
{
  return (char *)buffer; // OSC protocol ensures zero termination (checked in MIOS32_OSC_SearchElement)
}

/////////////////////////////////////////////////////////////////////////////
//! Puts a string into buffer and pads with 0 until word boundary has been reached
//! \param[in] buffer pointer to OSC message buffer 
//! \param[in] value the string which should be inserted
//! \return buffer pointer behind the inserted entry
/////////////////////////////////////////////////////////////////////////////
u8 *MIOS32_OSC_PutString(u8 *buffer, char *str)
{
  u8 *buffer_start = buffer;

  buffer = (u8 *)stpcpy((char *)buffer, str);
  *buffer++ = 0;

  // pad with zeroes until word boundary is reached
  while( (u32)(buffer-buffer_start) % 4 )
    *buffer++ = 0;

  return buffer;
}

/////////////////////////////////////////////////////////////////////////////
//! Returns the length of a Blob
//! \param[in] buffer pointer to OSC message buffer 
//! \return blob length
/////////////////////////////////////////////////////////////////////////////
u32 MIOS32_OSC_GetBlobLength(u8 *buffer)
{
  return *buffer;
}

/////////////////////////////////////////////////////////////////////////////
//! Returns the data part of a Blob
//! \param[in] buffer pointer to OSC message buffer 
//! \return pointer to data part of a Blob
/////////////////////////////////////////////////////////////////////////////
u8 *MIOS32_OSC_GetBlobData(u8 *buffer)
{
  return buffer+4;
}


/////////////////////////////////////////////////////////////////////////////
//! Puts an OSC-Blob into buffer and pads with 0 until word boundary has been reached
//! \param[in] buffer pointer to OSC message buffer 
//! \param[in] data blob data
//! \param[in] len blob size
//! \return buffer pointer behind the inserted entry
/////////////////////////////////////////////////////////////////////////////
u8 *MIOS32_OSC_PutBlob(u8 *buffer, u8 *data, u32 len)
{
  // ensure that length considers word alignment
  u32 aligned_len = (len+3) & 0xfffffffc;

  // add length
  buffer = MIOS32_OSC_PutWord(buffer, aligned_len);

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
//! Gets two words (8 bytes) from buffer and converts them to a 64bit signed integer.
//! \param[in] buffer pointer to OSC message buffer 
//! \return 64bit signed integer
/////////////////////////////////////////////////////////////////////////////
long long MIOS32_OSC_GetLongLong(u8 *buffer)
{
  return ((long long)MIOS32_OSC_GetWord(buffer) << 32) | MIOS32_OSC_GetWord(buffer+4);
}

/////////////////////////////////////////////////////////////////////////////
//! Puts a 64bit signed integer into buffer
//! \param[in] buffer pointer to OSC message buffer 
//! \param[in] value the "long long" value which should be inserted
//! \return buffer pointer behind the inserted entry
/////////////////////////////////////////////////////////////////////////////
u8 *MIOS32_OSC_PutLongLong(u8 *buffer, long long value)
{
  buffer = MIOS32_OSC_PutWord(buffer, (u32)(value >> 32));
  buffer = MIOS32_OSC_PutWord(buffer, (u32)value);
  return buffer;
}


/////////////////////////////////////////////////////////////////////////////
//! Gets two words (8 bytes) from buffer and converts them to a float with 
//! double precession
//! \param[in] buffer pointer to OSC message buffer 
//! \return float with double procession
/////////////////////////////////////////////////////////////////////////////
double MIOS32_OSC_GetDouble(u8 *buffer)
{
#if 0
  long long word = ((long long)MIOS32_OSC_GetWord(buffer) << 32) | MIOS32_OSC_GetWord(buffer+4);
  return *(double *)(&word);
#else
  // TK: doesn't work with my gcc installation (i686-apple-darwin9-gcc-4.0.1):
  // float not converted correctly - it works when optimisation is disabled!
  // according to http://gcc.gnu.org/ml/gcc-bugs/2003-02/msg01128.html this isn't a bug...
  // workaround:
  union { long long word; double d; } converted;
  converted.word = MIOS32_OSC_GetLongLong(buffer);
  return converted.d;
#endif
}

/////////////////////////////////////////////////////////////////////////////
//! Puts a float with double precission into buffer
//! \param[in] buffer pointer to OSC message buffer 
//! \param[in] value the double value which should be inserted
//! \return buffer pointer behind the inserted entry
/////////////////////////////////////////////////////////////////////////////
u8 *MIOS32_OSC_PutDouble(u8 *buffer, double value)
{
  union { long long word; double d; } converted;
  converted.d = value;
  return MIOS32_OSC_PutLongLong(buffer, converted.word);
}


/////////////////////////////////////////////////////////////////////////////
//! Returns a character
//! \param[in] buffer pointer to OSC message buffer 
//! \return a single character
/////////////////////////////////////////////////////////////////////////////
char MIOS32_OSC_GetChar(u8 *buffer)
{
  return *buffer; // just for completeness..
}

/////////////////////////////////////////////////////////////////////////////
//! Puts a character into buffer and pads with 3 zeros (word aligned)
//! \param[in] buffer pointer to OSC message buffer 
//! \param[in] c the character which should be inserted
//! \return buffer pointer behind the inserted entry
/////////////////////////////////////////////////////////////////////////////
u8 *MIOS32_OSC_PutChar(u8 *buffer, char c)
{
  return MIOS32_OSC_PutWord(buffer, (u32)c);
}



/////////////////////////////////////////////////////////////////////////////
//! Returns a MIDI package
//! \param[in] buffer pointer to OSC message buffer 
//! \return a MIOS32 compliant MIDI package
/////////////////////////////////////////////////////////////////////////////
mios32_midi_package_t MIOS32_OSC_GetMIDI(u8 *buffer)
{
  mios32_midi_package_t p;

  // I find it great that the OSC spec matches with the common MIOS32 package format... :-)
  p.cable = *buffer; // only first 4 bits taken
  p.type = *(buffer+1) >> 4;
  p.evnt0 = *(buffer+1);
  p.evnt1 = *(buffer+2);
  p.evnt2 = *(buffer+3);
  return p;
}

/////////////////////////////////////////////////////////////////////////////
//! Puts a MIDI package into buffer
//! \param[in] buffer pointer to OSC message buffer 
//! \param[in] p the MIDI package which should be inserted
//! \return buffer pointer behind the inserted entry
/////////////////////////////////////////////////////////////////////////////
u8 *MIOS32_OSC_PutMIDI(u8 *buffer, mios32_midi_package_t p)
{
  u32 word = (p.cable << 24) | (p.evnt0 << 16) | (p.evnt1 << 8) | (p.evnt2 << 0);
  return MIOS32_OSC_PutWord(buffer, word);
}


/////////////////////////////////////////////////////////////////////////////
//! Parses an incoming OSC packet and calls OSC methods defined in search_tree
//! on matching addresses
//! \param[in] packet pointer to OSC packet
//! \param[in] len length of packet
//! \param[in] search_tree a tree which defines address parts and methods to be called
//! \return 0 if packet has been parsed w/o errors
//! \return -1 if packet format invalid
//! \return -2 if the packet contains an OSC element with invalid format
//! \return -3 if the packet contains an OSC element with an unsupported format
//! returns -4 if MIOS32_OSC_MAX_PATH_PARTS has been exceeded
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_OSC_ParsePacket(u8 *packet, u32 len, mios32_osc_search_tree_t *search_tree)
{
  // store osc arguments (and more...) into osc_args variable
  mios32_osc_args_t osc_args;

  // check if we got a bundle
  if( strncmp((char *)packet, "#bundle", len) == 0 ) {
    u32 pos = 8;

    // we expect at least 8 bytes for the timetag
    if( (pos+8) > len )
      return -1; // invalid format


    // get timetag
    osc_args.timetag = MIOS32_OSC_GetTimetag((u8 *)packet+pos);
    pos += 8;

    // parse elements
    while( (pos+4) <= len ) {
      // get element size
      u32 elem_size = MIOS32_OSC_GetWord((u8 *)(packet+pos));
      pos += 4;

      // invalid packet if elem_size exceeds packet length
      if( (pos+elem_size) > len )
	return -1; // invalid packet

      // parse element if size > 0
      if( elem_size ) {
	s32 status = MIOS32_OSC_SearchElement((u8 *)(packet+pos), elem_size, &osc_args, search_tree);
	if( status < 0 )
	  return status;
      }

      // switch to next element
      pos += elem_size;
    }
    
  } else {
    // no timetag
    osc_args.timetag.seconds = 0;
    osc_args.timetag.fraction = 0;

    // get element size
    u32 elem_size = MIOS32_OSC_GetWord(packet);

    // invalid packet if elem_size exceeds packet length
    if( elem_size > len )
      return -1; // invalid packet

    // parse element if size > 0
    if( elem_size ) {
      s32 status = MIOS32_OSC_SearchElement((u8 *)(packet+4), elem_size, &osc_args, search_tree);
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
static s32 MIOS32_OSC_SearchElement(u8 *buffer, u32 len, mios32_osc_args_t *osc_args, mios32_osc_search_tree_t *search_tree)
{
  // exit immediately if element is empty
  if( !len )
    return 0;

  // according to OSC spec, the path could be ommitted, and the element could start with the argument list
  // I don't know how to handle this correctly, but we should at least exit in this case
  u8 *path = buffer;
  if( *path == ',' )
    return -3; // unsupported element format

  // path: determine string length
  size_t path_len = my_strnlen((char *)path, len);

  // check for valid string
  if( path_len < 2 || path[path_len] != 0 ) // expect at least two characters, e.g. "/*"
    return -2; // invalid element format

  // path must start with '/'
  if( *path != '/' )
    return -2; // invalid element format

  // tags are starting at word aligned offset
  // add +1 to path_len, since \0 terminator is counted as well
  size_t tags_pos = (path_len+1 + 3) & 0xfffffc;

  // check that tags are at valid position
  if( tags_pos >= len )
    return -2; // invalid element format

  // tags: determine string length
  u8 *tags = (u8 *)(buffer + tags_pos);
  size_t tags_len = my_strnlen((char *)tags, len-tags_pos);

  // check for valid string
  if( tags_len == 0 || tags[tags_len] != 0 )
    return -2; // invalid element format

  // check that tags are starting with comma
  if( *tags != ',' )
    return -2; // invalid element format

  // number of arguments:
  u32 num_args = tags_len - 1;

  // limit by max number of args which can be stored in osc_args structure
  if( num_args > MIOS32_OSC_MAX_ARGS )
    num_args = MIOS32_OSC_MAX_ARGS;

  // arguments are starting at word aligned offset
  // add +1 to tags_len, since \0 terminator is counted as well
  size_t arg_pos = (tags_pos + tags_len+1 + 3) & 0xfffffc;

  // parse arguments
  osc_args->num_args = 0;
  u32 arg;
  for(arg=0; arg<num_args; ++arg) {
    // check that argument is at valid position
    if( arg_pos >= len )
      return -2; // invalid element format

    // store type and pointer to argument
    osc_args->arg_type[osc_args->num_args] = tags[arg+1];
    osc_args->arg_ptr[osc_args->num_args] = (u8 *)(buffer + arg_pos);

    // branch depending on argument tag
    u8 known_arg = 0;
    switch( tags[arg+1] ) {
      case 'i': // int32
      case 'f': // float32
      case 'c': // ASCII character
      case 'r': // 32bit RGBA color
      case 'm': // 4 byte MIDI message
	known_arg = 1;
	arg_pos += 4;
	break;

      case 's':   // OSC-string
      case 'S': { // OSC alternate string
	known_arg = 1;
	char *str = (char *)osc_args->arg_ptr[osc_args->num_args];
	size_t str_len = my_strnlen(str, len-arg_pos);
	// check for valid string
	if( str_len == 0 || str[str_len] != 0 )
	  return -2; // invalid element format
	// next argument at word aligned offset
	// add +1 to str_len, since \0 terminator is counted as well
	arg_pos = (arg_pos + str_len+1 + 3) & 0xfffffc;
      } break;

      case 'b': { // OSC-blob
	known_arg = 1;
	u32 blob_len = MIOS32_OSC_GetBlobLength((u8 *)(buffer + arg_pos));
	// next argument at word aligned offset
	arg_pos = (arg_pos + 4 + blob_len + 3) & 0xfffffc;
      } break;

      case 'h': // long64
      case 't': // OSC timetag
      case 'd': // float64 (double)
	known_arg = 1;
	arg_pos += 8;
	break;

      case 'T': // TRUE
      case 'F': // FALSE
      case 'N': // NIL
      case 'I': // Infinitum
      case '[': // Begin of Array
      case ']': // End of Array
	known_arg = 1;
	break;

    }

    // according to OSC V1.0 spec, nonstandard arguments should be discarded (don't report error)
    // since we don't know the position to the next argument, we have to stop parsing here!
    if( known_arg )
      ++osc_args->num_args;
    else
      break;
  }

  // finally parse for elements which are matching the OSC address
  osc_args->num_path_parts = 0;
  return MIOS32_OSC_SearchPath((char *)&path[1], osc_args, 0x00000000, search_tree);
}


/////////////////////////////////////////////////////////////////////////////
// Internal function:
// searches in search_tree for matching OSC addresses
// returns -4 if MIOS32_OSC_MAX_PATH_PARTS has been exceeded
/////////////////////////////////////////////////////////////////////////////
static s32 MIOS32_OSC_SearchPath(char *path, mios32_osc_args_t *osc_args, u32 method_arg, mios32_osc_search_tree_t *search_tree)
{
  if( osc_args->num_path_parts >= MIOS32_OSC_MAX_PATH_PARTS )
    return -4; // maximum number of path parts exceeded

  while( search_tree->address != NULL ) {
    // compare OSC address with name of tree item
    u8 match = 1;
    u8 wildcard = 0;

    char *str1 = path;
    char *str2 = (char *)search_tree->address;
    size_t sep_pos = 0;

    while( *str1 != 0 && *str1 != '/' ) {
      if( *str1 == '*' ) {
	// '*' wildcard: continue to end of address part
	while( *str1 != 0 && *str1 != '/' ) {
	  ++sep_pos;
	  ++str1;
	}
	wildcard = 1;
	break;
      } else {
	// no wildcard: check for matching characters
	++sep_pos;
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
      // store number of path parts in local variable, since content of osc_args is changed recursively
      // we don't want to copy the whole structure to save (a lot of...) memory
      u8 num_path_parts = osc_args->num_path_parts;
      // add pointer to path part
      osc_args->path_part[num_path_parts] = (char *)search_tree->address;
      osc_args->num_path_parts = num_path_parts + 1;

      // OR method args of current node to the args to propagate optional parameters
      u32 combined_method_arg = method_arg | search_tree->method_arg;

      if( search_tree->osc_method ) {
	s32 (*osc_method)(mios32_osc_args_t *osc_args, u32 method_arg) = search_tree->osc_method;
	osc_method(osc_args, combined_method_arg);
      } else if( search_tree->next ) {

	// continue search in next hierarchy level
	s32 status = MIOS32_OSC_SearchPath((char *)&path[sep_pos+1], osc_args, combined_method_arg, search_tree->next);
	if( status < 0 )
	  return status;
      }

      // restore number of path parts (which has been changed recursively)
      osc_args->num_path_parts = num_path_parts;
    }

    ++search_tree;
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! Sends the argument list of a method to the debug terminal.
//!
//! By default, the message is sent via MIDI to the MIOS Studio terminal.
//!
//! Optionally another output function can be used (e.g. printf in emulation)
//! by overruling MIOS32_OSC_DEBUG_MSG in mios32_config.h
//!
//! Usage Example:
//! \code
//! static s32 OSC_SERVER_MyOSCMethod(mios32_osc_args_t *osc_args, u32 method_arg)
//! {
//!   MIOS32_OSC_SendDebugMessage(osc_args, method_arg);
//!
//!   return 0; // no error
//! }
//! \endcode
//!
//! \param[in] osc_args pointer to OSC argument list as forwarded by MIOS32_OSC_ParsePacket()
//! \param[in] method_args 32bit value as forwarded by MIOS32_OSC_ParsePacket()
//! \return < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_OSC_SendDebugMessage(mios32_osc_args_t *osc_args, u32 method_arg)
{
  // for debugging: merge path parts to complete path
  char path[128]; // should be enough?
  int i;
  char *path_ptr = path;

  for(i=0; i<osc_args->num_path_parts; ++i) {
    path_ptr = stpcpy(path_ptr, "/");
    path_ptr = stpcpy(path_ptr, osc_args->path_part[i]);
  }

  MIOS32_OSC_DEBUG_MSG("[%s] timetag %d.%d (%d args), Method Arg: 0x%08x\n", 
		       path,
		       osc_args->timetag.seconds,
		       osc_args->timetag.fraction,
		       osc_args->num_args,
		       method_arg);

  for(i=0; i < osc_args->num_args; ++i) {
    switch( osc_args->arg_type[i] ) {
      case 'i': // int32
	MIOS32_OSC_DEBUG_MSG("[%s] %d: %d (int32)\n", path, i, MIOS32_OSC_GetInt(osc_args->arg_ptr[i]));
	  break;

        case 'f': { // float32
	  float value = MIOS32_OSC_GetFloat(osc_args->arg_ptr[i]);
	  // note: the simple printf function doesn't support %f
	  MIOS32_OSC_DEBUG_MSG("[%s] %d: %d.%03d (float32)\n", path, i, 
				       (int)value, (int)((value*1000))%1000);
	} break;

        case 's': // string
        case 'S': // alternate string
	  MIOS32_OSC_DEBUG_MSG("[%s] %d: '%s'\n", path, i, MIOS32_OSC_GetString(osc_args->arg_ptr[i]));
	  break;

        case 'b': // blob
	  MIOS32_OSC_DEBUG_MSG("[%s] %d: blob with length %u\n", path, i, MIOS32_OSC_GetWord(osc_args->arg_ptr[i]));
	  break;

        case 'h': { // int64
	  long long value = MIOS32_OSC_GetLongLong(osc_args->arg_ptr[i]);
	  MIOS32_OSC_DEBUG_MSG("[%s] %d: 0x%08x%08x (int64)\n", path, i, 
				       (u32)(value >> 32), (u32)value);
	} break;

        case 't': { // timetag
	  mios32_osc_timetag_t timetag = MIOS32_OSC_GetTimetag(osc_args->arg_ptr[i]);
	  MIOS32_OSC_DEBUG_MSG("[%s] %d: seconds %u fraction %u\n", path, i, 
				       timetag.seconds, timetag.fraction);
	} break;

        case 'd': { // float64 (double)
	  double value = MIOS32_OSC_GetDouble(osc_args->arg_ptr[i]);
	  // note: the simple printf function doesn't support %f
	  MIOS32_OSC_DEBUG_MSG("[%s] %d: %d.%03d (float64)\n", path, i, 
				       (int)value, (int)((value*1000))%1000);
	} break;

        case 'c': // ASCII character
	  MIOS32_OSC_DEBUG_MSG("[%s] %d: char %c\n", path, i, MIOS32_OSC_GetChar(osc_args->arg_ptr[i]));
	  break;

        case 'r': // 32 bit RGBA color
	  MIOS32_OSC_DEBUG_MSG("[%s] %d: %08X (RGBA color)\n", path, i, MIOS32_OSC_GetWord(osc_args->arg_ptr[i]));
	  break;

        case 'm': // MIDI message
	  MIOS32_OSC_DEBUG_MSG("[%s] %d: %08X (MIDI)\n", path, i, MIOS32_OSC_GetMIDI(osc_args->arg_ptr[i]).ALL);
	  break;

        case 'T': // TRUE
	  MIOS32_OSC_DEBUG_MSG("[%s] %d: TRUE\n", path, i);
	  break;

        case 'F': // FALSE
	  MIOS32_OSC_DEBUG_MSG("[%s] %d: FALSE\n", path, i);
	  break;

        case 'N': // NIL
	  MIOS32_OSC_DEBUG_MSG("[%s] %d: NIL\n", path, i);
	  break;

        case 'I': // Infinitum
	  MIOS32_OSC_DEBUG_MSG("[%s] %d: Infinitum\n", path, i);
	  break;

        case '[': // beginning of array
	  MIOS32_OSC_DEBUG_MSG("[%s] %d: [ (beginning of array)\n", path, i);
	  break;

        case ']': // end of array
	  MIOS32_OSC_DEBUG_MSG("[%s] %d: [ (end of array)\n", path, i);
	  break;

        default:
	  MIOS32_OSC_DEBUG_MSG("[%s] %d: unknown arg type '%c'\n", path, i, osc_args->arg_type[i]);
	  break;
      }
  }

  return 0; // no error
}



//
// strnlen() not available for all libc's, therefore we use a local solution here
static size_t my_strnlen(char *str, size_t max_len)
{
  size_t len = 0;

  while( *str++ && (len < max_len) )
    ++len;

  return len;
}


#endif /* MIOS32_DONT_USE_OSC */
