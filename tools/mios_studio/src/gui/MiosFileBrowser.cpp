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

    String getUniqueName() const
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
        if( getOwnerView()->isEnabled() )
            browser->treeItemClicked(this);
    }

    void itemDoubleClicked (const MouseEvent& e)
    {
        TreeViewItem::itemDoubleClicked(e);
        if( getOwnerView()->isEnabled() )
            browser->treeItemDoubleClicked(this);
    }

    var getDragSourceDescription()
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
    , currentDirOpenStates(NULL)
    , transferSelectionCtr(0)
    , openTextEditorAfterRead(false)
    , openHexEditorAfterRead(false)
    , currentReadInProgress(false)
    , currentReadFileBrowserItem(NULL)
    , currentReadFileStream(NULL)
    , currentReadError(false)
    , currentWriteInProgress(false)
    , currentWriteError(false)
    , writeBlockCtrDefault(32) // send 32 blocks (=two 512 byte SD Card Sectors) at once to speed-up write operations
    , writeBlockSizeDefault(32) // send 32 bytes per block
{
    addAndMakeVisible(editLabel = new Label(T("Edit"), String::empty));
    editLabel->setJustificationType(Justification::left);
    editLabel->setText(String::empty, true);

    addAndMakeVisible(statusLabel = new Label(T("Status"), String::empty));
    statusLabel->setJustificationType(Justification::left);
    statusLabel->setText(T("Please connect to MIOS32 core by pressing the Update button!"), true);

    addAndMakeVisible(updateButton = new TextButton(T("Update")));
    updateButton->addListener(this);

    addAndMakeVisible(uploadButton = new TextButton(T("Upload")));
    uploadButton->addListener(this);

    addAndMakeVisible(downloadButton = new TextButton(T("Download")));
    downloadButton->addListener(this);

    addAndMakeVisible(editTextButton = new TextButton(T("Edit Text")));
    editTextButton->addListener(this);

    addAndMakeVisible(editHexButton = new TextButton(T("Edit Hex")));
    editHexButton->addListener(this);

    addAndMakeVisible(createDirButton = new TextButton(T("Create Dir")));
    createDirButton->addListener(this);

    addAndMakeVisible(createFileButton = new TextButton(T("Create File")));
    createFileButton->addListener(this);

    addAndMakeVisible(removeButton = new TextButton(T("Remove Button")));
    removeButton->setButtonText(T("Remove"));
    removeButton->addListener(this);

    addAndMakeVisible(cancelButton = new TextButton(T("Cancel")));
    cancelButton->addListener(this);
    cancelButton->setEnabled(false);

    addAndMakeVisible(saveButton = new TextButton(T("Save")));
    saveButton->addListener(this);
    saveButton->setEnabled(false);

    addAndMakeVisible(treeView = new TreeView(T("SD Card")));
    treeView->setMultiSelectEnabled(true);

    addAndMakeVisible(hexEditor = new HexTextEditor(statusLabel));
    hexEditor->setReadOnly(true);

    addAndMakeVisible(textEditor = new TextEditor(String::empty));
    textEditor->setMultiLine(true);
    textEditor->setReturnKeyStartsNewLine(true);
    textEditor->setReadOnly(false);
    textEditor->setScrollbarsShown(true);
    textEditor->setCaretVisible(true);
    textEditor->setPopupMenuEnabled(true);
    textEditor->setReadOnly(true);
    textEditor->setVisible(false); // either hex or text editor visible
#if JUCE_MAJOR_VERSION==1 && JUCE_MINOR_VERSION<51
#if defined(JUCE_WIN32)
    textEditor->setFont(Font(Typeface::defaultTypefaceNameMono, 10.0, 0));
#else
    textEditor->setFont(Font(Typeface::defaultTypefaceNameMono, 13.0, 0));
#endif
#else
#if defined(JUCE_WIN32)
    textEditor->setFont(Font(Font::getDefaultMonospacedFontName(), 10.0, 0));
#else
    textEditor->setFont(Font(Font::getDefaultMonospacedFontName(), 13.0, 0));
#endif
#endif
    textEditor->addListener(this);

    rootFileItem = new MiosFileBrowserFileItem(T(""), T("/"), true);
    currentDirItem = rootFileItem;
    updateTreeView(false);

    resizeLimits.setSizeLimits(500, 320, 2048, 2048);
    addAndMakeVisible(resizer = new ResizableCornerComponent(this, &resizeLimits));

    setSize(950, 500);
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
 
    updateButton->setBounds    (10 , sendButtonY + 0*32 + 0*16, sendButtonWidth, 24);
    downloadButton->setBounds  (10 , sendButtonY + 1*32 + 1*16, sendButtonWidth, 24);
    uploadButton->setBounds    (10 , sendButtonY + 2*32 + 1*16, sendButtonWidth, 24);
    createDirButton->setBounds (10 , sendButtonY + 3*32 + 3*16, sendButtonWidth, 24);
    createFileButton->setBounds(10 , sendButtonY + 4*32 + 3*16, sendButtonWidth, 24);
    removeButton->setBounds    (10 , sendButtonY + 5*32 + 4*16, sendButtonWidth, 24);

    treeView->setBounds        (10 + 80, 16, 210, getHeight()-32-24);

    hexEditor->setBounds       (10+80+220, 48, getWidth()-10-80-220-10, getHeight()-32-32-24);
    textEditor->setBounds      (10+80+220, 48, getWidth()-10-80-220-10, getHeight()-32-32-24);

    editTextButton->setBounds  (10+80+220 + 0*(sendButtonWidth+10), sendButtonY, sendButtonWidth, 24);
    editHexButton->setBounds   (10+80+220 + 1*(sendButtonWidth+10), sendButtonY, sendButtonWidth, 24);
    cancelButton->setBounds    (getWidth()-10-sendButtonWidth, sendButtonY, sendButtonWidth, 24);
    saveButton->setBounds      (getWidth()-10-2*sendButtonWidth-10, sendButtonY, sendButtonWidth, 24);

    unsigned editLabelLeft  = editHexButton->getX()+editHexButton->getWidth() + 10;
    unsigned editLabelRight = saveButton->getX() - 10;
    editLabel->setBounds       (editLabelLeft, sendButtonY, editLabelRight-editLabelLeft, 24);

    statusLabel->setBounds(10, getHeight()-24, getWidth()-20, 24);
    resizer->setBounds(getWidth()-16, getHeight()-16, 16, 16);
}

