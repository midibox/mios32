// $Id: dpot.c $
//! \defgroup DPOT
//!
//! DPOT module driver
//!
//! The application interface (API) has been designed to communicate with a 
//! variety of digital potentiometer (DPOT) integrated circuit (IC) chips.  As 
//! of this time, there are no officially supported digital potentiometer 
//! modules that are part of the MIDIbox Hardware Platform (MBHP).
//! 
//! The number of DPOTs is software-limitted to 32 by the 'dpot_update_req' 
//! variable (32-bit wide unsigned integer).  The limit of supported DPOTs will
//! depend upon the limitations of the data bus (SPI or IIC, for example), the
//! limitations of the ICs that contain the DPOTs, and possibly the performance 
//! limitations of the MIOS32 core.
//! 
//! This file contains the generic interface that is presented to the MIOS32 
//! application.  The IC-specific implementation is contained in a separate 
//! file.  This allows the the MIOS32_DPOT implementation to be easily modified
//! for any number of DPOT ICs.
//!
//! DPOT output values (typically specifying the wiper pin position as a 
//! percentage of "full scale") are managed in 16bit resolution.  Be aware that
//! typical DPOTs have 128 positions or 256 positions, which requires only 8 
//! bits of resolution.  The use of 16bit resolution allows for the possibility
//! that a DPOT that features a large resolution can be supported.
//!
//! Before DPOT changes are applied to the external hardware, this API will 
//! comapare the new value with the current one.  If equal, the application to
//! the hardware will be omitted, otherwise it will be performed once 
//! DPOT_Update() is called.
//! 
//! This method has two advantages:
//! <UL>
//!   <LI>if DPOT_PinSet doesn't notice value changes, the appr. DPOT channels
//!     won't be updated to save CPU performance
//!   <LI>all DPOTs will be updated at the same moment
//! </UL>
//!
//! The voltage configuration jumper of J19 has to be set to appropriately
//! supply the DPOT IC (5V or 3V3), and the 4x1k Pull-Up resistor array should
//! be installed to supply the 3.3V->5V level shifting if 5V supply is used.
//!
//! An usage example can be found under $MIOS32_PATH/apps/tutorials/026_dpot
//!
//! \{
/* ==========================================================================
 *
 *  Copyright (C) 2010 Jonathan Farmer (JRFarmer.com@gmail.com)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

/////////////////////////////////////////////////////////////////////////////
// Include files
/////////////////////////////////////////////////////////////////////////////

#include <mios32.h>
#include "dpot.h"


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

static u16 dpot_value[DPOT_NUM_POTS];
static u32 dpot_update_req;


/////////////////////////////////////////////////////////////////////////////
// Driver Implementation
/////////////////////////////////////////////////////////////////////////////

#if defined (DPOT_MCP41XXX) || defined (DPOT_MCP42XXX)
    #include "dpot_mcp41xxx.inc"
/*
#elif defined <insert macro here> 
    #include <insert driver file here> 
*/
#else
    // no digital pot is specified
    #error DPOT device has not been specified.
#endif


/////////////////////////////////////////////////////////////////////////////
//! Initializes DPOT driver
//! Should be called from Init() during startup
//! \param[in] mode currently only mode 0 supported
//! \return < 0 if initialisation failed
/////////////////////////////////////////////////////////////////////////////
s32 DPOT_Init(u32 mode)
{
    s32 status = 0;
    int pot;

    // currently only mode 0 supported
    if (mode != 0)
    {
        return -1; // unsupported mode
    }

    // set all DPOT values to 0
    for(pot = 0; pot < DPOT_NUM_POTS; pot++)
    {
        dpot_value[pot] = 0;
    }

    // initialize hardware
    status = DPOT_Device_Init(mode);

    return status;
}


/////////////////////////////////////////////////////////////////////////////
//! This function sets an output channel to a given 16-bit value.
//!
//! The output value won't be transfered to the module immediately, but will 
//! be buffered instead. By calling DPOT_Update() the requested changes
//! will take place.
//! \param[in] pot the pot number (0..DPOT_NUM_POTS-1)
//! \param[in] value the 16bit value
//! \return -1 if pin not available
//! \return 0 on success
/////////////////////////////////////////////////////////////////////////////
s32 DPOT_Set_Value(u8 pot, u16 value)
{
    if (pot >= DPOT_NUM_POTS)
    {
        return -1; // pot not available
    }

    if (value != dpot_value[pot])
    {
        dpot_value[pot] = value;
        dpot_update_req |= (0x00000001 << pot);
    }

    return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! This function returns the current DPOT value of a DPOT channel.
//! \param[in] pot the pot number (0..DPOT_NUM_POTS-1)
//! \return -1 if pot not available
//! \return >= 0 if pot available (16bit output value)
/////////////////////////////////////////////////////////////////////////////
s32 DPOT_Get_Value(u8 pot)
{
    if (pot >= DPOT_NUM_POTS)
    {
        return -1; // pin not available
    }
  
    return dpot_value[pot];
}


/////////////////////////////////////////////////////////////////////////////
//! Updates the output channels of all DPOT channels
//! Should be called, whenever changes have been requested via 
//! DPOT_Set_Value()
//! \return < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 DPOT_Update(void)
{
    s32 status = 0;
    u32 req = 0;
  
    // check for DPOT channel update requests
    MIOS32_IRQ_Disable();
    req = dpot_update_req;
    dpot_update_req = 0;
    MIOS32_IRQ_Enable();

    // update hardware
    if (req)
    {
        status = DPOT_Device_Update(req);
    }

    return status;
}

//! \}
