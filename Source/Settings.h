// Settings.h — per-user preferences shared by every Posy on the machine:
// tooltips on or off, and the chord sets you've starred.
//
//   macOS:   ~/Library/Application Support/Posy/Posy.settings
//   Windows: %APPDATA%\Posy\Posy.settings
//   Linux:   ~/.config/Posy/Posy.settings
//
// Message thread only.
#pragma once

#include <juce_data_structures/juce_data_structures.h>
#include <juce_events/juce_events.h>
#include <set>

namespace posy
{
struct SettingsStore
{
    SettingsStore();
    juce::PropertiesFile props;
};

class Settings
{
public:
    bool tooltipsEnabled() const;          // default: on
    void setTooltipsEnabled (bool);

    std::set<int> starred() const;         // chord set numbers, 1-based
    bool isStarred (int setNumber) const;
    void setStarred (int setNumber, bool);

    static juce::File file();

private:
    juce::SharedResourcePointer<SettingsStore> store;
};
} // namespace posy
