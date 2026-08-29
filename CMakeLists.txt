cmake_minimum_required(VERSION 3.22)

project(OrbitDelay VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Apple Silicon only. Universal binaries are a distribution concern, not a local one.
set(CMAKE_OSX_ARCHITECTURES arm64)

add_subdirectory(libs/JUCE)

juce_add_plugin(OrbitDelay
    COMPANY_NAME "Palm Muting"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE       # installs to ~/Library/Audio/Plug-Ins/VST3 on every build
    BUNDLE_ID com.palmmuting.orbitdelay
    PLUGIN_MANUFACTURER_CODE Palm
    PLUGIN_CODE Dly1
    NEEDS_WEB_BROWSER TRUE
    FORMATS VST3 Standalone
    PRODUCT_NAME "Orbit Delay")

target_sources(OrbitDelay PRIVATE
    src/PluginProcessor.cpp
    src/PluginEditor.cpp)

# ---------------------------------------------------------------------------
# Web UI assets
#
# JUCE's JavaScript frontend library is copied in beside our own files, the
# whole ui/public tree is zipped, and the zip is embedded as binary data. The
# editor serves files out of that zip at runtime, so the shipped plugin is
# self-contained with no external assets.
# ---------------------------------------------------------------------------
set(WEBVIEW_FILES_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/ui/public")

file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/libs/JUCE/modules/juce_gui_extra/native/javascript/"
     DESTINATION "${WEBVIEW_FILES_SOURCE_DIR}/js/juce/")

set(WEBVIEW_FILES_ZIP "${CMAKE_BINARY_DIR}/webview_files.zip")

add_custom_target(ZipWebViewFiles
    COMMAND ${CMAKE_COMMAND} -E tar cf "${WEBVIEW_FILES_ZIP}" --format=zip "public"
    BYPRODUCTS "${WEBVIEW_FILES_ZIP}"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/ui"
    COMMENT "Zipping web UI files"
    VERBATIM)

juce_add_binary_data(WebViewFiles
    HEADER_NAME WebViewFiles.h
    NAMESPACE webview_files
    SOURCES "${WEBVIEW_FILES_ZIP}")

add_dependencies(WebViewFiles ZipWebViewFiles)

target_compile_definitions(OrbitDelay
    PUBLIC
        JUCE_WEB_BROWSER=1
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
    PRIVATE
        # Prefix every zip entry carries, and the origin allowed to serve the UI
        # during design work so CSS changes don't need a C++ rebuild.
        ZIPPED_FILES_PREFIX="public/"
        ORBIT_DEV_SERVER_ADDRESS="http://localhost:5173")

target_link_libraries(OrbitDelay
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
        juce::juce_gui_extra
        WebViewFiles
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
