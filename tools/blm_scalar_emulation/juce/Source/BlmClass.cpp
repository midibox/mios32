/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
// $Id$
// BLM.cpp
//
//
// Created by Phil Taylor 2010

#include "JuceHeader.h"
//#include "BlmClass.h"
#include "MainComponent.h"

#define TIMER_CHECK_MIDI  1
#define TIMER_SEND_PING   2

BlmClass::BlmClass(MainComponent *_mainComponent, int cols,int rows)
    : mainComponent(_mainComponent)
    , ledSize(40)
    , ledColours(2)
    , midiDataReceived(false)
    , lastButtonX(-1)
    , lastButtonY(-1)
    , lastMidiChannel(-1)
    , lastMidiNote(-1)
    , prevShiftState(false)
{
	for(int x=0;x<MAX_COLS;x++) {
		for(int y=0;y<MAX_ROWS;y++){
			addAndMakeVisible (buttons[x][y] = new BlmButton (String::empty));
			buttons[x][y]->setColour (BlmButton::buttonColourId, Colours::azure);
			buttons[x][y]->setColour (BlmButton::buttonOnColourId, Colours::blue);
			//buttons[x][y]->addListener(this);
			buttons[x][y]->setInterceptsMouseClicks(false, false);
		}
	}

    for(int y=0; y<MAX_ROWS; y++) {
        addAndMakeVisible(rowLabelsGreen[y] = new Label("", String(y+1)));
        rowLabelsGreen[y]->setColour(Label::textColourId, Colours::green);
        rowLabelsGreen[y]->setJustificationType(Justification::centredRight);

        addAndMakeVisible(rowLabelsRed[y] = new Label("", ""));
        rowLabelsRed[y]->setColour(Label::textColourId, Colours::red);
        rowLabelsRed[y]->setJustificationType(Justification::centredRight);
    }

	mainComponent->audioDeviceManager.addMidiInputCallback(String::empty, this);

    MultiTimer::startTimer(TIMER_CHECK_MIDI, 1);
    MultiTimer::startTimer(TIMER_SEND_PING, 5000);

	addMouseListener(this,true); // Add mouse listener for all child components
	
    // listen on key changes
    this->addKeyListener(this);

    // define final dimensions and set size
    setBLMLayout("16x16+X");
}


BlmClass::~BlmClass()
{
	for(int x=0;x<MAX_COLS;x++){
		for(int y=0;y<MAX_ROWS;y++){
			deleteAndZero(buttons[x][y]);
		}
	}

    for(int y=0; y<MAX_ROWS; y++) {
        deleteAndZero(rowLabelsGreen[y]);
        deleteAndZero(rowLabelsRed[y]);
    }

	deleteAllChildren();
}


void BlmClass::resized()
{
    for(int y=0; y<MAX_ROWS; y++) {
        int yOffset = buttonExtraColumnY + y*ledSize;
        if( y == blmRows )
            yOffset = buttonShiftY;
        int greenWidth = 25;

        if( y < (blmRows+1) ) {
            rowLabelsGreen[y]->setVisible(true);
            rowLabelsGreen[y]->setBounds(rowLabelsWidth-greenWidth, yOffset, greenWidth, ledSize);
            rowLabelsRed[y]->setVisible(true);
            rowLabelsRed[y]->setBounds(0, yOffset, rowLabelsWidth-greenWidth, ledSize);
        } else {
            rowLabelsGreen[y]->setVisible(false);
            rowLabelsRed[y]->setVisible(false);
        }
    }

	for(int x=0; x<MAX_COLS; x++) {
		for(int y=0; y<MAX_ROWS; y++) {
            int xOffset = -1;
            int yOffset = -1;

            if( x < blmColumns && y < blmRows ) {
                // 16x16 matrix
                xOffset = buttonArray16x16X + x*ledSize + 2;
                yOffset = buttonArray16x16Y + y*ledSize + 2;
            } else if( x == MAX_COLS_EXTRA_OFFSET && y == MAX_ROWS_EXTRA_OFFSET ) {
                // shift button
                xOffset = buttonShiftX;
                yOffset = buttonShiftY;
            } else if( x == MAX_COLS_EXTRA_OFFSET && y < blmRows ) {
                // extra column
                xOffset = buttonExtraColumnX;
                yOffset = buttonExtraColumnY + y*ledSize + 2;
            } else if( x == (MAX_COLS_EXTRA_OFFSET+1) && y < blmRows ) {
                // extra shift column
                xOffset = buttonExtraShiftColumnX;
                yOffset = buttonExtraShiftColumnY + y*ledSize + 2;
            } else if( x < blmColumns && y == MAX_ROWS_EXTRA_OFFSET ) {
                // extra row
                xOffset = buttonExtraRowX + x*ledSize + 2;
                yOffset = buttonExtraRowY + 2;
            }

            if( xOffset >= 0 && yOffset >= 0 ) {
                buttons[x][y]->setVisible(true);
                buttons[x][y]->setSize(ledSize, ledSize);
                buttons[x][y]->setTopLeftPosition(xOffset, yOffset);
                buttons[x][y]->setToggleState(false,false);
                buttons[x][y]->setColour(BlmButton::buttonOnColourId, Colours::blue);
            } else {
				buttons[x][y]->setVisible(false);
			}
        }
    }

    // set frame size of this component
    setSize(buttonArray16x16X + buttonArray16x16Width + 2, buttonExtraRowY + buttonExtraRowHeight + 2);
}


