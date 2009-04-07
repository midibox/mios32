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



/////////////////////////////////////////////////////////////////////////////
// local prototypes
/////////////////////////////////////////////////////////////////////////////

static s32 vX_MIDI_Rx(mios32_midi_port_t port, u8 midi_byte);                   // MIOS callback
static void vX_MIDI_Tx(mios32_midi_port_t port, u8 midi_byte);                  // MIOS callback


/////////////////////////////////////////////////////////////////////////////
// This hook is called after startup to initialize the application
/////////////////////////////////////////////////////////////////////////////
void APP_Init(void) {
    MIOS32_DELAY_Init(0);
    MIOS32_BOARD_LED_Init(0xffffffff);
    MIOS32_BOARD_LED_Set(0xffffffff, 0);
    
    
    MIOS32_LCD_BColourSet(0x00, 0x00, 0x00);
    MIOS32_LCD_Clear();
    
    MIOS32_LCD_FColourSet(0xff, 0xff, 0xff);
    MIOS32_LCD_CursorSet(6, 3);
    MIOS32_LCD_PrintString("vX32 a-24");
    
    MIOS32_LCD_BColourSet(0x00, 0x00, 0xff);
    MIOS32_LCD_FColourSet(0x00, 0x00, 0x00);
    MIOS32_LCD_CursorSet(6, 5);
    MIOS32_LCD_PrintString("Seven");
    MIOS32_LCD_BColourSet(0x00, 0x00, 0x00);
    MIOS32_LCD_FColourSet(0xff, 0xff, 0xff);
    
    
    
    SEQ_MIDI_OUT_Init(0);                                                       // initialize SEQ module MIDI handler
    
    Mod_Init_ModuleData();                                                      // initialize vX modules
    
    
    Graph_Init();                                                               // initialize vX Rack
    
    Clocks_Init();                                                              // initialize vX Clocks
    
    MIOS32_MIDI_DirectRxCallback_Init((void *)vX_MIDI_Rx);                              // install MIDI Rx/Tx callback functions
    
    TASKS_Init(0);                                                              // start threads
    
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
    
static unsigned char testmodule1; //FIXME TESTING
static unsigned char testmodule2; //FIXME TESTING
static edge_t *testedge1; //FIXME TESTING
static unsigned char testmodule3; //FIXME TESTING
static unsigned char testmodule4; //FIXME TESTING
static edge_t *testedge2; //FIXME TESTING
static unsigned char testmodule5; //FIXME TESTING
static unsigned char testmodule6; //FIXME TESTING
static edge_t *testedge3; //FIXME TESTING

static edge_t *testedge4; //FIXME TESTING
static edge_t *testedge5; //FIXME TESTING
    

    // buffer up the incoming events
    // notify each host node by incrementing the process_req flag
    // process all the nodes downstream of the host node
    if ((midi_package.type == NoteOn) && (midi_package.evnt2 == 0x7f)) { //FIXME TESTING
        
    testmodule1 = UI_NewModule(MOD_MODULETYPE_SCLK);
    testmodule2 = UI_NewModule(MOD_MODULETYPE_SEQ);
    
    node[testmodule1].ports[MOD_SCLK_PORT_NUMERATOR] = 4;
    node[testmodule1].process_req++;
    
    testedge1 = UI_NewCable(testmodule1, MOD_SCLK_PORT_NEXTTICK, testmodule2, MOD_SEQ_PORT_NEXTTICK);
    
    
    
    testmodule3 = UI_NewModule(MOD_MODULETYPE_SCLK);    
    testmodule4 = UI_NewModule(MOD_MODULETYPE_SEQ);
    
    node[testmodule3].ports[MOD_SCLK_PORT_NUMERATOR] = 8;
    node[testmodule3].process_req++;
    
    testedge2 = UI_NewCable(testmodule3, MOD_SCLK_PORT_NEXTTICK, testmodule4, MOD_SEQ_PORT_NEXTTICK);
    
    
    
    testmodule5 = UI_NewModule(0);    
    testmodule6 = UI_NewModule(1);
    
    testedge3 = UI_NewCable(testmodule5, MOD_SCLK_PORT_NEXTTICK, testmodule6, MOD_SEQ_PORT_NEXTTICK);
    
    node[testmodule5].ports[MOD_SCLK_PORT_NUMERATOR] = 5;
    node[testmodule5].process_req++;
    
    
    
    testedge4 = UI_NewCable(testmodule2, MOD_SEQ_PORT_CURRENTSTEP, testmodule4, MOD_SEQ_PORT_NOTE0_NOTE);
    testedge5 = UI_NewCable(testmodule4, MOD_SEQ_PORT_CURRENTSTEP, testmodule6, MOD_SEQ_PORT_NOTE0_NOTE);
    
    
    }

    //FIXME TESTING BPM Master
    if (midi_package.evnt0 == 0xB0) {
        if (midi_package.evnt1 == 0x40) MClock_User_Start();
        if (midi_package.evnt1 == 0x45) MClock_User_Stop();
        if (midi_package.evnt1 == 0x7c) MClock_User_Continue();
        if (midi_package.evnt1 == 0x7d) {
            MIOS32_MIDI_SendCC(DEFAULT, Chn3, 0x02, UI_RemoveModule(testmodule3));
        }
        if (midi_package.evnt1 == 0x7e) {
            MIOS32_MIDI_SendCC(DEFAULT, Chn3, 0x01, UI_RemoveCable(testmodule2, MOD_SEQ_PORT_CURRENTSTEP, testmodule4, MOD_SEQ_PORT_NOTE0_NOTE));
        }
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
s32 vX_MIDI_Rx(mios32_midi_port_t port, u8 midi_byte) {
    SEQ_BPM_NotifyMIDIRx(midi_byte);                                // Forward incoming MIDI to the SEQ_BPM module
    return 0;
}

void vX_MIDI_Tx(mios32_midi_port_t port, u8 midi_byte) {
}



/////////////////////////////////////////////////////////////////////////////
// This task is switched each 1ms
/////////////////////////////////////////////////////////////////////////////
void vX_Task_Rack_Tick(void) {
    unsigned int timeout = RACK_TICK_TIMEOUT;
    while (--timeout > 0) {
            if (MUTEX_GRAPH_TAKE == pdTRUE) {                                           // Take the Mutex to write to the graph

            MIOS32_BOARD_LED_Set(0xffffffff, 1);                                    // Green LED
            
            Mod_PreProcess(DEAD_NODEID);                                            // preprocess data ASAP
            
            MIOS32_BOARD_LED_Set(0xffffffff, 2);                                    // Red LED
            
            Mod_Tick();                                                             // send the output queues and then preprocess
            
            MIOS32_BOARD_LED_Set(0xffffffff, 0);                                    // Both LEDs
            
            MClock_Tick();                                                          // check for clock ticks
            
            MUTEX_GRAPH_GIVE;                                                            // Give back the Mutex to write to the graph
            break;
        }
        
    }
                                                                                // timeout! Who's hogging the mutex? We should never get here!
}


/////////////////////////////////////////////////////////////////////////////
// This task is switched each 1ms
/////////////////////////////////////////////////////////////////////////////
void vX_Task_MIDI(void) {
    MIOS32_BOARD_LED_Set(0xffffffff, 0);                                        // LED off
    SEQ_MIDI_OUT_Handler();                                                     // send timestamped MIDI events
}


/////////////////////////////////////////////////////////////////////////////
// This task is switched each 1ms
/////////////////////////////////////////////////////////////////////////////
void vX_Task_UI(void) {
    UI_Tick();
    
}


// FIXME todo
// inputs! DIN/MIDI/ETH/etc....
