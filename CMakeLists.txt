cmake_minimum_required(VERSION 3.12)
project(GBLator LANGUAGES CXX)

# Use C++17 for this project
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add include directory so that headers can be found when building
include_directories(${CMAKE_SOURCE_DIR}/include)

# Gather all source files recursively
file(GLOB_RECURSE GBLATOR_SOURCES
     ${CMAKE_SOURCE_DIR}/src/*.cpp)

# Define the executable target
add_executable(gblator ${GBLATOR_SOURCES})

# Build tests
file(GLOB GBLATOR_TEST_SOURCES
     ${CMAKE_SOURCE_DIR}/tests/test_*.cpp)
if(GBLATOR_TEST_SOURCES)
    add_executable(gblator_tests ${GBLATOR_SOURCES} ${GBLATOR_TEST_SOURCES})
    # Enable CTest if available
    enable_testing()
    add_test(NAME gblator_unit_tests COMMAND gblator_tests)
endif()

# In the future you may choose to break the emulator into libraries
# and link them here.