#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace posy;

namespace
{
constexpr int kMaxTake = 64;
const juce::Identifier kTakeTag { "Take" };
const juce::Identifier kChordTag { "Chord" };
} // namespace

PosyProcessor::PosyProcessor()
    // An instrument with an always-silent audio output: Live loads VST3 MIDI
    // generators as instruments, and hosts expect instruments to have one.
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PosyState", createParameterLayout())
{
    for (auto* p : getParameters())
        if (auto* withId = dynamic_cast<juce::AudioProcessorParameterWithID*> (p))
            params[withId->paramID] = apvts.getRawParameterValue (withId->paramID);

    for (auto& n : uiNotes) n.store (0);
    events.reserve (1024);
    notesOut.reserve (4096);
    startTimerHz (20);
}

PosyProcessor::~PosyProcessor()
{
    stopTimer();
}

bool PosyProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::stereo() || out == juce::AudioChannelSet::mono();
}

void PosyProcessor::prepareToPlay (double, int)
{
    passthrough.ensureSize (4096);
}

PlayerSettings PosyProcessor::readSettings() const
{
    auto geti = [this] (const char* id) { return juce::roundToInt (getParam (id)); };
    auto getb = [this] (const char* id) { return getParam (id) > 0.5f; };

    PlayerSettings s;
    s.setIndex        = geti (ids::chordSet) - 1;
    s.voice.transpose = geti (ids::key);
    s.voice.octave    = geti (ids::octave);
    s.voice.invert    = geti (ids::invert);
    s.voice.spread    = (Spread) juce::jlimit (0, 2, geti (ids::spread));
    s.voice.bass      = getb (ids::bass);
    s.strumMs         = geti (ids::strum);
    s.strumDown       = getb (ids::strumDown);
    s.velocity        = geti (ids::velocity);
    s.fixedVelocity   = getb (ids::fixedVel);
    s.legato          = getb (ids::legato);
    s.followOctave    = getb (ids::followOct);
    s.hold            = getb (ids::hold);
    s.arp             = getb (ids::arp);
    s.arpMode         = (ArpMode) juce::jlimit (0, kNumArpModes - 1, geti (ids::arpMode));
    s.arpRate         = juce::jlimit (0, kNumArpRates - 1, geti (ids::arpRate));
    s.arpOctaves      = geti (ids::arpOct);
    s.arpGate         = geti (ids::arpGate);
    return s;
}

Voicing PosyProcessor::previewVoicing (int key) const
{
    const auto s = readSettings();
    return voice (chordSet (s.setIndex).chords[juce::jlimit (0, kKeys - 1, key)], s.voice);
}

