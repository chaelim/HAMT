cmake_minimum_required(VERSION 3.2)

# Maps to a solution file (Tutorial.sln). The solution will 
# have all targets (exe, lib, dll) as projects (.vcproj)
project(HAMT)

# Turn on the ability to create folders to organize projects (.vcproj)
# It creates "CMakePredefinedTargets" folder by default and adds CMake
# defined projects like INSTALL.vcproj and ZERO_CHECK.vcproj
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Turn on CMake testing capabilities
enable_testing()

# Sub-directories where more CMakeLists.txt exist
add_subdirectory(Test)