//==============================================================================
void MiosFileBrowser::buttonClicked(Button* buttonThatWasClicked)
{
    if( buttonThatWasClicked == updateButton ) {
        // ensure that we see the first position of the treeview
        treeView->clearSelectedItems();
        treeView->scrollToKeepItemVisible(rootItem);
        // change label
        statusLabel->setText(T(""), true);
        // request update
        requestUpdateTreeView();
    } else if( buttonThatWasClicked == downloadButton || 
               buttonThatWasClicked == editTextButton ||
               buttonThatWasClicked == editHexButton ) {

        // extra: if data is edited, switch between hex/text view
        if( saveButton->isEnabled() ) {
            // ensure that no tree item is selected to avoid confusion (selection is possible even if treeView is disabled!)
            if( currentReadFileBrowserItem ) {
                currentReadFileBrowserItem->setSelected(true, true);
                treeView->scrollToKeepItemVisible(currentReadFileBrowserItem);
            }

            if( buttonThatWasClicked == editTextButton && hexEditor->isVisible() ) {
                openTextEditor(hexEditor->getBinary());
                statusLabel->setText(T("Editing ") + currentReadFileName + T(" in text mode."), true);
            } else if( buttonThatWasClicked == editHexButton && textEditor->isVisible() ) {
                if( textEditor->isVisible() && !textEditor->isReadOnly() ) {
                    // visible & read only mode means, that binary data is edited -> transfer to Hex Editor only in this case!
                    String tmpStr(textEditor->getText());
                    Array<uint8> data((uint8 *)tmpStr.toUTF8().getAddress(), tmpStr.length());
                    openHexEditor(data);
                } else {
                    // otherwise re-use data in hex editor
                    openHexEditor(hexEditor->getBinary());
                }
                statusLabel->setText(T("Editing ") + currentReadFileName + T(" in hex mode."), true);
            }
        } else {
            if( treeView->getNumSelectedItems() ) {
                openTextEditorAfterRead = buttonThatWasClicked == editTextButton;
                openHexEditorAfterRead = buttonThatWasClicked == editHexButton;
                transferSelectionCtr = 0;
                downloadFileSelection(transferSelectionCtr);
            }
        }
    } else if( buttonThatWasClicked == uploadButton ) {
        uploadFile();
    } else if( buttonThatWasClicked == createDirButton ) {
        createDir();
    } else if( buttonThatWasClicked == createFileButton ) {
        createFile();
    } else if( buttonThatWasClicked == removeButton ) {
        if( treeView->getNumSelectedItems() ) {
            transferSelectionCtr = 0;
            deleteFileSelection(transferSelectionCtr);
        }
    } else if( buttonThatWasClicked == cancelButton ||
               buttonThatWasClicked == saveButton ) {

        if( buttonThatWasClicked == cancelButton ) {
            openHexEditorAfterRead = false;
            openTextEditorAfterRead = false;
            enableFileButtons();
            hexEditor->clear();
            hexEditor->setReadOnly(true);
            textEditor->clear();
            textEditor->setReadOnly(true);
            editLabel->setText(String::empty, true);

            disableFileButtons(); // to disable also editor buttons
            enableFileButtons();
        } else if( buttonThatWasClicked == saveButton ) {
            if( hexEditor->isVisible() ) {
                // write back the read file
                uploadBuffer(currentReadFileName, hexEditor->getBinary());
            } else if( textEditor->isVisible() ) {
                // write back the read file
                String txt(textEditor->getText());
                Array<uint8> tmp((uint8 *)txt.toUTF8().getAddress(), txt.length());
                uploadBuffer(currentReadFileName, tmp);
            }
        }
    }
}


