// $Id$
/*
 * MBNet handler as FreeRTOS task
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

#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>

#include <mbnet.h>

#include "mbnet_task.h"


/////////////////////////////////////////////////////////////////////////////
// Task Priorities
/////////////////////////////////////////////////////////////////////////////

// same priority like MIOS32 hooks
#define PRIORITY_TASK_UIP		( tskIDLE_PRIORITY + 3 )


// for mutual exclusive access to MBNet functions
// The mutex is handled with MUTEX_MBNET_TAKE and MUTEX_MBNET_GIVE macros
xSemaphoreHandle xMBNETSemaphore;


/////////////////////////////////////////////////////////////////////////////
// for optional debugging messages via DEBUG_MSG (defined in mios32_config.h)
/////////////////////////////////////////////////////////////////////////////

#define DEBUG_VERBOSE_LEVEL 1


/////////////////////////////////////////////////////////////////////////////
// Local defines
/////////////////////////////////////////////////////////////////////////////

#define PATCH_BUFFER_SIZE 512
#define ENS_BUFFER_SIZE   64

/////////////////////////////////////////////////////////////////////////////
// local variables
/////////////////////////////////////////////////////////////////////////////

static u8 patch_buffer[PATCH_BUFFER_SIZE];
static u8 ens_buffer[ENS_BUFFER_SIZE];

/////////////////////////////////////////////////////////////////////////////
// Local prototypes
/////////////////////////////////////////////////////////////////////////////
static void MBNET_TASK_Handler(void *pvParameters);
static void MBNET_TASK_Service(u8 master_id, mbnet_tos_req_t tos, u16 control, mbnet_msg_t req_msg, u8 dlc);


/////////////////////////////////////////////////////////////////////////////
// Initialize the MBNet task
/////////////////////////////////////////////////////////////////////////////
s32 MBNET_TASK_Init(u32 mode)
{
  if( mode > 0 )
    return -1; // only mode 0 supported yet

  xMBNETSemaphore = xSemaphoreCreateMutex();

  // start task
  xTaskCreate(MBNET_TASK_Handler, (signed portCHAR *)"MBNet", configMINIMAL_STACK_SIZE, NULL, PRIORITY_TASK_UIP, NULL);

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// The MBNet Task is executed each mS
/////////////////////////////////////////////////////////////////////////////
static void MBNET_TASK_Handler(void *pvParameters)
{
  // Initialise the xLastExecutionTime variable on task entry
  portTickType xLastExecutionTime = xTaskGetTickCount();

  // initialize MBNet
  MUTEX_MBNET_TAKE;
  MBNET_Init(0);
  MBNET_NodeIDSet(0x10);
  MUTEX_MBNET_GIVE;

  // endless loop
  while( 1 ) {
    vTaskDelayUntil(&xLastExecutionTime, 1 / portTICK_RATE_MS);

    MUTEX_MBNET_TAKE;
    MBNET_Handler(MBNET_TASK_Service);
    MUTEX_MBNET_GIVE;
  }
}


// Application specific ETOS, READ, WRITE and PING requests are forwarded to
// this callback function
// In any case, the function has to send an acknowledge message back
// to the master node!
static void MBNET_TASK_Service(u8 master_id, mbnet_tos_req_t tos, u16 control, mbnet_msg_t req_msg, u8 dlc)
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
      if( control >= 0xff00 ) { // ensemble buffer selected?
	int i;
	for(i=0; i<8; ++i) {
	  u32 addr = (control & 0xff) + i;
	  if( addr < ENS_BUFFER_SIZE ) // no error if address is greater than allowed, just ignore it
	    ack_msg.bytes[i] = ens_buffer[addr];
	  else
	    ack_msg.bytes[i] = 0;
	}
      } else { // patch buffer selected
	int i;
	for(i=0; i<8; ++i) {
	  u32 addr = control + i;
	  if( addr < PATCH_BUFFER_SIZE ) // no error if address is greater than allowed, just ignore it
	    ack_msg.bytes[i] = patch_buffer[addr];
	  else
	    ack_msg.bytes[i] = 0;
	}
      }
      MBNET_SendAck(master_id, MBNET_ACK_READ, ack_msg, 8); // master_id, tos, msg, dlc
      break;

    case MBNET_REQ_RAM_WRITE:
      if( dlc == 0 ) { // error of no byte has been received
	MBNET_SendAck(master_id, MBNET_ACK_ERROR, ack_msg, 0); // master_id, tos, msg, dlc
      } else {
	if( control >= 0xff00 ) { // ensemble buffer selected?
	  int i;
	  for(i=0; i<dlc; ++i) {
	    u32 addr = (control & 0xff) + i;
	    if( addr < ENS_BUFFER_SIZE ) // no error if address is greater than allowed, just ignore it
	      ens_buffer[addr] = req_msg.bytes[i];
	  }
	} else { // patch buffer selected
	  int i;
	  for(i=0; i<dlc; ++i) {
	    u32 addr = control + i;
	    if( addr < PATCH_BUFFER_SIZE ) // no error if address is greater than allowed, just ignore it
	      patch_buffer[addr] = req_msg.bytes[i];
	  }
	}
	MBNET_SendAck(master_id, MBNET_ACK_OK, ack_msg, 0); // master_id, tos, msg, dlc
      }
      break;

    case MBNET_REQ_PING:
      ack_msg.protocol_version = 1;   // should be 1
      ack_msg.node_type[0]    = 'S'; // 4 characters which identify the node type
      ack_msg.node_type[1]    = 'I';
      ack_msg.node_type[2]    = 'D';
      ack_msg.node_type[3]    = ' ';
      ack_msg.node_version    = 2;   // version number (0..255)
      ack_msg.node_subversion = 0;   // subversion number (0..65535)
      MBNET_SendAck(master_id, MBNET_ACK_OK, ack_msg, 8); // master_id, tos, msg, dlc
      break;

    default:
      // unexpected TOS
      MBNET_SendAck(master_id, MBNET_ACK_ERROR, ack_msg, 0); // master_id, tos, msg, dlc
  }
}



/////////////////////////////////////////////////////////////////////////////
// This function transfers a 8bit value into patch memory
/////////////////////////////////////////////////////////////////////////////
s32 MBNET_TASK_PatchWrite8(u8 sid, u16 addr, u8 value)
{
  s32 status;

#if 1
  MIOS32_BOARD_LED_Set(1, 1); // flicker LED for debugging
#endif

  MUTEX_MBNET_TAKE;

  mbnet_msg_t msg;
  msg.bytes[0] = value;
  u8 dlc = 1;

  if( (status=MBNET_SendReq(sid, MBNET_REQ_RAM_WRITE, addr, msg, dlc)) < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[MBNET_TASK_WritePatch8] MBNET_SendReq failed with status %d\n", status);
#endif
  } else {
    mbnet_msg_t ack_msg;
    if( (status=MBNET_WaitAck(sid, &ack_msg, &dlc)) < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[MBNET_TASK_WritePatch8] MBNET_WaitAck failed with status %d\n", status);
#endif
    }
  }

  MUTEX_MBNET_GIVE;

#if 1
  MIOS32_BOARD_LED_Set(1, 0); // flicker LED for debugging
#endif

  return status;
}


/////////////////////////////////////////////////////////////////////////////
// This function transfers a 16bit value into patch memory
/////////////////////////////////////////////////////////////////////////////
s32 MBNET_TASK_PatchWrite16(u8 sid, u16 addr, u16 value)
{
  s32 status;

#if 1
  MIOS32_BOARD_LED_Set(1, 1); // flicker LED for debugging
#endif

  MUTEX_MBNET_TAKE;

  mbnet_msg_t msg;
  msg.bytes[0] = value & 0xff;
  msg.bytes[1] = value >> 8;
  u8 dlc = 2;

  if( (status=MBNET_SendReq(sid, MBNET_REQ_RAM_WRITE, addr, msg, dlc)) < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[MBNET_TASK_WritePatch16] MBNET_SendReq failed with status %d\n", status);
#endif
  } else {
    mbnet_msg_t ack_msg;
    if( (status=MBNET_WaitAck(sid, &ack_msg, &dlc)) < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[MBNET_TASK_WritePatch16] MBNET_WaitAck failed with status %d\n", status);
#endif
    }
  }

  MUTEX_MBNET_GIVE;

#if 1
  MIOS32_BOARD_LED_Set(1, 0); // flicker LED for debugging
#endif


  return status;
}




