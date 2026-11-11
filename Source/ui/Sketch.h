// Sketch.h
// The hand-drawn look: colours, fonts and slightly wobbly paths.

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <cmath>
#include <cstdint>

#if VIBRATO_HAS_FONTS
 #include "BinaryData.h"
#endif

namespace sketch
{

namespace colour
{
    inline const juce::Colour paper   { 0xFFFBF8F1 };
    inline const juce::Colour knob    { 0xFFFFFDF7 };
    inline const juce::Colour ink     { 0xFF2E2C38 };
    inline const juce::Colour soft    { 0xFF5B5866 };
    inline const juce::Colour accent  { 0xFFC2553F };
    inline const juce::Colour tint    { 0xFFF4DCD3 };
}

// ---- Fonts -----------------------------------------------------------------
// Uses fonts dropped in Assets/Fonts when present (Patrick Hand for text,
// Caveat for titles), otherwise the macOS fonts Chalkboard SE / Marker Felt.
// Shared through juce::SharedResourcePointer<Fonts>, so the typefaces are
// freed with the last editor (never at static shutdown).
struct Fonts
{
    Fonts()
    {
        handTf  = find ("PatrickHand");
        titleTf = find ("Caveat-Bold");
        if (titleTf == nullptr) titleTf = find ("Caveat");
    }

    juce::Font hand (float height) const
    {
        if (handTf != nullptr) return juce::Font (juce::FontOptions (handTf).withHeight (height));
        return juce::Font (juce::FontOptions ("Chalkboard SE", height, juce::Font::plain));
    }

    juce::Font title (float height) const
    {
        if (titleTf != nullptr) return juce::Font (juce::FontOptions (titleTf).withHeight (height));
        return juce::Font (juce::FontOptions ("Marker Felt", height, juce::Font::bold));
    }

private:
    juce::Typeface::Ptr handTf, titleTf;

    static juce::Typeface::Ptr find (const char* key)
    {
       #if VIBRATO_HAS_FONTS
        for (int i = 0; i < BinaryData::namedResourceListSize; ++i)
        {
            const juce::String file (BinaryData::getNamedResourceOriginalFilename (BinaryData::namedResourceList[i]));
            if (file.containsIgnoreCase (key))
            {
                int size = 0;
                if (auto* data = BinaryData::getNamedResource (BinaryData::namedResourceList[i], size))
                    return juce::Typeface::createSystemTypefaceFor (data, (size_t) size);
            }
        }
       #else
        juce::ignoreUnused (key);
       #endif
        return nullptr;
    }
};

// ---- Wobbly paths ----------------------------------------------------------
// The wobble is smooth (two slow sine waves with a seeded phase), so lines
// look drawn by hand rather than noisy, and stay the same on every repaint.

inline float seededPhase (uint32_t seed, int k)
{
    uint32_t x = seed * 2654435761u + (uint32_t) k * 40503u + 0x9E3779B9u;
    x ^= x >> 15; x *= 2246822519u; x ^= x >> 13; x *= 3266489917u; x ^= x >> 16;
    return (float) (x & 0xFFFF) / 65535.0f * juce::MathConstants<float>::twoPi;
}

// t in 0..1. Integer frequencies, so closed shapes join up.
inline float wobble (float t, uint32_t seed, float amount)
{
    const float tp = juce::MathConstants<float>::twoPi * t;
    return amount * (0.6f * std::sin (2.0f * tp + seededPhase (seed, 0))
                   + 0.4f * std::sin (5.0f * tp + seededPhase (seed, 1)));
}

inline juce::Path ellipse (juce::Rectangle<float> r, uint32_t seed, float amount = 0.8f)
{
    juce::Path p;
    const auto c = r.getCentre();
    const float rx = r.getWidth() * 0.5f, ry = r.getHeight() * 0.5f;
    const int n = 72;
    for (int i = 0; i <= n; ++i)
    {
        const float t = (float) i / (float) n;
        const float a = juce::MathConstants<float>::twoPi * t;
        const float w = wobble (t, seed, amount);
        const juce::Point<float> pt (c.x + (rx + w) * std::sin (a), c.y - (ry + w) * std::cos (a));
        if (i == 0) p.startNewSubPath (pt); else p.lineTo (pt);
    }
    p.closeSubPath();
    return p;
}

inline juce::Path line (juce::Point<float> a, juce::Point<float> b, uint32_t seed, float amount = 0.8f)
{
    juce::Path p;
    const float len = a.getDistanceFrom (b);
    const int n = juce::jmax (2, (int) (len / 5.0f));
    const auto dir = (b - a) / juce::jmax (0.001f, len);
    const juce::Point<float> normal (-dir.y, dir.x);
    for (int i = 0; i <= n; ++i)
    {
        const float t = (float) i / (float) n;
        const float env = std::sin (juce::MathConstants<float>::pi * t); // ends stay put
        const auto pt = a + (b - a) * t + normal * (wobble (t, seed, amount) * env);
        if (i == 0) p.startNewSubPath (pt); else p.lineTo (pt);
    }
    return p;
}

// Four slightly overshooting strokes, like a quick pen rectangle.
inline juce::Path rectStroke (juce::Rectangle<float> r, uint32_t seed, float amount = 0.8f, float overshoot = 2.0f)
{
    juce::Path p;
    const auto tl = r.getTopLeft(), tr = r.getTopRight(), br = r.getBottomRight(), bl = r.getBottomLeft();
    const float o = overshoot;
    p.addPath (line ({ tl.x - o, tl.y }, { tr.x + o * 0.5f, tr.y }, seed + 1, amount));
    p.addPath (line ({ tr.x, tr.y - o * 0.5f }, { br.x, br.y + o }, seed + 2, amount));
    p.addPath (line ({ br.x + o, br.y }, { bl.x - o * 0.5f, bl.y }, seed + 3, amount));
    p.addPath (line ({ bl.x, bl.y + o * 0.5f }, { tl.x, tl.y - o }, seed + 4, amount));
    return p;
}

inline void stroke (juce::Graphics& g, const juce::Path& p, float width, juce::Colour c = colour::ink)
{
    g.setColour (c);
    g.strokePath (p, juce::PathStrokeType (width, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

// A boxed area: paper-white fill (or a tint) plus a hand-drawn outline.
inline void box (juce::Graphics& g, juce::Rectangle<float> r, uint32_t seed, juce::Colour fill = colour::knob, float width = 2.0f)
{
    g.setColour (fill);
    g.fillRoundedRectangle (r.reduced (1.0f), 4.0f);
    stroke (g, rectStroke (r, seed), width);
}

inline uint32_t seedOf (const juce::String& s) { return (uint32_t) s.hashCode(); }

} // namespace sketch
