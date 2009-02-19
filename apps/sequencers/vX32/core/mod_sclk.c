/* $Id:  $ */
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


const unsigned char mod_sclk_porttypes[mod_sclk_ports] = {
	mod_porttype_value, //status
	mod_porttype_value, //numerator
	mod_porttype_value, //denominator
	dead_porttype, //padding
	mod_porttype_timestamp, //cycle length
	dead_porttype,
	dead_porttype,
	dead_porttype,
	mod_porttype_timestamp, //next tick
	dead_porttype,
	dead_porttype,
	dead_porttype,
	mod_porttype_timestamp, //sync tick
	dead_porttype,
	dead_porttype,
	dead_porttype,
};

//port pointers for use in functions, copy and use as requ'd
// FIXME these shoud really be signed and used as relative numbers, that's for later on
	/*
	u8 *portstatus;
	portstatus = (u8 *) &(node[nodeid].ports[MOD_SCLK_PORT_STATUS]);
	u8 *portnumerator;
	portnumerator = (u8 *) &(node[nodeid].ports[MOD_SCLK_PORT_NUMERATOR]);
	u8 *portdenominator;
	portdenominator = (u8 *) &(node[nodeid].ports[MOD_SCLK_PORT_DENOMINATOR]);
	u32 *portcyclelen;
	portcyclelen = (u32 *) &(node[nodeid].ports[MOD_SCLK_PORT_CYCLELEN]);
	u32 *portnexttick;
	portnexttick = (u32 *) &(node[nodeid].ports[MOD_SCLK_PORT_NEXTTICK]);
	u32 *portsynctick;
	portsynctick = (u32 *) &(node[nodeid].ports[MOD_SCLK_PORT_SYNCTICK]);

	u16 *privquotient;
	privquotient = (u16 *) &(node[nodeid].privvars[MOD_SCLK_PRIV_QUOTIENT]);
	u16 *privmodulus;
	privmodulus = (u16 *) &(node[nodeid].privvars[MOD_SCLK_PRIV_MODULUS]);
	u16 *privmodcounter;
	privmodcounter = (u16 *) &(node[nodeid].privvars[MOD_SCLK_PRIV_MODCOUNTER]);
	u16 *privcountdown;
	privcountdown = (u16 *) &(node[nodeid].privvars[MOD_SCLK_PRIV_COUNTDOWN]);
	u8 *privstatus;
	privstatus = (u8 *) &(node[nodeid].ports[MOD_SCLK_PRIV_STATUS]);
	u32 *privlastsync;
	privlastsync = (u32 *) &(node[nodeid].privvars[MOD_SCLK_PRIV_LASTSYNC]);
	*/


void mod_init_sclk(unsigned char nodeid) {						// initialize a subclock module
	u8 *portstatus;
	portstatus = (u8 *) &(node[nodeid].ports[MOD_SCLK_PORT_STATUS]);
	u8 *portnumerator;
	portnumerator = (u8 *) &(node[nodeid].ports[MOD_SCLK_PORT_NUMERATOR]);
	u8 *portdenominator;
	portdenominator = (u8 *) &(node[nodeid].ports[MOD_SCLK_PORT_DENOMINATOR]);
	u32 *portnexttick;
	portnexttick = (u32 *) &(node[nodeid].ports[MOD_SCLK_PORT_NEXTTICK]);
	u32 *portsynctick;
	portsynctick = (u32 *) &(node[nodeid].ports[MOD_SCLK_PORT_SYNCTICK]);
	
	
	 // FIXME Should it be initialised as already active and synced to master ? probably not... Anyway it's an alpha version ffs ;)
	util_setbit(*portstatus, MOD_SCLK_PORT_STATUS_BIT_ACTIVE);
	util_setbit(*portstatus, MOD_SCLK_PORT_STATUS_BIT_SYNC);
	util_clrbit(*portstatus, MOD_SCLK_PORT_STATUS_BIT_HARDSYNC);
	
	*portnumerator = 4; // 1/4 notes
	*portdenominator = 1; // 1/4 notes
	
	
	*portsynctick = dead_timestamp;
	*portnexttick = dead_timestamp;
	
	mod_sclk_resetcounters(nodeid);
	mod_setnexttick(nodeid, mod_sclk_getnexttick(nodeid), &mod_reset_sclk);
	
}

