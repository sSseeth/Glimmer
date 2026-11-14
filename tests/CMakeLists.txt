add_executable(
  glimmer_tests
  ai_test.cpp
  ai_note_repository_test.cpp
  database_test.cpp
  date_test.cpp
  entry_repository_test.cpp
  json_test.cpp
  migrations_test.cpp)

target_link_libraries(glimmer_tests PRIVATE glimmer::core GTest::gtest_main)

if(GLIMMER_WARNINGS_AS_ERRORS)
  glimmer_set_warnings(glimmer_tests STRICT WERROR)
else()
  glimmer_set_warnings(glimmer_tests STRICT)
endif()
glimmer_enable_sanitizers(glimmer_tests)

include(GoogleTest)
gtest_discover_tests(glimmer_tests DISCOVERY_MODE PRE_TEST)
