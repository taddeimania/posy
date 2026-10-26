#include "PluginEditor.h"

using namespace posy;
using namespace posy::ui;
using juce::Rectangle;
using juce::String;

namespace
{
// Section boxes (design space).
const Rectangle<float> kBrowserBox { 12.0f,   84.0f, 416.0f, 804.0f };
const Rectangle<float> kChordsBox  { 440.0f,  84.0f, 948.0f, 360.0f };
const Rectangle<float> kOutBox     { 440.0f, 458.0f, 948.0f,  62.0f };
const Rectangle<float> kTakeBox    { 440.0f, 534.0f, 948.0f,  66.0f };
const Rectangle<float> kVoiceBox   { 440.0f, 614.0f, 460.0f, 274.0f };
const Rectangle<float> kArpBox     { 910.0f, 614.0f, 330.0f, 274.0f };
const Rectangle<float> kHoldBox    { 1250.0f, 614.0f, 138.0f, 274.0f };
const Rectangle<float> kArpStatus  { 1134.0f, 772.0f, 96.0f, 60.0f };

String takeLenName (int beats)
{
    return beats <= 2 ? "1/2 BAR" : beats <= 4 ? "1 BAR" : "2 BARS";
}
} // namespace

// ===========================================================================
PosyPanel::PosyPanel (PosyProcessor& p) : proc (p)
{
    setLookAndFeel (&lnf);
    lnf.setColour (juce::ScrollBar::thumbColourId, colours::inkDim);
    lnf.setColour (juce::TextEditor::highlightColourId, colours::brand.withAlpha (0.4f));
    lnf.setColour (juce::CaretComponent::caretColourId, colours::lcd);
    setSize (designW, designH);

    // --- header ----------------------------------------------------------------
    addAndMakeVisible (setDisplay);
    setDisplay.setBounds (300, 12, 560, 54);
    setDisplay.setTooltip ("The loaded chord set and the chord playing now. Click to search.");
    setDisplay.onClick = [this] { browser.focusSearch(); };

    addButton (prevBtn, { 870, 12, 46, 54 }, "Previous set in the list (as filtered).",
               [this] (const juce::ModifierKeys&) { browser.step (-1); });
    addButton (nextBtn, { 922, 12, 46, 54 }, "Next set in the list (as filtered).",
               [this] (const juce::ModifierKeys&) { browser.step (1); });
    prevBtn.setFontHeight (20.0f);
    nextBtn.setFontHeight (20.0f);

    addChoice (channelChoice, ids::channel, {}, { 1010, 16, 120, 46 }, "The MIDI channel Posy sends on.");
    channelChoice.getText = [this] { return "CH " + String (juce::roundToInt (proc.getParam (ids::channel))); };

    addButton (tipsBtn, { 1142, 16, 84, 46 }, "Show or hide these tips (remembered for every Posy).",
               [this] (const juce::ModifierKeys&)
               {
                   settings.setTooltipsEnabled (! settings.tooltipsEnabled());
                   syncTooltips();
               });
    addButton (panicBtn, { 1236, 16, 140, 46 },
               "Stop every note Posy is playing and let go of a held chord.",
               [this] (const juce::ModifierKeys&) { proc.panic(); });

    // --- browser ---------------------------------------------------------------
    addAndMakeVisible (browser);
    browser.setBounds (28, 104, 384, 772);

    // --- chords ------------------------------------------------------------------
    addAndMakeVisible (pads);
    pads.setBounds (452, 100, 924, 334);

    // --- custom banks: in the gap above E and F, where the keyboard has no sharp
    addButton (editBtn, { 788, 104, 120, 40 },
               "EDIT: play your own chords onto the pads. Click a pad, play a chord on your MIDI keyboard "
               "(you hear it as you play) and let go: it lands on the pad, named, and the next pad is "
               "selected. You can also click notes on the OUT keyboard to add or remove them. Editing a "
               "factory set starts a bank from a copy of it.",
               [this] (const juce::ModifierKeys&) { proc.setEditing (! proc.editing()); });
    editBtn.setFontHeight (15.0f);
    addButton (saveBtn, { 788, 150, 120, 32 }, "Save your bank (asks for a name the first time).",
               [this] (const juce::ModifierKeys&) { posy::ui::saveBank (proc, this); });
    addButton (saveAsBtn, { 788, 188, 120, 32 },
               "Save this bank, or the factory set you're on, as a new bank under MY SETS.",
               [this] (const juce::ModifierKeys&) { posy::ui::saveBankAs (proc, this); });
    addButton (newBtn, { 788, 226, 120, 32 }, "Start an empty bank and switch EDIT on.",
               [this] (const juce::ModifierKeys&)
               {
                   posy::ui::whenBankIsSafe (proc, this, [this] { proc.newBank(); });
               });

    addAndMakeVisible (strip);
    strip.setBounds (456, 474, 916, 36);
    strip.setTooltip ("Every note Posy is sending right now. C labels follow Live: middle C is C3.");

    // --- take --------------------------------------------------------------------
    addAndMakeVisible (takeStrip);
    takeStrip.setBounds (456, 548, 630, 40);
    takeStrip.setTooltip ("The chords you've played, in order (a repeat counts once). Drag the strip onto a "
                          "clip slot in Live to drop them in as a MIDI clip, one chord per LENGTH. "
                          "Right-click a chord to remove it.");
    addButton (recBtn, { 1096, 548, 64, 40 }, "Collect chords into the take as you play them.",
               [this] (const juce::ModifierKeys&) { proc.setTakeRecording (! proc.takeRecording()); });
    takeLenChoice.getText = [this] { return takeLenName (proc.takeBeats()); };
    takeLenChoice.onOpen = [this]
    {
        juce::PopupMenu m;
        const int beats[] { 2, 4, 8 };
        for (int b : beats)
            m.addItem (takeLenName (b), true, proc.takeBeats() == b, [this, b] { proc.setTakeBeats (b); });
        m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&takeLenChoice));
    };
    takeLenChoice.setTooltip ("How long each chord lasts in the dragged clip.");
    addAndMakeVisible (takeLenChoice);
    takeLenChoice.setBounds (1168, 548, 110, 40);
    choices.push_back (&takeLenChoice);
    addButton (clearTakeBtn, { 1286, 548, 88, 40 }, "Empty the take.",
               [this] (const juce::ModifierKeys&) { proc.clearTake(); });

    // --- voice -------------------------------------------------------------------
    addKnob (keyKnob,    ids::key,    "KEY",    { 452, 632, 86, 100 },
             "Transpose every chord, in semitones. The pads show the new names.");
    addKnob (octaveKnob, ids::octave, "OCTAVE", { 540, 632, 86, 100 }, "Move every chord up or down by octaves.");
    addKnob (invertKnob, ids::invert, "INVERT", { 628, 632, 86, 100 },
             "Invert the voicing: each step right lifts the lowest note an octave, left drops the highest.");
    addKnob (spreadKnob, ids::spread, "SPREAD", { 716, 632, 86, 100 },
             "Close: as stored. Open: lift the second note up an octave. Wide: lift every other note.");
    keyKnob.bipolar = octaveKnob.bipolar = invertKnob.bipolar = true;

    addToggle (bassBtn,   ids::bass,      { 808, 640, 80, 34 }, "Double the lowest note an octave below.");
    addToggle (legatoBtn, ids::legato,    { 808, 682, 80, 34 },
               "Notes shared by two chords keep ringing instead of being struck again. Smooth pads.");
    addToggle (followBtn, ids::followOct, { 808, 724, 80, 34 },
               "On: the octave you play on a MIDI keyboard moves the chord (C3 is home). "
               "Off: every octave plays the same chord.");

    addKnob (strumKnob,    ids::strum,    "STRUM",    { 452, 770, 86, 100 },
             "Spread the notes of a chord out in time, like a strum. 0 plays them together.");
    addToggle (strumDownBtn, ids::strumDown, { 540, 800, 80, 34 }, "Strum from the top note down.");
    addKnob (velocityKnob, ids::velocity, "VELOCITY", { 628, 770, 86, 100 },
             "Velocity for the pads when FIXED is on (and for every chord, from a keyboard too).");
    addToggle (fixedVelBtn, ids::fixedVel, { 716, 800, 80, 34 },
               "On: every chord plays at VELOCITY. Off: keyboard velocity, and pads play louder higher up.");

    // --- arp -----------------------------------------------------------------------
    addToggle (arpBtn, ids::arp, { 926, 632, 298, 40 },
               "Arpeggiate the chord instead of playing it at once. Locks to Live's tempo and grid while "
               "Live is playing, and free-runs at Live's tempo when it isn't.");
    arpBtn.setFontHeight (16.0f);
    addChoice (arpModeChoice, ids::arpMode, "MODE", { 926, 686, 186, 56 },
               "Note order. CHORD restrikes the whole chord every step (rhythmic stabs).");
    addChoice (arpRateChoice, ids::arpRate, "RATE", { 1120, 686, 104, 56 }, "Step length. T = triplet.");
    addKnob (arpOctKnob,  ids::arpOct,  "OCTAVES", { 926, 756, 96, 104 }, "How many octaves the arpeggio climbs.");
    addKnob (arpGateKnob, ids::arpGate, "GATE",    { 1030, 756, 96, 104 }, "How long each arp note lasts, as part of a step.");

    // --- hold ---------------------------------------------------------------------
    addToggle (holdBtn, ids::hold, { 1262, 634, 114, 190 },
               "Latch: the chord keeps playing after you let go, until you play another or turn HOLD off.");
    holdBtn.setFontHeight (22.0f);

    startTimerHz (30);
    timerCallback();
}

