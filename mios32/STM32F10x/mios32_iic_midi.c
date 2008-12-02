// $Id$
/*
 * IIC MIDI layer for MIOS32
 *
 * ==========================================================================
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
static s32 MIOS32_IIC_MIDI_GetRI(u8 iic_port);


/////////////////////////////////////////////////////////////////////////////
// Global variables
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

// available interfaces
static u8 iic_port_available = 0;


/////////////////////////////////////////////////////////////////////////////
// Initializes IIC MIDI layer
// IN: <mode>: currently only mode 0 supported
// OUT: returns < 0 if initialisation failed
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_IIC_MIDI_Init(u32 mode)
{
  // currently only mode 0 supported
  if( mode != 0 )
    return -1; // unsupported mode

#if MIOS32_IIC_MIDI_NUM == 0
  return -1; // IIC MIDI interface not explicitely enabled in mios32_config.h
#else
  // configure RI_N pins as inputs with internal pull-up
  GPIO_InitTypeDef GPIO_InitStructure;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
  GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IPU;

#if MIOS32_IIC_MIDI0_ENABLED == 2
  GPIO_InitStructure.GPIO_Pin   = MIOS32_IIC_MIDI0_RI_N_PIN;
  GPIO_Init(MIOS32_IIC_MIDI0_RI_N_PORT, &GPIO_InitStructure);
#endif
#if MIOS32_IIC_MIDI1_ENABLED == 2
  GPIO_InitStructure.GPIO_Pin   = MIOS32_IIC_MIDI1_RI_N_PIN;
  GPIO_Init(MIOS32_IIC_MIDI1_RI_N_PORT, &GPIO_InitStructure);
#endif
#if MIOS32_IIC_MIDI2_ENABLED == 2
  GPIO_InitStructure.GPIO_Pin   = MIOS32_IIC_MIDI2_RI_N_PIN;
  GPIO_Init(MIOS32_IIC_MIDI2_RI_N_PORT, &GPIO_InitStructure);
#endif
#if MIOS32_IIC_MIDI3_ENABLED == 2
  GPIO_InitStructure.GPIO_Pin   = MIOS32_IIC_MIDI3_RI_N_PIN;
  GPIO_Init(MIOS32_IIC_MIDI3_RI_N_PORT, &GPIO_InitStructure);
#endif
#if MIOS32_IIC_MIDI4_ENABLED == 2
  GPIO_InitStructure.GPIO_Pin   = MIOS32_IIC_MIDI4_RI_N_PIN;
  GPIO_Init(MIOS32_IIC_MIDI4_RI_N_PORT, &GPIO_InitStructure);
#endif
#if MIOS32_IIC_MIDI5_ENABLED == 2
  GPIO_InitStructure.GPIO_Pin   = MIOS32_IIC_MIDI5_RI_N_PIN;
  GPIO_Init(MIOS32_IIC_MIDI5_RI_N_PORT, &GPIO_InitStructure);
#endif
#if MIOS32_IIC_MIDI6_ENABLED == 2
  GPIO_InitStructure.GPIO_Pin   = MIOS32_IIC_MIDI6_RI_N_PIN;
  GPIO_Init(MIOS32_IIC_MIDI6_RI_N_PORT, &GPIO_InitStructure);
#endif
#if MIOS32_IIC_MIDI7_ENABLED == 2
  GPIO_InitStructure.GPIO_Pin   = MIOS32_IIC_MIDI7_RI_N_PIN;
  GPIO_Init(MIOS32_IIC_MIDI7_RI_N_PORT, &GPIO_InitStructure);
#endif

  // initialize IIC interface
  if( MIOS32_IIC_Init(0) < 0 )
    return -1; // initialisation of IIC Interface failed

  // scan for available MBHP_IIC_MIDI modules
  if( MIOS32_IIC_MIDI_ScanInterfaces() < 0 )
    return -2; // we don't expect that any other task accesses the IIC port yet!

  return 0; // no error
#endif
}


/////////////////////////////////////////////////////////////////////////////
// Scans all MBHP_IIC_MIDI modules to check the availablility by sending
// dummy requests and checking the ACK response.
// Per module, this procedure takes at least ca. 25 uS, if no module is 
// connected ca. 75 uS (3 retries), or even more if we have to wait for 
// completion of the previous IIC transfer.
// Therefore this function should only be rarely used (e.g. once per second),
// and the state should be saved somewhere in the application
// IN: -
// OUT: returns 0 if all IIC interfaces scanned
//      returns -2 if IIC interface blocked by another task (retry the scan!)
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
  if( MIOS32_IIC_TransferBegin(IIC_Non_Blocking) < 0 )
    return -2; // request a retry

  u8 iic_port;
  iic_port_available = 0;
  for(iic_port=0; iic_port<MIOS32_IIC_MIDI_NUM; ++iic_port) {
    if( iic_port_enabled[iic_port] ) {
      // try to address the peripheral 3 times
      u8 retries=3;
      s32 error = -1;

      while( error < 0 && retries-- ) {
	s32 error = MIOS32_IIC_Transfer(IIC_Write, MIOS32_IIC_MIDI_ADDR_BASE + 2*iic_port, NULL, 0);
	if( !error )
	  error = MIOS32_IIC_TransferWait();
	if( !error )
	  iic_port_available |= (1 << iic_port);
      }
    }
  }

  // release IIC peripheral
  MIOS32_IIC_TransferFinished();

  return 0; // no error during scan
#endif
}


/////////////////////////////////////////////////////////////////////////////
// This function checks the availability of the MBHP_IIC_MIDI module
// taken from the last results of MIOS32_IIC_MIDI_ScanInterface()
// IN: module number (0..7)
// OUT: 1: interface available
//      0: interface not available
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
      if( MIOS32_IIC_TransferBegin(IIC_Non_Blocking) < 0 )
	return -2; // retry until interface available

      error = MIOS32_IIC_TransferCheck();
      if( error ) {
	MIOS32_IIC_TransferFinished();
	return -2; // retry, regardless if MBHP_IIC_MIDI module is busy or not accessible
      }
      error = MIOS32_IIC_Transfer(IIC_Write, MIOS32_IIC_MIDI_ADDR_BASE + 2*iic_port, buffer, len);
    } else {
      u8 retries = 3; // for blocking mode: retry to access the MBHP_IIC_MIDI module 3 times

      MIOS32_IIC_TransferBegin(IIC_Blocking);

      error = -1;
      while( error < 0 && retries-- ) {
	error = MIOS32_IIC_Transfer(IIC_Write, MIOS32_IIC_MIDI_ADDR_BASE + 2*iic_port, buffer, len);
	if( !error )
	  error = MIOS32_IIC_TransferWait();
      }
    }

    MIOS32_IIC_TransferFinished();

    if( error >= 0 ) {
      MIOS32_MIDI_SendPackageToTxCallback(IIC0 + iic_port, package);
      return 0; // no error
    }

    return -3; // IIC error
  } else {
    return 0; // no bytes to send -> no error
  }
#endif
}


/////////////////////////////////////////////////////////////////////////////
// This function sends a new MIDI package to the selected IIC_MIDI port
// IN: IIC_MIDI module number (0..7) in <iic_port>, MIDI package in <package>
// OUT: 0: no error
//      -1: IIC_MIDI device not available
//      -2: IIC_MIDI buffer is full
//          caller should retry until buffer is free again
//      -3: IIC error during transfer
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_IIC_MIDI_PackageSend_NonBlocking(u8 iic_port, mios32_midi_package_t package)
{
  return _MIOS32_IIC_MIDI_PackageSend(iic_port, package, 1); // non-blocking mode
}


/////////////////////////////////////////////////////////////////////////////
// This function sends a new MIDI package to the selected IIC_MIDI port
// (blocking function)
// IN: IIC_MIDI module number (0..7) in <iic_port>, MIDI package in <package>
// OUT: 0: no error
//      -1: IIC_MIDI device not available
//      -3: IIC error during transfer
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

  u8 buffer[4];

  // request IIC
  if( non_blocking_mode ) {
    if( MIOS32_IIC_TransferBegin(IIC_Non_Blocking) < 0 )
      return -2; // request retry
  } else {
    MIOS32_IIC_TransferBegin(IIC_Blocking);
  }

  s32 error = MIOS32_IIC_Transfer(IIC_Read_AbortIfFirstByteIs0, MIOS32_IIC_MIDI_ADDR_BASE + 2*iic_port, buffer, 4);
  if( !error )
    error = MIOS32_IIC_TransferWait();

  if( !error ) {
    package->type  = buffer[0];
    package->evnt0 = buffer[1];
    package->evnt1 = buffer[2];
    package->evnt2 = buffer[3];

    if( error == 0 )
      MIOS32_MIDI_SendPackageToRxCallback(IIC0 + iic_port, package);

    error = package->type ? 0 : -1; // return 0 if new package is available
  } else {
    error = -3; // IIC error
  }

  // release IIC port
  MIOS32_IIC_TransferFinished();

  return error;
#endif
}


/////////////////////////////////////////////////////////////////////////////
// This function checks for a new package
// IN: IIC_MIDI module number (0..7) in <iic_port>, 
//     pointer to MIDI package in <package> (received package will be put into the given variable)
// OUT: 0: no error
//      -1: no package in buffer
//      -2: IIC interface allocated - retry (only in Non Blocking mode)
//      -3: IIC error during transfer
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_IIC_MIDI_PackageReceive_NonBlocking(u8 iic_port, mios32_midi_package_t *package)
{
  return _MIOS32_IIC_MIDI_PackageReceive(iic_port, package, 1); // non-blocking
}


/////////////////////////////////////////////////////////////////////////////
// This function checks for a new package
// (blocking function)
// IN: IIC_MIDI module number (0..7) in <iic_port>, 
//     pointer to MIDI package in <package> (received package will be put into the given variable)
// OUT: 0: no error
//      -1: no package in buffer
//      -3: IIC error during transfer
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_IIC_MIDI_PackageReceive(u8 iic_port, mios32_midi_package_t *package)
{
  return _MIOS32_IIC_MIDI_PackageReceive(iic_port, package, 0); // blocking
}


/////////////////////////////////////////////////////////////////////////////
// returns inverted state of RI_N pin
// IN: iic_port (0..7) in <iic_port>
// OUT: 1: RI_N active, 0: RI_N not active
//      always 1 if RI_N pin not configured (driver uses polling method in this case)
//      always 0 if invalid IIC port (>= 8)
/////////////////////////////////////////////////////////////////////////////
static s32 MIOS32_IIC_MIDI_GetRI(u8 iic_port)
{
  switch( iic_port ) {
#if MIOS32_IIC_MIDI0_ENABLED == 2
    case 0: return GPIO_ReadInputDataBit(MIOS32_IIC_MIDI0_RI_N_PORT, MIOS32_IIC_MIDI0_RI_N_PIN) ? 0 : 1;
#else
    case 0: return 1;
#endif
#if MIOS32_IIC_MIDI1_ENABLED == 2
    case 1: return GPIO_ReadInputDataBit(MIOS32_IIC_MIDI1_RI_N_PORT, MIOS32_IIC_MIDI1_RI_N_PIN) ? 0 : 1;
#else
    case 1: return 1;
#endif
#if MIOS32_IIC_MIDI2_ENABLED == 2
    case 2: return GPIO_ReadInputDataBit(MIOS32_IIC_MIDI2_RI_N_PORT, MIOS32_IIC_MIDI2_RI_N_PIN) ? 0 : 1;
#else
    case 2: return 1;
#endif
#if MIOS32_IIC_MIDI3_ENABLED == 2
    case 3: return GPIO_ReadInputDataBit(MIOS32_IIC_MIDI3_RI_N_PORT, MIOS32_IIC_MIDI3_RI_N_PIN) ? 0 : 1;
#else
    case 3: return 1;
#endif
#if MIOS32_IIC_MIDI4_ENABLED == 2
    case 4: return GPIO_ReadInputDataBit(MIOS32_IIC_MIDI4_RI_N_PORT, MIOS32_IIC_MIDI4_RI_N_PIN) ? 0 : 1;
#else
    case 4: return 1;
#endif
#if MIOS32_IIC_MIDI5_ENABLED == 2
    case 5: return GPIO_ReadInputDataBit(MIOS32_IIC_MIDI5_RI_N_PORT, MIOS32_IIC_MIDI5_RI_N_PIN) ? 0 : 1;
#else
    case 5: return 1;
#endif
#if MIOS32_IIC_MIDI6_ENABLED == 2
    case 6: return GPIO_ReadInputDataBit(MIOS32_IIC_MIDI6_RI_N_PORT, MIOS32_IIC_MIDI6_RI_N_PIN) ? 0 : 1;
#else
    case 6: return 1;
#endif
#if MIOS32_IIC_MIDI7_ENABLED == 2
    case 7: return GPIO_ReadInputDataBit(MIOS32_IIC_MIDI7_RI_N_PORT, MIOS32_IIC_MIDI7_RI_N_PIN) ? 0 : 1;
#else
    case 7: return 1;
#endif
  }
}

#endif /* MIOS32_DONT_USE_IIC_MIDI */
