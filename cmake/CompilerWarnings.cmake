# glimmer_set_warnings(<target> STRICT|RELAXED [WERROR])
#
# STRICT  - used for core/ and tests/: catches implicit conversions, shadowing,
#           old-style casts and other common C++ footguns.
# RELAXED - used for app/: Qt's API uses int/qsizetype heavily, so the
#           conversion warnings would mostly be noise there.
function(glimmer_set_warnings target level)
  cmake_parse_arguments(ARG "WERROR" "" "" ${ARGN})

  if(MSVC)
    set(warnings /W4 /permissive-)
    if(ARG_WERROR)
      list(APPEND warnings /WX)
    endif()
  else()
    set(warnings
        -Wall
        -Wextra
        -Wpedantic
        -Wshadow
        -Wnon-virtual-dtor
        -Woverloaded-virtual
        -Wcast-align
        -Wformat=2
        -Wimplicit-fallthrough)
    if(level STREQUAL "STRICT")
      list(APPEND warnings
           -Wconversion
           -Wsign-conversion
           -Wold-style-cast
           -Wnull-dereference
           -Wdouble-promotion)
    endif()
    if(ARG_WERROR)
      list(APPEND warnings -Werror)
    endif()
  endif()

  target_compile_options(${target} PRIVATE ${warnings})
endfunction()
