/* $Id$ */
/*
vX32 pre-alpha
not for any use whatsoever
copyright stryd_one
bite me corp 2008

big props to nILS for being my fourth eye and TK for obvious reasons
*/


/////////////////////////////////////////////////////////////////////////////
// Include files
/////////////////////////////////////////////////////////////////////////////

#include <FreeRTOS.h>
#include <portmacro.h>

#include <mios32.h>

#include "tasks.h"

#include "app.h"
#include "graph.h"
#include "mclock.h"
#include "modules.h"
#include "patterns.h"
#include "utils.h"
#include "ui.h"

#include <seq_midi_out.h>
#include <seq_bpm.h>


/////////////////////////////////////////////////////////////////////////////
// Global Variables
/////////////////////////////////////////////////////////////////////////////

unsigned char testmodule1; //FIXME TESTING
unsigned char testmodule2; //FIXME TESTING
edge_t *testedge1; //FIXME TESTING

/////////////////////////////////////////////////////////////////////////////
// Local Variables
/////////////////////////////////////////////////////////////////////////////

static void vx_midi_rx(mios32_midi_port_t port, u8 midi_byte);
static void vx_midi_tx(mios32_midi_port_t port, u8 midi_byte);


/////////////////////////////////////////////////////////////////////////////
// This hook is called after startup to initialize the application
/////////////////////////////////////////////////////////////////////////////
void APP_Init(void) {
	MIOS32_DELAY_Init(0);
	MIOS32_BOARD_LED_Init(0xffffffff);
	MIOS32_BOARD_LED_Set(0xffffffff, 0);

	
	MIOS32_LCD_BColourSet(0x00, 0x00, 0x00);
	MIOS32_LCD_Clear();

	MIOS32_LCD_CursorSet(1, 1);
	MIOS32_LCD_PrintString("stryd_one");

	MIOS32_LCD_FColourSet(0x00, 0x00, 0xff);
	MIOS32_LCD_CursorSet(2, 2);
	MIOS32_LCD_PrintString("vX0.0");
	
	// initialize SEQ module MIDI handler
	SEQ_MIDI_OUT_Init(0);
	
	// initialize vX Rack
	graph_init();
	
	// initialize vX Clocks
	clocks_init();
	

	// install MIDI Rx/Tx callback functions
	MIOS32_MIDI_DirectRxTxCallback_Init(vx_midi_rx, vx_midi_tx);
	
	TASKS_Init();
	
	// FIXME TESTING
		
	sclock_init(0, 8, 1, 1, 1);

	sclock[0].status = 0x80; 
		
	sclock_init(1, 10, 1, 1, 1);

	sclock[1].status = 0x80; 
	
	
	testmodule1 = node_add(1);
	
	mod_set_clocksource(testmodule1, 0); //FIXME TESTING
	
	testmodule2 = node_add(1);
	
	mod_set_clocksource(testmodule2, 1); //FIXME TESTING
	
	// FIXME TESTING
	
}


/////////////////////////////////////////////////////////////////////////////
// This task is running endless in background
/////////////////////////////////////////////////////////////////////////////
void APP_Background(void) {
}