// ---------------------------------------------------------------------------
void PosyProcessor::processBlock (juce::AudioBuffer<float>& audio, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    const int numSamples = audio.getNumSamples();
    audio.clear();   // MIDI only

    if (panicRequested.exchange (false))
        player.panic();

    // --- keys: the panel's pads first, then the incoming MIDI ----------------
    events.clear();
    {
        const auto scope = padFifo.read (padFifo.getNumReady());
        for (int i = 0; i < scope.blockSize1; ++i) events.push_back (padEvents[(size_t) (scope.startIndex1 + i)]);
        for (int i = 0; i < scope.blockSize2; ++i) events.push_back (padEvents[(size_t) (scope.startIndex2 + i)]);
    }

    // Notes are taken over: each key of the keyboard picks a chord of the set,
    // and its octave moves the chord (KEYBOARD OCTAVE). Everything else -- mod
    // wheel, pitch bend, sustain -- passes straight through to the instrument.
    passthrough.clear();
    for (const auto meta : midi)
    {
        const auto msg = meta.getMessage();
        const int pos = juce::jlimit (0, juce::jmax (0, numSamples - 1), meta.samplePosition);
        if (msg.isNoteOnOrOff())
        {
            const int n = msg.getNoteNumber();
            KeyEvent e;
            e.offset   = pos;
            e.on       = msg.isNoteOn();
            e.source   = n;
            e.key      = n % 12;
            e.octave   = n / 12 - 5;         // MIDI 60 (C3 in Live) is the home octave
            e.velocity = msg.getVelocity();
            events.push_back (e);
        }
        else if (msg.isAllNotesOff() || msg.isAllSoundOff())
        {
            player.panic();
            passthrough.addEvent (msg, pos);
        }
        else
        {
            passthrough.addEvent (msg, pos);
        }
    }
    std::stable_sort (events.begin(), events.end(),
                      [] (const KeyEvent& a, const KeyEvent& b) { return a.offset < b.offset; });

    // --- transport -------------------------------------------------------------
    BlockInfo info;
    info.numSamples = numSamples;
    info.sampleRate = getSampleRate() > 0.0 ? getSampleRate() : 44100.0;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
        {
            info.playing = pos->getIsPlaying();
            info.bpm     = pos->getBpm().orFallback (120.0);
            info.ppq     = pos->getPpqPosition().orFallback (0.0);
        }

    // --- play --------------------------------------------------------------------
    notesOut.clear();
    player.process (info, readSettings(), events.data(), (int) events.size(), notesOut);

    const int channel = juce::jlimit (1, 16, juce::roundToInt (getParam (ids::channel)));
    midi.clear();
    midi.addEvents (passthrough, 0, numSamples, 0);
    for (const auto& n : notesOut)
    {
        if (n.on)
            midi.addEvent (juce::MidiMessage::noteOn (channel, n.note, (juce::uint8) n.velocity), n.offset);
        else
            midi.addEvent (juce::MidiMessage::noteOff (channel, n.note), n.offset);
        sounding[(size_t) n.note] = n.on;
    }

    // --- panel state ---------------------------------------------------------------
    std::array<uint32_t, 4> bits {};
    for (int i = 0; i < 128; ++i)
        if (sounding[(size_t) i])
            bits[(size_t) (i >> 5)] |= 1u << (i & 31);
    for (size_t i = 0; i < 4; ++i)
        uiNotes[i].store (bits[i], std::memory_order_relaxed);

    uint32_t down = 0;
    for (int k = 0; k < kKeys; ++k)
        if (player.keyIsDown (k))
            down |= 1u << k;
    uiKeysDown.store (down, std::memory_order_relaxed);
    uiKey.store (player.currentKey(), std::memory_order_relaxed);
    uiPlaying.store (info.playing, std::memory_order_relaxed);

    const auto& st = player.started();
    if (st.serial != lastSerial)
    {
        lastSerial = st.serial;
        const auto scope = startedFifo.write (1);
        if (scope.blockSize1 > 0)
            startedEvents[(size_t) scope.startIndex1] = st;
    }
}

// ---------------------------------------------------------------------------
// Panel
// ---------------------------------------------------------------------------
void PosyProcessor::pressPad (int key, int velocity)
{
    const juce::SpinLock::ScopedLockType lock (padWriteLock);
    const auto scope = padFifo.write (1);
    if (scope.blockSize1 > 0)
    {
        KeyEvent e;
        e.on = true; e.source = kPadSource + key; e.key = key; e.octave = 0;
        e.velocity = juce::jlimit (1, 127, velocity);
        padEvents[(size_t) scope.startIndex1] = e;
    }
}

void PosyProcessor::releasePad (int key)
{
    const juce::SpinLock::ScopedLockType lock (padWriteLock);
    const auto scope = padFifo.write (1);
    if (scope.blockSize1 > 0)
    {
        KeyEvent e;
        e.on = false; e.source = kPadSource + key; e.key = key;
        padEvents[(size_t) scope.startIndex1] = e;
    }
}

void PosyProcessor::setParam (const juce::String& id, float plainValue)
{
    if (auto* p = apvts.getParameter (id))
    {
        p->beginChangeGesture();
        p->setValueNotifyingHost (p->convertTo0to1 (plainValue));
        p->endChangeGesture();
    }
}

