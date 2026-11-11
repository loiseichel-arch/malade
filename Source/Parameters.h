// Parameters.h
// Every automatable parameter of the plugin, in one place.

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <array>

namespace params
{

// Parameter IDs (also used as automation names' keys in Pro Tools).
namespace id
{
    inline constexpr const char* rate      = "rate";
    inline constexpr const char* rateDiv   = "rateDiv";
    inline constexpr const char* sync      = "sync";
    inline constexpr const char* depth     = "depth";
    inline constexpr const char* shape     = "shape";
    inline constexpr const char* width     = "width";
    inline constexpr const char* flutter   = "flutter";
    inline constexpr const char* vintage   = "vintage";
    inline constexpr const char* mix       = "mix";
    inline constexpr const char* output    = "output";
    inline constexpr const char* active    = "active";

    inline constexpr const char* threshold = "threshold";
    inline constexpr const char* rateMove  = "rateMove";
    inline constexpr const char* rate1     = "rate1";
    inline constexpr const char* rate2     = "rate2";
    inline constexpr const char* rate1Div  = "rate1Div";
    inline constexpr const char* rate2Div  = "rate2Div";
    inline constexpr const char* rateTime  = "rateTime";
    inline constexpr const char* depthMove = "depthMove";
    inline constexpr const char* delay     = "delay";
    inline constexpr const char* depth1    = "depth1";
    inline constexpr const char* depth2    = "depth2";
    inline constexpr const char* depthTime = "depthTime";
}

// Tempo divisions, slowest to fastest (length in quarter notes).
struct Division { const char* name; double beats; };

inline constexpr std::array<Division, 14> divisions {{
    { "1/1",   4.0 },       { "1/2",   2.0 },       { "1/4.",  1.5 },
    { "1/2T",  4.0 / 3.0 }, { "1/4",   1.0 },       { "1/8.",  0.75 },
    { "1/4T",  2.0 / 3.0 }, { "1/8",   0.5 },       { "1/16.", 0.375 },
    { "1/8T",  1.0 / 3.0 }, { "1/16",  0.25 },      { "1/16T", 1.0 / 6.0 },
    { "1/32",  0.125 },     { "1/32T", 1.0 / 12.0 }
}};

inline juce::StringArray divisionNames()
{
    juce::StringArray s;
    for (auto& d : divisions) s.add (d.name);
    return s;
}

inline double divisionToHz (int index, double bpm)
{
    index = juce::jlimit (0, (int) divisions.size() - 1, index);
    return (bpm / 60.0) / divisions[(size_t) index].beats;
}

// Inverse, as a continuous position 0..1 along the division list
// (used to animate a synced knob while the envelope moves it).
inline float hzToDivisionNorm (double hz, double bpm)
{
    const auto n = (int) divisions.size();
    const double beats = (bpm / 60.0) / juce::jmax (0.001, hz);
    if (beats >= divisions[0].beats) return 0.0f;
    if (beats <= divisions[(size_t) n - 1].beats) return 1.0f;
    for (int i = 0; i < n - 1; ++i)
    {
        const double a = divisions[(size_t) i].beats, b = divisions[(size_t) i + 1].beats;
        if (beats <= a && beats >= b)
        {
            const double frac = std::log (a / beats) / std::log (a / b);
            return (float) ((i + frac) / (n - 1));
        }
    }
    return 0.0f;
}

inline juce::StringArray shapeNames()
{
    return { "Sine", "Triangle", "Square", "Saw Up", "Saw Down", "Random" };
}

// ---- Value formatting ----
inline juce::String fmtHz (float v, int)  { return juce::String (v, v < 10.0f ? 2 : 1) + " Hz"; }
inline juce::String fmtPct (float v, int) { return juce::String (juce::roundToInt (v)) + "%"; }
inline juce::String fmtDb (float v, int)  { return juce::String (v, 1) + " dB"; }
inline juce::String fmtMs (float v, int)  { return juce::String (juce::roundToInt (v)) + " ms"; }
inline juce::String fmtSec (float v, int) { return v < 1.0f ? juce::String (juce::roundToInt (v * 1000.0f)) + " ms"
                                                            : juce::String (v, 2) + " s"; }

inline juce::AudioProcessorValueTreeState::ParameterLayout createLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout layout;

