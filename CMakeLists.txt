<<<<<<< HEAD
cmake_minimum_required(VERSION 3.10)
project(SandDunes)

set(CMAKE_CXX_STANDARD 14)

# Set Homebrew path for macOS ARM (if needed)
set(HOMEBREW_PREFIX "/opt/homebrew")

=======
cmake_minimum_required(VERSION 3.31)
project(Project)

set(CMAKE_CXX_STANDARD 20)

# Set Homebrew path for ARM Macs (M1/M2/M3 chips)
set(HOMEBREW_PREFIX "/opt/homebrew")

# Include directories
>>>>>>> def41c5eedef44d1d9c9b5b12da43760f6f99860
include_directories(
        ${HOMEBREW_PREFIX}/include
        ${HOMEBREW_PREFIX}/include/GL
)

<<<<<<< HEAD
=======
# Link directories
>>>>>>> def41c5eedef44d1d9c9b5b12da43760f6f99860
link_directories(
        ${HOMEBREW_PREFIX}/lib
)

<<<<<<< HEAD
add_executable(SandDunes main.cpp)


target_link_libraries(SandDunes
=======
add_executable(Project main.cpp)

# Link libraries
target_link_libraries(Project
>>>>>>> def41c5eedef44d1d9c9b5b12da43760f6f99860
        glfw
        GLEW
        "-framework OpenGL"
)
<<<<<<< HEAD
=======

>>>>>>> def41c5eedef44d1d9c9b5b12da43760f6f99860