bool BlmClass::searchButtonIndex(const int& x, const int& y, int& buttonX, int& buttonY, int& midiChannel, int& midiNote)
{
    if( x >= buttonArray16x16X && x < (buttonArray16x16X+buttonArray16x16Width) &&
        y >= buttonArray16x16Y && y < (buttonArray16x16Y+buttonArray16x16Height) ) {
        buttonX = (x-buttonArray16x16X) / ledSize;
        buttonY = (y-buttonArray16x16Y) / ledSize;
        midiNote = buttonX;
        midiChannel = buttonY;
        //printf("X=%d Y=%d\n", buttonX, buttonY);
        return true;
    }

    if( x >= buttonExtraShiftColumnX && x < (buttonExtraShiftColumnX+buttonExtraShiftColumnWidth) &&
        y >= buttonExtraShiftColumnY && y < (buttonExtraShiftColumnY+buttonExtraShiftColumnHeight) ) {
        buttonX = MAX_COLS_EXTRA_OFFSET + ((x-buttonExtraShiftColumnX) / ledSize);
        buttonY = (y-buttonExtraShiftColumnY) / ledSize;
        midiNote = 0x50; // Event + Shift
        midiChannel = buttonY;
        return true;
    }

    if( x >= buttonExtraColumnX && x < (buttonExtraColumnX+buttonExtraColumnWidth) &&
        y >= buttonExtraColumnY && y < (buttonExtraColumnY+buttonExtraColumnHeight) ) {
        buttonX = MAX_COLS_EXTRA_OFFSET + ((x-buttonExtraColumnX) / ledSize);
        buttonY = (y-buttonExtraColumnY) / ledSize;
        midiNote = 0x40;
        midiChannel = buttonY;
        return true;
    }

    if( x >= buttonExtraRowX && x < (buttonExtraRowX+buttonExtraRowWidth) &&
        y >= buttonExtraRowY && y < (buttonExtraRowY+buttonExtraRowHeight) ) {
        buttonX = (x-buttonExtraRowX) / ledSize;
        buttonY = MAX_ROWS_EXTRA_OFFSET + ((y-buttonExtraRowY) / ledSize);
        midiNote = 0x60 + buttonX;
        midiChannel = 0x0;
        return true;
    }

    if( x >= buttonShiftX && x < (buttonShiftX+buttonShiftWidth) &&
        y >= buttonShiftY && y < (buttonShiftY+buttonShiftHeight) ) {
        buttonX = MAX_COLS_EXTRA_OFFSET;
        buttonY = MAX_ROWS_EXTRA_OFFSET;
        midiNote = 0x60;
        midiChannel = 0xf;
        return true;
    }

    return false;
}

