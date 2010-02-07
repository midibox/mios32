// BLM.cpp
//
//
// Created by Phil Taylor 2010

#include "includes.h"
//#include "BlmClass.h"
#include "MainComponent.h"
BlmClass::BlmClass(int cols,int rows)
{
	setBlmDimensions(cols,rows); 
	ledColours=2;
	int x,y;
	for (x=0;x<MAX_COLS;x++){
		for (y=0;y<MAX_ROWS;y++){
			addAndMakeVisible (buttons[x][y] = new TextButton (String::empty));
		    buttons[x][y]->addButtonListener (this);
			//buttons[x][y]->setClickingTogglesState(true);
			//if (x<cols && y < rows){
				buttons[x][y]->setColour (TextButton::buttonColourId, Colours::azure);
				buttons[x][y]->setColour (TextButton::buttonOnColourId, Colours::blue);
			//} else {
			//	buttons[x][y]->setEnabled(false);
			//}
		}
	}
    Timer::startTimer(1);

}


BlmClass::~BlmClass()
{
	//midiInput->stop();
    Timer::stopTimer();
	deleteAndZero (midiInput);
	deleteAndZero (midiOutput);

	int x,y;
	for (x=0;x<MAX_COLS;x++){
		for (y=0;y<MAX_ROWS;y++){
			deleteAndZero (buttons[x][y]);
		}
	}
}

void BlmClass::resized()
{
	int ledSize=getWidth()/getBlmColumns();

	int x,y;
	for (x=0;x<MAX_COLS;x++){
		for (y=0;y<MAX_ROWS;y++){
			if (x<blmColumns && y<blmRows) {
				buttons[x][y]->setVisible(true);
				buttons[x][y]->setSize(ledSize,ledSize);
				buttons[x][y]->setTopLeftPosition(x*ledSize,y*ledSize);
			} else {
				buttons[x][y]->setVisible(false);
			}
		}
	}
	sendBLMLayout();

}

void BlmClass::buttonClicked (Button* buttonThatWasClicked)
{
}

void BlmClass::buttonStateChanged (Button* buttonThatWasClicked)
{

	if(!midiOutput || !buttonThatWasClicked->isMouseOver()) // Should stop phantom events!
		return;
	
	int x,y,row=-1,col=-1;
	for (x=0;x<getBlmColumns();x++){
		for (y=0;y<getBlmRows();y++){
			if (buttonThatWasClicked==buttons[x][y]) {
				col=x;
				row=y;
				break;
			}
		}
	}
	if ((row == -1) && ( col == -1)) {
		// Something REALLY bad happened
		return;
	}

	sendNoteEvent(row,col,(buttonThatWasClicked->isMouseButtonDown() ? 0x7f:0x00));
}


void BlmClass::closeMidiPorts(void)
{
	if (midiInput)
		midiInput->stop();
}


void BlmClass::setMidiOutput(int port)
{
	outputPort=port;
	midiOutput=MidiOutput::openDevice(port);
}

void BlmClass::setMidiInput(int port)
{
	inputPort=port;
	midiInput=MidiInput::openDevice(port,this);
	midiInput->start();
}


void BlmClass::setButtonState(int col, int row, int state)
{
	Colour LED_State;
	switch (state) {
	case 1: LED_State= Colours::green; break;
	case 2: LED_State= Colours::red; break;
	case 3: LED_State= Colours::yellow; break;
	default: LED_State= Colours::blue; break;
	}
	buttons[col][row]->setColour(TextButton::buttonOnColourId,LED_State);
}

int BlmClass::getButtonState(int col, int row)
{
	// Use the current colour property to determine the button state.
	int LED_State;
	Colour current=buttons[col][row]->findColour(TextButton::buttonOnColourId);
	if (current==Colours::green) 
		LED_State=1;
	else if (current==Colours::red)
		LED_State=2; 
	else if (current==Colours::yellow)
		LED_State=3; 
	else
		LED_State=0;
	
	return LED_State;
}

void BlmClass::handleIncomingMidiMessage(MidiInput *source, const MidiMessage &message)
{
	midiQueue.push(message);
}



