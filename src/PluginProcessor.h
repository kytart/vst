#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

class OrbitDelayAudioProcessor final : public juce::AudioProcessor
{
public:
    OrbitDelayAudioProcessor();
    ~OrbitDelayAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }

    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return maxDelaySeconds; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    juce::AudioProcessorValueTreeState apvts;

private:
    static constexpr float maxDelaySeconds = 2.0f;

    // Linear interpolation lets the read position sit between samples, which is what
    // makes a moving delay time possible at all.
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> delayLine;

    // One pole (6 dB/oct) sitting inside the feedback path. Gentle on purpose: a
    // steeper filter collapses the repeats to mud within a few passes, and
    // resonance here would be a second gain stage inside a feedback loop.
    juce::dsp::FirstOrderTPTFilter<float> toneFilter;

    // Gliding the read position instead of jumping it is what turns a click into
    // the tape-style pitch bend people actually want from a delay.
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothedDelaySamples;
    static constexpr double timeGlideSeconds = 0.120;

    double currentSampleRate = 44100.0;

    // Cached raw pointers so processBlock never does a string lookup on the audio thread.
    std::atomic<float>* timeMsParam     = nullptr;
    std::atomic<float>* feedbackParam   = nullptr;
    std::atomic<float>* mixParam        = nullptr;
    std::atomic<float>* toneHzParam     = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OrbitDelayAudioProcessor)
};
