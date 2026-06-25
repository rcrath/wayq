/*
  ==============================================================================
    AppPrefs.h
    WayQ — shared OS-level application preferences file.

    Single inline accessor; all callers in the same binary share the same
    ApplicationProperties instance (C++17 inline-function guarantee). Used by
    the top panel for lastFileChooserDir + perf-mode persistence only.

    (c) 2026 Rich Rath / Way.Net
  ==============================================================================
*/

#pragma once

#include <juce_data_structures/juce_data_structures.h>

inline juce::PropertiesFile& getAppPrefs()
{
    static juce::ApplicationProperties appProps;
    static bool init = false;
    if (! init)
    {
        juce::PropertiesFile::Options opts;
        opts.applicationName     = "WayQ";
        opts.filenameSuffix      = "settings";
        opts.osxLibrarySubFolder = "Application Support";
        appProps.setStorageParameters (opts);
        init = true;
    }
    return *appProps.getUserSettings();
}
