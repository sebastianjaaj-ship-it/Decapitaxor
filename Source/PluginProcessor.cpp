#include "PluginProcessor.h"
#include "PluginEditor.h"

using APVTS = juce::AudioProcessorValueTreeState;

static const juce::StringArray kStyleNames { "Tape", "Console", "Transformer", "Triode", "Pentode" };

APVTS::ParameterLayout DecapitaxorProcessor::createLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;
    using PID = juce::ParameterID;

    auto hz = [] (float v, int) { return v >= 1000.0f ? juce::String (v / 1000.0f, 1) + " kHz"
                                                      : juce::String (juce::roundToInt (v)) + " Hz"; };

    p.push_back (std::make_unique<juce::AudioParameterFloat> (PID { ParamID::drive, 1 }, "Drive",
        juce::NormalisableRange<float> (0.0f, 10.0f, 0.01f), 3.0f,
        juce::AudioParameterFloatAttributes().withStringFromValueFunction ([] (float v, int) { return juce::String (v, 1); })));

    p.push_back (std::make_unique<juce::AudioParameterBool> (PID { ParamID::punish, 1 }, "Punish", false));

    p.push_back (std::make_unique<juce::AudioParameterChoice> (PID { ParamID::style, 1 }, "Style", kStyleNames, 0));

    juce::NormalisableRange<float> lowR (20.0f, 1000.0f, 0.1f);
    lowR.setSkewForCentre (120.0f);
    p.push_back (std::make_unique<juce::AudioParameterFloat> (PID { ParamID::lowCut, 1 }, "Low Cut", lowR, 20.0f,
        juce::AudioParameterFloatAttributes().withLabel ("Hz").withStringFromValueFunction (hz)));

    p.push_back (std::make_unique<juce::AudioParameterBool> (PID { ParamID::thump, 1 }, "Thump", false));

    p.push_back (std::make_unique<juce::AudioParameterFloat> (PID { ParamID::tone, 1 }, "Tone",
        juce::NormalisableRange<float> (-1.0f, 1.0f, 0.001f), 0.0f,
        juce::AudioParameterFloatAttributes().withStringFromValueFunction ([] (float v, int)
        {
            if (std::abs (v) < 0.01f) return juce::String ("Flat");
            return (v < 0 ? juce::String ("Dark ") : juce::String ("Bright ")) + juce::String (juce::roundToInt (std::abs (v) * 100)) + "%";
        })));

    juce::NormalisableRange<float> highR (1000.0f, 20000.0f, 1.0f);
    highR.setSkewForCentre (5000.0f);
    p.push_back (std::make_unique<juce::AudioParameterFloat> (PID { ParamID::highCut, 1 }, "High Cut", highR, 20000.0f,
        juce::AudioParameterFloatAttributes().withLabel ("Hz").withStringFromValueFunction ([hz] (float v, int n)
        { return v >= 19990.0f ? juce::String ("Off") : hz (v, n); })));

    p.push_back (std::make_unique<juce::AudioParameterBool> (PID { ParamID::steep, 1 }, "Steep", false));

    p.push_back (std::make_unique<juce::AudioParameterFloat> (PID { ParamID::output, 1 }, "Output",
        juce::NormalisableRange<float> (-24.0f, 6.0f, 0.01f), 0.0f,
        juce::AudioParameterFloatAttributes().withLabel ("dB").withStringFromValueFunction ([] (float v, int)
        { return juce::String (v, 1) + " dB"; })));

    p.push_back (std::make_unique<juce::AudioParameterBool> (PID { ParamID::autoGain, 1 }, "Auto", true));

    p.push_back (std::make_unique<juce::AudioParameterFloat> (PID { ParamID::mix, 1 }, "Mix",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 100.0f,
        juce::AudioParameterFloatAttributes().withLabel ("%").withStringFromValueFunction ([] (float v, int)
        { return juce::String (juce::roundToInt (v)) + " %"; })));

    return { p.begin(), p.end() };
}

DecapitaxorProcessor::DecapitaxorProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "DECAPITAXOR", createLayout())
{
    pDrive   = apvts.getRawParameterValue (ParamID::drive);
    pPunish  = apvts.getRawParameterValue (ParamID::punish);
    pStyle   = apvts.getRawParameterValue (ParamID::style);
    pLowCut  = apvts.getRawParameterValue (ParamID::lowCut);
    pThump   = apvts.getRawParameterValue (ParamID::thump);
    pTone    = apvts.getRawParameterValue (ParamID::tone);
    pHighCut = apvts.getRawParameterValue (ParamID::highCut);
    pSteep   = apvts.getRawParameterValue (ParamID::steep);
    pOutput  = apvts.getRawParameterValue (ParamID::output);
    pAuto    = apvts.getRawParameterValue (ParamID::autoGain);
    pMix     = apvts.getRawParameterValue (ParamID::mix);
}

