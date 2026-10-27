#include "Player.h"

#include <algorithm>
#include <cmath>

namespace posy
{
namespace
{
constexpr size_t kMaxPending = 1000;

int clampi (int v, int lo, int hi) { return std::min (hi, std::max (lo, v)); }
} // namespace

Player::Player()
{
    pending.reserve (kMaxPending + 32);
}

bool Player::keyIsDown (int key) const noexcept
{
    for (int i = 0; i < numHeld; ++i)
        if (held[(size_t) i].key == key)
            return true;
    return false;
}

// ---------------------------------------------------------------------------
// Scheduling
// ---------------------------------------------------------------------------
void Player::schedule (int64_t time, int note, int velocity, bool on, bool chordOn)
{
    if (note < 0 || note > 127 || pending.size() >= kMaxPending)
        return;
    // Keep the queue in time order; events at the same time stay in the order
    // they were scheduled, so an off scheduled before an on goes out first.
    auto it = std::upper_bound (pending.begin(), pending.end(), time,
                                [] (int64_t t, const Pending& p) { return t < p.time; });
    pending.insert (it, Pending { time, note, clampi (velocity, 1, 127), on, chordOn });
}

void Player::cancelChordOns()
{
    pending.erase (std::remove_if (pending.begin(), pending.end(), [this] (const Pending& p)
    {
        if (p.on && p.chordOn)
        {
            chordNotes[(size_t) p.note] = false;
            return true;
        }
        return false;
    }), pending.end());
}

void Player::flushUntil (int64_t end, std::vector<NoteOut>& out)
{
    size_t i = 0;
    for (; i < pending.size() && pending[i].time < end; ++i)
    {
        const auto& p = pending[i];
        const int offset = (int) std::max<int64_t> (0, p.time - now);
        auto& on = outOn[(size_t) p.note];
        if (p.on)
        {
            if (on)
                out.push_back ({ offset, p.note, 0, false });   // restrike
            out.push_back ({ offset, p.note, p.velocity, true });
            on = true;
        }
        else if (on)
        {
            out.push_back ({ offset, p.note, 0, false });
            on = false;
        }
    }
    pending.erase (pending.begin(), pending.begin() + (std::ptrdiff_t) i);
}

void Player::allOff (int64_t time)
{
    pending.clear();
    chordNotes.fill (false);
    for (int n = 0; n < 128; ++n)
        if (outOn[(size_t) n])
            schedule (time, n, 0, false);
}

// ---------------------------------------------------------------------------
// Chords
// ---------------------------------------------------------------------------
Voicing Player::voicingFor (int key, int octave) const
{
    const ChordDef* chords = s.chords != nullptr ? s.chords : chordSet (s.setIndex).chords;
    return voice (chords[clampi (key, 0, kKeys - 1)], s.voice, s.followOctave ? octave : 0);
}

void Player::chordTo (const Voicing& v, int64_t time, bool strum)
{
    cancelChordOns();

    std::array<int, kMaxVoice> fresh {};
    int numFresh = 0;
    for (int n = 0; n < 128; ++n)
    {
        if (! chordNotes[(size_t) n])
            continue;
        if (s.legato && v.contains (n))
            continue;                        // shared note: let it ring
        schedule (time, n, 0, false);
        chordNotes[(size_t) n] = false;
    }
    for (int i = 0; i < v.count; ++i)
    {
        const int n = v.notes[(size_t) i];
        if (! chordNotes[(size_t) n])
            fresh[(size_t) numFresh++] = n;
    }
    if (s.strumDown)
        std::reverse (fresh.begin(), fresh.begin() + numFresh);

    const double gap = strum ? s.strumMs * info.sampleRate / 1000.0 : 0.0;
    for (int i = 0; i < numFresh; ++i)
    {
        const int n = fresh[(size_t) i];
        schedule (time + (int64_t) std::llround (gap * i), n, curVel, true, true);
        chordNotes[(size_t) n] = true;
    }
    chord = v;
}

void Player::engage (int64_t time, int key, int octave, int velocity, bool report)
{
    const bool wasActive = curKey >= 0;
    curKey = key;
    curOct = octave;
    curVel = clampi (velocity, 1, 127);
    const Voicing v = voicingFor (key, octave);

    if (s.arp)
    {
        chord = v;
        arpStep = 0;
        lastRandom = -1;
        if (! wasActive)
        {
            // Starting from silence: play straight away, then fall in with the
            // grid (skipping a grid step that would land right on top of us).
            arpFire (time);
            const double stepQ = kArpRateQuarters[clampi (s.arpRate, 0, kNumArpRates - 1)];
            if (info.playing)
            {
                const double spq = info.sampleRate * 60.0 / std::max (1.0, info.bpm);
                const double p = info.ppq + (double) (time - now) / spq;
                const auto kNext = (int64_t) std::ceil (p / stepQ - 1e-9);
                const double gap = (double) kNext * stepQ - p;
                lastFiredK = gap < 0.5 * stepQ ? kNext : kNext - 1;
            }
            else
            {
                nextStepTime = (double) time + stepSamples();
            }
        }
        // Already running: keep the rhythm, the next step plays the new chord.
    }
    else
    {
        chordTo (v, time, true);
    }

    if (report)
    {
        lastStarted.serial++;
        lastStarted.setIndex = s.setIndex;
        lastStarted.key = key;
        lastStarted.octave = s.followOctave ? octave : 0;
        lastStarted.voicing = v;
    }
}

void Player::release (int64_t time)
{
    if (! s.arp)
    {
        cancelChordOns();
        for (int n = 0; n < 128; ++n)
            if (chordNotes[(size_t) n])
            {
                schedule (time, n, 0, false);
                chordNotes[(size_t) n] = false;
            }
    }
    // Arp: the last note finishes its gate on its own.
    curKey = -1;
    chord = {};
}

// ---------------------------------------------------------------------------
// Arpeggiator
// ---------------------------------------------------------------------------
double Player::stepSamples() const
{
    const double spq = info.sampleRate * 60.0 / std::max (1.0, info.bpm);
    return std::max (1.0, spq * kArpRateQuarters[clampi (s.arpRate, 0, kNumArpRates - 1)]);
}

void Player::arpFire (int64_t time)
{
    if (chord.count == 0)
        return;

    const int octs = clampi (s.arpOctaves, 1, 4);
    const double step = stepSamples();
    const auto gate = (int64_t) std::max (1.0, std::floor (step * clampi (s.arpGate, 5, 100) / 100.0));

    if (s.arpMode == ArpMode::chord)
    {
        const int lift = 12 * (int) (arpStep % (uint32_t) octs);
        for (int i = 0; i < chord.count; ++i)
        {
            const int n = chord.notes[(size_t) i] + lift;
            if (n > 127) continue;
            schedule (time, n, curVel, true);
            schedule (time + gate, n, 0, false);
        }
        ++arpStep;
        return;
    }

    std::array<int, kMaxVoice * 4> seq {};
    int n = 0;
    for (int o = 0; o < octs; ++o)
        for (int i = 0; i < chord.count; ++i)
        {
            const int note = chord.notes[(size_t) i] + 12 * o;
            if (note <= 127)
                seq[(size_t) n++] = note;
        }
    if (n == 0)
        return;

    const auto st = arpStep;
    int idx = 0;
    switch (s.arpMode)
    {
        case ArpMode::up:   idx = (int) (st % (uint32_t) n); break;
        case ArpMode::down: idx = n - 1 - (int) (st % (uint32_t) n); break;
        case ArpMode::upDown:
        case ArpMode::downUp:
        {
            if (n < 2) { idx = 0; break; }
            const int period = 2 * n - 2;
            const int p = (int) (st % (uint32_t) period);
            idx = p < n ? p : period - p;
            if (s.arpMode == ArpMode::downUp)
                idx = n - 1 - idx;
            break;
        }
        case ArpMode::upDownInc:
        {
            const int period = 2 * n;
            const int p = (int) (st % (uint32_t) period);
            idx = p < n ? p : period - 1 - p;
            break;
        }
        case ArpMode::converge:
        {
            const int p = (int) (st % (uint32_t) n);
            idx = (p % 2 == 0) ? p / 2 : n - 1 - p / 2;
            break;
        }
        case ArpMode::random:
        {
            rng ^= rng << 13; rng ^= rng >> 17; rng ^= rng << 5;
            idx = (int) (rng % (uint32_t) n);
            if (n > 1 && idx == lastRandom)
                idx = (idx + 1) % n;
            lastRandom = idx;
            break;
        }
        case ArpMode::chord: break;
    }

    const int note = seq[(size_t) idx];
    schedule (time, note, curVel, true);
    schedule (time + gate, note, 0, false);
    ++arpStep;
}

void Player::arpRun (int64_t from, int64_t to)
{
    if (! s.arp || curKey < 0 || chord.count == 0 || to <= from)
        return;

    const double stepQ = kArpRateQuarters[clampi (s.arpRate, 0, kNumArpRates - 1)];
    if (info.playing)
    {
        // Locked to the host's grid.
        const double spq = info.sampleRate * 60.0 / std::max (1.0, info.bpm);
        const double pFrom = info.ppq + (double) (from - now) / spq;
        const double pTo   = info.ppq + (double) (to - now) / spq;
        for (auto k = (int64_t) std::ceil (pFrom / stepQ - 1e-9); (double) k * stepQ < pTo - 1e-9; ++k)
        {
            if (k <= lastFiredK)
                continue;
            lastFiredK = k;
            auto t = now + (int64_t) std::llround (((double) k * stepQ - info.ppq) * spq);
            t = std::clamp (t, from, to - 1);
            if (t < skipBefore)
                continue;
            arpFire (t);
        }
    }
    else
    {
        // Free-running at the host tempo.
        const double step = stepSamples();
        if (nextStepTime < (double) from - step)
            nextStepTime = (double) from;
        while (nextStepTime < (double) to)
        {
            arpFire (std::max (from, (int64_t) std::llround (nextStepTime)));
            nextStepTime += step;
        }
    }
}

// ---------------------------------------------------------------------------
void Player::process (const BlockInfo& block, const PlayerSettings& settings,
                      const KeyEvent* events, int numEvents, std::vector<NoteOut>& out)
{
    info = block;
    s = settings;
    s.setIndex   = clampi (s.setIndex, 0, numChordSets() - 1);
    s.arpOctaves = clampi (s.arpOctaves, 1, 4);

    const int64_t end = now + std::max (0, info.numSamples);

    if (panicRequested)
    {
        panicRequested = false;
        allOff (now);
        numHeld = 0;
        curKey = -1;
        chord = {};
    }

    // Arp switched on or off: silence what's sounding and carry on in the
    // other mode with the same chord.
    if (s.arp != arpWasOn)
    {
        arpWasOn = s.arp;
        allOff (now);
        if (curKey >= 0)
        {
            const int k = curKey;
            curKey = -1;                        // engage() starts it fresh
            engage (now, k, curOct, curVel, false);
        }
    }

    // Transport: hand the arp between its free and synced clocks.
    if (s.arp && curKey >= 0)
    {
        const double stepQ = kArpRateQuarters[clampi (s.arpRate, 0, kNumArpRates - 1)];
        const auto kStart = (int64_t) std::ceil (info.ppq / stepQ - 1e-9);
        if (info.playing && ! wasPlaying)
            lastFiredK = kStart - 1;
        else if (info.playing && lastFiredK > kStart)
            lastFiredK = kStart - 1;            // the host looped or jumped back
        else if (! info.playing && wasPlaying)
            nextStepTime = (double) now + stepSamples();
    }
    wasPlaying = info.playing;

    // HOLD switched off with nothing held: let the chord go.
    if (! s.hold && numHeld == 0 && curKey >= 0)
        release (now);

    // A knob moved or the set changed under a sounding chord: re-voice it.
    if (curKey >= 0)
    {
        const Voicing v = voicingFor (curKey, curOct);
        if (v != chord)
        {
            if (s.arp)
                chord = v;
            else
                chordTo (v, now, false);
        }
    }
    lastSet = s.setIndex;

    int64_t pos = now;
    for (int e = 0; e < numEvents; ++e)
    {
        const auto& ev = events[e];
        const int64_t t = std::max (pos, now + clampi (ev.offset, 0, std::max (0, info.numSamples - 1)));
        arpRun (pos, t);
        flushUntil (t, out);
        pos = t;

        if (ev.velocityOnly)
        {
            for (int i = 0; i < numHeld; ++i)
                if (held[(size_t) i].source == ev.source)
                {
                    held[(size_t) i].velocity = clampi (ev.velocity, 1, 127);
                    if (i == numHeld - 1 && ! s.fixedVelocity)
                        curVel = held[(size_t) i].velocity;
                }
            continue;
        }

        const int key = clampi (ev.key, 0, kKeys - 1);
        if (ev.on)
        {
            for (int i = 0; i < numHeld; ++i)
                if (held[(size_t) i].source == ev.source)
                {
                    std::move (held.begin() + i + 1, held.begin() + numHeld, held.begin() + i);
                    --numHeld;
                    break;
                }
            const int vel = s.fixedVelocity ? s.velocity : ev.velocity;
            if (numHeld < (int) held.size())
                held[(size_t) numHeld++] = { ev.source, key, ev.octave, vel };
            engage (t, key, ev.octave, vel, true);
        }
        else
        {
            int idx = -1;
            for (int i = 0; i < numHeld; ++i)
                if (held[(size_t) i].source == ev.source)
                    idx = i;
            if (idx < 0)
                continue;
            const bool wasTop = idx == numHeld - 1;
            std::move (held.begin() + idx + 1, held.begin() + numHeld, held.begin() + idx);
            --numHeld;
            if (! wasTop)
                continue;
            if (numHeld > 0)
            {
                const auto& top = held[(size_t) numHeld - 1];
                if (top.key != curKey || top.octave != curOct)
                    engage (t, top.key, top.octave, top.velocity, true);
            }
            else if (! s.hold)
            {
                release (t);
            }
        }
    }

    arpRun (pos, end);
    flushUntil (end, out);
    now = end;
}
} // namespace posy