PosyPanel::~PosyPanel()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

// ---------------------------------------------------------------------------
void PosyPanel::addKnob (Knob& k, const char* id, const String& caption, Rectangle<int> r, const String& tip)
{
    k.caption = caption;
    k.setTooltip (tip);
    if (auto* param = proc.apvts.getParameter (id))
        k.format = [param] (double v) { return param->getText (param->convertTo0to1 ((float) v), 32); };
    addAndMakeVisible (k);
    k.setBounds (r);
    attachments.push_back (std::make_unique<SliderAttachment> (proc.apvts, id, k));
}

void PosyPanel::addToggle (PanelButton& b, const char* id, Rectangle<int> r, const String& tip)
{
    addButton (b, r, tip, [this, id] (const juce::ModifierKeys&)
    {
        proc.setParam (id, proc.getParam (id) > 0.5f ? 0.0f : 1.0f);
    });
    toggles.push_back ({ &b, id });
}

void PosyPanel::addButton (PanelButton& b, Rectangle<int> r, const String& tip,
                           std::function<void (const juce::ModifierKeys&)> click)
{
    b.setTooltip (tip);
    b.onClick = std::move (click);
    addAndMakeVisible (b);
    b.setBounds (r);
}

void PosyPanel::addChoice (ChoiceDisplay& c, const char* id, const String& caption, Rectangle<int> r, const String& tip)
{
    c.caption = caption;
    c.setTooltip (tip);
    if (! c.getText)
        c.getText = [this, id]
        {
            if (auto* param = proc.apvts.getParameter (id))
                return param->getCurrentValueAsText();
            return String();
        };
    c.onOpen = [this, &c, id] { chooseParam (c, id); };
    addAndMakeVisible (c);
    c.setBounds (r);
    choices.push_back (&c);
}

