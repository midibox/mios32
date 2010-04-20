
/*
    This file includes the entire juce source tree via the amalgamated file.

    You could add the amalgamated file directly to your project, but doing it
    like this allows you to put your app's config settings in the
    juce_AppConfig.h file and have them applied to both the juce headers and
    the source code.
*/

#include "juce_AppConfig.h"

#if defined(WIN32) && (JUCE_MAJOR_VERSION==1 && JUCE_MINOR_VERSION==50)
#include "../../../../../juce/juce.h"
#else
#include "../../../../../juce/juce_amalgamated.cpp"
#endif
