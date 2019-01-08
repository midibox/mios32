//
//  MIOS32_MIDI_Wrapper.m
//  midibox_seq_v4
//
//  Created by Thorsten Klose on 05.12.08.
//  Copyright 2008 __MyCompanyName__. All rights reserved.
//

#import "MIOS32_MIDI_Wrapper.h"
#import <PYMIDI/PYMIDI.h>

#include <mios32.h>

@implementation MIOS32_MIDI_Wrapper

#define NUM_MIDI_IN 4
#define NUM_MIDI_OUT 4

static u8 rx_buffer[NUM_MIDI_IN][MIOS32_UART_RX_BUFFER_SIZE];
static volatile u16 rx_buffer_tail[NUM_MIDI_IN];
static volatile u16 rx_buffer_head[NUM_MIDI_IN];
static volatile u16 rx_buffer_size[NUM_MIDI_IN];

static u8 tx_sysex_buffer[NUM_MIDI_OUT][MIOS32_UART_TX_BUFFER_SIZE];
static u16 tx_sysex_buffer_ptr[NUM_MIDI_OUT];

static NSObject *_self;

PYMIDIVirtualDestination *virtualMIDI_IN[NUM_MIDI_IN];
PYMIDIVirtualSource *virtualMIDI_OUT[NUM_MIDI_OUT];


