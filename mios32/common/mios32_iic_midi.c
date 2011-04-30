// $Id$
//! \defgroup MIOS32_IIC_MIDI
//!
//! IIC MIDI layer for MIOS32
//! 
//! Except for \ref MIOS32_IIC_MIDI_ScanInterfaces applications shouldn't call
//! these functions directly, instead please use \ref MIOS32_MIDI layer functions
//! 
//! \{
/* ==========================================================================
 *
 *  Copyright (C) 2008 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

/////////////////////////////////////////////////////////////////////////////
// Include files
/////////////////////////////////////////////////////////////////////////////

#include <mios32.h>

// this module can be optionally disabled in a local mios32_config.h file (included from mios32.h)
#if !defined(MIOS32_DONT_USE_IIC_MIDI)


/////////////////////////////////////////////////////////////////////////////
// Local Prototypes
/////////////////////////////////////////////////////////////////////////////
#if MIOS32_IIC_MIDI_NUM
static s32 MIOS32_IIC_MIDI_GetRI(u8 iic_port);
#endif


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

#if MIOS32_IIC_MIDI_NUM
// available interfaces
static u8 iic_port_available = 0;

static u8 rs_optimisation;
#endif


/////////////////////////////////////////////////////////////////////////////
//! Initializes IIC MIDI layer
//! \param[in] mode currently only mode 0 supported
//! \return < 0 if initialisation failed
//! \note Applications shouldn't call this function directly, instead please use \ref MIOS32_MIDI layer functions
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_IIC_MIDI_Init(u32 mode)
{
  // currently only mode 0 supported
  if( mode != 0 )
    return -1; // unsupported mode

#if MIOS32_IIC_MIDI_NUM == 0
  return -1; // IIC MIDI interface not explicitely enabled in mios32_config.h
#else

#if defined(MIOS32_FAMILY_STM32F10x)
  // configure RI_N pins as inputs with internal pull-up
  GPIO_InitTypeDef GPIO_InitStructure;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
  GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IPU;

#if MIOS32_IIC_MIDI0_ENABLED == 3
  GPIO_InitStructure.GPIO_Pin   = MIOS32_IIC_MIDI0_RI_N_PIN;
  GPIO_Init(MIOS32_IIC_MIDI0_RI_N_PORT, &GPIO_InitStructure);
#endif
#if MIOS32_IIC_MIDI1_ENABLED == 3
  GPIO_InitStructure.GPIO_Pin   = MIOS32_IIC_MIDI1_RI_N_PIN;
  GPIO_Init(MIOS32_IIC_MIDI1_RI_N_PORT, &GPIO_InitStructure);
#endif
#if MIOS32_IIC_MIDI2_ENABLED == 3
  GPIO_InitStructure.GPIO_Pin   = MIOS32_IIC_MIDI2_RI_N_PIN;
  GPIO_Init(MIOS32_IIC_MIDI2_RI_N_PORT, &GPIO_InitStructure);
#endif
#if MIOS32_IIC_MIDI3_ENABLED == 3
  GPIO_InitStructure.GPIO_Pin   = MIOS32_IIC_MIDI3_RI_N_PIN;
  GPIO_Init(MIOS32_IIC_MIDI3_RI_N_PORT, &GPIO_InitStructure);
#endif
#if MIOS32_IIC_MIDI4_ENABLED == 3
  GPIO_InitStructure.GPIO_Pin   = MIOS32_IIC_MIDI4_RI_N_PIN;
  GPIO_Init(MIOS32_IIC_MIDI4_RI_N_PORT, &GPIO_InitStructure);
#endif
#if MIOS32_IIC_MIDI5_ENABLED == 3
  GPIO_InitStructure.GPIO_Pin   = MIOS32_IIC_MIDI5_RI_N_PIN;
  GPIO_Init(MIOS32_IIC_MIDI5_RI_N_PORT, &GPIO_InitStructure);
#endif
#if MIOS32_IIC_MIDI6_ENABLED == 3
  GPIO_InitStructure.GPIO_Pin   = MIOS32_IIC_MIDI6_RI_N_PIN;
  GPIO_Init(MIOS32_IIC_MIDI6_RI_N_PORT, &GPIO_InitStructure);
#endif
#if MIOS32_IIC_MIDI7_ENABLED == 3
  GPIO_InitStructure.GPIO_Pin   = MIOS32_IIC_MIDI7_RI_N_PIN;
  GPIO_Init(MIOS32_IIC_MIDI7_RI_N_PORT, &GPIO_InitStructure);
#endif

#elif defined(MIOS32_FAMILY_LPC17xx)
#if MIOS32_IIC_MIDI0_ENABLED == 3
  MIOS32_SYS_LPC_PINSEL(MIOS32_IIC_MIDI0_RI_N_PORT, MIOS32_IIC_MIDI0_RI_N_PIN, 0);
  MIOS32_SYS_LPC_FIODIR(MIOS32_IIC_MIDI0_RI_N_PORT, MIOS32_IIC_MIDI0_RI_N_PIN, 0);
#endif
#if MIOS32_IIC_MIDI1_ENABLED == 3
  MIOS32_SYS_LPC_PINSEL(MIOS32_IIC_MIDI1_RI_N_PORT, MIOS32_IIC_MIDI1_RI_N_PIN, 0);
  MIOS32_SYS_LPC_FIODIR(MIOS32_IIC_MIDI1_RI_N_PORT, MIOS32_IIC_MIDI1_RI_N_PIN, 0);
#endif
#if MIOS32_IIC_MIDI2_ENABLED == 3
  MIOS32_SYS_LPC_PINSEL(MIOS32_IIC_MIDI2_RI_N_PORT, MIOS32_IIC_MIDI2_RI_N_PIN, 0);
  MIOS32_SYS_LPC_FIODIR(MIOS32_IIC_MIDI2_RI_N_PORT, MIOS32_IIC_MIDI2_RI_N_PIN, 0);
#endif
#if MIOS32_IIC_MIDI3_ENABLED == 3
  MIOS32_SYS_LPC_PINSEL(MIOS32_IIC_MIDI3_RI_N_PORT, MIOS32_IIC_MIDI3_RI_N_PIN, 0);
  MIOS32_SYS_LPC_FIODIR(MIOS32_IIC_MIDI3_RI_N_PORT, MIOS32_IIC_MIDI3_RI_N_PIN, 0);
#endif
#if MIOS32_IIC_MIDI4_ENABLED == 3
  MIOS32_SYS_LPC_PINSEL(MIOS32_IIC_MIDI4_RI_N_PORT, MIOS32_IIC_MIDI4_RI_N_PIN, 0);
  MIOS32_SYS_LPC_FIODIR(MIOS32_IIC_MIDI4_RI_N_PORT, MIOS32_IIC_MIDI4_RI_N_PIN, 0);
#endif
#if MIOS32_IIC_MIDI5_ENABLED == 3
  MIOS32_SYS_LPC_PINSEL(MIOS32_IIC_MIDI5_RI_N_PORT, MIOS32_IIC_MIDI5_RI_N_PIN, 0);
  MIOS32_SYS_LPC_FIODIR(MIOS32_IIC_MIDI5_RI_N_PORT, MIOS32_IIC_MIDI5_RI_N_PIN, 0);
#endif
#if MIOS32_IIC_MIDI6_ENABLED == 3
  MIOS32_SYS_LPC_PINSEL(MIOS32_IIC_MIDI6_RI_N_PORT, MIOS32_IIC_MIDI6_RI_N_PIN, 0);
  MIOS32_SYS_LPC_FIODIR(MIOS32_IIC_MIDI6_RI_N_PORT, MIOS32_IIC_MIDI6_RI_N_PIN, 0);
#endif
#if MIOS32_IIC_MIDI7_ENABLED == 3
  MIOS32_SYS_LPC_PINSEL(MIOS32_IIC_MIDI7_RI_N_PORT, MIOS32_IIC_MIDI7_RI_N_PIN, 0);
  MIOS32_SYS_LPC_FIODIR(MIOS32_IIC_MIDI7_RI_N_PORT, MIOS32_IIC_MIDI7_RI_N_PIN, 0);
#endif

#else
#error "MIOS32_IIC_MIDI_Init() not prepared for this MIOS32_FAMILY!"
#endif


  // initialize IIC interface
  if( MIOS32_IIC_Init(0) < 0 )
    return -1; // initialisation of IIC Interface failed

  // scan for available MBHP_IIC_MIDI modules
  if( MIOS32_IIC_MIDI_ScanInterfaces() < 0 )
    return -2; // we don't expect that any other task accesses the IIC port yet!

#if MIOS32_IIC_MIDI_NUM
  // enable running status optimisation by default for all ports
  rs_optimisation = ~0; // -> all-one

  // TODO: send optimisation flag to IIC_MIDI device once it has been scanned!
#endif

  return 0; // no error
#endif
}


/////////////////////////////////////////////////////////////////////////////
//! Scans all MBHP_IIC_MIDI modules to check the availablility by sending
//! dummy requests and checking the ACK response.
//!
//! Per module, this procedure takes at least ca. 25 uS, if no module is 
//! connected ca. 75 uS (3 retries), or even more if we have to wait for 
//! completion of the previous IIC transfer.
//!
//! Therefore this function should only be rarely used (e.g. once per second),
//! and the state should be saved somewhere in the application
//! \return 0 if all IIC interfaces scanned
//! \return -2 if IIC interface blocked by another task (retry the scan!)
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_IIC_MIDI_ScanInterfaces(void)
{
#if MIOS32_IIC_MIDI_NUM == 0
  return 0; // IIC MIDI interface not explicitely enabled in mios32_config.h
#else
    // create availability map based on compile options
  const u8 iic_port_enabled[8] = {
    MIOS32_IIC_MIDI0_ENABLED,
    MIOS32_IIC_MIDI1_ENABLED,
    MIOS32_IIC_MIDI2_ENABLED,
    MIOS32_IIC_MIDI3_ENABLED,
    MIOS32_IIC_MIDI4_ENABLED,
    MIOS32_IIC_MIDI5_ENABLED,
    MIOS32_IIC_MIDI6_ENABLED,
    MIOS32_IIC_MIDI7_ENABLED
  };

  // try to get the IIC peripheral
  if( MIOS32_IIC_TransferBegin(MIOS32_IIC_MIDI_PORT, IIC_Non_Blocking) < 0 )
    return -2; // request a retry

  u8 iic_port;
  iic_port_available = 0;
  for(iic_port=0; iic_port<MIOS32_IIC_MIDI_NUM; ++iic_port) {
    if( iic_port_enabled[iic_port] ) {
      // try to address the peripheral 3 times
      u8 retries=3;
      s32 error = -1;

      while( error < 0 && retries-- ) {
	s32 error = MIOS32_IIC_Transfer(MIOS32_IIC_MIDI_PORT, IIC_Write, MIOS32_IIC_MIDI_ADDR_BASE + 2*iic_port, NULL, 0);
	if( !error )
	  error = MIOS32_IIC_TransferWait(MIOS32_IIC_MIDI_PORT);
	if( !error )
	  iic_port_available |= (1 << iic_port);
      }
    }
  }

  // release IIC peripheral
  MIOS32_IIC_TransferFinished(MIOS32_IIC_MIDI_PORT);

  return 0; // no error during scan
#endif
}


/////////////////////////////////////////////////////////////////////////////
//! This function checks the availability of the MBHP_IIC_MIDI module
//! taken from the last results of MIOS32_IIC_MIDI_ScanInterface()
//! \param[in] iic_port module number (0..7)
//! \return 1: interface available
//! \return 0: interface not available
//! \note Applications shouldn't call this function directly, instead please use \ref MIOS32_MIDI layer functions
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_IIC_MIDI_CheckAvailable(u8 iic_port)
{
#if MIOS32_IIC_MIDI_NUM == 0
  return 0x00; // IIC MIDI interface not explicitely enabled in mios32_config.h
#else
  return (iic_port_available & (1 << iic_port)) ? 1 : 0;
#endif
}


/////////////////////////////////////////////////////////////////////////////
//! This function enables/disables running status optimisation for a given
//! MIDI OUT port to improve bandwidth if MIDI events with the same
//! status byte are sent back-to-back.<BR>
//! Note that the optimisation is enabled by default.
//! \param[in] iic_port module number (0..7)
//! \param[in] enable 0=optimisation disabled, 1=optimisation enabled
//! \return -1 if port not available
//! \return 0 on success
//! \note Applications shouldn't call this function directly, instead please use \ref MIOS32_MIDI layer functions
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_IIC_MIDI_RS_OptimisationSet(u8 iic_port, u8 enable)
{
#if MIOS32_IIC_MIDI_NUM == 0
  return -1; // port not available
#else
  if( iic_port >= MIOS32_IIC_MIDI_NUM )
    return -1; // port not available

  u8 mask = 1 << iic_port;
  rs_optimisation &= ~mask;
  if( enable )
    rs_optimisation |= mask;

  // TODO: send optimisation flag to IIC_MIDI device!

  return 0; // no error
#endif
}


/////////////////////////////////////////////////////////////////////////////
//! This function returns the running status optimisation enable/disable flag
//! for the given MIDI OUT port.
//! \param[in] iic_port module number (0..7)
//! \return -1 if port not available
//! \return 0 if optimisation disabled
//! \return 1 if optimisation enabled
//! \note Applications shouldn't call this function directly, instead please use \ref MIOS32_MIDI layer functions
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_IIC_MIDI_RS_OptimisationGet(u8 iic_port)
{
#if MIOS32_IIC_MIDI_NUM == 0
  return -1; // port not available
#else
  if( iic_port >= MIOS32_IIC_MIDI_NUM )
    return -1; // port not available

  return (rs_optimisation & (1 << iic_port)) ? 1 : 0;
#endif
}

/////////////////////////////////////////////////////////////////////////////
//! This function resets the current running status, so that it will be sent
//! again with the next MIDI Out package.
//! \param[in] iic_port module number (0..7)
//! \return -1 if port not available
//! \return < 0 on errors
//! \note Applications shouldn't call this function directly, instead please use \ref MIOS32_MIDI layer functions
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_IIC_MIDI_RS_Reset(u8 iic_port)
{
#if MIOS32_IIC_MIDI_NUM == 0
  return -1; // port not available
#else
  if( iic_port >= MIOS32_IIC_MIDI_NUM )
    return -1; // port not available

  // TODO: send RS status reset command to IIC_MIDI device!

  return 0;
#endif
}


/////////////////////////////////////////////////////////////////////////////
//! This function should be called periodically each mS to handle timeout
//! and expire counters.
//!
//! Not for use in an application - this function is called from
//! MIOS32_MIDI_Periodic_mS(), which is called by a task in the programming
//! model!
//! 
//! \return < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_IIC_MIDI_Periodic_mS(void)
{
  // currently only a dummy - RS optimisation handled by IIC_MIDI device

  return 0;
}


/////////////////////////////////////////////////////////////////////////////
// Help function - see descriptions of MIOS32_IIC_MIDI_PackageSend* functions
/////////////////////////////////////////////////////////////////////////////
static s32 _MIOS32_IIC_MIDI_PackageSend(u8 iic_port, mios32_midi_package_t package, u8 non_blocking_mode)
{
#if MIOS32_IIC_MIDI_NUM == 0
  return -1; // IIC MIDI interface not explicitely enabled in mios32_config.h
#else
  // exit if IIC port not available
  if( !MIOS32_IIC_MIDI_CheckAvailable(iic_port) )
    return -1;

  u8 len = mios32_midi_pcktype_num_bytes[package.cin];
  if( len ) {
    u8 buffer[3] = {package.evnt0, package.evnt1, package.evnt2};
    s32 error = -1;

    if( non_blocking_mode ) {
      if( MIOS32_IIC_TransferBegin(MIOS32_IIC_MIDI_PORT, IIC_Non_Blocking) < 0 )
	return -2; // retry until interface available

      error = MIOS32_IIC_TransferCheck(MIOS32_IIC_MIDI_PORT);
      if( error ) {
	MIOS32_IIC_TransferFinished(MIOS32_IIC_MIDI_PORT);
	return -2; // retry, regardless if MBHP_IIC_MIDI module is busy or not accessible
      }
      error = MIOS32_IIC_Transfer(MIOS32_IIC_MIDI_PORT, IIC_Write, MIOS32_IIC_MIDI_ADDR_BASE + 2*iic_port, buffer, len);
    } else {
      u8 retries = 3; // for blocking mode: retry to access the MBHP_IIC_MIDI module 3 times

      MIOS32_IIC_TransferBegin(MIOS32_IIC_MIDI_PORT, IIC_Blocking);

      error = -1;
      while( error < 0 && retries-- ) {
	error = MIOS32_IIC_Transfer(MIOS32_IIC_MIDI_PORT, IIC_Write, MIOS32_IIC_MIDI_ADDR_BASE + 2*iic_port, buffer, len);
	if( !error )
	  error = MIOS32_IIC_TransferWait(MIOS32_IIC_MIDI_PORT);
      }
    }

    MIOS32_IIC_TransferFinished(MIOS32_IIC_MIDI_PORT);

    return (error < 0) ? -3 : 0; // IIC error if status < 0
  } else {
    return 0; // no bytes to send -> no error
  }
#endif
}


/////////////////////////////////////////////////////////////////////////////
//! This function sends a new MIDI package to the selected IIC_MIDI port
//! \param[in] iic_port IIC_MIDI module number (0..7)
//! \param[in] package MIDI package
//! \return 0: no error
//! \return -1: IIC_MIDI device not available
//! \return -2: IIC_MIDI buffer is full
//!             caller should retry until buffer is free again
//! \return -3: IIC error during transfer
//! \return -4: Error reported by Rx callback function
//! \note Applications shouldn't call this function directly, instead please use \ref MIOS32_MIDI layer functions
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_IIC_MIDI_PackageSend_NonBlocking(u8 iic_port, mios32_midi_package_t package)
{
  return _MIOS32_IIC_MIDI_PackageSend(iic_port, package, 1); // non-blocking mode
}


/////////////////////////////////////////////////////////////////////////////
//! This function sends a new MIDI package to the selected IIC_MIDI port
//! (blocking function)
//! \param[in] iic_port IIC_MIDI module number (0..7)
//! \param[in] package MIDI package
//! \return 0: no error
//! \return -1: IIC_MIDI device not available
//! \return -3: IIC error during transfer
//! \return -4: Error reported by Rx callback function
//! \note Applications shouldn't call this function directly, instead please use \ref MIOS32_MIDI layer functions
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_IIC_MIDI_PackageSend(u8 iic_port, mios32_midi_package_t package)
{
  return _MIOS32_IIC_MIDI_PackageSend(iic_port, package, 0); // blocking mode
}


/////////////////////////////////////////////////////////////////////////////
// Help function - see descriptions of MIOS32_IIC_MIDI_PackageReceive* functions
/////////////////////////////////////////////////////////////////////////////
static s32 _MIOS32_IIC_MIDI_PackageReceive(u8 iic_port, mios32_midi_package_t *package, u8 non_blocking_mode)
{
#if MIOS32_IIC_MIDI_NUM == 0
  return -1; // IIC MIDI interface not explicitely enabled in mios32_config.h
#else
  // exit if IIC port not available
  if( !MIOS32_IIC_MIDI_CheckAvailable(iic_port) )
    return -1;

  // exit if no new data
  if( !MIOS32_IIC_MIDI_GetRI(iic_port) )
    return -1;

  // TODO: error -10 (package timeout) not supported yet

  u8 buffer[4];

  // request IIC
  if( non_blocking_mode ) {
    if( MIOS32_IIC_TransferBegin(MIOS32_IIC_MIDI_PORT, IIC_Non_Blocking) < 0 )
      return -2; // request retry
  } else {
    MIOS32_IIC_TransferBegin(MIOS32_IIC_MIDI_PORT, IIC_Blocking);
  }

  s32 error = MIOS32_IIC_Transfer(MIOS32_IIC_MIDI_PORT, IIC_Read_AbortIfFirstByteIs0, MIOS32_IIC_MIDI_ADDR_BASE + 2*iic_port, buffer, 4);
  if( !error )
    error = MIOS32_IIC_TransferWait(MIOS32_IIC_MIDI_PORT);

  if( !error ) {
    error = MIOS32_MIDI_SendPackageToRxCallback(IIC0 + iic_port, *package);
    if( error )
      return error < 0 ? -4 : 0; // don't forward error if package has been filtered
  }

  if( !error ) {
    package->type  = buffer[0];
    package->evnt0 = buffer[1];
    package->evnt1 = buffer[2];
    package->evnt2 = buffer[3];

    error = package->type ? 0 : -1; // return 0 if new package is available
  } else {
    error = -3; // IIC error
  }

  // release IIC port
  MIOS32_IIC_TransferFinished(MIOS32_IIC_MIDI_PORT);

  return error;
#endif
}


/////////////////////////////////////////////////////////////////////////////
//! This function checks for a new package
//! \param[in] iic_port IIC_MIDI module number (0..7)
//! \param[out] package pointer to MIDI package (received package will be put into the given variable)
//! \return 0: no error
//! \return -1: no package in buffer
//! \return -2: IIC interface allocated - retry (only in Non Blocking mode)
//! \return -3: IIC error during transfer
//! \return -10: IIC package timed out
//! \note Applications shouldn't call this function directly, instead please use \ref MIOS32_MIDI layer functions
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_IIC_MIDI_PackageReceive_NonBlocking(u8 iic_port, mios32_midi_package_t *package)
{
  return _MIOS32_IIC_MIDI_PackageReceive(iic_port, package, 1); // non-blocking
}


/////////////////////////////////////////////////////////////////////////////
//! This function checks for a new package<BR>
//! (blocking function)
//! \param[in] iic_port IIC_MIDI module number (0..7)
//! \param[out] package pointer to MIDI package (received package will be put into the given variable)
//! \return 0: no error
//! \return -1: no package in buffer
//! \return -3: IIC error during transfer
//! \return -10: IIC package timed out
//! \note Applications shouldn't call this function directly, instead please use \ref MIOS32_MIDI layer functions
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_IIC_MIDI_PackageReceive(u8 iic_port, mios32_midi_package_t *package)
{
  return _MIOS32_IIC_MIDI_PackageReceive(iic_port, package, 0); // blocking
}


/////////////////////////////////////////////////////////////////////////////
//! Returns inverted state of RI_N pin
//! \param[in] iic_port IIC_MIDI module number (0..7)
//! \return 1: RI_N active
//! \return 0: RI_N not active
//! \return always 1 if RI_N pin not configured (driver uses polling method in this case)
//! \return always 0 if invalid IIC port (>= 8)
//! \note Applications shouldn't call this function directly, instead please use \ref MIOS32_MIDI layer functions
/////////////////////////////////////////////////////////////////////////////
#if MIOS32_IIC_MIDI_NUM
static s32 MIOS32_IIC_MIDI_GetRI(u8 iic_port)
{
  switch( iic_port ) {
#if MIOS32_IIC_MIDI0_ENABLED <= 1
    case 0: return 0; // disabled or MIDI Out Only
#elif MIOS32_IIC_MIDI0_ENABLED == 3
# if defined(MIOS32_FAMILY_STM32F10x)
    case 0: return GPIO_ReadInputDataBit(MIOS32_IIC_MIDI0_RI_N_PORT, MIOS32_IIC_MIDI0_RI_N_PIN) ? 0 : 1;
# elif defined(MIOS32_FAMILY_LPC17xx)
    case 0: return MIOS32_SYS_LPC_FIOGET(MIOS32_IIC_MIDI0_RI_N_PORT, MIOS32_IIC_MIDI0_RI_N_PIN) ? 0 : 1;
# else
# warning "unsupported MIOS32_FAMILY"
    case 0: return 0; // unknown family
# endif
#else
    case 0: return 1;
#endif
#if MIOS32_IIC_MIDI1_ENABLED <= 1
    case 1: return 0; // disabled or MIDI Out Only
#elif MIOS32_IIC_MIDI1_ENABLED == 3
# if defined(MIOS32_FAMILY_STM32F10x)
    case 1: return GPIO_ReadInputDataBit(MIOS32_IIC_MIDI1_RI_N_PORT, MIOS32_IIC_MIDI1_RI_N_PIN) ? 0 : 1;
# elif defined(MIOS32_FAMILY_LPC17xx)
    case 1: return MIOS32_SYS_LPC_FIOGET(MIOS32_IIC_MIDI1_RI_N_PORT, MIOS32_IIC_MIDI1_RI_N_PIN) ? 0 : 1;
# else
# warning "unsupported MIOS32_FAMILY"
    case 1: return 0; // unknown family
# endif
#else
    case 1: return 1;
#endif
#if MIOS32_IIC_MIDI2_ENABLED <= 1
    case 2: return 0; // disabled or MIDI Out Only
#elif MIOS32_IIC_MIDI2_ENABLED == 3
# if defined(MIOS32_FAMILY_STM32F10x)
    case 2: return GPIO_ReadInputDataBit(MIOS32_IIC_MIDI2_RI_N_PORT, MIOS32_IIC_MIDI2_RI_N_PIN) ? 0 : 1;
# elif defined(MIOS32_FAMILY_LPC17xx)
    case 2: return MIOS32_SYS_LPC_FIOGET(MIOS32_IIC_MIDI2_RI_N_PORT, MIOS32_IIC_MIDI2_RI_N_PIN) ? 0 : 1;
# else
# warning "unsupported MIOS32_FAMILY"
    case 2: return 0; // unknown family
# endif
#else
    case 2: return 1;
#endif
#if MIOS32_IIC_MIDI3_ENABLED <= 1
    case 3: return 0; // disabled or MIDI Out Only
#elif MIOS32_IIC_MIDI3_ENABLED == 3
# if defined(MIOS32_FAMILY_STM32F10x)
    case 3: return GPIO_ReadInputDataBit(MIOS32_IIC_MIDI3_RI_N_PORT, MIOS32_IIC_MIDI3_RI_N_PIN) ? 0 : 1;
# elif defined(MIOS32_FAMILY_LPC17xx)
    case 3: return MIOS32_SYS_LPC_FIOGET(MIOS32_IIC_MIDI3_RI_N_PORT, MIOS32_IIC_MIDI3_RI_N_PIN) ? 0 : 1;
# else
# warning "unsupported MIOS32_FAMILY"
    case 3: return 0; // unknown family
# endif
#else
    case 3: return 1;
#endif
#if MIOS32_IIC_MIDI4_ENABLED <= 1
    case 4: return 0; // disabled or MIDI Out Only
#elif MIOS32_IIC_MIDI4_ENABLED == 3
# if defined(MIOS32_FAMILY_STM32F10x)
    case 4: return GPIO_ReadInputDataBit(MIOS32_IIC_MIDI4_RI_N_PORT, MIOS32_IIC_MIDI4_RI_N_PIN) ? 0 : 1;
# elif defined(MIOS32_FAMILY_LPC17xx)
    case 4: return MIOS32_SYS_LPC_FIOGET(MIOS32_IIC_MIDI4_RI_N_PORT, MIOS32_IIC_MIDI4_RI_N_PIN) ? 0 : 1;
# else
# warning "unsupported MIOS32_FAMILY"
    case 4: return 0; // unknown family
# endif
#else
    case 4: return 1;
#endif
#if MIOS32_IIC_MIDI5_ENABLED <= 1
    case 5: return 0; // disabled or MIDI Out Only
#elif MIOS32_IIC_MIDI5_ENABLED == 3
# if defined(MIOS32_FAMILY_STM32F10x)
    case 5: return GPIO_ReadInputDataBit(MIOS32_IIC_MIDI5_RI_N_PORT, MIOS32_IIC_MIDI5_RI_N_PIN) ? 0 : 1;
# elif defined(MIOS32_FAMILY_LPC17xx)
    case 5: return MIOS32_SYS_LPC_FIOGET(MIOS32_IIC_MIDI5_RI_N_PORT, MIOS32_IIC_MIDI5_RI_N_PIN) ? 0 : 1;
# else
# warning "unsupported MIOS32_FAMILY"
    case 5: return 0; // unknown family
# endif
#else
    case 5: return 1;
#endif
#if MIOS32_IIC_MIDI6_ENABLED <= 1
    case 6: return 0; // disabled or MIDI Out Only
#elif MIOS32_IIC_MIDI6_ENABLED == 3
# if defined(MIOS32_FAMILY_STM32F10x)
    case 6: return GPIO_ReadInputDataBit(MIOS32_IIC_MIDI6_RI_N_PORT, MIOS32_IIC_MIDI6_RI_N_PIN) ? 0 : 1;
# elif defined(MIOS32_FAMILY_LPC17xx)
    case 6: return MIOS32_SYS_LPC_FIOGET(MIOS32_IIC_MIDI6_RI_N_PORT, MIOS32_IIC_MIDI6_RI_N_PIN) ? 0 : 1;
# else
# warning "unsupported MIOS32_FAMILY"
    case 6: return 0; // unknown family
# endif
#else
    case 6: return 1;
#endif
#if MIOS32_IIC_MIDI7_ENABLED <= 1
    case 7: return 0; // disabled or MIDI Out Only
#elif MIOS32_IIC_MIDI7_ENABLED == 3
# if defined(MIOS32_FAMILY_STM32F10x)
    case 7: return GPIO_ReadInputDataBit(MIOS32_IIC_MIDI7_RI_N_PORT, MIOS32_IIC_MIDI7_RI_N_PIN) ? 0 : 1;
# elif defined(MIOS32_FAMILY_LPC17xx)
    case 7: return MIOS32_SYS_LPC_FIOGET(MIOS32_IIC_MIDI7_RI_N_PORT, MIOS32_IIC_MIDI7_RI_N_PIN) ? 0 : 1;
# else
# warning "unsupported MIOS32_FAMILY"
    case 7: return 0; // unknown family
# endif
#else
    case 7: return 1;
#endif
  }

  return 0;
}
#endif

//! \}

#endif /* MIOS32_DONT_USE_IIC_MIDI */
