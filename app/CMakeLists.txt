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

qt_add_resources(glimmer "icons" PREFIX "/" FILES resources/glimmer.png)

# --- Per-platform packaging metadata -------------------------------------------
set_target_properties(glimmer PROPERTIES WIN32_EXECUTABLE ON MACOSX_BUNDLE ON)

if(WIN32)
  target_sources(glimmer PRIVATE resources/glimmer.rc)  # .exe icon
endif()

if(APPLE)
  set(icns "${CMAKE_CURRENT_SOURCE_DIR}/resources/glimmer.icns")
  target_sources(glimmer PRIVATE "${icns}")
  set_source_files_properties("${icns}" PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
  set_target_properties(
    glimmer
    PROPERTIES OUTPUT_NAME Glimmer  # -> Glimmer.app
               MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/resources/Info.plist.in"
               MACOSX_BUNDLE_BUNDLE_NAME Glimmer
               MACOSX_BUNDLE_GUI_IDENTIFIER io.github.ssseeth.glimmer
               MACOSX_BUNDLE_ICON_FILE glimmer.icns
               MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
               MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}")
endif()

if(GLIMMER_APP_WARNINGS_AS_ERRORS)
  glimmer_set_warnings(glimmer RELAXED WERROR)
else()
  glimmer_set_warnings(glimmer RELAXED)
endif()
glimmer_enable_sanitizers(glimmer)

# Packaging installs only COMPONENT app (not third-party headers/libs).
install(
  TARGETS glimmer
  BUNDLE DESTINATION . COMPONENT app
  RUNTIME DESTINATION bin COMPONENT app)

if(UNIX AND NOT APPLE)
  # Freedesktop integration: app menus, launchers and the AppImage use these.
  install(
    FILES resources/glimmer.desktop
    DESTINATION share/applications
    COMPONENT app)
  install(
    FILES resources/glimmer.png
    DESTINATION share/icons/hicolor/512x512/apps
    COMPONENT app)
endif()
