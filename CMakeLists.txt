cmake_minimum_required(VERSION 3.31)
project(SandDunes)

set(CMAKE_CXX_STANDARD 20)

# Set Homebrew path for macOS ARM (if needed)
set(HOMEBREW_PREFIX "/opt/homebrew")

include_directories(
        ${HOMEBREW_PREFIX}/include
        ${HOMEBREW_PREFIX}/include/GL
)

link_directories(
        ${HOMEBREW_PREFIX}/lib
)

add_executable(SandDunes main.cpp)


target_link_libraries(SandDunes
        glfw
        GLEW
        "-framework OpenGL"
)
