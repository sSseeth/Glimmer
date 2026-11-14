add_library(
  glimmer_core STATIC
  src/database.cpp
  src/date.cpp
  src/entry.cpp
  src/migrations.cpp)
add_library(glimmer::core ALIAS glimmer_core)

target_include_directories(glimmer_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(glimmer_core PRIVATE SQLite::SQLite3)
target_compile_features(glimmer_core PUBLIC cxx_std_20)


if(GLIMMER_WARNINGS_AS_ERRORS)
  glimmer_set_warnings(glimmer_core STRICT WERROR)
else()
  glimmer_set_warnings(glimmer_core STRICT)
endif()
glimmer_enable_sanitizers(glimmer_core)
