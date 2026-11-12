#include "PluginEditor.h"

using namespace sketch;
namespace pid = params::id;

VibratoEditor::VibratoEditor (VibratoProcessor& p)
    : AudioProcessorEditor (&p), proc (p)
{
    setLookAndFeel (&laf);
    auto& st = proc.apvts;

    // ---- Main window ----
    activeAtt = std::make_unique<ButtonAttachment> (st, pid::active, activeButton);
    syncAtt   = std::make_unique<ButtonAttachment> (st, pid::sync,   syncButton);
    addAndMakeVisible (activeButton);
    addAndMakeVisible (syncButton);
    addAndMakeVisible (scope);
    addAndMakeVisible (rateLed);
    addAndMakeVisible (shapes);

    rate.attach     (st, pid::rate);
    rateSync.attach (st, pid::rateDiv);
    depth.attach    (st, pid::depth);
    width.attach    (st, pid::width);
    flutter.attach  (st, pid::flutter);
    vintage.attach  (st, pid::vintage);
    mix.attach      (st, pid::mix);
    output.attach   (st, pid::output);
    for (auto* k : { &rate, &rateSync, &depth, &width, &flutter, &vintage, &mix, &output })
        addAndMakeVisible (k);

    envelopeButton.onClick = [this] { setEnvelopeOpen (! envelopeOpen); };
    addAndMakeVisible (envelopeButton);

    // ---- Envelope panel ----
    closeButton.onClick = [this] { setEnvelopeOpen (false); };
    closeButton.setTooltip ("close");
    rateMoveButton.setName ("rateMove");
    depthMoveButton.setName ("depthMove");
    rateMoveButton.setTooltip ("rate follows the envelope");
    depthMoveButton.setTooltip ("depth follows the envelope");
    rateMoveAtt  = std::make_unique<ButtonAttachment> (st, pid::rateMove,  rateMoveButton);
    depthMoveAtt = std::make_unique<ButtonAttachment> (st, pid::depthMove, depthMoveButton);

    threshold.attach (st, pid::threshold);
    rate1.attach     (st, pid::rate1);
    rate2.attach     (st, pid::rate2);
    rate1Sync.attach (st, pid::rate1Div);
    rate2Sync.attach (st, pid::rate2Div);
    rateTime.attach  (st, pid::rateTime);
    delay.attach     (st, pid::delay);
    depth1.attach    (st, pid::depth1);
    depth2.attach    (st, pid::depth2);
    depthTime.attach (st, pid::depthTime);

    for (juce::Component* c : std::initializer_list<juce::Component*> {
             &closeButton, &threshold, &meter, &rateMoveButton, &depthMoveButton,
             &rate1, &rate2, &rate1Sync, &rate2Sync, &rateTime, &delay, &depth1, &depth2, &depthTime })
        addChildComponent (c);

    setEnvelopeOpen ((bool) st.state.getProperty ("envOpen", false));
    timerCallback();
    startTimerHz (60);   // smooth rate LED
}

VibratoEditor::~VibratoEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

bool VibratoEditor::paramOn (const char* id) const
{
    if (auto* v = proc.apvts.getRawParameterValue (id))
        return v->load() > 0.5f;
    return false;
}

void VibratoEditor::setEnvelopeOpen (bool open)
{
    envelopeOpen = open;
    proc.apvts.state.setProperty ("envOpen", open, nullptr);
    envelopeButton.setChevronDown (! open);

    for (juce::Component* c : std::initializer_list<juce::Component*> {
             &closeButton, &threshold, &meter, &rateMoveButton, &depthMoveButton, &rateTime,
             &delay, &depth1, &depth2, &depthTime })
        c->setVisible (open);

    const bool sync = paramOn (pid::sync);
    rate1.setVisible (open && ! sync);
    rate2.setVisible (open && ! sync);
    rate1Sync.setVisible (open && sync);
    rate2Sync.setVisible (open && sync);

    setSize (W, open ? mainH + envH : mainH);
}