void PosyPanel::chooseParam (juce::Component& target, const char* id)
{
    auto* param = proc.apvts.getParameter (id);
    if (param == nullptr)
        return;
    const auto range = proc.apvts.getParameterRange (id);
    const int lo = juce::roundToInt (range.start), hi = juce::roundToInt (range.end);
    const int cur = juce::roundToInt (proc.getParam (id));

    juce::PopupMenu m;
    for (int v = lo; v <= hi; ++v)
    {
        const auto text = param->getText (param->convertTo0to1 ((float) v), 32);
        m.addItem (text, true, v == cur, [this, id, v] { proc.setParam (id, (float) v); });
    }
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&target));
}

void PosyPanel::syncTooltips()
{
    const bool on = settings.tooltipsEnabled();
    tipsBtn.setLit (on);
    if ((int) on != tooltipsApplied && onTooltips)
    {
        tooltipsApplied = (int) on;
        onTooltips (on);
    }
}

void PosyPanel::timerCallback()
{
    browser.refresh();
    pads.refresh();
    strip.refresh();
    takeStrip.refresh();
    setDisplay.refresh();
    for (auto* c : choices)
        c->refresh();
    for (auto& [b, id] : toggles)
        b->setLit (proc.getParam (id) > 0.5f);
    recBtn.setLit (proc.takeRecording());
    editBtn.setLit (proc.editing());
    const bool canSave = proc.isCustom() && proc.bankDirty();
    saveBtn.setAlpha (canSave ? 1.0f : 0.4f);
    saveBtn.setEnabled (canSave);

    const bool arpOn = proc.getParam (ids::arp) > 0.5f;
    const String status = ! arpOn ? String() : proc.hostPlaying() ? "SYNCED" : "FREE";
    if (status != arpStatus)
    {
        arpStatus = status;
        repaint (kArpStatus.toNearestInt());
    }
}

