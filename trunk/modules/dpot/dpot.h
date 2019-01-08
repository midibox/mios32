// $Id: dpot.h $
/*
 * Header file for DPOT API functions
 *
 * ==========================================================================
 *
 *  Copyright (C) 2010 Jonathan Farmer (JRFarmer.com@gmail.com)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _DPOT_H
#define _DPOT_H

#ifdef __cplusplus
extern "C" {
#endif

/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////

// Maximum number of DPOT channels (1..32)
// (Number of channels within this range can be changed via soft-configuration during runtime.)
#ifndef DPOT_NUM_POTS
    #define DPOT_NUM_POTS 32
#else
    #if (DPOT_NUM_POTS <= 0) || (DPOT_NUM_POTS > 32)
        #error Invalid definition of DPOT_NUM_POTS
    #endif
#endif


/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 DPOT_Init(u32 mode);

extern s32 DPOT_Set_Value(u8 pot, u16 value);
extern s32 DPOT_Get_Value(u8 pot);

extern s32 DPOT_Update(void);


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////


#ifdef __cplusplus
}
#endif

#endif /* _DPOT_H */
