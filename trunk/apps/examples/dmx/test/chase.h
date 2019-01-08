/*
 * Header for chases
 *
 * ==========================================================================
 *
 *  Copyright (C) 2009 Phil Taylor (phil@taylor.org.uk)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _CHASE_H
#define _CHASE_H

/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////



/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////


typedef struct T_ChaseDetails {
	u8 name[24]; 	// Name of chase
	u8 on:1;		// Chase is enabled
	u8 hold:1;		//Pause the chase
	u8 advance:1;	// if true advance to next step in chase
	u8 direction:1;	// 0 = up 1 = down
	u8 rubbish:4;	// remainder of bit-fields
	u32 speed;		// Speed of chase in mS
	u32 num_steps;	// Number of steps in chase
	u32 current_step;	// Current step we are on
}

typedef struct T_StepDetails {
	u32 step_num;	// Automatically generated step number
	u8 active:1;	// Is step currently active
	u8 deleted:1;	// has step has been deleted 
	u8 rubbish:6;	// remainder of bit-fields
	u8 name[24];	// Name of step
	u8 values[512];	// DMX Values
}


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////

#endif /* _FS_H */