// ---------------------------------------------------------------------------
// Take
// ---------------------------------------------------------------------------
void PosyProcessor::timerCallback()
{
    const auto scope = startedFifo.read (startedFifo.getNumReady());
    auto take = [this] (const Started& st)
    {
        if (! recordTake || st.key < 0 || st.voicing.count == 0)
            return;
        TakeChord c;
        c.setNumber = st.setIndex + 1;
        c.key = st.key;
        c.name = chordSet (st.setIndex).chords[st.key].name;
        for (int i = 0; i < st.voicing.count; ++i)
            c.notes.add (st.voicing.notes[(size_t) i]);

        // The same chord again straight away (a restrike, a held repeat) is
        // one chord in the take.
        if (! takeChords.isEmpty() && takeChords.getLast().notes == c.notes)
            return;
        takeChords.add (c);
        while (takeChords.size() > kMaxTake)
            takeChords.remove (0);
        ++takeVersion;
    };
    for (int i = 0; i < scope.blockSize1; ++i) take (startedEvents[(size_t) (scope.startIndex1 + i)]);
    for (int i = 0; i < scope.blockSize2; ++i) take (startedEvents[(size_t) (scope.startIndex2 + i)]);
}

juce::File PosyProcessor::writeTakeMidi() const
{
    constexpr int ppq = 960;
    juce::MidiMessageSequence seq;
    seq.addEvent (juce::MidiMessage::textMetaEvent (3, "Posy take"), 0.0);
    double t = 0.0;
    const double len = (double) beatsPerTakeChord * ppq;
    const int velocity = juce::roundToInt (getParam (ids::velocity));
    for (const auto& c : takeChords)
    {
        for (int n : c.notes)
        {
            seq.addEvent (juce::MidiMessage::noteOn (1, n, (juce::uint8) velocity), t);
            seq.addEvent (juce::MidiMessage::noteOff (1, n), t + len - 1.0);
        }
        t += len;
    }
    seq.updateMatchedPairs();

    juce::MidiFile file;
    file.setTicksPerQuarterNote (ppq);
    file.addTrack (seq);

    auto dir = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("Posy");
    dir.createDirectory();
    const auto first = takeChords.isEmpty() ? juce::String ("Empty") : takeChords.getFirst().name;
    auto out = dir.getChildFile (juce::File::createLegalFileName (
        "Posy " + juce::String (takeChords.size()) + " chords from " + first + ".mid"));
    out.deleteFile();
    if (auto stream = out.createOutputStream())
        file.writeTo (*stream);
    return out;
}

// ---------------------------------------------------------------------------
// State
// ---------------------------------------------------------------------------
void PosyProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    for (int i = state.getNumChildren(); --i >= 0;)
        if (state.getChild (i).hasType (kTakeTag))
            state.removeChild (i, nullptr);

    juce::ValueTree take (kTakeTag);
    take.setProperty ("record", recordTake, nullptr);
    take.setProperty ("beats", beatsPerTakeChord, nullptr);
    for (const auto& c : takeChords)
    {
        juce::StringArray notes;
        for (int n : c.notes) notes.add (juce::String (n));
        juce::ValueTree t (kChordTag);
        t.setProperty ("set", c.setNumber, nullptr);
        t.setProperty ("key", c.key, nullptr);
        t.setProperty ("name", c.name, nullptr);
        t.setProperty ("notes", notes.joinIntoString (","), nullptr);
        take.appendChild (t, nullptr);
    }
    state.appendChild (take, nullptr);
    state.setProperty ("stateVersion", 1, nullptr);

    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void PosyProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    auto xml = getXmlFromBinary (data, sizeInBytes);
    if (xml == nullptr)
        return;
    auto state = juce::ValueTree::fromXml (*xml);
    if (! state.hasType (apvts.state.getType()))
        return;

    takeChords.clear();
    if (auto take = state.getChildWithName (kTakeTag); take.isValid())
    {
        recordTake = (bool) take.getProperty ("record", true);
        beatsPerTakeChord = juce::jlimit (1, 16, (int) take.getProperty ("beats", 4));
        for (auto t : take)
        {
            TakeChord c;
            c.setNumber = t["set"];
            c.key = t["key"];
            c.name = t["name"].toString();
            for (auto& s : juce::StringArray::fromTokens (t["notes"].toString(), ",", {}))
                c.notes.add (juce::jlimit (0, 127, s.getIntValue()));
            if (! c.notes.isEmpty())
                takeChords.add (c);
        }
        state.removeChild (take, nullptr);
    }
    ++takeVersion;
    apvts.replaceState (state);
}

juce::AudioProcessorEditor* PosyProcessor::createEditor()
{
    return new PosyEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PosyProcessor();
}
