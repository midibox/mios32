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

#ifndef MAX_NODES                                                               // number of modules that can be created
#define MAX_NODES 0x3f                                                          // Maximum 254. The basic struct is statically allocated
#endif

#ifndef MAX_INDEGREE                                                            // number of patch cables going IN to any module
#define MAX_INDEGREE 0xfe                                                       // Maximum 254, They're dynamically allocated
#endif



// Do not change defines below here

#define MODULE_NAME_STRING_LENGTH 8                                             // number of characters in a module's name string

#define NODE_STATUS_DELETING                                                    // flag used to tell the tickpriority function to ignore this node
#define NODE_STATUS_TICKSEEN                                                    // flag used to tell the tickpriority function to ignore this node

#define DEAD_NODEID (MAX_NODES+1)                                               // Do not change
#define DEAD_INDEGREE (MAX_INDEGREE+1)                                          // Do not change

#define NODEIDINUSE_MOD (MAX_NODES%8)                                           // Do not change
#if NODEIDINUSE_MOD > 0
#define NODEIDINUSE_BYTES ((MAX_NODES/8)+1)
#else
#define NODEIDINUSE_BYTES (MAX_NODES/8)
#endif


#define DO_TOPOSORT 1
#define DONT_TOPOSORT 0

/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////

typedef struct edge_t {                                                         // type for an edge (patch cable)
    unsigned char tailnodeID;                                                   // node we're going to
    unsigned char tailport;                                                     // port we're going from (we already know the node id, it's hosting this edge
    unsigned char headnodeID;                                                   // node we're going to
    unsigned char headport;                                                     // port we're going to
    void (*msgxlate) (unsigned char tail_nodeID, unsigned char tail_port
                    , unsigned char head_nodeID, unsigned char head_port);      // function pointer calculated at edge creation based on port types
    struct edge_t *next;                                                        // pointer to next edge in the list
    struct edge_t *head_next;                                                   // pointer to next inward edge in the list at the headnode
} edge_t;


typedef struct {                                                                // type for an output buffer
    mios32_midi_port_t port;                                                    // output port
    mios32_midi_package_t midi_package;                                         // package to send
} outbuffer_t;


typedef union {
    struct {
        unsigned all: 8;
    };
    struct {
        unsigned deleting: 1;
        unsigned tickseen: 1;
    };
} nodestatus_t;

typedef struct {                                                                // type for a node (module)
    unsigned char ticked;                                                       // counter to show that the timestamp for this module has matched the global timestamp, meaning, it's ticked'
    unsigned char process_req;                                                  // flag to request preprocessing. this flag should always be incremented if you have changed any values of the module, and then Mod_PreProcess(nodeID) should be called
    unsigned char moduletype;                                                   // used to select functions to handle your modules. see modules.c, array named mod_ModuleData_Type[]
    nodestatus_t status;                                                        // status flags
    
    u32 nexttick;                                                               // next tick timestamp for this node. compared with master clock timestamp during Mod_Tick, and flags are set if it's hit
    u32 downstreamtick;                                                         // the soonest downstream tick from this node. used for prioritising preprocessing
    
    unsigned char indegree;                                                     // counter of inward edges (in degree). also used to mark the node as dead for error checking, by setting it to DEAD_INDEGREE (0xFF usually)
    unsigned char indegree_uv;                                                  // counter of unvisited inward edges used by topological sort
    unsigned char outbuffer_size;                                               // size in bytes of each of those buffers (eg mios package type is 5, for: port, status, channel, note number, velocity)
    unsigned char outbuffer_req;                                                // counter of how many buffers should be sent, and are filled and ready to go (eg 2 would send only two of your 3 midi notes as per the above examples))
    
    struct edge_t *edgelist;                                                    // linked list of outward edges (patch cables going from here to elsewhere)
    struct edge_t *edgelist_in;                                                 // linked list of inward edges (patch cables coming from elsewhere to here)
    
    signed char *ports;                                                         // pointer to ports array which is dynamically allocated at module creation time. size dictated by module type
    signed char *privvars;                                                      // pointer to private variables array which is dynamically allocated at module creation time. size dictated by module type
    signed char *outbuffer;                                                     // pointer to output buffer array which is dynamically allocated at module creation time
    
    char name[MODULE_NAME_STRING_LENGTH];                                       // array to hold text name (eg "bass 1")
    
} node_t;


typedef struct nodelist_t {                                                     // linked list for node IDs
    unsigned char nodeID;                                                       // this node ID (the array element in nodes[]
    struct nodelist_t *next;                                                    // pointer to the next
} nodelist_t;



/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////

extern nodelist_t *topoListHead;                                                // list of topologically sorted nodeIDs

extern node_t node[MAX_NODES];                                                  // array of sructs to hold node/module info

extern unsigned char node_Count;                                                // count of how many nodes we have alive



/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern void Graph_Init(void);                                                   // initializes the graph (vX rack)


extern unsigned char Node_Add(unsigned char moduletype);                        // add a new node (vX module)


extern unsigned char Node_Del(unsigned char delnodeID);                         // delete a node



extern char *Node_SetName(unsigned char nodeID, char *newname);                 // set node name

extern unsigned char Node_GetID(char *getname, unsigned char length);           // find a node ID by name

extern unsigned char Node_GetType(unsigned char nodeID);                        // get node name by ID

extern const char *Node_GetTypeName(unsigned char moduletype);                  // get name of module type

unsigned char Node_GetTypeID(char *getname, unsigned char length);              // get module type by name

extern char *Node_GetName(unsigned char nodeID);                                // get node name





extern edge_t *Edge_Add(unsigned char tail_nodeID, unsigned char tail_port
                        , unsigned char head_nodeID, unsigned char head_port);  // add an edge (vX patch cable)

extern unsigned char Edge_Del(edge_t *deledge, unsigned char dosort);           // delete an edge




extern edge_t *Edge_GetID(unsigned char tail_nodeID, unsigned char tail_port
                        , unsigned char head_nodeID, unsigned char head_port);  // find a pointer to an edge

extern unsigned char Edge_GetTailNode(edge_t * edge);

extern unsigned char Edge_GetHeadNode(edge_t * edge);

extern unsigned char Edge_GetTailPort(edge_t * edge);

extern unsigned char Edge_GetHeadPort(edge_t * edge);




void (*Edge_Get_Xlator(unsigned char tail_nodeID, unsigned char tail_port
                    , unsigned char head_nodeID, unsigned char head_port)) 
                    (unsigned char tail_nodeID, unsigned char tail_port
                    , unsigned char head_nodeID, unsigned char head_port);      // returns a function pointer for translating data from one port type to another



extern unsigned char TopoSort(void);                                            // does a topological sort of all active nodes

void TopoList_Clear(void);                                                      // trashes the topo sort list



#endif /* _GRAPH_H */
