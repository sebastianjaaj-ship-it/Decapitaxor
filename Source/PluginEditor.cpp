#include "PluginEditor.h"

namespace
{
    const juce::Colour cream     { 0xffe9dfc4 };
    const juce::Colour creamDim  { 0xffb9ae93 };
    const juce::Colour plateTop  { 0xff2a2a2c };
    const juce::Colour plateBot  { 0xff161617 };
    const juce::Colour amber     { 0xffffb347 };
    const juce::Colour lampRed   { 0xffff3b2a };

    constexpr float kStart = juce::MathConstants<float>::pi * 1.25f;
    constexpr float kEnd   = juce::MathConstants<float>::pi * 2.75f;

    juce::Font font (float h, bool bold = true)
    {
        return juce::Font (juce::FontOptions().withHeight (h).withStyle (bold ? "Bold" : "Regular"));
    }

    juce::Point<float> polar (juce::Point<float> c, float r, float a)
    {
        return { c.x + r * std::sin (a), c.y - r * std::cos (a) };
    }

    void drawScrew (juce::Graphics& g, float cx, float cy, float r, float angle)
    {
        juce::ColourGradient grad (juce::Colour (0xffb8b8b8), cx - r, cy - r, juce::Colour (0xff4a4a4a), cx + r, cy + r, false);
        g.setGradientFill (grad);
        g.fillEllipse (cx - r, cy - r, 2 * r, 2 * r);
        g.setColour (juce::Colours::black.withAlpha (0.6f));
        g.drawEllipse (cx - r, cy - r, 2 * r, 2 * r, 1.0f);
        g.setColour (juce::Colour (0xff222222));
        const auto d = polar ({ 0, 0 }, r * 0.75f, angle);
        g.drawLine (cx - d.x, cy - d.y, cx + d.x, cy + d.y, 1.6f);
    }

    void drawWood (juce::Graphics& g, juce::Rectangle<float> r, int seed)
    {
        juce::ColourGradient grad (juce::Colour (0xff6b3a1c), r.getX(), 0, juce::Colour (0xff4a2410), r.getRight(), 0, false);
        grad.addColour (0.5, juce::Colour (0xff7c4522));
        g.setGradientFill (grad);
        g.fillRect (r);

        juce::Random rnd (seed);
        for (int i = 0; i < 26; ++i)
        {
            const float x = r.getX() + rnd.nextFloat() * r.getWidth();
            juce::Path p;
            p.startNewSubPath (x, r.getY());
            for (float y = r.getY(); y <= r.getBottom(); y += 12.0f)
                p.lineTo (x + 1.6f * std::sin (y * 0.035f + (float) i), y);
            g.setColour ((rnd.nextBool() ? juce::Colour (0xff2e1407) : juce::Colour (0xff9a5a2e))
                             .withAlpha (0.12f + 0.18f * rnd.nextFloat()));
            g.strokePath (p, juce::PathStrokeType (0.6f + rnd.nextFloat() * 1.4f));
        }
        g.setColour (juce::Colours::black.withAlpha (0.45f));
        g.drawRect (r, 1.0f);
    }
}

//==============================================================================
VintageLookAndFeel::VintageLookAndFeel()
{
    setColour (juce::BubbleComponent::backgroundColourId, juce::Colour (0xff111111));
    setColour (juce::BubbleComponent::outlineColourId, amber.withAlpha (0.6f));
    setColour (juce::TooltipWindow::textColourId, cream);
}

void VintageLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                                           float startAngle, float endAngle, juce::Slider& s)
{
    const auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (2.0f);
    const float r = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const auto c = bounds.getCentre();
    const float angle = startAngle + pos * (endAngle - startAngle);
    const bool big = s.getProperties().contains ("big");

    // drop shadow
    g.setColour (juce::Colours::black.withAlpha (0.55f));
    g.fillEllipse (c.x - r + 2, c.y - r + 4, r * 2, r * 2);

    if (big)
    {
        // Wide metal skirt
        juce::ColourGradient skirt (juce::Colour (0xff5a5a5c), c.x, c.y - r, juce::Colour (0xff141414), c.x, c.y + r, false);
        g.setGradientFill (skirt);
        g.fillEllipse (c.x - r, c.y - r, 2 * r, 2 * r);
        g.setColour (juce::Colour (0xff0a0a0a));
        g.drawEllipse (c.x - r, c.y - r, 2 * r, 2 * r, 1.5f);

        // knurling on the skirt rim
        for (int i = 0; i < 72; ++i)
        {
            const float a = juce::MathConstants<float>::twoPi * (float) i / 72.0f;
            g.setColour ((i % 2 ? juce::Colours::white : juce::Colours::black).withAlpha (0.10f));
            g.drawLine ({ polar (c, r * 0.88f, a), polar (c, r * 0.99f, a) }, 1.2f);
        }

        // Pointer ("chicken head") body
        juce::Path ptr;
        const float len = r * 0.92f, tail = r * 0.55f, wFront = r * 0.13f, wBack = r * 0.42f;
        ptr.startNewSubPath (-wFront, -len + wFront);
        ptr.quadraticTo (0, -len - wFront * 0.6f, wFront, -len + wFront);
        ptr.lineTo (wBack, tail * 0.35f);
        ptr.quadraticTo (wBack * 0.9f, tail, 0, tail);
        ptr.quadraticTo (-wBack * 0.9f, tail, -wBack, tail * 0.35f);
        ptr.closeSubPath();
        ptr.applyTransform (juce::AffineTransform::rotation (angle).translated (c));

        g.setColour (juce::Colours::black.withAlpha (0.5f));
        g.fillPath (ptr, juce::AffineTransform::translation (2.0f, 4.0f));
        juce::ColourGradient body (juce::Colour (0xff3a3a3c), c.x - r * 0.5f, c.y - r * 0.6f,
                                   juce::Colour (0xff050505), c.x + r * 0.5f, c.y + r * 0.6f, false);
        g.setGradientFill (body);
        g.fillPath (ptr);
        g.setColour (juce::Colour (0xff6a6a6a).withAlpha (0.7f));
        g.strokePath (ptr, juce::PathStrokeType (1.0f));

        // centre cap
        const float cr = r * 0.30f;
        juce::ColourGradient cap (juce::Colour (0xff4c4c4e), c.x - cr, c.y - cr, juce::Colour (0xff0c0c0c), c.x + cr, c.y + cr, false);
        g.setGradientFill (cap);
        g.fillEllipse (c.x - cr, c.y - cr, 2 * cr, 2 * cr);

        // cream indicator line
        g.setColour (cream);
        g.drawLine ({ polar (c, r * 0.38f, angle), polar (c, r * 0.86f, angle) }, 3.0f);
        return;
    }

    // Small bakelite knob with knurled edge
    juce::ColourGradient body (juce::Colour (0xff3b3b3d), c.x, c.y - r, juce::Colour (0xff0b0b0b), c.x, c.y + r, false);
    g.setGradientFill (body);
    g.fillEllipse (c.x - r, c.y - r, 2 * r, 2 * r);
    for (int i = 0; i < 40; ++i)
    {
        const float a = juce::MathConstants<float>::twoPi * (float) i / 40.0f + angle;
        g.setColour (juce::Colours::black.withAlpha (0.55f));
        g.drawLine ({ polar (c, r * 0.80f, a), polar (c, r, a) }, 1.4f);
    }
    const float ir = r * 0.74f;
    juce::ColourGradient top (juce::Colour (0xff47474a), c.x - ir * 0.6f, c.y - ir, juce::Colour (0xff121212), c.x + ir * 0.4f, c.y + ir, false);
    g.setGradientFill (top);
    g.fillEllipse (c.x - ir, c.y - ir, 2 * ir, 2 * ir);
    g.setColour (juce::Colours::white.withAlpha (0.08f));
    g.fillEllipse (c.x - ir * 0.7f, c.y - ir * 0.9f, ir * 1.2f, ir * 0.7f);
    g.setColour (cream);
    g.drawLine ({ polar (c, r * 0.18f, angle), polar (c, r * 0.95f, angle) }, 2.4f);
}

void VintageLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& b, bool hi, bool down)
{
    const auto r = b.getLocalBounds().toFloat();
    const bool on = b.getToggleState();
    const bool lampStyle = b.getProperties().contains ("lamp");

    if (lampStyle)
    {
        // Jewel lamp + text underneath (used for PUNISH)
        const float d = juce::jmin (r.getHeight() * 0.56f, r.getWidth() * 0.5f);
        auto lamp = juce::Rectangle<float> (d, d).withCentre ({ r.getCentreX(), r.getY() + d * 0.5f + 2 });
        g.setColour (juce::Colour (0xff8a8a8a));
        g.fillEllipse (lamp.expanded (3.0f));
        g.setColour (juce::Colours::black);
        g.fillEllipse (lamp.expanded (1.0f));
        if (on)
        {
            g.setColour (lampRed.withAlpha (0.35f));
            g.fillEllipse (lamp.expanded (7.0f));
        }
        juce::ColourGradient jewel (on ? juce::Colour (0xffffb0a0) : juce::Colour (0xff6a1d16), lamp.getCentreX() - d * 0.2f, lamp.getY(),
                                    on ? lampRed : juce::Colour (0xff2a0805), lamp.getCentreX(), lamp.getBottom(), true);
        g.setGradientFill (jewel);
        g.fillEllipse (lamp);
        g.setColour (juce::Colours::white.withAlpha (0.35f));
        g.fillEllipse (lamp.getX() + d * 0.22f, lamp.getY() + d * 0.12f, d * 0.3f, d * 0.2f);

        g.setColour (hi ? cream : creamDim.brighter (0.2f));
        g.setFont (font (12.0f));
        g.drawText (b.getButtonText(), r.withTop (lamp.getBottom() + 4), juce::Justification::centredTop);
        return;
    }

    // Small LED + label
    const float d = 9.0f;
    auto led = juce::Rectangle<float> (d, d).withCentre ({ r.getX() + d * 0.5f + 3, r.getCentreY() });
    if (on)
    {
        g.setColour (amber.withAlpha (0.35f));
        g.fillEllipse (led.expanded (4.0f));
    }
    g.setColour (juce::Colours::black);
    g.fillEllipse (led.expanded (1.5f));
    g.setColour (on ? amber : juce::Colour (0xff4a3418));
    g.fillEllipse (led);
    g.setColour (juce::Colours::white.withAlpha (on ? 0.6f : 0.15f));
    g.fillEllipse (led.getX() + 2, led.getY() + 1.5f, 3.0f, 2.5f);

    g.setColour (down ? cream.darker (0.2f) : (hi ? cream : creamDim));
    g.setFont (font (10.5f));
    g.drawText (b.getButtonText(), r.withLeft (led.getRight() + 5), juce::Justification::centredLeft);
}

void VintageLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&, bool hi, bool down)
{
    auto r = b.getLocalBounds().toFloat().reduced (1.5f);
    const bool on = b.getToggleState();

    g.setColour (juce::Colours::black);
    g.fillRoundedRectangle (r.expanded (1.5f), 4.0f);

    if (on)
    {
        juce::ColourGradient grad (amber.brighter (0.3f), r.getX(), r.getY(), juce::Colour (0xffc76b10), r.getX(), r.getBottom(), false);
        g.setGradientFill (grad);
    }
    else
    {
        juce::ColourGradient grad (juce::Colour (hi ? 0xff5a5a5c : 0xff4a4a4c), r.getX(), r.getY(),
                                   juce::Colour (0xff1c1c1d), r.getX(), r.getBottom(), false);
        g.setGradientFill (grad);
    }
    g.fillRoundedRectangle (down ? r.reduced (1.0f) : r, 3.0f);
    g.setColour (juce::Colours::white.withAlpha (on ? 0.35f : 0.12f));
    g.drawLine (r.getX() + 4, r.getY() + 1.5f, r.getRight() - 4, r.getY() + 1.5f, 1.0f);
}