bool DecapitaxorProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;
    return layouts.getMainInputChannelSet() == out;
}

void DecapitaxorProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    sr = sampleRate;
    const int numCh = juce::jmax (1, juce::jmin (2, getTotalNumOutputChannels()));

    // 4x oversampling, linear-phase FIR half-band filters with integer latency
    // so the dry path can be delayed exactly (no comb filtering on Mix).
    oversampler = std::make_unique<juce::dsp::Oversampling<float>> (
        (size_t) numCh, 2, juce::dsp::Oversampling<float>::filterHalfBandFIREquiripple, true, true);
    oversampler->initProcessing ((size_t) samplesPerBlock);
    osChannels = numCh;
    latency = (int) std::lround (oversampler->getLatencyInSamples());
    setLatencySamples (latency);

    for (auto& c : channels)
    {
        c.dryDelay.assign ((size_t) latency + 1, 0.0f);
        c.reset();
    }
    dryBuffer.setSize (2, samplesPerBlock, false, false, true);

    driveGain.reset (sampleRate, 0.05);
    outGain.reset (sampleRate, 0.05);
    mixAmt.reset (sampleRate, 0.05);
    autoGainSm.reset (sampleRate, 0.08);
    driveGain.setCurrentAndTargetValue (1.0f);
    outGain.setCurrentAndTargetValue (1.0f);
    mixAmt.setCurrentAndTargetValue (pMix->load() * 0.01f);
    autoGainSm.setCurrentAndTargetValue (1.0f);

    inRms.setTime (sampleRate, 0.35);
    wetRms.setTime (sampleRate, 0.35);
    meterRms.setTime (sampleRate, 0.3);
    inRms.reset(); wetRms.reset(); meterRms.reset();
    autoRatio = 1.0f;

    updateFilters (true);
}

void DecapitaxorProcessor::updateFilters (bool force)
{
    const int   style   = juce::jlimit (0, dcx::NumStyles - 1, (int) pStyle->load());
    const float lowCut  = pLowCut->load();
    const float highCut = pHighCut->load();
    const float tone    = pTone->load();
    const bool  thump   = pThump->load() > 0.5f;
    const bool  steep   = pSteep->load() > 0.5f;

    if (force || style != cStyle)
    {
        const auto& v = dcx::voicing (style);
        for (auto& c : channels)
        {
            c.preLow.lowShelf   (sr, v.preLowHz,   v.preLowDb);
            c.preHigh.highShelf (sr, v.preHighHz,  v.preHighDb);
            c.dcBlock.highPass  (sr, 8.0, 0.707);
            c.postLow.lowShelf  (sr, v.postLowHz,  v.postLowDb);
            c.postHigh.highShelf(sr, v.postHighHz, v.postHighDb);
            c.postPeak.peak     (sr, v.postPeakHz, v.postPeakQ, v.postPeakDb);
        }
        cStyle = style;
    }

    if (force || lowCut != cLowCut || thump != cThump)
    {
        // Thump: resonant low cut that leaves a bump right at the cutoff.
        for (auto& c : channels) c.lowCut.highPass (sr, lowCut, thump ? 2.2 : 0.707);
        cLowCut = lowCut; cThump = thump;
    }

    if (force || tone != cTone)
    {
        // Tilt EQ: Dark lowers highs / lifts lows, Bright does the opposite.
        const double db = tone * 7.0;
        for (auto& c : channels)
        {
            c.tiltLow.lowShelf   (sr, 500.0,  -db * 0.6, 0.7);
            c.tiltHigh.highShelf (sr, 2200.0,  db,       0.7);
        }
        cTone = tone;
    }

    if (force || highCut != cHighCut || steep != cSteep)
    {
        highCutActive = highCut < 19990.0f && highCut < sr * 0.45;
        for (auto& c : channels)
        {
            if (steep)
            {   // 24 dB/oct Butterworth (two cascaded sections)
                c.hiCut1.lowPass (sr, highCut, 0.5412);
                c.hiCut2.lowPass (sr, highCut, 1.3066);
            }
            else
            {   // 12 dB/oct
                c.hiCut1.lowPass (sr, highCut, 0.707);
                c.hiCut2.setIdentity();
            }
        }
        cHighCut = highCut; cSteep = steep;
    }
}

void DecapitaxorProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int numIn  = getTotalNumInputChannels();
    const int numOut = getTotalNumOutputChannels();
    const int n      = buffer.getNumSamples();
    for (int ch = numIn; ch < numOut; ++ch) buffer.clear (ch, 0, n);
    if (n == 0 || oversampler == nullptr) return;

    const int numCh = juce::jmin (2, numOut, osChannels);
    if (dryBuffer.getNumSamples() < n) dryBuffer.setSize (2, n, false, false, true);

    updateFilters (false);

    const auto& v = dcx::voicing (cStyle);
    const float driveDb = pDrive->load() * 3.6f + (pPunish->load() > 0.5f ? 20.0f : 0.0f) + (float) v.preGainDb;
    driveGain.setTargetValue ((float) dcx::dbToGain (driveDb));
    const bool autoOn = pAuto->load() > 0.5f;
    outGain.setTargetValue ((float) dcx::dbToGain (pOutput->load() + (autoOn ? 0.0 : v.outTrimDb)));
    mixAmt.setTargetValue (pMix->load() * 0.01f);

    // ---- 1. Dry copy (delayed by the oversampling latency) + input analysis
    for (int ch = 0; ch < numCh; ++ch)
    {
        auto& c = channels[(size_t) ch];
        const float* in = buffer.getReadPointer (ch);
        float* dry = dryBuffer.getWritePointer (ch);
        const int len = (int) c.dryDelay.size();
        for (int i = 0; i < n; ++i)
        {
            c.dryDelay[(size_t) c.dryPos] = in[i];
            c.dryPos = (c.dryPos + 1) % len;
            dry[i] = c.dryDelay[(size_t) c.dryPos];
        }
    }

    // ---- 2. Pre stage: low cut, style pre-emphasis, drive
    {
        for (int i = 0; i < n; ++i)
        {
            const float g = driveGain.getNextValue();
            float mono = 0.0f;
            for (int ch = 0; ch < numCh; ++ch)
            {
                auto& c = channels[(size_t) ch];
                float* x = buffer.getWritePointer (ch);
                float s = c.lowCut.process (x[i]);
                mono += s;
                s = c.preHigh.process (c.preLow.process (s));
                x[i] = s * g;
            }
            inRms.push (mono / (float) numCh);
        }
    }

    // ---- 3. Non-linear stage at 4x
    {
        juce::dsp::AudioBlock<float> block (buffer.getArrayOfWritePointers(), (size_t) numCh, (size_t) n);
        auto up = oversampler->processSamplesUp (block);
        const int style = cStyle;
        for (size_t ch = 0; ch < up.getNumChannels(); ++ch)
        {
            float* d = up.getChannelPointer (ch);
            for (size_t i = 0; i < up.getNumSamples(); ++i)
                d[i] = dcx::shape (style, d[i]);
        }
        oversampler->processSamplesDown (block);
    }

    // ---- 4. Post stage: de-emphasis, tone, high cut, gain, mix
    for (int i = 0; i < n; ++i)
    {
        float wet[2] {};
        float wetMono = 0.0f;
        for (int ch = 0; ch < numCh; ++ch)
        {
            auto& c = channels[(size_t) ch];
            float s = buffer.getReadPointer (ch)[i];
            s = c.dcBlock.process (s);
            s = c.postPeak.process (c.postHigh.process (c.postLow.process (s)));
            wetMono += s; // Auto measures the saturation stage only (not Tone / High Cut)
            s = c.tiltHigh.process (c.tiltLow.process (s));
            if (highCutActive) s = c.hiCut2.process (c.hiCut1.process (s));
            wet[ch] = s;
        }

        wetRms.push (wetMono / (float) numCh);

        // Auto: keep the processed level matched to the input level.
        if ((i & 31) == 0)
        {
            if (autoOn)
            {
                const double inL = inRms.rms(), wetL = wetRms.rms();
                if (inL > 1.0e-5 && wetL > 1.0e-6)
                    autoRatio = (float) juce::jlimit (0.015, 4.0, inL / wetL);
                autoGainSm.setTargetValue (autoRatio);
            }
            else
                autoGainSm.setTargetValue (1.0f);
        }

        const float gain = outGain.getNextValue() * autoGainSm.getNextValue();
        const float m = mixAmt.getNextValue();
        float mono = 0.0f;
        for (int ch = 0; ch < numCh; ++ch)
        {
            const float dry = dryBuffer.getReadPointer (ch)[i];
            float o = dry + m * (wet[ch] * gain - dry);
            if (! std::isfinite (o)) o = 0.0f;
            buffer.getWritePointer (ch)[i] = o;
            mono += o;
        }
        meterRms.push (mono / (float) numCh);
    }

    meterDb.store ((float) dcx::gainToDb (meterRms.rms() * 1.41421356));
}

juce::AudioProcessorEditor* DecapitaxorProcessor::createEditor()
{
    return new DecapitaxorEditor (*this);
}

void DecapitaxorProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void DecapitaxorProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new DecapitaxorProcessor();
}
