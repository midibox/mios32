/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
// $Id$
/*
 * Text Editor variant which allows to edit hex bytes
 *
 * ==========================================================================
 *
 *  Copyright (C) 2010 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#include "HexTextEditor.h"


//==============================================================================
HexTextEditor::HexTextEditor(Label *_statusLabel)
    : TextEditor(String::empty)
    , statusLabel(_statusLabel)
    , bufferedText(String::empty)
{
    setMultiLine(true);
    setReturnKeyStartsNewLine(true);
    setReadOnly(false);
    setScrollbarsShown(true);
    setCaretVisible(true);
    setPopupMenuEnabled(true);
    setInputRestrictions(1000000, T("0123456789ABCDEFabcdef \n"));
#if JUCE_MAJOR_VERSION==1 && JUCE_MINOR_VERSION<51
#ifdef JUCE_WIN32
    setFont(Font(Typeface::defaultTypefaceNameMono, 10.0, 0));
#else
    setFont(Font(Typeface::defaultTypefaceNameMono, 13.0, 0));
#endif
#else
#ifdef JUCE_WIN32
    setFont(Font(Font::getDefaultMonospacedFontName(), 10.0, 0));
#else
    setFont(Font(Font::getDefaultMonospacedFontName(), 13.0, 0));
#endif
#endif
	addListener(this);

    setSize(600, 200);
}

HexTextEditor::~HexTextEditor()
{
}

//==============================================================================
void HexTextEditor::clear()
{
    bufferedText = String::empty;
    TextEditor::clear();
}

//==============================================================================
void HexTextEditor::textEditorTextChanged(TextEditor &editor)
{
    String hexStr = getText();
    int size = hexStr.length();
#if JUCE_MAJOR_VERSION==1 && JUCE_MINOR_VERSION<51
	const char *strBuffer = (const char *)hexStr; // for *much* faster access! The String[pos] handling of Juce should be optimized!
#else
	const char *strBuffer = hexStr.toCString(); // for *much* faster access! The String[pos] handling of Juce should be optimized!
#endif

    int numBytes = 0;
    int charCounter = 0;
    bool invalidBytesFound = false;

    // we assume that only hex digits are entered (ensured via setInputRestrictions())
    for(int pos=0; pos<size; ++pos) {
        if( strBuffer[pos] == ' ' || strBuffer[pos] == '\n' ) {
            if( charCounter > 0 )
                ++numBytes;
            charCounter = 0;
        } else {
            if( ++charCounter > 2 )
                invalidBytesFound = true;
        }
    }
    if( charCounter > 0 )
        ++numBytes;

    if( invalidBytesFound )
        statusLabel->setText(T("Invalid Syntax!"), true);
    else if( numBytes == 0 )
        statusLabel->setText(String::empty, true);
    else if( numBytes == 1 )
        statusLabel->setText(T("1 byte"), true);
    else
        statusLabel->setText(String(numBytes) + T(" bytes"), true);
}

void HexTextEditor::textEditorReturnKeyPressed(TextEditor &editor)
{
}

void HexTextEditor::textEditorEscapeKeyPressed(TextEditor &editor)
{
}

void HexTextEditor::textEditorFocusLost(TextEditor &editor)
{
}

//==============================================================================
void HexTextEditor::setBinary(uint8 *buffer, const int &size)
{
    bufferedText = String::empty;

    clear();
    int64 lineBegin = 0;
    for(int64 pos=0; pos<size; ++pos) {
        if( buffer[pos] == 0xf7 || pos == (size-1) ) {
            if( bufferedText.length() > 0 )
                bufferedText += T("\n");
            bufferedText += String::toHexString(&buffer[lineBegin], pos-lineBegin+1);
            lineBegin = pos+1;
        }
    }

    setText(bufferedText, true);
}


//==============================================================================
void HexTextEditor::addBinary(uint8 *buffer, const int &size)
{
    if( size > 0 ) {
#if 0
        String hexStr = String::toHexString(buffer, size);
        if( !isEmpty() )
            insertTextAtCursor(T("\n"));
        insertTextAtCursor(hexStr);
#else
        // works faster! Will be removed once the Juce TextEditor has been optimized
        if( bufferedText != String::empty )
            bufferedText += T("\n");
        bufferedText += String::toHexString(buffer, size);
        setText(bufferedText);
#endif
    }
}

//==============================================================================
Array<uint8> HexTextEditor::getBinary(void)
{
    String hexStr = getText();
    int size = hexStr.length();

#if JUCE_MAJOR_VERSION==1 && JUCE_MINOR_VERSION<51
	const char *strBuffer = (const char *)hexStr; // for *much* faster access! The String[pos] handling of Juce should be optimized!
#else
	const char *strBuffer = hexStr.toCString(); // for *much* faster access! The String[pos] handling of Juce should be optimized!
#endif

    Array<uint8> retArray;
    int charCounter = 0;
    uint8 b = 0;

    // we assume that only hex digits are entered (ensured via setInputRestrictions())
    for(int pos=0; pos<size; ++pos) {
        char c = strBuffer[pos];
        if( c == ' ' || c == '\n' ) {
            if( charCounter > 0 )
                retArray.add(b);
            charCounter = 0;
        } else {
            uint8 nibble = 0;
            if( c >= '0' && c <= '9' )
                nibble = c - '0';
            else if( c >= 'A' && c <= 'F' )
                nibble = c - 'A' + 10;
            else if( c >= 'a' && c <= 'f' )
                nibble = c - 'a' + 10;

            if( charCounter == 0 )
                b = nibble << 4;
            else if( charCounter == 1 )
                b |= nibble;
            else {
                retArray.clear();
                return retArray;
            }
            ++charCounter;
        }
    }

    if( charCounter > 0 )
        retArray.add(b);

    return retArray;
}
