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

# In the future you may choose to break the emulator into libraries
# and link them here.