void VintageLookAndFeel::drawButtonText (juce::Graphics& g, juce::TextButton& b, bool, bool)
{
    g.setFont (getTextButtonFont (b, b.getHeight()));
    g.setColour (b.getToggleState() ? juce::Colour (0xff2a1500) : cream);
    g.drawText (b.getButtonText(), b.getLocalBounds(), juce::Justification::centred);
}

juce::Font VintageLookAndFeel::getTextButtonFont (juce::TextButton&, int) { return font (12.0f); }
juce::Font VintageLookAndFeel::getSliderPopupFont (juce::Slider&) { return font (13.0f); }

void VintageLookAndFeel::drawBubble (juce::Graphics& g, juce::BubbleComponent&, const juce::Point<float>&, const juce::Rectangle<float>& body)
{
    g.setColour (juce::Colour (0xee101010));
    g.fillRoundedRectangle (body, 4.0f);
    g.setColour (amber.withAlpha (0.7f));
    g.drawRoundedRectangle (body, 4.0f, 1.0f);
}

//==============================================================================
void VuMeter::setLevelDb (float db)
{
    // 0 VU = -14 dBFS. Needle travel is linear in voltage (like a real VU).
    const float vu = juce::jlimit (-22.0f, 4.0f, db + 14.0f);
    const float lo = std::pow (10.0f, -20.0f / 20.0f), hi = std::pow (10.0f, 3.0f / 20.0f);
    const float target = juce::jlimit (0.0f, 1.05f, (std::pow (10.0f, vu / 20.0f) - lo) / (hi - lo));
    needle += (target - needle) * 0.35f;
    repaint();
}

