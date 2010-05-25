/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
// $Id: UploadHandler.cpp 928 2010-02-20 23:38:06Z tk $
// BLM.cpp
//
//
// Created by Phil Taylor 2010

#include "includes.h"
//#include "BlmClass.h"
#include "MainComponent.h"
BlmClass::BlmClass(int cols,int rows)
    : ledSize(32)
    , ledColours(2)
    , sysexRequestTimer(0)
    , midiDataReceived(false)
    , lastButtonX(-1)
    , lastButtonY(-1)
    , lastMidiChannel(-1)
    , lastMidiNote(-1)
    , prevShiftState(false)
{
	for(int x=0;x<MAX_COLS;x++){
		for(int y=0;y<MAX_ROWS;y++){
			addAndMakeVisible (buttons[x][y] = new TextButton (String::empty));
			buttons[x][y]->setColour (TextButton::buttonColourId, Colours::azure);
			buttons[x][y]->setColour (TextButton::buttonOnColourId, Colours::blue);
			//buttons[x][y]->addButtonListener(this);
			buttons[x][y]->setInterceptsMouseClicks(false, false);
		}
	}

    for(int y=0; y<MAX_ROWS; y++) {
        addAndMakeVisible(rowLabelsGreen[y] = new Label(T(""), String(y+1)));
        rowLabelsGreen[y]->setColour(Label::textColourId, Colours::green);
        rowLabelsGreen[y]->setJustificationType(Justification::centredRight);

        addAndMakeVisible(rowLabelsRed[y] = new Label(T(""), T("")));
        rowLabelsRed[y]->setColour(Label::textColourId, Colours::red);
        rowLabelsRed[y]->setJustificationType(Justification::centredRight);

        switch( y ) {
        case  0: rowLabelsRed[y]->setText(T("Start"), false); break;
        case  1: rowLabelsRed[y]->setText(T("Stop"), false); break;
        case 12: rowLabelsRed[y]->setText(T("Keyboard"), false); break;
        case 13: rowLabelsRed[y]->setText(T("Patterns"), false); break;
        case 14: rowLabelsRed[y]->setText(T("Tracks"), false); break;
        case 15: rowLabelsRed[y]->setText(T("Grid"), false); break;
        case 16: rowLabelsRed[y]->setText(T("Shift"), false); rowLabelsGreen[y]->setText(String::empty, false); break;
        }
    }

	audioDeviceManager.addMidiInputCallback(String::empty, this);

    Timer::startTimer(1);
	addMouseListener(this,true); // Add mouse listener for all child components
	
    // listen on key changes
    this->addKeyListener(this);

    // define final dimensions and set size
	setBlmDimensions(cols, rows); 
}


