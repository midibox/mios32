/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
// $Id$
/*
 * File browser for MIOS32 applications
 *
 * ==========================================================================
 *
 *  Copyright (C) 2012 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#include "MiosFileBrowser.h"
#include "MiosStudio.h"

//==============================================================================
class MiosFileBrowserFileItem
{
public:
    String parentPath;
    String name;
    bool isDirectory;

    OwnedArray<MiosFileBrowserFileItem> childs;

    MiosFileBrowserFileItem(String _parentPath, String _itemName, bool _isDirectory)
        : parentPath(_parentPath)
        , name(_itemName)
        , isDirectory(_isDirectory)
    {
    }

    ~MiosFileBrowserFileItem()
    {
    }

    MiosFileBrowserFileItem* createChild(String parentPath, String childName, bool isDirectory) {
        MiosFileBrowserFileItem* child = new MiosFileBrowserFileItem(parentPath, childName, isDirectory);
        childs.add(child);
        return child;
    }
};


//==============================================================================
class MiosFileBrowserItem : public TreeViewItem
{
public:
    MiosFileBrowserFileItem *fileItem;
    MiosFileBrowser* browser;

    MiosFileBrowserItem(MiosFileBrowserFileItem *_fileItem, MiosFileBrowser* _browser)
        : fileItem(_fileItem)
        , browser(_browser)
    {
    }

    ~MiosFileBrowserItem()
    {
    }

    int getItemWidth() const
    {
        return 100;
    }

    const String getUniqueName() const
    {
        if( fileItem->parentPath.compare(T("/")) == 0 ) {
            return T("/") + fileItem->name;
        } else {
            if( fileItem->name.compare(T("/")) == 0 )
                return T("/");
            else
                return fileItem->parentPath + T("/") + fileItem->name;
        }
    }

    bool mightContainSubItems()
    {
        return fileItem->isDirectory;
    }

    void paintItem (Graphics& g, int width, int height)
    {
        // if this item is selected, fill it with a background colour..
        if( isSelected() )
            g.fillAll(Colours::blue.withAlpha (0.3f));

        // use a "colour" attribute in the xml tag for this node to set the text colour..
        g.setColour(Colour(0xff000000));

        g.setFont(height * 0.7f);

        // draw the item name
        String name(fileItem->name);
        g.drawText(name,
                   4, 0, width - 4, height,
                   Justification::centredLeft, true);
    }

    void itemOpennessChanged (bool isNowOpen)
    {
        if( isNowOpen ) {
            // if we've not already done so, we'll now add the tree's sub-items. You could
            // also choose to delete the existing ones and refresh them if that's more suitable
            // in your app.
            if( getNumSubItems() == 0 ) {
                for(int i=0; i<fileItem->childs.size(); ++i) {
                    MiosFileBrowserItem* item;
                    addSubItem(item = new MiosFileBrowserItem(fileItem->childs[i], browser));
                    if( fileItem->isDirectory )
                        item->setOpen(true);
                }
            }
        } else {
            // in this case, we'll leave any sub-items in the tree when the node gets closed,
            // though you could choose to delete them if that's more appropriate for
            // your application.
        }
    }


    void itemClicked (const MouseEvent& e)
    {
        browser->treeItemClicked(this);
    }

    void itemDoubleClicked (const MouseEvent& e)
    {
        TreeViewItem::itemDoubleClicked(e);
        browser->treeItemDoubleClicked(this);
    }

    const String getDragSourceDescription()
    {
        return "MIOS Filebrowser Items";
    }

private:
};

//==============================================================================
//==============================================================================
//==============================================================================
MiosFileBrowser::MiosFileBrowser(MiosStudio *_miosStudio)
    : miosStudio(_miosStudio)
    , rootItem(NULL)
    , rootFileItem(NULL)
    , currentReadInProgress(false)
    , currentReadError(false)
    , currentWriteInProgress(false)
    , currentWriteError(false)
{
    addAndMakeVisible(statusLabel = new Label(T("Status"), String::empty));
    statusLabel->setJustificationType(Justification::left);
    statusLabel->setText(T("Please connect to MIOS32 core by pressing the Update button!"), true);

    addAndMakeVisible(updateButton = new TextButton(T("Update Button")));
    updateButton->setButtonText(T("Update"));
    updateButton->addListener(this);

    addAndMakeVisible(uploadButton = new TextButton(T("Upload Button")));
    uploadButton->setButtonText(T("Upload"));
    uploadButton->addListener(this);

    addAndMakeVisible(downloadButton = new TextButton(T("Download Button")));
    downloadButton->setButtonText(T("Download"));
    downloadButton->addListener(this);

    addAndMakeVisible(editTextButton = new TextButton(T("EditText Button")));
    editTextButton->setButtonText(T("Edit Text"));
    editTextButton->addListener(this);

    addAndMakeVisible(editHexButton = new TextButton(T("EditHex Button")));
    editHexButton->setButtonText(T("Edit Hex"));
    editHexButton->addListener(this);

    addAndMakeVisible(createDirButton = new TextButton(T("CreateDir Button")));
    createDirButton->setButtonText(T("Create Dir"));
    createDirButton->addListener(this);

    addAndMakeVisible(removeButton = new TextButton(T("Remove Button")));
    removeButton->setButtonText(T("Remove"));
    removeButton->addListener(this);

    addAndMakeVisible(treeView = new TreeView(T("SD Card")));
    treeView->setMultiSelectEnabled(false);

    rootFileItem = new MiosFileBrowserFileItem(T(""), T("/"), true);
    currentDirItem = rootFileItem;
    updateTreeView(false);

    resizeLimits.setSizeLimits(100, 300, 2048, 2048);
    addAndMakeVisible(resizer = new ResizableCornerComponent(this, &resizeLimits));

    setSize(860, 500);
}

MiosFileBrowser::~MiosFileBrowser()
{
}

//==============================================================================
void MiosFileBrowser::paint (Graphics& g)
{
    //g.fillAll(Colour(0xffc1d0ff));
    g.fillAll(Colour(0xffffffff));
}

void MiosFileBrowser::resized()
{
    int sendButtonY = 16;
    int sendButtonWidth = 72;

    updateButton->setBounds   (10 , sendButtonY + 0*32 + 0*16, sendButtonWidth, 24);
    downloadButton->setBounds (10 , sendButtonY + 1*32 + 1*16, sendButtonWidth, 24);
    uploadButton->setBounds   (10 , sendButtonY + 2*32 + 1*16, sendButtonWidth, 24);
    editTextButton->setBounds (10 , sendButtonY + 3*32 + 2*16, sendButtonWidth, 24);
    editHexButton->setBounds  (10 , sendButtonY + 4*32 + 2*16, sendButtonWidth, 24);
    createDirButton->setBounds(10 , sendButtonY + 5*32 + 3*16, sendButtonWidth, 24);
    removeButton->setBounds   (10 , sendButtonY + 6*32 + 4*16, sendButtonWidth, 24);

    treeView->setBounds(10 + 80, 16, getWidth(), getHeight()-32-24);

    statusLabel->setBounds(10, getHeight()-24, getWidth()-20, 24);
    resizer->setBounds(getWidth()-16, getHeight()-16, 16, 16);
}

//==============================================================================
void MiosFileBrowser::buttonClicked(Button* buttonThatWasClicked)
{
    if( buttonThatWasClicked == updateButton ) {
        if( rootFileItem )
            delete rootFileItem;
        rootFileItem = new MiosFileBrowserFileItem(T(""), T("/"), true);
        currentDirItem = rootFileItem;

        updateTreeView(false);

        disableFileButtons();
        currentDirFetchItems.clear();
        currentDirPath = T("/");
        sendCommand(T("dir ") + currentDirPath);
    } else if( buttonThatWasClicked == downloadButton || 
               buttonThatWasClicked == editTextButton ||
               buttonThatWasClicked == editHexButton ) {

        if( treeView->getNumSelectedItems() ) {
            TreeViewItem* selectedItem = treeView->getSelectedItem(0);
            currentReadFile = selectedItem->getUniqueName();
            disableFileButtons();
            sendCommand(T("read ") + currentReadFile);
        }
    } else if( buttonThatWasClicked == uploadButton ) {
        disableFileButtons();
        if( !uploadFile() ) {
            enableFileButtons();
        }
    } else if( buttonThatWasClicked == createDirButton ) {
        disableFileButtons();
        sendCommand(T("mkdir ") + getSelectedPath() + T("test")); // tmp.
    } else if( buttonThatWasClicked == removeButton ) {
        if( treeView->getNumSelectedItems() ) {
            TreeViewItem* selectedItem = treeView->getSelectedItem(0);
            String fileName(selectedItem->getUniqueName());
            if( AlertWindow::showOkCancelBox(AlertWindow::WarningIcon,
                                             T("Removing ") + fileName,
                                             T("Do you really want to remove\n") + fileName + T("?"),
                                             T("Remove"),
                                             T("Cancel")) ) {
                disableFileButtons();
                sendCommand(T("del ") + fileName);
            }
        }
    }
}


//==============================================================================
void MiosFileBrowser::filenameComponentChanged(FilenameComponent *fileComponentThatHasChanged)
{
}

//==============================================================================
String MiosFileBrowser::getSelectedPath(void)
{
    String selectedPath(T("/"));

    TreeViewItem* selectedItem = treeView->getSelectedItem(0);
    if( selectedItem ) {
        selectedPath = selectedItem->getUniqueName();
        if( selectedPath.compare(T("/")) != 0 ) {
            if( !selectedItem->mightContainSubItems() ) {
                selectedPath = selectedPath.substring(0, selectedPath.lastIndexOfChar('/')) + T("/");
            } else {
                selectedPath += T("/");
            }
        }
    }

    //std::cout << "Selected Path: " << selectedPath << std::endl;

    return selectedPath;
}

//==============================================================================
void MiosFileBrowser::disableFileButtons(void)
{
    uploadButton->setEnabled(false);
    downloadButton->setEnabled(false);
    editTextButton->setEnabled(false);
    editHexButton->setEnabled(false);
    createDirButton->setEnabled(false);
    removeButton->setEnabled(false);
}

void MiosFileBrowser::enableFileButtons(void)
{
    uploadButton->setEnabled(true);
    downloadButton->setEnabled(true);
    editTextButton->setEnabled(true);
    editHexButton->setEnabled(true);
    createDirButton->setEnabled(true);
    removeButton->setEnabled(true);
}

void MiosFileBrowser::enableDirButtons(void)
{
    uploadButton->setEnabled(true);
    createDirButton->setEnabled(true);
    removeButton->setEnabled(true);
}

//==============================================================================
void MiosFileBrowser::updateTreeView(bool accessPossible)
{
    disableFileButtons();

#if 0
    // TK: currently crashes for whatever reason...
    XmlElement* openStates = treeView->getOpennessState(true); // including scroll position
#endif

    if( rootItem )
        treeView->deleteRootItem();

    if( rootFileItem ) {
        rootItem = new MiosFileBrowserItem(rootFileItem, this);
        rootItem->setOpen(true);
        treeView->setRootItem(rootItem);

#if 0
        if( !treeView->getSelectedItem(0) )
            treeView->scrollToKeepItemVisible(rootItem);
#endif

#if 0
        if( openStates ) {
            treeView->restoreOpennessState(*openStates);
            delete openStates;
        }
#endif

        if( accessPossible ) {
            uploadButton->setEnabled(true);
            createDirButton->setEnabled(true);
        }
    }
}

void MiosFileBrowser::treeItemClicked(MiosFileBrowserItem* item)
{
    disableFileButtons();

    if( item->fileItem->isDirectory ) {
        enableDirButtons();
    } else {
        enableFileButtons();
    }
}

void MiosFileBrowser::treeItemDoubleClicked(MiosFileBrowserItem* item)
{
}

//==============================================================================
bool MiosFileBrowser::storeDownloadedFile(void)
{
    if( !currentReadData.size() )
        return false;

    // restore default path
    String defaultPath(File::getSpecialLocation(File::userHomeDirectory).getFullPathName());
    PropertiesFile *propertiesFile = ApplicationProperties::getInstance()->getCommonSettings(true);
    if( propertiesFile ) {
        defaultPath = propertiesFile->getValue(T("defaultFilebrowserPath"), defaultPath);
    }
    String readFileName(currentReadFile.substring(currentReadFile.lastIndexOfChar('/')+1));
    File defaultPathFile(defaultPath + "/" + readFileName);
    FileChooser myChooser(T("Store ") + currentReadFile, defaultPathFile);
    if( !myChooser.browseForFileToSave(true) ) {
        statusLabel->setText(T("Cancled save operation for ") + currentReadFile, true);
        return false;
    } else {
        File outFile(myChooser.getResult());

        // store default path
        if( propertiesFile ) {
            propertiesFile->setValue(T("defaultFilebrowserPath"), outFile.getParentDirectory().getFullPathName());
        }

        FileOutputStream *outFileStream = NULL;
        outFile.deleteFile();
        if( !(outFileStream=outFile.createOutputStream()) ||
            outFileStream->failedToOpen() ) {
            AlertWindow::showMessageBox(AlertWindow::WarningIcon,
                                        String::empty,
                                        T("File cannot be created!"),
                                        String::empty);
            statusLabel->setText(T("Failed to save ") + defaultPathFile.getFullPathName(), true);
            return false;
        } else {
            outFileStream->write((uint8 *)&currentReadData.getReference(0), currentReadData.size());
            delete outFileStream;
            statusLabel->setText(T("Saved ") + defaultPathFile.getFullPathName(), true);
        }
    }

    return true;
}

bool MiosFileBrowser::uploadFile(void)
{
    // restore default path
    String defaultPath(File::getSpecialLocation(File::userHomeDirectory).getFullPathName());
    PropertiesFile *propertiesFile = ApplicationProperties::getInstance()->getCommonSettings(true);
    if( propertiesFile ) {
        defaultPath = propertiesFile->getValue(T("defaultFilebrowserPath"), defaultPath);
    }
    File defaultPathFile(defaultPath);
    FileChooser myChooser(T("Upload File to Core"), defaultPathFile);
    if( !myChooser.browseForFileToOpen() ) {
        return false;
    } else {
        File inFile(myChooser.getResult());

        // store default path
        if( propertiesFile ) {
            propertiesFile->setValue(T("defaultFilebrowserPath"), inFile.getParentDirectory().getFullPathName());
        }

        FileInputStream *inFileStream = inFile.createInputStream();
        if( !inFileStream || inFileStream->isExhausted() || !inFileStream->getTotalLength() ) {
            AlertWindow::showMessageBox(AlertWindow::WarningIcon,
                                        T("The file ") + inFile.getFileName(),
                                        T("doesn't exist!"),
                                        String::empty);
        } else if( inFileStream->isExhausted() || !inFileStream->getTotalLength() ) {
            AlertWindow::showMessageBox(AlertWindow::WarningIcon,
                                        T("The file ") + inFile.getFileName(),
                                        T("is empty!"),
                                        String::empty);
        } else {
            uint64 size = inFileStream->getTotalLength();
            uint8 *buffer = (uint8 *)juce_malloc(size);
            currentWriteSize = inFileStream->read(buffer, size);
            currentWriteFile = getSelectedPath() + inFile.getFileName();
            //currentWriteData.resize(currentWriteSize); // doesn't exist in Juce 1.53
            Array<uint8> dummy(buffer, currentWriteSize);
            currentWriteData = dummy;
            juce_free(buffer);
            statusLabel->setText(T("Uploading ") + currentWriteFile + T(" (") + String(currentWriteSize) + T(" bytes)"), true);

            currentWriteInProgress = true;
            currentWriteError = false;
            currentWriteStartTime = Time::currentTimeMillis();
            sendCommand(T("write ") + currentWriteFile + T(" ") + String(currentWriteSize));
            startTimer(3000);
        }

        if( inFileStream )
            delete inFileStream;
    }

    return true;
}

//==============================================================================
void MiosFileBrowser::timerCallback()
{
    if( currentReadInProgress ) {
        if( currentReadError ) {
            statusLabel->setText(T("Invalid response from MIOS32 core during read operation!"), true);
        } else {
            statusLabel->setText(T("No response from MIOS32 core during read operation!"), true);
        }
    } else {
        statusLabel->setText(T("No response from MIOS32 core!"), true);
    }
}

//==============================================================================
void MiosFileBrowser::sendCommand(const String& command)
{
    Array<uint8> dataArray = SysexHelper::createMios32DebugMessage(miosStudio->uploadHandler->getDeviceId());
    dataArray.add(0x01); // filebrowser string
    for(int i=0; i<command.length(); ++i)
        dataArray.add(command[i] & 0x7f);
    dataArray.add('\n');
    dataArray.add(0xf7);
    MidiMessage message = SysexHelper::createMidiMessage(dataArray);
    miosStudio->sendMidiMessage(message);
    startTimer(3000);
}

//==============================================================================
void MiosFileBrowser::receiveCommand(const String& command)
{
    String statusMessage;

    stopTimer(); // will be restarted if required

    if( command.length() ) {
        switch( command[0] ) {

        ////////////////////////////////////////////////////////////////////
        case '?': {
            statusMessage = String(T("Command not supported by MIOS32 application - please check if a firmware update is available!"));
        } break;

        ////////////////////////////////////////////////////////////////////
        case 'D': {
            if( command[1] == '!' ) {
                statusMessage = String(T("SD Card not mounted!"));
            } else if( command[1] == '-' ) {
                statusMessage = String(T("Failed to access directory!"));
            } else {
                statusMessage = String(T("Received directory structure."));

                int posStartName = 2;
                for(int pos=2; pos<command.length(); ++pos) {
                    if( command[pos] == 'F' || command[pos] == 'D' ) {
                        for(++pos; pos < command.length() && command[pos] != ','; ++pos);
                        String fileName(command.substring(posStartName, pos));
                        //std::cout << "XXX " << fileName << std::endl;
                        if( fileName[0] == 'F' ) {
                            currentDirItem->createChild(currentDirPath, fileName.substring(1), false);
                        } else if( fileName[0] == 'D' ) {
                            currentDirFetchItems.add(currentDirItem->createChild(currentDirPath, fileName.substring(1), true));
                        } else {
                            std::cout << "INVALID Response: " << currentDirPath << fileName << std::endl;
                            statusMessage = String(T("Invalid Response!"));
                            break;
                        }
                        posStartName = pos+1;
                    } else {
                        statusMessage = String(T("Invalid Response!"));
                        break;
                    }
                }
            }

            // do we have to fetch some additional subdirectories?
            if( currentDirFetchItems.size() ) {
                currentDirItem = currentDirFetchItems.remove(0);
                if( currentDirItem->parentPath.compare(T("/")) == 0 )
                    currentDirPath = currentDirItem->parentPath + currentDirItem->name;
                else
                    currentDirPath = currentDirItem->parentPath + T("/") + currentDirItem->name;
                //std::cout << "Fetch " << currentDirPath << std::endl;
                sendCommand(T("dir ") + currentDirPath);
            } else {
                updateTreeView(true);
            }
        } break;

        ////////////////////////////////////////////////////////////////////
        case 'R': {
            if( command[1] == '!' ) {
                statusMessage = String(T("SD Card not mounted!"));
            } else if( command[1] == '-' ) {
                statusMessage = String(T("Failed to access " + currentReadFile + "!"));
            } else {
                currentReadSize = (command.substring(1)).getIntValue();
                currentReadData.clear();
                if( currentReadSize ) {
                    statusMessage = String(T("Receiving ") + currentReadFile + T(" with ") + String(currentReadSize) + T(" bytes."));
                    currentReadInProgress = true;
                    currentReadError = false;
                    currentReadStartTime = Time::currentTimeMillis();
                    startTimer(3000);
                } else {
                    statusMessage = String(currentReadFile + T(" is empty!"));
                }
            }
        } break;

        ////////////////////////////////////////////////////////////////////
        case 'r': {
            if( !currentReadInProgress ) {
                statusMessage = String(T("There is a read operation in progress - please wait!"));
            } else {
                String strAddress = command.substring(1, 9);
                String strPayload = command.substring(10);

                unsigned address = strAddress.getHexValue32();

                if( address >= currentReadSize ) {
                    statusMessage = String(currentReadFile + T(" received invalid payload!"));
                    currentReadError = true;
                } else {
                    for(int pos=0; pos<strPayload.length(); pos+=2) {
                        uint8 b = (strPayload.substring(pos, pos+2)).getHexValue32();
                        currentReadData.set(address + (pos/2), b);
                    }

                    unsigned receivedSize = currentReadData.size();
                    uint32 currentReadFinished = Time::currentTimeMillis();
                    float downloadTime = (float)(currentReadFinished-currentReadStartTime) / 1000.0;
                    float dataRate = ((float)receivedSize/1000.0) / downloadTime;
                    if( receivedSize >= currentReadSize ) {
                        statusMessage = String(T("Download of ") + currentReadFile +
                                               T(" (") + String(receivedSize) + T(" bytes) completed in ") +
                                               String::formatted(T("%2.1fs (%2.1f kb/s)"), downloadTime, dataRate));
                        currentReadInProgress = false;

                        statusLabel->setText(statusMessage, true);
                        storeDownloadedFile();
                        statusMessage = String::empty; // status has been updated by storeDownloadedFile()

                        enableFileButtons();
                    } else {
                        statusMessage = String(T("Downloading ") + currentReadFile + T(": ") +
                                               String(receivedSize) + T(" bytes received") +
                                               String::formatted(T(" (%d%%, %2.1f kb/s)"),
                                                                 (int)(100.0*(float)receivedSize/(float)currentReadSize),
                                                                 dataRate));
                        startTimer(3000);
                    }
                }
            }
        } break;

        ////////////////////////////////////////////////////////////////////
        case 'W': {
            if( !currentWriteInProgress ) {
                statusMessage = String(T("There is a write operation in progress - please wait!"));
            } else if( command[1] == '!' ) {
                statusMessage = String(T("SD Card not mounted!"));
            } else if( command[1] == '-' ) {
                statusMessage = String(T("Failed to access " + currentWriteFile + "!"));
            } else if( command[1] == '~' ) {
                statusMessage = String(T("FATAL: invalid parameters for write operation!"));
            } else if( command[1] == '#' ) {
                currentWriteInProgress = false;

                uint32 currentWriteFinished = Time::currentTimeMillis();
                float downloadTime = (float)(currentWriteFinished-currentWriteStartTime) / 1000.0;
                float dataRate = ((float)currentWriteSize/1000.0) / downloadTime;

                statusMessage = String(T("Upload of ") + currentWriteFile +
                                       T(" (") + String(currentWriteSize) + T(" bytes) completed in ") +
                                       String::formatted(T("%2.1fs (%2.1f kb/s)"), downloadTime, dataRate));

                // emulate "update" button
                buttonClicked(updateButton);
            } else {
                unsigned addressOffset = command.substring(1).getHexValue32();

                String writeCommand(String::formatted(T("writedata %08X "), addressOffset));
                for(int i=0; i<32 && (i+addressOffset)<currentWriteSize; ++i) {
                    writeCommand += String::formatted(T("%02X"), currentWriteData[addressOffset + i]);
                }
                sendCommand(writeCommand);

                uint32 currentWriteFinished = Time::currentTimeMillis();
                float downloadTime = (float)(currentWriteFinished-currentWriteStartTime) / 1000.0;
                float dataRate = ((float)addressOffset/1000.0) / downloadTime;

                statusMessage = String(T("Uploading ") + currentWriteFile + T(": ") +
                                       String(addressOffset) + T(" bytes transmitted") +
                                       String::formatted(T(" (%d%%, %2.1f kb/s)"),
                                                         (int)(100.0*(float)addressOffset/(float)currentWriteSize),
                                                         dataRate));
                startTimer(3000);
            }
        } break;


        ////////////////////////////////////////////////////////////////////
        case 'M': {
            if( command[1] == '!' ) {
                statusMessage = String(T("SD Card not mounted!"));
            } else if( command[1] == '-' ) {
                statusMessage = String(T("Failed to create directory!"));
            } else if( command[1] == '#' ) {
                statusMessage = String(T("Directory has been created!"));
                // emulate "update" button
                buttonClicked(updateButton);
            } else {
                statusMessage = String(T("Unsupported response from mkdir command!"));
            }
        } break;


        ////////////////////////////////////////////////////////////////////
        case 'X': {
            if( command[1] == '!' ) {
                statusMessage = String(T("SD Card not mounted!"));
            } else if( command[1] == '-' ) {
                statusMessage = String(T("Failed to delete file!"));
            } else if( command[1] == '#' ) {
                statusMessage = String(T("File has been removed!"));
                // emulate "update" button
                buttonClicked(updateButton);
            } else {
                statusMessage = String(T("Unsupported response from del command!"));
            }
        } break;


        ////////////////////////////////////////////////////////////////////
        default:
            statusMessage = String(T("Received unsupported Filebrowser Command! Please update your MIOS Studio installation!"));
        }
    }

    if( statusMessage.length() ) {
        statusLabel->setText(statusMessage, true);
    }
}

//==============================================================================
void MiosFileBrowser::handleIncomingMidiMessage(const MidiMessage& message, uint8 runningStatus)
{
    uint8 *data = message.getRawData();
    uint32 size = message.getRawDataSize();
    int messageOffset = 0;

    bool messageReceived = false;
    if( runningStatus == 0xf0 &&
        SysexHelper::isValidMios32DebugMessage(data, size, -1) &&
        data[7] == 0x41 ) {
            messageOffset = 8;
            messageReceived = true;
    } else if( runningStatus == 0xf0 &&
        SysexHelper::isValidMios32Error(data, size, -1) &&
        data[7] == 0x10 ) {
        stopTimer();
        statusLabel->setText(T("Filebrowser access not implemented by this application!"), true);
    }

    if( messageReceived ) {
        String command;

        for(int i=messageOffset; i<size; ++i) {
            if( data[i] < 0x80 ) {
                if( data[i] != '\n' || size < (i+1) )
                    command += String::formatted(T("%c"), data[i] & 0x7f);
            }
        }
        receiveCommand(command);
    }
}
