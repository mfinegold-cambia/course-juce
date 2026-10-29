
# When you hit Build button in Clion, it calls CMake which calls this CMakeLists.txt file
# This CMakeLists.txt file tells CMake how to compile the code, link the libraries, and build the Executable

# Mandatory line, sets the minimum version of CMake that should be used with this repository.
# 3.22 is the minimum CMake version required by JUCE.
# 3.25 is required for the MSVC_DEBUG_INFORMATION_FORMAT feature to work.
# To verify your version run
# $ cmake --version
# If you don't have at least this version of Cmake installed, Cmake will throw an error and stop
cmake_minimum_required(VERSION 3.25 FATAL_ERROR)

# Enable setting CMAKE_MSVC_DEBUG_INFORMATION_FORMAT
# This is used when using Microsoft Windows C++ compiler
set(CMAKE_POLICY_DEFAULT_CMP0141        NEW      CACHE STRING "" FORCE)

# Sets the minimum deployment target
# macOS 10.11 is the minimum required by JUCE
#  This says that you need a minimum of Macos 10.11 to run this app
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.11" CACHE STRING "" FORCE)

# Builds a binary that works on Apple Silicon (arm64) and Intel-based Macs
#  This says for Macos that the target will be Mac Intel or Mac Silicon
set(CMAKE_OSX_ARCHITECTURES "x86_64;arm64" CACHE STRING "" FORCE)

# Sets a few variables, like PROJECT_NAME.
# The project here populates "JucePluginDevelopmentCourseTremoloPlugin" value into the $PROJECT_NAME and $CMAKE_PROJECT_NAME vars
# Clion uses this name as its Project name when you Open Recents
project(TodoJucePluginDevelopmentCourseTremoloPlugin VERSION 1.0.0)

# Set the C++ standard only if this project is standalone;
# otherwise, let clients decide.
# This should be true since we are declaring the "project" in this file
if(PROJECT_IS_TOP_LEVEL)
  # CMAKE_CXX_STANDARD value will be passed as a compiler flag to your compiler telling it which C++ version it is targeting
  if (NOT CMAKE_CXX_STANDARD)
    # Always use the newest C++ standard on green-field projects if possible.
    set(CMAKE_CXX_STANDARD 23)
  endif()

  # These are used for Microsoft C++ compiler
  # Links to C++ runtime statically
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
  # Embeds debug information
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<$<CONFIG:Debug,RelWithDebInfo>:Embedded>" CACHE STRING "" FORCE)
endif()

# I like to download the dependencies to the same folder as the project.
# If you want to install them system-wide, set CPM_SOURCE_CACHE with a path to the dependencies
# either as an environment variable or pass it with -DCPM_SOURCE_CACHE=<path> when invoking cmake.
# This creates a LIB_DIR var and sets it to  ./libs dir  ( CMAKE_CURRENT_SOURCE_DIR is just current dir here )
set(LIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/libs)

# Downloads CPM if not already downloaded. CPM is an easy-to-use package manager nicely integrated with CMake.
# This downloads the CPM package manager which we use to download project dependencies like JUCE
include(cmake/cpm.cmake)

# This commands downloads AND configures JUCE. It sets up some variables, like JUCE_SOURCE_DIR.
# This downloads the CPM package manager so we can download any dependencies are project needs like JUCE
# Here we download JUCE into our /lib/juce folder
cpmaddpackage(
  NAME
    JUCE
  GIT_TAG
    8.0.12
  GITHUB_REPOSITORY
    juce-framework/JUCE
  SOURCE_DIR
    ${LIB_DIR}/juce
)

# Adds compiler warning utilities & other.
# This
include(cmake/CompilerWarnings.cmake)
include(cmake/Util.cmake)

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
  # Sets the Windows VST3 installation location to one
  # that doesn't require elevated permissions
  set_directory_properties(PROPERTIES JUCE_VST3_COPY_DIR "$ENV{LOCALAPPDATA}/Programs/Common/VST3")
endif()