void VuMeter::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();

    // bezel
    juce::ColourGradient bez (juce::Colour (0xff6a6a6a), r.getX(), r.getY(), juce::Colour (0xff1a1a1a), r.getX(), r.getBottom(), false);
    g.setGradientFill (bez);
    g.fillRoundedRectangle (r, 8.0f);
    auto face = r.reduced (7.0f);
    g.setColour (juce::Colours::black);
    g.fillRoundedRectangle (face.expanded (1.5f), 5.0f);

    // backlit cream face
    juce::ColourGradient fg (juce::Colour (0xfffff2cf), face.getCentreX(), face.getY() + face.getHeight() * 0.35f,
                             juce::Colour (0xffd9b773), face.getX(), face.getBottom(), true);
    g.setGradientFill (fg);
    g.fillRoundedRectangle (face, 4.0f);

    const juce::Point<float> pivot { face.getCentreX(), face.getBottom() + face.getHeight() * 0.28f };
    const float rad = face.getHeight() * 0.98f;
    const float aMin = juce::degreesToRadians (-48.0f), aMax = juce::degreesToRadians (48.0f);

    auto posOf = [] (float vu)
    {
        const float lo = std::pow (10.0f, -1.0f), hi = std::pow (10.0f, 3.0f / 20.0f);
        return (std::pow (10.0f, vu / 20.0f) - lo) / (hi - lo);
    };

    g.saveState();
    g.reduceClipRegion (face.toNearestInt());

    // scale arc (black up to 0, red above)
    juce::Path arcB, arcR;
    const float a0 = aMin + posOf (0.0f) * (aMax - aMin);
    arcB.addCentredArc (pivot.x, pivot.y, rad * 0.80f, rad * 0.80f, 0, aMin, a0, true);
    arcR.addCentredArc (pivot.x, pivot.y, rad * 0.80f, rad * 0.80f, 0, a0, aMax, true);
    g.setColour (juce::Colour (0xff1a1a1a));
    g.strokePath (arcB, juce::PathStrokeType (1.6f));
    g.setColour (juce::Colour (0xffc21d12));
    g.strokePath (arcR, juce::PathStrokeType (5.0f));

    const float marks[] = { -20, -10, -7, -5, -3, -2, -1, 0, 1, 2, 3 };
    g.setFont (font (9.5f));
    for (float m : marks)
    {
        const float a = aMin + posOf (m) * (aMax - aMin);
        g.setColour (m > 0 ? juce::Colour (0xffc21d12) : juce::Colour (0xff1a1a1a));
        g.drawLine ({ polar (pivot, rad * 0.80f, a), polar (pivot, rad * 0.87f, a) }, 1.4f);
        const auto tp = polar (pivot, rad * 0.95f, a);
        juce::String txt = juce::String ((int) std::abs (m));
        g.drawText (txt, juce::Rectangle<float> (22, 12).withCentre (tp), juce::Justification::centred);
    }

    g.setColour (juce::Colour (0xff2a2a2a));
    g.setFont (font (8.0f, false));
    g.drawText ("-", juce::Rectangle<float> (face.getX() + 6, face.getY() + 4, 12, 12), juce::Justification::centred);
    g.setColour (juce::Colour (0xffc21d12));
    g.drawText ("+", juce::Rectangle<float> (face.getRight() - 18, face.getY() + 4, 12, 12), juce::Justification::centred);

    g.setColour (juce::Colour (0xff2a2a2a));
    g.setFont (font (13.0f));
    g.drawText ("HEAT", face.withTrimmedTop (face.getHeight() * 0.50f).withHeight (16), juce::Justification::centred);
    g.setFont (font (7.5f, false));
    g.drawText ("DX-5  SATURATION LEVEL", face.withTrimmedTop (face.getHeight() * 0.66f).withHeight (12), juce::Justification::centred);

    // needle
    const float a = aMin + juce::jlimit (-0.02f, 1.05f, needle) * (aMax - aMin);
    g.setColour (juce::Colours::black.withAlpha (0.18f));
    g.drawLine ({ pivot.translated (3, 2), polar (pivot, rad * 0.92f, a).translated (3, 2) }, 1.8f);
    g.setColour (juce::Colour (0xff111111));
    g.drawLine ({ pivot, polar (pivot, rad * 0.92f, a) }, 1.6f);

    // pivot housing
    g.setColour (juce::Colour (0xff141414));
    g.fillEllipse (juce::Rectangle<float> (face.getWidth() * 0.34f, face.getHeight() * 0.34f)
                       .withCentre ({ pivot.x, face.getBottom() }));
    g.restoreState();

    // glass glare
    g.setColour (juce::Colours::white.withAlpha (0.10f));
    juce::Path glare;
    glare.addRoundedRectangle (face.getX(), face.getY(), face.getWidth(), face.getHeight() * 0.42f, 4.0f);
    g.fillPath (glare);
}

