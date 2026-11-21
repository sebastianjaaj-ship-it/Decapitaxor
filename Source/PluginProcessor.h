#pragma once

#include <JuceHeader.h>
#include <array>
#include <atomic>
#include <vector>
#include "DSP.h"

namespace ParamID
{
    static const juce::String drive   { "drive" };
    static const juce::String punish  { "punish" };
    static const juce::String style   { "style" };
    static const juce::String lowCut  { "lowcut" };
    static const juce::String thump   { "thump" };
    static const juce::String tone    { "tone" };
    static const juce::String highCut { "highcut" };
    static const juce::String steep   { "steep" };
    static const juce::String output  { "output" };
    static const juce::String autoGain{ "auto" };
    static const juce::String mix     { "mix" };
}

class DecapitaxorProcessor : public juce::AudioProcessor
{
public:
    DecapitaxorProcessor();
    ~DecapitaxorProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return "Default"; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    juce::AudioProcessorValueTreeState apvts;

    // Meter value for the UI (output level in dBFS, VU ballistics).
    std::atomic<float> meterDb { -60.0f };

private:
    struct Channel
    {
        dcx::Biquad lowCut, preLow, preHigh;
        dcx::Biquad dcBlock, postLow, postHigh, postPeak;
        dcx::Biquad tiltLow, tiltHigh;
        dcx::Biquad hiCut1, hiCut2;
        std::vector<float> dryDelay;
        int dryPos = 0;

        void reset()
        {
            for (auto* b : { &lowCut, &preLow, &preHigh, &dcBlock, &postLow, &postHigh,
                             &postPeak, &tiltLow, &tiltHigh, &hiCut1, &hiCut2 })
                b->reset();
            std::fill (dryDelay.begin(), dryDelay.end(), 0.0f);
            dryPos = 0;
        }
    };

    void updateFilters (bool force);

    std::unique_ptr<juce::dsp::Oversampling<float>> oversampler;
    std::array<Channel, 2> channels;
    juce::AudioBuffer<float> dryBuffer;

    double sr = 44100.0;
    int latency = 0;
    int osChannels = 0;

    // cached parameter values used to decide when coefficients must change
    int   cStyle = -1;
    float cLowCut = -1, cHighCut = -1, cTone = -99;
    bool  cThump = false, cSteep = false, highCutActive = false;

    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Multiplicative> driveGain, outGain;
    juce::SmoothedValue<float> mixAmt;
    juce::SmoothedValue<float> autoGainSm;

    dcx::RmsFollower inRms, wetRms, meterRms;
    float autoRatio = 1.0f;

    std::atomic<float>* pDrive; std::atomic<float>* pPunish; std::atomic<float>* pStyle;
    std::atomic<float>* pLowCut; std::atomic<float>* pThump; std::atomic<float>* pTone;
    std::atomic<float>* pHighCut; std::atomic<float>* pSteep; std::atomic<float>* pOutput;
    std::atomic<float>* pAuto; std::atomic<float>* pMix;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DecapitaxorProcessor)
};
