Currently prepared for Juce 5.4.3 which can be downloaded from
https://github.com/WeAreROLI/JUCE/releases

MacOS users: unpack it and move it to ~/JUCE (your home directory)
Linux users: unpack it and move it to ~/JUCE (your home directory)
Windows users: unpack it and move it to C:\JUCE

Notes:
- MacOS: we build for 10.8 to ensure that users with older MacOS versions can use the tool.
Build will fail due to an unknown enum in juce_mac_CoreAudioLayouts.h (kAudioChannelLayoutTag_AAC_7_1_C)
Just comment out this line, thereafter build should pass.

Dependencies
------------

Here are (possibly incomplete) lists of required packages to build MIOSStudio:

Ubuntu 18.10 | libasound2-dev libcurl4-gnutls-dev libfreetype6-dev libwebkit2gtk-4.0-dev