//==============================================================================
Faceplate::Faceplate (DecapitaxorProcessor& p) : proc (p)
{
    setLookAndFeel (&lnf);
    auto& s = proc.apvts;

    auto setupKnob = [this] (juce::Slider& k)
    {
        k.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        k.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        k.setRotaryParameters (kStart, kEnd, true);
        k.setPopupDisplayEnabled (true, true, this, 1200);
        k.setVelocityBasedMode (false);
        k.setMouseDragSensitivity (220);
        addAndMakeVisible (k);
    };
    for (auto* k : { &drive, &lowCut, &tone, &highCut, &output, &mix }) setupKnob (*k);
    drive.getProperties().set ("big", true);
    drive.setMouseDragSensitivity (300);

    for (auto* b : { &punish, &thump, &steep, &autoBtn }) addAndMakeVisible (*b);
    punish.getProperties().set ("lamp", true);

    aDrive = std::make_unique<SA> (s, ParamID::drive,   drive);
    aLow   = std::make_unique<SA> (s, ParamID::lowCut,  lowCut);
    aTone  = std::make_unique<SA> (s, ParamID::tone,    tone);
    aHigh  = std::make_unique<SA> (s, ParamID::highCut, highCut);
    aOut   = std::make_unique<SA> (s, ParamID::output,  output);
    aMix   = std::make_unique<SA> (s, ParamID::mix,     mix);
    aPunish = std::make_unique<BA> (s, ParamID::punish,   punish);
    aThump  = std::make_unique<BA> (s, ParamID::thump,    thump);
    aSteep  = std::make_unique<BA> (s, ParamID::steep,    steep);
    aAuto   = std::make_unique<BA> (s, ParamID::autoGain, autoBtn);

    // double-click resets
    drive.setDoubleClickReturnValue (true, 3.0);
    lowCut.setDoubleClickReturnValue (true, 20.0);
    tone.setDoubleClickReturnValue (true, 0.0);
    highCut.setDoubleClickReturnValue (true, 20000.0);
    output.setDoubleClickReturnValue (true, 0.0);
    mix.setDoubleClickReturnValue (true, 100.0);

    styleParam = s.getParameter (ParamID::style);
    const char* names[] = { "TAPE", "CONSOLE", "IRON", "TRIODE", "PENTODE" };
    const char* tips[]  = { "Tape: smooth compression, soft highs, low-end bump",
                            "Console: bright, forward desk saturation",
                            "Iron (transformer): thick low-mids, strong 2nd harmonic",
                            "Triode: warm asymmetric tube stage",
                            "Pentode: aggressive, gritty, heavy odd harmonics" };
    for (int i = 0; i < 5; ++i)
    {
        auto& b = styleBtns[i];
        b.setButtonText (names[i]);
        b.setTooltip (tips[i]);
        b.setClickingTogglesState (false);
        b.onClick = [this, i]
        {
            styleParam->beginChangeGesture();
            styleParam->setValueNotifyingHost (styleParam->convertTo0to1 ((float) i));
            styleParam->endChangeGesture();
        };
        addAndMakeVisible (b);
    }

    addAndMakeVisible (meter);
    setSize (W, H);
    startTimerHz (30);
}

Faceplate::~Faceplate()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void Faceplate::timerCallback()
{
    meter.setLevelDb (proc.meterDb.load());
    const int st = juce::roundToInt (styleParam->convertFrom0to1 (styleParam->getValue()));
    if (st != shownStyle)
    {
        shownStyle = st;
        for (int i = 0; i < 5; ++i) styleBtns[i].setToggleState (i == st, juce::dontSendNotification);
    }
}

// ---- layout ---------------------------------------------------------------
namespace L
{
    const juce::Rectangle<int> vu      { 58, 100, 210, 142 };
    const juce::Rectangle<int> drive   { 323, 112, 140, 140 };
    const juce::Rectangle<int> punish  { 360, 266, 66, 44 };
    const juce::Rectangle<int> lowCut  { 517, 104, 56, 56 };
    const juce::Rectangle<int> tone    { 612, 104, 56, 56 };
    const juce::Rectangle<int> highCut { 707, 104, 56, 56 };
    const juce::Rectangle<int> output  { 532, 240, 56, 56 };
    const juce::Rectangle<int> mix     { 692, 240, 56, 56 };
    const int styleY = 336;
}

void Faceplate::resized()
{
    meter.setBounds (L::vu);
    drive.setBounds (L::drive);
    punish.setBounds (L::punish);
    lowCut.setBounds (L::lowCut);
    tone.setBounds (L::tone);
    highCut.setBounds (L::highCut);
    output.setBounds (L::output);
    mix.setBounds (L::mix);

    thump.setBounds (L::lowCut.getCentreX() - 30, L::lowCut.getBottom() + 26, 70, 16);
    steep.setBounds (L::highCut.getCentreX() - 30, L::highCut.getBottom() + 26, 70, 16);
    autoBtn.setBounds (L::output.getRight() + 20, L::output.getCentreY() - 8, 64, 16);

    const int bw = 90, gap = 8;
    int x = 286;
    for (auto& b : styleBtns) { b.setBounds (x, L::styleY, bw, 30); x += bw + gap; }
}