//==============================================================================
void BlmClass::setButtonState(int col, int row, int state)
{
	buttons[col][row]->setButtonState(state);

    if( !state )
        buttons[col][row]->setToggleState(false, false);
    else
        buttons[col][row]->setToggleState(true, false);

    if( col < MAX_COLS_EXTRA_OFFSET && row < MAX_ROWS_EXTRA_OFFSET ) {
        mainComponent->extCtrlWindow->extCtrl->sendGridLed(col, row, state);
    } else if( col == MAX_COLS_EXTRA_OFFSET && row < MAX_ROWS_EXTRA_OFFSET ) {
        mainComponent->extCtrlWindow->extCtrl->sendExtraColumnLed(0, row, state);
    } else if( col == (MAX_COLS_EXTRA_OFFSET+1) && row < MAX_ROWS_EXTRA_OFFSET ) {
        mainComponent->extCtrlWindow->extCtrl->sendExtraColumnLed(1, row, state);
    } else if( col < MAX_COLS_EXTRA_OFFSET && row == MAX_ROWS_EXTRA_OFFSET ) {
        mainComponent->extCtrlWindow->extCtrl->sendExtraRowLed(col, 0, state);
    }

}

int BlmClass::getButtonState(int col, int row)
{
    return buttons[col][row]->getButtonState();
}

void BlmClass::setLed(const int& col, const int& row, const int& colourIx, const int& enabled)
{
    int stateMask = 1 << colourIx;

    int state = getButtonState(col, row) & ~stateMask;
    if( enabled )
        state |= stateMask;
    setButtonState(col, row, state);
}

void BlmClass::setLedPattern8_H(const int& colOffset, const int& row, const int& colourIx, const unsigned char& pattern)
{
    int stateMask = 1 << colourIx;

    for(int i=0; i<8; ++i) {
        int col = colOffset + i;
        int state = getButtonState(col, row) & ~stateMask;
        if( pattern & (1 << i) )
            state |= stateMask;
        setButtonState(col, row, state);
    }
}

void BlmClass::setLedPattern8_V(const int& col, const int& rowOffset, const int& colourIx, const unsigned char& pattern)
{
    int stateMask = 1 << colourIx;

    for(int i=0; i<8; ++i) {
        int row = rowOffset + i;
        int state = getButtonState(col, row) & ~stateMask;
        if( pattern & (1 << i) )
            state |= stateMask;
        setButtonState(col, row, state);
    }
}


//==============================================================================
void BlmClass::handleIncomingMidiMessage(MidiInput *source, const MidiMessage &message)
{
#ifdef JUCE_LINUX
    // TK: not understood why the queue doesn't work reliable under Linux...
    // Under MacOS the queue is mantadory, otherwise GUI can hang-up if too many MIDI events are received
    if (message.getRawData()[0]<0xf8)
        BLMIncomingMidiMessage(message, runningStatus);
#else
	midiQueue.push(message);
#endif
}



