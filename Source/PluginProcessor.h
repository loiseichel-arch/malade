#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <atomic>
#include "dsp/VibratoEngine.h"
#include "Parameters.h"

class VibratoProcessor : public juce::AudioProcessor
{
public:
    VibratoProcessor();
    ~VibratoProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override  { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.05; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

    // Live values for the interface (written by the audio thread).
    std::atomic<float> uiRateNorm   { 0.0f };   // position of the main Rate knob, 0..1
    std::atomic<float> uiRateHz     { 5.0f };
    std::atomic<float> uiDepthPct   { 0.0f };
    std::atomic<float> uiLevelDb    { -120.0f };
    std::atomic<float> uiPhase      { 0.0f };
    std::atomic<double> uiBpm       { 120.0 };

private:
    // Direct pointers to the parameter values (no lookups on the audio thread).
    struct Raw
    {
        std::atomic<float> *rate, *rateDiv, *sync, *depth, *shape, *width, *flutter, *vintage, *mix, *output, *active,
                           *threshold, *rateMove, *rate1, *rate2, *rate1Div, *rate2Div, *rateTime,
                           *depthMove, *delay, *depth1, *depth2, *depthTime;
    } raw {};
    juce::RangedAudioParameter* rateParam = nullptr;

    vib::Engine engine;
    double bpm = 120.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VibratoProcessor)
};
