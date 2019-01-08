/* $Id$ */
/*
vX32 pre-alpha
not for any use whatsoever
copyright stryd_one
bite me corp 2008

big props to nILS for being my fourth eye and TK for obvious reasons
stay tuned for UI prototyping courtesy of lucem!

*/


#ifndef _MCLOCK_H
#define _MCLOCK_H


/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////

#ifndef VXPPQN
#define VXPPQN 384                                                              // internal clock resolution. should be a multiple of 24
#endif

#ifndef VXLATENCY
#define VXLATENCY 24                                                              // internal clock resolution. should be a multiple of 24
#endif


#ifndef MCLOCKTIMERNUMBER
#define MCLOCKTIMERNUMBER 0                                                     // used to force a specific timer to be used for master clock. not in use. 
//#define SEQ_BPM_MIOS32_TIMER_NUM MCLOCKTIMERNUMBER
#endif



/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////

typedef union {
    struct {
        unsigned all: 8;
    };
    struct {
        unsigned run: 1;
        unsigned spp_hunt: 1;
        unsigned reset_req: 1;
    };
} mclockstatus_t;

typedef struct {
    mclockstatus_t status;                                                      // bit 7 is run/stop
    float bpm;                                                                  // Master BPM
    
    unsigned int res;                                                           // MIDI Clock PPQN
    unsigned int ticked;                                                        // Counter to show how many ticks are waiting to be processed
    
    unsigned int cyclelen;                                                      // Length of master track measured in ticks.
    unsigned int rt_latency;                                                      // Length of master track measured in ticks.

    unsigned char timesigu;                                                     // Upper value of the Master Time Signature, max 16
    unsigned char timesigl;                                                     // Lower value of the Master Time Signature, min 2
 
} mclock_t;

                                                                                // bits of the status char above
#define MCLOCK_STATUS_RUN 7                                                     // bit 7 is run/stop request
#define MCLOCK_STATUS_SPP_HUNT 6                                                // bit 6 is SPP hunt flag (kills output when hunting for the correct song position)
#define MCLOCK_STATUS_RESETREQ 0                                                // bit 0 is master reset request



/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////

extern mclock_t mClock;                                                         // Allocate the Master Clock struct

extern u32 mod_Tick_Timestamp;                                                  // holds the master clock timestamp



/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern void Clocks_Init(void);

extern void MClock_Init(void);

extern void MClock_SetBPM(float bpm);

extern void MClock_Tick(void);


extern void MClock_Stop(void);

extern void MClock_Continue(void);

extern void MClock_Reset(void);



extern void MClock_User_SetBPM(float bpm);

extern void MClock_User_Start(void);

extern void MClock_User_Continue(void);

extern void MClock_User_Stop(void);


#endif /* _MCLOCK_H */
