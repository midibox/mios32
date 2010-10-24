// $Id$
//! \defgroup MIOS32_COM
//!
//! COM layer functions for MIOS32
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
#include <string.h>
#include <stdarg.h>

// this module can be optionally disabled in a local mios32_config.h file (included from mios32.h)
#if !defined(MIOS32_DONT_USE_COM)


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

static mios32_com_port_t default_port = MIOS32_COM_DEFAULT_PORT;
static mios32_com_port_t debug_port = MIOS32_COM_DEBUG_PORT;

static s32 (*receive_callback_func)(mios32_midi_port_t port, char c);


/////////////////////////////////////////////////////////////////////////////
//! Initializes COM layer
//! \param[in] mode currently only mode 0 supported
//! \return < 0 if initialisation failed
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_COM_Init(u32 mode)
{
  s32 ret = 0;

  // currently only mode 0 supported
  if( mode != 0 )
    return -1; // unsupported mode

  // disable callback by default
  receive_callback_func = NULL;

  // set default/debug port as defined in mios32.h/mios32_config.h
  default_port = MIOS32_COM_DEFAULT_PORT;
  debug_port = MIOS32_COM_DEBUG_PORT;

#if defined(MIOS32_USE_USB_COM)
  if( MIOS32_USB_COM_Init(0) < 0 )
    ret |= (1 << 0);
#endif

  // if any COM assignment:
#if MIOS32_UART0_ASSIGNMENT == 2 || MIOS32_UART1_ASSIGNMENT == 2
  if( MIOS32_UART_Init(0) < 0 )
    ret |= (1 << 1);
#endif

  return -ret;
}


/////////////////////////////////////////////////////////////////////////////
//! This function checks the availability of a COM port
//! \param[in] port COM port (DEFAULT, USB0..USB7, UART0..UART1, IIC0..IIC7)
//! \return 1: port available
//! \return 0: port not available
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_COM_CheckAvailable(mios32_com_port_t port)
{
  // if default/debug port: select mapped port
  if( !(port & 0xf0) ) {
    port = (port == COM_DEBUG) ? debug_port : default_port;
  }

  // branch depending on selected port
  switch( port >> 4 ) {
    case 1:
#if !defined(MIOS32_DONT_USE_USB) && defined(MIOS32_USE_USB_COM)
      return MIOS32_USB_COM_CheckAvailable();
#else
      return 0; // USB has been disabled
#endif

    case 2:
#if !defined(MIOS32_DONT_USE_UART)
      switch( port & 0xf ) {
#if MIOS32_UART_NUM >= 1
        case 0: return MIOS32_UART0_ASSIGNMENT == 2 ? 1 : 0; // UART0 assigned to COM?
#endif
#if MIOS32_UART_NUM >= 2
        case 1: return MIOS32_UART1_ASSIGNMENT == 2 ? 1 : 0; // UART1 assigned to COM?
#endif
      }
#endif
      return 0; // no UART
      
    case 3:
      return 0; // IIC COM not implemented yet (but should be easy)

    case 4:
      return 0; // Ethernet not implemented yet
      
    default:
      // invalid port
      return 0;
  }
}


/////////////////////////////////////////////////////////////////////////////
//! Sends a package over given port
//! \param[in] port COM port (DEFAULT, USB0..USB7, UART0..UART1, IIC0..IIC7)
//! \param[in] buffer character buffer
//! \param[in] len buffer length
//! \return -1 if port not available
//! \return -2 if non-blocking mode activated: buffer is full
//!            caller should retry until buffer is free again
//! \return 0 on success
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_COM_SendBuffer_NonBlocking(mios32_com_port_t port, u8 *buffer, u16 len)
{
  // if default/debug port: select mapped port
  if( !(port & 0xf0) ) {
    port = (port == COM_DEBUG) ? debug_port : default_port;
  }

  // branch depending on selected port
  switch( port >> 4 ) {
    case 1:
#if !defined(MIOS32_DONT_USE_USB) && defined(MIOS32_USE_USB_COM)
      return MIOS32_USB_COM_TxBufferPutMore_NonBlocking(port & 0xf, buffer, len);
#else
      return -1; // USB has been disabled
#endif

    case 2:
#if !defined(MIOS32_DONT_USE_UART)
      return MIOS32_UART_TxBufferPutMore_NonBlocking(port & 0xf, buffer, len);
#else
      return -1; // UART has been disabled
#endif

    case 3:
      return -1; // not implemented yet

    case 4:
      return -1; // Ethernet not implemented yet
      
    default:
      // invalid port
      return -1;
  }
}



