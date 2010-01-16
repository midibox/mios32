#!/bin/sh
/bin/rm -rf ~/Library/Audio/Plug-Ins/Components/MIDIboxSID.component
/bin/cp -r build/Debug/MIDIboxSID.component ~/Library/Audio/Plug-Ins/Components
auval -v aumu msid MBTK
