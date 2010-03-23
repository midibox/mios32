// $Id: dpot_mcp41xxx.c $
/* ==========================================================================
 *
 *  Copyright (C) 2010 Jonathan Farmer (JRFarmer.com@gmail.com)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _DPOT_DEV_C
#define _DPOT_DEV_C

/////////////////////////////////////////////////////////////////////////////
// Include files
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////

// Number of DPOTs per IC chip
#if defined (DPOT_MCP41XXX) 
    // MCP41XXX cannot be daisy-chained
    // Therefore, only 1 digital potentiometer is supported
    #if (DPOT_NUM_POTS != 1)
        #error Invalid definition of DPOT_NUM_POTS
    #endif  
    #define DPOT_NUM_CHAN_ON_CHIP 1
    #define DPOT_NUM_IC 1
#elif defined (DPOT_MCP42XXX)
    #define DPOT_NUM_CHAN_ON_CHIP 2
    #define DPOT_NUM_IC (DPOT_NUM_POTS/DPOT_NUM_CHAN_ON_CHIP + \
                        (DPOT_NUM_POTS%DPOT_NUM_CHAN_ON_CHIP == 0 ? 0 : 1))
#endif

// Which SPI peripheral should be used
// allowed values: 0 (J16), 1 (J8/9) and 2 (J19)
#ifndef DPOT_SPI
    #define DPOT_SPI 2
#else
    #if (DPOT_SPI < 0) || (DPOT_SPI > 2)
        #error Invalid definition of DPOT_SPI
    #endif
#endif

// Which RC pin of the SPI port should be used
// allowed values: 0 or 1 for SPI0 (J16:RC1, J16:RC2), 0 for SPI1 (J8/9:RC), 0 or 1 for SPI2 (J19:RC1, J19:RC2)
#ifndef DPOT_SPI_RC_PIN
    #define DPOT_SPI_RC_PIN 0
#else
    #if ((DPOT_SPI == 0) && (DPOT_SPI_RC_PIN != 0) && (DPOT_SPI_RC_PIN != 1)) || \
        ((DPOT_SPI == 1) && (DPOT_SPI_RC_PIN != 0)) || \
        ((DPOT_SPI == 2) && (DPOT_SPI_RC_PIN != 0) && (DPOT_SPI_RC_PIN != 1))
        #error Invalid definition of DPOT_SPI_RC_PIN
    #endif
#endif

// By default, output pins should be used in Open Drain mode (perfect for 3.3V->5V levelshifting)
#ifndef DPOT_SPI_OUTPUTS_OD
    #define DPOT_SPI_OUTPUTS_OD 1
#endif

enum enmDigipotCommand
{
    eNoCommand = 0,
    eWriteDataCommand,
    eShutDownCommand,
};


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
//! Initializes IC-specific driver
//! This method is called from DPOT_Init() during startup
//! \param[in] mode currently only mode 0 supported
//! \return < 0 if initialisation failed
/////////////////////////////////////////////////////////////////////////////
static s32 DPOT_Device_Init(u32 mode)
{
    s32 status = 0;
    int dac;

    // currently only mode 0 supported
    if( mode != 0 )
    {
        return -1; // unsupported mode
    }
  
#if (DPOT_SPI == 0)
    // Ensure that internal DAC pins are disabled
    // These DAC pins coincide with SPI0 on J16
    for (dac = 0; dac < 2; dac++)
    {
        MIOS32_BOARD_DAC_PinInit(dac, 0);
    }
#endif

#if (DPOT_SPI_OUTPUTS_OD == 1)
    // pins in open drain mode (to pull-up the outputs to 5V)
    status |= MIOS32_SPI_IO_Init(DPOT_SPI, MIOS32_SPI_PIN_DRIVER_STRONG_OD);
#else
    // pins in push-pull mode (3.3V output voltage)
    status |= MIOS32_SPI_IO_Init(DPOT_SPI, MIOS32_SPI_PIN_DRIVER_STRONG);
#endif

    // init SPI port for fast frequency access
    status |= MIOS32_SPI_TransferModeInit(DPOT_SPI, MIOS32_SPI_MODE_CLK0_PHASE1, MIOS32_SPI_PRESCALER_4);

    // ensure that CS is released
    status |= MIOS32_SPI_RC_PinSet(DPOT_SPI, DPOT_SPI_RC_PIN, 1); // spi, rc_pin, pin_value

    return status;
}


/////////////////////////////////////////////////////////////////////////////
//! Updates the output channels of all DPOT channels
//! This method is called from DPOT_Update() 
//! DPOT_Set_Value
//! \return < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 DPOT_Device_Update(u32 req)
{
    s32 status = 0;
    int device;
    int chan_num;
    u8  pot_num;
    u8  commmand;

    if (req)
    {
        // Update the first pot on each chip (chan_num == 0), then repeat 
        // for each subsequent chan_num
        for (chan_num = 0; chan_num < DPOT_NUM_CHAN_ON_CHIP; chan_num++)
        {
            // activate chip select
            status |= MIOS32_SPI_RC_PinSet(DPOT_SPI, DPOT_SPI_RC_PIN, 0); // spi, rc_pin, pin_value
      
            // loop through devices (last device command/data has to be sent first)
            for (device = DPOT_NUM_IC - 1; device >= 0; device--)
            {
                // setup pot_num for this particular potentiometer
                pot_num = (device*DPOT_NUM_CHAN_ON_CHIP + chan_num);

                // check for update request on this potentiometer
                if (req & (0x00000001 << pot_num))
                {
                    // update is requested, setup write command for this pot
                    commmand = ((u8)eWriteDataCommand << 4) | (u8)(chan_num + 1);
                }
                else
                {
                    // no update requested, setup no-op command
                    commmand = (u8)eNoCommand << 4;
                }
        
                // transfer command and data byte
                status |= MIOS32_SPI_TransferByte(DPOT_SPI, commmand);
                status |= MIOS32_SPI_TransferByte(DPOT_SPI, (dpot_value[pot_num] & 0xFF));          
            }
      
            // deactivate chip select
            status |= MIOS32_SPI_RC_PinSet(DPOT_SPI, DPOT_SPI_RC_PIN, 1); // spi, rc_pin, pin_value
        }
    }

    return status;
}
#endif /* _DPOT_DEV_C */

