/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
// $Id$
/*
 * MIDIO128 Tool Window
 *
 * ==========================================================================
 *
 *  Copyright (C) 2010 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#include "ConfigTableComponents.h"


ConfigTableComboBox::ConfigTableComboBox(ConfigTableController& _owner)
    : owner(_owner)
{
    addAndMakeVisible(comboBox = new ComboBox(String::empty));
    comboBox->addListener(this);
    comboBox->setWantsKeyboardFocus(true);
}

ConfigTableComboBox::~ConfigTableComboBox()
{
    deleteAllChildren();
}

void ConfigTableComboBox::resized()
{
    comboBox->setBoundsInset(BorderSize(2));
}

void ConfigTableComboBox::setRowAndColumn(const int newRow, const int newColumn)
{
    row = newRow;
    columnId = newColumn;
    comboBox->setSelectedId(owner.getTableValue(row, columnId), true);
}

void ConfigTableComboBox::comboBoxChanged(ComboBox* comboBoxThatHasChanged)
{
    owner.setTableValue(row, columnId, comboBox->getSelectedId());
}

void ConfigTableComboBox::addItem(const String &newItemText, const int newItemId)
{
    comboBox->addItem(newItemText, newItemId);
}


//==============================================================================
//==============================================================================
//==============================================================================
ConfigTableSlider::ConfigTableSlider(ConfigTableController& _owner)
    : owner(_owner)
{
    addAndMakeVisible(slider = new Slider(String::empty));
    slider->addListener(this);
    slider->setRange(0, 127, 1);
    slider->setSliderStyle(Slider::IncDecButtons);
    slider->setTextBoxStyle(Slider::TextBoxLeft, false, 80, 20);
    slider->setDoubleClickReturnValue(true, 0);
}

ConfigTableSlider::~ConfigTableSlider()
{
    deleteAllChildren();
}

void ConfigTableSlider::resized()
{
    slider->setBoundsInset(BorderSize(2));
}

void ConfigTableSlider::setRowAndColumn(const int newRow, const int newColumn)
{
    row = newRow;
    columnId = newColumn;
    slider->setValue(owner.getTableValue(row, columnId), true);
}

void ConfigTableSlider::sliderValueChanged(Slider *slider)
{
    owner.setTableValue(row, columnId, slider->getValue());
}

void ConfigTableSlider::setRange(const double newMinimum, const double newMaximum)
{
    slider->setRange(newMinimum, newMaximum, 1);
}


//==============================================================================
//==============================================================================
//==============================================================================
ConfigTableToggleButton::ConfigTableToggleButton(ConfigTableController& _owner)
    : owner(_owner)
{
    addAndMakeVisible(toggleButton = new ToggleButton(String::empty));
    toggleButton->addListener(this);
}

ConfigTableToggleButton::~ConfigTableToggleButton()
{
    deleteAllChildren();
}

void ConfigTableToggleButton::resized()
{
    toggleButton->setBoundsInset(BorderSize(2));
}

void ConfigTableToggleButton::setRowAndColumn(const int newRow, const int newColumn)
{
    row = newRow;
    columnId = newColumn;
    toggleButton->setToggleState(owner.getTableValue(row, columnId), true);
}

void ConfigTableToggleButton::buttonClicked(Button *button)
{
    owner.setTableValue(row, columnId, button->getToggleState());
}


//==============================================================================
//==============================================================================
//==============================================================================
ConfigTableLabel::ConfigTableLabel(ConfigTableController& _owner)
    : owner(_owner)
{
    addAndMakeVisible(label = new Label(String::empty));
    label->setEditable(true);
    label->addListener(this);
}

ConfigTableLabel::~ConfigTableLabel()
{
    deleteAllChildren();
}

void ConfigTableLabel::resized()
{
    label->setBoundsInset(BorderSize(2));
}

void ConfigTableLabel::setRowAndColumn(const int newRow, const int newColumn)
{
    row = newRow;
    columnId = newColumn;

    String content(owner.getTableString(row, columnId));

    label->setText(content, false);
    if( !content.compare(T("no name available")) ) {
        label->setEnabled(false);
        label->setEditable(false);
    } else {
        label->setEnabled(true);
        label->setEditable(true);
    }
}

void ConfigTableLabel::labelTextChanged(Label *labelThatHasBeenChanged)
{
    owner.setTableString(row, columnId, label->getText());
}