# This tells JUCE all the info about your Plugin
# This will be the name of your Artefact folder (called a Bundle) binary that will be deployed to  ~/Library/Audio/Plug-Ins/VST3   on Macos
# Adds a plugin target.
# NOTE that if COPY_PLUGIN_AFTER_BUILD = TRUE and you've made changes, running a Build will automatically copy your VST3 Artefact bundler/folder to   ~/Library/Audio/Plug-Ins/VST3  on Macos in your Home dir
set(TREMOLO_PLUGIN_NAME "mitchTremoloPlugin")
juce_add_plugin(
  TremoloCoursePlugin
  PRODUCT_NAME
    ${TREMOLO_PLUGIN_NAME}
  VERSION
    0.0.0
  COMPANY_NAME
    MitchCompany
  PLUGIN_MANUFACTURER_CODE
    Mtch
  PLUGIN_CODE
    Mtpg
  FORMATS
    Standalone
    AU
    VST3
  IS_SYNTH
    FALSE
  NEEDS_MIDI_INPUT
    FALSE
  NEEDS_MIDI_OUTPUT
    FALSE
  VST3_AUTO_MANIFEST
    FALSE
  COPY_PLUGIN_AFTER_BUILD
    TRUE
)

# Configures the JUCE sources included in the plugin Target.
# Here you say TremoloCoursePlugin will be the name of the build Target folder in the cmake-build-debug folder - this will have subfolders for AU, Standalone, and VST artefacts
target_compile_definitions(TremoloCoursePlugin
  PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)

juce_add_binary_data(tremolo_plugin_assets
  HEADER_NAME
    "TremoloPluginAssets.h"
  NAMESPACE
    tremolo::assets
  SOURCES
    assets/Background.png
    assets/Logo.png
    # TODO: Add custom font files
)

add_library(tremolo::tremolo_plugin_assets ALIAS tremolo_plugin_assets)

# Registers the juce custom Module
# Creates a CMake target named after the folder
#  The name "tremolo_plugin" means that the enclosing folder is called "tremolo_plugin" with a header file in it named "tremolo_plugin.h"
juce_add_module(tremolo_plugin ALIAS_NAMESPACE tremolo)

# Additional definitions required by all users of this module.
# Adding the TREMOLO_PLUGIN_NAME var to the target_compile_definitions makes the var accessible in your C++ application code ( added as a Preprocessor definition )
target_compile_definitions(tremolo_plugin
  INTERFACE
    TREMOLO_PLUGIN_NAME="${TREMOLO_PLUGIN_NAME}"
)

# Enables strict C++ warnings.
target_compile_options(tremolo_plugin INTERFACE "${PROJECT_WARNINGS_CXX}")

target_link_libraries(tremolo_plugin INTERFACE tremolo::tremolo_plugin_assets)

#  This says which libs should be linked to the build Target called TremoloCoursePlugin ( referenced in target_compile_definitions )
#  This says all the Includes in tremolo_plugin.h should be Linked to the build Target called TremoloCoursePlugin
#  It tells the Linker which libs should be linked to your build Target when building the Executable file
target_link_libraries(
  TremoloCoursePlugin
  PRIVATE
    tremolo::tremolo_plugin
  PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)



# In Visual Studio this command provides a nice grouping of source files in "filters".
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/..)

option(BUILD_TESTS "When on, will download the GoogleTest framework and add the automated tests project" OFF)
if(BUILD_TESTS)
  # This command allows running tests from the "build" folder (the one where CMake generates the project to).
  enable_testing()

  # Adds all the targets configured in the "test" folder.
  add_subdirectory(test)
endif()


# Tell CMake to Build the juce AudioPluginHost executable as part of our Tremolo build
# It puts the AudioPluginHost  Bundle at /Documents/course-juce/todo/cmake-build-debug/_deps/juce-build/extras/AudioPluginHost/AudioPluginHost_artefacts/Debug/AudioPluginHost.app/Contents/MacOS
# In the Run/Debug edit configuration, in Executable dropdown, we select AudioPluginHost so that it runs everytime our Tremolo project is Run
set(JUCE_BUILD_EXTRAS ON CACHE INTERNAL "")
