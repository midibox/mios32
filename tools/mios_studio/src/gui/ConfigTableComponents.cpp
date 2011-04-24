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
    toggleButton->addButtonListener(this);
}

ConfigTableToggleButton::~ConfigTableToggleButton()
{
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
