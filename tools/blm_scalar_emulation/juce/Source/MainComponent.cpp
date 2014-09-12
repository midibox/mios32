/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
// $Id: UploadHandler.cpp 928 2010-02-20 23:38:06Z tk $


#include "MainComponent.h"

#define OSC_BUFFER_SIZE 2000


//==============================================================================
MainComponent::MainComponent ()
    : blmClass(0)
    , udpSocket(NULL)
    , midiInput(0)
    , midiOutput(0)
    , label(0)
    , label2(0)
    , oscPort(0)
    , initialMidiScanCounter(1) // start step-wise MIDI port scan
{
    addAndMakeVisible (blmClass = new BlmClass (this, 16, 16));
    blmClass->setName ("blmClass");

    addAndMakeVisible(midiInput = new ComboBox ("MIDI Inputs"));
    midiInput->setEditableText(false);
    midiInput->setJustificationType(Justification::centredLeft);
    midiInput->setTextWhenNothingSelected(String::empty);
    midiInput->setTextWhenNoChoicesAvailable("(no choices)");
    midiInput->addItem(TRANS("<< device scan running >>"), -1);
    midiInput->addListener(this);

    addAndMakeVisible(label = new Label("", "MIDI IN: "));
#if JUCE_IOS
    label->setColour(Label::textColourId, Colours::white);
#endif
    label->attachToComponent(midiInput, true);

    addAndMakeVisible (midiOutput = new ComboBox ("MIDI Output"));
    midiOutput->setEditableText(false);
    midiOutput->setJustificationType(Justification::centredLeft);
    midiOutput->setTextWhenNothingSelected(String::empty);
    midiOutput->setTextWhenNoChoicesAvailable("(no choices)");
    midiOutput->addItem(TRANS("<< device scan running >>"), -1);
    midiOutput->addListener(this);

    addAndMakeVisible(label = new Label("", "MIDI OUT: "));
#if JUCE_IOS
    label->setColour(Label::textColourId, Colours::white);
#endif
    label->attachToComponent(midiOutput, true);


    addAndMakeVisible(remoteHostLabel = new Label("Remote Host:", "Remote Host:"));
#if JUCE_IOS
    remoteHostLabel->setColour(Label::textColourId, Colours::white);
#endif
    remoteHostLabel->setJustificationType(Justification::right);

    addAndMakeVisible(remoteHostLine = new TextEditor(String::empty));
    remoteHostLine->setMultiLine(false);
    remoteHostLine->setReturnKeyStartsNewLine(false);
    remoteHostLine->setReadOnly(false);
    remoteHostLine->setScrollbarsShown(false);
    remoteHostLine->setCaretVisible(true);
    remoteHostLine->setPopupMenuEnabled(true);
    remoteHostLine->setTextToShowWhenEmpty("(hostname)", Colours::grey);

    addAndMakeVisible(portNumberWriteLabel = new Label("Port:", "Remote Port:"));
#if JUCE_IOS
    portNumberWriteLabel->setColour(Label::textColourId, Colours::white);
#endif
    portNumberWriteLabel->setJustificationType(Justification::right);

    addAndMakeVisible(portNumberWriteLine = new TextEditor(String::empty));
    portNumberWriteLine->setMultiLine(false);
    portNumberWriteLine->setReturnKeyStartsNewLine(false);
    portNumberWriteLine->setReadOnly(false);
    portNumberWriteLine->setScrollbarsShown(false);
    portNumberWriteLine->setCaretVisible(true);
    portNumberWriteLine->setPopupMenuEnabled(true);
    portNumberWriteLine->setTextToShowWhenEmpty("(port number)", Colours::grey);
    portNumberWriteLine->setInputRestrictions(1000000, "0123456789");

    addAndMakeVisible(portNumberReadLabel = new Label("Port:", "Local Port:"));
#if JUCE_IOS
    portNumberReadLabel->setColour(Label::textColourId, Colours::white);
#endif
    portNumberReadLabel->setJustificationType(Justification::right);

    addAndMakeVisible(portNumberReadLine = new TextEditor(String::empty));
    portNumberReadLine->setMultiLine(false);
    portNumberReadLine->setReturnKeyStartsNewLine(false);
    portNumberReadLine->setReadOnly(false);
    portNumberReadLine->setScrollbarsShown(false);
    portNumberReadLine->setCaretVisible(true);
    portNumberReadLine->setPopupMenuEnabled(true);
    portNumberReadLine->setTextToShowWhenEmpty("(port number)", Colours::grey);
    portNumberReadLine->setInputRestrictions(1000000, "0123456789");

    addAndMakeVisible(connectButton = new TextButton("Connect Button"));
    connectButton->setButtonText("Connect");
    connectButton->addListener(this);

    addAndMakeVisible(disconnectButton = new TextButton("Disconnect Button"));
    disconnectButton->setButtonText("Disconnect");
    disconnectButton->setEnabled(false);
    disconnectButton->addListener(this);

    // restore settings
    PropertiesFile *propertiesFile = BlmProperties::getInstance()->getCommonSettings(true);
    if( propertiesFile ) {
        remoteHostLine->setText(propertiesFile->getValue("oscRemoteHost", String::empty));
        portNumberWriteLine->setText(propertiesFile->getValue("oscPortWrite", "10001"));
        portNumberReadLine->setText(propertiesFile->getValue("oscPortRead", "10000"));
    }

    Timer::startTimer(1);

    setSize(10 + blmClass->getWidth(), 2*(5 + 24) + blmClass->getHeight());
}