    auto addFloat = [&] (const char* pid, const char* name, NormalisableRange<float> range, float def,
                         String (*fmt) (float, int), const char* unit)
    {
        layout.add (std::make_unique<AudioParameterFloat> (
            ParameterID { pid, 1 }, name, range, def,
            AudioParameterFloatAttributes().withStringFromValueFunction (fmt).withLabel (unit)));
    };

    auto rateRange = [] { NormalisableRange<float> r (0.1f, 12.0f, 0.0f); r.setSkewForCentre (3.0f); return r; }();
    auto timeRange = [] { NormalisableRange<float> r (0.05f, 10.0f, 0.0f); r.setSkewForCentre (1.0f); return r; }();
    auto delayRange = [] { NormalisableRange<float> r (0.0f, 2000.0f, 1.0f); r.setSkewForCentre (300.0f); return r; }();
    const NormalisableRange<float> pct (0.0f, 100.0f, 0.1f);

    // Main window
    addFloat (id::rate,    "Rate",    rateRange, 5.0f, fmtHz, "Hz");
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { id::rateDiv, 1 }, "Rate Sync", divisionNames(), 9));
    layout.add (std::make_unique<AudioParameterBool>   (ParameterID { id::sync, 1 },    "Tempo Sync", false));
    addFloat (id::depth,   "Depth",   pct, 35.0f, fmtPct, "%");
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { id::shape, 1 },   "Shape", shapeNames(), 0));
    addFloat (id::width,   "Width",   pct, 0.0f,   fmtPct, "%");
    addFloat (id::flutter, "Flutter", pct, 0.0f,   fmtPct, "%");
    addFloat (id::vintage, "Vintage", pct, 0.0f,   fmtPct, "%");
    addFloat (id::mix,     "Mix",     pct, 100.0f, fmtPct, "%");
    addFloat (id::output,  "Output",  NormalisableRange<float> (-24.0f, 12.0f, 0.1f), 0.0f, fmtDb, "dB");
    layout.add (std::make_unique<AudioParameterBool>   (ParameterID { id::active, 1 },  "On", true));

    // Envelope panel
    addFloat (id::threshold, "Threshold", NormalisableRange<float> (-60.0f, 0.0f, 0.1f), -30.0f, fmtDb, "dB");
    layout.add (std::make_unique<AudioParameterBool>   (ParameterID { id::rateMove, 1 }, "Rate Move", false));
    addFloat (id::rate1, "Rate 1", rateRange, 8.0f, fmtHz, "Hz");
    addFloat (id::rate2, "Rate 2", rateRange, 2.0f, fmtHz, "Hz");
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { id::rate1Div, 1 }, "Rate 1 Sync", divisionNames(), 12));
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { id::rate2Div, 1 }, "Rate 2 Sync", divisionNames(), 7));
    addFloat (id::rateTime,  "Rate Time",  timeRange, 1.2f, fmtSec, "s");
    layout.add (std::make_unique<AudioParameterBool>   (ParameterID { id::depthMove, 1 }, "Depth Move", false));
    addFloat (id::delay,     "Delay",      delayRange, 250.0f, fmtMs, "ms");
    addFloat (id::depth1,    "Depth 1",    pct, 45.0f, fmtPct, "%");
    addFloat (id::depth2,    "Depth 2",    pct, 15.0f, fmtPct, "%");
    addFloat (id::depthTime, "Depth Time", timeRange, 0.8f, fmtSec, "s");

    return layout;
}

} // namespace params
