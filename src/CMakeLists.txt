project(HAMT)

if (MSVC)
    add_definitions(-DHAMT_WIN)
endif()

include_directories(../include)
include(../cmake/BuildSettings.cmake)

file(GLOB SRCFILES *.cpp)
file(GLOB INCFILES ../include/*.h)
add_library (HAMT ${SRCFILES} ${INCFILES} )
