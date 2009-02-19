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



const unsigned char mod_seq_porttypes[mod_seq_ports] = {
	mod_porttype_value, //status
	mod_porttype_value, //chan
	mod_porttype_value, //note
	mod_porttype_value, //vel
	mod_porttype_timestamp, //length reference
	dead_porttype,
	dead_porttype,
	dead_porttype, //length reference
	mod_porttype_timestamp, //next tick
	dead_porttype,
	dead_porttype,
	dead_porttype, //next tick
	mod_porttype_value, //currentstep
	dead_porttype,
	dead_porttype,
	dead_porttype, //currentstep
	mod_porttype_value, //length mult
	mod_porttype_value, //length div
	dead_porttype,
	dead_porttype,
	dead_porttype,
	dead_porttype,
	dead_porttype,
	dead_porttype, //dummies until i get around to making proper modules heheh
	dead_porttype,
	dead_porttype,
	dead_porttype,
	dead_porttype,
	dead_porttype,
	dead_porttype,
	dead_porttype,
	dead_porttype,
};




void mod_init_seq(unsigned char nodeid) {							// initialize a seq module
	u8 *portcurrentstep;
	portcurrentstep =  (u8 *) &node[nodeid].ports[MOD_SEQ_PORT_CURRENTSTEP];
	u8 *portchan;
	portchan =  (u8 *) &node[nodeid].ports[MOD_SEQ_PORT_NOTE0_CHAN];
	u8 *portnote;
	portnote =  (u8 *) &node[nodeid].ports[MOD_SEQ_PORT_NOTE0_NOTE];
	u8 *portvel;
	portvel =  (u8 *) &node[nodeid].ports[MOD_SEQ_PORT_NOTE0_VEL];
	s8 *portlenh;
	portlenh =  (s8 *) &node[nodeid].ports[MOD_SEQ_PORT_NOTE0_LENH];
	s8 *portlenl;
	portlenl =  (s8 *) &node[nodeid].ports[MOD_SEQ_PORT_NOTE0_LENL];
	u32 *portlenref;
	portlenref =  (u32 *) &node[nodeid].ports[MOD_SEQ_PORT_NOTE0_LENREF];
	u32 *portnexttick;
	portnexttick =  (u32 *) &node[nodeid].ports[MOD_SEQ_PORT_NEXTTICK];
	
	*portcurrentstep = 0;
	*portchan = 0x09;
	*portnote = -26+(8*nodeid); //FIXME TESTING
	*portvel = 100;
	*portlenh = 8;	//  1
	*portlenl = 0;	// 32nd notes
	//*lenl = 31;	// 64th notes
	//*lenl = -32;	// whole notes
	
	*portlenref = dead_timestamp;
	
	*portnexttick = dead_timestamp;
	
}

void mod_proc_seq(unsigned char nodeid) {							// do stuff with inputs and push to the outputs 
	u8 *portcurrentstep;
	portcurrentstep =  (u8 *) &node[nodeid].ports[MOD_SEQ_PORT_CURRENTSTEP];
	u8 *portchan;
	portchan =  (u8 *) &node[nodeid].ports[MOD_SEQ_PORT_NOTE0_CHAN];
	u8 *portnote;
	portnote =  (u8 *) &node[nodeid].ports[MOD_SEQ_PORT_NOTE0_NOTE];
	u8 *portvel;
	portvel =  (u8 *) &node[nodeid].ports[MOD_SEQ_PORT_NOTE0_VEL];
	s8 *portlenh;
	portlenh =  (s8 *) &node[nodeid].ports[MOD_SEQ_PORT_NOTE0_LENH];
	s8 *portlenl;
	portlenl =  (s8 *) &node[nodeid].ports[MOD_SEQ_PORT_NOTE0_LENL];
	u32 *portlenref;
	portlenref =  (u32 *) &node[nodeid].ports[MOD_SEQ_PORT_NOTE0_LENREF];
	u32 *portnexttick;
	portnexttick =  (u32 *) &node[nodeid].ports[MOD_SEQ_PORT_NEXTTICK];
	
	
	mod_setnexttick(nodeid, *portnexttick, &mod_reset_seq);	
	
	
	
	if (node[nodeid].ticked) {
		if (++(*portcurrentstep) > 15) (*portcurrentstep) = 0;
		node[nodeid].ticked--;
	}
	
	
	
	
	node[nodeid].outbuffer[MOD_SEND_MIDI_NOTE0_CHAN] = *portchan;
		
	node[nodeid].outbuffer[MOD_SEND_MIDI_NOTE0_NOTE] = *portnote + *portcurrentstep;
		
	node[nodeid].outbuffer[MOD_SEND_MIDI_NOTE0_VEL] = *portvel;
	
	u8 lenh = (util_s8tou4(*portlenh)+1);
	u8 lenl = (util_s8tou6(*portlenl)+1);
	
	
	u32 lengthref;
	
	if (*portlenref >= dead_timestamp) {
		lengthref = mclock.cyclelen;
	} else {
		lengthref = *portlenref;
	}
	
	unsigned int length = (u16) (lenh * (lengthref / lenl));
	
	node[nodeid].outbuffer[MOD_SEND_MIDI_NOTE0_LENH] = (signed char)((length>>8)&0x00ff);
	node[nodeid].outbuffer[MOD_SEND_MIDI_NOTE0_LENL] = (signed char)(length&0x00ff);
	
	node[nodeid].outbuffer_req++;
	
}

void mod_uninit_seq(unsigned char nodeid) {							// uninitialize a seq module
}

u32 mod_reset_seq(unsigned char nodeid) {							// reset a seq module
	u8 *portcurrentstep;
	portcurrentstep =  (u8 *) &node[nodeid].ports[MOD_SEQ_PORT_CURRENTSTEP];
	u32 *portnexttick;
	portnexttick = (u32 *) &node[nodeid].ports[MOD_SEQ_PORT_NEXTTICK];
	
	*portcurrentstep = 0;
	
	return *portnexttick;
	
}
