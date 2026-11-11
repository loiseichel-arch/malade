#include "SketchLookAndFeel.h"

using namespace sketch;

SketchLookAndFeel::SketchLookAndFeel()
{
    setColour (juce::ResizableWindow::backgroundColourId, colour::paper);
    setColour (juce::TooltipWindow::backgroundColourId, colour::knob);
    setColour (juce::TooltipWindow::textColourId, colour::ink);
    setColour (juce::TooltipWindow::outlineColourId, colour::ink);
}

void SketchLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                          float sliderPos, float startAngle, float endAngle, juce::Slider& s)
{
    const auto& ov = s.getProperties()["overridePos"];
    if (! ov.isVoid())
        sliderPos = juce::jlimit (0.0f, 1.0f, (float) (double) ov);

    const auto area = juce::Rectangle<float> ((float) x, (float) y, (float) width, (float) height);
    const float R = juce::jmin (area.getWidth(), area.getHeight()) * 0.5f - 2.0f;
    const auto c = area.getCentre();
    const auto seed = seedOf (s.getName());
    const float scale = R / 50.0f;   // drawn at 100 px, scaled

    // Tick marks around the knob.
    juce::Path ticks;
    for (int i = 0; i <= 10; ++i)
    {
        const float a = startAngle + (endAngle - startAngle) * (float) i / 10.0f;
        ticks.addPath (line (c.getPointOnCircumference (R * 0.80f, a),
                             c.getPointOnCircumference (R * 0.97f, a), seed + (uint32_t) i, 0.3f));
    }
    stroke (g, ticks, juce::jmax (1.2f, 2.3f * scale));

    // Body.
    const auto body = juce::Rectangle<float> (R * 1.30f, R * 1.30f).withCentre (c);
    const auto bodyPath = ellipse (body, seed, 0.9f * scale + 0.3f);
    g.setColour (colour::knob);
    g.fillPath (bodyPath);
    stroke (g, bodyPath, juce::jmax (1.5f, 2.7f * scale));
    stroke (g, ellipse (body.reduced (R * 0.12f), seed + 7, 0.6f), juce::jmax (0.8f, 1.2f * scale),
            colour::ink.withAlpha (0.45f));

    // Pointer.
    const float angle = startAngle + sliderPos * (endAngle - startAngle);
    juce::Path ptr;
    ptr.startNewSubPath (c);
    ptr.lineTo (c.getPointOnCircumference (R * 0.56f, angle));
    stroke (g, ptr, juce::jmax (2.0f, 4.5f * scale), colour::accent);

    g.setColour (colour::ink);
    g.fillEllipse (juce::Rectangle<float> (7.0f * scale, 7.0f * scale).withCentre (c));
}

void SketchLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& b, bool highlighted, bool)
{
    const auto bounds = b.getLocalBounds().toFloat();
    const auto pill = juce::Rectangle<float> (2.0f, bounds.getCentreY() - 11.0f, 44.0f, 22.0f);
    const auto seed = seedOf (b.getName() + b.getButtonText());

    juce::Path outline;
    outline.addRoundedRectangle (pill, 11.0f);
    g.setColour (highlighted ? colour::tint.withAlpha (0.5f) : colour::knob);
    g.fillPath (outline);

    // Slightly irregular outline: two passes with a small offset.
    stroke (g, outline, 1.8f);
    g.setColour (colour::ink.withAlpha (0.35f));
    g.strokePath (outline, juce::PathStrokeType (1.0f), juce::AffineTransform::translation (0.6f, -0.5f));

    const bool on = b.getToggleState();
    const auto dot = juce::Rectangle<float> (14.0f, 14.0f)
                         .withCentre ({ on ? pill.getRight() - 11.0f : pill.getX() + 11.0f, pill.getCentreY() });
    const auto dotPath = ellipse (dot, seed, 0.4f);
    g.setColour (on ? colour::accent : colour::paper);
    g.fillPath (dotPath);
    stroke (g, dotPath, 1.5f);

    if (b.getButtonText().isNotEmpty())
    {
        g.setColour (colour::ink);
        g.setFont (fonts->hand (17.0f));
        g.drawText (b.getButtonText(), bounds.withLeft (pill.getRight() + 10.0f), juce::Justification::centredLeft, false);
    }
}
