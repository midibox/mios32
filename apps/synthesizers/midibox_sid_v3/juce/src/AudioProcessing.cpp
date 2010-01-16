/*
  ==============================================================================

   This file is part of the JUCE library - "Jules' Utility Class Extensions"
   Copyright 2004-7 by Raw Material Software ltd.

  ------------------------------------------------------------------------------

   JUCE can be redistributed and/or modified under the terms of the
   GNU General Public License, as published by the Free Software Foundation;
   either version 2 of the License, or (at your option) any later version.

   JUCE is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with JUCE; if not, visit www.gnu.org/licenses or write to the
   Free Software Foundation, Inc., 59 Temple Place, Suite 330,
   Boston, MA 02111-1307 USA

  ------------------------------------------------------------------------------

   If you'd like to release a closed-source product which uses JUCE, commercial
   licenses are also available: visit www.rawmaterialsoftware.com/juce for
   more information.

  ==============================================================================
*/

#include "includes.h"
#include "AudioProcessing.h"
#include "EditorComponent.h"

#include <mios32.h>

// for output to console (stderr)
// should be at least 1 to inform the user on fatal errors
#define DEBUG_VERBOSE_LEVEL 2

// sampling method
#define RESID_SAMPLING_METHOD SAMPLE_RESAMPLE_INTERPOLATE
//#define RESID_SAMPLING_METHOD SAMPLE_FAST

// SID frequency
#define RESID_FREQUENCY 1000000

// play testtone at startup?
// nice for first checks of the emulation w/o MIDI input
#define RESID_PLAY_TESTTONE 1



// these global variables are used by ReSID
double mixer_value1;
double mixer_value2;
double mixer_value3;


//==============================================================================
/**
    This function must be implemented to create a new instance of your
    plugin object.
*/
AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AudioProcessing();
}

//==============================================================================
AudioProcessing::AudioProcessing()
{
  gain = 1.0f;
  lastUIWidth = 400;
  lastUIHeight = 140;
  
  zeromem (&lastPosInfo, sizeof (lastPosInfo));
  lastPosInfo.timeSigNumerator = 4;
  lastPosInfo.timeSigDenominator = 4;
  lastPosInfo.bpm = 120;
  
  // reSID integration
  mixer_value1 = 1.0f;
  mixer_value2 = 1.0f;
  mixer_value3 = 1.0f;
  reSidDeltaCycleCounter = 0.0f;
  reSidSampleRate = 44100.0f;

#if SID_NUM
  reSidEnabled = 1;
  for(int i=0; i<SID_NUM; ++i) {
    reSID[i] = new SID;
    reSID[i]->reset();
    if( !reSID[i]->set_sampling_parameters(RESID_FREQUENCY, RESID_SAMPLING_METHOD, reSidSampleRate) ) {
#if DEBUG_VERBOSE_LEVEL >= 1
      fprintf(stderr, "Initialisation of reSID[%d] failed for unexpected reasons! All SIDs disabled now!\n", i);
#endif
      reSidEnabled = 0;
    }
  }
#else
  reSidEnabled = 0;
#if DEBUG_VERBOSE_LEVEL >= 1
  fprintf(stderr, "SID_NUM == 0: emulation disabled!\n");
#endif
#endif
}

AudioProcessing::~AudioProcessing()
{
#if SID_NUM
  for(int i=0; i<SID_NUM; ++i)
    delete reSID[i];
#endif
}

//==============================================================================
const String AudioProcessing::getName() const
{
  return "Juce Demo Filter";
}

int AudioProcessing::getNumParameters()
{
  return 1;
}

float AudioProcessing::getParameter (int index)
{
  switch( index ) {
    case 0: return gain;
  }
  
  return 0.0f;
}

void AudioProcessing::setParameter (int index, float newValue)
{
  switch( index ) {
    case 0:
      if( gain != newValue ) {
	gain = newValue;

	// if this is changing the gain, broadcast a change message which
	// our editor will pick up.
	sendChangeMessage (this);
      }
  }
}

