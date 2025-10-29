cmake_minimum_required(VERSION 3.30)

# At the top of the file, after setting the C++ standard:
file(GLOB_RECURSE GBLATOR_SOURCES
     ${CMAKE_SOURCE_DIR}/src/*.cpp)

# Exclude main.cpp from the library sources
list(FILTER GBLATOR_SOURCES EXCLUDE REGEX ".*/main\\.cpp$")

# Build the core emulator library
add_library(gblator_lib ${GBLATOR_SOURCES})
target_include_directories(gblator_lib PUBLIC ${CMAKE_SOURCE_DIR}/include)

# Build the emulator executable from the library plus main.cpp
add_executable(gblator src/main.cpp)
target_link_libraries(gblator PRIVATE gblator_lib)

# Build tests: link the library and your test file(s)
file(GLOB GBLATOR_TEST_SOURCES ${CMAKE_SOURCE_DIR}/tests/test_*.cpp)
if(GBLATOR_TEST_SOURCES)
    add_executable(gblator_tests ${GBLATOR_TEST_SOURCES})
    target_link_libraries(gblator_tests PRIVATE gblator_lib)
    enable_testing()
    add_test(NAME gblator_unit_tests COMMAND gblator_tests)
endif()