MainComponent::~MainComponent()
{
    deleteAndZero(udpSocket);
    deleteAndZero(blmClass);
    deleteAndZero(midiInput);
    deleteAndZero(midiOutput);
    deleteAndZero(label);
    deleteAndZero(label2);
	deleteAllChildren();
}

//==============================================================================
void MainComponent::paint (Graphics& g)
{
#if JUCE_IOS
    // resize when orientation has been changed
    Rectangle<int> iPadBounds = getPeer()->getBounds();

    if  ( (iPadBounds.getWidth()!=getWidth()) && (iPadBounds.getHeight()!=getHeight()) )
        getPeer()->handleScreenSizeChange();
#endif

#if JUCE_IOS
    g.fillAll(Colour(0xff000000));
#else
    g.fillAll(Colour(0xffd0d0d0));
#endif
}

void MainComponent::resized()
{
    int middle = getWidth() / 2;
    midiInput->setBounds(5+80, 5, middle-10-80, 24);
    midiOutput->setBounds(middle+5+80, 5, middle-10-80, 24);

    int buttonY = 5+24+10;
    int buttonWidth = 90;
    int ipLineWidth = 125;
    int buttonX = getWidth() - 2*buttonWidth - 10-4;
    remoteHostLabel->setBounds(10, buttonY, 75, 24);
    remoteHostLine->setBounds(10+75+4, buttonY, ipLineWidth, 24);
    portNumberWriteLabel->setBounds(10+75+4+ipLineWidth+10, buttonY, 75, 24);
    portNumberWriteLine->setBounds(10+75+4+ipLineWidth+10+75+4, buttonY, 50, 24);
    portNumberReadLabel->setBounds(10+75+4+ipLineWidth+10+75+4+50+4, buttonY, 75, 24);
    portNumberReadLine->setBounds(10+75+4+ipLineWidth+10+75+4+50+4+75+4, buttonY, 50, 24);
    connectButton->setBounds(buttonX, buttonY, buttonWidth, 24);
    disconnectButton->setBounds(buttonX + buttonWidth + 10, buttonY, buttonWidth, 24);

    blmClass->setTopLeftPosition(5, buttonY+5+24+10);
}


//==============================================================================
void MainComponent::comboBoxChanged (ComboBox* comboBoxThatHasChanged)
{
    if (comboBoxThatHasChanged == midiInput)
    {
		setMidiInput(midiInput->getText());
        blmClass->sendBLMLayout();
        PropertiesFile *propertiesFile = BlmProperties::getInstance()->getCommonSettings(true);
        if( propertiesFile )
            propertiesFile->setValue("midiIn", midiInput->getText());
    }
    else if (comboBoxThatHasChanged == midiOutput)
    {
		setMidiOutput(midiOutput->getText());
        blmClass->sendBLMLayout();
        PropertiesFile *propertiesFile = BlmProperties::getInstance()->getCommonSettings(true);
        if( propertiesFile )
            propertiesFile->setValue("midiOut", midiOutput->getText());
    }
}


//==============================================================================
void MainComponent::buttonClicked(Button* buttonThatWasClicked)
{
    if( buttonThatWasClicked == connectButton ) {
        // store settings
        PropertiesFile *propertiesFile = BlmProperties::getInstance()->getCommonSettings(true);
        if( propertiesFile ) {
            propertiesFile->setValue("oscRemoteHost", remoteHostLine->getText());
            propertiesFile->setValue("oscPortWrite", portNumberWriteLine->getText());
            propertiesFile->setValue("oscPortRead", portNumberReadLine->getText());
        }

        connectButton->setEnabled(false);
        disconnectButton->setEnabled(false); // will be set to true by timer once connection available
        remoteHostLine->setEnabled(false);
        portNumberWriteLine->setEnabled(false);
        portNumberReadLine->setEnabled(false);
        startTimer(1); // check for next message each 1 mS

        // send BLM layout to synchronize with MBSEQ
        blmClass->sendBLMLayout();
    } else if( buttonThatWasClicked == disconnectButton ) {
        connectButton->setEnabled(true);
        disconnectButton->setEnabled(false);
        remoteHostLine->setEnabled(true);
        portNumberWriteLine->setEnabled(true);
        portNumberReadLine->setEnabled(true);
        stopTimer();
        if( udpSocket )
            udpSocket->disconnect();
        deleteAndZero(udpSocket);
    }
}

