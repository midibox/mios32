/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
/*
 * MIDIbox SID Standalone Main Window
 *
 * ==========================================================================
 *
 *  Copyright (C) 2010 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#include "juce_StandaloneFilterWindow.h"
#include "../juce_PluginHeaders.h"

//==============================================================================
/** Somewhere in the codebase of your plugin, you need to implement this function
    and make it create an instance of the filter subclass that you're building.
*/
extern AudioProcessor* JUCE_CALLTYPE createPluginFilter();


//==============================================================================
StandaloneFilterWindow::StandaloneFilterWindow (const String& title,
                                                const Colour& backgroundColour)
    : DocumentWindow (title, backgroundColour,
                      DocumentWindow::minimiseButton
                       | DocumentWindow::closeButton),
      filter (0),
      deviceManager (0),
      optionsButton (0)
{
    // initialise our settings file..
    ApplicationProperties::getInstance()->setStorageParameters(T("MIDIbox_SID_Standalone"),
                                                               T(".xml"),
                                                               String::empty,
                                                               1000,
                                                               PropertiesFile::storeAsXML);

    setTitleBarButtonsRequired (DocumentWindow::minimiseButton | DocumentWindow::closeButton, false);

    PropertySet* const globalSettings = getGlobalSettings();

    optionsButton = new TextButton (T("options"));
    Component::addAndMakeVisible (optionsButton);
    optionsButton->addButtonListener (this);
    optionsButton->setTriggeredOnMouseDown (true);

    JUCE_TRY
    {
        filter = createPluginFilter();

        if (filter != 0)
        {
            deviceManager = new AudioFilterStreamingDeviceManager();
            deviceManager->setFilter (filter);

            XmlElement* savedState = 0;

            if (globalSettings != 0)
                savedState = globalSettings->getXmlValue (T("audioSetup"));

            deviceManager->initialise (filter->getNumInputChannels(),
                                       filter->getNumOutputChannels(),
                                       savedState,
                                       true);

            delete savedState;

            if (globalSettings != 0)
            {
                MemoryBlock data;

                if (data.fromBase64Encoding (globalSettings->getValue (T("filterState")))
                     && data.getSize() > 0)
                {
                    filter->setStateInformation (data.getData(), data.getSize());
                }
            }

            setContentComponent (filter->createEditorIfNeeded(), true, true);

            const int x = globalSettings->getIntValue (T("windowX"), -100);
            const int y = globalSettings->getIntValue (T("windowY"), -100);

            if (x != -100 && y != -100)
                setBoundsConstrained (x, y, getWidth(), getHeight());
            else
                centreWithSize (getWidth(), getHeight());

        }
    }
    JUCE_CATCH_ALL

        setVisible(true);

    if (deviceManager == 0)
    {
        jassertfalse    // Your filter didn't create correctly! In a standalone app that's not too great.
        JUCEApplication::quit();
    }
}

StandaloneFilterWindow::~StandaloneFilterWindow()
{
    PropertySet* const globalSettings = getGlobalSettings();

    globalSettings->setValue (T("windowX"), getX());
    globalSettings->setValue (T("windowY"), getY());

    deleteAndZero (optionsButton);

    if (globalSettings != 0 && deviceManager != 0)
    {
        XmlElement* const xml = deviceManager->createStateXml();
        globalSettings->setValue (T("audioSetup"), xml);
        delete xml;
    }

    deleteAndZero (deviceManager);

    if (globalSettings != 0 && filter != 0)
    {
        MemoryBlock data;
        filter->getStateInformation (data);

        globalSettings->setValue (T("filterState"), data.toBase64Encoding());
    }

    deleteFilter();
}

//==============================================================================
void StandaloneFilterWindow::deleteFilter()
{
    if (deviceManager != 0)
        deviceManager->setFilter (0);

    if (filter != 0 && getContentComponent() != 0)
    {
        filter->editorBeingDeleted (dynamic_cast <AudioProcessorEditor*> (getContentComponent()));
        setContentComponent (0, true);
    }

    deleteAndZero (filter);
}

