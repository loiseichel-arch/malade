// Components.h
// Knob with label, LFO shape selector, LFO scope, input meter, small buttons.

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <optional>
#include "Sketch.h"

// ============================================================================
// A knob with its label and value underneath, attached to a parameter.
// setLive() locks it and makes it show a live value (envelope moves).
class SketchKnob : public juce::Component
{
public:
    SketchKnob (const juce::String& labelText, int knobDiameter)
        : label (labelText), diameter (knobDiameter)
    {
        slider.setName (labelText);
        slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f,
                                    juce::MathConstants<float>::pi * 2.75f, true);
        slider.setMouseDragSensitivity (220);
        slider.onValueChange = [this] { repaint(); };
        addAndMakeVisible (slider);
    }

    void attach (juce::AudioProcessorValueTreeState& state, const char* paramId)
    {
        attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (state, paramId, slider);
        if (auto* p = state.getParameter (paramId))
            slider.setDoubleClickReturnValue (true, p->convertFrom0to1 (p->getDefaultValue()),
                                              juce::ModifierKeys::altModifier);
        slider.setTooltip (label.toLowerCase() + " (alt-click to reset)");
    }

    // norm = 0..1 position to show, text = value to print. nullopt = normal knob.
    void setLive (std::optional<float> norm, const juce::String& text = {})
    {
        if (norm.has_value())
        {
            slider.getProperties().set ("overridePos", (double) *norm);
            liveText = text;
            slider.setInterceptsMouseClicks (false, false);
        }
        else if (slider.getProperties().contains ("overridePos"))
        {
            slider.getProperties().remove ("overridePos");
            liveText.clear();
            slider.setInterceptsMouseClicks (true, true);
        }
        slider.repaint();
        repaint();
    }

    void setLed (bool on)  { if (led != on) { led = on; repaint(); } }

    void resized() override
    {
        auto b = getLocalBounds();
        slider.setBounds (b.removeFromTop (diameter + 6).withSizeKeepingCentre (diameter + 6, diameter + 6));
    }

    void paint (juce::Graphics& g) override
    {
        const bool big = diameter >= 90;
        auto b = getLocalBounds().toFloat();
        b.removeFromTop ((float) diameter + 6.0f);

        auto labelRow = b.removeFromTop (big ? 22.0f : 18.0f);
        const auto font = fonts->hand (big ? 19.0f : 15.0f);
        g.setFont (font);
        g.setColour (sketch::colour::ink);

        const float textW = juce::GlyphArrangement::getStringWidth (font, label);
        const float ledW = led ? 14.0f : 0.0f;
        const float x0 = labelRow.getCentreX() - (textW + ledW) * 0.5f;
        g.drawText (label, juce::Rectangle<float> (x0, labelRow.getY(), textW + 2.0f, labelRow.getHeight()),
                    juce::Justification::centredLeft, false);

        if (led)
        {
            const auto dot = juce::Rectangle<float> (8.0f, 8.0f).withCentre ({ x0 + textW + 9.0f, labelRow.getCentreY() });
            g.setColour (sketch::colour::accent.withAlpha (0.35f));
            g.fillEllipse (dot.expanded (3.0f));
            g.setColour (sketch::colour::accent);
            g.fillEllipse (dot);
            g.setColour (sketch::colour::ink);
            g.drawEllipse (dot, 1.3f);
        }

        const auto value = liveText.isNotEmpty() ? liveText : slider.getTextFromValue (slider.getValue());
        g.setColour (sketch::colour::soft);
        g.setFont (fonts->hand (big ? 17.0f : 14.0f));
        g.drawText (value, b.removeFromTop (big ? 20.0f : 17.0f), juce::Justification::centred, false);
    }

    juce::Slider slider;

private:
    juce::String label, liveText;
    int diameter;
    bool led = false;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    juce::SharedResourcePointer<sketch::Fonts> fonts;
};

// ============================================================================
// Little waveform icons, drawn in a 20 x 12 box.
inline juce::Path shapeIcon (int shape)
{
    juce::Path p;
    switch (shape)
    {
        case 0: p.startNewSubPath (1, 6); p.quadraticTo (5.5f, -2, 10, 6); p.quadraticTo (14.5f, 14, 19, 6); break;
        case 1: p.startNewSubPath (1, 6); p.lineTo (5.5f, 2); p.lineTo (14.5f, 10); p.lineTo (19, 6); break;
        case 2: p.startNewSubPath (1, 10); p.lineTo (1, 2); p.lineTo (10, 2); p.lineTo (10, 10); p.lineTo (19, 10); p.lineTo (19, 2); break;
        case 3: p.startNewSubPath (1, 10); p.lineTo (10, 2); p.lineTo (10, 10); p.lineTo (19, 2); break;
        case 4: p.startNewSubPath (1, 2); p.lineTo (10, 10); p.lineTo (10, 2); p.lineTo (19, 10); break;
        default: p.startNewSubPath (1, 7); p.lineTo (5, 7); p.lineTo (5, 2); p.lineTo (9, 2); p.lineTo (9, 9);
                 p.lineTo (13, 9); p.lineTo (13, 4); p.lineTo (19, 4); break;
    }
    return p;
}

