/* $Id$ */
/*
vX32 pre-alpha
not for any use whatsoever
copyright stryd_one
bite me corp 2008

big props to nILS for being my fourth eye and TK for obvious reasons
stay tuned for UI prototyping courtesy of lucem!

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


/////////////////////////////////////////////////////////////////////////////
// Local Variables
/////////////////////////////////////////////////////////////////////////////

static s32 vx_midi_rx(mios32_midi_port_t port, u8 midi_byte);
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
	MIOS32_LCD_PrintString("vX0.1");
	
	// initialize SEQ module MIDI handler
	SEQ_MIDI_OUT_Init(0);
	
	// initialize vX Rack
	graph_init();
	
	// initialize vX Clocks
	clocks_init();
	

	// install MIDI Rx/Tx callback functions
	MIOS32_MIDI_DirectRxCallback_Init(vx_midi_rx);
	
	TASKS_Init();
	
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
	
unsigned char testmodule1; //FIXME TESTING
unsigned char testmodule2; //FIXME TESTING
edge_t *testedge1; //FIXME TESTING
unsigned char testmodule3; //FIXME TESTING
unsigned char testmodule4; //FIXME TESTING
edge_t *testedge2; //FIXME TESTING
unsigned char testmodule5; //FIXME TESTING
unsigned char testmodule6; //FIXME TESTING
edge_t *testedge3; //FIXME TESTING
	
	
	// buffer up the incoming events
	// notify each host node by incrementing the process_req flag
	// process all the nodes downstream of the host node
	if ((midi_package.type == NoteOn) && (midi_package.evnt2 == 0x7f)) { //FIXME TESTING
		
	testmodule1 = node_add(0);
		
	testmodule2 = node_add(1);
	
	testedge1 = edge_add(testmodule1, MOD_SCLK_PORT_NEXTTICK, testmodule2, MOD_SEQ_PORT_NEXTTICK);
	
	
	testmodule3 = node_add(0);
	
	testmodule4 = node_add(1);
	
	testedge2 = edge_add(testmodule3, MOD_SCLK_PORT_NEXTTICK, testmodule4, MOD_SEQ_PORT_NEXTTICK);
/**/
	
/**/
	node[testmodule3].ports[MOD_SCLK_PORT_NUMERATOR] = 7;
	node[testmodule3].process_req++;
	mod_preprocess(testmodule3);

	
	testmodule5 = node_add(0);
	
	testmodule6 = node_add(1);
	
	testedge3 = edge_add(testmodule5, MOD_SCLK_PORT_NEXTTICK, testmodule6, MOD_SEQ_PORT_NEXTTICK);
/**/
	
/**/
	node[testmodule5].ports[MOD_SCLK_PORT_NUMERATOR] = 9;
	node[testmodule5].ports[MOD_SCLK_PORT_DENOMINATOR] = 2;
	node[testmodule5].process_req++;
	mod_preprocess(testmodule5);

	
	
	
	
	}
	//FIXME TESTING BPM Master
	if (midi_package.evnt0 == 0xB0) {
		if (midi_package.evnt1 == 0x40) SEQ_BPM_Start();
		if (midi_package.evnt1 == 0x45) SEQ_BPM_Stop();
		if (midi_package.evnt1 == 0x7c) SEQ_BPM_Cont();
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
s32 vx_midi_rx(mios32_midi_port_t port, u8 midi_byte) {
	SEQ_BPM_NotifyMIDIRx(midi_byte);
	return 0;
}

void vx_midi_tx(mios32_midi_port_t port, u8 midi_byte) {
}



/////////////////////////////////////////////////////////////////////////////
// This task is switched each 1ms
/////////////////////////////////////////////////////////////////////////////
void vx_task_rack_tick(void) {
	
	//FIXME TESTING 
	MIOS32_BOARD_LED_Set(0xffffffff, 2);
	
	mod_tick();														// send the output queues and then preprocess
	
	//FIXME TESTING 
	MIOS32_BOARD_LED_Set(0xffffffff, 1);
	
	mclock_tick();													// check for clock ticks
	
	//FIXME TESTING 
	MIOS32_BOARD_LED_Set(0xffffffff, 0);
	
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
		// Do CS stuff
		vTaskDelay( xDelay );
	}

}