void BlmClass::BLMIncomingMidiMessage(const MidiMessage &message, uint8 RunningStatus)
{
	uint8 *data=message.getRawData();
	uint8 event_type = data[0] >> 4;
	uint8 chn = data[0] & 0xf;
	uint8 evnt1 = data[1];
	uint8 evnt2 = data[2];

	int LED_State;
	if( evnt2 == 0 )
		LED_State = 0;
	else if( evnt2 < 0x40 )
		LED_State = 1;
	else if( evnt2 < 0x60 )
		LED_State = 2;
	else
		LED_State = 3;


	switch( event_type ) {
		case 0x8: // Note Off
			evnt2 = 0; // handle like Note On with velocity 0
		case 0x9:
			if( evnt1 < blmColumns ) {
				int row = chn;
				int column = evnt1;
				setButtonState(column,row,LED_State);
				if (!evnt2)
					buttons[column][row]->setToggleState(false,false);
				else
					buttons[column][row]->setToggleState(true,false);

			}
			break;

		case 0xb: {
			MainComponent* parent=(MainComponent *)this->getParentComponent();

			switch( evnt1 ) {
				case 0x40:
					parent->toggleTriggersButton(LED_State?true:false);
					//[buttonTriggers setLED_State:LED_State];
					break;
					
				case 0x41:
					parent->toggleTracksButton(LED_State?true:false);
					//[buttonTracks setLED_State:LED_State];
					break;
					
				case 0x42:
					parent->togglePatternsButton(LED_State?true:false);
					//[buttonPatterns setLED_State:LED_State];
					break;
					
				default: {
					int pattern = evnt2 | ((evnt1&1)<<7);
					int row = chn;
					int column_offset = 0;
					int state_mask = 0;
			
					if( (evnt1 & 0xf0) == 0x10 ) { // green LEDs
						state_mask = (1 << 0);
						column_offset = 8*((evnt1%16)/2);
					} else if( (evnt1 & 0xf0) == 0x20 ) { // red LEDs
						state_mask = (1 << 1);
						column_offset = 8*((evnt1%16)/2);
					}

					// change the colour of 8 LEDs if the appr. CC has been received
					if( state_mask > 0 ) {
						int column;
						for(column=0; column<8; ++column) {
							int led_mask = (pattern & (1 << column)) ? state_mask : 0;
							int _column = column+column_offset;
							int state = getButtonState(column,row);
							int new_state = (state & ~state_mask) | led_mask;
							setButtonState(_column,row,new_state);
							if (!new_state)
								buttons[_column][row]->setToggleState(false,false);
							else
								buttons[_column][row]->setToggleState(true,false);
						}
					}
				}
			}
		} break;

		case 0xf: {
			// in the hope that SysEx messages will always be sent in a single packet...
			int size=message.getRawDataSize();
			if( size >= 8 &&
			   data[0] == 0xf0 &&
			   data[1] == 0x00 &&
			   data[2] == 0x00 &&
			   data[3] == 0x7e &&
			   data[4] == 0x4e && // MBHP_BLM_SCALAR
			   data[5] == 0x00  // Device ID
			   ) {
				if( data[6] == 0x00 && data[7] == 0x00 ) {
					// no error checking... just send layout (the hardware version will check better)
					sendBLMLayout();
				}
			}
		} break;
	}
}


void BlmClass::sendBLMLayout(void)
{
	unsigned char sysex[11];
	sysex[0] = 0xf0;
	sysex[1] = 0x00;
	sysex[2] = 0x00;
	sysex[3] = 0x7e;
	sysex[4] = 0x4e; // MBHP_BLM_SCALAR ID
	sysex[5] = 0x00; // Device ID 00
	sysex[6] = 0x01; // Command #1 (Layout Info)
	sysex[7] = blmColumns & 0x7f;
	sysex[8] = blmRows;
	sysex[9] = ledColours;
	sysex[10] = 0xf7;
	MidiMessage message(sysex,11);
	if(midiOutput)
		midiOutput->sendMessageNow(message);
}


void BlmClass::sendCCEvent(int chn,int cc, int value)
{
	MidiMessage message(0xb0|chn,cc,value);
	if(midiOutput)
		midiOutput->sendMessageNow(message);
}


void BlmClass::sendNoteEvent(int chn,int key, int velocity)
{
	MidiMessage message(0x90|chn,key,velocity);
	if(midiOutput)
		midiOutput->sendMessageNow(message);
}

void BlmClass::timerCallback()
{
    MidiMessage message(0xfe);    

    while( !midiQueue.empty() ) {
        message = midiQueue.front();
        midiQueue.pop();

        uint8 *data = message.getRawData();
        if( data[0] >= 0x80 && data[0] < 0xf8 )
            runningStatus = data[0];

        // propagate incoming event to MIDI components
        BLMIncomingMidiMessage(message, runningStatus);

    }
}