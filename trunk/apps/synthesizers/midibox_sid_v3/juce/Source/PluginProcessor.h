/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
/*
  ==============================================================================

    This file was auto-generated!

    It contains the basic startup code for a Juce application.

  ==============================================================================
*/

#ifndef __PLUGINPROCESSOR_H_30AA520E__
#define __PLUGINPROCESSOR_H_30AA520E__

#include <JuceHeader.h>

#include "../resid/resid.h"
#include "MbSidEnvironment.h"
#include "MidiProcessing.h"


// number of emulated SID(s)
// if 0: emulation disabled
#define SID_NUM 2


//==============================================================================
/**
*/
class MidiboxSidAudioProcessor
    : public AudioProcessor
    , public ChangeBroadcaster
{
public:
    //==============================================================================
    MidiboxSidAudioProcessor();
    ~MidiboxSidAudioProcessor();

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock);
    void releaseResources();

    void processBlock (AudioSampleBuffer& buffer, MidiBuffer& midiMessages);

    //==============================================================================
    AudioProcessorEditor* createEditor();
    bool hasEditor() const;

    //==============================================================================
    const String getName() const;

    int getNumParameters();

    float getParameter (int index);
    void setParameter (int index, float newValue);

    const String getParameterName (int index);
    const String getParameterText (int index);

    const String getInputChannelName (int channelIndex) const;
    const String getOutputChannelName (int channelIndex) const;
    bool isInputChannelStereoPair (int index) const;
    bool isOutputChannelStereoPair (int index) const;

    bool acceptsMidi() const;
    bool producesMidi() const;
    bool silenceInProducesSilenceOut() const;
    double getTailLengthSeconds() const;

    const String getPatchName(void);
    const String getPatchNameFromBank(int bank, int patch);

    //==============================================================================
    int getNumPrograms();
    int getCurrentProgram();
    void setCurrentProgram (int index);
    const String getProgramName (int index);
    void changeProgramName (int index, const String& newName);

    //==============================================================================
    MidiProcessing midiProcessing;
  
    //==============================================================================
    void getStateInformation (MemoryBlock& destData);
    void setStateInformation (const void* data, int sizeInBytes);

    //==============================================================================
    // These properties are public so that our editor component can access them
    //  - a bit of a hacky way to do it, but it's only a demo!
  
    // this is kept up to date with the midi messages that arrive, and the UI component
    // registers with it so it can represent the incoming messages
    MidiKeyboardState keyboardState;
  
    // this keeps a copy of the last set of time info that was acquired during an audio
    // callback - the UI component will read this and display it.
    AudioPlayHead::CurrentPositionInfo lastPosInfo;

    //==============================================================================
    // the additional MIDI interface for SysEx data
    MidiMessageCollector midiCollector;

    // these are used to persist the UI's size - the values are stored along with the
    // SidEmu's other parameters, and the UI component will update them when it gets
    // resized.
    int lastUIWidth, lastUIHeight;

	// MIDI In/Out stored in preferences file
	String lastSysexMidiIn, lastSysexMidiOut;
	

#if SID_NUM
    SID *reSID[SID_NUM];
    MbSidEnvironment mbSidEnvironment;
#endif
  
    int reSidEnabled;
    double reSidSampleRate;
    double reSidDeltaCycleCounter;

    double mbSidUpdateCounter;

    sid_regs_t sidRegs[SID_NUM];
    sid_regs_t sidRegsShadow[SID_NUM];
    s32 RESID_Update(u32 mode);

private:
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MidiboxSidAudioProcessor)

    float gain;
    unsigned char bank;
    unsigned char patch;
};

#endif  // __PLUGINPROCESSOR_H_30AA520E__
