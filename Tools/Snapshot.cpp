// Snapshot.cpp — dev tool: render Posy's panel to a PNG without a host.
//   PosySnapshot out.png [set-number] [key-to-press]
// Plays a few chords first so the take, the pads and the note strip have
// something to show.
#include "PluginProcessor.h"
#include "PluginEditor.h"

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI gui;
    if (argc < 2)
    {
        std::printf ("usage: PosySnapshot out.png [set] [key]\n");
        return 1;
    }

    PosyProcessor proc;
    proc.setPlayConfigDetails (0, 2, 48000.0, 512);
    proc.prepareToPlay (48000.0, 512);
    if (argc > 2)
        proc.loadSet (juce::String (argv[2]).getIntValue() - 1);

    juce::AudioBuffer<float> audio (2, 512);
    juce::MidiBuffer midi;
    auto block = [&] { midi.clear(); proc.processBlock (audio, midi); };
    block();

    for (int key : { 9, 5, 0, 7 })
    {
        proc.pressPad (key, 100); block();
        juce::MessageManager::getInstance()->runDispatchLoopUntil (80);
        proc.releasePad (key); block();
    }
    const int key = argc > 3 ? juce::String (argv[3]).getIntValue() : 2;
    proc.pressPad (key, 100);
    block();
    juce::MessageManager::getInstance()->runDispatchLoopUntil (120);

    std::unique_ptr<juce::AudioProcessorEditor> ed (proc.createEditor());
    ed->setSize (PosyPanel::designW, PosyPanel::designH);
    juce::MessageManager::getInstance()->runDispatchLoopUntil (120);
    const auto img = ed->createComponentSnapshot (ed->getLocalBounds(), true, 1.0f);

    juce::File out (juce::File::getCurrentWorkingDirectory().getChildFile (argv[1]));
    out.deleteFile();
    juce::FileOutputStream stream (out);
    juce::PNGImageFormat().writeImageToStream (img, stream);
    std::printf ("wrote %s\n", out.getFullPathName().toRawUTF8());
    ed.reset();
    return 0;
}
