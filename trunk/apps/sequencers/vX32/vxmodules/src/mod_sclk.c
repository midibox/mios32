/* $Id$ */
/*
vX32 pre-alpha
not for any use whatsoever
copyright stryd_one
bite me corp 2008

big props to nILS for being my fourth eye and TK for obvious reasons
stay tuned for UI prototyping courtesy of lucem!

*/


/////////////////////////////////////////////////////////////////////////////
// Include files
/////////////////////////////////////////////////////////////////////////////

#include <FreeRTOS.h>
#include <portmacro.h>

#include <mios32.h>

#include "app.h"
#include "graph.h"
#include "mclock.h"
#include "modules.h"
#include "patterns.h"
#include "utils.h"
#include "ui.h"

#include <seq_midi_out.h>



/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

mod_moduledata_t mod_SClk_ModuleData = {
    &Mod_Init_SClk,                                                             // name of functions to initialise
    &Mod_Proc_SClk,                                                             // name of functions to process
    &Mod_Tick_SClk,                                                             // name of functions to tick
    &Mod_UnInit_SClk,                                                           // name of functions to uninitialise
    
    mod_sclk_ports,                                                             // size of char array to allocate
    mod_sclk_privvars,                                                          // size of char array to allocate
    
    MOD_SCLK_BUFFERS,                                                           // number of buffers to allocate
    MOD_SEND_TYPE_DUMMY,                                                        // type of those buffers (references the function to send them, and their size)
    
    mod_SClk_PortTypes,                                                         // pointer to port type lists
    mod_SClk_PrivVarTypes,                                                      // pointer to private var type lists
    "SubClock",                                                                 // 8 character name
};



mod_portdata_t mod_SClk_PortTypes[mod_sclk_ports] = {
    {MOD_PORTTYPE_VALUE, "Status  "},                                           //status
    {MOD_PORTTYPE_VALUE, "Numrator"},                                           //numerator
    {MOD_PORTTYPE_VALUE, "Denomntr"},                                           //denominator
    {DEAD_PORTTYPE, "NoPatch!"},                                                 //padding
    {MOD_PORTTYPE_TIMESTAMP, "LngthOut"},                                       //cycle length
    {DEAD_PORTTYPE, "NoPatch!"},
    {DEAD_PORTTYPE, "NoPatch!"},
    {DEAD_PORTTYPE, "NoPatch!"},
    {MOD_PORTTYPE_TIMESTAMP, "ClockOut"},                                       //next tick
    {DEAD_PORTTYPE, "NoPatch!"},
    {DEAD_PORTTYPE, "NoPatch!"},
    {DEAD_PORTTYPE, "NoPatch!"},
    {MOD_PORTTYPE_TIMESTAMP, "Clock In"},                                       //sync tick
    {DEAD_PORTTYPE, "NoPatch!"},
    {DEAD_PORTTYPE, "NoPatch!"},
    {DEAD_PORTTYPE, "NoPatch!"},
};

mod_portdata_t mod_SClk_PrivVarTypes[mod_sclk_privvars] = {
    {DEAD_PORTTYPE, "NoPatch!"},
    {DEAD_PORTTYPE, "NoPatch!"},
    {DEAD_PORTTYPE, "NoPatch!"},
    {DEAD_PORTTYPE, "NoPatch!"},
    {DEAD_PORTTYPE, "NoPatch!"},
    {DEAD_PORTTYPE, "NoPatch!"},
    {DEAD_PORTTYPE, "NoPatch!"},                                                //dummies until i do something real
    {DEAD_PORTTYPE, "NoPatch!"},
    {DEAD_PORTTYPE, "NoPatch!"},
    {DEAD_PORTTYPE, "NoPatch!"},
    {DEAD_PORTTYPE, "NoPatch!"},
    {DEAD_PORTTYPE, "NoPatch!"},
    {DEAD_PORTTYPE, "NoPatch!"},
    {DEAD_PORTTYPE, "NoPatch!"},
    {DEAD_PORTTYPE, "NoPatch!"},
    {DEAD_PORTTYPE, "NoPatch!"},
};

