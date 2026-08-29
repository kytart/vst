#include "PluginEditor.h"

#include "WebViewFiles.h"

namespace
{
    juce::String mimeTypeFor (const juce::String& path)
    {
        if (path.endsWith (".html")) return "text/html";
        if (path.endsWith (".js"))   return "text/javascript";
        if (path.endsWith (".css"))  return "text/css";
        if (path.endsWith (".svg"))  return "image/svg+xml";
        if (path.endsWith (".json")) return "application/json";
        if (path.endsWith (".woff2")) return "font/woff2";
        return "application/octet-stream";
    }

    juce::RangedAudioParameter& parameterFor (juce::AudioProcessorValueTreeState& apvts,
                                              const juce::String& id)
    {
        auto* param = apvts.getParameter (id);
        jassert (param != nullptr);
        return *param;
    }
}

OrbitDelayAudioProcessorEditor::OrbitDelayAudioProcessorEditor (OrbitDelayAudioProcessor& p)
    : AudioProcessorEditor (&p),
      webView (juce::WebBrowserComponent::Options {}
                   .withNativeIntegrationEnabled()
                   .withResourceProvider ([this] (const auto& url) { return getResource (url); },
                                          juce::URL { ORBIT_DEV_SERVER_ADDRESS }.getOrigin())
                   .withOptionsFrom (timeRelay)
                   .withOptionsFrom (feedbackRelay)
                   .withOptionsFrom (mixRelay)
                   .withOptionsFrom (toneRelay)
                   .withOptionsFrom (syncRelay)
                   .withOptionsFrom (divisionRelay)),
      timeAttachment     (parameterFor (p.apvts, "time"),     timeRelay),
      feedbackAttachment (parameterFor (p.apvts, "feedback"), feedbackRelay),
      mixAttachment      (parameterFor (p.apvts, "mix"),      mixRelay),
      toneAttachment     (parameterFor (p.apvts, "tone"),     toneRelay),
      syncAttachment     (parameterFor (p.apvts, "sync"),     syncRelay),
      divisionAttachment (parameterFor (p.apvts, "division"), divisionRelay)
{
    addAndMakeVisible (webView);

    webView.goToURL (juce::WebBrowserComponent::getResourceProviderRoot());

    setResizable (false, false);
    setSize (640, 420);
}

void OrbitDelayAudioProcessorEditor::resized()
{
    webView.setBounds (getLocalBounds());
}

std::optional<juce::WebBrowserComponent::Resource>
OrbitDelayAudioProcessorEditor::getResource (const juce::String& url) const
{
    const auto path = url == "/" ? juce::String ("index.html")
                                 : url.fromFirstOccurrenceOf ("/", false, false);

    juce::MemoryInputStream zipStream { webview_files::webview_files_zip,
                                        (size_t) webview_files::webview_files_zipSize,
                                        false };
    juce::ZipFile zip { zipStream };

    const auto* entry = zip.getEntry (juce::String (ZIPPED_FILES_PREFIX) + path);

    if (entry == nullptr)
        return std::nullopt;

    const std::unique_ptr<juce::InputStream> entryStream { zip.createStreamForEntry (*entry) };

    if (entryStream == nullptr)
        return std::nullopt;

    juce::MemoryBlock block;
    entryStream->readIntoMemoryBlock (block);

    std::vector<std::byte> data (block.getSize());
    std::memcpy (data.data(), block.getData(), block.getSize());

    return juce::WebBrowserComponent::Resource { std::move (data), mimeTypeFor (path) };
}
