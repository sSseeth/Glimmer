find_package(Qt6 6.4 REQUIRED COMPONENTS Widgets Multimedia Network Concurrent)
qt_standard_project_setup()  # AUTOMOC/AUTOUIC + sensible defaults

qt_add_executable(
  glimmer
  src/main.cpp
  src/ai_panel.cpp
  src/ai_panel.hpp
  src/collage_editor.cpp
  src/collage_editor.hpp
  src/diary_store.cpp
  src/diary_store.hpp
  src/entry_editor.cpp
  src/entry_editor.hpp
  src/main_window.cpp
  src/main_window.hpp
  src/omniroute_client.cpp
  src/omniroute_client.hpp
  src/qt_convert.hpp
  src/settings.cpp
  src/settings.hpp
  src/text_editor.cpp
  src/text_editor.hpp
  src/voice_editor.cpp
  src/voice_editor.hpp)

target_link_libraries(
  glimmer
  PRIVATE glimmer::core
          Qt6::Widgets
          Qt6::Multimedia
          Qt6::Network
          Qt6::Concurrent)

target_compile_definitions(glimmer PRIVATE GLIMMER_VERSION="${PROJECT_VERSION}")

set_target_properties(glimmer PROPERTIES WIN32_EXECUTABLE ON MACOSX_BUNDLE ON)

if(GLIMMER_APP_WARNINGS_AS_ERRORS)
  glimmer_set_warnings(glimmer RELAXED WERROR)
else()
  glimmer_set_warnings(glimmer RELAXED)
endif()
glimmer_enable_sanitizers(glimmer)

install(
  TARGETS glimmer
  BUNDLE DESTINATION .
  RUNTIME DESTINATION bin)
