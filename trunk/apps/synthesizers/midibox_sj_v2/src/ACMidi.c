/*
 *  ACMidi.c
 *  m5_sensorizer
 *
 */



#include "ACMidi.h"

#include <mios32.h>
#include "app.h"





/////////////////////////////////////////////////////////////////////////////
//	send Complete 3-byte Event
/////////////////////////////////////////////////////////////////////////////

// send Midi Event
void ACMidi_SendEvent(unsigned char evnt0, unsigned char evnt1, unsigned char evnt2) {
  MIOS32_MIDI_SendEvent(DEFAULT, evnt0, evnt1, evnt2);
}


/////////////////////////////////////////////////////////////////////////////
//	send Note Message on channel
/////////////////////////////////////////////////////////////////////////////

// send Note Message
void ACMidi_SendNote(unsigned char channel, unsigned char note, unsigned char velo) {
  MIOS32_MIDI_SendNoteOn(DEFAULT, channel, note, velo);
}

/////////////////////////////////////////////////////////////////////////////
//	send CC Message on channel
/////////////////////////////////////////////////////////////////////////////

void ACMidi_SendCC(unsigned char channel, unsigned char cc, unsigned char sevenBitValue)
{
  // send CC
  if(sevenBitValue < 0x80) {	// only send values < 128
    MIOS32_MIDI_SendCC(DEFAULT, channel, cc, sevenBitValue);
  }
}



/////////////////////////////////////////////////////////////////////////////
//	send Program Change message along with current bank
/////////////////////////////////////////////////////////////////////////////

void ACMidi_SendPRG_CH(unsigned char bank, unsigned char program) 
{
	// send bankSelect
	MIOS32_MIDI_SendCC(DEFAULT, MIDI_GLOBAL_CH, MIDI_CC_BANK_SELECT, bank);
	// send PRG-CH
	MIOS32_MIDI_SendProgramChange(DEFAULT, MIDI_GLOBAL_CH, program);
}



/////////////////////////////////////////////////////////////////////////////
//	send PANIC! If channel > 15 send panic on all channels (self recursive)
/////////////////////////////////////////////////////////////////////////////

void ACMidi_SendPanic(unsigned char channel)
{
	unsigned char c;
	if(channel > 15) {
		// send panic on all channels
		for(c=0; c<16; c++) {
			ACMidi_SendPanic(c);
		}
	} else {
		// send panic
	        MIOS32_MIDI_SendCC(DEFAULT, channel, MIDI_CC_ALL_NOTES_OFF, 0x00);
	}
}






