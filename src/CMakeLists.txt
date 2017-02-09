project(HAMT)

if (MSVC)
    add_definitions(-DHAMT_WIN)
endif()

include_directories(../include)
include(../cmake/BuildSettings.cmake)

file(GLOB SRCFILES *.cpp)
file(GLOB INCFILES ../include/*.h)
add_library (HAMT ${SRCFILES} ${INCFILES} )

set_target_properties(${PROJECT_NAME} PROPERTIES
    CXX_STANDARD 11
    CXX_STANDARD_REQUIRED ON
    COMPILE_DEFINITIONS "$<$<CXX_COMPILER_ID:MSVC>:_SCL_SECURE_NO_WARNINGS>"
    COMPILE_OPTIONS "$<$<CXX_COMPILER_ID:MSVC>:/EHsc>"
)
