// $Id$
//! \defgroup BLM_SCALAR_MASTER
//!
//! BLM_SCALAR_MASTER Driver
//!
//! \{
/* ==========================================================================
 *
 *  Copyright (C) 2016 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

/////////////////////////////////////////////////////////////////////////////
//! Include files
/////////////////////////////////////////////////////////////////////////////

#include <mios32.h>

#if BLM_SCALAR_MASTER_OSC_SUPPORT
#include <osc_server.h>
#endif

#include "blm_scalar_master.h"

/////////////////////////////////////////////////////////////////////////////
//! Local definitions
/////////////////////////////////////////////////////////////////////////////

// ack/disack code
#define SYSEX_DISACK   0x0e
#define SYSEX_ACK      0x0f

// disacknowledge arguments
#define SYSEX_DISACK_LESS_BYTES_THAN_EXP  0x01
#define SYSEX_DISACK_MORE_BYTES_THAN_EXP  0x02
#define SYSEX_DISACK_INVALID_COMMAND      0x0c


#define SYSEX_BLM_CMD_REQUEST      0x00
#define SYSEX_BLM_CMD_LAYOUT       0x01

// timeout after 10 seconds (timeout counter is incremented each mS)
#define BLM_TIMEOUT_RELOAD_VALUE 10000

// optimized transfer: how many MIDI packets should be bundled?
#define BLM_MAX_PACKETS 8


/////////////////////////////////////////////////////////////////////////////
//! Local types
/////////////////////////////////////////////////////////////////////////////

// command states
typedef enum {
  SYSEX_CMD_STATE_BEGIN,
  SYSEX_CMD_STATE_CONT,
  SYSEX_CMD_STATE_END
} sysex_cmd_state_t;

typedef union {
  struct {
    unsigned ALL:32;
  };

  struct {
    unsigned CTR:3;
    unsigned MY_SYSEX:1;
    unsigned CMD:1;
  } general;

  struct {
    unsigned CTR:3;
    unsigned MY_SYSEX:1;
    unsigned CMD:1;
    unsigned PING_BYTE_RECEIVED:1;
  } ping;

  struct {
    unsigned CTR:3;
    unsigned MY_SYSEX:1;
    unsigned CMD:1;
    unsigned COLUMNS_RECEIVED:1;
    unsigned ROWS_RECEIVED:1;
    unsigned COLOURS_RECEIVED:1;
  } blm;

} sysex_state_t;


/////////////////////////////////////////////////////////////////////////////
//! Local variables
/////////////////////////////////////////////////////////////////////////////


static mios32_midi_port_t blm_midi_port;
static blm_scalar_master_connection_state_t blm_connection_state;
static u16 blm_timeout_ctr;


static const u8 blm_sysex_header[5] = { 0xf0, 0x00, 0x00, 0x7e, 0x4e }; // Header of MBHP_BLM_SCALAR

static sysex_state_t sysex_state;
static u8 sysex_device_id;
static u8 sysex_cmd;

static u16 blm_scalar_master_leds_green_sent[BLM_SCALAR_MASTER_NUM_ROWS];
static u16 blm_scalar_master_leds_red_sent[BLM_SCALAR_MASTER_NUM_ROWS];

static u16 blm_scalar_master_leds_extracolumn_green_sent;
static u16 blm_scalar_master_leds_extracolumn_red_sent;
static u16 blm_scalar_master_leds_extracolumn_shift_green_sent;
static u16 blm_scalar_master_leds_extracolumn_shift_red_sent;
static u16 blm_scalar_master_leds_extrarow_green_sent;
static u16 blm_scalar_master_leds_extrarow_red_sent;
static u8  blm_scalar_master_leds_extra_green_sent;
static u8  blm_scalar_master_leds_extra_red_sent;

static u8 blm_leds_rotate_view;
static u8 blm_led_row_offset;

static u8 blm_num_columns;
static u8 blm_num_rows;
static u8 blm_num_colours;
static u8 blm_force_update;

static s32 (*blm_button_callback_func)(u8 blm, blm_scalar_master_element_t element_id, u8 button_x, u8 button_y, u8 button_depressed);
static s32 (*blm_fader_callback_func)(u8 blm, u8 fader, u8 value);


/////////////////////////////////////////////////////////////////////////////
//! Global variables
/////////////////////////////////////////////////////////////////////////////

// for direct access
u16 blm_scalar_master_leds_green[BLM_SCALAR_MASTER_NUM_ROWS];
u16 blm_scalar_master_leds_red[BLM_SCALAR_MASTER_NUM_ROWS];

u16 blm_scalar_master_leds_extracolumn_green;
u16 blm_scalar_master_leds_extracolumn_red;
u16 blm_scalar_master_leds_extracolumn_shift_green;
u16 blm_scalar_master_leds_extracolumn_shift_red;
u16 blm_scalar_master_leds_extrarow_green;
u16 blm_scalar_master_leds_extrarow_red;
u8  blm_scalar_master_leds_extra_green;
u8  blm_scalar_master_leds_extra_red;


/////////////////////////////////////////////////////////////////////////////
//! Local prototypes
/////////////////////////////////////////////////////////////////////////////

static s32 BLM_SCALAR_MASTER_SYSEX_CmdFinished(void);
static s32 BLM_SCALAR_MASTER_SYSEX_Cmd(mios32_midi_port_t port, sysex_cmd_state_t cmd_state, u8 midi_in);
static s32 BLM_SCALAR_MASTER_SYSEX_Cmd_Layout(mios32_midi_port_t port, sysex_cmd_state_t cmd_state, u8 midi_in);
static s32 BLM_SCALAR_MASTER_SYSEX_Cmd_Ping(mios32_midi_port_t port, sysex_cmd_state_t cmd_state, u8 midi_in);
static s32 BLM_SCALAR_MASTER_SYSEX_SendAck(mios32_midi_port_t port, u8 ack_code, u8 ack_arg);

static s32 BLM_SendPackets(mios32_midi_package_t *packets, u8 num_packets);


/////////////////////////////////////////////////////////////////////////////
//! Initializes the BLM_SCALAR_MASTER driver
//! Should be called from Init() during startup
//! \param[in] mode currently only mode 0 supported
//! \return < 0 if initialisation failed
/////////////////////////////////////////////////////////////////////////////
s32 BLM_SCALAR_MASTER_Init(u32 mode)
{
  // currently only mode 0 supported
  if( mode != 0 )
    return -1; // unsupported mode

  blm_midi_port = 0; // disabled by default, set it via BLM_SCALAR_MASTER_MIDI_PortSet();
  blm_connection_state = BLM_SCALAR_MASTER_CONNECTION_STATE_IDLE;
  blm_num_columns = 16;
  blm_num_rows = 16;
  blm_num_colours = 2;
  blm_force_update = 0;
  blm_leds_rotate_view = 0;
  blm_led_row_offset = 0;

  sysex_device_id = 0; // only device 0 supported yet

  blm_button_callback_func = NULL;
  blm_fader_callback_func = NULL;

  return 0;
}

/////////////////////////////////////////////////////////////////////////////
//! Sets the IN/OUT port for MBHP_BLM_SCALAR communication
/////////////////////////////////////////////////////////////////////////////
s32 BLM_SCALAR_MASTER_MIDI_PortSet(u8 blm, mios32_midi_port_t port)
{
  blm_midi_port = port;
  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
//! Returns the currently configured MIDI IN/OUT port for MBHP_BLM_SCALAR
//! communication
//! \return 0 no communication
//! \return the MIDI port if != 0
/////////////////////////////////////////////////////////////////////////////
mios32_midi_port_t BLM_SCALAR_MASTER_MIDI_PortGet(u8 blm)
{
  return blm_midi_port;
}


/////////////////////////////////////////////////////////////////////////////
//! Returns connection state to MBHP_BLM_SCALAR:
//! \return 0 no contact yet
//! \return 1 contact via MIDI protocol
//! \return 2 contact via simplified Lemur protocol
/////////////////////////////////////////////////////////////////////////////
blm_scalar_master_connection_state_t BLM_SCALAR_MASTER_ConnectionStateGet(u8 blm)
{
  return blm_connection_state;
}


/////////////////////////////////////////////////////////////////////////////
//! Installs callback function for BLM16x16 buttons
/////////////////////////////////////////////////////////////////////////////
s32 BLM_SCALAR_MASTER_ButtonCallback_Init(s32 (*button_callback_func)(u8 blm, blm_scalar_master_element_t element_id, u8 button_x, u8 button_y, u8 button_depressed))
{
  blm_button_callback_func = button_callback_func;
  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! Installs callback function for faders
/////////////////////////////////////////////////////////////////////////////
s32 BLM_SCALAR_MASTER_FaderCallback_Init(s32 (*fader_callback_func)(u8 blm, u8 fader, u8 value))
{
  blm_fader_callback_func = fader_callback_func;
  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! Sets a LED to the given colour
/////////////////////////////////////////////////////////////////////////////
s32 BLM_SCALAR_MASTER_LED_Set(u8 blm, blm_scalar_master_element_t element_id, u8 led_x, u8 led_y, blm_scalar_master_colour_t colour)
{
  u8 colour_green = (u8)colour & 1; // special colour encoding
  u8 colour_red = (u8)colour & 2; // special colour encoding
  u32 x_mask = 1 << led_x;
  u32 y_mask = 1 << led_y;

  switch( element_id ) {
  case BLM_SCALAR_MASTER_ELEMENT_GRID: {
    if( led_y < BLM_SCALAR_MASTER_NUM_ROWS ) {
      if( colour_green )
	blm_scalar_master_leds_green[led_y] |= x_mask;
      else
	blm_scalar_master_leds_green[led_y] &= ~x_mask;

      if( colour_red )
	blm_scalar_master_leds_red[led_y] |= x_mask;
      else
	blm_scalar_master_leds_red[led_y] &= ~x_mask;
    }
  } break;

  case BLM_SCALAR_MASTER_ELEMENT_EXTRA_ROW: {
    if( colour_green )
      blm_scalar_master_leds_extrarow_green |= x_mask;
    else
      blm_scalar_master_leds_extrarow_green &= ~x_mask;

    if( colour_red )
      blm_scalar_master_leds_extrarow_red |= x_mask;
    else
      blm_scalar_master_leds_extrarow_red &= ~x_mask;
  } break;

  case BLM_SCALAR_MASTER_ELEMENT_EXTRA_COLUMN: {
    if( colour_green )
      blm_scalar_master_leds_extracolumn_green |= y_mask;
    else
      blm_scalar_master_leds_extracolumn_green &= ~y_mask;
    
    if( colour_red )
      blm_scalar_master_leds_extracolumn_red |= y_mask;
    else
      blm_scalar_master_leds_extracolumn_red &= ~y_mask;
  } break;

  case BLM_SCALAR_MASTER_ELEMENT_SHIFT: {
    if( colour_green )
      blm_scalar_master_leds_extra_green |= y_mask;
    else
      blm_scalar_master_leds_extra_green &= ~y_mask;

    if( colour_red )
      blm_scalar_master_leds_extra_red |= y_mask;
    else
      blm_scalar_master_leds_extra_red &= ~y_mask;
  } break;
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! Returns the current colour of a LED
/////////////////////////////////////////////////////////////////////////////
blm_scalar_master_colour_t BLM_SCALAR_MASTER_LED_Get(u8 blm, blm_scalar_master_element_t element_id, u8 led_x, u8 led_y)
{
  u32 x_mask = 1 << led_x;
  u32 y_mask = 1 << led_y;

  switch( element_id ) {
  case BLM_SCALAR_MASTER_ELEMENT_GRID: {
    if( led_y < BLM_SCALAR_MASTER_NUM_ROWS ) {
      u8 colour = 0;
      if( blm_scalar_master_leds_green[led_y] & x_mask )
	colour |= 1;
      if( blm_scalar_master_leds_red[led_y] & x_mask )
	colour |= 2;
      return (blm_scalar_master_colour_t)colour;
    }
  } break;

  case BLM_SCALAR_MASTER_ELEMENT_EXTRA_ROW: {
    u8 colour = 0;
    if( blm_scalar_master_leds_extrarow_green & x_mask )
      colour |= 1;
    if( blm_scalar_master_leds_extrarow_red & x_mask )
      colour |= 2;
    return (blm_scalar_master_colour_t)colour;
  } break;

  case BLM_SCALAR_MASTER_ELEMENT_EXTRA_COLUMN: {
    u8 colour = 0;
    if( blm_scalar_master_leds_extracolumn_green & y_mask )
      colour |= 1;
    if( blm_scalar_master_leds_extracolumn_red & y_mask )
      colour |= 2;
    return (blm_scalar_master_colour_t)colour;
  } break;

  case BLM_SCALAR_MASTER_ELEMENT_SHIFT: {
    u8 colour = 0;
    if( blm_scalar_master_leds_extra_green & y_mask )
      colour |= 1;
    if( blm_scalar_master_leds_extra_red & y_mask )
      colour |= 2;
    return (blm_scalar_master_colour_t)colour;
  } break;
  }

  return BLM_SCALAR_MASTER_COLOUR_OFF;
}


/////////////////////////////////////////////////////////////////////////////
//! Rotates the Screen by 90 grad
/////////////////////////////////////////////////////////////////////////////
s32 BLM_SCALAR_MASTER_RotateViewSet(u8 blm, u8 rotate_view)
{
  blm_leds_rotate_view = rotate_view;
  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
//! Returns != 0 if screen currently rotated by 90 grad
/////////////////////////////////////////////////////////////////////////////
s32 BLM_SCALAR_MASTER_RotateViewGet(u8 blm)
{
  return blm_leds_rotate_view;
}


/////////////////////////////////////////////////////////////////////////////
//! Sets a row offsets when sending the display data for smaller BLM screens
/////////////////////////////////////////////////////////////////////////////
s32 BLM_SCALAR_MASTER_RowOffsetSet(u8 blm, u8 row_offset)
{
  blm_led_row_offset = row_offset;
  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
//! Returns the current row offset
/////////////////////////////////////////////////////////////////////////////
s32 BLM_SCALAR_MASTER_RowOffsetGet(u8 blm)
{
  return blm_led_row_offset;
}


/////////////////////////////////////////////////////////////////////////////
//! Returns the number of columns as reported by the BLM during layout request
/////////////////////////////////////////////////////////////////////////////
s32 BLM_SCALAR_MASTER_NumColumnsGet(u8 blm)
{
  return blm_num_columns;
}

/////////////////////////////////////////////////////////////////////////////
//! Returns the number of columns as reported by the BLM during layout request
/////////////////////////////////////////////////////////////////////////////
s32 BLM_SCALAR_MASTER_NumRowsGet(u8 blm)
{
  return blm_num_rows;
}

/////////////////////////////////////////////////////////////////////////////
//! Returns the number of colours as reported by the BLM during layout request
/////////////////////////////////////////////////////////////////////////////
s32 BLM_SCALAR_MASTER_NumColoursGet(u8 blm)
{
  return blm_num_colours;
}


/////////////////////////////////////////////////////////////////////////////
//! Directly sets the timeout counter, e.g. to 0 so that the BLM will be
//! disabled until a newly selected port receives a connection request
/////////////////////////////////////////////////////////////////////////////
s32 BLM_SCALAR_MASTER_TimeoutCtrSet(u8 blm, u16 ctr)
{
  blm_timeout_ctr = 0;
  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
//! \return the current timeout counter
/////////////////////////////////////////////////////////////////////////////
s32 BLM_SCALAR_MASTER_TimeoutCtrGet(u8 blm)
{
  return blm_timeout_ctr;
}


/////////////////////////////////////////////////////////////////////////////
//! Enforces a display update
/////////////////////////////////////////////////////////////////////////////
s32 BLM_SCALAR_MASTER_ForceDisplayUpdate(u8 blm)
{
  blm_force_update = 1;
  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! Receives a MIDI package from APP_NotifyReceivedEvent (-> app.c) if port
//! matches with blm_midi_port
/////////////////////////////////////////////////////////////////////////////
s32 BLM_SCALAR_MASTER_MIDI_Receive(mios32_midi_port_t port, mios32_midi_package_t midi_package)
{

  if( !blm_midi_port )
    return -1; // MIDI In not configured

  // extra for Lemur via OSC: simplified handshaking
  if( (port & 0xf0) == OSC0 ) {
    if( midi_package.event == CC && midi_package.chn == Chn16 && midi_package.cc_number == 0x7f ) {
      switch( midi_package.value ) {
      case 0x01: { // Layout
	// change connection state
	blm_connection_state = BLM_SCALAR_MASTER_CONNECTION_STATE_LEMUR;

	// update BLM
	blm_force_update = 1;

	// send acknowledge
	MIOS32_MIDI_SendCC(port, Chn16, 0x7f, 0x7f);

	// and reload timeout counter
	blm_timeout_ctr = BLM_TIMEOUT_RELOAD_VALUE;
      } break;

      case 0x0f: { // Ping
	// send acknowledge
	MIOS32_MIDI_SendCC(port, Chn16, 0x7f, 0x7f);

	// reload timeout counter
	blm_timeout_ctr = BLM_TIMEOUT_RELOAD_VALUE;
      } break;
      }
    }
  }

  // ignore any event on timeout
  if( !blm_timeout_ctr )
    return -1;

  // decode buttons/faders and call callback functions
  // for easier parsing: convert Note Off -> Note On with velocity 0
  if( midi_package.event == NoteOff ) {
    midi_package.event = NoteOn;
    midi_package.velocity = 0;
  }

  if( midi_package.event == NoteOn ) {

    if( midi_package.note < 0x40) { // 0x00..0x3f
      // 16x16 grid
      if( blm_button_callback_func ) {
	blm_button_callback_func(sysex_device_id, BLM_SCALAR_MASTER_ELEMENT_GRID, midi_package.note, midi_package.chn, midi_package.velocity ? 0 : 1);
      }
    } else if( midi_package.note < 0x60 ) { // 0x40..0x5f
      if( blm_button_callback_func ) {
	blm_button_callback_func(sysex_device_id, BLM_SCALAR_MASTER_ELEMENT_EXTRA_COLUMN, midi_package.note - 0x40, midi_package.chn, midi_package.velocity ? 0 : 1);
      }
    } else { // 0x60..0x7f
      if( blm_button_callback_func ) {
	if( midi_package.chn == 0xf ) {
	  blm_button_callback_func(sysex_device_id, BLM_SCALAR_MASTER_ELEMENT_SHIFT, midi_package.note - 0x60, 0, midi_package.velocity ? 0 : 1);
	} else {
	  blm_button_callback_func(sysex_device_id, BLM_SCALAR_MASTER_ELEMENT_EXTRA_ROW, midi_package.note - 0x60, midi_package.chn, midi_package.velocity ? 0 : 1);
	}
      }
    }

  } else if( midi_package.event == CC ) {
    if( blm_fader_callback_func ) {
      blm_fader_callback_func(sysex_device_id, midi_package.chn, midi_package.value);
    }
  }

  return 0;

}

/////////////////////////////////////////////////////////////////////////////
//! This function is called from NOTIFY_MIDI_TimeOut() in app.c if the 
//! MIDI parser runs into timeout
/////////////////////////////////////////////////////////////////////////////
s32 BLM_SCALAR_MASTER_MIDI_TimeOut(mios32_midi_port_t port)
{
  // if we receive a SysEx command (MY_SYSEX flag set), abort parser if port matches
  if( sysex_state.general.MY_SYSEX )
    BLM_SCALAR_MASTER_SYSEX_CmdFinished();

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! This function parses an incoming sysex stream for MIOS32 commands
//! Called from SysEx hook in app.c
/////////////////////////////////////////////////////////////////////////////
s32 BLM_SCALAR_MASTER_SYSEX_Parser(mios32_midi_port_t port, u8 midi_in)
{
  if( !blm_midi_port )
    return -1; // MIDI In not configured

  // check for MIDI port
  if( port != blm_midi_port )
    return -2;

  // ignore realtime messages (see MIDI spec - realtime messages can
  // always be injected into events/streams, and don't change the running status)
  if( midi_in >= 0xf8 )
    return 0;

  // branch depending on state
  if( !sysex_state.general.MY_SYSEX ) {
    if( (sysex_state.general.CTR < sizeof(blm_sysex_header) && midi_in != blm_sysex_header[sysex_state.general.CTR]) ||
	(sysex_state.general.CTR == sizeof(blm_sysex_header) && midi_in != sysex_device_id) ) {
      // incoming byte doesn't match
      BLM_SCALAR_MASTER_SYSEX_CmdFinished();
    } else {
      if( ++sysex_state.general.CTR > sizeof(blm_sysex_header) ) {
	// complete header received, waiting for data
	sysex_state.general.MY_SYSEX = 1;
      }
    }
  } else {
    // check for end of SysEx message or invalid status byte
    if( midi_in >= 0x80 ) {
      if( midi_in == 0xf7 && sysex_state.general.CMD ) {
      	BLM_SCALAR_MASTER_SYSEX_Cmd(port, SYSEX_CMD_STATE_END, midi_in);
      }
      BLM_SCALAR_MASTER_SYSEX_CmdFinished();
    } else {
      // check if command byte has been received
      if( !sysex_state.general.CMD ) {
	sysex_state.general.CMD = 1;
	sysex_cmd = midi_in;
	BLM_SCALAR_MASTER_SYSEX_Cmd(port, SYSEX_CMD_STATE_BEGIN, midi_in);
      }
      else
	BLM_SCALAR_MASTER_SYSEX_Cmd(port, SYSEX_CMD_STATE_CONT, midi_in);
    }
  }

  return 1; // don't forward package to APP_MIDI_NotifyPackage()
}

/////////////////////////////////////////////////////////////////////////////
//! This function is called at the end of a sysex command or on 
//! an invalid message
/////////////////////////////////////////////////////////////////////////////
static s32 BLM_SCALAR_MASTER_SYSEX_CmdFinished(void)
{
  // clear all status variables
  sysex_state.ALL = 0;
  sysex_cmd = 0;

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
//! This function handles the sysex commands
/////////////////////////////////////////////////////////////////////////////
static s32 BLM_SCALAR_MASTER_SYSEX_Cmd(mios32_midi_port_t port, sysex_cmd_state_t cmd_state, u8 midi_in)
{
  switch( sysex_cmd ) {
    case SYSEX_BLM_CMD_REQUEST: // ignore to avoid loopbacks
      break;

    case SYSEX_BLM_CMD_LAYOUT:
      BLM_SCALAR_MASTER_SYSEX_Cmd_Layout(port, cmd_state, midi_in);
      break;

    case 0x0e: // ignore to avoid loopbacks
      break;

    case 0x0f:
      BLM_SCALAR_MASTER_SYSEX_Cmd_Ping(port, cmd_state, midi_in);
      break;

    default:
      // unknown command
      BLM_SCALAR_MASTER_SYSEX_SendAck(port, MIOS32_MIDI_SYSEX_DISACK, MIOS32_MIDI_SYSEX_DISACK_INVALID_COMMAND);
      BLM_SCALAR_MASTER_SYSEX_CmdFinished();      
  }

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
//! Remote Command Handler
/////////////////////////////////////////////////////////////////////////////
static s32 BLM_SCALAR_MASTER_SYSEX_Cmd_Layout(mios32_midi_port_t port, sysex_cmd_state_t cmd_state, u8 midi_in)
{
  switch( cmd_state ) {

    case SYSEX_CMD_STATE_BEGIN:
      break;

    case SYSEX_CMD_STATE_CONT:
      if( !sysex_state.blm.COLUMNS_RECEIVED ) {
	sysex_state.blm.COLUMNS_RECEIVED = 1;
	blm_num_columns = midi_in;
	if( blm_num_columns >= BLM_SCALAR_MASTER_NUM_COLUMNS ) // limit to provided number of steps
	  blm_num_columns = BLM_SCALAR_MASTER_NUM_COLUMNS;
      } else if( !sysex_state.blm.ROWS_RECEIVED ) {
	sysex_state.blm.ROWS_RECEIVED = 1;
	blm_num_rows = midi_in;
	if( blm_num_rows >= BLM_SCALAR_MASTER_NUM_ROWS ) // limit to provided number of tracks
	  blm_num_rows = BLM_SCALAR_MASTER_NUM_ROWS;
      } else if( !sysex_state.blm.COLOURS_RECEIVED ) {
	sysex_state.blm.COLOURS_RECEIVED = 1;
	blm_num_colours = midi_in;
      }
      // ignore all other bytes
      // don't sent error message to allow future extensions
      break;

    default: // SYSEX_CMD_STATE_END
      // change connection state
      blm_connection_state = BLM_SCALAR_MASTER_CONNECTION_STATE_SYSEX;

      // update BLM
      blm_force_update = 1;

      // send acknowledge
      BLM_SCALAR_MASTER_SYSEX_SendAck(port, MIOS32_MIDI_SYSEX_ACK, 0x00);

      // and reload timeout counter
      blm_timeout_ctr = BLM_TIMEOUT_RELOAD_VALUE;
      break;
  }

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
//! Command 0F: Ping (just send back acknowledge if no additional byte has been received)
/////////////////////////////////////////////////////////////////////////////
static s32 BLM_SCALAR_MASTER_SYSEX_Cmd_Ping(mios32_midi_port_t port, sysex_cmd_state_t cmd_state, u8 midi_in)
{
  switch( cmd_state ) {

    case MIOS32_MIDI_SYSEX_CMD_STATE_BEGIN:
      sysex_state.ping.PING_BYTE_RECEIVED = 0;
      break;

    case MIOS32_MIDI_SYSEX_CMD_STATE_CONT:
      sysex_state.ping.PING_BYTE_RECEIVED = 1;
      break;

    default: // MIOS32_MIDI_SYSEX_CMD_STATE_END
      // send acknowledge if no additional byte has been received
      // to avoid feedback loop if two cores are directly connected
      if( !sysex_state.ping.PING_BYTE_RECEIVED )
	BLM_SCALAR_MASTER_SYSEX_SendAck(port, MIOS32_MIDI_SYSEX_ACK, 0x00);

      // and reload timeout counter
      blm_timeout_ctr = BLM_TIMEOUT_RELOAD_VALUE;

      break;
  }

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
//! This function sends a SysEx acknowledge to notify the user about the received command
//! expects acknowledge code (e.g. 0x0f for good, 0x0e for error) and additional argument
/////////////////////////////////////////////////////////////////////////////
static s32 BLM_SCALAR_MASTER_SYSEX_SendAck(mios32_midi_port_t port, u8 ack_code, u8 ack_arg)
{
  u8 sysex_buffer[32]; // should be enough?
  u8 *sysex_buffer_ptr = &sysex_buffer[0];
  int i;

  for(i=0; i<sizeof(blm_sysex_header); ++i)
    *sysex_buffer_ptr++ = blm_sysex_header[i];

  // device ID
  *sysex_buffer_ptr++ = sysex_device_id;

  // send ack code and argument
  *sysex_buffer_ptr++ = ack_code;
  *sysex_buffer_ptr++ = ack_arg;

  // send footer
  *sysex_buffer_ptr++ = 0xf7;

  // finally send SysEx stream
  BLM_SCALAR_MASTER_MUTEX_MIDIOUT_TAKE;
  s32 status = MIOS32_MIDI_SendSysEx(port, (u8 *)sysex_buffer, (u32)sysex_buffer_ptr - ((u32)&sysex_buffer[0]));
  BLM_SCALAR_MASTER_MUTEX_MIDIOUT_GIVE;
  return status;
}


/////////////////////////////////////////////////////////////////////////////
//! This function is called to request the layout
/////////////////////////////////////////////////////////////////////////////
s32 BLM_SCALAR_MASTER_SendRequest(u8 blm, u8 req)
{
  u8 sysex_buffer[32]; // should be enough?
  u8 *sysex_buffer_ptr = &sysex_buffer[0];
  int i;

  if( !blm_midi_port )
    return -1; // MIDI Out not configured

  for(i=0; i<sizeof(blm_sysex_header); ++i)
    *sysex_buffer_ptr++ = blm_sysex_header[i];

  // device ID
  *sysex_buffer_ptr++ = sysex_device_id;

  // send request
  *sysex_buffer_ptr++ = SYSEX_BLM_CMD_REQUEST;
  *sysex_buffer_ptr++ = req;

  // send footer
  *sysex_buffer_ptr++ = 0xf7;

  // finally send SysEx stream
  BLM_SCALAR_MASTER_MUTEX_MIDIOUT_TAKE;
  s32 status = MIOS32_MIDI_SendSysEx(blm_midi_port, (u8 *)sysex_buffer, (u32)sysex_buffer_ptr - ((u32)&sysex_buffer[0]));
  BLM_SCALAR_MASTER_MUTEX_MIDIOUT_GIVE;

  return status;
}


/////////////////////////////////////////////////////////////////////////////
//! This function should be periodically called each mS
/////////////////////////////////////////////////////////////////////////////
s32 BLM_SCALAR_MASTER_Periodic_mS(void)
{
  if( !blm_midi_port )
    return -1; // no BLM port defined yet (or explicitely disabled)

  ///////////////////////////////////////////////////////////////////////////
  //! handle Timeout
  ///////////////////////////////////////////////////////////////////////////
  if( !blm_timeout_ctr )
    return 0;

  if( --blm_timeout_ctr == 0 ) {
    blm_connection_state = BLM_SCALAR_MASTER_CONNECTION_STATE_IDLE;
    return 0;
  }

  ///////////////////////////////////////////////////////////////////////////
  //! take over update force flag
  ///////////////////////////////////////////////////////////////////////////
  MIOS32_IRQ_Disable();
  u8 force_update = blm_force_update;
  blm_force_update = 0;
  MIOS32_IRQ_Enable();


  ///////////////////////////////////////////////////////////////////////////
  //! send LED changes to BLM16x16
  ///////////////////////////////////////////////////////////////////////////
  mios32_midi_package_t packets[BLM_MAX_PACKETS];
  u8 num_packets = 0;

  // help macro to handle packet buffer
#define SEND_PACKET(p) { \
    packets[num_packets++] = p;                 \
    if( num_packets >= BLM_MAX_PACKETS ) {      \
      BLM_SendPackets(packets, num_packets);    \
      num_packets = 0;                          \
    } \
  }


  mios32_midi_package_t p;
  p.ALL = 0;
  p.cin = CC;
  p.event = CC;

  {
    int i;
    int num_rows = blm_leds_rotate_view ? BLM_SCALAR_MASTER_NUM_ROWS : blm_num_rows;
    for(i=0; i<num_rows; ++i) {
      u8 led_row = i + blm_led_row_offset;

      u16 pattern_green = blm_scalar_master_leds_green[led_row];
      u16 prev_pattern_green = blm_scalar_master_leds_green_sent[led_row];
      u16 pattern_red = blm_scalar_master_leds_red[led_row];
      u16 prev_pattern_red = blm_scalar_master_leds_red_sent[led_row];

      if( force_update || pattern_green != prev_pattern_green || pattern_red != prev_pattern_red ) {

        // Note: the MIOS32 MIDI driver will take care about running status to optimize the stream
        if( force_update || ((pattern_green ^ prev_pattern_green) & 0x00ff) ) {
          u8 pattern8 = pattern_green;
          p.chn = i;
          p.cc_number = 8*blm_leds_rotate_view + ((pattern8 & 0x80) ? 17 : 16); // CC number + MSB LED
          p.value = pattern8 & 0x7f; // remaining 7 LEDs

          SEND_PACKET(p);
        }

        if( force_update || ((pattern_green ^ prev_pattern_green) & 0xff00) ) {
          u8 pattern8 = pattern_green >> 8;
          p.chn = i;
          p.cc_number = 8*blm_leds_rotate_view + ((pattern8 & 0x80) ? 19 : 18); // CC number + MSB LED
          p.value = pattern8 & 0x7f; // remaining 7 LEDs

          SEND_PACKET(p);
        }       

        if( force_update || ((pattern_red ^ prev_pattern_red) & 0x00ff) ) {
          u8 pattern8 = pattern_red;
          p.chn = i;
          p.cc_number = 8*blm_leds_rotate_view + ((pattern8 & 0x80) ? 33 : 32); // CC number + MSB LED
          p.value = pattern8 & 0x7f; // remaining 7 LEDs

          SEND_PACKET(p);
        }

        if( force_update || ((pattern_red ^ prev_pattern_red) & 0xff00) ) {
          u8 pattern8 = pattern_red >> 8;
          p.chn = i;
          p.cc_number = 8*blm_leds_rotate_view + ((pattern8 & 0x80) ? 35 : 34); // CC number + MSB LED
          p.value = pattern8 & 0x7f; // remaining 7 LEDs

          SEND_PACKET(p);
        }       

        blm_scalar_master_leds_green_sent[led_row] = pattern_green;
        blm_scalar_master_leds_red_sent[led_row] = pattern_red;
      }
    }
  }


  ///////////////////////////////////////////////////////////////////////////
  //! send LED changes to extra buttons
  ///////////////////////////////////////////////////////////////////////////
  if( force_update || blm_scalar_master_leds_extra_green != blm_scalar_master_leds_extra_green_sent ) {
    p.chn = Chn16;
    p.cc_number = 0x60;
    p.value = blm_scalar_master_leds_extra_green;
    SEND_PACKET(p);
    blm_scalar_master_leds_extra_green_sent = blm_scalar_master_leds_extra_green;
  }

  if( force_update || blm_scalar_master_leds_extra_red != blm_scalar_master_leds_extra_red_sent ) {
    p.chn = Chn16;
    p.cc_number = 0x68;
    p.value = blm_scalar_master_leds_extra_red;
    SEND_PACKET(p);
    blm_scalar_master_leds_extra_red_sent = blm_scalar_master_leds_extra_red;
  }

  if( force_update || blm_scalar_master_leds_extracolumn_green != blm_scalar_master_leds_extracolumn_green_sent ) {
    p.chn = Chn1;
    p.cc_number = (blm_scalar_master_leds_extracolumn_green & 0x0080) ? 0x41 : 0x40;
    p.value = (blm_scalar_master_leds_extracolumn_green >> 0) & 0x7f;
    SEND_PACKET(p);

    p.cc_number = (blm_scalar_master_leds_extracolumn_green & 0x8000) ? 0x43 : 0x42;
    p.value = (blm_scalar_master_leds_extracolumn_green >> 8) & 0x7f;
    SEND_PACKET(p);

    blm_scalar_master_leds_extracolumn_green_sent = blm_scalar_master_leds_extracolumn_green;
  }

  if( force_update || blm_scalar_master_leds_extracolumn_red != blm_scalar_master_leds_extracolumn_red_sent ) {
    p.chn = Chn1;
    p.cc_number = (blm_scalar_master_leds_extracolumn_red & 0x0080) ? 0x49 : 0x48;
    p.value = (blm_scalar_master_leds_extracolumn_red >> 0) & 0x7f;
    SEND_PACKET(p);

    p.cc_number = (blm_scalar_master_leds_extracolumn_red & 0x8000) ? 0x4b : 0x4a;
    p.value = (blm_scalar_master_leds_extracolumn_red >> 8) & 0x7f;
    SEND_PACKET(p);

    blm_scalar_master_leds_extracolumn_red_sent = blm_scalar_master_leds_extracolumn_red;
  }


  if( force_update || blm_scalar_master_leds_extracolumn_shift_green != blm_scalar_master_leds_extracolumn_shift_green_sent ) {
    p.chn = Chn1;
    p.cc_number = (blm_scalar_master_leds_extracolumn_shift_green & 0x0080) ? 0x51 : 0x50;
    p.value = (blm_scalar_master_leds_extracolumn_shift_green >> 0) & 0x7f;
    SEND_PACKET(p);

    p.cc_number = (blm_scalar_master_leds_extracolumn_shift_green & 0x8000) ? 0x53 : 0x52;
    p.value = (blm_scalar_master_leds_extracolumn_shift_green >> 8) & 0x7f;
    SEND_PACKET(p);

    blm_scalar_master_leds_extracolumn_shift_green_sent = blm_scalar_master_leds_extracolumn_shift_green;
  }

  if( force_update || blm_scalar_master_leds_extracolumn_shift_red != blm_scalar_master_leds_extracolumn_shift_red_sent ) {
    p.chn = Chn1;
    p.cc_number = (blm_scalar_master_leds_extracolumn_shift_red & 0x0080) ? 0x59 : 0x58;
    p.value = (blm_scalar_master_leds_extracolumn_shift_red >> 0) & 0x7f;
    SEND_PACKET(p);

    p.cc_number = (blm_scalar_master_leds_extracolumn_shift_red & 0x8000) ? 0x5b : 0x5a;
    p.value = (blm_scalar_master_leds_extracolumn_shift_red >> 8) & 0x7f;
    SEND_PACKET(p);

    blm_scalar_master_leds_extracolumn_shift_red_sent = blm_scalar_master_leds_extracolumn_shift_red;
  }


  if( force_update || blm_scalar_master_leds_extrarow_green != blm_scalar_master_leds_extrarow_green_sent ) {
    p.chn = Chn1;
    p.cc_number = (blm_scalar_master_leds_extrarow_green & 0x0080) ? 0x61 : 0x60;
    p.value = (blm_scalar_master_leds_extrarow_green >> 0) & 0x7f;
    SEND_PACKET(p);

    p.cc_number = (blm_scalar_master_leds_extrarow_green & 0x8000) ? 0x63 : 0x62;
    p.value = (blm_scalar_master_leds_extrarow_green >> 8) & 0x7f;
    SEND_PACKET(p);

    blm_scalar_master_leds_extrarow_green_sent = blm_scalar_master_leds_extrarow_green;
  }

  if( force_update || blm_scalar_master_leds_extrarow_red != blm_scalar_master_leds_extrarow_red_sent ) {
    p.chn = Chn1;
    p.cc_number = (blm_scalar_master_leds_extrarow_red & 0x0080) ? 0x69 : 0x68;
    p.value = (blm_scalar_master_leds_extrarow_red >> 0) & 0x7f;
    SEND_PACKET(p);

    p.cc_number = (blm_scalar_master_leds_extrarow_red & 0x8000) ? 0x6b : 0x6a;
    p.value = (blm_scalar_master_leds_extrarow_red >> 8) & 0x7f;
    SEND_PACKET(p);

    blm_scalar_master_leds_extrarow_red_sent = blm_scalar_master_leds_extrarow_red;
  }

  // send remaining packets
  if( num_packets )
    BLM_SendPackets(packets, num_packets);

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
//! Help function to send MIDI packets for LED layout changes
/////////////////////////////////////////////////////////////////////////////
static s32 BLM_SendPackets(mios32_midi_package_t *packets, u8 num_packets)
{
  u8 to_lemur = ((blm_midi_port & 0xf0) == OSC0) && (blm_connection_state == BLM_SCALAR_MASTER_CONNECTION_STATE_LEMUR);

  BLM_SCALAR_MASTER_MUTEX_MIDIOUT_TAKE;

  if( to_lemur ) {
#if BLM_SCALAR_MASTER_OSC_SUPPORT
    int i;
    u8 packet[BLM_MAX_PACKETS*4+BLM_MAX_PACKETS+10];
    u8 *end_ptr = packet;
    end_ptr = MIOS32_OSC_PutString(end_ptr, "/BLM");

    char buffer[BLM_MAX_PACKETS+2];
    buffer[0] = ',';
    for(i=1; i<=num_packets; ++i)
      buffer[i] = 'i';
    buffer[i] = 0;

    end_ptr = MIOS32_OSC_PutString(end_ptr, buffer);

    for(i=0; i<num_packets; ++i)
      end_ptr = MIOS32_OSC_PutInt(end_ptr, packets[i].ALL);

    OSC_SERVER_SendPacket(blm_midi_port & 0x0f, packet, (u32)(end_ptr-packet));
#endif
  } else {
    int i;
    for(i=0; i<num_packets; ++i)
      MIOS32_MIDI_SendPackage(blm_midi_port, packets[i]);
  }

  BLM_SCALAR_MASTER_MUTEX_MIDIOUT_GIVE;

  return 0; // no error
}

//! \}
