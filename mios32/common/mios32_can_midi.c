// $Id: mios32_uart_midi.c 2312 2016-02-27 23:04:51Z tk $
//! \defgroup MIOS32_CAN_MIDI
//!
//! CAN MIDI layer for MIOS32
//!
//! Applications shouldn't call these functions directly, instead please use \ref MIOS32_MIDI layer functions
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
#include <stdlib.h>
#include <string.h>

// this module can be optionally disabled in a local mios32_config.h file (included from mios32.h)
#if defined(MIOS32_USE_CAN_MIDI)

#define MIOS32_CAN_MIDI_ID_MASK 0xe1fffffc
/////////////////////////////////////////////////////////////////////////////
// Global variables
/////////////////////////////////////////////////////////////////////////////


#define DEBUG_CAN_LVL1 {if(can_midi_verbose)MIOS32_MIDI_SendDebugMessage}
#define DEBUG_CAN_LVL2 {if(can_midi_verbose>=2)MIOS32_MIDI_SendDebugMessage}
#define DEBUG_CAN_LVL3 {if(can_midi_verbose>=3)MIOS32_MIDI_SendDebugMessage}
/////////////////////////////////////////////////////////////////////////////
// Local Types
/////////////////////////////////////////////////////////////////////////////

typedef union {
  u8 stat;
  struct {
    u8 waiting:1;
    u8 ready:1;
    u8 ending:1;
    u8 :5;
  };
} sysex_stat_rec_t;

typedef struct sysex_unpack_rec_t{
  sysex_stat_rec_t stat;
  u32 ext_id;
  u32 ports;
#if defined(MIOS32_CAN_MIDI_SYSEX_STREAM_FIRST)
  u8  buffer[1024];
#else
  u8  buffer[11];
#endif
  u16 ctr;
  u16 timeout_ctr;
} sysex_unpack_rec_t;

typedef struct sysex_repack_rec_t{
  sysex_stat_rec_t stat;
  u32 ext_id;
  u32 data_l;
  u8  buffer[10];
  u8  ctr;
  u16 packet;
  u16 timeout_ctr;
} sysex_repack_rec_t;


#if MIOS32_CAN_NUM
/////////////////////////////////////////////////////////////////////////////
// Global variables
/////////////////////////////////////////////////////////////////////////////

// CAN MIDI Node Id
#if defined MIOS32_CAN_MIDI_NODE_ID
u8 mios32_mcan_id = MIOS32_CAN_MIDI_NODE_ID;
#else
// if not precised it becomes Device_Id on init but can be changed by SW.
u8 mios32_mcan_id;
#endif

/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

static u8 mios32_mcan_enhanced = 0;

// status structure for sysex record
static sysex_unpack_rec_t sysex_unpack[MIOS32_CAN_MIDI_NUM_PORTS];

// SysEx repack data structure
static sysex_repack_rec_t sysex_repack[MIOS32_CAN_MIDI_NUM_PORTS];

// active sense delay counter
s16 node_active_sense_ctr;
// for frame rate calculation
static u32 frame_rate;
static u32 can_last_baudrate;

// callback for direct sysex stream
static s32 (*sysex_stream_callback_func)(mios32_mcan_header_t header, u8* stream, u16 size);
static s32 (*direct_package_callback_func)(mios32_mcan_header_t header, mios32_midi_package_t package);

// verbose
static u8 can_midi_verbose = 2;

#endif


/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

