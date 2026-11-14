# Third-party dependencies.
#
# SYSTEM marks fetched headers as system headers, so our strict warnings
# (-Werror) don't fire on third-party code.
#
# Policy (see docs/adr/0003-dependencies.md): prefer the system package, fall
# back to a pinned FetchContent download. Every pinned version lives in this
# file so upgrades are one reviewable diff.
include(FetchContent)

# --- SQLite (required) -------------------------------------------------------
find_package(SQLite3 REQUIRED)

# --- GoogleTest (tests only) -------------------------------------------------
if(GLIMMER_BUILD_TESTS)
  find_package(GTest CONFIG QUIET)
  if(NOT GTest_FOUND)
    message(STATUS "GoogleTest not found on the system; fetching v1.15.2")
    FetchContent_Declare(
      googletest
      URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.tar.gz
      DOWNLOAD_EXTRACT_TIMESTAMP TRUE
      SYSTEM)
    set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
  endif()
endif()

# --- whisper.cpp (optional, local speech-to-text) ----------------------------
if(GLIMMER_WITH_WHISPER)
  find_package(whisper CONFIG QUIET)
  if(NOT whisper_FOUND)
    message(STATUS "whisper.cpp not found on the system; fetching v1.7.5")
    FetchContent_Declare(
      whisper
      GIT_REPOSITORY https://github.com/ggml-org/whisper.cpp.git
      GIT_TAG v1.7.5
      GIT_SHALLOW TRUE
      SYSTEM)
    set(WHISPER_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(WHISPER_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(WHISPER_BUILD_SERVER OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(whisper)
  endif()
endif()