/////////////////////////////////////////////////////////////////////////////
//! Sends a package over given port
//! (blocking function)
//! \param[in] port COM port (DEFAULT, USB0..USB7, UART0..UART1, IIC0..IIC7)
//! \param[in] buffer character buffer
//! \param[in] len buffer length
//! \return -1 if port not available
//! \return 0 on success
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_COM_SendBuffer(mios32_com_port_t port, u8 *buffer, u16 len)
{
  // if default/debug port: select mapped port
  if( !(port & 0xf0) ) {
    port = (port == COM_DEBUG) ? debug_port : default_port;
  }

  // branch depending on selected port
  switch( port >> 4 ) {
    case 1:
#if !defined(MIOS32_DONT_USE_USB) && defined(MIOS32_USE_USB_COM)
      return MIOS32_USB_COM_TxBufferPutMore(port & 0xf, buffer, len);
#else
      return -1; // USB has been disabled
#endif

    case 2:
#if !defined(MIOS32_DONT_USE_UART)
      return MIOS32_UART_TxBufferPutMore(port & 0xf, buffer, len);
#else
      return -1; // UART has been disabled
#endif

    case 3:
      return -1; // not implemented yet

    case 4:
      return -1; // Ethernet not implemented yet
      
    default:
      // invalid port
      return -1;
  }
}


/////////////////////////////////////////////////////////////////////////////
//! Sends a single character over given port
//! \param[in] port COM port (DEFAULT, USB0..USB7, UART0..UART1, IIC0..IIC7)
//! \param[in] c character
//! \return -1 if port not available
//! \return -2 buffer is full
//!            caller should retry until buffer is free again
//! \return 0 on success
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_COM_SendChar_NonBlocking(mios32_com_port_t port, char c)
{
  return MIOS32_COM_SendBuffer_NonBlocking(port, (u8 *)&c, 1);
}


/////////////////////////////////////////////////////////////////////////////
//! Sends a single character over given port
//! (blocking function)
//! \param[in] port COM port (DEFAULT, USB0..USB7, UART0..UART1, IIC0..IIC7)
//! \param[in] c character
//! \return -1 if port not available
//! \return 0 on success
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_COM_SendChar(mios32_com_port_t port, char c)
{
  return MIOS32_COM_SendBuffer(port, (u8 *)&c, 1);
}


/////////////////////////////////////////////////////////////////////////////
//! Sends a string over given port
//! \param[in] port COM port (DEFAULT, USB0..USB7, UART0..UART1, IIC0..IIC7)
//! \param[in] str zero-terminated string
//! \return -1 if port not available
//! \return -2 buffer is full
//!         caller should retry until buffer is free again
//! \return 0 on success
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_COM_SendString_NonBlocking(mios32_com_port_t port, char *str)
{
  return MIOS32_COM_SendBuffer_NonBlocking(port, (u8 *)str, (u16)strlen(str));
}


/////////////////////////////////////////////////////////////////////////////
//! Sends a string over given port
//! (blocking function)
//! \param[in] port COM port (DEFAULT, USB0..USB7, UART0..UART1, IIC0..IIC7)
//! \param[in] str zero-terminated string
//! \return -1 if port not available
//! \return 0 on success
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_COM_SendString(mios32_com_port_t port, char *str)
{
  return MIOS32_COM_SendBuffer(port, (u8 *)str, strlen(str));
}


/////////////////////////////////////////////////////////////////////////////
//! Sends a formatted string (-> printf) over given port
//! \param[in] port COM port (DEFAULT, USB0..USB7, UART0..UART1, IIC0..IIC7)
//! \param[in] *format zero-terminated format string - 128 characters supported maximum!
//! \param[in] ... optional arguments,
//!        128 characters supported maximum!
//! \return -2 if non-blocking mode activated: buffer is full
//!         caller should retry until buffer is free again
//! \return 0 on success
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_COM_SendFormattedString_NonBlocking(mios32_com_port_t port, char *format, ...)
{
  u8 buffer[128]; // TODO: tmp!!! Provide a streamed COM method later!
  va_list args;

  va_start(args, format);
  vsprintf((char *)buffer, format, args);
  return MIOS32_COM_SendBuffer_NonBlocking(port, buffer, (u16)strlen((char *)buffer));
}


/////////////////////////////////////////////////////////////////////////////
//! Sends a formatted string (-> printf) over given port
//! (blocking function)
//! \param[in] port COM port (DEFAULT, USB0..USB7, UART0..UART1, IIC0..IIC7)
//! \param[in] *format zero-terminated format string - 128 characters supported maximum!
//! \param[in] ... optional arguments,
//! \return -1 if port not available
//! \return 0 on success
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_COM_SendFormattedString(mios32_com_port_t port, char *format, ...)
{
  u8 buffer[128]; // TODO: tmp!!! Provide a streamed COM method later!
  va_list args;

  va_start(args, format);
  vsprintf((char *)buffer, format, args);
  return MIOS32_COM_SendBuffer(port, buffer, (u16)strlen((char *)buffer));
}