BlmClass::~BlmClass()
{
    Timer::stopTimer();

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


//==============================================================================
void BlmClass::setBlmDimensions(int col,int row)
{
    blmColumns=col;
    blmRows=row;

    // set global layout variables
    rowLabelsWidth = 100;

    buttonArray16x16X = rowLabelsWidth + 1.25 * ledSize;
    buttonArray16x16Y = 0;
    buttonArray16x16Width = blmColumns * ledSize;
    buttonArray16x16Height = blmRows * ledSize;

    buttonExtraColumnX = rowLabelsWidth;
    buttonExtraColumnY = 0;
    buttonExtraColumnWidth = 1 * ledSize;
    buttonExtraColumnHeight = buttonArray16x16Height;

    buttonExtraRowX = rowLabelsWidth + 1.25 * ledSize;
    buttonExtraRowY = buttonArray16x16Y + buttonArray16x16Height + 0.25 * ledSize;
    buttonExtraRowWidth = buttonArray16x16Width;
    buttonExtraRowHeight = 1 * ledSize;

    buttonShiftX = buttonExtraColumnX;
    buttonShiftY = buttonExtraColumnY + buttonExtraColumnHeight + 0.25 * ledSize;
    buttonShiftWidth = 1 * ledSize;
    buttonShiftHeight = 1 * ledSize;

    setSize(blmColumns*ledSize, blmRows*ledSize);
}


void BlmClass::resized()
{
    for(int y=0; y<MAX_ROWS; y++) {
        int yOffset = buttonExtraColumnY + y*ledSize;
        if( y == blmRows )
            yOffset = buttonShiftY;
        int greenWidth = 25;
        rowLabelsGreen[y]->setBounds(rowLabelsWidth-greenWidth, yOffset, greenWidth, ledSize);
        rowLabelsRed[y]->setBounds(0, yOffset, rowLabelsWidth-greenWidth, ledSize);
    }

	for(int x=0; x<MAX_COLS; x++) {
		for(int y=0; y<MAX_ROWS; y++) {

            int xOffset = -1;
            int yOffset = -1;

            if( x < blmColumns && y < blmRows ) {
                // 16x16 matrix
                xOffset = buttonArray16x16X + x*ledSize + 2;
                yOffset = buttonArray16x16Y + y*ledSize + 2;
            } else if( x == blmRows && y == blmColumns ) {
                // shift button
                xOffset = buttonShiftX;
                yOffset = buttonShiftY;
            } else if( x == blmRows ) {
                // extra column
                xOffset = buttonExtraColumnX;
                yOffset = buttonExtraColumnY + y*ledSize + 2;
            } else if( y == blmRows ) {
                // extra row
                xOffset = buttonExtraRowX + x*ledSize + 2;
                yOffset = buttonExtraRowY + 2;
            }

            if( xOffset >= 0 && yOffset >= 0 ) {
                buttons[x][y]->setVisible(true);
                buttons[x][y]->setSize(ledSize-4, ledSize-4);
                buttons[x][y]->setTopLeftPosition(xOffset, yOffset);
                buttons[x][y]->setToggleState(false,false);
                buttons[x][y]->setColour(TextButton::buttonOnColourId, Colours::blue);
            } else {
				buttons[x][y]->setVisible(false);
			}
        }
    }

    // send layout to MBSEQ
	sendBLMLayout();

    // set frame size of this component
    setSize(buttonArray16x16X + buttonArray16x16Width, buttonExtraRowY + buttonExtraRowHeight);
}


bool BlmClass::searchButtonIndex(const int& x, const int& y, int& buttonX, int& buttonY, int& midiChannel, int& midiNote)
{
    if( x >= buttonArray16x16X && x < (buttonArray16x16X+buttonArray16x16Width) &&
        y >= buttonArray16x16Y && y < (buttonArray16x16Y+buttonArray16x16Height) ) {
        buttonX = (x-buttonArray16x16X) / ledSize;
        buttonY = (y-buttonArray16x16Y) / ledSize;
        midiNote = buttonX;
        midiChannel = buttonY;
        return true;
    }

    if( x >= buttonExtraColumnX && x < (buttonExtraColumnX+buttonExtraColumnWidth) &&
        y >= buttonExtraColumnY && y < (buttonExtraColumnY+buttonExtraColumnHeight) ) {
        buttonX = blmColumns + ((x-buttonExtraColumnX) / ledSize);
        buttonY = (y-buttonExtraColumnY) / ledSize;
        midiNote = 0x40;
        midiChannel = buttonY;
        return true;
    }

    if( x >= buttonExtraRowX && x < (buttonExtraRowX+buttonExtraRowWidth) &&
        y >= buttonExtraRowY && y < (buttonExtraRowY+buttonExtraRowHeight) ) {
        buttonX = (x-buttonExtraRowX) / ledSize;
        buttonY = blmRows + ((y-buttonExtraRowY) / ledSize);
        midiNote = 0x60 + buttonX;
        midiChannel = 0x0;
        return true;
    }

    if( x >= buttonShiftX && x < (buttonShiftX+buttonShiftWidth) &&
        y >= buttonShiftY && y < (buttonShiftY+buttonShiftHeight) ) {
        buttonX = blmColumns + (x-buttonShiftX) / ledSize;
        buttonY = blmRows + ((y-buttonShiftY) / ledSize);
        midiNote = 0x60;
        midiChannel = 0xf;
        return true;
    }

    return false;
}

//==============================================================================
void BlmClass::setMidiInput(const String &port)
{
    const StringArray allMidiIns(MidiInput::getDevices());
    for (int i = allMidiIns.size(); --i >= 0;) {
        bool enabled = allMidiIns[i] == port;
        audioDeviceManager.setMidiInputEnabled(allMidiIns[i], enabled);
    }
}

void BlmClass::setMidiOutput(const String &port)
{
    audioDeviceManager.setDefaultMidiOutput(port);
}


//==============================================================================
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

    if( !state )
        buttons[col][row]->setToggleState(false, false);
    else
        buttons[col][row]->setToggleState(true, false);
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
	midiQueue.push(message);
}



