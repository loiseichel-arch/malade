#pragma once

#include "Sketch.h"

// Draws the knobs and the on/off switches in the hand-drawn style.
//
// A knob can show a live value instead of its own position (used when the
// envelope moves Rate or Depth): set the slider property "overridePos"
// (0..1) and it is drawn there.
class SketchLookAndFeel : public juce::LookAndFeel_V4
{
public:
    SketchLookAndFeel();

    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height,
                           float sliderPos, float startAngle, float endAngle, juce::Slider&) override;

    void drawToggleButton (juce::Graphics&, juce::ToggleButton&,
                           bool highlighted, bool down) override;

    juce::SharedResourcePointer<sketch::Fonts> fonts;
};