/////////////////////////////////////////////////////////////////////////////
//! Checks for incoming COM messages, calls the callback function which has
//! been installed via MIOS32_COM_ReceiveCallback_Init()
//! 
//! Not for use in an application - this function is called by
//! by a task in the programming model!
//! 
//! \return < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_COM_Receive_Handler(void)
{
  u8 port = DEFAULT;

  u8 intf = 0; // interface to be checked
  u8 total_bytes_forwarded = 0; // number of forwards - stop after 10 forwards to yield some CPU time for other tasks
  u8 bytes_forwarded = 0;
  u8 again = 1;
  do {
    // Round Robin
    // TODO: maybe a list based approach would be better
    // it would allow to add/remove interfaces dynamically
    // this would also allow to give certain ports a higher priority (to add them multiple times to the list)
    // it would also improve this spagetthi code ;)
    s32 status = -1;
    switch( intf++ ) {
#if !defined(MIOS32_DONT_USE_USB) && defined(MIOS32_USE_USB_COM)
      case 0: status = MIOS32_USB_COM_RxBufferGet(0); port = USB0; break;
#else
      case 0: status = -1; break;
#endif
#if !defined(MIOS32_DONT_USE_UART) && MIOS32_UART0_ASSIGNMENT == 2
      case 1: status = MIOS32_UART_RxBufferGet(0); port = UART0; break;
#else
      case 1: status = -1; break;
#endif
#if !defined(MIOS32_DONT_USE_UART) && MIOS32_UART1_ASSIGNMENT == 2
      case 2: status = MIOS32_UART_RxBufferGet(1); port = UART1; break;
#else
      case 2: status = -1; break;
#endif
      default:
	// allow 64 forwards maximum to yield some CPU time for other tasks
	if( bytes_forwarded && total_bytes_forwarded < 64 ) {
	  intf = 0; // restart with USB
	  bytes_forwarded = 0; // for checking, if bytes still have been forwarded in next round
	} else {
	  again = 0; // no more interfaces to be processed
	}
	status = -1; // empty round - no message
    }

    // message received?
    if( status >= 0 ) {
      // notify that a package has been forwarded
      ++bytes_forwarded;
      ++total_bytes_forwarded;

      // call function
      if( receive_callback_func != NULL )
	receive_callback_func(port, (u8)status);
    }
  } while( again );

  return 0;
}


/////////////////////////////////////////////////////////////////////////////
//! Installs the callback function which is executed on incoming characters
//! from a COM interface.
//!
//! Example:
//! \code
//! s32 CONSOLE_Parse(mios32_com_port_t port, char c)
//! {
//!   // see $MIOS32_PATH/apps/examples/com_console/
//!   
//!   return 0; // no error
//! }
//! \endcode
//!
//! The callback function has been installed in an Init() function with:
//! \code
//!   MIOS32_COM_ReceiveCallback_Init(CONSOLE_Parse);
//! \endcode
//! \param[in] callback_debug_command the callback function (NULL disables the callback)
//! \return < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_COM_ReceiveCallback_Init(void *callback_receive)
{
  receive_callback_func = callback_receive;

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
//! This function allows to change the DEFAULT port.<BR>
//! The preset which will be used after application reset can be set in
//! mios32_config.h via "#define MIOS32_COM_DEFAULT_PORT <port>".<BR>
//! It's set to USB0 so long not overruled in mios32_config.h
//! \param[in] port COM port (USB0..USB7, UART0..UART1, IIC0..IIC7)
//! \return < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_COM_DefaultPortSet(mios32_com_port_t port)
{
  if( port == (mios32_com_port_t)DEFAULT ) // avoid recursion
    return -1;

  default_port = port;

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
//! This function returns the DEFAULT port
//! \return the default port
/////////////////////////////////////////////////////////////////////////////
mios32_com_port_t MIOS32_COM_DefaultPortGet(void)
{
  return default_port;
}

/////////////////////////////////////////////////////////////////////////////
//! This function allows to change the COM_DEBUG port.<BR>
//! The preset which will be used after application reset can be set in
//! mios32_config.h via "#define MIOS32_COM_DEBUG_PORT <port>".<BR>
//! It's set to USB0 so long not overruled in mios32_config.h
//! \param[in] port COM port (USB0..USB7, UART0..UART1, IIC0..IIC7)
//! \return < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_COM_DebugPortSet(mios32_com_port_t port)
{
  if( port == COM_DEBUG ) // avoid recursion
    return -1;

  debug_port = port;

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
//! This function returns the COM_DEBUG port
//! \return the debug port
/////////////////////////////////////////////////////////////////////////////
mios32_com_port_t MIOS32_COM_DebugPortGet(void)
{
  return debug_port;
}

//! \}

#endif /* MIOS32_DONT_USE_COM */
