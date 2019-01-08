/*
    MIOS32 Emu
*/

#ifndef MIOSJUCEDIALOGS_H
#define MIOSJUCEDIALOGS_H



#include <mios32.h>


class MIDIDialog : public DialogWindow
{
public:
     void closeButtonPressed()
    {
        MIOS32_MIDI_Init(0);
    }
     
};


class MenuItems : public MenuBarModel
{
public:
    MenuItems() : MenuBarModel()
    {
    }
    ~MenuItems()
    {
    }
    
    
    const StringArray getMenuBarNames()
    {
        const tchar* const names[] = { T("MIOS32"), T("About"), 0 };

        return StringArray ((const tchar**) names);
    }
    
    const PopupMenu getMenuForIndex (int menuIndex,
                                     const String& menuName)
    {
        PopupMenu menu;

        if (menuIndex == 0)
        {
            menu.addItem (1, T("MIDI Settings"));
            menu.addSeparator();
            menu.addItem (2, T("Quit"));
        }
        else if (menuIndex == 1)
        {
            menu.addItem (1, T("About MIOSJUCE"));
        }

        return menu;
    }
    
    void menuItemSelected (int menuItemID,
                           int topLevelMenuIndex)
    {
        if (topLevelMenuIndex == 0)
        {
            if (menuItemID == 1)
            {
                AudioDeviceSelectorComponent midiSettingsComp (*midiDeviceManager,
                                        0, 0,
                                        0, 0,
                                        true,
                                        true,
                                        false,
                                        false);

                // ...and show it in a DialogWindow...
                midiSettingsComp.setSize (400, 200);

                MIDIDialog::showModalDialog (T("MIDI Settings"),
                                               &midiSettingsComp,
                                               NULL,
                                               Colours::silver,
                                               true);

            }
            else if (menuItemID == 2)
            {
                JUCEApplication::quit();
            }
        }
        else if (topLevelMenuIndex == 1)
        {
            bool visitsite = AlertWindow::showOkCancelBox(AlertWindow::InfoIcon,
								T("About MIOSJUCE"),
								T("by stryd_one\nand lucem"),
								T("OK"),
								T("www.midibox.org"));
		  if (visitsite = false) {
			 URL midibox(T("www.midibox.org"));
			 midibox.launchInDefaultBrowser();
		  }
        }
    }
};


#endif   // MIOSJUCEDIALOGS_H
