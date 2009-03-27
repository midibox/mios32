/* $Id$ */
/*
vX32 pre-alpha
not for any use whatsoever
copyright stryd_one
bite me corp 2008

big props to nILS for being my fourth eye and TK for obvious reasons
stay tuned for UI prototyping courtesy of lucem!

*/


#ifndef _MODULES_H
#define _MODULES_H



/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////

#define MAX_MODULETYPES 4                                                       // number of module types. maximum 254

#define MOD_MODULETYPE_SCLK 0
#define MOD_MODULETYPE_SEQ 1
#define MOD_MODULETYPE_MIDIOUT 2
#define MOD_MODULETYPE_SXH 3


                                                                                // don't change defines below here
#define DEAD_MODULETYPE (MAX_MODULETYPES+1)                                     // don't change



/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////

typedef const struct {                                                                // port type and name
    const unsigned char porttype;
    const char portname[8];
} mod_portdata_t;


typedef const struct {                                                                // module type data like functions for processing etc
    void (*init_fn) (unsigned char nodeID);
    void (*proc_fn) (unsigned char nodeID);
    void (*tick_fn) (unsigned char nodeID);
    void (*uninit_fn) (unsigned char nodeID);
    
    const unsigned char ports;
    const unsigned char privvars;
    
    const unsigned char buffers;
    const unsigned char buffertype;
    
    const mod_portdata_t *porttypes;
    const mod_portdata_t *privvartypes;
    
    const char name[MODULE_NAME_STRING_LENGTH];
} mod_moduledata_t;



/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////

mod_moduledata_t *mod_ModuleData_Type[MAX_MODULETYPES];                         // array of module data



extern void (*mod_Init_Type[MAX_MODULETYPES]) (unsigned char nodeID);           // array of module init functions

extern void (*mod_Process_Type[MAX_MODULETYPES]) (unsigned char nodeID);        // array of module processing functions

extern void (*mod_Tick_Type[MAX_MODULETYPES]) (unsigned char nodeID);           // array of module ticking functions

extern void (*mod_UnInit_Type[MAX_MODULETYPES]) (unsigned char nodeID);         // array of module init functions


extern unsigned char mod_Ports[MAX_MODULETYPES];                                // array of sizes in bytes of ports per module type

extern unsigned char mod_PrivVars[MAX_MODULETYPES];                             // array of sizes in bytes of private vars per module type


extern mod_portdata_t *mod_PortTypes[MAX_MODULETYPES];                          // array of port doto per module type

extern mod_portdata_t *mod_PrivVarTypes[MAX_MODULETYPES];                       // array of port doto per module type


extern unsigned char mod_ReProcess;                                             // counts number of modules which have requested reprocessing (eg after distributing a reset timestamp)



/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern void Mod_Init_ModuleData(void);                                          // function for initialising module data on startup


extern void Mod_PreProcess(unsigned char startnodeID);                          // does preprocessing for all modules


void Mod_Process(unsigned char nodeID);                                         // does preprocessing for one module

void Mod_Tick_Node(unsigned char nodeID);                                       // does preprocessing for one module

void Mod_Propagate(unsigned char nodeID);                                       // pushes data out from ports on a module to all modules it is patched into



extern void Mod_Tick(void);                                                     // check for ticks on modules


extern void Mod_SetNextTick                                                     // set the node[nodeID].nexttick value and handle it right. never write that value directly.
                            (unsigned char nodeID, u32 timestamp, 
                            u32 ( *mod_reset_type)(unsigned char resetnodeID));

extern void Mod_TickPriority(unsigned char nodeID);                             // sets node[nodeID].downstream tick for this node and all nodes that patch in to it

void Mod_UnInit(unsigned char nodeID);                                          // ununitialised a module according to it's module type


void Mod_Init_Graph(unsigned char nodeID, unsigned char moduletype);            // mandatory graph inits, gets called automatically

void Mod_UnInit_Graph(unsigned char nodeID);                                    // mandatory graph uninits, gets called automatically



/////////////////////////////////////////////////////////////////////////////
// Includes
/////////////////////////////////////////////////////////////////////////////

                                                                                // includes for required functions
#include "mod_send.h"

#include "mod_xlate.h"

                                                                                // includes for modules
#include "vxmodules/mod_sclk.h"
#include "vxmodules/mod_seq.h"
#include "vxmodules/mod_midiout.h"
#include "vxmodules/mod_sxh.h"
                                                                                // modules just need to be added here and to the mod_ModuleData_Type array to include them in the app

#endif /* _MODULES_H */
