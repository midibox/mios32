//
//  MIOS32_MIDI_Wrapper.m
//  midibox_seq_v4
//
//  Created by Thorsten Klose on 05.12.08.
//  Copyright 2008 __MyCompanyName__. All rights reserved.
//

#import "MIOS32_MIDI_Wrapper.h"

#include <portmidi.h>

#include <mios32.h>

@implementation MIOS32_MIDI_Wrapper

#define NUM_MIDI_IN 1
#define NUM_MIDI_OUT 4
#define INPUT_BUFFER_SIZE 100
#define OUTPUT_BUFFER_SIZE 0
#define DRIVER_INFO NULL
#define TIME_PROC ((long (*)(void *)) Pt_Time)
#define TIME_INFO NULL

static u8 rx_buffer[NUM_MIDI_IN][MIOS32_UART_RX_BUFFER_SIZE];
static volatile u8 rx_buffer_tail[NUM_MIDI_IN];
static volatile u8 rx_buffer_head[NUM_MIDI_IN];
static volatile u8 rx_buffer_size[NUM_MIDI_IN];

static NSObject *_self;
PmStream *virtualMIDI_IN[NUM_MIDI_IN];
PmStream *virtualMIDI_OUT[NUM_MIDI_OUT];


//////////////////////////////////////////////////////////////////////////////
// init local variables
//////////////////////////////////////////////////////////////////////////////
- (void) awakeFromNib
{
	int i;
	
	_self = self;

	//create virtual MIDI ports
	for(i=0; i<NUM_MIDI_IN; ++i) {
	    Pm_OpenInput(&virtualMIDI_IN[i], 
                 i,
                 DRIVER_INFO, 
                 INPUT_BUFFER_SIZE, 
                 NULL, 
                 NULL);
		Pm_SetFilter(virtualMIDI_IN[i], PM_FILT_ACTIVE | PM_FILT_CLOCK | PM_FILT_SYSEX);
	}

	for(i=0; i<NUM_MIDI_OUT; ++i) {
	    Pm_OpenOutput(&virtualMIDI_OUT[i], 
                 i,
                 DRIVER_INFO, 
                 OUTPUT_BUFFER_SIZE, 
                 NULL, 
                 NULL,
				 0);
	}
}



/////////////////////////////////////////////////////////////////////////////
// Initializes UART interfaces
// IN: <mode>: currently only mode 0 supported
// OUT: returns < 0 if initialisation failed
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_UART_Init(u32 mode)
{
  // currently only mode 0 supported
  if( mode != 0 )
    return -1; // unsupported mode

#if MIOS32_UART_NUM == 0
  NSLog(@"MIOS32 No UARTs Found");
  return -1; // no UARTs
#else

	int i;
#if 1
    for (i = 0; i < Pm_CountDevices(); i++) {
        const PmDeviceInfo *info = Pm_GetDeviceInfo(i);
        NSLog(@"Found MIDI Interface - %d: %s, %s\n", i, info->interf, info->name);
	} 
#endif

  // clear buffer counters
  for(i=0; i<NUM_MIDI_IN; ++i) {
    rx_buffer_tail[i] = rx_buffer_head[i] = rx_buffer_size[i] = 0;
  }
  NSLog(@"MIOS32 UART Initialized");

  return 0; // no error
#endif
}




/////////////////////////////////////////////////////////////////////////////
// gets a byte from the receive buffer
// IN: UART number (0..1)
// OUT: -1 if UART not available
//      -2 if no new byte available
//      otherwise received byte
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_UART_RxBufferGet(u8 uart)
{	
	return 0;

#if MIOS32_UART_NUM == 0
  return -1; // no UART available
#else
  if( uart >= NUM_MIDI_IN )
    return -1; // UART not available

  if( !rx_buffer_size[uart] )
    return -2; // nothing new in buffer

  // get byte - this operation should be atomic!
  MIOS32_IRQ_Disable();
  u8 b = rx_buffer[uart][rx_buffer_tail[uart]];
  if( ++rx_buffer_tail[uart] >= MIOS32_UART_RX_BUFFER_SIZE )
    rx_buffer_tail[uart] = 0;
  --rx_buffer_size[uart];
  MIOS32_IRQ_Enable();

  return b; // return received byte
#endif
}


