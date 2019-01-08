/*
	FreeRTOS Emu
*/


#include <mios32.h>

#include <JUCEmidi.h>



s32 MIOS32_MIDI_Init(u32 mode)
{
	return JUCE_MIDI_Init(mode);
}



s32 MIOS32_MIDI_DirectRxCallback_Init(void *callback_rx)
{
	return JUCE_MIDI_DirectRxCallback_Init(callback_rx);
}



s32 MIOS32_MIDI_SendPackage(mios32_midi_port_t port, mios32_midi_package_t package)
{
	switch (package.event) {
		case NoteOff:
			return JUCE_MIDI_SendNoteOff(package.cable, package.chn, package.note, package.velocity);
		break;

		case NoteOn:
			return JUCE_MIDI_SendNoteOn(package.cable, package.chn, package.note, package.velocity);
		break;

		case PolyPressure:
			return JUCE_MIDI_SendPolyPressure(package.cable, package.chn, package.note, package.value);
		break;

		case CC:
			return JUCE_MIDI_SendCC(package.cable, package.chn, package.cc_number, package.value);
		break;

		case ProgramChange:

		break;

		case Aftertouch:

		break;

		case PitchBend:

		break;

		default:
			return JUCE_MIDI_SendPackage((int) port, (long) package.ALL);
		break;
	}

	return 1;
}

s32 MIOS32_MIDI_SendNoteOff(mios32_midi_port_t port, mios32_midi_chn_t chn, u8 note, u8 vel)
{
	return JUCE_MIDI_SendNoteOff((int) port, (char) chn, (char) note, (char) vel);
}

s32 MIOS32_MIDI_SendNoteOn(mios32_midi_port_t port, mios32_midi_chn_t chn, u8 note, u8 vel)
{
	return JUCE_MIDI_SendNoteOn((int) port, (char) chn, (char) note, (char) vel);
}

s32 MIOS32_MIDI_SendPolyPressure(mios32_midi_port_t port, mios32_midi_chn_t chn, u8 note, u8 val)
{
	return JUCE_MIDI_SendPolyPressure((int) port, (char) chn, (char) note, (char) val);
}

s32 MIOS32_MIDI_SendCC(mios32_midi_port_t port, mios32_midi_chn_t chn, u8 note, u8 val)
{
	return JUCE_MIDI_SendCC((int) port, (char) chn, (char) note, (char) val);
}

s32 MIOS32_MIDI_SendProgramChange(mios32_midi_port_t port, mios32_midi_chn_t chn, u8 prg)
{
	return JUCE_MIDI_SendProgramChange((int) port, (char) chn, (char) prg);
}

s32 MIOS32_MIDI_SendAftertouch(mios32_midi_port_t port, mios32_midi_chn_t chn, u8 val)
{
	return JUCE_MIDI_SendAftertouch((int) port, (char) chn, (char) val);
}

s32 MIOS32_MIDI_SendPitchBend(mios32_midi_port_t port, mios32_midi_chn_t chn, u16 val)
{
	return JUCE_MIDI_SendPitchBend((int) port, (char) chn, (int) val);
}

s32 MIOS32_MIDI_SendSysEx(mios32_midi_port_t port, u8 *stream, u32 count)
{
	return 0; //todo
}

s32 MIOS32_MIDI_SendMTC(mios32_midi_port_t port, u8 val)
{
	return 0; //todo
}

s32 MIOS32_MIDI_SendSongPosition(mios32_midi_port_t port, u16 val)
{
	return 0; //todo
}

s32 MIOS32_MIDI_SendSongSelect(mios32_midi_port_t port, u8 val)
{
	return 0; //todo
}

s32 MIOS32_MIDI_SendTuneRequest(mios32_midi_port_t port)
{
	return 0; //todo
}

s32 MIOS32_MIDI_SendClock(mios32_midi_port_t port) {
	return JUCE_MIDI_SendClock((int) port);
}


s32 MIOS32_MIDI_SendTick(mios32_midi_port_t port)
{
	return 0; //todo
}

s32 MIOS32_MIDI_SendStart(mios32_midi_port_t port) {
	return JUCE_MIDI_SendStart((int) port);
}

s32 MIOS32_MIDI_SendContinue(mios32_midi_port_t port) {
	return JUCE_MIDI_SendContinue((int) port);
}

s32 MIOS32_MIDI_SendStop(mios32_midi_port_t port) {
	return JUCE_MIDI_SendStop((int) port);
}

s32 MIOS32_MIDI_SendActiveSense(mios32_midi_port_t port)
{
	return 0; //todo
}

s32 MIOS32_MIDI_SendReset(mios32_midi_port_t port)
{
	return 0; //todo
}

