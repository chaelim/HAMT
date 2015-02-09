cmake_minimum_required(VERSION 2.8.6)

# Default build-time configuration options
# Can be modified via CMake GUI or via CMake command line
option(HAMT_TEST_USE_DLMALLOC "Use DLMalloc instead of the default C runtime platform malloc" ON)
#option(WIN64 "Default generate x64" ON)

# Write build-time configuration options to a header file
configure_file(config.h.in config.h)
include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(../include)
include(../cmake/BuildSettings.cmake)

# Create project and .exe
set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE INTERNAL "limited configs")
project(HAMTTest)
file(GLOB SRCFILES *.cpp)
file(GLOB INCFILES *.h)
if (HAMT_TEST_USE_DLMALLOC)
    list(APPEND SRCFILES dlmalloc/malloc.c)
endif()
add_executable(${PROJECT_NAME}  ${SRCFILES} ${INCFILES} config.h.in ${CMAKE_CURRENT_BINARY_DIR}/config.h)

include(../cmake/BuildSettings.cmake)

# Add subdirectories with dependencies before dependents so that each subdirectory
# inherits the include paths gathered from the dependencies before it.
add_subdirectory(../src hamt)

# Add libraries with dependencies after dependents to satisfy ld linker.
target_link_libraries(${PROJECT_NAME} HAMT)
