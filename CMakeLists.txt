cmake_minimum_required(VERSION 3.25)

project(
  glimmer
  VERSION 0.1.0
  DESCRIPTION "A small, private desktop diary"
  HOMEPAGE_URL "https://github.com/sSseeth/Glimmer"
  LANGUAGES C CXX)

# --- Language settings -------------------------------------------------------
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
# clangd (Neovim LSP) reads this file to understand include paths and flags.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

# --- Options -----------------------------------------------------------------
option(GLIMMER_BUILD_APP "Build the Qt 6 desktop application" ON)
option(GLIMMER_BUILD_TESTS "Build the unit tests" ON)
option(GLIMMER_WITH_WHISPER "Enable local voice transcription via whisper.cpp" OFF)
option(GLIMMER_ENABLE_SANITIZERS "Build with AddressSanitizer + UndefinedBehaviorSanitizer" OFF)
option(GLIMMER_WARNINGS_AS_ERRORS "Treat compiler warnings as errors in core/ and tests/" ON)
option(GLIMMER_APP_WARNINGS_AS_ERRORS "Treat compiler warnings as errors in app/" OFF)

# MSVC: sources are UTF-8 (the UI uses symbols like ✎ ♪ ▦), and use the
# standard-conforming preprocessor (the legacy one mangles raw string
# literals passed to macros, e.g. in the GoogleTest assertions).
add_compile_options($<$<CXX_COMPILER_ID:MSVC>:/utf-8> $<$<CXX_COMPILER_ID:MSVC>:/Zc:preprocessor>)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(CompilerWarnings)
include(Sanitizers)
include(Dependencies)

add_subdirectory(core)

if(GLIMMER_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()

if(GLIMMER_BUILD_APP)
  add_subdirectory(app)
endif()