//port pointers for use in functions, copy and use as requ'd
// FIXME these shoud really be signed and used as relative numbers, that's for later on
    /*
    u8 *portstatus;
    portstatus = (u8 *) &(node[nodeID].ports[MOD_SCLK_PORT_STATUS]);
    u8 *portnumerator;
    portnumerator = (u8 *) &(node[nodeID].ports[MOD_SCLK_PORT_NUMERATOR]);
    u8 *portdenominator;
    portdenominator = (u8 *) &(node[nodeID].ports[MOD_SCLK_PORT_DENOMINATOR]);
    u32 *portcyclelen;
    portcyclelen = (u32 *) &(node[nodeID].ports[MOD_SCLK_PORT_CYCLELEN]);
    u32 *portnexttick;
    portnexttick = (u32 *) &(node[nodeID].ports[MOD_SCLK_PORT_NEXTTICK]);
    u32 *portsynctick;
    portsynctick = (u32 *) &(node[nodeID].ports[MOD_SCLK_PORT_SYNCTICK]);

    u16 *privquotient;
    privquotient = (u16 *) &(node[nodeID].privvars[MOD_SCLK_PRIV_QUOTIENT]);
    u16 *privmodulus;
    privmodulus = (u16 *) &(node[nodeID].privvars[MOD_SCLK_PRIV_MODULUS]);
    s16 *privmodcounter;
    privmodcounter = (s16 *) &(node[nodeID].privvars[MOD_SCLK_PRIV_MODCOUNTER]);
    u16 *privcountdown;
    privcountdown = (u16 *) &(node[nodeID].privvars[MOD_SCLK_PRIV_COUNTDOWN]);
    u8 *privstatus;
    privstatus = (u8 *) &(node[nodeID].ports[MOD_SCLK_PRIV_STATUS]);
    u32 *privlastsync;
    privlastsync = (u32 *) &(node[nodeID].privvars[MOD_SCLK_PRIV_LASTSYNC]);
    */


/////////////////////////////////////////////////////////////////////////////
// local prototypes
/////////////////////////////////////////////////////////////////////////////

void Mod_Init_SClk(unsigned char nodeID) {                                      // initialize a subclock module
    u8 *portstatus;
    portstatus = (u8 *) &(node[nodeID].ports[MOD_SCLK_PORT_STATUS]);
    u8 *portnumerator;
    portnumerator = (u8 *) &(node[nodeID].ports[MOD_SCLK_PORT_NUMERATOR]);
    u8 *portdenominator;
    portdenominator = (u8 *) &(node[nodeID].ports[MOD_SCLK_PORT_DENOMINATOR]);
    u32 *portnexttick;
    portnexttick = (u32 *) &(node[nodeID].ports[MOD_SCLK_PORT_NEXTTICK]);
    u32 *portsynctick;
    portsynctick = (u32 *) &(node[nodeID].ports[MOD_SCLK_PORT_SYNCTICK]);
    
    
     // FIXME Should it be initialised as already active and synced to master ? probably not... Anyway it's an alpha version ffs ;)
    util_setbit(*portstatus, MOD_SCLK_PORT_STATUS_BIT_ACTIVE);
    util_setbit(*portstatus, MOD_SCLK_PORT_STATUS_BIT_SYNC);
    util_clrbit(*portstatus, MOD_SCLK_PORT_STATUS_BIT_HARDSYNC);
    
    *portnumerator = 4; // 1/4 notes
    *portdenominator = 1; // 1/4 notes
    
    
    *portsynctick = DEAD_TIMESTAMP;
    *portnexttick = DEAD_TIMESTAMP;
    
    Mod_SClk_ResetCounters(nodeID);
    Mod_SetNextTick(nodeID, Mod_SClk_GetNextTick(nodeID), &Mod_Reset_SClk);
    
}

void Mod_Proc_SClk(unsigned char nodeID) {                                      // do stuff with inputs and push to the outputs 
/*
    u32 *portnexttick;
    portnexttick = (u32 *) &(node[nodeID].ports[MOD_SCLK_PORT_NEXTTICK]);
    
    Mod_SetNextTick(nodeID, Mod_SClk_GetNextTick(nodeID), &Mod_Reset_SClk);
    
    *portnexttick = node[nodeID].nexttick;
*/
    Mod_Tick_SClk(nodeID);
}

void Mod_Tick_SClk(unsigned char nodeID) {                                      // set a new timestamp
    u32 *portnexttick;
    portnexttick = (u32 *) &(node[nodeID].ports[MOD_SCLK_PORT_NEXTTICK]);
    
    Mod_SetNextTick(nodeID, Mod_SClk_GetNextTick(nodeID), &Mod_Reset_SClk);
    
    *portnexttick = node[nodeID].nexttick;
    
}