void mod_proc_sclk(unsigned char nodeid) { 						// do stuff with inputs and push to the outputs 
	u32 *portnexttick;
	portnexttick = (u32 *) &(node[nodeid].ports[MOD_SCLK_PORT_NEXTTICK]);
	
	mod_setnexttick(nodeid, mod_sclk_getnexttick(nodeid), &mod_reset_sclk);
	
	*portnexttick = node[nodeid].nexttick;
	
}

void mod_uninit_sclk(unsigned char nodeid) { 						// uninitialize a sample and hold module
}


u32 mod_reset_sclk(unsigned char nodeid) {							// reset a seq module
	u32 *portsynctick;
	portsynctick = (u32 *) &(node[nodeid].ports[MOD_SCLK_PORT_SYNCTICK]);
	
	mod_sclk_resetcounters(nodeid);
	
	if (util_getbit((mclock.status), MCLOCK_STATUS_RESETREQ)) {
		return mod_tick_timestamp;
		
	} else {
		return mod_sclk_getnexttick(nodeid);
	}
	
}

u32 mod_sclk_getnexttick(unsigned char nodeid) {					// 

	s8 *portstatus;
	portstatus = (s8 *) &(node[nodeid].ports[MOD_SCLK_PORT_STATUS]);
	u8 *portnumerator;
	portnumerator = (u8 *) &(node[nodeid].ports[MOD_SCLK_PORT_NUMERATOR]);
	u8 *portdenominator;
	portdenominator = (u8 *) &(node[nodeid].ports[MOD_SCLK_PORT_DENOMINATOR]);
	u32 *portcyclelen;
	portcyclelen = (u32 *) &(node[nodeid].ports[MOD_SCLK_PORT_CYCLELEN]);
	u32 *portnexttick;
	portnexttick = (u32 *) &(node[nodeid].ports[MOD_SCLK_PORT_NEXTTICK]);
	u32 *portsynctick;
	portsynctick = (u32 *) &(node[nodeid].ports[MOD_SCLK_PORT_SYNCTICK]);

	u16 *privquotient;
	privquotient = (u16 *) &(node[nodeid].privvars[MOD_SCLK_PRIV_QUOTIENT]);
	u16 *privmodulus;
	privmodulus = (u16 *) &(node[nodeid].privvars[MOD_SCLK_PRIV_MODULUS]);
	u16 *privmodcounter;
	privmodcounter = (u16 *) &(node[nodeid].privvars[MOD_SCLK_PRIV_MODCOUNTER]);
	u16 *privcountdown;
	privcountdown = (u16 *) &(node[nodeid].privvars[MOD_SCLK_PRIV_COUNTDOWN]);
	u8 *privstatus;
	privstatus = (u8 *) &(node[nodeid].ports[MOD_SCLK_PRIV_STATUS]);
	u32 *privlastsync;
	privlastsync = (u32 *) &(node[nodeid].privvars[MOD_SCLK_PRIV_LASTSYNC]);


	*portcyclelen = ((mclock.cyclelen) * (*portdenominator)); // heh once upon a time this was processor intensive. now it's three cycles. LOL.
	*privquotient = ((*portcyclelen) / (*portnumerator));
	*privmodulus = ((*portcyclelen) % (*portnumerator));

	u32 newtick;
	
	
	//FIXME this is a mess
	
	if (!(util_getbit(*portstatus, MOD_SCLK_PORT_STATUS_BIT_ACTIVE))) { 
		mod_sclk_resetcounters(nodeid);
		newtick = dead_timestamp;
		
	} else {
		if (util_getbit(*portstatus, MOD_SCLK_PORT_STATUS_BIT_SYNC)) {
			if (!(util_getbit(*portstatus, MOD_SCLK_PORT_STATUS_BIT_HARDSYNC))) {
				if (!(util_getbit(*portstatus, MOD_SCLK_PORT_STATUS_BIT_GOTSYNC))) {
					mod_sclk_resetcounters(nodeid);;
					newtick = *portsynctick;
					
				} else {
					newtick = node[nodeid].nexttick;
					
				}
				
			} else {
				if (*privlastsync != *portsynctick) {
					*privlastsync = *portsynctick;
					if (*portsynctick != dead_timestamp) {
						mod_sclk_resetcounters(nodeid);
						newtick = *portsynctick;
						
					} else {
						newtick = node[nodeid].nexttick;
					
					}
					
				} else {
					newtick = node[nodeid].nexttick;
					
				}
				
				node[nodeid].ticked--;
				
			}
			
			
			if (newtick == dead_timestamp) {
				mod_sclk_resetcounters(nodeid);
				newtick = mod_tick_timestamp;
				
			}
			
			util_setbit(*portstatus, MOD_SCLK_PORT_STATUS_BIT_GOTSYNC);
			
		}
		
		
		if (node[nodeid].ticked) {
			
			*privcountdown = *privquotient;
			if ((*privmodulus) > 0) {
				*privmodcounter = *privmodcounter + *privmodulus;
				if (*privmodcounter > *privmodulus) {						// If the SubClock Modulus Counter is greater than SubClock Modulus then
					(*privcountdown)++;										// The SubClock Countdown is incremented by 1 (this is the distribution of the modulus)
					*privmodcounter = *privmodcounter - *portnumerator;		// The SubClock Numerator is subtracted from SubClock Modulus Counter (this controls how often the modulus is distributed)
					
				}
				
			}
			
			newtick += (*privcountdown);
			
			node[nodeid].ticked--;
			
		}
		
		
	}
	
	return newtick;
	
}

