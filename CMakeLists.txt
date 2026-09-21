cmake_minimum_required(VERSION 3.22)

project(Auralis VERSION 0.2.0 LANGUAGES CXX)

if(NOT WIN32)
    message(FATAL_ERROR "This initial application targets Windows only.")
endif()

include(FetchContent)
# A reviewed, immutable JUCE revision. Use the local checkout when available.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/JUCE/CMakeLists.txt")
    add_subdirectory(external/JUCE EXCLUDE_FROM_ALL)
else()
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 29396c22c93392d6738e021b83196283d6e4d850)
    FetchContent_MakeAvailable(JUCE)
endif()

juce_add_binary_data(AuralisAssets SOURCES assets/library.json assets/auralis-icon.png)
juce_add_gui_app(Auralis PRODUCT_NAME "Auralis" COMPANY_NAME "Auralis" VERSION ${PROJECT_VERSION}
    ICON_BIG assets/auralis-icon.png ICON_SMALL assets/auralis-icon.png)
target_sources(Auralis PRIVATE src/main.cpp src/ui/Theme.cpp src/ui/Browser.cpp
    src/ui/Transport.cpp src/ui/Arrangement.cpp src/ui/Workspace.cpp src/ui/WaveformPreview.cpp
    src/ui/PianoRoll.cpp src/ui/ClipTimeline.cpp src/ui/Mixer.cpp src/ui/MixerControls.cpp src/platform/ProcessMetrics.cpp)
target_include_directories(Auralis PRIVATE src)
target_compile_features(Auralis PRIVATE cxx_std_20)
target_compile_definitions(Auralis PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0
    NOMINMAX WIN32_LEAN_AND_MEAN)
target_link_libraries(Auralis PRIVATE AuralisAssets juce::juce_gui_basics juce::juce_audio_formats psapi)

if(MSVC)
    target_compile_options(Auralis PRIVATE /W4 /permissive- /utf-8)
endif()

include(CTest)
if(BUILD_TESTING)
    add_executable(AuralisStateTests tests/StateTests.cpp)
    target_include_directories(AuralisStateTests PRIVATE src)
    target_compile_features(AuralisStateTests PRIVATE cxx_std_20)
    add_test(NAME AuralisStateTests COMMAND AuralisStateTests)
    add_executable(AuralisMidiTests tests/MidiTests.cpp)
    target_include_directories(AuralisMidiTests PRIVATE src)
    target_compile_features(AuralisMidiTests PRIVATE cxx_std_20)
    add_test(NAME AuralisMidiTests COMMAND AuralisMidiTests)
    add_executable(AuralisMixerTests tests/MixerStateTests.cpp)
    target_include_directories(AuralisMixerTests PRIVATE src)
    target_compile_features(AuralisMixerTests PRIVATE cxx_std_20)
    add_test(NAME AuralisMixerTests COMMAND AuralisMixerTests)
endif()