// Pitch shape of the LFO (what you hear), for the scope. p = 0..1, k = cycle.
inline float pitchShape (int shape, float p, int k)
{
    switch (shape)
    {
        case 0: return std::sin (juce::MathConstants<float>::twoPi * p);
        case 1: return p < 0.25f ? 4.0f * p : (p < 0.75f ? 2.0f - 4.0f * p : 4.0f * p - 4.0f);
        case 2: return p < 0.5f ? 1.0f : -1.0f;
        case 3: return 2.0f * p - 1.0f;
        case 4: return 1.0f - 2.0f * p;
        default:
        {
            // Illustration of a smooth random wander.
            const float a = (float) std::sin (k * 12.9898 + 1.0), b = (float) std::sin ((k + 1) * 12.9898 + 1.0);
            return a + (b - a) * 0.5f * (1.0f - std::cos (juce::MathConstants<float>::pi * p));
        }
    }
}

// ============================================================================
// Six buttons choosing the LFO shape.
class ShapeSelector : public juce::Component
{
public:
    ShapeSelector (juce::AudioProcessorValueTreeState& state)
    {
        const juce::StringArray names { "Sine", "Triangle", "Square", "Saw up", "Saw down", "Random" };
        for (int i = 0; i < 6; ++i)
        {
            auto* b = buttons.add (new ShapeButton (i, names[i]));
            b->onClick = [this, i]
            {
                if (attachment == nullptr) return;
                // Alt-click: back to the default shape (Sine).
                const bool alt = juce::ModifierKeys::currentModifiers.isAltDown();
                attachment->setValueAsCompleteGesture (alt ? (float) defaultShape : (float) i);
            };
            addAndMakeVisible (b);
        }
        if (auto* p = state.getParameter ("shape"))
        {
            defaultShape = juce::roundToInt (p->convertFrom0to1 (p->getDefaultValue()));
            attachment = std::make_unique<juce::ParameterAttachment> (*p, [this] (float v) { select (juce::roundToInt (v)); });
            attachment->sendInitialUpdate();
        }
    }

    void resized() override
    {
        const int n = buttons.size(), gap = 6;
        const int w = (getWidth() - gap * (n - 1)) / n;
        for (int i = 0; i < n; ++i)
            buttons[i]->setBounds (i * (w + gap), 0, w, getHeight());
    }

private:
    struct ShapeButton : juce::Button
    {
        ShapeButton (int s, const juce::String& n) : juce::Button (n), shape (s) { setTooltip (n); }

        void paintButton (juce::Graphics& g, bool over, bool) override
        {
            auto r = getLocalBounds().toFloat().reduced (1.5f);
            sketch::box (g, r, sketch::seedOf (getName()),
                         selected ? sketch::colour::tint : (over ? sketch::colour::paper : sketch::colour::knob), 1.8f);
            auto icon = shapeIcon (shape);
            icon.applyTransform (juce::AffineTransform::scale (1.1f).translated (r.getCentreX() - 11.0f, r.getCentreY() - 6.6f));
            sketch::stroke (g, icon, 1.8f);
        }

        int shape;
        bool selected = false;
    };

    void select (int index)
    {
        for (int i = 0; i < buttons.size(); ++i)
        {
            auto* b = static_cast<ShapeButton*> (buttons[i]);
            b->selected = (i == index);
            b->repaint();
        }
    }

    juce::OwnedArray<juce::Button> buttons;
    std::unique_ptr<juce::ParameterAttachment> attachment;
    int defaultShape = 0;
};

// ============================================================================
// Shows two cycles of the LFO pitch shape (static drawing).
class LfoScope : public juce::Component
{
public:
    void setShape (int newShape)
    {
        if (newShape != shape)
        {
            shape = newShape;
            repaint();
        }
    }

    void paint (juce::Graphics& g) override
    {
        const auto r = getLocalBounds().toFloat().reduced (1.5f);
        sketch::box (g, r, 991);

        const auto inner = r.reduced (8.0f, 14.0f);
        const float mid = inner.getCentreY(), amp = inner.getHeight() * 0.5f;

        juce::Path centre;
        centre.startNewSubPath (inner.getX(), mid);
        centre.lineTo (inner.getRight(), mid);
        const float dashes[] = { 4.0f, 5.0f };
        juce::Path dashed;
        juce::PathStrokeType (1.0f).createDashedStroke (dashed, centre, dashes, 2);
        g.setColour (sketch::colour::ink.withAlpha (0.35f));
        g.fillPath (dashed);

        juce::Path wave;
        const int n = 240;
        for (int i = 0; i <= n; ++i)
        {
            const float x = (float) i / (float) n;
            const float cycles = x * 2.0f;
            const int k = (int) cycles;
            const float p = juce::jmin (0.9999f, cycles - (float) k);
            const juce::Point<float> pt (inner.getX() + x * inner.getWidth(), mid - pitchShape (shape, p, k) * amp);
            if (i == 0) wave.startNewSubPath (pt); else wave.lineTo (pt);
        }
        sketch::stroke (g, wave, 2.2f);
    }

private:
    int shape = 0;
};