void mod_sclk_resetcounters(unsigned char nodeid) {
	u8 *portstatus;
	portstatus = (u8 *) &(node[nodeid].ports[MOD_SCLK_PORT_STATUS]);
	u8 *portnumerator;
	portnumerator = (u8 *) &(node[nodeid].ports[MOD_SCLK_PORT_NUMERATOR]);
	u8 *portdenominator;
	portdenominator = (u8 *) &(node[nodeid].ports[MOD_SCLK_PORT_DENOMINATOR]);
	u32 *portcyclelen;
	portcyclelen = (u32 *) &(node[nodeid].ports[MOD_SCLK_PORT_CYCLELEN]);
	
	u32 *privlastsync;
	privlastsync = (u32 *) &(node[nodeid].privvars[MOD_SCLK_PRIV_LASTSYNC]);
	u16 *privmodcounter;
	privmodcounter = (u16 *) &(node[nodeid].privvars[MOD_SCLK_PRIV_MODCOUNTER]);
	u16 *privcountdown;
	privcountdown = (u16 *) &(node[nodeid].privvars[MOD_SCLK_PRIV_COUNTDOWN]);
	u16 *privquotient;
	privquotient = (u16 *) &(node[nodeid].privvars[MOD_SCLK_PRIV_QUOTIENT]);
	u16 *privmodulus;
	privmodulus = (u16 *) &(node[nodeid].privvars[MOD_SCLK_PRIV_MODULUS]);
	
	
	*portcyclelen = ((mclock.cyclelen) * (*portdenominator)); // heh once upon a time this was processor intensive. now it's three cycles. LOL.
	*privquotient = ((*portcyclelen) / (*portnumerator));
	*privmodulus = ((*portcyclelen) % (*portnumerator));
	
	*privcountdown = 0;
	*privmodcounter = 0;
	
	*privlastsync = dead_timestamp;
	
	util_clrbit(*portstatus, MOD_SCLK_PORT_STATUS_BIT_GOTSYNC);
	
}