void Mod_UnInit_SClk(unsigned char nodeID) {                                    // uninitialize a sample and hold module
}


u32 Mod_Reset_SClk(unsigned char nodeID) {                                      // reset a seq module
    u32 *portsynctick;
    portsynctick = (u32 *) &(node[nodeID].ports[MOD_SCLK_PORT_SYNCTICK]);
    
    Mod_SClk_ResetCounters(nodeID);
    
    if (mClock.status.reset_req > 0) {
        return mod_Tick_Timestamp;
        
    } else {
        return Mod_SClk_GetNextTick(nodeID);
    }
    
}


u32 Mod_SClk_GetNextTick(unsigned char nodeID) {

    s8 *portstatus;
    portstatus = (s8 *) &(node[nodeID].ports[MOD_SCLK_PORT_STATUS]);
    u8 *portnumerator;
    portnumerator = (u8 *) &(node[nodeID].ports[MOD_SCLK_PORT_NUMERATOR]);
    u8 *portdenominator;
    portdenominator = (u8 *) &(node[nodeID].ports[MOD_SCLK_PORT_DENOMINATOR]);
    u32 *portcyclelen;
    portcyclelen = (u32 *) &(node[nodeID].ports[MOD_SCLK_PORT_CYCLELEN]);
    u32 *portnexttick;
    portnexttick = (u32 *) &(node[nodeID].ports[MOD_SCLK_PORT_NEXTTICK]);
    u32 *portsynctick;
    portsynctick = (u32 *) &(node[nodeID].ports[MOD_SCLK_PORT_SYNCTICK]);

    u16 *privquotient;
    privquotient = (u16 *) &(node[nodeID].privvars[MOD_SCLK_PRIV_QUOTIENT]);
    u16 *privmodulus;
    privmodulus = (u16 *) &(node[nodeID].privvars[MOD_SCLK_PRIV_MODULUS]);
    s16 *privmodcounter;
    privmodcounter = (s16 *) &(node[nodeID].privvars[MOD_SCLK_PRIV_MODCOUNTER]);
    u16 *privcountdown;
    privcountdown = (u16 *) &(node[nodeID].privvars[MOD_SCLK_PRIV_COUNTDOWN]);
    u8 *privstatus;
    privstatus = (u8 *) &(node[nodeID].ports[MOD_SCLK_PRIV_STATUS]);
    u32 *privlastsync;
    privlastsync = (u32 *) &(node[nodeID].privvars[MOD_SCLK_PRIV_LASTSYNC]);


    *portcyclelen = ((mClock.cyclelen) * (*portdenominator)); // heh once upon a time this was processor intensive. now it's three cycles. LOL.
    *privquotient = ((*portcyclelen) / (*portnumerator));
    *privmodulus = ((*portcyclelen) % (*portnumerator));

//FIXME TESTING
#if vX_DEBUG_VERBOSE_LEVEL >= 1
        DEBUG_MSG("[vX][sclk] cyclelen %u\n", *portcyclelen);
        DEBUG_MSG("[vX][sclk] quotient %u\n", *privquotient);
        DEBUG_MSG("[vX][sclk] modulus %u\n", *privmodulus);
#endif

    u32 newtick;
    
    
    //FIXME this is a mess (works though) heh
    
    if (!(util_getbit(*portstatus, MOD_SCLK_PORT_STATUS_BIT_ACTIVE))) { 
        Mod_SClk_ResetCounters(nodeID);
        newtick = DEAD_TIMESTAMP;
        
    } else {
        if (util_getbit(*portstatus, MOD_SCLK_PORT_STATUS_BIT_SYNC)) {
            if (!(util_getbit(*portstatus, MOD_SCLK_PORT_STATUS_BIT_HARDSYNC))) {
                if (!(util_getbit(*portstatus, MOD_SCLK_PORT_STATUS_BIT_GOTSYNC))) {
                    Mod_SClk_ResetCounters(nodeID);;
                    newtick = *portsynctick;
                    
                } else {
                    newtick = node[nodeID].nexttick;
                    
                }
                
            } else {
                if (*privlastsync != *portsynctick) {
                    *privlastsync = *portsynctick;
                    if (*portsynctick != DEAD_TIMESTAMP) {
                        Mod_SClk_ResetCounters(nodeID);
                        newtick = *portsynctick;
                        
                    } else {
                        newtick = node[nodeID].nexttick;
                    
                    }
                    
                } else {
                    newtick = node[nodeID].nexttick;
                    
                }
                
                node[nodeID].ticked--;
                
            }
            
            
            if (newtick == DEAD_TIMESTAMP) {
                Mod_SClk_ResetCounters(nodeID);
                newtick = mod_Tick_Timestamp;
                
            }
            
            util_setbit(*portstatus, MOD_SCLK_PORT_STATUS_BIT_GOTSYNC);
            
        } else {                                                                // we aren't trying to sync so just go on from here
            newtick = node[nodeID].nexttick;
        }
        
        
        if (node[nodeID].ticked) {
            
            *privcountdown = *privquotient;
            if ((*privmodulus) > 0) {
#if vX_DEBUG_VERBOSE_LEVEL >= 1
        DEBUG_MSG("[vX][sclk] modcounter before %d\n", (*privmodcounter));
#endif
                *privmodcounter = *privmodcounter + *privmodulus;
                if (*privmodcounter > *privmodulus) {                           // If the SubClock Modulus Counter is greater than SubClock Modulus then
                    (*privcountdown)++;                                         // The SubClock Countdown is incremented by 1 (this is the distribution of the modulus)
                    *privmodcounter = *privmodcounter - *portnumerator;         // The SubClock Numerator is subtracted from SubClock Modulus Counter (this controls how often the modulus is distributed)
                    
                }
#if vX_DEBUG_VERBOSE_LEVEL >= 1
        DEBUG_MSG("[vX][sclk] modcounter after %d\n", (*privmodcounter));
#endif
                
            }
#if vX_DEBUG_VERBOSE_LEVEL >= 1
        DEBUG_MSG("[vX][sclk] countdown %u\n", (*privcountdown));
#endif
            
            newtick += (*privcountdown);
            
            node[nodeID].ticked--;
            
        }
        
        
    }
    
    
#if vX_DEBUG_VERBOSE_LEVEL >= 1
        DEBUG_MSG("[vX][sclk] newtick %u\n", newtick);
#endif
    
    return newtick;
    
}

