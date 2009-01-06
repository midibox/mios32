/* $Id:  $ */
/*
vX32 pre-alpha
not for any use whatsoever
copyright stryd_one
bite me corp 2008

big props to nILS for being my fourth eye and TK for obvious reasons
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





void mod_init_seq(unsigned char nodeid) {							// initialize a seq module
	node[nodeid].ports[MOD_SEQ_PORT_NOTE0_CHAN] = 0x09;
	node[nodeid].ports[MOD_SEQ_PORT_NOTE0_NOTE] = 0;
	node[nodeid].ports[MOD_SEQ_PORT_NOTE0_VEL] = 100;
	node[nodeid].ports[MOD_SEQ_PORT_NOTE0_LENH] = -8;	//  1
	node[nodeid].ports[MOD_SEQ_PORT_NOTE0_LENL] = 0;	// 32nd notes
	//node[nodeid].ports[MOD_SEQ_PORT_NOTE0_LENL] = 31;	// 64th notes
}

void mod_proc_seq(unsigned char nodeid) {							// do stuff with inputs and push to the outputs 
	
	if (node[nodeid].ticked) {
		if (++(node[nodeid].ports[MOD_SEQ_PORT_NOTE0_NOTE]) > 15) (node[nodeid].ports[MOD_SEQ_PORT_NOTE0_NOTE]) = 0;
	}
	
	node[nodeid].outbuffer[MOD_SEND_MIDI_NOTE0_CHAN] = node[nodeid].ports[MOD_SEQ_PORT_NOTE0_CHAN];
	node[nodeid].outbuffer[MOD_SEND_MIDI_NOTE0_NOTE] = node[nodeid].ports[MOD_SEQ_PORT_NOTE0_NOTE]+nodeid;
	node[nodeid].outbuffer[MOD_SEND_MIDI_NOTE0_VEL] = node[nodeid].ports[MOD_SEQ_PORT_NOTE0_VEL];
	
	u8 lenh = (util_s8tou4(node[nodeid].ports[MOD_SEQ_PORT_NOTE0_LENH])+1);
	u8 lenl = (util_s8tou6(node[nodeid].ports[MOD_SEQ_PORT_NOTE0_LENL])+1);
	
	unsigned int length = lenh * (sclock[node[nodeid].clocksource].cyclelen / lenl);
	
	
	node[nodeid].outbuffer[MOD_SEND_MIDI_NOTE0_LENH] = (signed char)(length>>8);
	node[nodeid].outbuffer[MOD_SEND_MIDI_NOTE0_LENL] = (signed char)(length&0x00ff);
	
	node[nodeid].outbuffer_req++;
	
}

void mod_uninit_seq(unsigned char nodeid) {							// uninitialize a seq module
}