/////////////////////////////////////////////////////////////////////////////
//  This hook is called when a complete MIDI event has been received
/////////////////////////////////////////////////////////////////////////////
void APP_NotifyReceivedEvent(mios32_midi_port_t port, mios32_midi_package_t midi_package) {
	// buffer up the incoming events
	// notify each host node by incrementing the process_req flag
	// process all the nodes downstream of the host node
	if (midi_package.evnt2 == 0x7f) {
		
		testmodule1 = node_add(1);
		
		mod_set_clocksource(testmodule1, 0); //FIXME TESTING
	
	}
	
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called when a SysEx byte has been received
/////////////////////////////////////////////////////////////////////////////
void APP_NotifyReceivedSysEx(mios32_midi_port_t port, u8 sysex_byte) {
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called when a byte has been received via COM interface
/////////////////////////////////////////////////////////////////////////////
void APP_NotifyReceivedCOM(mios32_com_port_t port, u8 byte) {
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called before the shift register chain is scanned
/////////////////////////////////////////////////////////////////////////////
void APP_SRIO_ServicePrepare(void) {
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called after the shift register chain has been scanned
/////////////////////////////////////////////////////////////////////////////
void APP_SRIO_ServiceFinish(void) {
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called when a button has been toggled
// pin_value is 1 when button released, and 0 when button pressed
/////////////////////////////////////////////////////////////////////////////
void APP_DIN_NotifyToggle(u32 pin, u32 pin_value) {
	// jump to the CS handler
	// edit a value of one of the modules according to the menus
	// notify modified node by incrementing the process_req flag
	// process all the nodes downstream of the host node
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called when an encoder has been moved
// incrementer is positive when encoder has been turned clockwise, else
// it is negative
/////////////////////////////////////////////////////////////////////////////
void APP_ENC_NotifyChange(u32 encoder, s32 incrementer) {
	// see APP_DIN_NotifyToggle
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called when a pot has been moved
/////////////////////////////////////////////////////////////////////////////
void APP_AIN_NotifyChange(u32 pin, u32 pin_value) {
}





/////////////////////////////////////////////////////////////////////////////
// Installed via MIOS32_MIDI_DirectRxTxCallback_Init
// Executed immediately on each incoming MIDI byte, partly from interrupt
// handlers!
// These function should be executed so fast as possible. They can be used
// to trigger MIDI Rx/Tx LEDs or to trigger on MIDI clock events. In order to
// avoid MIDI buffer overruns, the max. recommented execution time is 100 uS!
/////////////////////////////////////////////////////////////////////////////
void vx_midi_rx(mios32_midi_port_t port, u8 midi_byte) {
	SEQ_BPM_NotifyMIDIRx(midi_byte);
}

void vx_midi_tx(mios32_midi_port_t port, u8 midi_byte) {
}



/////////////////////////////////////////////////////////////////////////////
// This task is switched each 1ms
/////////////////////////////////////////////////////////////////////////////
void vx_task_rack_tick(void) {
	mod_tick();				 										// send the output queues
	mclock_tick();													// check for sclock ticks
    SEQ_MIDI_OUT_Handler();											// send timestamped MIDI events
}


/////////////////////////////////////////////////////////////////////////////
// This task is switched each 1ms
/////////////////////////////////////////////////////////////////////////////
void vx_task_midi(void) {
	// handle buffers of incoming MIDI data
}


/////////////////////////////////////////////////////////////////////////////
// This task is switched each 1ms
/////////////////////////////////////////////////////////////////////////////
void vx_task_period1ms(void) {
static unsigned int perfctr = 2500;

const portTickType xDelay = 500 / portTICK_RATE_MS;



	while (1) {
		vportMallInfo_t memuse = vPortMallInfo();
		
		MIOS32_LCD_Clear();
		
		//MIOS32_LCD_FColourSet(0x00, 0x00, 0xff);
		MIOS32_LCD_CursorSet(1, 4);
		/* This is the total size of the heap */
		MIOS32_LCD_PrintFormattedString( "Total %u B", memuse.totalheap);
		
		MIOS32_LCD_CursorSet(1, 5);
		/* This is the total size of memory occupied by chunks handed out by malloc. */
		MIOS32_LCD_PrintFormattedString( "In use %d B", memuse.totalheap - memuse.freespace);
		
		MIOS32_LCD_CursorSet(1, 6);
		/* This is the total size of memory occupied by free (not in use) chunks. */
		MIOS32_LCD_PrintFormattedString( "Free %u B", memuse.freespace);
		
		MIOS32_LCD_CursorSet(1, 7);
		/* This is the total number of chunks free to allocate with malloc. */
		MIOS32_LCD_PrintFormattedString( "Free %u chunks", memuse.freeblocks);
		
		MIOS32_LCD_CursorSet(1, 8);
		/* This is the size of the bottom-most free chunk that normally borders
		the start of the heap (i.e., the smallest available block). */
		MIOS32_LCD_PrintFormattedString( "Smallest is %d B", memuse.smallestblock);
		
		MIOS32_LCD_CursorSet(1, 9);
		/* This is the size of the top-most free chunk that normally borders
		the end of the heap (i.e., the largest available block). */
		MIOS32_LCD_PrintFormattedString( "Largest is %d B", memuse.largestblock);
		
		
		// Do CS stuff
		vTaskDelay( xDelay );
	}

}