/////////////////////////////////////////////////////////////////////////////
// returns the next byte of the receive buffer without taking it
// IN: UART number (0..1)
// OUT: -1 if UART not available
//      -2 if no new byte available
//      otherwise received byte
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_UART_RxBufferPeek(u8 uart)
{
	return 0;

#if MIOS32_UART_NUM == 0
  return -1; // no UART available
#else
  if( uart >= NUM_MIDI_IN )
    return -1; // UART not available

  if( !rx_buffer_size[uart] )
    return -2; // nothing new in buffer
	// get byte - this operation should be atomic!
  MIOS32_IRQ_Disable();
  u8 b = rx_buffer[uart][rx_buffer_tail[uart]];
  MIOS32_IRQ_Enable();

  return b; // return received byte
#endif
}


/////////////////////////////////////////////////////////////////////////////
// puts a byte onto the receive buffer
// IN: UART number (0..1) and byte to be sent
// OUT: 0 if no error
//      -1 if UART not available
//      -2 if buffer full (retry)
//      
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_UART_RxBufferPut(u8 uart, u8 b)
{
	return 0;

#if MIOS32_UART_NUM == 0
  return -1; // no UART available//
#else
  if( uart >= NUM_MIDI_IN )
    return -1; // UART not available

	if( rx_buffer_size[uart] >= MIOS32_UART_RX_BUFFER_SIZE )
    return -2; // buffer full (retry)

  // copy received byte into receive buffer
  // this operation should be atomic!
  MIOS32_IRQ_Disable();
  rx_buffer[uart][rx_buffer_head[uart]] = b;
  if( ++rx_buffer_head[uart] >= MIOS32_UART_RX_BUFFER_SIZE )
    rx_buffer_head[uart] = 0;
  ++rx_buffer_size[uart];
  MIOS32_IRQ_Enable();

  return 0; // no error//
#endif
}


/////////////////////////////////////////////////////////////////////////////
// returns number of free bytes in transmit buffer
// IN: UART number (0..1)
// OUT: number of free bytes
//      if uart not available: 0
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_UART_TxBufferFree(u8 uart)
{
	return 0;
#if MIOS32_UART_NUM == 0
  return 0; // no UART available
#else
  if( uart >= NUM_MIDI_OUT )
    return 0;
  else
    return 256; // 256 byte package size provided by MacOS - was "MIOS32_UART_TX_BUFFER_SIZE - tx_buffer_size[uart];"
#endif
}


/////////////////////////////////////////////////////////////////////////////
// returns number of used bytes in transmit buffer
// IN: UART number (0..1)
// OUT: number of used bytes
//      if uart not available: 0
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_UART_TxBufferUsed(u8 uart)
{
	return 0;
#if MIOS32_UART_NUM == 0
  return 0; // no UART available
#else
  if( uart >= NUM_MIDI_OUT )
    return 0;
  else
    return 0; // was: tx_buffer_size[uart];
#endif
}


