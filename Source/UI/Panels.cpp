#include "Panels.h"

namespace posy::ui
{
using namespace juce;

namespace
{
const char* const kSharps[12] { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };

int letterPc (juce_wchar c)
{
    switch (c)
    {
        case 'C': return 0; case 'D': return 2; case 'E': return 4; case 'F': return 5;
        case 'G': return 7; case 'A': return 9; case 'B': return 11; default: return -1;
    }
}

// Rewrite each note name in a chord symbol (the root, and the bass after a
// slash) through `fn`, which gets the pitch class and returns its new name.
String mapRoots (const String& s, const std::function<String (int)>& fn)
{
    String out;
    const int n = s.length();
    for (int i = 0; i < n;)
    {
        const bool rootPos = i == 0 || s[i - 1] == '/';
        const int pc = rootPos ? letterPc (s[i]) : -1;
        if (pc >= 0)
        {
            int p = pc, len = 1;
            if (i + 1 < n && s[i + 1] == '#') { p += 1; len = 2; }
            else if (i + 1 < n && s[i + 1] == 'b') { p += 11; len = 2; }
            out << fn (p % 12);
            i += len;
        }
        else
        {
            out << String::charToString (s[i]);
            ++i;
        }
    }
    return out;
}

String shiftNote (int pc, int semis) { return kSharps[((pc + semis) % 12 + 12) % 12]; }

// Only chord symbols are transposed: "Am7", "F#/C", a list of notes like
// "C Db G". A name you typed ("Gospel lift") is left alone.
String transposeName (const String& name, int semis)
{
    if (((semis % 12) + 12) % 12 == 0 || name.isEmpty())
        return name;

    if (name.containsChar (' '))
    {
        auto tokens = StringArray::fromTokens (name, " ", "");
        for (auto& t : tokens)
        {
            const int pc = letterPc (t[0]);
            const bool note = pc >= 0 && (t.length() == 1 || (t.length() == 2 && (t[1] == '#' || t[1] == 'b')));
            if (! note)
                return name;
            t = shiftNote (pc + (t.length() == 2 ? (t[1] == '#' ? 1 : 11) : 0), semis);
        }
        return tokens.joinIntoString (" ");
    }

    if (letterPc (name[0]) < 0)
        return name;
    const juce_wchar second = name.length() > 1 ? name[1] : 0;
    if (CharacterFunctions::isLowerCase (second) && String ("bmads").indexOfChar (second) < 0)
        return name;
    return mapRoots (name, [semis] (int pc) { return shiftNote (pc, semis); });
}

String star (bool filled)
{
    return String::fromUTF8 (filled ? "\xe2\x98\x85" : "\xe2\x98\x86");
}

// Browser families: a set belongs if its genre contains any of the words.
struct Family { const char* label; std::vector<const char*> words; };
const std::vector<Family>& families()
{
    static const std::vector<Family> f {
        { "ALL", {} },
        { "MY SETS", {} },
        { "STARRED", {} },
        { "POP", { "Pop" } },
        { "ROCK", { "Rock" } },
        { "JAZZ", { "Jazz", "Bossa", "Blues" } },
        { "SOUL R&B", { "Soul", "Gospel", "R&B", "Funk", "Lofi", "Afro" } },
        { "HOUSE EDM", { "House", "Techno", "Trance", "EDM" } },
        { "SYNTH", { "Synth" } },
        { "CINEMA", { "Cinematic", "Film", "New Age", "Classical", "Modern" } },
        { "AMBIENT", { "Ambient", "Shoegaze", "Dream", "New Age" } },
        { "GAME", { "Game" } },
        { "WORLD", { "World", "Bossa", "Samba", "Afro" } },
        { "MODAL", { "Modal" } },
        { "TRAD", { "Trad" } },
        { "UTILITY", { "Utility", "Stack" } },
    };
    return f;
}

// Normalised chord names of every set, built once.
const std::vector<std::array<String, kKeys>>& normalisedNames()
{
    static const std::vector<std::array<String, kKeys>> names = []
    {
        std::vector<std::array<String, kKeys>> v;
        for (const auto& set : chordSets())
        {
            std::array<String, kKeys> a;
            for (int k = 0; k < kKeys; ++k)
                a[(size_t) k] = normaliseChord (set.chords[k].name);
            v.push_back (a);
        }
        return v;
    }();
    return names;
}

bool chordMatches (const String& name, const String& tok)
{
    if (letterPc (tok[0]) >= 0)
        return name.startsWith (tok) && (name.length() == tok.length() || name[tok.length()] != '#');
    return name.contains (tok);
}

String setNumber (int index) { return String (index + 1).paddedLeft ('0', 3); }

constexpr int kFamilyAll = 0, kFamilyMine = 1, kFamilyStarred = 2;
} // namespace

String normaliseChord (const String& raw)
{
    auto s = raw.replace ("maj", "M").replace ("Maj", "M").replace ("min", "m")
                .replace ("Dim", "dim").replace ("Add", "add").replace ("Sus", "sus");
    return mapRoots (s, [] (int pc) { return String (kSharps[pc]); });
}

// ===========================================================================
// ChordPads
// ===========================================================================
namespace
{
constexpr int kWhiteKeys[7] { 0, 2, 4, 5, 7, 9, 11 };
// Black keys sit over the gap after this white key (0-based white index).
constexpr int kBlackKeys[5]  { 1, 3, 6, 8, 10 };
constexpr int kBlackAfter[5] { 0, 1, 3, 4, 5 };
}

ChordPads::ChordPads (PosyProcessor& p) : proc (p)
{
    setTooltip ("Click a chord to play it, drag across to move between chords. Higher on a pad plays "
                "louder (unless FIXED velocity is on). A MIDI keyboard on this track plays them too: "
                "each key of the octave picks a chord. In EDIT mode, click a pad to choose where the "
                "next chord you play lands; double-click to rename it, right-click to clear it.");
}

Rectangle<float> ChordPads::padBounds (int key) const
{
    const auto b = getLocalBounds().toFloat();
    const float colW = b.getWidth() / 7.0f;
    const float rowH = (b.getHeight() - 10.0f) / 2.0f;
    for (int i = 0; i < 7; ++i)
        if (kWhiteKeys[i] == key)
            return { colW * (float) i, rowH + 10.0f, colW, rowH };
    for (int i = 0; i < 5; ++i)
        if (kBlackKeys[i] == key)
            return { colW * ((float) kBlackAfter[i] + 0.5f), 0.0f, colW, rowH };
    return {};
}

int ChordPads::keyAt (Point<float> p) const
{
    for (int k = 0; k < kKeys; ++k)
        if (padBounds (k).reduced (3.0f).contains (p))
            return k;
    return -1;
}

void ChordPads::press (int key, Point<float> p)
{
    int vel = roundToInt (proc.getParam (ids::velocity));
    if (proc.getParam (ids::fixedVel) < 0.5f)
    {
        const auto r = padBounds (key).reduced (3.0f);
        const float up = jlimit (0.0f, 1.0f, 1.0f - (p.y - r.getY()) / jmax (1.0f, r.getHeight()));
        vel = jlimit (1, 127, roundToInt (40.0f + up * 87.0f));
    }
    proc.pressPad (key, vel);
    mouseKey = key;
    repaint();
}

void ChordPads::releaseHeld()
{
    if (mouseKey >= 0)
        proc.releasePad (mouseKey);
    mouseKey = -1;
    repaint();
}

void ChordPads::mouseDown (const MouseEvent& e)
{
    const int k = keyAt (e.position);
    if (k < 0)
        return;
    if (proc.editing())
    {
        proc.selectPad (k);
        if (e.mods.isPopupMenu())
        {
            padMenu (k);
            return;
        }
    }
    else if (e.mods.isPopupMenu())
    {
        return;
    }
    press (k, e.position);
}

void ChordPads::mouseDoubleClick (const MouseEvent& e)
{
    if (! proc.editing())
        return;
    if (const int k = keyAt (e.position); k >= 0)
    {
        releaseHeld();
        rename (k);
    }
}

void ChordPads::rename (int key)
{
    askForName (this, "Name the " + String (keyName (key)) + " pad (empty: name it from its notes)",
                proc.chordName (key), [this, key] (const String& name) { proc.setPadName (key, name); });
}

void ChordPads::padMenu (int key)
{
    const auto& c = proc.bank().chords[(size_t) key];
    PopupMenu m;
    m.addSectionHeader (String (keyName (key)) + " pad");
    m.addItem ("Rename...", [this, key] { rename (key); });
    m.addItem ("Name it from its notes", c.named, false, [this, key] { proc.setPadName (key, {}); });
    m.addItem ("Clear", ! c.notes.isEmpty(), false, [this, key] { proc.clearPad (key); });
    m.showMenuAsync (PopupMenu::Options().withTargetComponent (this));
}

void ChordPads::mouseDrag (const MouseEvent& e)
{
    const int k = keyAt (e.position);
    if (k == mouseKey || k < 0)
        return;
    releaseHeld();
    press (k, e.position);
}

void ChordPads::mouseUp (const MouseEvent&)
{
    releaseHeld();
}

void ChordPads::refresh()
{
    String sig;
    sig << proc.setIndex() << ":" << proc.playingKey() << ":" << mouseKey << ":" << (int) proc.isCustom()
        << ":" << (int) proc.bankEdits() << ":" << (int) proc.editing() << ":" << proc.editPad();
    for (int k = 0; k < kKeys; ++k) sig << (proc.keyDown (k) ? "1" : "0");
    for (auto* id : { ids::key, ids::octave, ids::invert, ids::spread, ids::bass })
        sig << ":" << roundToInt (proc.getParam (id));
    if (sig != lastSignature)
    {
        lastSignature = sig;
        repaint();
    }
}

void ChordPads::paint (Graphics& g)
{
    const int transpose = roundToInt (proc.getParam (ids::key));
    const int playing = proc.playingKey();
    const bool editing = proc.editing();

    for (int k = 0; k < kKeys; ++k)
    {
        const auto r = padBounds (k).reduced (3.0f);
        const bool lit = k == playing;
        const bool down = k == mouseKey || proc.keyDown (k);
        const bool sharp = String (keyName (k)).containsChar ('#');
        const bool target = editing && k == proc.editPad();

        g.setColour (lit ? colours::cream : (sharp ? colours::black : Colour (0xff1a1a1c)));
        g.fillRect (r);
        g.setColour (down || target ? colours::brand : colours::ink);
        g.drawRect (r, target ? 3.5f : down ? 2.5f : 1.5f);

        const auto fg  = lit ? colours::black : colours::ink;
        const auto dim = lit ? colours::black.withAlpha (0.6f) : colours::inkDim;

        auto inner = r.reduced (10.0f, 8.0f);
        auto top = inner.removeFromTop (16.0f);
        drawText (g, keyName (k), top, 13.0f, dim, true, Justification::centredLeft);
        if (target)
            drawText (g, "PLAY A CHORD", top, 10.5f, colours::brand, true, Justification::centredRight);
        else
            drawLed (g, { top.getRight() - 6.0f, top.getCentreY() }, 5.0f, colours::brand, lit, lit ? 0.6f : 0.0f);

        // Notes along the bottom.
        const auto v = proc.previewVoicing (k);
        StringArray notes;
        for (int i = 0; i < v.count; ++i)
            notes.add (String (noteName (v.notes[(size_t) i])));
        auto bottom = inner.removeFromBottom (16.0f);
        drawText (g, v.count > 0 ? notes.joinIntoString (" ") : String ("empty"), bottom, 12.0f, dim, false,
                  Justification::centredLeft);

        // The chord name, as large as fits.
        const auto raw = proc.chordName (k);
        const auto name = raw.isNotEmpty() ? transposeName (raw, transpose) : String::fromUTF8 ("\xe2\x80\x94");
        float h = 30.0f;
        while (h > 13.0f && GlyphArrangement::getStringWidth (panelFont (h), name) > inner.getWidth())
            h -= 1.0f;
        drawText (g, name, inner, h, fg, true, Justification::centred);
    }
}

// ===========================================================================
// Browser
// ===========================================================================
Browser::Browser (PosyProcessor& p, Settings& s) : proc (p), settings (s)
{
    search.setTextToShowWhenEmpty ("Search: genre, chords (Am7 F), or a number", colours::inkDim);
    search.setFont (panelFont (16.0f, false));
    search.setIndents (10, 8);
    search.setSelectAllWhenFocused (true);
    search.addListener (this);
    search.addKeyListener (this);
    search.setTooltip ("Type to filter. Words match genres, bank names, chord names (Am7, F#m9, /G) and set "
                       "numbers; every word must match, so \"Dm7 G7 jazz\" finds jazz sets with both chords. "
                       "Up/Down step through the results, Esc clears.");
    addAndMakeVisible (search);

    const auto& fam = families();
    for (int i = 0; i < (int) fam.size(); ++i)
    {
        auto b = std::make_unique<PanelButton> (i == kFamilyStarred ? star (true) + " STARRED"
                                                                    : String (fam[(size_t) i].label));
        b->setFontHeight (11.0f);
        b->onClick = [this, i] (const ModifierKeys&) { setFamily (i); };
        b->setTooltip (i == kFamilyStarred ? "Only the sets you've starred (click the star on a row)."
                     : i == kFamilyMine    ? "Your own banks: chords you played in with EDIT and saved."
                     : i == kFamilyAll     ? "Every set: the factory sets, then your own banks."
                                           : "Show " + String (fam[(size_t) i].label).toLowerCase() + " sets.");
        addAndMakeVisible (*b);
        chips.push_back (std::move (b));
    }
    chips[0]->setLit (true);

    list.setModel (this);
    list.setRowHeight (50);
    list.setColour (ListBox::backgroundColourId, colours::black);
    list.setColour (ListBox::outlineColourId, colours::inkDim);
    list.setOutlineThickness (1);
    list.getVerticalScrollBar().setColour (ScrollBar::thumbColourId, colours::inkDim);
    addAndMakeVisible (list);

    starredCache = settings.starred();
    rebuild();
}

Browser::~Browser()
{
    search.removeKeyListener (this);
    list.setModel (nullptr);
}

void Browser::resized()
{
    auto r = getLocalBounds();
    search.setBounds (r.removeFromTop (36));
    r.removeFromTop (10);
    const int per = 4, gap = 6;
    const int w = (r.getWidth() - gap * (per - 1)) / per;
    for (int i = 0; i < (int) chips.size(); ++i)
        chips[(size_t) i]->setBounds (r.getX() + (i % per) * (w + gap), r.getY() + (i / per) * 34, w, 28);
    const int rows = ((int) chips.size() + per - 1) / per;
    r.removeFromTop (rows * 34 + 6);
    r.removeFromBottom (22);
    list.setBounds (r);
    lastCurrent.clear();   // scroll the loaded set into view now the list has a size
    refresh();
}

void Browser::paint (Graphics& g)
{
    drawText (g, countText, getLocalBounds().removeFromBottom (18).toFloat(), 12.0f, colours::inkDim, true,
              Justification::centredLeft);
}

void Browser::setFamily (int f)
{
    family = f;
    for (int i = 0; i < (int) chips.size(); ++i)
        chips[(size_t) i]->setLit (i == f);
    refilter();
}

void Browser::rebuild()
{
    entries.clear();
    for (int i = 0; i < numChordSets(); ++i)
    {
        Entry e;
        e.factory = i;
        e.title = chordSet (i).genre;
        for (int k = 0; k < kKeys; ++k)
        {
            e.chordNames.add (chordSet (i).chords[k].name);
            e.normalised[(size_t) k] = normalisedNames()[(size_t) i][(size_t) k];
        }
        entries.push_back (e);
    }

    numBanks = 0;
    for (auto& f : banks::list())
    {
        CustomBank b;
        if (! banks::load (f, b))
            continue;
        Entry e;
        e.file = f;
        e.title = b.name;
        for (int k = 0; k < kKeys; ++k)
        {
            const auto& name = b.chords[(size_t) k].name;
            e.chordNames.add (name.isNotEmpty() ? name : String ("-"));
            e.normalised[(size_t) k] = normaliseChord (name);
        }
        entries.push_back (e);
        ++numBanks;
    }
    filesSeen = proc.bankFilesEdits();
    refilter();
}

bool Browser::matches (const Entry& e) const
{
    const bool isBank = e.factory < 0;
    if (family == kFamilyMine && ! isBank)
        return false;
    if (family == kFamilyStarred && (isBank || starredCache.count (e.factory + 1) == 0))
        return false;
    if (family > kFamilyStarred)
    {
        if (isBank)
            return false;
        bool in = false;
        for (auto* w : families()[(size_t) family].words)
            in = in || e.title.containsIgnoreCase (w);
        if (! in)
            return false;
    }

    auto tokens = StringArray::fromTokens (search.getText(), " ,", "");
    tokens.removeEmptyStrings();
    for (auto tok : tokens)
    {
        if (tok.containsOnly ("0123456789"))
        {
            if (isBank || ! String (e.factory + 1).startsWith (String (tok.getIntValue())))
                return false;
            continue;
        }
        if (e.title.containsIgnoreCase (tok) || (isBank && String ("my sets").containsIgnoreCase (tok)))
            continue;

        if (letterPc (CharacterFunctions::toUpperCase (tok[0])) >= 0)
            tok = String::charToString (CharacterFunctions::toUpperCase (tok[0])) + tok.substring (1);
        tok = normaliseChord (tok);
        bool any = false;
        for (const auto& n : e.normalised)
            any = any || chordMatches (n, tok);
        if (! any)
            return false;
    }
    return true;
}

bool Browser::isCurrent (const Entry& e) const
{
    if (e.factory >= 0)
        return ! proc.isCustom() && e.factory == proc.setIndex();
    return proc.isCustom() && proc.bank().file == e.file;
}

void Browser::refilter()
{
    shown.clear();
    for (int i = 0; i < (int) entries.size(); ++i)
        if (matches (entries[(size_t) i]))
            shown.push_back (i);
    countText = String ((int) shown.size()) + " of " + String ((int) entries.size()) + " sets"
              + (numBanks > 0 ? "  (" + String (numBanks) + " yours)" : String());
    list.updateContent();
    list.repaint();
    lastCurrent.clear();   // re-scroll to the loaded set
    refresh();
    repaint();
}

void Browser::refresh()
{
    if (proc.bankFilesEdits() != filesSeen)
    {
        rebuild();
        return;
    }
    const String cur = proc.isCustom() ? "bank:" + proc.bank().file.getFullPathName()
                                       : "set:" + String (proc.setIndex());
    if (cur == lastCurrent)
        return;
    lastCurrent = cur;
    for (int row = 0; row < (int) shown.size(); ++row)
        if (isCurrent (entries[(size_t) shown[(size_t) row]]))
            list.scrollToEnsureRowIsOnscreen (row);
    list.repaint();
}

void Browser::paintListBoxItem (int row, Graphics& g, int w, int h, bool)
{
    if (! isPositiveAndBelow (row, (int) shown.size()))
        return;
    const auto& e = entries[(size_t) shown[(size_t) row]];
    const bool current = isCurrent (e);
    const bool isBank = e.factory < 0;

    auto r = Rectangle<float> (0.0f, 0.0f, (float) w, (float) h);
    g.setColour (current ? colours::cream : (row % 2 ? Colour (0xff17171a) : colours::black));
    g.fillRect (r);

    const auto num = current ? colours::black : (isBank ? colours::brand : colours::lcd);
    const auto fg  = current ? colours::black : colours::ink;
    const auto dim = current ? colours::black.withAlpha (0.65f) : colours::inkDim;

    auto line1 = r.reduced (10.0f, 4.0f).removeFromTop (22.0f);
    if (! isBank)
    {
        const bool starred = starredCache.count (e.factory + 1) > 0;
        drawText (g, star (starred), line1.removeFromRight (24.0f), 18.0f,
                  starred ? (current ? colours::black : colours::brand) : dim, true, Justification::centredRight);
    }
    drawText (g, isBank ? String ("MY") : setNumber (e.factory), line1.removeFromLeft (40.0f), 17.0f, num, true,
              Justification::centredLeft);
    const bool dirty = isBank && current && proc.bankDirty();
    drawText (g, e.title + (dirty ? " *" : ""), line1, 15.0f, fg, true, Justification::centredLeft);

    auto line2 = r.reduced (10.0f, 4.0f).withTrimmedTop (24.0f).withTrimmedLeft (40.0f);
    g.setColour (dim);
    g.setFont (panelFont (12.5f, false));
    g.drawText (e.chordNames.joinIntoString ("  "), line2, Justification::centredLeft, true);
}

void Browser::open (const Entry& e)
{
    if (isCurrent (e))
        return;
    const auto file = e.file;
    const int factory = e.factory;
    whenBankIsSafe (proc, this, [this, file, factory]
    {
        if (factory >= 0)
            proc.loadSet (factory);
        else if (! proc.loadBank (file))
            AlertWindow::showAsync (MessageBoxOptions().withTitle ("Couldn't open that bank")
                                        .withMessage (file.getFullPathName()).withButton ("OK"), nullptr);
        refresh();
    });
}

void Browser::bankMenu (const Entry& e)
{
    const auto file = e.file;
    const auto title = e.title;
    PopupMenu m;
    m.addSectionHeader (title);
    m.addItem ("Rename...", [this, file, title]
    {
        askForName (this, "Rename bank", title, [this, file] (const String& raw)
        {
            const auto name = banks::sanitise (raw);
            const auto to = banks::fileFor (name);
            if (name.isEmpty() || to == file)
                return;
            if (to.exists())
            {
                AlertWindow::showAsync (MessageBoxOptions().withTitle ("There's already a bank called " + name)
                                            .withButton ("OK"), nullptr);
                return;
            }
            CustomBank b;
            if (! banks::load (file, b))
                return;
            b.name = name;
            if (banks::save (b, to))
            {
                file.moveToTrash() || file.deleteFile();
                proc.bankFileRenamed (file, to, name);
            }
        });
    });
    m.addItem ("Show in folder", [file] { file.revealToUser(); });
    m.addSeparator();
    m.addItem ("Delete", [this, file, title]
    {
        auto opts = MessageBoxOptions().withTitle ("Delete " + title + "?")
                        .withMessage ("The bank goes to the Trash. A Live set that uses it keeps its own copy.")
                        .withButton ("Delete").withButton ("Cancel").withAssociatedComponent (this);
        AlertWindow::showAsync (opts, [this, file] (int result)
        {
            if (result != 1)
                return;
            file.moveToTrash() || file.deleteFile();
            proc.bankFileRenamed (file, File(), proc.isCustom() ? proc.bank().name : String());
        });
    });
    m.showMenuAsync (PopupMenu::Options().withTargetComponent (&list));
}

void Browser::listBoxItemClicked (int row, const MouseEvent& e)
{
    if (! isPositiveAndBelow (row, (int) shown.size()))
        return;
    const auto entry = entries[(size_t) shown[(size_t) row]];
    if (entry.factory < 0)
    {
        if (e.mods.isPopupMenu())
            bankMenu (entry);
        else
            open (entry);
        return;
    }
    if (e.x > list.getVisibleRowWidth() - 40)
    {
        const bool on = starredCache.count (entry.factory + 1) == 0;
        settings.setStarred (entry.factory + 1, on);
        starredCache = settings.starred();
        if (family == kFamilyStarred)
            refilter();
        list.repaint();
        return;
    }
    if (! e.mods.isPopupMenu())
        open (entry);
}

String Browser::getTooltipForRow (int row)
{
    if (! isPositiveAndBelow (row, (int) shown.size()))
        return {};
    const auto& e = entries[(size_t) shown[(size_t) row]];
    StringArray lines;
    for (int k = 0; k < kKeys; ++k)
        lines.add (String (keyName (k)).paddedRight (' ', 3) + "  " + e.chordNames[k]);
    if (e.factory < 0)
        return e.title + "  (your bank)\n" + lines.joinIntoString ("\n")
             + "\n\nClick to load. Right-click to rename, show or delete it.";
    return "Set " + setNumber (e.factory) + "  " + e.title + "\n" + lines.joinIntoString ("\n")
         + "\n\nClick to load. Click the star to keep it in STARRED.";
}

void Browser::step (int delta)
{
    if (shown.empty())
        return;
    int row = -1;
    for (int i = 0; i < (int) shown.size(); ++i)
        if (isCurrent (entries[(size_t) shown[(size_t) i]]))
            row = i;
    row = row < 0 ? (delta > 0 ? 0 : (int) shown.size() - 1)
                  : jlimit (0, (int) shown.size() - 1, row + delta);
    open (entries[(size_t) shown[(size_t) row]]);
}

bool Browser::keyPressed (const KeyPress& k, Component*)
{
    if (k == KeyPress::downKey)     { step (1);  return true; }
    if (k == KeyPress::upKey)       { step (-1); return true; }
    if (k == KeyPress::pageDownKey) { step (8);  return true; }
    if (k == KeyPress::pageUpKey)   { step (-8); return true; }
    return false;
}

void Browser::textEditorReturnKeyPressed (TextEditor&)
{
    for (int i : shown)
        if (isCurrent (entries[(size_t) i]))
            return;
    if (! shown.empty())
        open (entries[(size_t) shown.front()]);
}

void Browser::textEditorEscapeKeyPressed (TextEditor&)
{
    search.clear();
    refilter();
}

// ===========================================================================
// NoteStrip
// ===========================================================================
void NoteStrip::refresh()
{
    std::array<uint32_t, 4> bits {};
    for (int n = 0; n < 128; ++n)
        if (proc.noteSounding (n))
            bits[(size_t) (n >> 5)] |= 1u << (n & 31);
    const uint32_t edits = proc.editing() ? proc.bankEdits() * 2u + 1u : 0u;
    if (bits != lastBits || edits != lastEdits)
    {
        lastBits = bits;
        lastEdits = edits;
        repaint();
    }
}

int NoteStrip::noteAt (Point<float> p) const
{
    auto isBlack = [] (int n) { const int pc = n % 12; return pc == 1 || pc == 3 || pc == 6 || pc == 8 || pc == 10; };
    int whites = 0;
    for (int n = lowNote; n < highNote; ++n)
        whites += isBlack (n) ? 0 : 1;
    const auto b = getLocalBounds().toFloat();
    const float ww = b.getWidth() / (float) whites;

    int wi = 0;
    if (p.y < b.getY() + b.getHeight() * 0.6f)       // the black keys sit on top
        for (int n = lowNote; n < highNote; ++n)
        {
            if (! isBlack (n)) { ++wi; continue; }
            const float x = b.getX() + ww * (float) wi;
            if (std::abs (p.x - x) <= ww * 0.32f)
                return n;
        }
    wi = 0;
    for (int n = lowNote; n < highNote; ++n)
    {
        if (isBlack (n)) continue;
        if (p.x >= b.getX() + ww * (float) wi && p.x < b.getX() + ww * (float) (wi + 1))
            return n;
        ++wi;
    }
    return -1;
}

void NoteStrip::mouseDown (const MouseEvent& e)
{
    if (! proc.editing())
        return;
    if (const int n = noteAt (e.position); n >= 0)
        proc.togglePadNote (proc.editPad(), n);
}

void NoteStrip::paint (Graphics& g)
{
    auto isBlack = [] (int n) { const int pc = n % 12; return pc == 1 || pc == 3 || pc == 6 || pc == 8 || pc == 10; };
    int whites = 0;
    for (int n = lowNote; n < highNote; ++n)
        whites += isBlack (n) ? 0 : 1;

    const auto b = getLocalBounds().toFloat();
    const float ww = b.getWidth() / (float) whites;

    // In EDIT mode, the selected pad's notes get a mark.
    juce::Array<int> padNotes;
    if (proc.editing() && proc.isCustom())
        padNotes = proc.bank().chords[(size_t) proc.editPad()].notes;
    auto mark = [&g] (Rectangle<float> key, Colour c)
    {
        const float d = jmin (key.getWidth() * 0.55f, 7.0f);
        g.setColour (c);
        g.fillEllipse (Rectangle<float> (d, d).withCentre ({ key.getCentreX(), key.getBottom() - d }));
    };

    // Whites.
    int wi = 0;
    for (int n = lowNote; n < highNote; ++n)
    {
        if (isBlack (n)) continue;
        const auto r = Rectangle<float> (b.getX() + ww * (float) wi, b.getY(), ww, b.getHeight());
        g.setColour (proc.noteSounding (n) ? colours::brand : colours::cream);
        g.fillRect (r.reduced (0.5f, 0.0f));
        if (padNotes.contains (n))
            mark (r.withTrimmedBottom (2.0f), colours::black);
        else if (n % 12 == 0)
            drawText (g, noteName (n), r.withTrimmedTop (r.getHeight() - 13.0f), 9.0f, colours::black, false);
        ++wi;
    }
    // Blacks.
    wi = 0;
    for (int n = lowNote; n < highNote; ++n)
    {
        if (! isBlack (n)) { ++wi; continue; }
        const auto r = Rectangle<float> (b.getX() + ww * (float) wi - ww * 0.32f, b.getY(), ww * 0.64f, b.getHeight() * 0.6f);
        g.setColour (proc.noteSounding (n) ? colours::brand.darker (0.25f) : colours::black);
        g.fillRect (r);
        if (padNotes.contains (n))
            mark (r.withTrimmedBottom (2.0f), colours::cream);
    }
    g.setColour (colours::ink);
    g.drawRect (b, 1.0f);
}

// ===========================================================================
// TakeStrip
// ===========================================================================
std::vector<std::pair<int, Rectangle<float>>> TakeStrip::layoutChips() const
{
    std::vector<std::pair<int, Rectangle<float>>> out;
    const auto& take = proc.take();
    auto area = getLocalBounds().toFloat().reduced (6.0f).withTrimmedRight (26.0f);
    float x = area.getRight();
    for (int i = take.size(); --i >= 0;)
    {
        const float w = jmax (54.0f, GlyphArrangement::getStringWidth (panelFont (14.0f), take[i].name) + 18.0f);
        if (x - w < area.getX() + 34.0f && i > 0)
            break;
        if (x - w < area.getX())
            break;
        out.push_back ({ i, { x - w, area.getY(), w, area.getHeight() } });
        x -= w + 6.0f;
    }
    std::reverse (out.begin(), out.end());
    return out;
}

int TakeStrip::chipAt (Point<float> p) const
{
    for (auto& [i, r] : layoutChips())
        if (r.contains (p))
            return i;
    return -1;
}

void TakeStrip::refresh()
{
    if (proc.takeEdits() != shownVersion)
    {
        shownVersion = proc.takeEdits();
        repaint();
    }
}

void TakeStrip::paint (Graphics& g)
{
    const auto b = getLocalBounds().toFloat();
    g.setColour (colours::black);
    g.fillRect (b);
    g.setColour (colours::inkDim);
    g.drawRect (b, 1.0f);

    // Grip on the right: this is the bit you drag.
    const auto grip = b.withLeft (b.getRight() - 24.0f).reduced (7.0f, 10.0f);
    g.setColour (proc.take().isEmpty() ? colours::socket : colours::ink);
    for (int i = 0; i < 3; ++i)
        g.fillRect (grip.getX() + (float) i * 4.0f, grip.getY(), 2.0f, grip.getHeight());

    const auto& take = proc.take();
    if (take.isEmpty())
    {
        drawText (g, proc.takeRecording() ? "Chords you play collect here. Then drag this strip into a clip slot."
                                          : "REC is off: chords aren't being collected.",
                  b.reduced (12.0f, 0.0f), 13.0f, colours::inkDim, false, Justification::centredLeft);
        return;
    }

    const auto chipsShown = layoutChips();
    if (! chipsShown.empty() && chipsShown.front().first > 0)
        drawText (g, "+" + String (chipsShown.front().first), b.withWidth (34.0f), 12.0f, colours::inkDim, true);

    for (auto& [i, r] : chipsShown)
    {
        const bool last = i == take.size() - 1;
        g.setColour (last ? colours::cream : colours::panel);
        g.fillRect (r);
        g.setColour (colours::ink);
        g.drawRect (r, 1.0f);
        drawText (g, take[i].name, r, 14.0f, last ? colours::black : colours::ink, true);
    }
}

void TakeStrip::mouseDown (const MouseEvent& e)
{
    dragging = false;
    if (! e.mods.isPopupMenu())
        return;
    const int i = chipAt (e.position);
    PopupMenu m;
    if (i >= 0)
        m.addItem (1, "Remove " + proc.take()[i].name);
    m.addItem (2, "Clear take", ! proc.take().isEmpty());
    m.showMenuAsync (PopupMenu::Options().withTargetComponent (this), [this, i] (int r)
    {
        if (r == 1 && i >= 0) proc.removeTake (i);
        if (r == 2) proc.clearTake();
        refresh();
    });
}

void TakeStrip::mouseDrag (const MouseEvent& e)
{
    if (dragging || e.mods.isPopupMenu() || e.getDistanceFromDragStart() < 6 || proc.take().isEmpty())
        return;
    dragging = true;
    const auto file = proc.writeTakeMidi();
    if (file.existsAsFile())
        DragAndDropContainer::performExternalDragDropOfFiles ({ file.getFullPathName() }, false, this,
                                                              [this] { dragging = false; });
}

// ===========================================================================
// SetDisplay
// ===========================================================================
void SetDisplay::refresh()
{
    String sig;
    sig << proc.setIndex() << ":" << proc.playingKey() << ":" << roundToInt (proc.getParam (ids::key))
        << ":" << roundToInt (proc.getParam (ids::octave)) << ":" << (int) proc.bankEdits()
        << ":" << (int) proc.isCustom() << ":" << (int) proc.editing();
    if (sig != shown)
    {
        shown = sig;
        repaint();
    }
}

void SetDisplay::paint (Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced (1.0f);
    g.setColour (colours::black);
    g.fillRect (r);
    g.setColour (colours::inkDim);
    g.drawRect (r, 1.0f);

    const int idx = proc.setIndex();
    const bool custom = proc.isCustom();
    const int transpose = roundToInt (proc.getParam (ids::key));
    const int oct = roundToInt (proc.getParam (ids::octave));

    r = r.reduced (14.0f, 4.0f);
    drawText (g, custom ? String ("MY") : setNumber (idx), r.removeFromLeft (62.0f), 32.0f,
              custom ? colours::brand : colours::lcd, true, Justification::centredLeft);

    // Right: the chord now playing.
    const int k = proc.playingKey();
    auto right = r.removeFromRight (190.0f);
    drawText (g, "NOW", right.removeFromLeft (36.0f), 11.0f, colours::inkDim, true, Justification::centredLeft);
    const auto nowName = k >= 0 ? proc.chordName (k) : String();
    const String now = nowName.isNotEmpty() ? transposeName (nowName, transpose) : String::fromUTF8 ("\xe2\x80\x94");
    float h = 26.0f;
    while (h > 12.0f && GlyphArrangement::getStringWidth (panelFont (h), now) > right.getWidth())
        h -= 1.0f;
    drawText (g, now, right, h, colours::lcd, true, Justification::centredRight);

    // Middle: genre or bank name, then what EDIT is doing, or the key/octave if moved.
    auto top = r.removeFromTop (r.getHeight() * 0.58f);
    drawText (g, proc.setTitle().toUpperCase(), top, 18.0f, colours::lcd, true, Justification::bottomLeft);
    String sub;
    if (proc.editing())
    {
        sub << "EDIT: PLAY A CHORD FOR " << keyName (proc.editPad());
        if (proc.lastCaptured().isNotEmpty())
            sub << "   (" << proc.lastCaptured() << ")";
    }
    else
    {
        if (transpose != 0) sub << "KEY " << ids::keyText (transpose) << "   ";
        if (oct != 0)       sub << "OCT " << ids::signedText (oct);
        if (sub.isEmpty())  sub = custom ? (proc.bank().file == File() ? "NOT SAVED" : "YOUR BANK") : "12 CHORDS";
    }
    drawText (g, sub, r, 11.0f, proc.editing() ? colours::brand : colours::inkDim, true, Justification::topLeft);
}
// ===========================================================================
// Bank dialogs
// ===========================================================================
void askForName (Component* near, const String& title, const String& initial,
                 std::function<void (const String&)> done)
{
    auto* w = new AlertWindow (title, {}, MessageBoxIconType::NoIcon, near);
    w->addTextEditor ("name", initial);
    w->addButton ("OK", 1, KeyPress (KeyPress::returnKey));
    w->addButton ("Cancel", 0, KeyPress (KeyPress::escapeKey));
    if (auto* te = w->getTextEditor ("name"))
        te->setSelectAllWhenFocused (true);
    w->enterModalState (true, ModalCallbackFunction::create ([w, done] (int result)
    {
        if (result == 1 && done)
            done (w->getTextEditorContents ("name"));
    }), true);
}

void saveBankAs (PosyProcessor& proc, Component* near, std::function<void()> then)
{
    const auto initial = proc.isCustom() ? proc.bank().name : String ("My Bank");
    Component::SafePointer<Component> safe (near);
    askForName (near, "Save bank as", initial, [&proc, safe, then] (const String& raw)
    {
        const auto name = banks::sanitise (raw);
        if (name.isEmpty())
            return;
        auto finish = [&proc, safe, then, name]
        {
            proc.startEditing();
            if (proc.saveBankAs (name))
            {
                if (then) then();
            }
            else
            {
                AlertWindow::showAsync (MessageBoxOptions().withTitle ("Couldn't save " + name)
                                            .withMessage (banks::folder().getFullPathName())
                                            .withButton ("OK"), nullptr);
            }
        };
        const auto target = banks::fileFor (name);
        if (target.exists() && target != proc.bank().file)
        {
            auto opts = MessageBoxOptions().withTitle ("Replace " + name + "?")
                            .withMessage ("There's already a bank with that name.")
                            .withButton ("Replace").withButton ("Cancel").withAssociatedComponent (safe.getComponent());
            AlertWindow::showAsync (opts, [finish] (int r) { if (r == 1) finish(); });
            return;
        }
        finish();
    });
}

void saveBank (PosyProcessor& proc, Component* near, std::function<void()> then)
{
    if (proc.isCustom() && proc.bank().file != File() && proc.saveBank())
    {
        if (then) then();
        return;
    }
    saveBankAs (proc, near, std::move (then));
}

void whenBankIsSafe (PosyProcessor& proc, Component* near, std::function<void()> then)
{
    const auto& b = proc.bank();
    const bool nothingToLose = ! proc.isCustom() || ! proc.bankDirty()
                            || (b.file == File() && b.isEmpty());
    if (nothingToLose)
    {
        then();
        return;
    }
    auto* w = new AlertWindow ("Save changes to " + b.name + "?",
                               "Your edits to this bank haven't been saved.", MessageBoxIconType::NoIcon, near);
    w->addButton ("Save", 1, KeyPress (KeyPress::returnKey));
    w->addButton ("Don't save", 2);
    w->addButton ("Cancel", 0, KeyPress (KeyPress::escapeKey));
    Component::SafePointer<Component> safe (near);
    w->enterModalState (true, ModalCallbackFunction::create ([&proc, safe, then] (int result)
    {
        if (result == 1)
            saveBank (proc, safe.getComponent(), then);
        else if (result == 2)
            then();
    }), true);
}
} // namespace posy::ui
