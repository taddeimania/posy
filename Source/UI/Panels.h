// Panels.h — Posy's bigger panel components: the chord pads, the set browser,
// the note strip and the take strip. Each reads the processor directly and is
// repainted from the editor's timer when what it shows has changed.
#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>
#include <set>

#include "../PluginProcessor.h"
#include "../Settings.h"
#include "Widgets.h"

namespace posy::ui
{
// ---------------------------------------------------------------------------
// Chord pads: the twelve chords of the set laid out like an octave of a
// keyboard, sharps above. Press to play; how high on the pad sets the velocity,
// and moving up and down inside a held pad changes it. Dragging onto another
// pad (either row) moves to that chord.
// In EDIT mode a click also selects the pad that the next chord you play
// lands on; double-click renames it, right-click offers rename and clear.
// ---------------------------------------------------------------------------
class ChordPads final : public juce::Component, public juce::SettableTooltipClient
{
public:
    explicit ChordPads (PosyProcessor&);

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;

    void refresh();                        // repaint if anything shown changed
    juce::Rectangle<float> padBounds (int key) const;

private:
    int keyAt (juce::Point<float>) const;
    void press (int key, juce::Point<float>);
    void releaseHeld();
    int heightVelocity (int key, juce::Point<float>) const;   // higher on the pad = louder
    void padMenu (int key);
    void rename (int key);

    PosyProcessor& proc;
    int mouseKey = -1;
    int dragVel = -1;                      // the held pad's velocity now (-1: no pad held)
    juce::String lastSignature;
};

// ---------------------------------------------------------------------------
// Browser: search box, genre families, starred sets, your own banks (MY SETS)
// and the list itself.
// ---------------------------------------------------------------------------
class Browser final : public juce::Component,
                      private juce::ListBoxModel,
                      private juce::TextEditor::Listener,
                      private juce::KeyListener
{
public:
    Browser (PosyProcessor&, Settings&);
    ~Browser() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    void refresh();                        // follow the loaded set, pick up saved banks
    void focusSearch() { search.grabKeyboardFocus(); }
    // Hand the keyboard back to Live, so its computer MIDI keyboard plays again.
    void releaseSearchFocus();
    bool isSearchBox (const juce::Component* c) const { return c == &search || search.isParentOf (c); }
    void step (int delta);                 // load the next/previous set in the list as filtered

private:
    struct Entry
    {
        int factory = -1;                  // factory set index, or -1 for one of your banks
        juce::File file;                   // the bank's file
        juce::String title;                // genre, or the bank's name
        juce::StringArray chordNames;
        std::array<juce::String, kKeys> normalised;
    };

    // ListBoxModel
    int  getNumRows() override { return (int) shown.size(); }
    void paintListBoxItem (int row, juce::Graphics&, int w, int h, bool selected) override;
    void listBoxItemClicked (int row, const juce::MouseEvent&) override;
    void selectedRowsChanged (int) override {}
    juce::String getTooltipForRow (int row) override;

    // TextEditor::Listener
    void textEditorTextChanged (juce::TextEditor&) override { lastTypedMs = juce::Time::getMillisecondCounter(); refilter(); }
    void textEditorReturnKeyPressed (juce::TextEditor&) override;
    void textEditorEscapeKeyPressed (juce::TextEditor&) override;

    // KeyListener: arrow keys in the search box move through the list.
    bool keyPressed (const juce::KeyPress&, juce::Component*) override;

    void rebuild();                        // factory sets plus the bank files on disk
    void refilter();
    bool matches (const Entry&) const;
    bool isCurrent (const Entry&) const;
    void open (const Entry&);
    void bankMenu (const Entry&);
    void setFamily (int f);

    PosyProcessor& proc;
    Settings& settings;

    juce::TextEditor search;
    std::vector<std::unique_ptr<PanelButton>> chips;
    int family = 0;
    juce::ListBox list;
    std::vector<Entry> entries;            // every factory set, then your banks
    std::vector<int> shown;                // indices into entries passing the filter
    std::set<int> starredCache;
    juce::String lastCurrent;              // what was loaded when we last scrolled to it
    uint32_t filesSeen = 0xffffffffu;
    int numBanks = 0;
    juce::String countText;
    juce::uint32 lastTypedMs = 0;
    bool searchWasFocused = false;
};

// ---------------------------------------------------------------------------
// Note strip: a long keyboard showing exactly which notes are going out. In
// EDIT mode it also marks the selected pad's notes, and clicking a key adds
// it to the pad or takes it away.
// ---------------------------------------------------------------------------
class NoteStrip final : public juce::Component, public juce::SettableTooltipClient
{
public:
    explicit NoteStrip (PosyProcessor& p) : proc (p) {}
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void refresh();

    static constexpr int lowNote = 24, highNote = 108;   // C0 .. B6 in Live's names

private:
    int noteAt (juce::Point<float>) const;

    PosyProcessor& proc;
    std::array<uint32_t, 4> lastBits {};
    uint32_t lastEdits = 0xffffffffu;
};

// ---------------------------------------------------------------------------
// Take strip: the chords you've played, in order. Drag it into a Live clip
// slot to get them as a MIDI clip.
// ---------------------------------------------------------------------------
class TakeStrip final : public juce::Component, public juce::SettableTooltipClient
{
public:
    explicit TakeStrip (PosyProcessor& p) : proc (p) {}
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void refresh();

private:
    int chipAt (juce::Point<float>) const;
    std::vector<std::pair<int, juce::Rectangle<float>>> layoutChips() const;

    PosyProcessor& proc;
    uint32_t shownVersion = 0xffffffffu;
    bool dragging = false;
};

// ---------------------------------------------------------------------------
// The header readout: set number, genre and the chord now playing.
// ---------------------------------------------------------------------------
class SetDisplay final : public juce::Component, public juce::SettableTooltipClient
{
public:
    explicit SetDisplay (PosyProcessor& p) : proc (p) {}
    void paint (juce::Graphics&) override;
    void refresh();
    std::function<void()> onClick;
    void mouseDown (const juce::MouseEvent&) override { if (onClick) onClick(); }

private:
    PosyProcessor& proc;
    juce::String shown;
};

// Chord-name search helpers (exposed for the browser).
juce::String normaliseChord (const juce::String&);   // flats to sharps, maj/min to M/m

// Dialogs for banks, shared by the browser and the editor's buttons.
void askForName (juce::Component* near, const juce::String& title, const juce::String& initial,
                 std::function<void (const juce::String&)> done);
void saveBankAs (PosyProcessor&, juce::Component* near, std::function<void()> then = {});
void saveBank (PosyProcessor&, juce::Component* near, std::function<void()> then = {});
// Runs `then` straight away unless the loaded bank has unsaved changes, in
// which case it asks first: save, discard, or cancel (then never runs).
void whenBankIsSafe (PosyProcessor&, juce::Component* near, std::function<void()> then);
} // namespace posy::ui
