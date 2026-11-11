#pragma once

#include "PluginProcessor.h"
#include "ui/SketchLookAndFeel.h"
#include "ui/Components.h"

class VibratoEditor : public juce::AudioProcessorEditor,
                      private juce::Timer
{
public:
    explicit VibratoEditor (VibratoProcessor&);
    ~VibratoEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void setEnvelopeOpen (bool open);
    bool paramOn (const char* pid) const;

    static constexpr int W = 760, mainH = 460, envH = 290;

    VibratoProcessor& proc;
    SketchLookAndFeel laf;
    juce::SharedResourcePointer<sketch::Fonts> fonts;
    juce::TooltipWindow tooltips { this, 600 };

    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    // ---- Main window ----
    juce::ToggleButton activeButton { "on" }, syncButton { "tempo sync" };
    std::unique_ptr<ButtonAttachment> activeAtt, syncAtt;

    LfoScope scope;
    ShapeSelector shapes { proc.apvts };

    SketchKnob rate     { "RATE", 112 }, rateSync { "RATE", 112 }, depth { "DEPTH", 112 };
    SketchKnob width    { "WIDTH", 54 }, flutter { "FLUTTER", 54 }, vintage { "VINTAGE", 54 },
               mix      { "MIX", 54 },   output  { "OUTPUT", 54 };

    SketchTextButton envelopeButton { "envelope", true, true };

    // ---- Envelope panel ----
    SketchTextButton closeButton { "", false, false };
    SketchKnob threshold { "THRESHOLD", 58 };
    LevelMeter meter;

    juce::ToggleButton rateMoveButton, depthMoveButton;
    std::unique_ptr<ButtonAttachment> rateMoveAtt, depthMoveAtt;

    SketchKnob rate1 { "RATE 1", 54 }, rate2 { "RATE 2", 54 }, rate1Sync { "RATE 1", 54 }, rate2Sync { "RATE 2", 54 },
               rateTime { "TIME", 54 };
    SketchKnob delay { "DELAY", 54 }, depth1 { "DEPTH 1", 54 }, depth2 { "DEPTH 2", 54 }, depthTime { "TIME", 54 };

    bool envelopeOpen = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VibratoEditor)
};