// ---------------------------------------------------------------------------
void PosyPanel::paint (juce::Graphics& g)
{
    g.fillAll (colours::panel);

    // Wordmark.
    drawText (g, "POSY", { 24.0f, 8.0f, 240.0f, 44.0f }, 42.0f, colours::cream, true, juce::Justification::centredLeft);
    drawText (g, "CHORD GENERATOR", { 26.0f, 50.0f, 240.0f, 16.0f }, 11.0f, colours::brand, true,
              juce::Justification::centredLeft);

    drawSection (g, kBrowserBox, "SETS");
    drawSection (g, kChordsBox,  "CHORDS");
    drawSection (g, kOutBox,     "OUT");
    drawSection (g, kTakeBox,    "TAKE");
    drawSection (g, kVoiceBox,   "VOICE");
    drawSection (g, kArpBox,     "ARPEGGIO");
    drawSection (g, kHoldBox,    "HOLD");

    if (arpStatus.isNotEmpty())
    {
        const auto r = kArpStatus;
        drawLed (g, { r.getCentreX(), r.getY() + 14.0f }, 6.0f,
                 arpStatus == "SYNCED" ? colours::ledGreen : colours::ledYellow, true, 0.3f);
        drawText (g, arpStatus, r.withTrimmedTop (26.0f).withHeight (16.0f), 12.0f, colours::ink, true);
        drawText (g, arpStatus == "SYNCED" ? "to Live" : "Live stopped", r.withTrimmedTop (42.0f).withHeight (14.0f),
                  10.5f, colours::inkDim, false);
    }
    drawText (g, "latch chords", { 1262.0f, 834.0f, 114.0f, 30.0f }, 11.0f, colours::inkDim, false);
}

// ===========================================================================
PosyEditor::PosyEditor (PosyProcessor& p)
    : AudioProcessorEditor (p), panel (p)
{
    addAndMakeVisible (panel);

    panel.onTooltips = [this] (bool on)
    {
        if (on && tooltips == nullptr)
            tooltips = std::make_unique<juce::TooltipWindow> (this, 700);
        else if (! on)
            tooltips.reset();
    };
    panel.syncTooltips();

    setResizable (true, true);
    const double ratio = (double) PosyPanel::designW / PosyPanel::designH;
    setResizeLimits (900, (int) (900 / ratio), 2600, (int) (2600 / ratio));
    if (auto* c = getConstrainer())
        c->setFixedAspectRatio (ratio);
    setSize (1200, (int) (1200 / ratio));
}

void PosyEditor::paint (juce::Graphics& g)
{
    g.fillAll (colours::panel);
}

void PosyEditor::resized()
{
    const float s = (float) getWidth() / (float) PosyPanel::designW;
    panel.setTransform (juce::AffineTransform::scale (s));
    panel.setBounds (0, 0, PosyPanel::designW, PosyPanel::designH);
}
