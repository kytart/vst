#include "PluginProcessor.h"

namespace
{
    // Frequency and time both feel logarithmic to the ear, so a linear knob would
    // cram everything audible into the bottom of its travel. setSkewForCentre puts
    // the given value at the halfway point of the knob.
    juce::NormalisableRange<float> skewedRange (float min, float max, float centre, float interval)
    {
        juce::NormalisableRange<float> range { min, max, interval };
        range.setSkewForCentre (centre);
        return range;
    }
}

juce::AudioProcessorValueTreeState::ParameterLayout OrbitDelayAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "time", 1 }, "Time",
        skewedRange (1.0f, 2000.0f, 300.0f, 0.01f), 400.0f,
        juce::AudioParameterFloatAttributes().withLabel ("ms")));

    // Capped below unity: without a saturator in the loop, >=100% grows without bound.
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "feedback", 1 }, "Feedback",
        juce::NormalisableRange<float> { 0.0f, 95.0f, 0.1f }, 35.0f,
        juce::AudioParameterFloatAttributes().withLabel ("%")));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "mix", 1 }, "Mix",
        juce::NormalisableRange<float> { 0.0f, 100.0f, 0.1f }, 35.0f,
        juce::AudioParameterFloatAttributes().withLabel ("%")));

    // Geometric centre of 200..20000 is 2000, which makes the knob feel even.
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "tone", 1 }, "Tone",
        skewedRange (200.0f, 20000.0f, 2000.0f, 1.0f), 6000.0f,
        juce::AudioParameterFloatAttributes().withLabel ("Hz")));

    return layout;
}

OrbitDelayAudioProcessor::OrbitDelayAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMETERS", createParameterLayout())
{
    timeMsParam   = apvts.getRawParameterValue ("time");
    feedbackParam = apvts.getRawParameterValue ("feedback");
    mixParam      = apvts.getRawParameterValue ("mix");
    toneHzParam   = apvts.getRawParameterValue ("tone");
}

void OrbitDelayAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;

    const juce::dsp::ProcessSpec spec {
        sampleRate,
        static_cast<juce::uint32> (samplesPerBlock),
        static_cast<juce::uint32> (juce::jmax (1, getTotalNumOutputChannels()))
    };

    // All allocation happens here. processBlock must never allocate.
    delayLine.prepare (spec);
    delayLine.setMaximumDelayInSamples (static_cast<int> (std::ceil (maxDelaySeconds * sampleRate)) + 1);
    delayLine.reset();
}

void OrbitDelayAudioProcessor::releaseResources()
{
    delayLine.reset();
}

bool OrbitDelayAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& out = layouts.getMainOutputChannelSet();

    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;

    return layouts.getMainInputChannelSet() == out;
}

void OrbitDelayAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    for (auto i = getTotalNumInputChannels(); i < getTotalNumOutputChannels(); ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    const auto numSamples  = buffer.getNumSamples();
    const auto numChannels = juce::jmin (getTotalNumInputChannels(), getTotalNumOutputChannels());

    const auto delaySamples = juce::jlimit (
        1.0f,
        static_cast<float> (maxDelaySeconds * currentSampleRate),
        static_cast<float> (timeMsParam->load() * 0.001 * currentSampleRate));

    const auto feedback = feedbackParam->load() * 0.01f;

    // Equal power: at 50% both signals sit at ~0.707, so the midpoint doesn't dip in level.
    const auto mix      = mixParam->load() * 0.01f;
    const auto dryGain  = std::cos (mix * juce::MathConstants<float>::halfPi);
    const auto wetGain  = std::sin (mix * juce::MathConstants<float>::halfPi);

    for (int channel = 0; channel < numChannels; ++channel)
    {
        auto* samples = buffer.getWritePointer (channel);

        // Sample-by-sample, because each output feeds back into the next input.
        for (int i = 0; i < numSamples; ++i)
        {
            const auto dry     = samples[i];
            const auto delayed = delayLine.popSample (channel, delaySamples, true);

            delayLine.pushSample (channel, dry + delayed * feedback);

            samples[i] = dry * dryGain + delayed * wetGain;
        }
    }
}

juce::AudioProcessorEditor* OrbitDelayAudioProcessor::createEditor()
{
    return new juce::GenericAudioProcessorEditor (*this);
}

void OrbitDelayAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void OrbitDelayAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new OrbitDelayAudioProcessor();
}
