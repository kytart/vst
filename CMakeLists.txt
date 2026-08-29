cmake_minimum_required(VERSION 3.22)

project(PalmMutingPlugins VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Apple Silicon only. Universal binaries are a distribution concern, not a local one.
set(CMAKE_OSX_ARCHITECTURES arm64)

# JUCE is added once, here. Its modules are the bulk of the build time, and
# doing this per plugin would recompile all of JUCE for every plugin.
add_subdirectory(libs/JUCE)

# Where plugins find JUCE's bundled JavaScript frontend library. Set here so a
# plugin's CMakeLists never has to reach up out of its own directory.
set(JUCE_MODULES_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/libs/JUCE/modules" CACHE INTERNAL "")

# One line per plugin.
add_subdirectory(plugins/orbit-delay)