void BlmClass::BLMIncomingMidiMessage(const MidiMessage &message, uint8 RunningStatus)
{
	uint8 *data = (uint8 *)message.getRawData();
	uint8 event_type = data[0] >> 4;
	uint8 chn = data[0] & 0xf;
	uint8 evnt1 = data[1];
	uint8 evnt2 = data[2];

#if 0
	fprintf(stderr,"Got Event... Chn: 0x%02x, event_type: 0x%02x, evnt1: 0x%02x, evnt2: 0x%02x\n",chn,event_type,evnt1,evnt2);
    fflush(stderr);
#endif


	switch( event_type ) {
    case 0x8: // Note Off
        evnt2 = 0; // handle like Note On with velocity 0
    case 0x9: {
        int ledState;
        if( evnt2 == 0 )
            ledState = 0;
        else if( evnt2 < 0x40 )
            ledState = 1;
        else if( evnt2 < 0x60 )
            ledState = 2;
        else
            ledState = 3;

        if( evnt1 < blmColumns ) {
            int row = chn;
            int column = evnt1;
            setButtonState(column, row, ledState);
        } else if( evnt1 == 0x40 ) {
            setButtonState(MAX_COLS_EXTRA_OFFSET, chn, ledState);
        } else if( chn == 0 && evnt1 >= 0x60 && evnt1 <= 0x6f ) {
            setButtonState(evnt1-0x60, MAX_ROWS_EXTRA_OFFSET, ledState);
        } else if( chn == 15 && evnt1 == 0x60 ) {
            setButtonState(MAX_COLS_EXTRA_OFFSET, MAX_ROWS_EXTRA_OFFSET, ledState);
        }

        midiDataReceived = true;
    } break;

    case 0xb: {
        unsigned char pattern = evnt2;
        if( evnt1 & 0x01 )
            pattern |= (1 << 7);

        switch( evnt1 & 0xfe ) {
        // 16x16 green
        case 0x10: setLedPattern8_H(0, chn, 0, pattern); break;
        case 0x12: setLedPattern8_H(8, chn, 0, pattern); break;

        // 16x16 green rotated
        case 0x18: setLedPattern8_V(chn, 0, 0, pattern); break;
        case 0x1a: setLedPattern8_V(chn, 8, 0, pattern); break;

        // 16x16 red
        case 0x20: setLedPattern8_H(0, chn, 1, pattern); break;
        case 0x22: setLedPattern8_H(8, chn, 1, pattern); break;

        // 16x16 red rotated
        case 0x28: setLedPattern8_V(chn, 0, 1, pattern); break;
        case 0x2a: setLedPattern8_V(chn, 8, 1, pattern); break;

        // extra column green
        case 0x40: if( chn == 0 ) setLedPattern8_V(MAX_COLS_EXTRA_OFFSET, 0, 0, pattern); break;
        case 0x42: if( chn == 0 ) setLedPattern8_V(MAX_COLS_EXTRA_OFFSET, 8, 0, pattern); break;

        // extra column red
        case 0x48: if( chn == 0 ) setLedPattern8_V(MAX_COLS_EXTRA_OFFSET, 0, 1, pattern); break;
        case 0x4a: if( chn == 0 ) setLedPattern8_V(MAX_COLS_EXTRA_OFFSET, 8, 1, pattern); break;

        // extra shift column green
        case 0x50: if( chn == 0 ) setLedPattern8_V(MAX_COLS_EXTRA_OFFSET+1, 0, 0, pattern); break;
        case 0x52: if( chn == 0 ) setLedPattern8_V(MAX_COLS_EXTRA_OFFSET+1, 8, 0, pattern); break;

        // extra shift column red
        case 0x58: if( chn == 0 ) setLedPattern8_V(MAX_COLS_EXTRA_OFFSET+1, 0, 1, pattern); break;
        case 0x5a: if( chn == 0 ) setLedPattern8_V(MAX_COLS_EXTRA_OFFSET+1, 8, 1, pattern); break;

        // extra row green & shift
        case 0x60:
            if( chn == 0 ) setLedPattern8_H(0, MAX_ROWS_EXTRA_OFFSET, 0, pattern);
            else if( chn == 15 ) setLed(MAX_COLS_EXTRA_OFFSET, MAX_ROWS_EXTRA_OFFSET, 0, pattern & 1);
            break;
        case 0x62: if( chn == 0 ) setLedPattern8_H(8, MAX_ROWS_EXTRA_OFFSET, 0, pattern); break;

        // extra row red & shift
        case 0x68:
            if( chn == 0 ) setLedPattern8_H(0, MAX_ROWS_EXTRA_OFFSET, 1, pattern);
            else if( chn == 15 ) setLed(MAX_COLS_EXTRA_OFFSET, MAX_ROWS_EXTRA_OFFSET, 1, pattern & 1);
            break;
        case 0x6a: if( chn == 0 ) setLedPattern8_H(8, MAX_ROWS_EXTRA_OFFSET, 1, pattern); break;
        }

        midiDataReceived = true;
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
            } else if( data[6] == 0x0f && data[7] == 0xf7 ) {
                sendAck();
            }
        }
    } break;
    }
}


