#include "PluginProcessor.h"
#include "PluginEditor.h"

VibratoProcessor::VibratoProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "VibratoState", params::createLayout())
{
    auto get = [this] (const char* pid)
    {
        auto* p = apvts.getRawParameterValue (pid);
        jassert (p != nullptr);
        return p;
    };
    using namespace params;
    raw.rate      = get (id::rate);      raw.rateDiv  = get (id::rateDiv);  raw.sync     = get (id::sync);
    raw.depth     = get (id::depth);     raw.shape    = get (id::shape);    raw.width    = get (id::width);
    raw.flutter   = get (id::flutter);   raw.vintage  = get (id::vintage);  raw.mix      = get (id::mix);
    raw.output    = get (id::output);    raw.active   = get (id::active);
    raw.threshold = get (id::threshold); raw.rateMove = get (id::rateMove);
    raw.rate1     = get (id::rate1);     raw.rate2    = get (id::rate2);
    raw.rate1Div  = get (id::rate1Div);  raw.rate2Div = get (id::rate2Div); raw.rateTime = get (id::rateTime);
    raw.depthMove = get (id::depthMove); raw.delay    = get (id::delay);
    raw.depth1    = get (id::depth1);    raw.depth2   = get (id::depth2);   raw.depthTime = get (id::depthTime);

    rateParam = apvts.getParameter (id::rate);
}

void VibratoProcessor::prepareToPlay (double sampleRate, int)
{
    engine.prepare (sampleRate, juce::jmax (1, getTotalNumOutputChannels()));
}

bool VibratoProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;
    return layouts.getMainInputChannelSet() == out;
}

void VibratoProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    for (auto ch = getTotalNumInputChannels(); ch < getTotalNumOutputChannels(); ++ch)
        buffer.clear (ch, 0, buffer.getNumSamples());

    // Host tempo, for tempo sync.
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
            if (auto b = pos->getBpm())
                if (*b > 1.0) bpm = *b;

    auto on  = [] (std::atomic<float>* a) { return a->load() > 0.5f; };
    auto idx = [] (std::atomic<float>* a) { return juce::roundToInt (a->load()); };
    const bool sync = on (raw.sync);

    vib::Params p;
    p.rateHz     = sync ? (float) params::divisionToHz (idx (raw.rateDiv), bpm) : raw.rate->load();
    p.depthPct   = raw.depth->load();
    p.shape      = (vib::Shape) juce::jlimit (0, 5, idx (raw.shape));
    p.widthPct   = raw.width->load();
    p.flutterPct = raw.flutter->load();
    p.vintagePct = raw.vintage->load();
    p.mixPct     = raw.mix->load();
    p.outputDb   = raw.output->load();
    p.active     = on (raw.active);

    p.thresholdDb  = raw.threshold->load();
    p.rateMove     = on (raw.rateMove);
    p.rate1Hz      = sync ? (float) params::divisionToHz (idx (raw.rate1Div), bpm) : raw.rate1->load();
    p.rate2Hz      = sync ? (float) params::divisionToHz (idx (raw.rate2Div), bpm) : raw.rate2->load();
    p.rateTimeSec  = raw.rateTime->load();
    p.depthMove    = on (raw.depthMove);
    p.delayMs      = raw.delay->load();
    p.depth1Pct    = raw.depth1->load();
    p.depth2Pct    = raw.depth2->load();
    p.depthTimeSec = raw.depthTime->load();

    engine.process (buffer.getArrayOfWritePointers(), buffer.getNumChannels(), buffer.getNumSamples(), p);

    // Live values for the knobs, LED and meters.
    const float hz = engine.getCurrentRateHz();
    uiRateHz.store (hz);
    uiBpm.store (bpm);
    if (sync)
        uiRateNorm.store (params::hzToDivisionNorm (hz, bpm));
    else if (rateParam != nullptr)
        uiRateNorm.store (rateParam->convertTo0to1 (juce::jlimit (0.1f, 12.0f, hz)));
    uiDepthPct.store (engine.getCurrentDepthPct());
    uiLevelDb.store (engine.getInputLevelDb());
    uiPhase.store (engine.getPhase());
    uiCycle.store ((int) (engine.getCycle() & 0x7fffffff));
}

void VibratoProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void VibratoProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessorEditor* VibratoProcessor::createEditor()
{
    return new VibratoEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new VibratoProcessor();
}
