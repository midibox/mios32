
/*
 * Header for XML file access
 *
 * ==========================================================================
 *
 *  Copyright (C) 2009 Phil Taylor (phil@taylor.org.uk)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _XML_H
#define _XML_H

/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////

/* basicxmlnode: simple xml node memory representation */
struct basicxmlnode
{
  char * tag; /* always non-NULL */
  char * text; /* body + all whitespace, always non-NULL */
  char * * attrs; /* array of strings, NULL marks end */
  char * * values; /* array of strings, NULL marks end */
  struct basicxmlnode * * children; /* similar */
  int * childreni; /* children positions in text */
};

/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern struct basicxmlnode * readbasicxmlnode( FILE * fpi );
extern void printbasicxmlnode( struct basicxmlnode * node );
extern void printbasicxmlnodetagnames( struct basicxmlnode * node );
extern void deletebasicxmlnode( struct basicxmlnode * node );

/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////

#endif /* _FS_H */
