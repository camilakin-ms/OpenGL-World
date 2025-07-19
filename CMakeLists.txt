cmake_minimum_required(VERSION 3.31)
project(Project)

set(CMAKE_CXX_STANDARD 20)

# Set Homebrew path for ARM Macs (M1/M2/M3 chips)
set(HOMEBREW_PREFIX "/opt/homebrew")

# Include directories
include_directories(
        ${HOMEBREW_PREFIX}/include
        ${HOMEBREW_PREFIX}/include/GL
)

# Link directories
link_directories(
        ${HOMEBREW_PREFIX}/lib
)

add_executable(Project main.cpp)

# Link libraries
target_link_libraries(Project
        glfw
        GLEW
        "-framework OpenGL"
)