void BlmClass::setBLMLayout(const String& layout)
{
    if( layout == "16x8+X" ) {
        blmColumns=16;
        blmRows=8;
    } else {
        // 16x16+X
        blmColumns=16;
        blmRows=16;
    }

    // set global layout variables
#if JUCE_IOS
    rowLabelsWidth = 35;
#else
    rowLabelsWidth = 100;
#endif

    const char *row8Labels[9] = {
        "Start", "Stop", "", "", "Keyboard", "Patterns", "Tracks", "Grid",
        "Shift"
    };

    const char *row16Labels[17] = {
        "Start", "Stop", "", "", "", "", "", "",
        "", "", "", "303", "Keyboard", "Patterns", "Tracks", "Grid",
        "Shift"
    };

    const char **rowLabels = (blmRows <= 8) ? row8Labels : row16Labels;

    for(int i=0; i<=blmRows; ++i) {
        rowLabelsRed[i]->setText(rowLabels[i], false);
        if( i < blmRows ) {
            rowLabelsGreen[i]->setText(String(i+1), false);
        } else {
            rowLabelsGreen[i]->setText("", false);
        }
    }

    buttonArray16x16X = rowLabelsWidth + 2*1.25 * ledSize;
    buttonArray16x16Y = 0;
    buttonArray16x16Width = blmColumns * ledSize;
    buttonArray16x16Height = blmRows * ledSize;

    buttonExtraShiftColumnX = rowLabelsWidth + 0*1.25 * ledSize;
    buttonExtraShiftColumnY = 0;
    buttonExtraShiftColumnWidth = 1 * ledSize;
    buttonExtraShiftColumnHeight = buttonArray16x16Height;

    buttonExtraColumnX = rowLabelsWidth + 1*1.25 * ledSize;
    buttonExtraColumnY = 0;
    buttonExtraColumnWidth = 1 * ledSize;
    buttonExtraColumnHeight = buttonArray16x16Height;

    buttonExtraRowX = rowLabelsWidth + 2*1.25 * ledSize;
    buttonExtraRowY = buttonArray16x16Y + buttonArray16x16Height + 0.25 * ledSize;
    buttonExtraRowWidth = buttonArray16x16Width;
    buttonExtraRowHeight = 1 * ledSize;

    buttonShiftX = buttonExtraColumnX ;
    buttonShiftY = buttonExtraColumnY + buttonExtraColumnHeight + 0.25 * ledSize;
    buttonShiftWidth = 1 * ledSize;
    buttonShiftHeight = 1 * ledSize;

    setSize(blmColumns*ledSize, blmRows*ledSize);
}

void BlmClass::sendBLMLayout(void)
{
	unsigned char sysex[14];
	sysex[0] = 0xf0;
	sysex[1] = 0x00;
	sysex[2] = 0x00;
	sysex[3] = 0x7e;
	sysex[4] = 0x4e; // MBHP_BLM_SCALAR ID
	sysex[5] = 0x00; // Device ID 00
	sysex[6] = 0x01; // Command #1 (Layout Info)
	sysex[7] = blmColumns & 0x7f; // number of columns
	sysex[8] = blmRows & 0x7f; // number of rows
	sysex[9] = ledColours & 0x7f; // number of LED colours
	sysex[10] = 1; // number of extra rows
	sysex[11] = 1; // number of extra columns
	sysex[12] = 1; // number of extra buttons (e.g. shift)
	sysex[13] = 0xf7;
	MidiMessage message(sysex,14);
    mainComponent->sendMidiMessage(message);
}

void BlmClass::sendAck(void)
{
	unsigned char sysex[9];
	sysex[0] = 0xf0;
	sysex[1] = 0x00;
	sysex[2] = 0x00;
	sysex[3] = 0x7e;
	sysex[4] = 0x4e; // MBHP_BLM_SCALAR ID
	sysex[5] = 0x00; // Device ID 00
	sysex[6] = 0x0f; // Acknowledge
	sysex[7] = 0x00; // dummy
	sysex[8] = 0xf7;
	MidiMessage message(sysex, 9);
    mainComponent->sendMidiMessage(message);
}


void BlmClass::sendCCEvent(int chn, int cc, int value)
{
	MidiMessage message(0xb0|chn, cc, value);
    mainComponent->sendMidiMessage(message);
}


void BlmClass::sendNoteEvent(int chn, int key, int velocity)
{
	MidiMessage message(0x90|chn, key, velocity);
    mainComponent->sendMidiMessage(message);
}