const String AudioProcessing::getParameterName (int index)
{
  switch( index ) {
    case 0: return T("gain");
  }

  return String::empty;
}

const String AudioProcessing::getParameterText (int index)
{
  switch( index ) {
    case 0: return String (gain, 2);
  }
  
  return String::empty;
}

const String AudioProcessing::getInputChannelName (const int channelIndex) const
{
  return String (channelIndex + 1);
}

const String AudioProcessing::getOutputChannelName (const int channelIndex) const
{
  return String (channelIndex + 1);
}

bool AudioProcessing::isInputChannelStereoPair (int index) const
{
  return false;
}

bool AudioProcessing::isOutputChannelStereoPair (int index) const
{
  return false;
}

bool AudioProcessing::acceptsMidi() const
{
  return true;
}

bool AudioProcessing::producesMidi() const
{
  return true;
}

//==============================================================================
void AudioProcessing::prepareToPlay (double sampleRate, int samplesPerBlock)
{
  // do your pre-playback setup stuff here..
  keyboardState.reset();

#if SID_NUM
  reSidEnabled = 1;
  reSidSampleRate = sampleRate;

  for(int i=0; i<SID_NUM; ++i) {
    reSID[i]->reset();
    if( !reSID[i]->set_sampling_parameters(RESID_FREQUENCY, RESID_SAMPLING_METHOD, reSidSampleRate) ) {
#if DEBUG_VERBOSE_LEVEL >= 1
      fprintf(stderr, "Initialisation of reSID[%d] failed at sample rate %7.2f Hz! All SIDs disabled now!\n", i, sampleRate);
#endif
      reSidEnabled = 0;
    }

#if RESID_PLAY_TESTTONE
    if( reSidEnabled ) {
#if DEBUG_VERBOSE_LEVEL >= 2
      fprintf(stderr, "playing testtone on SID %d\n", i);
#endif
      reSID[i]->write(0x18, 0x0f); // Volume
      reSID[i]->write(0x05, 0x00); // Attack/Decay
      reSID[i]->write(0x06, 0xf8); // Sustain/Release
      reSID[i]->write(0x00, 16777 & 0xff); // Frequency Low
      reSID[i]->write(0x01, 16777 >> 8); // Frequency High
      reSID[i]->write(0x04, 0x11); // Waveform + Gate
    }
#endif
  }
#endif
}

void AudioProcessing::releaseResources()
{
  // when playback stops, you can use this as an opportunity to free up any
  // spare memory, etc.
}

void AudioProcessing::processBlock (AudioSampleBuffer& buffer,
				    MidiBuffer& midiMessages)
{
  // determine how many channels have to be services
  int numChannels = getNumInputChannels();

#if SID_NUM
  if( reSidEnabled ) {
    if( numChannels < SID_NUM )
      numChannels = SID_NUM;
    if( numChannels > getNumOutputChannels() )
      numChannels = getNumOutputChannels();
    
    // number of samples which have to be rendered
    int numSamples = buffer.getNumSamples();
    
    // add SID sound(s) to output(s)
    // TK: this nested loop isn't optimal for CPU load, but we have to ensure that all SIDs are in lock-step
    for(int i=0; i<numSamples; ++i) {
      // number of SID cycles per sample
      reSidDeltaCycleCounter += (double)RESID_FREQUENCY / reSidSampleRate;
      cycle_count delta_t = (cycle_count)reSidDeltaCycleCounter;
      reSidDeltaCycleCounter -= (double)delta_t;
      
      for(int channel = 0; channel < numChannels; ++channel) {
	reSID[channel]->clock(delta_t);
	float currentSample = (float)reSID[channel]->output(16) / 32768.0;
	*buffer.getSampleData(channel, i) = currentSample;
      }
    }
  }
#endif

  // for each of our input channels, we'll attenuate its level by the
  // amount that our volume parameter is set to.
  for (int channel = 0; channel < numChannels; ++channel) {
    buffer.applyGain (channel, 0, buffer.getNumSamples(), gain);
  }
  
  // in case we have more outputs than inputs, we'll clear any output
  // channels that didn't contain input data, (because these aren't
  // guaranteed to be empty - they may contain garbage).
  for (int i = numChannels; i < getNumOutputChannels(); ++i) {
    buffer.clear (i, 0, buffer.getNumSamples());
  }
  
  // if any midi messages come in, use them to update the keyboard state object. This
  // object sends notification to the UI component about key up/down changes
  keyboardState.processNextMidiBuffer (midiMessages,
				       0, buffer.getNumSamples(),
				       true);
  
  // have a go at getting the current time from the host, and if it's changed, tell
  // our UI to update itself.
  AudioPlayHead::CurrentPositionInfo pos;
  
  if (getPlayHead() != 0 && getPlayHead()->getCurrentPosition (pos)) {
    if (memcmp (&pos, &lastPosInfo, sizeof (pos)) != 0) {
      lastPosInfo = pos;
      sendChangeMessage (this);
    }
  } else {
    zeromem (&lastPosInfo, sizeof (lastPosInfo));
    lastPosInfo.timeSigNumerator = 4;
    lastPosInfo.timeSigDenominator = 4;
    lastPosInfo.bpm = 120;
  }
}

