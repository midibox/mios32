// $Id$
/*
 * Header file for OSC functions
 *
 * ==========================================================================
 *
 *  Copyright (C) 2009 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _MIOS32_OSC_H
#define _MIOS32_OSC_H


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
// Global Types
/////////////////////////////////////////////////////////////////////////////

typedef struct mios32_osc_search_tree_t {
  const char                     *address;    // OSC address part
  struct mios32_osc_search_tree_t *next;      // link to the next address part or NULL if the leaf has been reached (method reached)
  void                           *osc_method; // if leaf: pointer to function which dispatches the addressed OSC method
  u32                            method_arg;  // optional argument for methods (32bit)
} mios32_osc_search_tree_t;


typedef struct {
  u32 seconds;
  u32 fraction;
} mios32_osc_timetag_t;

typedef struct {
  mios32_osc_timetag_t timetag; // the timetag (seconds and fraction)

  u8                   num_path_parts; // number of address parts
  const char           *path_part[MIOS32_OSC_MAX_PATH_PARTS]; // an array of address paths without wildcards (!) - this allows the method to reconstruct the complete path, e.g. to send parameters to different targets

  u8                   num_args; // number of arguments
  char                 arg_type[MIOS32_OSC_MAX_ARGS]; // array of argument tags
  u8                   *arg_ptr[MIOS32_OSC_MAX_ARGS]; // pointer to arguments (have to be fetched with MIOS32_OSC_Get*() functions)
} mios32_osc_args_t;


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 MIOS32_OSC_Init(u32 mode);

extern u32 MIOS32_OSC_GetWord(u8 *buffer);
extern mios32_osc_timetag_t MIOS32_OSC_GetTimetag(u8 *buffer);
extern s32 MIOS32_OSC_GetInt(u8 *buffer);
extern float MIOS32_OSC_GetFloat(u8 *buffer);
extern char *MIOS32_OSC_GetString(u8 *buffer);
extern u32 MIOS32_OSC_GetBlobLength(u8 *buffer);
extern u8 *MIOS32_OSC_GetBlobData(u8 *buffer);
extern double MIOS32_OSC_GetDouble(u8 *buffer);
extern long long MIOS32_OSC_GetLongLong(u8 *buffer);
extern char MIOS32_OSC_GetChar(u8 *buffer);
extern mios32_midi_package_t MIOS32_OSC_GetMIDI(u8 *buffer);

extern s32 MIOS32_OSC_ParsePacket(u8 *packet, u32 len, mios32_osc_search_tree_t *search_tree);


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////


#endif /* _MIOS32_OSC_H */