//==============================================================================
// Should be called after startup once window is visible
void MainComponent::scanMidiInDevices()
{
    midiInput->setEnabled(false);
	midiInput->clear();
    midiInput->addItem (TRANS("<< none >>"), -1);
    midiInput->addSeparator();

    // restore MIDI input port from .xml file
    PropertiesFile *propertiesFile = BlmProperties::getInstance()->getCommonSettings(true);
    String selectedPort = propertiesFile ? propertiesFile->getValue("midiIn", String::empty) : String::empty;

    StringArray midiPorts = MidiInput::getDevices();

    int current = -1;
    for(int i=0; i<midiPorts.size(); ++i) {
        midiInput->addItem(midiPorts[i], i+1);
        bool enabled = midiPorts[i] == selectedPort;

        if( enabled )
            current = i + 1;

        audioDeviceManager.setMidiInputEnabled(midiPorts[i], enabled);
    }
    midiInput->setSelectedId(current, true);
    midiInput->setEnabled(true);
}


//==============================================================================
// Should be called after startup once window is visible
void MainComponent::scanMidiOutDevices()
{
    midiOutput->setEnabled(false);
	midiOutput->clear();
    midiOutput->addItem (TRANS("<< none >>"), -1);
    midiOutput->addSeparator();

    // restore MIDI output port from .xml file
    PropertiesFile *propertiesFile = BlmProperties::getInstance()->getCommonSettings(true);
    String selectedPort = propertiesFile ? propertiesFile->getValue("midiOut", String::empty) : String::empty;

    StringArray midiPorts = MidiOutput::getDevices();

    int current = -1;
    for(int i=0; i<midiPorts.size(); ++i) {
        midiOutput->addItem(midiPorts[i], i+1);
        bool enabled = midiPorts[i] == selectedPort;

        if( enabled ) {
            current = i + 1;
            audioDeviceManager.setDefaultMidiOutput(midiPorts[i]);
        }
    }
    midiOutput->setSelectedId(current, true);
    midiOutput->setEnabled(true);
}


//==============================================================================
void MainComponent::setMidiInput(const String &port)
{
    const StringArray allMidiIns(MidiInput::getDevices());
    for (int i = allMidiIns.size(); --i >= 0;) {
        bool enabled = allMidiIns[i] == port;
        audioDeviceManager.setMidiInputEnabled(allMidiIns[i], enabled);
    }
}

void MainComponent::setMidiOutput(const String &port)
{
    audioDeviceManager.setDefaultMidiOutput(port);
}

//==============================================================================
void MainComponent::sendMidiMessage(const MidiMessage& message)
{
    // common MIDI
    MidiOutput *out = audioDeviceManager.getDefaultMidiOutput();
    if( out )
        out->sendMessageNow(message);

    // MIDI via OSC
    if( udpSocket != NULL ) {
        Array<uint8>packet;

        uint32 size = message.getRawDataSize();
        uint8 *data = (uint8 *)message.getRawData();

        if( size > 1 && data[0] == 0xf0 ) {
            // SysEx messages will be sent as blobs
            char midiPath[10];
            sprintf(midiPath, "/midi%d", oscPort+1);
            packet.addArray(OscHelper::createString(String(midiPath)));
            packet.addArray(OscHelper::createString(",b"));
            packet.addArray(OscHelper::createBlob(data, size));
        } else {
            // and remainaing events as MIDI packet
            unsigned value = 0;
            if( size >= 1 )
                value |= data[0] << 24;
            if( size >= 2 )
                value |= data[1] << 16;
            if( size >= 3 )
                value |= data[2] << 8;

            char midiPath[10];
            sprintf(midiPath, "/midi%d", oscPort+1);
            packet.addArray(OscHelper::createString(String(midiPath)));
            packet.addArray(OscHelper::createString(",m"));
            packet.addArray(OscHelper::createMIDI(value));
        }

        if( packet.size() )
            udpSocket->write((unsigned char*)&packet.getReference(0), packet.size());
    }
}

