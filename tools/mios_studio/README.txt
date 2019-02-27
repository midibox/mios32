Currently prepared for Juce 5.4.3 which can be downloaded from
https://github.com/WeAreROLI/JUCE/releases

Unpack it and rename it to "juce" in the upper folder (../juce)


MacOS Build only
Linux and Windows build setups will be adapted the next days


Notes:
- MacOS: we build for 10.8 to ensure that users with older MacOS versions can use the tool.
Build will fail due to an unknown enum in juce_mac_CoreAudioLayouts.h (kAudioChannelLayoutTag_AAC_7_1_C)
Just comment out this line, thereafter build should pass.