//==============================================================================
void MiosFileBrowser::textEditorTextChanged(TextEditor &editor)
{
    // unfortunately not updated on any cursor move...
#if 0
    int pos = editor.getCaretPosition();
    String txt(editor.getText());
    statusLabel->setText(String::formatted(T("(%d/%d)"), pos, txt.length()), true);
#endif
}

void MiosFileBrowser::textEditorReturnKeyPressed(TextEditor &editor)
{
}

void MiosFileBrowser::textEditorEscapeKeyPressed(TextEditor &editor)
{
}

void MiosFileBrowser::textEditorFocusLost(TextEditor &editor)
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
    updateButton->setEnabled(true);
    uploadButton->setEnabled(false);
    downloadButton->setEnabled(false);
    createDirButton->setEnabled(false);
    createFileButton->setEnabled(false);
    removeButton->setEnabled(false);

    // important: don't disable save/cancel button as long as an open editor has content
    // (in case of write error)
    if( !(textEditor->isVisible() && textEditor->getText().length()) &&
        !(hexEditor->isVisible() && hexEditor->getBinary().size()) ) {
        editTextButton->setEnabled(false);
        editHexButton->setEnabled(false);
        hexEditor->setReadOnly(true);
        textEditor->setReadOnly(true);
        saveButton->setEnabled(false);
        cancelButton->setEnabled(false);
    }
}

void MiosFileBrowser::enableFileButtons(void)
{
    treeView->setEnabled(true);

    updateButton->setEnabled(true);
    uploadButton->setEnabled(true);
    downloadButton->setEnabled(true);
    editTextButton->setEnabled(true);
    editHexButton->setEnabled(true);
    createDirButton->setEnabled(true);
    createFileButton->setEnabled(true);
    removeButton->setEnabled(true);
}

void MiosFileBrowser::enableDirButtons(void)
{
    updateButton->setEnabled(true);
    uploadButton->setEnabled(true);
    createDirButton->setEnabled(true);
    createFileButton->setEnabled(true); // no error: we want to create a file in the selected directory
    removeButton->setEnabled(true);
}

void MiosFileBrowser::enableEditorButtons(void)
{
    treeView->setEnabled(false);

    updateButton->setEnabled(false);
    editTextButton->setEnabled(true);
    editHexButton->setEnabled(true);
    cancelButton->setEnabled(true);
    saveButton->setEnabled(true);
}

//==============================================================================
String MiosFileBrowser::convertToString(const Array<uint8>& data, bool& hasBinaryData)
{
    String str;
    hasBinaryData = false;
    
    uint8 *ptr = (uint8 *)&data.getReference(0);
    for(int i=0; i<data.size(); ++i, ++ptr) {
        if( *ptr == 0 ) {
            hasBinaryData = true; // full conversion to string not possible
            str += "\\0"; // show \0 instead
        } else {
            str += (const wchar_t)*ptr;
        }
    }

    return str;
}

