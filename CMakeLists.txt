cmake_minimum_required(VERSION 3.22)

# macOS 11+. Builds for the machine you're on by default; for a universal
# binary pass -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64". Set before project().
set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum macOS version")

project(Posy VERSION 0.3.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- JUCE ---------------------------------------------------------------------
# By default JUCE is downloaded on first configure. To use a local checkout:
#   cmake -B build -DPOSY_JUCE_DIR=/path/to/JUCE
set(POSY_JUCE_DIR "" CACHE PATH "Optional local JUCE checkout")

if(POSY_JUCE_DIR)
    add_subdirectory(${POSY_JUCE_DIR} JUCE)
else()
    include(FetchContent)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG        8.0.9
        GIT_SHALLOW    TRUE)
    FetchContent_MakeAvailable(JUCE)
endif()

# --- Plugin -------------------------------------------------------------------
# Copy the built plugin into the system VST3 folder after each build:
#   macOS   ~/Library/Audio/Plug-Ins/VST3
#   Linux   ~/.vst3
#   Windows C:\Program Files\Common Files\VST3  (needs an administrator shell,
#           so it is off by default there)
if(WIN32)
    set(_posy_copy_default OFF)
else()
    set(_posy_copy_default ON)
endif()
option(POSY_COPY_PLUGIN "Install the plugin into the system VST3 folder after building" ${_posy_copy_default})

# Posy only sends MIDI, but Live loads VST3 MIDI generators as instruments, so
# it's an instrument with a silent audio output.
juce_add_plugin(Posy
    COMPANY_NAME                "Taddei"
    BUNDLE_ID                   com.taddei.posy
    PLUGIN_MANUFACTURER_CODE    Tadd
    PLUGIN_CODE                 Posy
    PRODUCT_NAME                "Posy"
    DESCRIPTION                 "Chord generator"
    FORMATS                     VST3
    IS_SYNTH                    TRUE
    NEEDS_MIDI_INPUT            TRUE
    NEEDS_MIDI_OUTPUT           TRUE
    IS_MIDI_EFFECT              FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    VST3_CATEGORIES             Instrument Generator
    COPY_PLUGIN_AFTER_BUILD     ${POSY_COPY_PLUGIN})

set(POSY_ENGINE_SOURCES
    Source/Engine/Chords.cpp
    Source/Engine/ChordNamer.cpp
    Source/Engine/Player.cpp)

target_sources(Posy PRIVATE
    ${POSY_ENGINE_SOURCES}
    Source/Settings.cpp
    Source/Banks.cpp
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/UI/Widgets.cpp
    Source/UI/Panels.cpp)
target_include_directories(Posy PRIVATE Source)
target_compile_definitions(Posy PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)
target_link_libraries(Posy
    PRIVATE
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

# --- Engine tests (no JUCE needed) --------------------------------------------
enable_testing()
add_executable(PosyEngineTests Tests/EngineTests.cpp ${POSY_ENGINE_SOURCES})
target_include_directories(PosyEngineTests PRIVATE Source)
add_test(NAME engine COMMAND PosyEngineTests)

# --- Optional dev tool --------------------------------------------------------
#   PosySnapshot   render the panel to a PNG (headless)
option(POSY_BUILD_TOOLS "Build the snapshot tool" OFF)
if(POSY_BUILD_TOOLS)
    juce_add_console_app(PosySnapshot PRODUCT_NAME "PosySnapshot")
    target_sources(PosySnapshot PRIVATE
        Tools/Snapshot.cpp
        ${POSY_ENGINE_SOURCES}
        Source/Settings.cpp
        Source/Banks.cpp
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
        Source/UI/Widgets.cpp
        Source/UI/Panels.cpp)
    target_include_directories(PosySnapshot PRIVATE Source)
    target_compile_definitions(PosySnapshot PRIVATE
        JucePlugin_Name="Posy"
        JUCE_MODAL_LOOPS_PERMITTED=1
        JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
    target_link_libraries(PosySnapshot PRIVATE
        juce::juce_audio_utils
        juce::juce_recommended_config_flags)
endif()
