#include "Settings.h"

namespace posy
{
namespace
{
juce::PropertiesFile::Options options()
{
    juce::PropertiesFile::Options o;
    o.applicationName         = "Posy";
    o.filenameSuffix          = ".settings";
   #if JUCE_LINUX
    o.folderName              = ".config/Posy";
   #else
    o.folderName              = "Posy";
   #endif
    o.osxLibrarySubFolder     = "Application Support";
    o.storageFormat           = juce::PropertiesFile::storeAsXML;
    o.millisecondsBeforeSaving = -1;   // every change is saved explicitly
    return o;
}

constexpr const char* kTooltips = "tooltips";
constexpr const char* kStarred  = "starred";
} // namespace

SettingsStore::SettingsStore() : props (options()) {}

juce::File Settings::file() { return options().getDefaultFile(); }

bool Settings::tooltipsEnabled() const
{
    return store->props.getBoolValue (kTooltips, true);
}

void Settings::setTooltipsEnabled (bool on)
{
    store->props.setValue (kTooltips, on);
    store->props.saveIfNeeded();
}

std::set<int> Settings::starred() const
{
    std::set<int> out;
    auto items = juce::StringArray::fromTokens (store->props.getValue (kStarred), ",", {});
    for (auto& s : items)
        if (const int n = s.getIntValue(); n > 0)
            out.insert (n);
    return out;
}

bool Settings::isStarred (int n) const
{
    return starred().count (n) > 0;
}

void Settings::setStarred (int n, bool on)
{
    auto s = starred();
    if (on) s.insert (n); else s.erase (n);
    juce::StringArray items;
    for (int v : s)
        items.add (juce::String (v));
    store->props.setValue (kStarred, items.joinIntoString (","));
    store->props.saveIfNeeded();
}
} // namespace posy
