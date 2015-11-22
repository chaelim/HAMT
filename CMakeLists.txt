cmake_minimum_required(VERSION 3.2)

# Maps to a solution file (Tutorial.sln). The solution will 
# have all targets (exe, lib, dll) as projects (.vcproj)
project(HAMT)

# Turn on the ability to create folders to organize projects (.vcproj)
# It creates "CMakePredefinedTargets" folder by default and adds CMake
# defined projects like INSTALL.vcproj and ZERO_CHECK.vcproj
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Sub-directories where more CMakeLists.txt exist
add_subdirectory(Test)

# Turn on CMake testing capabilities
#enable_testing()

# Add test cases
#add_test(AppTest1 ${PROJECT_BINARY_DIR}/bin/app.exe 100)
#add_test(AppTest2 ${PROJECT_BINARY_DIR}/bin/app.exe 200)
#add_test(AppTest3 ${PROJECT_BINARY_DIR}/bin/app.exe 300)