/////////////////////////////////////////////////////////////////////////////
// puts more than one byte onto the transmit buffer (used for atomic sends)
// IN: UART number (0..1), buffer to be sent and buffer length
// OUT: 0 if no error
//      -1 if UART not available
//      -2 if buffer full or cannot get all requested bytes (retry)
//      -3 if UART not supported by MIOS32_UART_TxBufferPut Routine
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_UART_TxBufferPutMore_NonBlocking(u8 uart, u8 *buffer, u16 len)
{
	return 0;

#if MIOS32_UART_NUM == 0
  return -1; // no UART available
#else
  if( uart >= NUM_MIDI_OUT )
    return -1; // UART not available
/*
	if( len ) {
		//int num_packets=len>>2;
		
	    PmEvent buffer[1];    
		int i;
		for(i=0; i<len; i+=4)
			 buffer, 1);
			 
			 
		Pm_Write(virtualMIDI_OUT[uart],
		MIOS32_MIDI_SendByteToTxCallback(UART0 + uart, buffer[i]);
		MIOS32_MIDI_SendByteToTxCallback(UART0 + uart, buffer[i+1];
		MIOS32_MIDI_SendByteToTxCallback(UART0 + uart, buffer[i+2]);
		MIOS32_MIDI_SendByteToTxCallback(UART0 + uart, buffer[i+3]);
	}
	*/
#if 0
  if( (tx_buffer_size[uart]+len) >= MIOS32_UART_TX_BUFFER_SIZE )
    return -2; // buffer full or cannot get all requested bytes (retry)
#endif


  return 0; // no error
#endif
}

/////////////////////////////////////////////////////////////////////////////
// puts more than one byte onto the transmit buffer (used for atomic sends)
// (blocking function)
// IN: UART number (0..1), buffer to be sent and buffer length
// OUT: 0 if no error
//      -1 if UART not available
//      -3 if UART not supported by MIOS32_UART_TxBufferPut Routine
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_UART_TxBufferPutMore(u8 uart, u8 *buffer, u16 len)
{
	return 0;
  s32 error;

  //while( (error=MIOS32_UART_TxBufferPutMore_NonBlocking(uart, buffer, len)) == -2 );

  return error;
}


/////////////////////////////////////////////////////////////////////////////
// puts a byte onto the transmit buffer
// IN: UART number (0..1) and byte to be sent
// OUT: 0 if no error
//      -1 if UART not available
//      -2 if buffer full (retry)
//      -3 if UART not supported by MIOS32_UART_TxBufferPut Routine
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_UART_TxBufferPut_NonBlocking(u8 uart, u8 b)
{
	return  0;
  // for more comfortable usage...
  // -> just forward to MIOS32_UART_TxBufferPutMore
  //return MIOS32_UART_TxBufferPutMore(uart, &b, 1);
}


/////////////////////////////////////////////////////////////////////////////
// puts a byte onto the transmit buffer
// (blocking function)
// IN: UART number (0..1) and byte to be sent
// OUT: 0 if no error
//      -1 if UART not available
//      -3 if UART not supported by MIOS32_UART_TxBufferPut Routine
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_UART_TxBufferPut(u8 uart, u8 b)
{
	return 0;
  s32 error;

  //while( (error=MIOS32_UART_TxBufferPutMore(uart, &b, 1)) == -2 );

  return error;
}


//s32   MIOS32_MIDI_SendByteToRxCallback(UART0, b);

/*
s32 pollforMIDIMessage()
{

	if Pm_Poll(midi);
            length = Pm_Read(midi,buffer, 1);
	int i;
	return 0;

	for(i=0; i<size; ++i) {
		MIOS32_UART_RxBufferPut(0, message[i]); // TODO: multiple IN ports
		MIOS32_MIDI_SendByteToRxCallback(UART0, message[i]);
	}
}

- (void)processMIDIPacketList:(const MIDIPacketList*)packets sender:(id)sender
{
	// from http://svn.notahat.com/simplesynth/trunk/AudioSystem.m
    int						i, j;
    const MIDIPacket*		packet;
    Byte					message[256];
    int						messageSize = 0;
	return 0;
    
    // Step through each packet
    packet = packets->packet;
    for (i = 0; i < packets->numPackets; i++) {
        for (j = 0; j < packet->length; j++) { 
		           
           // Hand off the packet for processing when the next one starts
            if ((packet->data[j] & 0x80) != 0 && messageSize > 0) {
                [self handleMIDIMessage:message ofSize:messageSize];
                messageSize = 0;
            }
            
            message[messageSize++] = packet->data[j];			// push the data into the message
        }
        
        packet = MIDIPacketNext (packet);
    }
    
    if (messageSize > 0)
        [self handleMIDIMessage:message ofSize:messageSize];
}
*/
@end
