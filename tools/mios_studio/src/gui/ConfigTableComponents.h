/* -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*- */
// $Id$
/*
 * Various Config Table Components and the appr. controller
 *
 * ==========================================================================
 *
 *  Copyright (C) 2010 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _CONFIG_TABLE_COMPONENTS_H
#define _CONFIG_TABLE_COMPONENTS_H

#include "../includes.h"


// must be inherited by all tables which own table components
class ConfigTableController
{
public:
    //==============================================================================
    ConfigTableController() {};
    ~ConfigTableController() {};

    //==============================================================================
    virtual int getTableValue(const int rowNumber, const int columnId) = 0;
    virtual void setTableValue(const int rowNumber, const int columnId, const int newValue) = 0;
    virtual String getTableString(const int rowNumber, const int columnId) = 0;
    virtual void setTableString(const int rowNumber, const int columnId, const String newString) = 0;
};


//==============================================================================
//==============================================================================
//==============================================================================

class ConfigTableComboBox
    : public Component
    , public ComboBoxListener
{
public:
    ConfigTableComboBox(ConfigTableController& _owner);
    ~ConfigTableComboBox();

    void resized();
    void setRowAndColumn(const int newRow, const int newColumn);

    void comboBoxChanged(ComboBox* comboBoxThatHasChanged);

    void addItem(const String &newItemText, const int newItemId);

private:
    ConfigTableController& owner;
    ComboBox* comboBox;
    int row, columnId;
};



//==============================================================================
//==============================================================================
//==============================================================================
class ConfigTableSlider
    : public Component
    , public SliderListener
{
public:
    ConfigTableSlider(ConfigTableController& _owner);
    ~ConfigTableSlider();

    void resized();

    void setRowAndColumn(const int newRow, const int newColumn);

    void sliderValueChanged(Slider *slider);

    void setRange(const double newMinimum, const double newMaximum);

private:
    ConfigTableController& owner;
    Slider* slider;
    int row, columnId;
};


//==============================================================================
//==============================================================================
//==============================================================================
class ConfigTableToggleButton
    : public Component
    , public ButtonListener
{
public:
    ConfigTableToggleButton(ConfigTableController& _owner);
    ~ConfigTableToggleButton();

    void resized();

    void setRowAndColumn(const int newRow, const int newColumn);

    void buttonClicked(Button *button);

private:
    ConfigTableController& owner;
    ToggleButton* toggleButton;
    int row, columnId;
};


//==============================================================================
//==============================================================================
//==============================================================================
class ConfigTableLabel
    : public Component
    , public LabelListener
{
public:
    ConfigTableLabel(ConfigTableController& _owner);
    ~ConfigTableLabel();

    void resized();

    void setRowAndColumn(const int newRow, const int newColumn);

    //==============================================================================
    void labelTextChanged(Label *labelThatHasBeenChanged);

private:
    Label* label;
    ConfigTableController& owner;
    int row, columnId;
};


#endif /* _CONFIG_TABLE_COMPONENTS_H */
