// $Id$
/*
 * MIDIbox SID V3
 *
 * ==========================================================================
 *
 *  Copyright (C) 2009 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

/////////////////////////////////////////////////////////////////////////////
// Include files
/////////////////////////////////////////////////////////////////////////////

#include <mios32.h>

#include "tasks.h"

#include <aout.h>
#include <sid.h>
#include <mbnet.h>

#include "app.h"
#include "MbSidEnvironment.h"


/////////////////////////////////////////////////////////////////////////////
// for optional debugging messages via DEBUG_MSG (defined in mios32_config.h)
// should be at least 1 for sending error messages
/////////////////////////////////////////////////////////////////////////////
#define DEBUG_VERBOSE_LEVEL 1

// measure performance with the stopwatch
#define STOPWATCH_PERFORMANCE_MEASURING 1

/////////////////////////////////////////////////////////////////////////////
// Local prototypes
/////////////////////////////////////////////////////////////////////////////
extern "C" void SID_TIMER_SE_Update(void);
extern "C" s32 NOTIFY_MIDI_Rx(mios32_midi_port_t port, u8 byte);
extern "C" s32 NOTIFY_MIDI_Tx(mios32_midi_port_t port, mios32_midi_package_t package);
extern "C" s32 NOTIFY_MIDI_SysEx(mios32_midi_port_t port, u8 midi_in);
extern "C" s32 NOTIFY_MIDI_TimeOut(mios32_midi_port_t port);

extern "C" void MBNET_TASK_Service(u8 master_id, mbnet_tos_req_t tos, u16 control, mbnet_msg_t req_msg, u8 dlc);

/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////
static u32 stopwatch_value;
static u32 stopwatch_value_max;
static u32 stopwatch_value_accumulated;

static u32 sid_se_speed_factor;


/////////////////////////////////////////////////////////////////////////////
// C++ objects
/////////////////////////////////////////////////////////////////////////////
MbSidEnvironment mbSidEnvironment;


/////////////////////////////////////////////////////////////////////////////
// This hook is called after startup to initialize the application
/////////////////////////////////////////////////////////////////////////////
extern "C" void APP_Init(void)
{
  // initialize all LEDs
  MIOS32_BOARD_LED_Init(0xffffffff);

  // initialize AOUT driver
  AOUT_Init(0);

  // initialize SID module
  SID_Init(0);

  // start tasks (differs between MIOS32 and MacOS)
  TASKS_Init(0);

  // install MIDI Rx/Tx callback functions
  MIOS32_MIDI_DirectRxCallback_Init((void *)&NOTIFY_MIDI_Rx);
  MIOS32_MIDI_DirectTxCallback_Init((void *)&NOTIFY_MIDI_Tx);

  // install MIDI SysEx callback function
  MIOS32_MIDI_SysExCallback_Init((void *)&NOTIFY_MIDI_SysEx);

  // install timeout callback function
  MIOS32_MIDI_TimeOutCallback_Init((void *)&NOTIFY_MIDI_TimeOut);

  // init Stopwatch
  APP_StopwatchInit();

  // init MBNet
  MBNET_Init(0);
  MBNET_NodeIDSet(0x10);

  // initialize MbSidEnvironment
  sid_se_speed_factor = 2;
  mbSidEnvironment.updateSpeedFactorSet(sid_se_speed_factor);

  // start timer
  // TODO: increase  once performance has been evaluated
  MIOS32_TIMER_Init(2, 2000 / sid_se_speed_factor, (void *)&SID_TIMER_SE_Update, MIOS32_IRQ_PRIO_MID);
}


/////////////////////////////////////////////////////////////////////////////
// This task is running endless in background
/////////////////////////////////////////////////////////////////////////////
extern "C" void APP_Background(void)
{
  // toggle the state of all LEDs (allows to measure the execution speed with a scope)
#if 0
  MIOS32_BOARD_LED_Set(0xffffffff, ~MIOS32_BOARD_LED_Get());
#endif
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called when a MIDI package has been received
/////////////////////////////////////////////////////////////////////////////
extern "C" void APP_MIDI_NotifyPackage(mios32_midi_port_t port, mios32_midi_package_t midi_package)
{
  mbSidEnvironment.midiReceive(port, midi_package);
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called before the shift register chain is scanned
/////////////////////////////////////////////////////////////////////////////
extern "C" void APP_SRIO_ServicePrepare(void)
{
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called after the shift register chain has been scanned
/////////////////////////////////////////////////////////////////////////////
extern "C" void APP_SRIO_ServiceFinish(void)
{
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called when a button has been toggled
// pin_value is 1 when button released, and 0 when button pressed
/////////////////////////////////////////////////////////////////////////////
extern "C" void APP_DIN_NotifyToggle(u32 pin, u32 pin_value)
{
#if DEBUG_VERBOSE_LEVEL >= 2
  DEBUG_MSG("Pin %3d (SR%d:D%d) = %d\n", pin, (pin>>3)+1, pin&7, pin_value);
#endif
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called when an encoder has been moved
// incrementer is positive when encoder has been turned clockwise, else
// it is negative
/////////////////////////////////////////////////////////////////////////////
extern "C" void APP_ENC_NotifyChange(u32 encoder, s32 incrementer)
{
#if DEBUG_VERBOSE_LEVEL >= 2
  DEBUG_MSG("Enc %2d = %d\n", encoder, incrementer);
#endif
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called when a pot has been moved
/////////////////////////////////////////////////////////////////////////////
extern "C" void APP_AIN_NotifyChange(u32 pin, u32 pin_value)
{
}



/////////////////////////////////////////////////////////////////////////////
// This timer interrupt periodically calls the sound engine update
/////////////////////////////////////////////////////////////////////////////
extern "C" void SID_TIMER_SE_Update(void)
{
#if STOPWATCH_PERFORMANCE_MEASURING >= 1
  APP_StopwatchReset();
#endif

  if( !mbSidEnvironment.tick() )
    return; // no update required (e.g. in ASID mode)

#if STOPWATCH_PERFORMANCE_MEASURING == 1
  APP_StopwatchCapture();
#endif

  // update SID registers
  SID_Update(0);

#if STOPWATCH_PERFORMANCE_MEASURING == 2
  APP_StopwatchCapture();
#endif
}


/////////////////////////////////////////////////////////////////////////////
// This task is called periodically each mS
/////////////////////////////////////////////////////////////////////////////
extern "C" void SID_TASK_Period1mS(void)
{
  MUTEX_MIDIOUT_TAKE;

  if( !MBNET_ScanFinished() ) {
    MBNET_Handler(MBNET_TASK_Service);

    if( MBNET_ScanFinished() ) {
      int i;
      u8 sid_available = 0x00;
      for(i=MBNET_SLAVE_NODES_BEGIN; i<=MBNET_SLAVE_NODES_END; ++i) {
	mbnet_msg_t *info;
	if( MBNET_SlaveNodeInfoGet(i, &info) >= 0 ) {
	  DEBUG_MSG("Slave ID %02x P:%d T:%c%c%c%c V:%d.%d\n", 
		    i,
		    info->protocol_version,
		    info->node_type[0], info->node_type[1], info->node_type[2], info->node_type[3],
		    info->node_version, info->node_subversion);

	  if( info->node_type[0] == 'S' &&
	      info->node_type[1] == 'I' &&
	      info->node_type[2] == 'D' &&
	      info->node_version >= 2 ) {

	    // disable local engine of SID
	    u8 sid = i;
	    mbnet_msg_t dummy_msg;
	    dummy_msg.data_l = dummy_msg.data_h = 0;
	    u8 dlc = 0;
	    s32 status;
	    if( (status=MBNET_SendReq(sid, MBNET_REQ_SPECIAL, 22, dummy_msg, dlc)) < 0 ) {
	      DEBUG_MSG("[MBNET] can't access SID ID %02x, status %d\n", sid, status);
	    } else {
	      if( (status=MBNET_WaitAck(sid, &dummy_msg, &dlc)) < 0 ) {
		DEBUG_MSG("[MBNET] can't access SID ID %02x, status %d (no ACK)\n", sid, status);
	      } else {
		sid_available |= (3 << (2*i));
	      }
	    }
	    
	  } else {
	    DEBUG_MSG("No MIDIbox SID V2 or greater - skipped!\n");
	  }
	}
      }

      SID_AvailableSet(sid_available);
      DEBUG_MSG("MBNet Scan has been finished - available SIDs: 0x%02x\n", sid_available);
    }
  }
  MUTEX_MIDIOUT_GIVE;
}


/////////////////////////////////////////////////////////////////////////////
// This task is called each mS with lowest priority
/////////////////////////////////////////////////////////////////////////////
extern "C" void SID_TASK_Period1mS_LowPrio(void)
{
#if 0
  static s32 old_value = 0;
  MUTEX_MIDIOUT_TAKE;
  if( (sid_se_vars[0].mod_src[SID_SE_MOD_SRC_MOD4] / 256) != old_value ) {
    old_value = sid_se_vars[0].mod_src[SID_SE_MOD_SRC_MOD4] / 256;
    DEBUG_MSG("%d\n", sid_se_vars[0].mod_src[SID_SE_MOD_SRC_MOD4]);
  }
  MUTEX_MIDIOUT_GIVE;
#endif
}

/////////////////////////////////////////////////////////////////////////////
// This task is called periodically each second
/////////////////////////////////////////////////////////////////////////////
extern "C" void SID_TASK_Period1S(void)
{
  // output and reset current stopwatch max value
  MUTEX_MIDIOUT_TAKE;
  MIOS32_IRQ_Disable();
  u32 max_value = stopwatch_value_max;
  stopwatch_value_max = 0;
  u32 acc_value = stopwatch_value_accumulated;
  stopwatch_value_accumulated = 0;
  MIOS32_IRQ_Enable();
#if 0
  if( acc_value || max_value )
    DEBUG_MSG("%d uS (max: %d uS)\n", acc_value / (1000000 / (2000/sid_se_speed_factor)), max_value);
#endif
#if 1
  SID_PrintStatistics();
#endif
  MUTEX_MIDIOUT_GIVE;

  static u8 wait_boot_ctr = 2; // wait 2 seconds before loading from SD Card - this is to increase the time where the boot screen is print!
  //u8 load_sd_content = 0;

#if USE_MSD
  // don't check for SD Card if MSD enabled
  if( TASK_MSD_EnableGet() > 0 )
    return;
#endif

  // boot phase of 2 seconds finished?
  if( wait_boot_ctr ) {
    --wait_boot_ctr;
    return;
  }

  // check if SD Card connected
  MUTEX_SDCARD_TAKE;

#if 0
  s32 status = SID_FILE_CheckSDCard();
#endif

  MUTEX_SDCARD_GIVE;
}


/////////////////////////////////////////////////////////////////////////////
// Installed via MIOS32_MIDI_DirectRxCallback_Init
/////////////////////////////////////////////////////////////////////////////
extern "C" s32 NOTIFY_MIDI_Rx(mios32_midi_port_t port, u8 midi_byte)
{
  // TODO: better port filtering!
  if( port < USB1 || port >= UART0 ) {
    if( midi_byte >= 0xf8 )
      mbSidEnvironment.midiReceiveRealTimeEvent(port, midi_byte);
  }

  return 0; // no error, no filtering
}


/////////////////////////////////////////////////////////////////////////////
// Installed via MIOS32_MIDI_DirectTxCallback_Init
/////////////////////////////////////////////////////////////////////////////
extern "C" s32 NOTIFY_MIDI_Tx(mios32_midi_port_t port, mios32_midi_package_t package)
{
  return 0;
}


/////////////////////////////////////////////////////////////////////////////
// Installed via MIOS32_MIDI_SysExCallback_Init
/////////////////////////////////////////////////////////////////////////////
extern "C" s32 NOTIFY_MIDI_SysEx(mios32_midi_port_t port, u8 midi_in)
{
  return mbSidEnvironment.midiReceiveSysEx(port, midi_in);
}


/////////////////////////////////////////////////////////////////////////////
// Installed via MIOS32_MIDI_TimeoutCallback_Init
/////////////////////////////////////////////////////////////////////////////
extern "C" s32 NOTIFY_MIDI_TimeOut(mios32_midi_port_t port)
{
  mbSidEnvironment.midiTimeOut(port);

  return 0;
}


/////////////////////////////////////////////////////////////////////////////
// For performance Measurements
/////////////////////////////////////////////////////////////////////////////
s32 APP_StopwatchInit(void)
{
  stopwatch_value = 0;
  stopwatch_value_max = 0;
  stopwatch_value_accumulated = 0;
  return MIOS32_STOPWATCH_Init(1); // 1 uS resolution
}

s32 APP_StopwatchReset(void)
{
  return MIOS32_STOPWATCH_Reset();
}

s32 APP_StopwatchCapture(void)
{
  u32 value = MIOS32_STOPWATCH_ValueGet();
  MIOS32_IRQ_Disable();
  stopwatch_value = value;
  if( value > stopwatch_value_max )
    stopwatch_value_max = value;
  stopwatch_value_accumulated += value;
  MIOS32_IRQ_Enable();

  return 0; // no error
}


// Application specific ETOS, READ, WRITE and PING requests are forwarded to
// this callback function
// In any case, the function has to send an acknowledge message back
// to the master node!
extern "C" void MBNET_TASK_Service(u8 master_id, mbnet_tos_req_t tos, u16 control, mbnet_msg_t req_msg, u8 dlc)
{
  mbnet_msg_t ack_msg;
  ack_msg.data_l = 0;
  ack_msg.data_h = 0;

  // branch depending on TOS
  switch( tos ) {
    case MBNET_REQ_SPECIAL:
      // no support for MBSID specific ETOS yet - just accept them all
      MBNET_SendAck(master_id, MBNET_ACK_OK, ack_msg, 0); // master_id, tos, msg, dlc
      break;

    case MBNET_REQ_RAM_READ:
      // dummy
      MBNET_SendAck(master_id, MBNET_ACK_READ, ack_msg, 8); // master_id, tos, msg, dlc
      break;

    case MBNET_REQ_RAM_WRITE:
      // dummy
      MBNET_SendAck(master_id, MBNET_ACK_READ, ack_msg, 0); // master_id, tos, msg, dlc
      break;

    case MBNET_REQ_PING:
      ack_msg.protocol_version = 1;   // should be 1
      ack_msg.node_type[0]    = 'S'; // 4 characters which identify the node type
      ack_msg.node_type[1]    = 'I';
      ack_msg.node_type[2]    = 'D';
      ack_msg.node_type[3]    = ' ';
      ack_msg.node_version    = 3;   // version number (0..255)
      ack_msg.node_subversion = 0;   // subversion number (0..65535)
      MBNET_SendAck(master_id, MBNET_ACK_OK, ack_msg, 8); // master_id, tos, msg, dlc
      break;

    default:
      // unexpected TOS
      MBNET_SendAck(master_id, MBNET_ACK_ERROR, ack_msg, 0); // master_id, tos, msg, dlc
  }
}