//==============================================================================
AudioProcessorEditor* AudioProcessing::createEditor()
{
  return new EditorComponent (this);
}

//==============================================================================
void AudioProcessing::getStateInformation (MemoryBlock& destData)
{
  // you can store your parameters as binary data if you want to or if you've got
  // a load of binary to put in there, but if you're not doing anything too heavy,
  // XML is a much cleaner way of doing it - here's an example of how to store your
  // params as XML..
  
  // create an outer XML element..
  XmlElement xmlState (T("MYPLUGINSETTINGS"));
  
  // add some attributes to it..
  xmlState.setAttribute (T("pluginVersion"), 1);
  xmlState.setAttribute (T("gainLevel"), gain);
  xmlState.setAttribute (T("uiWidth"), lastUIWidth);
  xmlState.setAttribute (T("uiHeight"), lastUIHeight);
  
  // you could also add as many child elements as you need to here..
  
  
  // then use this helper function to stuff it into the binary blob and return it..
  copyXmlToBinary (xmlState, destData);
}

void AudioProcessing::setStateInformation (const void* data, int sizeInBytes)
{
  // use this helper function to get the XML from this binary blob..
  XmlElement* const xmlState = getXmlFromBinary (data, sizeInBytes);
  
  if (xmlState != 0)
  {
    // check that it's the right type of xml..
    if (xmlState->hasTagName (T("MYPLUGINSETTINGS")))
    {
      // ok, now pull out our parameters..
      gain = (float) xmlState->getDoubleAttribute (T("gainLevel"), gain);
      
      lastUIWidth = xmlState->getIntAttribute (T("uiWidth"), lastUIWidth);
      lastUIHeight = xmlState->getIntAttribute (T("uiHeight"), lastUIHeight);
      
      sendChangeMessage (this);
    }
    
    delete xmlState;
  }
}



// Accessible for C
extern "C" {
  void SID_Wrapper_Write(unsigned char cs, unsigned char addr, unsigned char data, unsigned char reset)
  {
#if 0
    // Requires some work to handle multiple AU instances :-/
#if SID_NUM
    if( reSidEnabled ) {
      int i;
      
#if DEBUG_VERBOSE_LEVEL >= 2
      fprintf(stderr, "cs%d %02x:%02x\n", cs, addr, data);
#endif
      if( reset ) {
	for(i=0; i<SID_NUM; ++i)
	  reSID[i].reset();
      } else {
	for(i=0; i<SID_NUM; ++i) {
	  if( cs & (1 << i) )
	    reSID[0].write(addr, data);
	}
      }
    }
#endif
#endif
  }
}

