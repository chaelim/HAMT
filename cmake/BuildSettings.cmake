#
# Global flags
#

# Enable C++11
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
# C++14: set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++1y")


#
# Platfor specific flags
#
if(${WIN32})
    add_definitions(-DWIN32)
endif()

if(${MSVC})

    # Use good Visual C++ compiler options for benchmarking
    add_definitions(-D_SECURE_SCL=0 -D_HAS_EXCEPTIONS=0)
    set(CMAKE_C_FLAGS "/GS- /Zi" CACHE STRING "Common C compiler settings" FORCE)
    set(CMAKE_CXX_FLAGS "/GS- /Zi" CACHE STRING "Common C++ compiler settings" FORCE)
    set(CMAKE_C_FLAGS_DEBUG "/MTd /Od /D_DEBUG" CACHE STRING "Additional C compiler settings in Debug" FORCE)
    set(CMAKE_CXX_FLAGS_DEBUG "/MTd /Od /D_DEBUG" CACHE STRING "Additional C++ compiler settings in Debug" FORCE)
    set(CMAKE_C_FLAGS_RELEASE "/MT /O2 /Ob1 /DNDEBUG" CACHE STRING "Additional C compiler settings in Release" FORCE)
    set(CMAKE_CXX_FLAGS_RELEASE "/MT /O2 /Ob1 /DNDEBUG" CACHE STRING "Additional C++ compiler settings in Release" FORCE)
    set(CMAKE_EXE_LINKER_FLAGS "/DYNAMICBASE:NO /DEBUG /INCREMENTAL:NO" CACHE STRING "Common linker settings" FORCE)
    set(CMAKE_EXE_LINKER_FLAGS_DEBUG "" CACHE STRING "Additional linker settings in Debug" FORCE)
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "" CACHE STRING "Additional linker settings in Release" FORCE)

    # Enable debug info in Release.
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /debug")

    # Set compiler flags and options.
    # Here it is setting the Visual Studio warning level to 4
    # set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")

elseif(${UNIX})

    # These are all required on Xcode 4.5.1 + iOS, because the defaults are no good.
    set(CMAKE_C_FLAGS "-pthread -g")
    set(CMAKE_CXX_FLAGS "-pthread -g")
    set(CMAKE_C_FLAGS_DEBUG "-O0")
    set(CMAKE_CXX_FLAGS_DEBUG "-O0")
    set(CMAKE_C_FLAGS_RELEASE "-Os")
    set(CMAKE_CXX_FLAGS_RELEASE "-Os")

endif()

if(${IOS})
    set(CMAKE_XCODE_EFFECTIVE_PLATFORMS "-iphoneos;-iphonesimulator")
    set_target_properties(${PROJECT_NAME} PROPERTIES XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "iPhone Developer")
endif()

