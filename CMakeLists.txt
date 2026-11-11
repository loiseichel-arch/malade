cmake_minimum_required (VERSION 3.22)

project (Vibrato VERSION 0.1.0 LANGUAGES C CXX)

set (CMAKE_CXX_STANDARD 17)
set (CMAKE_CXX_STANDARD_REQUIRED ON)

# Universal binary (Apple Silicon + Intel), macOS 11 and later.
set (CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "")
set (CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "")

# ---- JUCE ------------------------------------------------------------------
# Either point JUCE_PATH to a JUCE folder you already have, or leave it empty
# and CMake downloads JUCE (it includes the AAX SDK, nothing else to fetch).
set (JUCE_PATH "" CACHE PATH "Local JUCE folder (empty = download)")

if (JUCE_PATH)
    add_subdirectory ("${JUCE_PATH}" JUCE)
else()
    include (FetchContent)
    FetchContent_Declare (JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG        8.0.11
        GIT_SHALLOW    ON)
    FetchContent_MakeAvailable (JUCE)
endif()

# ---- Plugin ----------------------------------------------------------------
# Change COMPANY_NAME, the manufacturer code and the bundle id to your own
# before you sign and distribute the plugin.
juce_add_plugin (Vibrato
    COMPANY_NAME              "MyCompany"
    BUNDLE_ID                 "com.mycompany.vibrato"
    PLUGIN_MANUFACTURER_CODE  Mycp
    PLUGIN_CODE               Vbr1
    PRODUCT_NAME              "Vibrato"
    FORMATS                   AAX AU VST3 Standalone
    IS_SYNTH                  FALSE
    NEEDS_MIDI_INPUT          FALSE
    NEEDS_MIDI_OUTPUT         FALSE
    IS_MIDI_EFFECT            FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    AAX_IDENTIFIER            com.mycompany.vibrato
    AAX_CATEGORY              AAX_ePlugInCategory_Modulation
    AU_MAIN_TYPE              kAudioUnitType_Effect
    VST3_CATEGORIES           Fx Modulation
    COPY_PLUGIN_AFTER_BUILD   FALSE)

target_sources (Vibrato PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/ui/SketchLookAndFeel.cpp)

target_include_directories (Vibrato PRIVATE Source)

# Optional hand-written fonts: drop .ttf/.otf files in Assets/Fonts
# (e.g. Patrick Hand and Caveat from Google Fonts). Without them the plugin
# uses the macOS fonts Chalkboard SE and Marker Felt.
file (GLOB VIBRATO_FONTS CONFIGURE_DEPENDS
      "${CMAKE_CURRENT_SOURCE_DIR}/Assets/Fonts/*.ttf"
      "${CMAKE_CURRENT_SOURCE_DIR}/Assets/Fonts/*.otf")
if (VIBRATO_FONTS)
    juce_add_binary_data (VibratoAssets SOURCES ${VIBRATO_FONTS})
    target_link_libraries (Vibrato PRIVATE VibratoAssets)
    target_compile_definitions (Vibrato PRIVATE VIBRATO_HAS_FONTS=1)
endif()

target_compile_definitions (Vibrato PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)

target_link_libraries (Vibrato
    PRIVATE
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
