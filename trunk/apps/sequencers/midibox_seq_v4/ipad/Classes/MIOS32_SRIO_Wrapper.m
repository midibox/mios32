//
//  MIOS32_SRIO_Wrapper.m
//  midibox_seq_v4
//
//  Created by Thorsten Klose on 06.12.08.
//  Copyright 2008 __MyCompanyName__. All rights reserved.
//

#import "MIOS32_SRIO_Wrapper.h"

#include <mios32.h>


@implementation MIOS32_SRIO_Wrapper

/////////////////////////////////////////////////////////////////////////////
// Global variables
/////////////////////////////////////////////////////////////////////////////

// DOUT SR registers in reversed (!) order (since DMA doesn't provide a decrement address function)
// Note that also the bits are in reversed order compared to PIC based MIOS
// So long the array is accessed via MIOS32_DOUT_* functions, they programmer won't notice a difference!
volatile u8 mios32_srio_dout[MIOS32_SRIO_NUM_SR];

// DIN values of last scan
volatile u8 mios32_srio_din[MIOS32_SRIO_NUM_SR];

// DIN values of ongoing scan
// Note: during SRIO scan it is required to copy new DIN values into a temporary buffer
// to avoid that a task already takes a new DIN value before the whole chain has been scanned
// (e.g. relevant for encoder handler: it has to clear the changed flags, so that the DIN handler doesn't take the value)
volatile u8 mios32_srio_din_buffer[MIOS32_SRIO_NUM_SR];

// change notification flags
volatile u8 mios32_srio_din_changed[MIOS32_SRIO_NUM_SR];

//////////////////////////////////////////////////////////////////////////////
// local variables to bridge objects to C functions
//////////////////////////////////////////////////////////////////////////////
static NSObject *_self;


//////////////////////////////////////////////////////////////////////////////
// init local variables
//////////////////////////////////////////////////////////////////////////////
- (void) awakeFromNib
{
	_self = self;
}


/////////////////////////////////////////////////////////////////////////////
// Initializes SPI pins and peripheral
// IN: <mode>: currently only mode 0 supported
//             later we could provide different pin mappings and operation 
//             modes (e.g. output only)
// OUT: returns < 0 if initialisation failed
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_SRIO_Init(u32 mode)
{
	// currently only mode 0 supported
	if( mode != 0 )
		return -1; // unsupported mode

	// clear chains
	// will be done again in MIOS32_DIN_Init and MIOS32_DOUT_Init
	u8 i;

	// we don't reference to these functions here to allow the programmer to remove/replace these driver modules)
	for(i=0; i<MIOS32_SRIO_NUM_SR; ++i) {
		mios32_srio_dout[i] = 0x00;       // passive state (LEDs off)
		mios32_srio_din[i] = 0xff;        // passive state (Buttons depressed)
		mios32_srio_din_buffer[i] = 0xff; // passive state (Buttons depressed)
		mios32_srio_din_changed[i] = 0;   // no change
	}

	return 0;
}


/////////////////////////////////////////////////////////////////////////////
// (Re-)Starts the SPI IRQ Handler which scans the SRIO chain
// IN: notification function which will be called after the scan has been finished
//     (all DOUT registers written, all DIN registers read)
//     use NULL if no function should be called
// OUT: returns < 0 if operation failed
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_SRIO_ScanStart(void *_notify_hook)
{
#if MIOS32_SRIO_NUM_SR == 0
	return -1; // no SRIO scan required
#endif
	void (*srio_scan_finished_hook)(void) = _notify_hook;
	// in emulation, we can report new values immediately
	// copy/or buffered DIN values/changed flags
	int i;
	for(i=0; i<MIOS32_SRIO_NUM_SR; ++i) {
		mios32_srio_din_changed[i] |= mios32_srio_din[i] ^ mios32_srio_din_buffer[i];
		mios32_srio_din[i] = mios32_srio_din_buffer[i];
	}

	// call user specific hook if requested
	if( srio_scan_finished_hook != NULL )
		srio_scan_finished_hook();

	return 0;
}


/////////////////////////////////////////////////////////////////////////////
// dummy stub
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_SRIO_DebounceStart(void)
{
	return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// dummy stub
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_SRIO_DebounceSet(u8 _debounce_time)
{
	return 0; // no error
}

@end
