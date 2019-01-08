#!/bin/sh
export component=build/Release/MIDIboxSID.component
/usr/libexec/PlistBuddy -c "Delete AudioComponents" "$component/Contents/Info.plist"
/bin/rm -rf ~/Library/Audio/Plug-Ins/Components/MIDIboxSID.component
/bin/cp -r $component ~/Library/Audio/Plug-Ins/Components
auval -v aumu MSID MBox