void VibratoEditor::timerCallback()
{
    const bool sync      = paramOn (pid::sync);
    const bool rateMove  = paramOn (pid::rateMove);
    const bool depthMove = paramOn (pid::depthMove);

    // Synced knobs show tempo divisions instead of Hz.
    rate.setVisible (! sync);
    rateSync.setVisible (sync);
    rate1.setVisible (envelopeOpen && ! sync);
    rate2.setVisible (envelopeOpen && ! sync);
    rate1Sync.setVisible (envelopeOpen && sync);
    rate2Sync.setVisible (envelopeOpen && sync);

    // When the envelope drives Rate / Depth, the big knobs are locked and
    // show the live value, with the ENV LED lit.
    if (rateMove)
    {
        const float norm = proc.uiRateNorm.load();
        const float hz   = proc.uiRateHz.load();
        juce::String text;
        if (sync)
        {
            const int n = (int) params::divisions.size();
            text = params::divisions[(size_t) juce::jlimit (0, n - 1, juce::roundToInt (norm * (float) (n - 1)))].name;
        }
        else text = params::fmtHz (hz, 0);
        rate.setLive (norm, text);
        rateSync.setLive (norm, text);
    }
    else
    {
        rate.setLive (std::nullopt);
        rateSync.setLive (std::nullopt);
    }
    rate.setLed (rateMove);
    rateSync.setLed (rateMove);

    if (depthMove)
    {
        const float pct = proc.uiDepthPct.load();
        depth.setLive (juce::jlimit (0.0f, 1.0f, pct / 100.0f), params::fmtPct (pct, 0));
    }
    else depth.setLive (std::nullopt);
    depth.setLed (depthMove);

    if (auto* s = proc.apvts.getRawParameterValue (pid::shape))
    {
        const int shape = juce::roundToInt (s->load());
        scope.setShape (shape);

        // Rate LED: lit at the top of the LFO, dark at the bottom. Off when the plugin is off.
        const float v = pitchShape (shape, juce::jlimit (0.0f, 0.9999f, proc.uiPhase.load()), proc.uiCycle.load());
        rateLed.setBrightness (paramOn (pid::active) ? 0.5f * (1.0f + juce::jlimit (-1.0f, 1.0f, v)) : 0.0f);
    }

    if (envelopeOpen)
        if (auto* t = proc.apvts.getRawParameterValue (pid::threshold))
            meter.setLevels (proc.uiLevelDb.load(), t->load());
}