void Faceplate::drawKnobScale (juce::Graphics& g, juce::Rectangle<int> knob, const juce::StringArray& labels,
                               const juce::String& left, const juce::String& right, int ticks)
{
    const auto c = knob.toFloat().getCentre();
    const float r = knob.getWidth() * 0.5f;
    g.setColour (creamDim);
    for (int i = 0; i < ticks; ++i)
    {
        const float a = kStart + (kEnd - kStart) * (float) i / (float) (ticks - 1);
        const auto p = polar (c, r + 6.0f, a);
        g.fillEllipse (p.x - 1.4f, p.y - 1.4f, 2.8f, 2.8f);
    }
    if (! labels.isEmpty())
    {
        g.setFont (font (r > 50 ? 13.0f : 9.0f));
        for (int i = 0; i < labels.size(); ++i)
        {
            const float a = kStart + (kEnd - kStart) * (float) i / (float) (labels.size() - 1);
            const auto p = polar (c, r + 18.0f, a);
            g.drawText (labels[i], juce::Rectangle<float> (26, 14).withCentre (p), juce::Justification::centred);
        }
    }
    g.setFont (font (8.0f));
    g.setColour (creamDim.withAlpha (0.85f));
    if (left.isNotEmpty())
        g.drawText (left, juce::Rectangle<float> (44, 11).withCentre ({ c.x - r * 0.72f, c.y + r + 12 }), juce::Justification::centred);
    if (right.isNotEmpty())
        g.drawText (right, juce::Rectangle<float> (44, 11).withCentre ({ c.x + r * 0.72f, c.y + r + 12 }), juce::Justification::centred);
}

