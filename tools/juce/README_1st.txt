MIOS Studio is using a predefined Juce version which is part of the repository.

Please unpack the library with:
unzip unpack_me.zip



MacOS users:

We prefer to build MIOS Studio for MacOS 10.8, so that people who haven't
upgraded their Mac to newer OS versions can run the tool.

Compilation will fail if you are trying newer SDKs (e.g. 10.13) due
to incompatibilities.

The 10.8 SDK isn't included into Xcode anymore, but can be easily
installed the following way:


1) close Xcode

2) download the SDK from http://www.midibox.org/macos_support/MacOSX10.8.sdk.tgz

3) if downloaded via Safari, the file will probably unzipped and renamed to MacOSX10.8.sdk.tar
   Unpack the tar file with:
   cd ~/Downloads
   tar xfv MacOSX10.8.sdk.tar

4) tell Xcode to accept older SDK versions with:
   sudo /usr/libexec/PlistBuddy -c "Set :MinimumSDKVersion 10.8" /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Info.plist

5) open Xcode - MIOS Studio should now build fine.
   If you did changes in the build setup which won't work, you could revert to the original version
   provided by me with: svn revert tools/mios_studio/Builds/MacOSX/MIOS_Studio.xcodeproj/project.pbxproj