// ============================================================================
// "Rate" LED next to the LFO title, like the one on a Jupiter-4: its
// brightness follows the LFO (0 = off, 1 = fully lit), fading smoothly.
class RateLed : public juce::Component
{
public:
    RateLed() { setInterceptsMouseClicks (false, false); }

    void setBrightness (float b)
    {
        b = juce::jlimit (0.0f, 1.0f, b);
        if (std::abs (b - brightness) > 0.01f)
        {
            brightness = b;
            repaint();
        }
    }

    void paint (juce::Graphics& g) override
    {
        const auto c = getLocalBounds().toFloat().getCentre();
        const auto dot = juce::Rectangle<float> (10.0f, 10.0f).withCentre (c);
        const float b = brightness;

        g.setColour (sketch::colour::accent.withAlpha (0.45f * b));
        g.fillEllipse (dot.expanded (3.0f + 2.0f * b));
        g.setColour (sketch::colour::knob.interpolatedWith (sketch::colour::accent, 0.15f + 0.85f * b));
        g.fillEllipse (dot);
        g.setColour (sketch::colour::ink);
        g.drawEllipse (dot, 1.3f);
    }

private:
    float brightness = 0.0f;
};

// ============================================================================
// On/off switch that goes back to its default state on alt-click.
class ResetToggle : public juce::ToggleButton
{
public:
    explicit ResetToggle (const juce::String& buttonText = {}, bool defaultOn = false)
        : juce::ToggleButton (buttonText), defaultState (defaultOn) {}

protected:
    void clicked (const juce::ModifierKeys& mods) override
    {
        if (mods.isAltDown())
            setToggleState (defaultState, juce::sendNotificationSync);
        juce::ToggleButton::clicked (mods);
    }

private:
    bool defaultState;
};

// ============================================================================
// Input level bar with the threshold mark.
class LevelMeter : public juce::Component
{
public:
    void setLevels (float levelDb, float thresholdDb)
    {
        if (std::abs (levelDb - level) > 0.3f || std::abs (thresholdDb - threshold) > 0.05f)
        {
            level = levelDb; threshold = thresholdDb;
            repaint();
        }
    }

    void paint (juce::Graphics& g) override
    {
        const auto r = getLocalBounds().toFloat().reduced (1.5f);
        auto toX = [&] (float db) { return r.getX() + r.getWidth() * juce::jlimit (0.0f, 1.0f, (db + 60.0f) / 60.0f); };

        g.setColour (sketch::colour::knob);
        g.fillRect (r);
        g.setColour (sketch::colour::accent.withAlpha (level > threshold ? 0.85f : 0.45f));
        g.fillRect (r.withRight (toX (level)).reduced (0.0f, 2.0f));
        g.setColour (sketch::colour::ink);
        g.fillRect (juce::Rectangle<float> (2.0f, r.getHeight() + 4.0f).withCentre ({ toX (threshold), r.getCentreY() }));
        sketch::stroke (g, sketch::rectStroke (r, 555, 0.5f, 1.5f), 1.8f);
    }

private:
    float level = -120.0f, threshold = -30.0f;
};

// ============================================================================
// Text buttons drawn by hand: "envelope" tab and the close chevron.
class SketchTextButton : public juce::Button
{
public:
    SketchTextButton (const juce::String& text, bool chevronDown, bool filled)
        : juce::Button (text), down (chevronDown), tinted (filled) {}

    void paintButton (juce::Graphics& g, bool over, bool) override
    {
        auto r = getLocalBounds().toFloat().reduced (2.0f);
        sketch::box (g, r, sketch::seedOf (getName()),
                     tinted ? (over ? sketch::colour::tint.darker (0.05f) : sketch::colour::tint)
                            : (over ? sketch::colour::paper : sketch::colour::knob), 2.0f);

        juce::Path chev;
        const float cy = r.getCentreY();
        float cx = r.getCentreX();
        if (getName().isNotEmpty())
        {
            g.setColour (sketch::colour::ink);
            g.setFont (fonts->title (26.0f));
            g.drawText (getName(), r.withTrimmedRight (26.0f), juce::Justification::centred, false);
            cx = r.getRight() - 18.0f;
        }
        if (down) { chev.startNewSubPath (cx - 5, cy - 2.5f); chev.lineTo (cx, cy + 2.5f); chev.lineTo (cx + 5, cy - 2.5f); }
        else      { chev.startNewSubPath (cx - 5, cy + 2.5f); chev.lineTo (cx, cy - 2.5f); chev.lineTo (cx + 5, cy + 2.5f); }
        sketch::stroke (g, chev, 2.0f);
    }

    void setChevronDown (bool d) { down = d; repaint(); }

private:
    bool down, tinted;
    juce::SharedResourcePointer<sketch::Fonts> fonts;
};
