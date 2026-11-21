#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
class VintageLookAndFeel : public juce::LookAndFeel_V4
{
public:
    VintageLookAndFeel();

    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos,
                           float startAngle, float endAngle, juce::Slider&) override;
    void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool highlighted, bool down) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool highlighted, bool down) override;
    void drawButtonText (juce::Graphics&, juce::TextButton&, bool highlighted, bool down) override;
    juce::Font getTextButtonFont (juce::TextButton&, int) override;
    void drawBubble (juce::Graphics&, juce::BubbleComponent&, const juce::Point<float>&, const juce::Rectangle<float>&) override;
    juce::Font getSliderPopupFont (juce::Slider&) override;
};

//==============================================================================
class VuMeter : public juce::Component
{
public:
    void setLevelDb (float db);
    void paint (juce::Graphics&) override;
private:
    float needle = 0.0f; // 0..1
};

//==============================================================================
class Faceplate : public juce::Component, private juce::Timer
{
public:
    explicit Faceplate (DecapitaxorProcessor&);
    ~Faceplate() override;
    void paint (juce::Graphics&) override;
    void resized() override;

    static constexpr int W = 820, H = 392;

private:
    void timerCallback() override;
    void drawKnobScale (juce::Graphics&, juce::Rectangle<int> knob, const juce::StringArray& labels,
                        const juce::String& left, const juce::String& right, int ticks);

    using SA = juce::AudioProcessorValueTreeState::SliderAttachment;
    using BA = juce::AudioProcessorValueTreeState::ButtonAttachment;

    DecapitaxorProcessor& proc;
    VintageLookAndFeel lnf;

    juce::Slider drive, lowCut, tone, highCut, output, mix;
    juce::ToggleButton punish { "PUNISH" }, thump { "THUMP" }, steep { "STEEP" }, autoBtn { "AUTO" };
    juce::TextButton styleBtns[5];
    VuMeter meter;

    std::unique_ptr<SA> aDrive, aLow, aTone, aHigh, aOut, aMix;
    std::unique_ptr<BA> aPunish, aThump, aSteep, aAuto;
    juce::RangedAudioParameter* styleParam = nullptr;
    int shownStyle = -1;
};

//==============================================================================
class DecapitaxorEditor : public juce::AudioProcessorEditor
{
public:
    explicit DecapitaxorEditor (DecapitaxorProcessor&);
    void resized() override;
    void paint (juce::Graphics& g) override { g.fillAll (juce::Colours::black); }

private:
    Faceplate face;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DecapitaxorEditor)
};
