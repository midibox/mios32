/* $Id: modules.c 251 2009-01-06 13:30:53Z stryd_one $ */
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
// Global prototypes
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Port translator functions
/////////////////////////////////////////////////////////////////////////////

void mod_xlate_time_time(unsigned char tail_nodeid, unsigned char tail_port, 
						unsigned char head_nodeid, unsigned char head_port) {	// timestamp - timestamp
u32 *from, *to;
from = (u32 *) &(node[tail_nodeid].ports[tail_port]);
to = (u32 *) &(node[head_nodeid].ports[head_port]);
*to = *from;
}

void mod_xlate_time_value(unsigned char tail_nodeid, unsigned char tail_port, 
						unsigned char head_nodeid, unsigned char head_port) {	// timestamp - value
}

void mod_xlate_time_flag(unsigned char tail_nodeid, unsigned char tail_port, 
						unsigned char head_nodeid, unsigned char head_port) {	// timestamp - flag
}

void mod_xlate_value_time(unsigned char tail_nodeid, unsigned char tail_port, 
						unsigned char head_nodeid, unsigned char head_port) {	// value - timestamp
}

void mod_xlate_value_value(unsigned char tail_nodeid, unsigned char tail_port, 
						unsigned char head_nodeid, unsigned char head_port) {	// value - value
u8 *from, *to;
from = (u8 *) &(node[tail_nodeid].ports[tail_port]);
to = (u8 *) &(node[head_nodeid].ports[head_port]);
*to = *from;
}

void mod_xlate_value_flag(unsigned char tail_nodeid, unsigned char tail_port, 
						unsigned char head_nodeid, unsigned char head_port) {	// value - flag
}

void mod_xlate_flag_time(unsigned char tail_nodeid, unsigned char tail_port, 
						unsigned char head_nodeid, unsigned char head_port) {	// flag - timestamp
}

void mod_xlate_flag_value(unsigned char tail_nodeid, unsigned char tail_port, 
						unsigned char head_nodeid, unsigned char head_port) {	// flag - value
}

void mod_xlate_flag_flag(unsigned char tail_nodeid, unsigned char tail_port, 
						unsigned char head_nodeid, unsigned char head_port) {	// flag - flag
}



/////////////////////////////////////////////////////////////////////////////
// Write dead values to ports according to their type
/////////////////////////////////////////////////////////////////////////////

void mod_deadport_time(unsigned char nodeid, unsigned char port) {
	u32 *deadport;
	deadport = (u32 *) &node[nodeid].ports[port];
	*deadport = dead_timestamp;
	
}

void mod_deadport_value(unsigned char nodeid, unsigned char port) {
	s8 *deadport;
	deadport = (s8 *)&node[nodeid].ports[port];
	*deadport = dead_value;
	
}

void mod_deadport_flag(unsigned char nodeid, unsigned char port) {
	u8 *deadport;
	deadport = (u8 *) &node[nodeid].ports[port];
	*deadport = dead_value;
	
}
