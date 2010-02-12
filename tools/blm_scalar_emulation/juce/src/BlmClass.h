//
// BLM.h - blm_scalar_emulation 
//
// Created by Phil Taylor 2010
//
#ifndef _BLM_H

#include <queue>

#define MAX_ROWS 16
#define MAX_COLS 128

class BlmClass : public Component,
				 public MidiInputCallback,
				 public Timer
{
public:
	BlmClass(int cols, int rows);
	~BlmClass();

	void resized();

    void timerCallback();

	int getBlmColumns(void) { return blmColumns; }	
	int getBlmRows(void) { return blmRows; }

	void setBlmDimensions(int col,int row) {blmColumns=col; blmRows=row;}	

	void setMidiOutput(int port);
	void setMidiInput(int port);
	int getMidiInput(void) {return inputPort;}
	int getMidiOutput(void) {return outputPort;}
	void handleIncomingMidiMessage(MidiInput* source, const MidiMessage& message);
	void BLMIncomingMidiMessage(const MidiMessage &message, uint8 RunningStatus);
	void closeMidiPorts(void);

	void setButtonState(int col, int row, int state);
	int getButtonState(int col, int row);

	void sendBLMLayout(void);
	void sendCCEvent(int chn,int cc, int value);
	void sendNoteEvent(int chn,int key, int velocity);

	void mouseDown(const MouseEvent &e);
	void mouseUp(const MouseEvent &e);
	void mouseDrag(const MouseEvent &e);


protected:
	// TK: the Juce specific "MidiBuffer" sporatically throws an assertion when overloaded
    // therefore I'm using a std::queue instead
    std::queue<MidiMessage> midiQueue;
    uint8 runningStatus;

private:
	int blmColumns,blmRows,ledColours,inputPort,outputPort;
	int lastButtonX,lastButtonY;
	int ledSize;
	TextButton  *buttons[MAX_COLS][MAX_ROWS]; // Not ideal but saves dynamic memory alloc later.
	MidiOutput *midiOutput;
	MidiInput *midiInput;
    SelectedItemSet <Component*> selected;
	
};

#endif /* _BLM_H */