void Mod_SClk_ResetCounters(unsigned char nodeID) {
    u8 *portstatus;
    portstatus = (u8 *) &(node[nodeID].ports[MOD_SCLK_PORT_STATUS]);
    u8 *portnumerator;
    portnumerator = (u8 *) &(node[nodeID].ports[MOD_SCLK_PORT_NUMERATOR]);
    u8 *portdenominator;
    portdenominator = (u8 *) &(node[nodeID].ports[MOD_SCLK_PORT_DENOMINATOR]);
    u32 *portcyclelen;
    portcyclelen = (u32 *) &(node[nodeID].ports[MOD_SCLK_PORT_CYCLELEN]);
    
    u32 *privlastsync;
    privlastsync = (u32 *) &(node[nodeID].privvars[MOD_SCLK_PRIV_LASTSYNC]);
    s16 *privmodcounter;
    privmodcounter = (s16 *) &(node[nodeID].privvars[MOD_SCLK_PRIV_MODCOUNTER]);
    u16 *privcountdown;
    privcountdown = (u16 *) &(node[nodeID].privvars[MOD_SCLK_PRIV_COUNTDOWN]);
    u16 *privquotient;
    privquotient = (u16 *) &(node[nodeID].privvars[MOD_SCLK_PRIV_QUOTIENT]);
    u16 *privmodulus;
    privmodulus = (u16 *) &(node[nodeID].privvars[MOD_SCLK_PRIV_MODULUS]);
    
    
    *portcyclelen = ((mClock.cyclelen) * (*portdenominator));                   // heh once upon a time this was processor intensive. now it's three cycles. LOL.
    *privquotient = ((*portcyclelen) / (*portnumerator));
    *privmodulus = ((*portcyclelen) % (*portnumerator));
    
    *privcountdown = 0;
    *privmodcounter = 0;
    
    *privlastsync = DEAD_TIMESTAMP;
    
    util_clrbit(*portstatus, MOD_SCLK_PORT_STATUS_BIT_GOTSYNC);
    
}