void StandaloneFilterWindow::resetFilter()
{
    deleteFilter();

    filter = createPluginFilter();

    if (filter != 0)
    {
        if (deviceManager != 0)
            deviceManager->setFilter (filter);

        setContentComponent (filter->createEditorIfNeeded(), true, true);
    }

    PropertySet* const globalSettings = getGlobalSettings();

    if (globalSettings != 0)
        globalSettings->removeValue (T("filterState"));
}

//==============================================================================
void StandaloneFilterWindow::saveState()
{
    PropertySet* const globalSettings = getGlobalSettings();

    FileChooser fc (TRANS("Save current state"),
                    globalSettings != 0 ? File (globalSettings->getValue (T("lastStateFile")))
                                        : File::nonexistent);

    if (fc.browseForFileToSave (true))
    {
        MemoryBlock data;
        filter->getStateInformation (data);

        if (! fc.getResult().replaceWithData (data.getData(), data.getSize()))
        {
            AlertWindow::showMessageBox (AlertWindow::WarningIcon,
                                         TRANS("Error whilst saving"),
                                         TRANS("Couldn't write to the specified file!"));
        }
    }
}

void StandaloneFilterWindow::loadState()
{
    PropertySet* const globalSettings = getGlobalSettings();

    FileChooser fc (TRANS("Load a saved state"),
                    globalSettings != 0 ? File (globalSettings->getValue (T("lastStateFile")))
                                        : File::nonexistent);

    if (fc.browseForFileToOpen())
    {
        MemoryBlock data;

        if (fc.getResult().loadFileAsData (data))
        {
            filter->setStateInformation (data.getData(), data.getSize());
        }
        else
        {
            AlertWindow::showMessageBox (AlertWindow::WarningIcon,
                                         TRANS("Error whilst loading"),
                                         TRANS("Couldn't read from the specified file!"));
        }
    }
}

//==============================================================================
PropertySet* StandaloneFilterWindow::getGlobalSettings()
{
    /* If you want this class to store the plugin's settings, you can set up an
       ApplicationProperties object and use this method as it is, or override this
       method to return your own custom PropertySet.

       If using this method without changing it, you'll probably need to call
       ApplicationProperties::setStorageParameters() in your plugin's constructor to
       tell it where to save the file.
    */
    return ApplicationProperties::getInstance()->getUserSettings();
}

void StandaloneFilterWindow::showAudioSettingsDialog()
{
    AudioDeviceSelectorComponent selectorComp (*deviceManager,
                                               filter->getNumInputChannels(),
                                               filter->getNumInputChannels(),
                                               filter->getNumOutputChannels(),
                                               filter->getNumOutputChannels(),
                                               true, false, true, false);

    selectorComp.setSize (500, 450);

    DialogWindow::showModalDialog (TRANS("Audio Settings"), &selectorComp, this, Colours::lightgrey, true, false, false);
}

//==============================================================================
void StandaloneFilterWindow::closeButtonPressed()
{
    JUCEApplication::quit();
}

void StandaloneFilterWindow::resized()
{
    DocumentWindow::resized();

    if (optionsButton != 0)
        optionsButton->setBounds (8, 6, 60, getTitleBarHeight() - 8);
}

void StandaloneFilterWindow::buttonClicked (Button*)
{
    if (filter == 0)
        return;

    PopupMenu m;
    m.addItem (1, TRANS("Audio Settings..."));
    m.addSeparator();
    m.addItem (2, TRANS("Save current state..."));
    m.addItem (3, TRANS("Load a saved state..."));
    m.addSeparator();
    m.addItem (4, TRANS("Reset to default state"));

    switch (m.showAt (optionsButton))
    {
    case 1:
        showAudioSettingsDialog();
        break;

    case 2:
        saveState();
        break;

    case 3:
        loadState();
        break;

    case 4:
        resetFilter();
        break;

    default:
        break;
    }
}