//////////////////////////////////////////////////////////////////////////////
// init local variables
//////////////////////////////////////////////////////////////////////////////
- (void) awakeFromNib
{
	int i;
	
	_self = self;

	// create virtual MIDI ports
	for(i=0; i<NUM_MIDI_IN; ++i) {
		NSMutableString *portName = [[NSMutableString alloc] init];
		[portName appendFormat:@"vMBSEQ IN%d", i+1];
		virtualMIDI_IN[i] = [[PYMIDIVirtualDestination alloc] initWithName:portName];
		[virtualMIDI_IN[i] addReceiver:self];
		[portName release];
	}

	for(i=0; i<NUM_MIDI_OUT; ++i) {
		NSMutableString *portName = [[NSMutableString alloc] init];
		[portName appendFormat:@"vMBSEQ OUT%d", i+1];
		virtualMIDI_OUT[i] = [[PYMIDIVirtualSource alloc] initWithName:portName];
		[portName release];
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
  return -1; // no UARTs
#else
  // initialisation of PYMIDI channels already done in awakeFromNib
  
  // clear buffer counters
  int i;
  for(i=0; i<NUM_MIDI_IN; ++i) {
	  MIOS32_IRQ_Disable();
	  rx_buffer_tail[i] = rx_buffer_head[i] = rx_buffer_size[i] = 0;
	  MIOS32_IRQ_Enable();
  }

  // no running status optimisation
  for(i=0; i<NUM_MIDI_OUT; ++i) {
	  tx_sysex_buffer_ptr[i] = 0;
    MIOS32_UART_MIDI_RS_OptimisationSet(i, 0);
  }

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
#if MIOS32_UART_NUM == 0
  return -1; // no UART available
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

  return 0; // no error
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
#if MIOS32_UART_NUM == 0
  return -1; // no UART available
#else
  int i;
	
  if( uart >= NUM_MIDI_OUT )
    return -1; // UART not available

  MIOS32_IRQ_Disable(); // operation should be atomic
	
  // SysEx streams should be buffered to ensure atomic sends
  if( buffer[0] == 0xf0 ) { // we know that F0 always located at beginning of buffer due to MIOS32 package format
    tx_sysex_buffer_ptr[uart] = 0;
	for(i=0; i<len; ++i)
	  tx_sysex_buffer[uart][tx_sysex_buffer_ptr[uart]++] = buffer[i];
	len = 0; // don't send buffer
  } else if( tx_sysex_buffer_ptr[uart] && buffer[0] < 0xf8 ) {
    u8 send_sysex = 0;

	if( (buffer[0] & 0x80) && buffer[0] != 0xf7 ) {
	  send_sysex = 1; // SysEx hasn't been terminated with F7... however
	  // send buffer, therefore don't zero len
	} else {
	  if( (tx_sysex_buffer_ptr[uart] + len) >= MIOS32_UART_TX_BUFFER_SIZE ) {
	    // stream too long and cannot be sent completely - skip and don't return error state! (or should we split it into multiple sends?)
		tx_sysex_buffer_ptr[uart] = 0;
		len = 0; // don't send buffer
	  } else {
		for(i=0; i<len; ++i) {
		  if( buffer[i] == 0xf7 )
		    send_sysex = 1;
		  tx_sysex_buffer[uart][tx_sysex_buffer_ptr[uart]++] = buffer[i];
	    }
		len = 0; // don't send buffer
	  }
	}
	  
    if( send_sysex ) {
		MIDIPacketList packetList;
		MIDIPacket *packet = MIDIPacketListInit(&packetList);
		
		packet = MIDIPacketListAdd(&packetList, sizeof(packetList), packet,
								   0, // timestamp
								   tx_sysex_buffer_ptr[uart], (u8 *)&tx_sysex_buffer[uart]);
		[virtualMIDI_OUT[uart] addSender:_self];
		[virtualMIDI_OUT[uart] processMIDIPacketList:&packetList sender:_self];
		[virtualMIDI_OUT[uart] removeSender:_self];		
	}
  }

  MIDIPacketList packetList;
  MIDIPacket *packet = MIDIPacketListInit(&packetList);

  if( len ) {
    packet = MIDIPacketListAdd(&packetList, sizeof(packetList), packet,
							   0, // timestamp
							   len, buffer);
	[virtualMIDI_OUT[uart] addSender:_self];
	[virtualMIDI_OUT[uart] processMIDIPacketList:&packetList sender:_self];
	[virtualMIDI_OUT[uart] removeSender:_self];		
  }

  MIOS32_IRQ_Enable();
	
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
  s32 error;

  while( (error=MIOS32_UART_TxBufferPutMore_NonBlocking(uart, buffer, len)) == -2 );

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
  // for more comfortable usage...
  // -> just forward to MIOS32_UART_TxBufferPutMore
  return MIOS32_UART_TxBufferPutMore(uart, &b, 1);
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
  s32 error;

  while( (error=MIOS32_UART_TxBufferPutMore(uart, &b, 1)) == -2 );

  return error;
}



/////////////////////////////////////////////////////////////////////////////
// MIDI Event Receiver
/////////////////////////////////////////////////////////////////////////////
- (void)processMIDIPacketList:(const MIDIPacketList*)packets sender:(id)sender
{
	// from http://svn.notahat.com/simplesynth/trunk/AudioSystem.m
    int						i, j, k;
    const MIDIPacket*		packet;
    Byte					message[256];
    int						messageSize = 0;
    mios32_midi_port_t      port = DEFAULT;

	for(i=0; i<NUM_MIDI_IN; ++i) {
		if( sender == virtualMIDI_IN[i] ) {
			port = UART0 + i;
			break;
		}
	}

	if( i == NUM_MIDI_IN ) {
		DEBUG_MSG("[processMIDIPacketList] FATAL ERROR: unknown MIDI port!\n");
		return;
	}

    // Step through each packet
    packet = packets->packet;
    for (i = 0; i < packets->numPackets; i++) {
        for (j = 0; j < packet->length; j++) { 

            if (packet->data[j] >= 0xF8) {
				if( MIOS32_MIDI_SendByteToRxCallback(port, packet->data[j]) == 0 )
					MIOS32_UART_RxBufferPut(port & 0x0f, packet->data[j]);
			} else {
				// Hand off the packet for processing when the next one starts
				if ((packet->data[j] & 0x80) != 0 && (messageSize > 0 || packet->data[j] >= 0xf8) ) {
					for(k=0; k<messageSize; ++k) {
						if( MIOS32_MIDI_SendByteToRxCallback(port, message[k]) == 0 )
							MIOS32_UART_RxBufferPut(port & 0x0f, message[k]);
					}
					messageSize = 0;
				}
				
				message[messageSize++] = packet->data[j];			// push the data into the message
            }
        }
        
        packet = MIDIPacketNext (packet);
    }
    
    if (messageSize > 0) {
		for(k=0; k<messageSize; ++k) {
			if( MIOS32_MIDI_SendByteToRxCallback(port, message[k]) == 0 )
				MIOS32_UART_RxBufferPut(port & 0x0f, message[k]);
		}
	}
}

@end