void VibratoEditor::paint (juce::Graphics& g)
{
    g.fillAll (colour::paper);

    // ---- Main window ----
    stroke (g, rectStroke ({ 7.0f, 7.0f, (float) W - 14.0f, (float) mainH - 14.0f }, 11, 1.0f, 3.0f), 2.4f);

    g.setColour (colour::accent);
    g.setFont (fonts->title (56.0f));
    g.drawText ("Malade", juce::Rectangle<float> (400.0f, 12.0f, 290.0f, 62.0f), juce::Justification::centredRight, false);
    g.setColour (colour::soft);
    g.setFont (fonts->hand (14.0f));
    g.drawText ("v0.2", juce::Rectangle<float> (694.0f, 44.0f, 40.0f, 18.0f), juce::Justification::centredLeft, false);

    auto heading = [&] (const juce::String& text, float x, float y)
    {
        g.setColour (colour::ink);
        g.setFont (fonts->hand (18.0f));
        g.drawText (text, juce::Rectangle<float> (x, y, 200.0f, 20.0f), juce::Justification::centredLeft, false);
        const float w = juce::GlyphArrangement::getStringWidth (fonts->hand (18.0f), text);
        stroke (g, line ({ x, y + 21.0f }, { x + w + 2.0f, y + 21.0f }, seedOf (text), 0.6f), 1.5f);
    };

    heading ("LFO", 28.0f, 80.0f);

    stroke (g, line ({ 318.0f, 84.0f }, { 318.0f, 292.0f }, 21, 1.6f), 1.6f, colour::ink.withAlpha (0.6f));
    stroke (g, line ({ 28.0f, 302.0f }, { 732.0f, 302.0f }, 22, 1.6f), 1.6f, colour::ink.withAlpha (0.6f));

    // ---- Envelope panel ----
    if (! envelopeOpen)
        return;

    const float y0 = (float) mainH;
    stroke (g, rectStroke ({ 7.0f, y0 + 2.0f, (float) W - 14.0f, (float) envH - 10.0f }, 12, 1.0f, 3.0f), 2.4f);

    g.setColour (colour::accent);
    g.setFont (fonts->title (38.0f));
    g.drawText ("envelope", juce::Rectangle<float> (28.0f, y0 + 12.0f, 300.0f, 42.0f), juce::Justification::centredLeft, false);

    heading ("DETECTION",  28.0f,  y0 + 62.0f);
    heading ("RATE MOVE",  188.0f, y0 + 62.0f);
    heading ("DEPTH MOVE", 468.0f, y0 + 62.0f);

    stroke (g, line ({ 172.0f, y0 + 64.0f }, { 172.0f, y0 + 262.0f }, 31, 1.4f), 1.6f, colour::ink.withAlpha (0.6f));
    stroke (g, line ({ 454.0f, y0 + 64.0f }, { 454.0f, y0 + 262.0f }, 32, 1.4f), 1.6f, colour::ink.withAlpha (0.6f));

    g.setColour (colour::soft);
    g.setFont (fonts->hand (14.0f));
    g.drawText ("input", juce::Rectangle<float> (28.0f, y0 + 230.0f, 130.0f, 18.0f), juce::Justification::centredLeft, false);
    g.drawText (juce::String (juce::CharPointer_UTF8 ("attack: rate 1 \xe2\x86\x92 sustain: rate 2")),
                juce::Rectangle<float> (188.0f, y0 + 236.0f, 260.0f, 18.0f), juce::Justification::centredLeft, false);
    g.drawText (juce::String (juce::CharPointer_UTF8 ("trigger \xe2\x86\x92 delay \xe2\x86\x92 depth 1 \xe2\x86\x92 depth 2")),
                juce::Rectangle<float> (468.0f, y0 + 236.0f, 270.0f, 18.0f), juce::Justification::centredLeft, false);
}

void VibratoEditor::resized()
{
    // ---- Main window ----
    activeButton.setBounds (28, 30, 110, 28);
    scope.setBounds  (28, 106, 272, 104);
    {
        // LED just right of the "LFO" title.
        const float w = juce::GlyphArrangement::getStringWidth (fonts->hand (18.0f), "LFO");
        rateLed.setBounds (28 + juce::roundToInt (w) + 8, 80, 20, 20);
    }
    shapes.setBounds (28, 218, 272, 32);
    syncButton.setBounds (28, 260, 180, 28);

    const juce::Rectangle<int> bigKnob (0, 0, 150, 160);
    rate.setBounds     (bigKnob.withCentre ({ 435, 186 }));
    rateSync.setBounds (bigKnob.withCentre ({ 435, 186 }));
    depth.setBounds    (bigKnob.withCentre ({ 633, 186 }));

    int x = 24;
    for (auto* k : { &width, &flutter, &vintage, &mix, &output })
    {
        k->setBounds (x, 316, 84, 100);
        x += 88;
    }
    envelopeButton.setBounds (590, 344, 142, 44);

    // ---- Envelope panel ----
    const int y0 = mainH;
    closeButton.setBounds (696, y0 + 18, 36, 36);

    threshold.setBounds (40, y0 + 92, 100, 104);
    meter.setBounds     (28, y0 + 208, 130, 18);

    rateMoveButton.setBounds  (392, y0 + 58, 52, 30);
    depthMoveButton.setBounds (682, y0 + 58, 52, 30);

    const int ky = y0 + 100;
    rate1.setBounds     (186, ky, 84, 100);
    rate1Sync.setBounds (186, ky, 84, 100);
    rate2.setBounds     (274, ky, 84, 100);
    rate2Sync.setBounds (274, ky, 84, 100);
    rateTime.setBounds  (362, ky, 84, 100);

    delay.setBounds     (462, ky, 68, 100);
    depth1.setBounds    (530, ky, 68, 100);
    depth2.setBounds    (598, ky, 68, 100);
    depthTime.setBounds (666, ky, 68, 100);
}