//==============================================================================
void MainComponent::timerCallback()
{
    unsigned char oscBuffer[OSC_BUFFER_SIZE];

    // step-wise MIDI port scan after startup
    if( initialMidiScanCounter ) {
        switch( initialMidiScanCounter ) {
        case 1:
            scanMidiInDevices();
            ++initialMidiScanCounter;
            break;

        case 2:
            scanMidiOutDevices();
            ++initialMidiScanCounter;
            break;

        case 3:
            blmClass->sendBLMLayout();

            initialMidiScanCounter = 0; // stop scan
            Timer::stopTimer();
#if 0
            break;
#else
            return; // timer will be called again once connect button has been pressed
#endif
        }
    }

    if( initialMidiScanCounter == 0 ) {
        // UDP/OSC handling
        if( udpSocket == NULL ) {
            stopTimer(); // will be continued if connection established

            udpSocket = new UdpSocket();
            if( !udpSocket->connect(remoteHostLine->getText(), portNumberReadLine->getText().getIntValue(), portNumberWriteLine->getText().getIntValue()) ) {
                AlertWindow::showMessageBox(AlertWindow::WarningIcon,
                                            "No connection to remote host",
                                            "Check remote host name (or IP) and port numbers!",
                                            String::empty);

                deleteAndZero(udpSocket);
                connectButton->setEnabled(true);
                disconnectButton->setEnabled(false);
                remoteHostLine->setEnabled(true);
                portNumberWriteLine->setEnabled(true);
                portNumberReadLine->setEnabled(true);
            } else {
                disconnectButton->setEnabled(true);
                startTimer(1); // continue timer
            }

            return;
        }

        // check for new OSC packet(s)
        // check up to 25 times per mS
        int checkCounter = 0;
        unsigned size;
        while( (++checkCounter <= 25) && (size=udpSocket->read(oscBuffer, OSC_BUFFER_SIZE)) ) {

            OscHelper::OscSearchTreeT searchTree[] = {
                { "midi",  NULL, this, 0x00000000 },
                { "midi1", NULL, this, 0x00000000 },
                { "midi2", NULL, this, 0x00000001 },
                { "midi3", NULL, this, 0x00000002 },
                { "midi4", NULL, this, 0x00000003 },

#if 0
                { NULL, NULL, this, 0 } // terminator - will receive all messages that haven't been parsed
#else
                // disabled "this" - if it's set, parsedOscPacket will be called on any packet (again)
                { NULL, NULL, NULL, 0 } // terminator - will receive all messages that haven't been parsed
#endif
            };

            int status = OscHelper::parsePacket(oscBuffer, size, searchTree);
            if( status == -1 )
                printf("[OSC] received packet with invalid format!\n");
            else if( status == -2 )
                printf("[OSC] received packet with invalid element format!\n");
            else if( status == -3 )
                printf("[OSC] received packet with unsupported format!\n");
            else if( status == -4 )
                printf("[OSC] MIOS32_OSC_MAX_PATH_PARTS has been exceeded!\n");
            else if( status == -5 )
                printf("[OSC] received erroneous packet with status %d!\n", status);
            else {
#if 0
                printf("[OSC] received valid packet!\n");
#endif
            }
        }
    }
}


//==============================================================================
void MainComponent::parsedOscPacket(const OscHelper::OscArgsT& oscArgs, const unsigned& methodArg)
{
    // we expect at least 1 argument
    if( oscArgs.numArgs < 1 )
        return; // wrong number of arguments

    // check for MIDI event
    if( oscArgs.argType[0] == 'm' ) {
        unsigned m = OscHelper::getMIDI(oscArgs.argPtr[0]);
#if 0
        printf("[OSC] received MIDI event 0x%08x\n", m);
#endif
        MidiMessage message((uint8)((m >> 24) & 0xff),
                            (uint8)((m >> 16) & 0xff),
                            (uint8)((m >>  8) & 0xff));
        uint8 runningStatus = (m >> 24) & 0xff;

        // switch to OSC port from which we received
        oscPort = methodArg & 0xf;

        blmClass->BLMIncomingMidiMessage(message, runningStatus);
    } else  if( oscArgs.argType[0] == 'b' ) {
        // SysEx stream is embedded into blob
        unsigned len = OscHelper::getBlobLength(oscArgs.argPtr[0]);
        unsigned char *blob = OscHelper::getBlobData(oscArgs.argPtr[0]);
#if 0
        printf("[OSC] received SysEx blob with %d bytes\n", len);
#endif

        // switch to OSC port from which we received
        oscPort = methodArg & 0xf;

        MidiMessage message(blob, len);
        uint8 runningStatus = blob[0];

        blmClass->BLMIncomingMidiMessage(message, runningStatus);
    }
}