//==============================================================================
#if JUCE_MAJOR_VERSION==1 && JUCE_MINOR_VERSION<51
void BlmClass::timerCallback(const int timerId)
#else
void BlmClass::timerCallback(int timerId)
#endif
{
    if( timerId == TIMER_CHECK_MIDI ) {
        // parse incoming MIDI events
        while( !midiQueue.empty() ) {
            MidiMessage &message = midiQueue.front();
            midiQueue.pop();

            // propagate incoming event to MIDI components
            if (message.getRawData()[0]<0xf8)
                BLMIncomingMidiMessage(message, runningStatus);
        }
    } else if( timerId == TIMER_SEND_PING ) {
        if( midiDataReceived )
            sendAck();
        else
            sendBLMLayout();

        midiDataReceived = false;
    }
}

//==============================================================================
void BlmClass::mouseDown(const MouseEvent &e)
{
	// Get rid of spurious messages
	// I think that setToggleState() can sometimes trigger a mouseDown()???
	if (!e.mouseWasClicked())
		return;

    int x, y, midiChannel, midiNote;
    if( !searchButtonIndex(e.x, e.y, x, y, midiChannel, midiNote) )
        return;
    
#if 0
	fprintf(stderr,"In MouseDown: [%d,%d]\n",x,y);
	fflush(stderr);
#endif
	if (x!=lastButtonX && y!=lastButtonY)
	{
		buttons[x][y]->triggerClick();
        lastMidiChannel = midiChannel;
        lastMidiNote = midiNote & 0x7f;
		lastButtonX=x;
		lastButtonY=y;
		sendNoteEvent(midiChannel, midiNote & 0x7f, 0x7f);
	}
}
void BlmClass::mouseUp(const MouseEvent &e)
{
    int x, y, midiChannel, midiNote;

    if( !searchButtonIndex(e.x, e.y, x, y, midiChannel, midiNote) )
        return;

#if 0
	fprintf(stderr,"In MouseUp: [%d,%d]\n",x,y);
	fflush(stderr);
#endif
    if( lastMidiChannel >= 0 ) {
        sendNoteEvent(lastMidiChannel, lastMidiNote, 0x00);
        lastMidiChannel = -1;
        lastMidiNote = -1;
		lastButtonX=-1;
		lastButtonY=-1;
	}
	//selected.addToSelectionOnMouseUp(buttons[x][y],e.mods,true,true);
}


void BlmClass::mouseDrag(const MouseEvent &e)
{
    int x, y, midiChannel, midiNote;

    if( e.x < 0 || e.y < 0 || !searchButtonIndex(e.x, e.y, x, y, midiChannel, midiNote) ) {
        // Dragged outside boundary so send off event and clear button.
		if( lastMidiChannel >= 0 )
			sendNoteEvent(lastMidiChannel, lastMidiNote, 0x00);
		 
        lastMidiChannel = -1;
        lastMidiNote = -1;
		lastButtonX = -1;
		lastButtonY = -1;
		return;
    }


#if 0
	fprintf(stderr,"In MouseDrag: [%d,%d]\n",x,y);
	fflush(stderr);
#endif

	if( (lastButtonX != x || lastButtonY !=y ) ) {
		buttons[x][y]->triggerClick();
		if( lastMidiChannel >= 0 )
			sendNoteEvent(lastMidiChannel, lastMidiNote, 0x00);

		sendNoteEvent(midiChannel, midiNote, 0x7f);
        lastMidiChannel = midiChannel;
        lastMidiNote = midiNote;
		lastButtonX = x;
		lastButtonY = y;
	} 
}


//==============================================================================
bool BlmClass::keyPressed(const KeyPress& key, Component *originatingComponent)
{
    if( key.isKeyCode(KeyPress::spaceKey) ) {
        return true;
    }

    return false;
}

bool BlmClass::keyStateChanged(const bool isKeyDown, Component *originatingComponent)
{
    // unfortunately Juce doesn't trigger this function on shift button changes
    // therefore we take the space button instead
    bool newShiftState = KeyPress::isKeyCurrentlyDown(KeyPress::spaceKey);

    if( !prevShiftState && newShiftState ) {
		sendNoteEvent(0xf, 0x60, 0x7f);
    } else if( prevShiftState && !newShiftState ) {
		sendNoteEvent(0xf, 0x60, 0x00);
    }

    prevShiftState = newShiftState;


    return false;
}

