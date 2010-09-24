/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
// $Id: UploadHandler.cpp 928 2010-02-20 23:38:06Z tk $
//
// BLM.h - blm_scalar_emulation 
//
// Created by Phil Taylor 2010
//
#ifndef _BLM_CLASS_H

#include <queue>

// 16x16 + extra row/column
#define MAX_ROWS (16+1)
#define MAX_COLS (16+1)

// forward declaration
class MainComponent;

class BlmClass : public Component,
				 public MidiInputCallback,
				 public MultiTimer,
				 public KeyListener
{
public:
	BlmClass(MainComponent *_mainComponent, int cols, int rows);
	~BlmClass();


    // reference to main component (to send MIDI)
    MainComponent *mainComponent;

#if JUCE_MAJOR_VERSION==1 && JUCE_MINOR_VERSION<51
    void timerCallback(const int timerId);
#else
    void timerCallback(int timerId);
#endif

	int getBlmColumns(void) { return blmColumns; }	
	int getBlmRows(void) { return blmRows; }
	int getLedSize(void) { return ledSize; }

	void setBlmDimensions(int col,int row);
	void resized();
    bool searchButtonIndex(const int& x, const int& y, int& buttonX, int& buttonY, int& midiChannel, int& midiNote);

	void handleIncomingMidiMessage(MidiInput* source, const MidiMessage& message);
	void BLMIncomingMidiMessage(const MidiMessage &message, uint8 RunningStatus);
	void closeMidiPorts(void);

	void setButtonState(int col, int row, int state);
	int getButtonState(int col, int row);

    void setLed(const int& col, const int& row, const int& colourIx, const int& enabled);
    void setLedPattern8_H(const int& colOffset, const int& row, const int& colourIx, const unsigned char& pattern);
    void setLedPattern8_V(const int& col, const int& rowOffset, const int& colourIx, const unsigned char& pattern);

	void sendBLMLayout(void);
	void sendAck(void);
	void sendCCEvent(int chn,int cc, int value);
	void sendNoteEvent(int chn,int key, int velocity);

	void mouseDown(const MouseEvent &e);
	void mouseUp(const MouseEvent &e);
	void mouseDrag(const MouseEvent &e);

    bool keyPressed(const KeyPress& key, Component *originatingComponent);
#if JUCE_MAJOR_VERSION==1 && JUCE_MINOR_VERSION<51 
	bool keyStateChanged(const bool isKeyDown, Component *originatingComponent);
#else
	virtual bool keyStateChanged(bool isKeyDown, Component *originatingComponent);
#endif

protected:
	// TK: the Juce specific "MidiBuffer" sporatically throws an assertion when overloaded
    // therefore I'm using a std::queue instead
    std::queue<MidiMessage> midiQueue;
    uint8 runningStatus;

private:
	int blmColumns,blmRows,ledColours,inputPort,outputPort;
	int lastButtonX,lastButtonY, lastMidiChannel, lastMidiNote;
	int ledSize;
	TextButton  *buttons[MAX_COLS][MAX_ROWS]; // Not ideal but saves dynamic memory alloc later.
    Label* rowLabelsGreen[MAX_ROWS];
    Label* rowLabelsRed[MAX_ROWS];

    SelectedItemSet <Component*> selected;

    bool midiDataReceived;

    int rowLabelsWidth;

    int buttonArray16x16X;
    int buttonArray16x16Y;
    int buttonArray16x16Width;
    int buttonArray16x16Height;

    int buttonExtraColumnX;
    int buttonExtraColumnY;
    int buttonExtraColumnWidth;
    int buttonExtraColumnHeight;

    int buttonExtraRowX;
    int buttonExtraRowY;
    int buttonExtraRowWidth;
    int buttonExtraRowHeight;

    int buttonShiftX;
    int buttonShiftY;
    int buttonShiftWidth;
    int buttonShiftHeight;

    bool prevShiftState;
};

#endif /* _BLM_CLASS_H */