// internal function to reset the record structure
static s32 MIOS32_CAN_MIDI_SysexUnpackReset(sysex_unpack_rec_t* sysex_unpack)
{
#if MIOS32_CAN_NUM > 0
  
  memset(sysex_unpack, 0, sizeof (struct sysex_unpack_rec_t));
 
#endif
  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

// internal function to reset the record structure
static s32 MIOS32_CAN_MIDI_SysexRepackReset(sysex_repack_rec_t* sysex_repack)
{
#if MIOS32_CAN_NUM > 0
  
  memset(sysex_repack, 0, sizeof (struct sysex_repack_rec_t));
  
#endif
  return 0; // no error
}
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

// internal function to parse configuration commands
static s32 MIOS32_CAN_MIDI_LocalCmdParser(mios32_mcan_header_t header, mios32_midi_package_t* package)
{
#if MIOS32_CAN_NUM > 0
  

  
#endif
  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
//! Initializes CAN MIDI layer
//! \param[in] mode currently only mode 0 supported
//! \return < 0 if initialisation failed
//! \note Applications shouldn't call this function directly, instead please use \ref MIOS32_MIDI layer functions
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_CAN_MIDI_Init(u32 mode)
{

#if MIOS32_CAN_NUM == 0
  return -1; // no CAN enabled
#else
  int i;
  
  // currently only mode 0 supported
  if( mode != 0 )
    return -2; // unsupported mode

#if defined MIOS32_CAN_MIDI_ENHANCED
  MIOS32_CAN_MIDI_ModeSet(1);
#else
  MIOS32_CAN_MIDI_ModeSet(0);
#endif
  
#if defined MIOS32_CAN_MIDI_VERBOSE_LEVEL
  // set defined verbose
  MIOS32_CAN_MIDI_VerboseSet(MIOS32_CAN_MIDI_VERBOSE_LEVEL);
#endif

  // set active sense delay counter in mS
  node_active_sense_ctr = 1000;
  
  // initialize MIDI record
  for (i=0; i<MIOS32_CAN_MIDI_NUM_PORTS; i++) {
    MIOS32_CAN_MIDI_SysexUnpackReset(&sysex_unpack[i]);
    MIOS32_CAN_MIDI_SysexRepackReset(&sysex_repack[i]);
  }
  
  
  sysex_stream_callback_func = NULL;
  
  // if any MIDI assignment:
#if MIOS32_CAN1_ASSIGNMENT == 1 
  // initialize CAN interface
  if( MIOS32_CAN_Init(0) < 0 )
    return -3; // initialisation of CAN Interface failed
  
  // by default the Node ID is the Sysex Device ID
  // Think about configure a different Device_ID for each of your device present on the MIDIan
  // via the bootloader update tool.
  //MIOS32_CAN_MIDI_NodeIDSet(MIOS32_MIDI_DeviceIDGet());
  
  // Initialize permanent filtering for Node System Exclusive and Common messages
  MIOS32_CAN_MIDI_FilterInit(1);
  
  
#endif
  
  return 0; // no error
#endif
}

/////////////////////////////////////////////////////////////////////////////
//! Initializes CAN MIDI layer
//! \param[in] mode currently only mode 0 supported
//! \return < 0 if initialisation failed
//! \note Applications shouldn't call this function directly, instead please use \ref MIOS32_MIDI layer functions
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_CAN_MIDI_VerboseSet(u8 level)
{
#if MIOS32_CAN_NUM == 0
  return -1; // no CAN enabled
#else
  
  can_midi_verbose = level;
  MIOS32_CAN_VerboseSet(level); // they are independant e.g. CAN is used but not for midi purpose
  return 0; // no error
#endif
}

/////////////////////////////////////////////////////////////////////////////
//! Initializes CAN MIDI layer
//! \param[in] mode currently only mode 0 supported
//! \return < 0 if initialisation failed
//! \note Applications shouldn't call this function directly, instead please use \ref MIOS32_MIDI layer functions
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_CAN_MIDI_VerboseGet(void)
{
#ifndef MIOS32_CAN_MIDI_VERBOSE_LEVEL
  return 0;
#else
#if MIOS32_CAN_NUM == 0
  return -1; // no CAN enabled
#else
  return can_midi_verbose; // no error
#endif
#endif
}


/////////////////////////////////////////////////////////////////////////////
// Changes the Node ID, Default startup Id is the Device_Id
// IN: node_id: configures the CAN_MIDI Node ID
//              MIDIan has no master and slave ID node
// OUT: returns < 0 if configuration failed
// Note id is the same for both CAN
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_CAN_MIDI_NodeIDSet(u8 node_id)
{
#ifndef MIOS32_CAN_MIDI_VERBOSE_LEVEL
  return 0;
#else
#if MIOS32_CAN_NUM == 0
  return -1; // no CAN enabled
#else

  // my node_id must be 0..127
  if( node_id >= 128 )
    return -1;
  
  // take over node ID
  mios32_mcan_id = node_id;
  
  return 0; // no error
#endif
#endif
}


/////////////////////////////////////////////////////////////////////////////
// Returns the Node ID
// IN: -
// OUT: if -1: node not configured yet
//      if >= 0: node ID
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_CAN_MIDI_NodeIDGet(void)
{
#if MIOS32_CAN_NUM == 0
  return -1; // no CAN enabled
#else
  return mios32_mcan_id;
#endif
}

/////////////////////////////////////////////////////////////////////////////
// Changes the Node ID, Default startup Id is the Device_Id
// IN: node_id: configures the CAN_MIDI Node ID
//              MIDIan has no master and slave ID node
// OUT: returns < 0 if configuration failed
// Note id is the same for both CAN
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_CAN_MIDI_ModeSet(u8 mode)
{
#if MIOS32_CAN_NUM == 0
  return -1; // no CAN enabled
#else
  mios32_mcan_enhanced = 0;
  if(mode)mios32_mcan_enhanced = 1;
  
  return 0; // no error
#endif
}


/////////////////////////////////////////////////////////////////////////////
// Returns the Node ID
// IN: -
// OUT: if -1: node not configured yet
//      if >= 0: node ID
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_CAN_MIDI_ModeGet(void)
{
#if MIOS32_CAN_NUM == 0
  return -1; // no CAN enabled
#else
  return mios32_mcan_enhanced;
#endif
}


/////////////////////////////////////////////////////////////////////////////
//! Initializes CAN MIDI layer
//! \param[in] bypass 0:off, >=1:on
//! \return < 0 if initialisation failed
//! \note Applications shouldn't call this function directly, instead please use \ref MIOS32_MIDI layer functions
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_CAN_MIDI_FilterInit(u8 bypass)
{
#if MIOS32_CAN_NUM == 0
  return -1; // no CAN enabled
#else
  
  //u8 i;
  //mios32_can_ext_filter_t filt32;
  mios32_can_std_filter_t filt16[2];
  
//  /* Node System messages and bypass -> Fifo0 */
//  // filter Bank#0, checks for incoming Node SysEx and SysCom messages
//  filt32.filt.ALL = 0;
//  filt32.filt.event = 0xf8;
//  //filt32.filt.dst_devid = mios32_mcan_id;
//  filt32.filt.ide = 1; // 29bits extented id
//  filt32.mask.ALL = 0;
//  filt32.mask.event = 0xf8; // =>0xf8 only
//  //filt32.mask.dst_devid = 0x7f;
//  filt32.mask.ide = 1;
//  MIOS32_CAN_Init32bitFilter(0, 0, filt32, 1); // CAN1, FIFO0, filter, enabled
  
  // filter Bank#1, checks for incoming Node System Common and bypass
  
  
  filt16[0].filt.ALL = 0; // Second 16bit filter is a Bypass filter
  //this 11bit id must never happen.
  //filt16[0].mask.event = (bypass ? 0x00 : 0xff); // 0xff: bypass off, 0x00: bypass on.
  filt16[0].mask.ALL = 0x00;
  
  filt16[1].filt.ALL = 0; // Second not used
  filt16[1].mask.ALL = 0x00;
  MIOS32_CAN_Init16bitFilter(0, 0, filt16[0], filt16[1], 1); // CAN1, FIFO0, filter, enabled
  
//  /* Device System messages and Real Time -> Fifo0 */
//  // filter Bank#2, checks for incoming Device(s) SysEx messages
//  filt32.filt.ALL = 0;
//  filt32.filt.event = 0xf0;
//  filt32.filt.ide = 1; // 29bits extented id
//  filt32.mask.ALL = 0;
//  filt32.mask.event = 0xff; // =>0xf0 only
//  filt32.mask.ide = 1;
//  MIOS32_CAN_Init32bitFilter(2, 0, filt32, 1); // CAN1, FIFO0, filter, enabled
//  
//  // filter Bank#3, checks for incoming Device System Common and Real Time
//  filt16[0].filt.ALL = 0; // first 16bit filter, Device SysCom
//  filt16[0].filt.event = 0xf0;
//  filt16[0].mask.ALL = 0;
//  filt16[0].mask.event = 0xf8; // =>0xf1...0xf7
//  
//  filt16[1].filt.ALL = 0; // Second 16bit filter, Real Time
//  filt16[1].filt.event = 0x78;
//  filt16[1].filt.ide = 0; // 11bits standard id
//  filt16[1].mask.ALL = 0;
//  filt16[1].mask.event = 0xf8; // =>0x78...0x7f
//  filt16[1].mask.ide = 0;
//  MIOS32_CAN_Init16bitFilter(3, 0, filt16[0], filt16[1], 1); // CAN1, FIFO0, filter, enabled
//  
//  /* Device Voicing messages -> Fifo1 */
//  // filter Bank#0x7..0xd , checks for incoming Device Channel Voice messages
//  filt32.filt.ALL = 0;
//  filt32.filt.ide = 1; // 29bits extented id
//  filt32.mask.ALL = 0;
//  filt32.mask.status = 0xf;
//  filt32.mask.ide = 1;
//  for (i=0x8; i<=0xe; i++) {
//    filt32.filt.status = i;
//    MIOS32_CAN_Init32bitFilter(i-1, 1, filt32, 1); // CAN1, FIFO0, filter, enabled
//  }
  return 0; // no error
#endif
}

/////////////////////////////////////////////////////////////////////////////
//! This function can be used to determine, if a CAN interface is available
//! \param[in] can_port CAN number (0..2)
//! \return 1: interface available
//! \return 0: interface not available
//! \note Applications shouldn't call this function directly, instead please use \ref MIOS32_MIDI layer functions
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_CAN_MIDI_CheckAvailable(u8 cable)
{
#if MIOS32_CAN_NUM == 0
  return -1; // all CANs explicitely disabled
#else
    if (MIOS32_CAN_IsAssignedToMIDI(0) == 0) // CAN assigned to MIDI?
      return 0;
    if( (cable & 0x0f) >= MIOS32_CAN_MIDI_NUM_PORTS )
      return 0;
#endif
  return 1;
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
s32 MIOS32_CAN_MIDI_Periodic_mS(void)
{
#if MIOS32_CAN_NUM == 0
  return -1; // all CANs explicitely disabled
#else
  u8 cable;
  
  MIOS32_IRQ_Disable();
  for(cable=0; cable<MIOS32_CAN_MIDI_NUM_PORTS; cable++) {
    // simplify addressing of sysex record
    sysex_repack_rec_t *rpk = &sysex_repack[cable];
    sysex_unpack_rec_t *upk = &sysex_unpack[cable];
    //    // increment the expire counters for running status optimisation.
    //    //
    //    // The running status will expire after 1000 ticks (1 second)
    //    // to ensure, that the current status will be sent at least each second
    //    // to cover the case that the MIDI cable is (re-)connected during runtime.
    //    if( rs_expire_ctr[can_port] < 65535 )
    //      ++rs_expire_ctr[can_port];
    
    // increment timeout counter for incoming packages
    // an incomplete event will be timed out after 1000 ticks (1 second)
    if( rpk->timeout_ctr < 65535 )
      rpk->timeout_ctr++;
    if( upk->timeout_ctr < 65535 )
      upk->timeout_ctr++;
  }
  MIOS32_IRQ_Enable();
  // (atomic operation not required in MIOS32_CAN_MIDI_PackageSend_NonBlocking() due to single-byte accesses)
  if(can_midi_verbose == 0) return 0; // no error
  if( (--node_active_sense_ctr) <=0 ){
    node_active_sense_ctr = 1000; // every second
    u32 new_frame_rate;
    if( MIOS32_CAN_MIDI_CheckAvailable(0) ){

      mios32_can_stat_report_t report;
      MIOS32_CAN_ReportGetCurr(0, &report);

      new_frame_rate = report.rx_packets_ctr - can_last_baudrate;
      can_last_baudrate = report.rx_packets_ctr;
      if(frame_rate != new_frame_rate){
        frame_rate = new_frame_rate;
        u8 percent =(u8)(new_frame_rate /245);
        MIOS32_MIDI_SendDebugMessage("[MIOS32_CAN_MIDI_Periodic_mS] framerate: %d f/s, %d%%", new_frame_rate, percent);
      }

      mios32_midi_package_t  p;
      p.ALL = 0;
      p.cin = 0x5;
      p.cable = 0x0;
      p.evnt0 = 0xfe ; // active sense

      MIOS32_CAN_MIDI_PackageSend(p);
      //MIOS32_CAN_MIDI_PacketTransmit(0, p);
    }
}
  
#endif
  
  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! This function transfers a new MCAN packet to the selected MCAN port
//! \param[in] MCAN Packet
//! \return 0: no error
//! \return -1: CAN_MIDI device not available
//! \return -2: CAN_MIDI buffer is full
//!             caller should retry until buffer is free again
//! \note Applications shouldn't call this function directly, instead please use \ref MIOS32_MIDI layer functions
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_CAN_MIDI_PacketTransmit_NonBlocking(mios32_mcan_packet_t p)
{
#if MIOS32_CAN_NUM == 0
  return -1; // all CANs explicitely disabled
#else
  // tx enabler bit
  p.id.txrq = 1;
#if defined(MIOS32_CAN_MIDI_VERBOSE_LEVEL)
  if(can_midi_verbose>=3){DEBUG_MSG("[MIOS32_CAN_MIDI_PacketTransmit] [0x%02x][0x%02x][0x%02x][0x%02x][0x%02x][0x%02x][0x%02x][0x%02x]\n", p.data.bytes[0],p.data.bytes[1],p.data.bytes[2],p.data.bytes[3],p.data.bytes[4],p.data.bytes[5],p.data.bytes[6],p.data.bytes[7]);}
#endif
  switch( MIOS32_CAN_TxBufferPut(0, p.packet) ) {
  //switch( MIOS32_CAN_Transmit(can_port, p, 0) ) {
    case  0 ... 2: return  0; // transfer successfull
    case -1: return -1;
    case -2: return -2; // buffer full, request retry
    case -3: return -3; // CAN bus error
    default: return -4; // CAN not available
  }
  
#endif
}

/////////////////////////////////////////////////////////////////////////////
//! This function transfers a new MCAN packet to the selected MCAN port
//! (blocking function)
//! \param[in] MCAN Packet
//! \return 0: no error
//! \return -1: CAN_MIDI device not available
//! \note Applications shouldn't call this function directly, instead please use \ref MIOS32_MIDI layer functions
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_CAN_MIDI_PacketTransmit(mios32_mcan_packet_t p)
{
  s32 error;
  
  while( (error=MIOS32_CAN_MIDI_PacketTransmit_NonBlocking(p)) == -2);
  
  return error;
}

/////////////////////////////////////////////////////////////////////////////
//! This function formats and transfers a new MCAN packet to the selected MCAN port
//! \param[in] header MCAN Extra Infos
//! \param[in] package MIDI package
//! \return 0: no error
//! \note Applications shouldn't call this function directly, instead please use \ref MIOS32_MIDI layer functions
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_CAN_MIDI_PacketSend_NonBlocking(mios32_mcan_header_t header, mios32_midi_package_t package)
{
#if MIOS32_CAN_NUM == 0
  return -1; // all CANs explicitely disabled
#else
  // exit if MCANx port not available
  if( !MIOS32_CAN_MIDI_CheckAvailable(package.cable) )
    return -3;
  
  s32 error = 0;
  
  // Parse package to Sysex repack
  // notes: see "const u8 mios32_mcan_pcktype_num_bytes[16]"
  if(package.evnt0 == 0xf0 && package.type == 0x4) {
    //header.cin_cable = 0x40 + package.cable;  // Start
    error = MIOS32_CAN_MIDI_SysexRepackSend(header, package);
    return error;
  }else if(package.type == 0x4) {
    //header.cin_cable = 0x60 + package.cable; // Cont
    error = MIOS32_CAN_MIDI_SysexRepackSend(header, package);
    return error;
  }else if(package.type == 0x5 && package.evnt0 == 0xf7){
    //header.cin_cable = 0x70 + package.cable;  // end
    error = MIOS32_CAN_MIDI_SysexRepackSend(header, package);
    return error;
  }else if(package.type == 0x6 || package.type == 0x7){
    //header.cin_cable = 0x70 + package.cable;  // end
    error = MIOS32_CAN_MIDI_SysexRepackSend(header, package);
    return error;
  }else{ // This is not Sysex
    mios32_mcan_packet_t p;
    //p.frame_id = header.frame_id;
    u8* byte = &p.data.bytes[0];
    p.ctrl.dlc = 0;
#if defined(MIOS32_CAN_MIDI_ENHANCED)
    // On extended frame adds the src/dst ports infos
    if(header.is_extended == 1){
      *byte++ = header.dst_port;
      *byte++ = header.src_port;
      p.ctrl.dlc += 2;
    }
#endif
    // Adds data depending on expected bytes
    u8 expected_bytes = mios32_midi_pcktype_num_bytes[header.type];
    switch (expected_bytes) {
      case 1:
        *byte = package.evnt0;
        p.ctrl.dlc += expected_bytes;
        break;
      case 2:
        *byte++ = package.evnt0;
        *byte = package.evnt1;
        p.ctrl.dlc += expected_bytes;
        break;
      case 3:
        *byte++ = package.evnt0;
        *byte++ = package.evnt1;
        *byte = package.evnt2;
        p.ctrl.dlc += expected_bytes;
        break;
      default:
        return -4; // wrong datas
    }
    p.id = header.id;
    while( (error = MIOS32_CAN_MIDI_PacketTransmit_NonBlocking(p)) == -2);
#if defined(MIOS32_CAN_MIDI_VERBOSE_LEVEL)
    if(can_midi_verbose>=2){DEBUG_MSG("[MIOS32_CAN_MIDI_PacketSend] type:0x%02x, cable:%d, ide:%d, dlc:%d, package:0x%08x\n", header.type, header.cable, header.is_extended, p.ctrl.dlc, package.ALL);}
#endif
    return error;
  }
  return error;
  
#endif
}

/////////////////////////////////////////////////////////////////////////////
//! This function formats and transfers a new MCAN packet to the selected MCAN port
//! (blocking function)
//! \param[in] header MCAN Extra Infos
//! \param[in] package MIDI package
//! \return 0: no error
//! \return -1: CAN_MIDI device not available
//! \note Applications shouldn't call this function directly, instead please use \ref MIOS32_MIDI layer functions
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_CAN_MIDI_PacketSend(mios32_mcan_header_t header, mios32_midi_package_t package)
{
  s32 error;
  //DEBUG_MSG("[MIOS32_CAN_MIDI_PackageSend] 0x%08x\n", package.ALL);
  
  while( (error=MIOS32_CAN_MIDI_PacketSend_NonBlocking(header, package)) == -2);
  
  return error;
}

/////////////////////////////////////////////////////////////////////////////
//! This function sends a new MIDI package to the selected MCAN port
//! \param[in] package MIDI package
//! \return 0: no error
//! \note Applications shouldn't call this function directly, instead please use \ref MIOS32_MIDI layer functions
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_CAN_MIDI_PackageSend_NonBlocking(mios32_midi_package_t package)
{
  s32 error;
  
  mios32_mcan_header_t header;
  MIOS32_CAN_MIDI_DefaultHeaderInit(&header);
  header.cable = package.cable;
  header.type = package.type;
  error = MIOS32_CAN_MIDI_PacketSend_NonBlocking(header, package);
  
  return error;
}


/////////////////////////////////////////////////////////////////////////////
//! This function sends a new MIDI package to the selected CAN_MIDI port
//! (blocking function)
//! \param[in] package MIDI package
//! \return 0: no error
//! \return -1: CAN_MIDI device not available
//! \note Applications shouldn't call this function directly, instead please use \ref MIOS32_MIDI layer functions
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_CAN_MIDI_PackageSend(mios32_midi_package_t package)
{
  s32 error;
  
  while( (error=MIOS32_CAN_MIDI_PackageSend_NonBlocking(package)) == -2);
  
  return error;
}

/////////////////////////////////////////////////////////////////////////////
//! This function repacks a Sysex MIDI Package to a MCAN Sysex Packet
//! It groups the 3 bytes packages in 8 bytes packet(s) and send it to the can back to back
//! (blocking function)
//! \param[in] header MCAN Extra Infos
//! \param[in] package MIDI package
//! \return 0: no error
//! \return -1: CAN_MIDI device not available
//! \note Applications shouldn't call this function directly, instead please use \ref MIOS32_MIDI layer functions
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_CAN_MIDI_SysexRepackSend(mios32_mcan_header_t header, mios32_midi_package_t package)
{
#if MIOS32_CAN_NUM == 0
  return -1; // all CANs explicitely disabled
#else
  
  // exit if MCANx port not available
  if( !MIOS32_CAN_MIDI_CheckAvailable(header.cable) )
    return -1;
  
  u8 i;
  s32 error = 0;
  
  sysex_repack_rec_t *rpk = &sysex_repack[header.cable];// simplify addressing of sysex record
  
  // incoming MIDI package timed out (incomplete package received)
  if( rpk->stat.waiting && rpk->timeout_ctr > 1000 ) { // 1000 mS = 1 second
    // stop waiting
    MIOS32_CAN_MIDI_SysexRepackReset(rpk);
    // notify that incomplete package has been received
    return error;
  }
  
  switch (header.type) {
      
    case 4: // Start/Cont
      if(package.evnt0 == 0xf0){
        if(rpk->stat.waiting!=0)return error;
        rpk->ctr = 0; // Re-init byte and packet counters on 0xf0
        rpk->packet = 0;
        rpk->ext_id = header.ext_id & MIOS32_CAN_MIDI_ID_MASK;
        // On extended frame adds the src/dst ports infos
#if defined(MIOS32_CAN_MIDI_ENHANCED)
        if(header.is_extended){
          rpk->buffer[rpk->ctr++] = header.src_port;
          rpk->buffer[rpk->ctr++] = header.dst_port;
        }
#endif
        rpk->stat.waiting = 1;
      }
      if(rpk->stat.waiting==0)return error;
      if(rpk->ext_id != (header.ext_id & MIOS32_CAN_MIDI_ID_MASK))return error;
      rpk->buffer[rpk->ctr++] = package.evnt0;
      rpk->buffer[rpk->ctr++] = package.evnt1;
      rpk->buffer[rpk->ctr++] = package.evnt2;
      rpk->timeout_ctr = 0;
      break;
    case 7: // Ends with three bytes
      if(rpk->stat.waiting==0)return error;
      if(rpk->ext_id != (header.ext_id & MIOS32_CAN_MIDI_ID_MASK))return error;
      rpk->buffer[rpk->ctr++] = package.evnt0;
      rpk->buffer[rpk->ctr++] = package.evnt1;
      rpk->buffer[rpk->ctr++] = package.evnt2;
      rpk->stat.ending = 1;
      break;
    case 6: // Ends with two bytes
      if(rpk->stat.waiting==0)return error;
      if(rpk->ext_id != (header.ext_id & MIOS32_CAN_MIDI_ID_MASK))return error;
      rpk->buffer[rpk->ctr++] = package.evnt0;
      rpk->buffer[rpk->ctr++] = package.evnt1;
      rpk->stat.ending = 1;
      break;
    case 5: // Ends with a single byte
      if(rpk->stat.waiting==0)return error;
      if(rpk->ext_id != (header.ext_id & MIOS32_CAN_MIDI_ID_MASK))return error;
      rpk->buffer[rpk->ctr++] = package.evnt0;
      rpk->stat.ending = 1;
      break;
    default:
      break;
  }
  
  if(rpk->ctr >= 8){ // a full packet is ready
    if (rpk->packet == 0)header.type = 0x4;  // =>Start
    else header.type = 0x6;  // =>Cont
    mios32_mcan_packet_t p;
    // copy id
    p.id = header.id;
    // dlc
    p.ctrl.dlc = 8;
    for(i=0;i<8;i++)p.data.bytes[i] = rpk->buffer[i]; // Copy to packet
    while( (error = MIOS32_CAN_MIDI_PacketTransmit_NonBlocking(p)) == -2);
    rpk->packet++;
    rpk->ctr %= 8; // removes sent packet from counter
    // shift down the remaining bytes
    for(i=0;i<rpk->ctr;i++)rpk->buffer[i] = rpk->buffer[i+8];
#if defined(MIOS32_CAN_MIDI_VERBOSE_LEVEL)
    if(can_midi_verbose>=2){DEBUG_MSG("[MIOS32_CAN_MIDI_SysexRepackSend] type:0x%02x, cable:%d, ide:%d, dlc:%d\n", header.type, header.cable, header.is_extended, p.ctrl.dlc);}
#endif
  }
  if(rpk->stat.ending == 1){ // this is last packet
    if (rpk->packet == 0)header.type = 0x4;  // =>Start(special case only one packet stream)
    else header.type = 0x7;  // =>End
    mios32_mcan_packet_t p;
    // copy id
    p.id = header.id;
    // dlc
    p.ctrl.dlc = rpk->ctr;
    for(i=0;i<rpk->ctr;i++)p.data.bytes[i] = rpk->buffer[i]; // Copy to packet
    while( (error = MIOS32_CAN_MIDI_PacketTransmit_NonBlocking(p)) == -2);
#if defined(MIOS32_CAN_MIDI_VERBOSE_LEVEL)
    if(can_midi_verbose>=2){DEBUG_MSG("[MIOS32_CAN_MIDI_SysexRepackSend] type:0x%02x, cable:%d, ide:%d, dlc:%d\n", header.type, header.cable, header.is_extended, p.ctrl.dlc);}
#endif
    // repack reset
    MIOS32_CAN_MIDI_SysexRepackReset(rpk);
  }
  return error;
  
#endif
}


/////////////////////////////////////////////////////////////////////////////
//! This function sends a Sysex Stream MIDI to the selected MCAN port
//! It divides the stream in 8 bytes packet(s) and send it to the can back to back
//! \param[in] header MCAN Extra Infos
//! \param[in] stream
//! \param[in] size of the stream
//! \return 0: no error
//! \return -1: CAN_MIDI device not available
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_CAN_MIDI_SysexSend_NonBlocking(mios32_mcan_header_t header, u8 *stream, u16 size)
{
#if MIOS32_CAN_NUM == 0
  return -1; // all CANs explicitely disabled
#else
  // exit if MCANx port not available
  if( !MIOS32_CAN_MIDI_CheckAvailable(header.cable) )
    return -3;
  // no bytes to send -> no error
  if( !size )
    return 0;
  
  // Prepare the Packet
  s32 error;
  mios32_mcan_packet_t p;
  p.ctrl.ALL = 0;
  p.data.data_l = 0;
  p.data.data_h = 0;
  
  // On extended frame adds the src/dst ports
  // infos at start of first packet.
  if(header.is_extended){
    size += 2;
  }
  
  // Calc packet number and last packet DLC
  u16 packet_num;
  u8 i, j, last_dlc;
  packet_num = (u16)(size >> 3);
  last_dlc = (u8)(size % 8);
  if(last_dlc != 0)packet_num++;
  
  u8* stream_ptr = &stream[0];
  // loops packets
  for (i=0; i<packet_num; i++) {
    u8* byte = &p.data.bytes[0];
    // Set DLC and type depending on packet
    if(i == 0){    // Start
      // On extended frame adds the src/dst ports infos
      header.type = 0x4;  // Start Flag if one unique packet(<= 8 bytes in stream)
      if(packet_num == 1)p.ctrl.dlc = last_dlc;
      else p.ctrl.dlc = 8;
#if defined(MIOS32_CAN_MIDI_ENHANCED)
      if(header.is_extended){
        *byte++ = header.dst_port;
        *byte++ = header.src_port;
        for (j=0; j<(p.ctrl.dlc-2); j++)*byte++ = *stream_ptr++;
      }else{
        for (j=0; j<p.ctrl.dlc; j++)*byte++ = *stream_ptr++;
      }
#else
      for (j=0; j<p.ctrl.dlc; j++)*byte++ = *stream_ptr++;
#endif
    }else if(i == (packet_num-1)){   //last packet
      header.type = 0x7;  // End
      p.ctrl.dlc = last_dlc;
      for (j=0; j<p.ctrl.dlc; j++)*byte++ = *stream_ptr++;
    }else{  //others packets
      header.type = 0x6;  // Cont
      p.ctrl.dlc = 8;
      for (j=0; j<8; j++)*byte++ = *stream_ptr++;
    }
#if defined(MIOS32_CAN_MIDI_VERBOSE_LEVEL)
    if(can_midi_verbose>=2){DEBUG_MSG("[MIOS32_CAN_MIDI_SysexSend] type:0x%02x, cable:%d, ide:%d, dlc:%d\n", header.type, header.cable, header.is_extended, p.ctrl.dlc);}
#endif
    p.id = header.id;
    while( (error = MIOS32_CAN_MIDI_PacketTransmit_NonBlocking(p)) == -2);
    if(error <0 ){
      // it must stop cause it can not retry
      return error;
    }
  }
  return 0;
#endif
}

/////////////////////////////////////////////////////////////////////////////
//! This function sends a Sysex Stream MIDI to the selected CAN_MIDI port
//! It divides the stream in 8 bytes packet(s) and send it to the can back to back
//! (blocking function)
//! \param[in] can_port CAN_MIDI module number (0..1)
//! \param[in] package MIDI package
//! \return 0: no error
//! \return -1: CAN_MIDI device not available
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_CAN_MIDI_SysexSend(mios32_mcan_header_t header, u8 *stream, u16 size)
{
#if MIOS32_CAN_NUM == 0
  return -1; // all CANs explicitely disabled - accordingly no package in buffer
#else
  s32 error;
  
  while( (error=MIOS32_CAN_MIDI_SysexSend_NonBlocking(header, stream, size)) == -2);
  
  return error;
#endif
}


/////////////////////////////////////////////////////////////////////////////
//! Installs an optional SysEx callback which is called by
//! MIOS32_MIDI_Receive_Handler() to simplify the parsing of SysEx streams.
//!
//! Without this callback (or with MIOS32_MIDI_SysExCallback_Init(NULL)),
//! SysEx messages are only forwarded to APP_MIDI_NotifyPackage() in chunks of
//! 1, 2 or 3 bytes, tagged with midi_package.type == 0x4..0x7 or 0xf
//!
//! In this case, the application has to take care for different transmission
//! approaches which are under control of the package sender. E.g., while Windows
//! uses Package Type 4..7 to transmit a SysEx stream, PortMIDI under MacOS sends
//! a mix of 0xf (single byte) and 0x4 (continued 3-byte) packages instead.
//!
//! By using the SysEx callback, the type of package doesn't play a role anymore,
//! instead the application can parse a serial stream.
//!
//! MIOS32 ensures, that realtime events (0xf8..0xff) are still forwarded to
//! APP_MIDI_NotifyPackage(), regardless if they are transmitted in a package
//! type 0x5 or 0xf, so that the SysEx parser doesn't need to filter out such
//! events, which could otherwise appear inside a SysEx stream.
//!
//! \param[in] *callback_sysex pointer to callback function:<BR>
//! \code
//!    s32 callback_sysex(mios32_midi_port_t port, u8 sysex_byte)
//!    {
//!       //
//!       // .. parse stream
//!       //
//!
//!       return 1; // don't forward package to APP_MIDI_NotifyPackage()
//!    }
//! \endcode
//! If the function returns 0, SysEx bytes will be forwarded to APP_MIDI_NotifyPackage() as well.
//! With return value != 0, APP_MIDI_NotifyPackage() won't get the already processed package.
//! \return < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_CAN_MIDI_SysExStreamCallback_Init(s32 (*callback_sysex_stream)(mios32_mcan_header_t header, u8* stream, u16 size))
{
#if MIOS32_CAN_NUM == 0
  return -1; // all CANs explicitely disabled - accordingly no package in buffer
#else
  sysex_stream_callback_func = callback_sysex_stream;
  
  return 0; // no error
#endif
}

//////////////////////////////////////////////////////////////////////////
//! Installs an optional Direct Midi package Callback callback
//! If the function returns 0, SysEx bytes will be forwarded to APP_MIDI_NotifyPackage() as well.
//! With return value != 0, APP_MIDI_NotifyPackage() won't get the already processed package.
//! \return < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_CAN_MIDI_PackageCallback_Init(s32 (*direct_package_callback)(mios32_mcan_header_t header, mios32_midi_package_t package))
{
#if MIOS32_CAN_NUM == 0
  return -1; // all CANs explicitely disabled - accordingly no package in buffer
#else
  direct_package_callback_func = direct_package_callback;
  
  return 0; // no error
#endif
}

/////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////
static s32 MIOS32_CAN_MIDI_SysexUnPack(u8 cable, mios32_midi_package_t *package)
{
  int i;
  // midi repack from sysex stream
  sysex_unpack_rec_t *upk = &sysex_unpack[cable];
  
  if(upk->stat.ending == 1 && upk->ctr<=3){
    switch(upk->ctr){
      case 3:
        package->type = 0x7; // F: tree bytes
        package->cable = cable;
        package->evnt0 = upk->buffer[0];
        package->evnt1 = upk->buffer[1];
        package->evnt2 = upk->buffer[2];
        break;
      case 2:
        package->type = 0x6; // F: two bytes
        package->cable = cable;
        package->evnt0 = upk->buffer[0];
        package->evnt1 = upk->buffer[1];
        break;
      case 1:
        package->type = 0x5; // F: single byte
        package->cable = cable;
        package->evnt0 = upk->buffer[0];
        break;
      default:
        break;
    }
    // toDo package callback
    MIOS32_CAN_MIDI_SysexUnpackReset(upk);
    return 1; //last package ready
  }else  if(upk->ctr>=3){
    package->type = 0x4; // F: single byte
    package->cable = cable;
    package->evnt0 = upk->buffer[0];
    package->evnt1 = upk->buffer[1];
    package->evnt2 = upk->buffer[2];
    upk->ctr -=3;
    // shift down the remaining bytes
    for(i=0;i<upk->ctr;i++)upk->buffer[i] = upk->buffer[i+3];
    return 1;
  }else return 0;
}


/////////////////////////////////////////////////////////////////////////////
//! This function checks for a new package
//! \param[in] can_port CAN_MIDI module number (0..2)
//! \param[out] package pointer to MIDI package (received package will be put into the given variable
//! \return 0: no error
//! \return -1: no package in buffer
//! \return -10: incoming MIDI package timed out (incomplete package received)
//! \note Applications shouldn't call this function directly, instead please use \ref MIOS32_MIDI layer functions
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_CAN_MIDI_PackageReceive(mios32_midi_package_t *package)
{
#if MIOS32_CAN_NUM == 0
  return -1; // all CANs explicitely disabled - accordingly no package in buffer
#else
  
  int i;
  s32 status = 0;
  // clear MIDI package
  package->ALL = 0;
  
  // Checks for stream repack first.
  for (i=0; i<MIOS32_CAN_MIDI_NUM_PORTS; i++) {
    // incoming MIDI package timed out (incomplete package received)
    if( sysex_unpack[i].stat.waiting && sysex_unpack[i].timeout_ctr > 1000 ) { // 1000 mS = 1 second
      // stop waiting, reset
      MIOS32_CAN_MIDI_SysexUnpackReset(&sysex_unpack[i]);
    }
    // there's some
    if(sysex_unpack[i].stat.ready == 1)
      status = MIOS32_CAN_MIDI_SysexUnPack(i, package);
    if(status == 1)return status;
  }
  
  mios32_mcan_packet_t p;
  // Something in the buffer?
  if(MIOS32_CAN_RxBufferGet(0, &p.packet) >= 0) {
    // usable structure
    mios32_mcan_header_t header;
    header.id = p.id;
    // exit if CAN port not available
    if( !MIOS32_CAN_MIDI_CheckAvailable(header.cable) )
      return 0;
//    //test
//#if defined(MIOS32_CAN_MIDI_VERBOSE_LEVEL)
//    if(can_midi_verbose>=3)DEBUG_MSG("[MIOS32_CAN_MIDI_PackageReceive] 0x%08x 0x%08x\n", header.ext_id, header.ext_id & MIOS32_CAN_MIDI_ID_MASK);
//#endif
    if(header.type == 0x4 || header.type == 0x6 || header.type == 0x7){
      // simplify addressing of sysex record
      sysex_unpack_rec_t *upk = &sysex_unpack[header.cable];

#if defined(MIOS32_CAN_MIDI_VERBOSE_LEVEL)
      if(can_midi_verbose>=2){DEBUG_MSG("[MIOS32_CAN_MIDI_PackageReceive] type:0x%02x, cable:%d, ide:%d, dlc:%d, data: 0x%08x%08x\n", header.type, header.cable, header.is_extended, p.ctrl.dlc, p.data.data_h, p.data.data_l);}
#endif
      status = 0;
      switch (header.type) {
        case 0x4:
          // if busy filters new sysex message
          if(upk->stat.waiting)break; // not forwarded
          // store current stream header
          upk->ext_id = header.ext_id & MIOS32_CAN_MIDI_ID_MASK;   // Mask removes type field
          // basic/enhanced?
          if(header.is_extended == 1){
#if defined(MIOS32_CAN_MIDI_ENHANCED)
            // breaks if third byte in data is not SOX
            if(p.data.bytes[2] != 0xf0)break;
            // copy ports from the two first bytes of data
            upk->ports = p.data.data[0];
            // because of precedent lines stream counter is dlc -2
            upk->ctr = p.ctrl.dlc -2;
            // copy data bytes
            for (i=0; i<upk->ctr; i++)upk->buffer[i] = p.data.bytes[i+2];
#else
            break;
#endif
          }else{
            // breaks if first byte in data is not SOX
            if(p.data.bytes[0] != 0xf0)break;
            // stream counter is dlc
            upk->ctr = p.ctrl.dlc;
            for (i=0; i<upk->ctr; i++)upk->buffer[i] = p.data.bytes[i];
          }
          // Ends if last byte is EOX
          if(p.data.bytes[p.ctrl.dlc-1] == 0xf7)upk->stat.ending = 1; // last more sysex packet-
          else upk->stat.waiting = 1; // we are waiting for more sysex packet
          // reset timeout
          upk->timeout_ctr = 0;
#if !defined(MIOS32_CAN_MIDI_SYSEX_STREAM_FIRST)
          upk->stat.ready = 1;
          status = MIOS32_CAN_MIDI_SysexUnPack(header.cable, package);
#endif
          break;
        case 0x6 ... 0x7:
          if(!upk->stat.waiting)break; // not forwarded
          // compare headers, breaks if no match with stream packet
          if(upk->ext_id != (header.ext_id & MIOS32_CAN_MIDI_ID_MASK))break; // not forwarded
          // copy data bytes
          for (i=0; i<p.ctrl.dlc; i++)
            upk->buffer[upk->ctr+i] = p.data.bytes[i];
          // adds dlc to stream counter
          upk->ctr += p.ctrl.dlc;
          if(header.type == 0x7)upk->stat.ending = 1; // last more sysex packet-
          else upk->stat.waiting = 1; // we are waiting for more sysex packet
          // reset timeout
          upk->timeout_ctr = 0;
#if !defined(MIOS32_CAN_MIDI_SYSEX_STREAM_FIRST)
          upk->stat.ready = 1;
          status = MIOS32_CAN_MIDI_SysexUnPack(header.cable, package);
#endif
          break;
        default:
          break;
      }

      if(upk->stat.ending){ //
        //upk->ending = 0;
        upk->stat.waiting = 0;
#if defined(MIOS32_CAN_MIDI_SYSEX_STREAM_FIRST)
        // Sysex stream callback if callback returns 0 then no midi repack
        s32 callback_status = 0;
        if( sysex_stream_callback_func != NULL ){
          header.ext_id = upk->ext_id;
          header.type = 0x4;
          header.ports = upk->ports;
          callback_status = sysex_stream_callback_func(header, upk->buffer, upk->ctr); // -> forwarded as SysEx stream
        }
        if(!callback_status){
          // start packaging
          upk->stat.ready = 1;
          return MIOS32_CAN_MIDI_SysexUnPack(header.cable, package);
        }else{
          MIOS32_CAN_MIDI_SysexUnpackReset(upk);
          return 0; // stream forwarded but no midi repack
        }
#else
        return status; //last package ready
#endif
      }else{
        return status;  // no midi package
      }

    } else { // Others messages than Sysex
      // prepare header
      mios32_mcan_header_t header;
      header.id = p.id;
      // data pointer
      u8* byte = &p.data.bytes[0];
      // is enhanced packet
      if(header.is_extended){
#if defined(MIOS32_CAN_MIDI_ENHANCED)
        header.src_port = *byte++;
        header.src_port = *byte++;
        p.ctrl.dlc -=2;
#else
        return 0;  // not forwarded
#endif
      }
      u8 expected_bytes = mios32_midi_pcktype_num_bytes[header.type];
      if( expected_bytes == p.ctrl.dlc) { // Packet is single byte cmd, expected_bytes=0 and DLC=0
        
        package->type = header.type;
        package->cable = header.cable;
        for (i=0; i<expected_bytes; i++)package->bytes[i+1] = *byte++;
#if defined(MIOS32_CAN_MIDI_VERBOSE_LEVEL)
        if(can_midi_verbose>=2){DEBUG_MSG("[MIOS32_CAN_MIDI_PackageReceive] type:0x%02x, cable:%d, ide:%d, dlc:%d, package:0x%08x\n", header.type, header.cable, header.is_extended, p.ctrl.dlc, package->ALL);}
#endif
        if(header.type == 0x0){
          return 0;
        }else if(header.type == 0x1){ // Local command filter
          MIOS32_CAN_MIDI_LocalCmdParser(header, package);
          return 0;
        }else{
          s32 callback_status = 0;
          if( direct_package_callback_func != NULL ){
            mios32_midi_package_t direct_package = *package;
            callback_status = direct_package_callback_func(header, direct_package); // -> forwarded as MCAN Midi Package
          }
          if(!callback_status)return 1; // midi package ok
          else return 0;  // not forwarded
        }
      }else return 0;  // no midi package
    }
  }else return -2;
  
#endif
}

/////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////
u32 MIOS32_CAN_MIDI_DefaultHeaderInit(mios32_mcan_header_t* header)
{
#if MIOS32_CAN_NUM == 0
  return -1; // all CANs explicitely disabled - accordingly no package in buffer
#else
  // reset
  header->ext_id = 0;
  header->ports = 0;

  if(mios32_mcan_enhanced > 0)header->is_extended = 1;else header->is_extended = 0;
  //header->is_extended = 0;
  header->src_node  = mios32_mcan_id;
  header->src_port  = MCAN0;
  header->dst_node  = MIOS32_CAN_MIDI_DEF_DEST_ID;
  header->dst_port  = MCAN0;
  header->app_type  = MIOS32_CAN_MIDI_APP_CODE;
  header->vman_prio = MIOS32_CAN_MIDI_DEF_VMAN;

  return 0; // no error
#endif
}

//!}

#endif /* MIOS32_USE_CAN_MIDI */
