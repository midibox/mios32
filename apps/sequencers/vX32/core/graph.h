/* $Id$ */
/*
vX32 pre-alpha
not for any use whatsoever
copyright stryd_one
bite me corp 2008

big props to nILS for being my fourth eye and TK for obvious reasons
stay tuned for UI prototyping courtesy of lucem!

*/


#ifndef _GRAPH_H
#define _GRAPH_H


/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////

#ifndef max_nodes																// number of modules that can be created
#define max_nodes 0x3f															// Maximum 254. The basic struct is statically allocated
#endif

#ifndef max_indegree															// number of patch cables going IN to any module
#define max_indegree 0xfe														// Maximum 254, They're dynamically allocated
#endif



// Do not change defines below here
#define dead_nodeid (max_nodes+1)												// Do not change
#define dead_indegree (max_indegree+1)											// Do not change

#define nodeidinuse_mod (max_nodes%8)											// Do not change
#if nodeidinuse_mod > 0
#define nodeidinuse_bytes ((max_nodes/8)+1)
#else
#define nodeidinuse_bytes (max_nodes/8)
#endif



/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////

typedef struct edge_t {															// type for an edge (patch cable)
	unsigned char tailnodeid;													// node we're going to
	unsigned char tailport;														// port we're going from (we already know the node id, it's hosting this edge
	unsigned char headnodeid;													// node we're going to
	unsigned char headport;														// port we're going to
	void (*msgxlate) (unsigned char tail_nodeid, unsigned char tail_port
					, unsigned char head_nodeid, unsigned char head_port);		// function pointer calculated at edge creation based on port types
	struct edge_t *next;														// pointer to next edge in the list
	struct edge_t *head_next;													// pointer to next inward edge in the list at the headnode
}edge_t;


typedef struct {																// type for an output buffer
	mios32_midi_port_t port;													// output port
	mios32_midi_package_t midi_package;											// package to send
}outbuffer_t;

typedef struct {																// type for a node (module)
	struct edge_t *edgelist;													// linked list of outward edges (patch cables going from here to elsewhere)
	struct edge_t *edgelist_in;													// linked list of inward edges (patch cables coming from elsewhere to here)
	
	unsigned char indegree;														// counter of inward edges (in degree). also used to mark the node as dead for error checking, by setting it to dead_indegree (0xFF usually)
	unsigned char indegree_uv;													// counter of unvisited inward edges used by topological sort
	signed char *ports;															// pointer to ports array which is dynamically allocated at module creation time. size dictated by module type
	signed char *privvars;														// pointer to private variables array which is dynamically allocated at module creation time. size dictated by module type
	
	signed char *outbuffer;														// pointer to output buffer array which is dynamically allocated at module creation time
	unsigned char outbuffer_count;												// number of output buffers to create (eg 3  would be polyphonic output)
	unsigned char outbuffer_size;												// size in bytes of each of those buffers (eg mios package type is 5, for: port, status, channel, note number, velocity)
	unsigned char outbuffer_type;												// type of output data, used to select a function to grab the data from the module and send it. see mod_send.c, array is named mod_send_buffer_type[]
	
	unsigned char outbuffer_req;												// counter of how many buffers should be sent, and are filled and ready to go (eg 2 would send only two of your 3 midi notes as per the above examples))
	unsigned char moduletype;													// used to select functions to handle your modules. see modules.c, array named mod_moduledata_type[]
	unsigned char ticked;														// counter to show that the timestamp for this module has matched the global timestamp, meaning, it's ticked'
	unsigned char process_req;													// flag to request preprocessing. this flag should always be incremented if you have changed any values of the module, and then mod_preprocess(nodeid) should be called
	
	unsigned char name[8];														// array to hold text name (eg "bass 1")
	
	unsigned char deleting;														// flag used to tell the tickpriority function to ignore this node
	
	u32 nexttick;																// next tick timestamp for this node. compared with master clock timestamp during mod_tick, and flags are set if it's hit
	u32 downstreamtick;															// the soonest downstream tick from this node. used for prioritising preprocessing
	
}node_t;


typedef struct nodelist_t {														// linked list for node IDs
	unsigned char nodeid;														// this node ID (the array element in nodes[]
	struct nodelist_t *next;													// pointer to the next
}nodelist_t;



/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////

extern nodelist_t *topolisthead;												// list of topologically sorted nodeids


extern node_t node[max_nodes];													// array of sructs to hold node/module info




/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern void graph_init(void);													// initializes the graph (vX rack)


extern unsigned char node_add(unsigned char moduletype);						// add a new node (vX module)

unsigned char nodeid_gen(void);													// returns the first available node ID


extern unsigned char node_del(unsigned char delnodeid);							// delete a node

unsigned char nodeid_free(unsigned char nodeid);								// mark this node ID available


extern edge_t *edge_add(unsigned char tail_nodeid, unsigned char tail_port
						, unsigned char head_nodeid, unsigned char head_port);	// add an edge (vX patch cable)

extern unsigned char edge_del(edge_t *deledge, unsigned char dosort);			// delete an edge

void (*edge_get_xlator(unsigned char tail_nodeid, unsigned char tail_port
					, unsigned char head_nodeid, unsigned char head_port)) 
					(unsigned char tail_nodeid, unsigned char tail_port
					, unsigned char head_nodeid, unsigned char head_port);		// returns a function pointer for translating data from one port type to another



extern unsigned char toposort(void);											// does a topological sort of all active nodes

void topolist_clear(void);														// trashes the topo sort list


extern unsigned char node_count;												// count of how many nodes we have alive

#endif /* _GRAPH_H */
