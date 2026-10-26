#include "Banks.h"

namespace posy
{
namespace
{
const juce::Identifier kChordTag { "Chord" };
}

bool CustomBank::isEmpty() const
{
    for (auto& c : chords)
        if (! c.notes.isEmpty())
            return false;
    return true;
}

juce::ValueTree CustomBank::toTree() const
{
    juce::ValueTree t (banks::tag);
    t.setProperty ("name", name, nullptr);
    t.setProperty ("version", 1, nullptr);
    for (int k = 0; k < kKeys; ++k)
    {
        const auto& c = chords[(size_t) k];
        juce::StringArray notes;
        for (int n : c.notes) notes.add (juce::String (n));
        juce::ValueTree ch (kChordTag);
        ch.setProperty ("key", k, nullptr);
        ch.setProperty ("name", c.name, nullptr);
        ch.setProperty ("notes", notes.joinIntoString (","), nullptr);
        if (c.named)
            ch.setProperty ("named", 1, nullptr);
        t.appendChild (ch, nullptr);
    }
    return t;
}

CustomBank CustomBank::fromTree (const juce::ValueTree& t)
{
    CustomBank b;
    b.name = t["name"].toString();
    for (auto ch : t)
    {
        if (! ch.hasType (kChordTag))
            continue;
        const int k = (int) ch["key"];
        if (! juce::isPositiveAndBelow (k, kKeys))
            continue;
        auto& c = b.chords[(size_t) k];
        c.name = ch["name"].toString();
        c.named = (int) ch.getProperty ("named", 0) != 0;
        c.notes.clear();
        for (auto& s : juce::StringArray::fromTokens (ch["notes"].toString(), ",", {}))
            if (s.trim().isNotEmpty())
                c.notes.addIfNotAlreadyThere (juce::jlimit (0, 127, s.getIntValue()));
        c.notes.sort();
        while (c.notes.size() > kMaxStored)
            c.notes.remove (c.notes.size() - 1);
    }
    return b;
}

namespace banks
{
juce::File folder()
{
    auto dir = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory);
   #if JUCE_MAC
    dir = dir.getChildFile ("Application Support");
   #elif JUCE_LINUX
    dir = juce::File::getSpecialLocation (juce::File::userHomeDirectory).getChildFile (".config");
   #endif
    return dir.getChildFile ("Posy").getChildFile ("Banks");
}

juce::Array<juce::File> list()
{
    auto files = folder().findChildFiles (juce::File::findFiles, false, juce::String ("*") + extension);
    std::sort (files.begin(), files.end(), [] (const juce::File& a, const juce::File& b)
    {
        return a.getFileNameWithoutExtension().compareNatural (b.getFileNameWithoutExtension()) < 0;
    });
    return files;
}

bool load (const juce::File& f, CustomBank& out)
{
    auto xml = juce::XmlDocument::parse (f);
    if (xml == nullptr)
        return false;
    auto tree = juce::ValueTree::fromXml (*xml);
    if (! tree.hasType (tag))
        return false;
    out = CustomBank::fromTree (tree);
    if (out.name.isEmpty())
        out.name = f.getFileNameWithoutExtension();
    out.file = f;
    return true;
}

bool save (const CustomBank& b, const juce::File& f)
{
    if (! f.getParentDirectory().createDirectory())
        return false;
    auto xml = b.toTree().createXml();
    return xml != nullptr && xml->writeTo (f);
}

juce::String sanitise (const juce::String& name)
{
    return juce::File::createLegalFileName (name.trim()).substring (0, 64).trim();
}

juce::File fileFor (const juce::String& name)
{
    return folder().getChildFile (sanitise (name) + extension);
}
} // namespace banks
} // namespace posy