void BlmClass::BLMIncomingMidiMessage(const MidiMessage &message, uint8 RunningStatus)
{
	uint8 *data=message.getRawData();
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
        int LED_State;
        if( evnt2 == 0 )
            LED_State = 0;
        else if( evnt2 < 0x40 )
            LED_State = 1;
        else if( evnt2 < 0x60 )
            LED_State = 2;
        else
            LED_State = 3;

        if( evnt1 < blmColumns) {
            int row = chn;
            int column = evnt1;
            setButtonState(column,row,LED_State);
        } else if( evnt1 == 0x40 ) {
            setButtonState(blmColumns, chn, LED_State);
        } else if( chn == 0 && evnt1 >= 0x60 && evnt1 <= 0x6f ) {
            setButtonState(evnt1-0x60, blmRows, LED_State);
        } else if( chn == 15 && evnt1 == 0x60 ) {
            setButtonState(blmColumns, blmRows, LED_State);
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
        case 0x40: if( chn == 0 ) setLedPattern8_V(blmColumns, 0, 0, pattern); break;
        case 0x42: if( chn == 0 ) setLedPattern8_V(blmColumns, 8, 0, pattern); break;

        // extra column red
        case 0x48: if( chn == 0 ) setLedPattern8_V(blmColumns, 0, 1, pattern); break;
        case 0x4a: if( chn == 0 ) setLedPattern8_V(blmColumns, 8, 1, pattern); break;

        // extra row green & shift
        case 0x60:
            if( chn == 0 ) setLedPattern8_H(0, blmRows, 0, pattern);
            else if( chn == 15 ) setLed(blmColumns, blmRows, 0, pattern & 1);
            break;
        case 0x62: if( chn == 0 ) setLedPattern8_H(8, blmRows, 0, pattern); break;

        // extra row red & shift
        case 0x68:
            if( chn == 0 ) setLedPattern8_H(0, blmRows, 1, pattern);
            else if( chn == 15 ) setLed(blmColumns, blmRows, 1, pattern & 1);
            break;
        case 0x6a: if( chn == 0 ) setLedPattern8_H(8, blmRows, 1, pattern); break;
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
    MidiOutput *out = audioDeviceManager.getDefaultMidiOutput();
    if( out )
        out->sendMessageNow(message);
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
    MidiOutput *out = audioDeviceManager.getDefaultMidiOutput();
    if( out )
        out->sendMessageNow(message);
}


void BlmClass::sendCCEvent(int chn,int cc, int value)
{
	MidiMessage message(0xb0|chn,cc,value);
    MidiOutput *out = audioDeviceManager.getDefaultMidiOutput();
    if( out )
        out->sendMessageNow(message);
}


void BlmClass::sendNoteEvent(int chn,int key, int velocity)
{
	MidiMessage message(0x90|chn,key,velocity);
    MidiOutput *out = audioDeviceManager.getDefaultMidiOutput();
    if( out )
        out->sendMessageNow(message);
}

//==============================================================================
void BlmClass::timerCallback()
{
    // parse incoming MIDI events
    while( !midiQueue.empty() ) {
        MidiMessage &message = midiQueue.front();
        midiQueue.pop();

        // propagate incoming event to MIDI components
		if (message.getRawData()[0]<0xf8)
	        BLMIncomingMidiMessage(message, runningStatus);
    }

    // send layout/acknowledge each 5 seconds
    if( ++sysexRequestTimer >= 4000 ) { // TK: timer not accurate, therefore changed to 4 seconds
        sysexRequestTimer = 0;

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
        lastMidiNote = midiNote;
		lastButtonX=x;
		lastButtonY=y;
		sendNoteEvent(midiChannel, midiNote, 0x7f);
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
	if (lastButtonX>-1){
		sendNoteEvent(midiChannel, midiNote, 0x00);
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

