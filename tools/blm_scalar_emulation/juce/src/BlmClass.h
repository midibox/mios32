//
// BLM.h - blm_scalar_emulation 
//
// Created by Phil Taylor 2010
//
#ifndef _BLM_H

#define MAX_ROWS 16
#define MAX_COLS 128

class BlmClass : public Component,
                 public ButtonListener,
				 public MidiInputCallback
{
public:
	BlmClass(int cols, int rows);
	~BlmClass();

	int getBlmColumns(void) { return blmColumns; }	
	int getBlmRows(void) { return blmRows; }
	void resized();
    void buttonClicked (Button* buttonThatWasClicked);
    void buttonStateChanged (Button* buttonThatWasClicked);
	void setMidiOutput(int port);
	void setMidiInput(int port);
	int getMidiInput(void) {return inputPort;}
	int getMidiOutput(void) {return outputPort;}
	void setBlmDimensions(int col,int row) {blmColumns=col; blmRows=row;}	
	void sendBLMLayout(void);
	void handleIncomingMidiMessage(MidiInput* source, const MidiMessage& message);
	void closeMidiPorts(void);
	void setButtonState(int col, int row, int state);
	int getButtonState(int col, int row);
	void sendCCEvent(int chn,int cc, int value);
	void sendNoteEvent(int chn,int key, int velocity);

private:
	int blmColumns,blmRows,ledColours,inputPort,outputPort;
	TextButton  *buttons[MAX_COLS][MAX_ROWS]; // Not ideal but saves dynamic memory alloc later.
	MidiOutput *midiOutput;
	MidiInput *midiInput;
	
};

#endif /* _BLM_H */