bool MiosFileBrowser::updateEditors(const Array<uint8>& data)
{
    // transfer data into both editors, make them read-only and invisible
    hexEditor->setReadOnly(false);
    hexEditor->clear();
    hexEditor->addBinary((uint8 *)&data.getReference(0), data.size());
    hexEditor->setReadOnly(true);
    hexEditor->setVisible(false);

    bool hasBinaryData = false;
    String tmpStr(convertToString(data, hasBinaryData));
    textEditor->setReadOnly(false);
    textEditor->clear();
    textEditor->setText(tmpStr, true);
    textEditor->setReadOnly(true);
    textEditor->setVisible(false);

    // ensure that no tree item is selected to avoid confusion (selection is possible even if treeView is disabled!)
    if( currentReadFileBrowserItem ) {
        currentReadFileBrowserItem->setSelected(true, true);
        treeView->scrollToKeepItemVisible(currentReadFileBrowserItem);
    }

    return hasBinaryData;
}

void MiosFileBrowser::openTextEditor(const Array<uint8>& data)
{
    // transfer data to both editors
    bool hasBinaryData = updateEditors(data);

    disableFileButtons();
    enableEditorButtons();

    // make text editor visible and enable read access if no binary data
    textEditor->setVisible(true);
    if( !hasBinaryData ) {
        textEditor->setReadOnly(false);
    } else {
        // TK: crashes Juce2 - it seems that we are not allowed to use an AlertWindow from the MiosStudio::timerCallback() thread
#if 0
        AlertWindow::showMessageBox(AlertWindow::WarningIcon,
                                    T("Found binary data!"),
                                    T("This file contains binary data, therefore it\nisn't possible modify it with the text editor!\nPlease use the hex editor instead!"),
                                    T("Ok"));
#endif
    }
    textEditor->setCaretVisible(true); // TK: required since Juce2 - I don't know why...
}

void MiosFileBrowser::openHexEditor(const Array<uint8>& data)
{
    // transfer data to both editors
    updateEditors(data);

    disableFileButtons();
    enableEditorButtons();

    hexEditor->setVisible(true);
    hexEditor->setReadOnly(false);

    hexEditor->setCaretVisible(true); // TK: required since Juce2 - I don't know why...
}

//==============================================================================
void MiosFileBrowser::requestUpdateTreeView(void)
{
    treeView->setEnabled(true);
    currentDirOpenStates = treeView->getOpennessState(true); // including scroll position

    if( rootFileItem )
        delete rootFileItem;
    rootFileItem = new MiosFileBrowserFileItem(T(""), T("/"), true);
    currentDirItem = rootFileItem;

    updateTreeView(false);

    disableFileButtons();
    currentDirFetchItems.clear();
    currentDirPath = T("/");
    sendCommand(T("dir ") + currentDirPath);
}

