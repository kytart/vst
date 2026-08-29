#pragma once

#include "PluginProcessor.h"

#include <juce_gui_extra/juce_gui_extra.h>

#include <optional>

class OrbitDelayAudioProcessorEditor final : public juce::AudioProcessorEditor
{
public:
    explicit OrbitDelayAudioProcessorEditor (OrbitDelayAudioProcessor&);
    ~OrbitDelayAudioProcessorEditor() override = default;

    void resized() override;

private:
    // Serves the web UI out of the zip embedded in the binary.
    std::optional<juce::WebBrowserComponent::Resource> getResource (const juce::String& url) const;

    // Relay names are the parameter IDs, so the JavaScript refers to controls by
    // the same strings the processor declares.
    juce::WebSliderRelay       timeRelay     { "time" };
    juce::WebSliderRelay       feedbackRelay { "feedback" };
    juce::WebSliderRelay       mixRelay      { "mix" };
    juce::WebSliderRelay       toneRelay     { "tone" };
    juce::WebToggleButtonRelay syncRelay     { "sync" };
    juce::WebComboBoxRelay     divisionRelay { "division" };

    juce::WebBrowserComponent webView;

    juce::WebSliderParameterAttachment       timeAttachment;
    juce::WebSliderParameterAttachment       feedbackAttachment;
    juce::WebSliderParameterAttachment       mixAttachment;
    juce::WebSliderParameterAttachment       toneAttachment;
    juce::WebToggleButtonParameterAttachment syncAttachment;
    juce::WebComboBoxParameterAttachment     divisionAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OrbitDelayAudioProcessorEditor)
};
