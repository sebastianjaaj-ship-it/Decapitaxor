// Headless test: runs audio through the processor and renders the editor to PNG.
#include <JuceHeader.h>
#include "../Source/PluginProcessor.h"
#include "../Source/PluginEditor.h"

static void setParam (DecapitaxorProcessor& p, const juce::String& id, float v)
{
    auto* prm = p.apvts.getParameter (id);
    prm->setValueNotifyingHost (prm->convertTo0to1 (v));
}

static double runSine (DecapitaxorProcessor& p, double fs, float freq, float amp, int blocks, double* inRmsOut = nullptr)
{
    const int bs = 512;
    juce::AudioBuffer<float> buf (2, bs);
    juce::MidiBuffer midi;
    double phase = 0, sumOut = 0, sumIn = 0; long cnt = 0;
    for (int b = 0; b < blocks; ++b)
    {
        for (int i = 0; i < bs; ++i)
        {
            const float s = amp * (float) std::sin (phase);
            phase += 2 * juce::MathConstants<double>::pi * freq / fs;
            buf.setSample (0, i, s); buf.setSample (1, i, s);
            if (b >= blocks / 2) sumIn += s * s;
        }
        p.processBlock (buf, midi);
        if (b >= blocks / 2)
            for (int i = 0; i < bs; ++i)
            {
                const float o = buf.getSample (0, i);
                if (! std::isfinite (o)) { std::printf ("NON-FINITE OUTPUT!\n"); std::exit (1); }
                sumOut += o * o; ++cnt;
            }
    }
    if (inRmsOut) *inRmsOut = std::sqrt (sumIn / cnt);
    return std::sqrt (sumOut / cnt);
}

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI init;
    const double fs = 48000.0;
    DecapitaxorProcessor p;
    p.setPlayConfigDetails (2, 2, fs, 512);
    p.prepareToPlay (fs, 512);
    std::printf ("latency: %d samples\n", p.getLatencySamples());

    const char* names[] = { "Tape", "Console", "Iron", "Triode", "Pentode" };
    for (int st = 0; st < 5; ++st)
    {
        setParam (p, ParamID::style, (float) st);
        for (float drv : { 0.0f, 5.0f, 10.0f })
        {
            setParam (p, ParamID::drive, drv);
            setParam (p, ParamID::autoGain, 0.0f);
            double in;
            const double raw = runSine (p, fs, 220.0f, 0.25f, 120, &in);
            setParam (p, ParamID::autoGain, 1.0f);
            const double au = runSine (p, fs, 220.0f, 0.25f, 200);
            std::printf ("%-8s drive %4.1f  in %6.1f dB  out %6.1f dB  auto %6.1f dB\n", names[st], drv,
                         juce::Decibels::gainToDecibels (in), juce::Decibels::gainToDecibels (raw), juce::Decibels::gainToDecibels (au));
        }
    }

    // Mix = 0 must be a clean (delayed) pass-through
    setParam (p, ParamID::mix, 0.0f); setParam (p, ParamID::punish, 1.0f);
    double in; const double dry = runSine (p, fs, 1000.0f, 0.3f, 100, &in);
    std::printf ("mix 0: in %.4f out %.4f\n", in, dry);
    setParam (p, ParamID::mix, 100.0f); setParam (p, ParamID::punish, 0.0f);

    // Filters
    setParam (p, ParamID::drive, 0.0f); setParam (p, ParamID::autoGain, 0.0f);
    setParam (p, ParamID::highCut, 2000.0f); setParam (p, ParamID::steep, 1.0f);
    std::printf ("highcut 2k steep, 8k tone: %.1f dB\n", juce::Decibels::gainToDecibels (runSine (p, fs, 8000.0f, 0.1f, 100) / 0.0707));
    setParam (p, ParamID::highCut, 20000.0f);
    setParam (p, ParamID::lowCut, 500.0f);
    std::printf ("lowcut 500, 60Hz tone: %.1f dB\n", juce::Decibels::gainToDecibels (runSine (p, fs, 60.0f, 0.1f, 100) / 0.0707));

    // Silence after processing must stay silent
    juce::AudioBuffer<float> z (2, 512); z.clear(); juce::MidiBuffer m;
    for (int i = 0; i < 200; ++i) { z.clear(); p.processBlock (z, m); }
    std::printf ("silence peak: %g\n", z.getMagnitude (0, 512));

    // Render editor
    setParam (p, ParamID::lowCut, 20.0f); setParam (p, ParamID::drive, 6.2f); setParam (p, ParamID::style, 3.0f);
    setParam (p, ParamID::autoGain, 1.0f);
    runSine (p, fs, 220.0f, 0.3f, 50);
    std::unique_ptr<juce::AudioProcessorEditor> ed (p.createEditor());
    auto img = ed->createComponentSnapshot (ed->getLocalBounds(), true, 2.0f);
    juce::File out (argc > 1 ? argv[1] : "editor.png");
    out.deleteFile();
    juce::FileOutputStream os (out);
    juce::PNGImageFormat().writeImageToStream (img, os);
    std::printf ("wrote %s\n", out.getFullPathName().toRawUTF8());
    return 0;
}