void MiosFileBrowser::updateTreeView(bool accessPossible)
{
    disableFileButtons();

    if( rootItem )
        treeView->deleteRootItem();

    if( rootFileItem ) {
        rootItem = new MiosFileBrowserItem(rootFileItem, this);
        rootItem->setOpen(true);
        treeView->setRootItem(rootItem);

        if( accessPossible ) {
            if( currentDirOpenStates ) {
                treeView->restoreOpennessState(*currentDirOpenStates, true);
                deleteAndZero(currentDirOpenStates);
            }

            uploadButton->setEnabled(true);
            createDirButton->setEnabled(true);
            createFileButton->setEnabled(true);
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
bool MiosFileBrowser::downloadFileSelection(unsigned selection)
{
    treeView->setEnabled(false);
    TreeViewItem* selectedItem = treeView->getSelectedItem(selection);
    currentReadFileBrowserItem = selectedItem;

    if( selectedItem ) {
        currentReadFileName = selectedItem->getUniqueName();

        if( openHexEditorAfterRead || openTextEditorAfterRead ) {
            disableFileButtons();
            sendCommand(T("read ") + currentReadFileName);
            return true;
        } else {
            // restore default path
            String defaultPath(File::getSpecialLocation(File::userHomeDirectory).getFullPathName());
            PropertiesFile *propertiesFile = MiosStudioProperties::getInstance()->getCommonSettings(true);
            if( propertiesFile ) {
                defaultPath = propertiesFile->getValue(T("defaultFilebrowserPath"), defaultPath);
            }

            String readFileName(currentReadFileName.substring(currentReadFileName.lastIndexOfChar('/')+1));
            File defaultPathFile(defaultPath + "/" + readFileName);
            FileChooser myChooser(T("Store ") + currentReadFileName, defaultPathFile);
            if( !myChooser.browseForFileToSave(true) ) {
                statusLabel->setText(T("Cancled save operation for ") + currentReadFileName, true);
            } else {
                currentReadFile = myChooser.getResult();

                // store default path
                if( propertiesFile ) {
                    propertiesFile->setValue(T("defaultFilebrowserPath"), currentReadFile.getParentDirectory().getFullPathName());
                }

                currentReadFileStream = NULL;
                currentReadFile.deleteFile();
                if( !(currentReadFileStream=currentReadFile.createOutputStream()) ||
                    currentReadFileStream->failedToOpen() ) {
                    AlertWindow::showMessageBox(AlertWindow::WarningIcon,
                                                String::empty,
                                                T("File cannot be created!"),
                                                String::empty);
                    statusLabel->setText(T("Failed to open ") + currentReadFile.getFullPathName(), true);
                } else {
                    disableFileButtons();
                    sendCommand(T("read ") + currentReadFileName);
                    return true;
                }
            }
        }
    }

    // operation not successfull (or no more files)
    treeView->setEnabled(true);
    enableFileButtons();

    return false;
}

bool MiosFileBrowser::downloadFinished(void)
{
    if( openHexEditorAfterRead ) {
        openHexEditor(currentReadData);
        statusLabel->setText(T("Editing ") + currentReadFileName, true);
        editLabel->setText(currentReadFileName, true);
    } else if( openTextEditorAfterRead ) {
        openTextEditor(currentReadData);
        statusLabel->setText(T("Editing ") + currentReadFileName, true);
        editLabel->setText(currentReadFileName, true);
    } else if( currentReadFileStream ) {
        currentReadFileStream->write((uint8 *)&currentReadData.getReference(0), currentReadData.size());
        delete currentReadFileStream;
        statusLabel->setText(T("Saved ") + currentReadFile.getFullPathName(), true);

        // try next file (if there is still another selection
        downloadFileSelection(++transferSelectionCtr);
    }

    return true;
}

//==============================================================================
bool MiosFileBrowser::deleteFileSelection(unsigned selection)
{
    treeView->setEnabled(false);
    TreeViewItem* selectedItem = treeView->getSelectedItem(selection);

    if( selectedItem ) {
        String fileName(selectedItem->getUniqueName());
        if( AlertWindow::showOkCancelBox(AlertWindow::WarningIcon,
                                         T("Removing ") + fileName,
                                         T("Do you really want to remove\n") + fileName + T("?"),
                                         T("Remove"),
                                         T("Cancel")) ) {
            disableFileButtons();
            sendCommand(T("del ") + fileName);
            return true;
        }
    }

    // operation finished
    requestUpdateTreeView();

    return false;
}

bool MiosFileBrowser::deleteFinished(void)
{
    statusLabel->setText(T("File has been removed!"), true);

    // try next file (if there is still another selection
    deleteFileSelection(++transferSelectionCtr);

    return true;
}

//==============================================================================
bool MiosFileBrowser::createDir(void)
{
    AlertWindow enterName(T("Creating new Directory"),
                          T("Please enter directory name:"),
                          AlertWindow::QuestionIcon);

    enterName.addButton(T("Create"), 1);
    enterName.addButton(T("Cancel"), 0);
    enterName.addTextEditor(T("Name"), String::empty);

    if( enterName.runModalLoop() ) {
        String name(enterName.getTextEditorContents(T("Name")));
        if( name.length() ) {
            if( name[0] == '/' )
                sendCommand(T("mkdir ") + name);
            else
                sendCommand(T("mkdir ") + getSelectedPath() + name);
        }        
    }

    return false;
}

bool MiosFileBrowser::createDirFinished(void)
{
    statusLabel->setText(T("Directory has been created!"), true);

    requestUpdateTreeView();

    return true;
}

//==============================================================================
bool MiosFileBrowser::createFile(void)
{
    AlertWindow enterName(T("Creating new File"),
                          T("Please enter filename:"),
                          AlertWindow::QuestionIcon);

    enterName.addButton(T("Create"), 1);
    enterName.addButton(T("Cancel"), 0);
    enterName.addTextEditor(T("Name"), String::empty);

    if( enterName.runModalLoop() ) {
        String name(enterName.getTextEditorContents(T("Name")));
        if( name.length() ) {
            Array<uint8> emptyBuffer;
            if( name[0] == '/' )
                return uploadBuffer(name, emptyBuffer);
            else
                return uploadBuffer(getSelectedPath() + name, emptyBuffer);
        }        
    }

    return false;
}

//==============================================================================
bool MiosFileBrowser::uploadFile(void)
{
    // restore default path
    String defaultPath(File::getSpecialLocation(File::userHomeDirectory).getFullPathName());
    PropertiesFile *propertiesFile = MiosStudioProperties::getInstance()->getCommonSettings(true);
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
        } else if( inFileStream->isExhausted() ) { // || !inFileStream->getTotalLength() -> disabled, we also want to handle zero-length files
            AlertWindow::showMessageBox(AlertWindow::WarningIcon,
                                        T("The file ") + inFile.getFileName(),
                                        T("can't be read!"),
                                        String::empty);
        } else {
            disableFileButtons();

            uint64 size = inFileStream->getTotalLength();
            uint8 *buffer = (uint8 *)juce_malloc(size);
            size = inFileStream->read(buffer, size);
            //currentWriteData.resize(currentWriteSize); // doesn't exist in Juce 1.53
            Array<uint8> tmp(buffer, size);
            juce_free(buffer);
            uploadBuffer(getSelectedPath() + inFile.getFileName(), tmp);
        }

        if( inFileStream )
            delete inFileStream;
    }

    return true;
}

bool MiosFileBrowser::uploadBuffer(String filename, const Array<uint8>& buffer)
{
    currentWriteFileName = filename;
    currentWriteData = buffer;
    currentWriteSize = buffer.size();

    statusLabel->setText(T("Uploading ") + currentWriteFileName + T(" (") + String(currentWriteSize) + T(" bytes)"), true);

    currentWriteInProgress = true;
    currentWriteError = false;
    currentWriteFirstBlockOffset = 0;
    currentWriteBlockCtr = writeBlockCtrDefault;
    currentWriteStartTime = Time::currentTimeMillis();
    sendCommand(T("write ") + currentWriteFileName + T(" ") + String(currentWriteSize));
    startTimer(5000);

    return true;
}

bool MiosFileBrowser::uploadFinished(void)
{
    currentWriteInProgress = false;
    String extraText;

    // finished edit operation?
    if( openHexEditorAfterRead || openTextEditorAfterRead ) {
#if 0
        openHexEditorAfterRead = false;
        openTextEditorAfterRead = false;
        hexEditor->clear();
        hexEditor->setReadOnly(true);
        textEditor->clear();
        textEditor->setReadOnly(true);
        editLabel->setText(String::empty, true);
#else
        extraText = T(" - you can continue editing; click CANCEL to close editor!");
        // don't close editor
#endif
    }

    uint32 currentWriteFinished = Time::currentTimeMillis();
    float downloadTime = (float)(currentWriteFinished-currentWriteStartTime) / 1000.0;
    float dataRate = ((float)currentWriteSize/1000.0) / downloadTime;


    statusLabel->setText(T("Upload of ") + currentWriteFileName +
                         T(" (") + String(currentWriteSize) + T(" bytes) completed in ") +
                         String::formatted(T("%2.1fs (%2.1f kb/s)"), downloadTime, dataRate) +
                         extraText, true);

#if 0 // not required
    requestUpdateTreeView();
#else
    if( !openHexEditorAfterRead && !openTextEditorAfterRead ) {
        enableFileButtons();
    }
#endif


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
    startTimer(5000);
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
                //statusMessage = String(T("Received directory structure."));

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
                statusMessage = String(T("Failed to access " + currentReadFileName + "!"));
            } else {
                currentReadSize = (command.substring(1)).getIntValue();
                currentReadData.clear();
                if( currentReadSize ) {
                    statusMessage = String(T("Receiving ") + currentReadFileName + T(" with ") + String(currentReadSize) + T(" bytes."));
                    currentReadInProgress = true;
                    currentReadError = false;
                    currentReadStartTime = Time::currentTimeMillis();
                    startTimer(5000);
                } else {
                    statusMessage = String(currentReadFileName + T(" is empty!"));
                    // ok, we accept this to edit zero-length files
                    // fake transfer:
                    currentReadInProgress = false;
                    currentReadData.clear();
                    statusLabel->setText(statusMessage, true);
                    downloadFinished();
                    statusMessage = String::empty; // status has been updated by downloadFinished()
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
                    statusMessage = String(currentReadFileName + T(" received invalid payload!"));
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
                        statusMessage = String(T("Download of ") + currentReadFileName +
                                               T(" (") + String(receivedSize) + T(" bytes) completed in ") +
                                               String::formatted(T("%2.1fs (%2.1f kb/s)"), downloadTime, dataRate));
                        currentReadInProgress = false;

                        statusLabel->setText(statusMessage, true);
                        downloadFinished();
                        statusMessage = String::empty; // status has been updated by downloadFinished()
                    } else {
                        statusMessage = String(T("Downloading ") + currentReadFileName + T(": ") +
                                               String(receivedSize) + T(" bytes received") +
                                               String::formatted(T(" (%d%%, %2.1f kb/s)"),
                                                                 (int)(100.0*(float)receivedSize/(float)currentReadSize),
                                                                 dataRate));
                        startTimer(5000);
                    }
                }
            }
        } break;

        ////////////////////////////////////////////////////////////////////
        case 'W': {
            if( currentWriteError ) {
                // ignore
            } else if( !currentWriteInProgress ) {
                statusMessage = String(T("There is a write operation in progress - please wait!"));
            } else if( command[1] == '!' ) {
                statusMessage = String(T("SD Card not mounted!"));
            } else if( command[1] == '-' ) {
                statusMessage = String(T("Failed to access " + currentWriteFileName + "!"));
            } else if( command[1] == '~' ) {
                statusMessage = String(T("FATAL: invalid parameters for write operation!"));
            } else if( command[1] == '#' ) {
                uploadFinished();
                statusMessage = String::empty; // status has been updated by uploadFinished()
            } else {
                unsigned addressOffset = command.substring(1).getHexValue32();

                if( currentWriteBlockCtr < writeBlockCtrDefault ) {
                    // check for valid response
                    unsigned expectedOffset = currentWriteFirstBlockOffset + (writeBlockSizeDefault*(currentWriteBlockCtr+1));
                    if( addressOffset != expectedOffset ) {
                        currentWriteError = true;
                        statusLabel->setText(String::formatted(T("ERROR: the application has requested file position 0x%08X, but filebrowser expected 0x%08X! Please check with TK!"), addressOffset, expectedOffset), true);
                    } else {
                        ++currentWriteBlockCtr; // block received
                    }                    
                }

                // new burst?
                if( currentWriteBlockCtr >= writeBlockCtrDefault ) {
                    currentWriteFirstBlockOffset = addressOffset;
                    currentWriteBlockCtr = 0;

                    for(unsigned block=0; block<writeBlockCtrDefault; ++block, addressOffset += writeBlockSizeDefault) {
                        String writeCommand(String::formatted(T("writedata %08X "), addressOffset));
                        for(int i=0; i<writeBlockSizeDefault && (i+addressOffset)<currentWriteSize; ++i) {
                            writeCommand += String::formatted(T("%02X"), currentWriteData[addressOffset + i]);
                        }
                        sendCommand(writeCommand);

                        if( (writeBlockSizeDefault+addressOffset) >= currentWriteSize )
                            break;
                    }

                    uint32 currentWriteFinished = Time::currentTimeMillis();
                    float downloadTime = (float)(currentWriteFinished-currentWriteStartTime) / 1000.0;
                    float dataRate = ((float)addressOffset/1000.0) / downloadTime;

                    statusMessage = String(T("Uploading ") + currentWriteFileName + T(": ") +
                                           String(addressOffset) + T(" bytes transmitted") +
                                           String::formatted(T(" (%d%%, %2.1f kb/s)"),
                                                             (int)(100.0*(float)addressOffset/(float)currentWriteSize),
                                                             dataRate));
                    startTimer(5000);
                }
            }
        } break;


        ////////////////////////////////////////////////////////////////////
        case 'M': {
            if( command[1] == '!' ) {
                statusMessage = String(T("SD Card not mounted!"));
            } else if( command[1] == '-' ) {
                statusMessage = String(T("Failed to create directory!"));
            } else if( command[1] == '#' ) {
                createDirFinished();
                statusMessage = String::empty; // status has been updated by createDirFinished()
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
                deleteFinished();
                statusMessage = String::empty; // status has been updated by deleteFinished()
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
    uint8 *data = (uint8 *)message.getRawData();
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
