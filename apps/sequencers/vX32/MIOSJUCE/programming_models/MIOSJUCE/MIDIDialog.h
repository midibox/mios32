/*
    MIOS32 Emu
*/

#ifndef MIDIDIALOG_H
#define MIDIDIALOG_H



#include <mios32.h>


class MIDIDialog : public DialogWindow
{
public:
	void closeButtonPressed()
    {
        MIOS32_MIDI_Init(0);
    }
	
};


#endif   // MIDIDIALOG_H
