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
// keyboard, sharps above. Press to play; drag across to move between chords.
// ---------------------------------------------------------------------------
class ChordPads final : public juce::Component, public juce::SettableTooltipClient
{
public:
    explicit ChordPads (PosyProcessor&);

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;

    void refresh();                        // repaint if anything shown changed
    juce::Rectangle<float> padBounds (int key) const;

private:
    int keyAt (juce::Point<float>) const;
    void press (int key, juce::Point<float>);
    void releaseHeld();

    PosyProcessor& proc;
    int mouseKey = -1;
    juce::String lastSignature;
};

// ---------------------------------------------------------------------------
// Browser: search box, genre families, starred sets, and the list itself.
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

    void refresh();                        // follow the loaded set
    void focusSearch() { search.grabKeyboardFocus(); }
    void step (int delta);                 // load the next/previous set in the list as filtered

private:
    // ListBoxModel
    int  getNumRows() override { return (int) shown.size(); }
    void paintListBoxItem (int row, juce::Graphics&, int w, int h, bool selected) override;
    void listBoxItemClicked (int row, const juce::MouseEvent&) override;
    void selectedRowsChanged (int) override {}
    juce::String getTooltipForRow (int row) override;

    // TextEditor::Listener
    void textEditorTextChanged (juce::TextEditor&) override { refilter(); }
    void textEditorReturnKeyPressed (juce::TextEditor&) override;
    void textEditorEscapeKeyPressed (juce::TextEditor&) override;

    // KeyListener: arrow keys in the search box move through the list.
    bool keyPressed (const juce::KeyPress&, juce::Component*) override;

    void refilter();
    bool matches (int setIndex) const;
    void setFamily (int f);

    PosyProcessor& proc;
    Settings& settings;

    juce::TextEditor search;
    std::vector<std::unique_ptr<PanelButton>> chips;
    int family = 0;
    juce::ListBox list;
    std::vector<int> shown;                // set indices passing the filter
    std::set<int> starredCache;
    int lastLoaded = -1;
    juce::String countText;
};

// ---------------------------------------------------------------------------
// Note strip: a long keyboard showing exactly which notes are going out.
// ---------------------------------------------------------------------------
class NoteStrip final : public juce::Component, public juce::SettableTooltipClient
{
public:
    explicit NoteStrip (PosyProcessor& p) : proc (p) {}
    void paint (juce::Graphics&) override;
    void refresh();

    static constexpr int lowNote = 24, highNote = 108;   // C0 .. B6 in Live's names

private:
    PosyProcessor& proc;
    std::array<uint32_t, 4> lastBits {};
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
} // namespace posy::ui