void Faceplate::paint (juce::Graphics& g)
{
    const auto all = getLocalBounds().toFloat();
    const float woodW = 30.0f;

    // ---- wood cheeks
    drawWood (g, all.withWidth (woodW), 7);
    drawWood (g, all.withLeft (all.getRight() - woodW), 13);

    // ---- faceplate
    auto plate = all.reduced (woodW, 0).reduced (0, 8);
    g.setColour (juce::Colours::black);
    g.fillRect (all.reduced (woodW, 0));
    juce::ColourGradient pg (plateTop, 0, plate.getY(), plateBot, 0, plate.getBottom(), false);
    g.setGradientFill (pg);
    g.fillRoundedRectangle (plate, 4.0f);

    {   // brushed metal texture
        juce::Random rnd (99);
        for (int i = 0; i < 260; ++i)
        {
            const float y = plate.getY() + rnd.nextFloat() * plate.getHeight();
            g.setColour (juce::Colours::white.withAlpha (0.012f + rnd.nextFloat() * 0.02f));
            g.drawHorizontalLine ((int) y, plate.getX() + 2, plate.getRight() - 2);
        }
    }
    g.setColour (juce::Colours::white.withAlpha (0.08f));
    g.drawRoundedRectangle (plate.reduced (0.5f), 4.0f, 1.0f);

    // screws
    drawScrew (g, plate.getX() + 14, plate.getY() + 14, 6.0f, 0.4f);
    drawScrew (g, plate.getRight() - 14, plate.getY() + 14, 6.0f, 1.3f);
    drawScrew (g, plate.getX() + 14, plate.getBottom() - 14, 6.0f, 2.1f);
    drawScrew (g, plate.getRight() - 14, plate.getBottom() - 14, 6.0f, 0.9f);

    // ---- brand header (engraved)
    const juce::Rectangle<float> brand (plate.getX() + 40, plate.getY() + 12, plate.getWidth() - 80, 40);
    g.setFont (font (30.0f).withExtraKerningFactor (0.22f));
    g.setColour (juce::Colours::black.withAlpha (0.7f));
    g.drawText ("DECAPITAXOR", brand.translated (1, 2), juce::Justification::centred);
    g.setColour (cream);
    g.drawText ("DECAPITAXOR", brand, juce::Justification::centred);
    const float tw = juce::GlyphArrangement::getStringWidth (font (30.0f).withExtraKerningFactor (0.22f), "DECAPITAXOR");
    g.setColour (creamDim.withAlpha (0.8f));
    const float ly = brand.getCentreY();
    g.fillRect (brand.getX() + 10, ly - 1, (brand.getWidth() - tw) * 0.5f - 26, 2.0f);
    g.fillRect (brand.getCentreX() + tw * 0.5f + 16, ly - 1, (brand.getWidth() - tw) * 0.5f - 26, 2.0f);

    g.setFont (font (9.0f, false).withExtraKerningFactor (0.35f));
    g.setColour (creamDim);
    g.drawText ("ANALOG  SATURATION  PROCESSOR", brand.translated (0, 33).withHeight (12), juce::Justification::centred);

    // ---- section separators
    g.setColour (juce::Colours::black.withAlpha (0.55f));
    g.drawVerticalLine (290, 90, 318);
    g.drawVerticalLine (496, 90, 318);
    g.setColour (juce::Colours::white.withAlpha (0.06f));
    g.drawVerticalLine (291, 90, 318);
    g.drawVerticalLine (497, 90, 318);

    // ---- labels + scales
    auto label = [&] (const juce::String& t, juce::Rectangle<int> knob, int dy = -32)
    {
        g.setColour (cream);
        g.setFont (font (11.5f).withExtraKerningFactor (0.08f));
        g.drawText (t, juce::Rectangle<int> (knob.getCentreX() - 50, knob.getY() + dy, 100, 16), juce::Justification::centred);
    };

    label ("DRIVE", L::drive, -42);
    drawKnobScale (g, L::drive, { "0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10" }, {}, {}, 11);

    label ("LOW CUT", L::lowCut);
    drawKnobScale (g, L::lowCut, {}, "20 Hz", "1 kHz", 11);
    label ("TONE", L::tone);
    drawKnobScale (g, L::tone, {}, "DARK", "BRIGHT", 11);
    label ("HIGH CUT", L::highCut);
    drawKnobScale (g, L::highCut, {}, "1 kHz", "OFF", 11);
    label ("OUTPUT", L::output);
    drawKnobScale (g, L::output, {}, "-24 dB", "+6 dB", 11);
    label ("MIX", L::mix);
    drawKnobScale (g, L::mix, {}, "DRY", "WET", 11);

    // ---- style bar
    const juce::Rectangle<float> bar (plate.getX() + 6, (float) L::styleY - 10, plate.getWidth() - 12, 50);
    g.setColour (juce::Colours::black.withAlpha (0.45f));
    g.fillRoundedRectangle (bar, 3.0f);
    g.setColour (juce::Colours::white.withAlpha (0.05f));
    g.drawRoundedRectangle (bar, 3.0f, 1.0f);

    g.setColour (cream);
    g.setFont (font (12.0f).withExtraKerningFactor (0.25f));
    g.drawText ("STYLE", juce::Rectangle<float> (bar.getX() + 40, bar.getY(), 120, bar.getHeight()), juce::Justification::centredLeft);
    g.setColour (creamDim.withAlpha (0.7f));
    g.fillRect (bar.getX() + 122, bar.getCentreY() - 1, (float) styleBtns[0].getX() - bar.getX() - 136, 2.0f);
    g.setFont (font (8.0f, false).withExtraKerningFactor (0.2f));
    g.drawText ("MODEL DX-5  /  4X OVERSAMPLED", juce::Rectangle<float> (58, 254, 210, 14), juce::Justification::centred);
}

//==============================================================================
DecapitaxorEditor::DecapitaxorEditor (DecapitaxorProcessor& p)
    : AudioProcessorEditor (&p), face (p)
{
    addAndMakeVisible (face);
    setResizable (true, true);
    setResizeLimits (Faceplate::W / 2, Faceplate::H / 2, Faceplate::W * 2, Faceplate::H * 2);
    getConstrainer()->setFixedAspectRatio ((double) Faceplate::W / Faceplate::H);
    setSize (Faceplate::W, Faceplate::H);
}

void DecapitaxorEditor::resized()
{
    const float scale = (float) getWidth() / (float) Faceplate::W;
    face.setBounds (0, 0, Faceplate::W, Faceplate::H);
    face.setTransform (juce::AffineTransform::scale (scale));
}
