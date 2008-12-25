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
// FIXME TESTING
	if (byte == 't') {
sclock_init(0, 4, 2, 1, 1);

sclock[0].status = 0x80; 
	} else if (byte == 'T'){
sclock_init(1, 4, 2, 3, 1);

sclock[1].status = 0x80; 
	
	} else if (byte == 'g'){
	

	testmodule1 = node_add(0); //FIXME TESTING
	testmodule2 = node_add(1); //FIXME TESTING
	mod_set_clocksource(testmodule1, 0); //FIXME TESTING
	mod_set_clocksource(testmodule2, 1); //FIXME TESTING

	
	} else if (byte == 'p'){
	
	mclock.status = 0x80;
	//mclock.ticked++;
	
	// if in auto mode and BPM generator is clocked in slave mode:
    // change to master mode
    SEQ_BPM_CheckAutoMaster();

    // start sequencer
    SEQ_BPM_Start();
	
	} else if (byte == 'n'){
	
	testedge1 = edge_add(testmodule1, 0, testmodule2, 3);
	
	
	} else if (byte == 'N'){
	
	edge_add(testmodule2, 5, testmodule1, 5);
	
	
	} else if (byte == 'd'){
	
	edge_del(testmodule1, testedge1, 1);
	
	
	} else if (byte == 'D'){
	
	node_del(testmodule1);
	
	
	} else if (byte == 'b'){
	
	mclock_setbpm(45);
	
	
	} else if (byte == 'B'){
	
	mclock_setbpm(180);
	
	
	} else if (byte == 'm'){
	
	testmodule1 = node_add(1);
	
	mod_set_clocksource(testmodule1, 0); //FIXME TESTING

	
	} else if (byte == 'M'){
	
	testmodule2 = node_add(1);
	mod_set_clocksource(testmodule2, 1); //FIXME TESTING
	
	
	} else {
	
	}
	
// FIXME TESTING
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
	// Do CS stuff
}


